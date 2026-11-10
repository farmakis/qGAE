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
#include "qGAELevel.h"

//qPCP
#include "OMPNumThreads.h"


Data::Data(unsigned N, const QString& name)
    : ccPointCloud(name)
{
    m_valid = reserve(N);
    nodeSize.reserve(N);
    superIndex.reserve(N);
    clusterList.reserve(N);
}

Data::Data(const ccPointCloud& pc, const QString& name)
    : ccPointCloud(name.isEmpty() ? pc.getName() : name)
{
    // appending to an empty cloud copies points and all associated fields
    *this += const_cast<ccPointCloud*>(&pc);
    m_valid = (size() == pc.size());
    nodeSize.reserve(pc.size());
    superIndex.reserve(pc.size());
    clusterList.reserve(pc.size());

    // // collect the scalar fields copied from the source cloud
    // const unsigned sfCount = getNumberOfScalarFields();
    // features.reserve(sfCount);
    // for (unsigned k = 0; k < sfCount; ++k)
    // {
    //     features.push_back(getCCScalarField(k));
    // }
}

int Data::addField(const QString& name)
{
    const std::string stdName = name.toStdString();
    int idx = getScalarFieldIndexByName(stdName);
    if (idx >= 0)
    {
        return idx;
    }

    // the scalar field is allocated with the currently reserved point count
    return addScalarField(stdName);
}

ccScalarField::Shared Data::getFeature(const QString& name)
{
    int sfIdx = getScalarFieldIndexByName(name.toStdString());
    return getCCScalarField(sfIdx);
}

/* adapted from the edge_list_to_forward_star().cpp file of the
 * grid-graph project by Hugo Raguet (https://github.com/1a7r0ch3/grid-graph) */
void Data::edgeListToForwardStar(int32_t* firstEdge, int32_t* target, int32_t* reindex) const
{
	const int32_t V = static_cast<int32_t>(size());
	const size_t  E = edges.size() / 2;

	/* compute number of edges for each vertex and keep track of indices */
	for (int32_t v = 0; v < V; v++)
	{
		firstEdge[v] = 0;
	}
	for (size_t e = 0; e < E; e++)
	{
		reindex[e] = firstEdge[edges[2 * e]]++;
	}

	/* compute cumulative sum and shift to the right */
	int32_t sum = 0; // firstEdge[0] is always 0
	for (int32_t v = 0; v <= V; v++)
	{
		int32_t tmp   = firstEdge[v];
		firstEdge[v] = sum;
		sum += tmp;
	} // firstEdge[V] should be total number of edges

/* finalize reindex */
#pragma omp parallel for NUM_THREADS(E)
	/* unsigned loop counter is allowed since OpenMP 3.0 (2008)
	 * but MSVC compiler still does not support it as of 2020 */
	for (long long e = 0; e < (long long)E; e++)
	{
		reindex[e] += firstEdge[edges[2 * e]];
		// the target vertex for original edge 'e' is stored at (2 * e + 1)
		target[reindex[e]] = edges[2 * e + 1];
	}
}

// bool Data::computeFeatures(GAT::MultiGeomCharacteristicRequestSet requests)
// {
//     std::string label = "Geometric characteristics computation";
// }