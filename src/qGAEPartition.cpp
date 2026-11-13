//##########################################################################
//#                                                                        #
//#                CLOUDCOMPARE PLUGIN: qGAE                              #
//#                                                                        #
//#  This program is free software; you can redistribute it and/or modify  #
//#  it under the terms of the GNU General Public License as published by  #
//#  the Free Software Foundation; version 2 of the License.               #
//#                                                                        #
//#  This program is distributed in the hope that it will be useful,       #
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of        #
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
//#  GNU General Public License for more details.                          #
//#                                                                        #
//#                             COPYRIGHT: Ioannis Farmakis                #
//#                                                                        #
//##########################################################################

// local
#include "qGAEPartition.h"
#include "qGAELevel.h"

// system
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <omp.h>

// qPCP
#include "OMPNumThreads.h"

//CCCoreLib
#include <CloudSamplingTools.h>
#include <GenericProgressCallback.h>
#include <ReferenceCloud.h>


VoxelPartition::VoxelPartition(double voxelSize, int connectivity)
    : m_voxelSize(voxelSize)
    , m_connectivity(connectivity)
{
}

Data* VoxelPartition::Process(ccPointCloud* cloud, QString& errorMessage, CCCoreLib::GenericProgressCallback* progressCb)
{
    ccOctree::Shared theOctree = cloud->getOctree();
    if (!theOctree)
    {
        theOctree = cloud->computeOctree(progressCb);
        if (!theOctree)
        {
            errorMessage = QObject::tr("[GAE] Couldn't compute octree for cloud '%1'!").arg(cloud->getName());
            return nullptr;
        }
    }

    auto subsampled = CCCoreLib::CloudSamplingTools::resampleCloudSpatially(cloud,
                                                                            static_cast<PointCoordinateType>(m_voxelSize),
                                                                            CCCoreLib::CloudSamplingTools::SFModulationParams(false),
                                                                            theOctree.data(),
                                                                            progressCb);

    if (!subsampled)
    {
        errorMessage = QObject::tr("[GAE] Failed to sample voxel cloud '%1'!").arg(cloud->getName());
        return nullptr;
    }

    ccPointCloud* pc = cloud->partialClone(subsampled);
    delete subsampled;
    subsampled = nullptr;
    if (!pc)
    {
        errorMessage = QObject::tr("[GAE] Not enough memory to clone voxel cloud '%1'!").arg(cloud->getName());
        return nullptr;
    }

    Data* data = new Data(*pc);
    delete pc;
    pc = nullptr;
    if (!data->isValid())
    {
        errorMessage = QObject::tr("[GAE] Failed to create Level-0 data from voxel cloud '%1'!").arg(cloud->getName());
        return nullptr;
    }

    ccOctree::Shared dataOctree = data->getOctree();
    if (!dataOctree)
    {
        dataOctree = data->computeOctree(progressCb);
        if (!dataOctree)
        {
            errorMessage = QObject::tr("[GAE] Couldn't compute octree for Level-0 data '%1'!").arg(cloud->getName());
            return nullptr;
        }
    }

    // instantiate the graph and compute edges
	PCP::Graph G(static_cast<int32_t>(data->size()), data, dataOctree.data());
	G.computeEdges(m_connectivity, m_voxelSize, progressCb);
    data->edges = G.getEdgeList();
    data->edgeAttr = G.getEdgeWeights();
    data->nodeSize.resize(data->size(), 1.0f); // initialize node sizes to 1.0
    
    return data;
}

CutPursuitPartition::CutPursuitPartition(PCP::Parameters params)
    : m_regularization(params.regularization)
    , m_spatialWeight(params.spatialWeight)
    , m_cutoff(params.cutoff)
{
}

Data* CutPursuitPartition::Process(Data& data, QString& errorMessage, CCCoreLib::GenericProgressCallback* progressCb)
{
    // more parallel cut pursuit params
    m_D = 3 + static_cast<int32_t>(data.numFeatures());
    m_Y.assign(static_cast<size_t>(data.size()) * m_D, 0.0f);

	std::vector<float>   edgeWeights(data.numEdges());
	std::vector<int32_t> firstEdge(data.numNodes() + 1);
	std::vector<int32_t> target(data.numEdges());
	std::vector<int32_t> reindex(data.numEdges());
	std::vector<float>   coorWeights(m_D, 1.0f);

    // settings for parallel processing
	int32_t maxSplitSize = data.numNodes();
	if (m_maxNumThreads < 0)
	{
		m_maxNumThreads = omp_get_max_threads();
	}

	// Allocate Comp array using malloc because cut pursuit uses C-style memory tracking
	int32_t* Comp = (int32_t*)calloc(data.numNodes(), sizeof(int32_t));
	if (!Comp)
	{
		return nullptr; // not enough memory
	}

    // Get the mean XYZ as the offset to apply to point cooordinates
    CCVector3d posOffset(0, 0, 0);
    for (unsigned i = 0; i < data.size(); ++i)
    {
        posOffset += data.getPoint(i)->toDouble();
    }
    posOffset /= static_cast<double>(data.size());

    // build the Y array with coordinates and features
    for (unsigned i = 0; i < data.size(); ++i)
    {
        const CCVector3d P = data.getPoint(i)->toDouble();

        auto Yi = m_Y.data() + i * m_D;
        // offset point coordinates to balance ranges in Y array
        Yi[0]   = static_cast<float>(P.x - posOffset.x);
        Yi[1]   = static_cast<float>(P.y - posOffset.y);
        Yi[2]   = static_cast<float>(P.z - posOffset.z);

        // if (useRGB && pc->hasColors())
        // {
        //     const ccColor::Rgba& C = pc->getPointColor(i);
        //     Yi[3]                  = C.r / 255.0f;
        //     Yi[4]                  = C.g / 255.0f;
        //     Yi[5]                  = C.b / 255.0f;
        // }

        for (unsigned f = 0; f < data.numFeatures(); ++f)
        {
            ccScalarField::Shared sf    = data.getCCScalarField(f);
            float                 value = static_cast<float>(sf->getValue(i));

            // Sanitize NaN/Inf, force it to 0.0
            if (!std::isfinite(value))
            {
                value = 0.0f;
            }
            // Scalar fields start at feature index 3, after the coordinates
            Yi[3 + f] = value;
        }
    }

    // compute CSR representation of the graph
	data.edgeListToForwardStar(firstEdge.data(), target.data(), reindex.data());

    // compute edge weights based on distances in CSR order
	for (int32_t e = 0; e < data.numEdges(); ++e)
	{
		// The weight for original edge 'e' maps to the same new position
		edgeWeights[reindex[e]] = data.edgeAttr[e] * m_regularization;
	}

    // apply spatial weight
	for (int32_t d = 0; d < 3; ++d)
	{
		coorWeights[d] *= m_spatialWeight;
	}

    // Here is where the actual partition happens
    // cut-pursuit with preconditioned forward-Douglas-Rachford
	CP* cp = new CP(data.numNodes(), data.numEdges(), firstEdge.data(), target.data(), m_Y.data(), m_D);

	cp->set_loss(static_cast<float>(m_D), m_Y.data(), data.nodeSize.data(), coorWeights.data());
	cp->set_edge_weights(edgeWeights.data(), m_regularization);
	cp->set_cp_param(m_difTol, m_iterations, m_verbose);
	cp->set_split_param(maxSplitSize, m_K, m_splitIterNum, m_splitDampRatio, m_kmppInitNum, m_kmppIterNum);
	cp->set_min_comp_weight(static_cast<float>(m_cutoff));
	cp->set_parallel_param(m_maxNumThreads, m_balanceParallelSplit);
	cp->set_components(0, Comp);;

	if (progressCb)
	{
		int cp_it = cp->cut_pursuit(true, progressCb);
	}
	else
	{
		int cp_it = cp->cut_pursuit(true);
	}

    // Get number of components and their lists of indices
	const int32_t* compAssign;
	const int32_t* compOffset;
	const int32_t* compList;
	auto           rV = cp->get_components(&compAssign, &compOffset, &compList);
    
    // error handling
    if (rV <= 0)
    {
        errorMessage = QObject::tr("[GAE] Failed to compute partition!");
        delete cp;
        return nullptr;
    }

    const int32_t* reducedEdgeList;
    const float*   reducedEdgeWeights;
    auto           rE = cp->get_reduced_graph(&reducedEdgeList, &reducedEdgeWeights);

    // Create the new Data object
    Data* d2 = new Data(static_cast<unsigned>(rV));

    // Copy results
    data.superIndex.assign(compAssign, compAssign + data.numNodes());
    data.clusterList.assign(compList, compList + data.numNodes());
    d2->subPointer.assign(compOffset, compOffset + rV + 1);

    d2->nodeSize.resize(rV);
    for (int32_t rv = 0; rv < rV; rv++)
    {
        const int32_t start = compOffset[rv];
        const int32_t end   = compOffset[rv + 1];

        d2->nodeSize[rv] = static_cast<float>(end - start);

        CCVector3d pos(0, 0, 0);
        for (int32_t i = start; i < end; ++i)
        {
            const CCVector3* P = data.getPoint(static_cast<unsigned>(compList[i]));
            pos.x += P->x;
            pos.y += P->y;
            pos.z += P->z;
        }
        const double n = std::max<int32_t>(end - start, 1);
        d2->addPoint(CCVector3(static_cast<PointCoordinateType>(pos.x / n),
                               static_cast<PointCoordinateType>(pos.y / n),
                               static_cast<PointCoordinateType>(pos.z / n)));
    }

    d2->edges.assign(reducedEdgeList, reducedEdgeList + rE * 2);
    d2->edgeAttr.resize(rE);
    for (int32_t e = 0; e < rE; ++e)
    {
        d2->edgeAttr[e] = reducedEdgeWeights[e] / m_regularization;
    }

	delete cp;
	return d2;
}
