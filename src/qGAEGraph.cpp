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

#include "qGAEGraph.h"

//system
#include <numeric>
#include <utility>
#include <vector>

// CCCoreLib
#include <GeometricalAnalysisTools.h>
#include <DgmOctreeReferenceCloud.h>

using namespace CCCoreLib;
using GAT = CCCoreLib::GeometricalAnalysisTools;


NAG::NAG(std::vector<Data*> dataList)
    : std::vector<Data*>(std::move(dataList))
{
}

std::vector<int32_t> NAG::getSuperIndex(int high, int low)
{ 
    assert(low >= 0 && high < static_cast<int>(size()) && low < high);

    // start with the superIndex of the lowest level
    std::vector<int32_t> superIndex = (*this)[low]->superIndex;
    for (int level = low + 1; level < high; ++level) 
    {
        for (std::size_t i = 0; i < superIndex.size(); ++i)
        {
            superIndex[i] = (*this)[level]->superIndex[superIndex[i]];
        }
    }
    return superIndex;
}

bool NAG::computeLevelFeatures(const std::vector<Request>& requests, 
                                                            int level, 
                                                            int subLevel/*=0*/, 
                                        const ccProgressDialog* pDlg/*=nullptr*/)
{
    // Recover the level Data object we will be working on
    if (level <= subLevel || subLevel < 0 || level >= static_cast<int>(size()))
    {
        assert(false);
        return false;
    }
    Data* data = (*this)[level];
    Data* subData = (*this)[subLevel];

    // helper arrays
    std::vector<int32_t> superIndex = getSuperIndex(level, subLevel); 
    std::vector<int32_t> pointers(data->numNodes() + 1, 0);
    std::vector<int32_t> clusters(subData->numNodes()); // low-level indices grouped by high-level segment

    // get super index histogram
    for (int i = 0; i < subData->numNodes(); ++i)
        ++pointers[superIndex[i] + 1];

    // prefix sum -> start of each segment
    for (int i = 0; i < data->numNodes(); ++i)
        pointers[i + 1] += pointers[i];

    // scatter (counting sort, stable: indices stay ascending inside a segment)
    std::vector<int32_t> cursor(pointers.begin(), pointers.end() - 1);
    for (int i = 0; i < superIndex.size(); ++i)
        clusters[cursor[superIndex[i]]++] = i;

    for (int rv = 0; rv < data->numNodes(); ++rv)
    {
        const CCVector3* queryPoint = data->getPoint(rv);

        // get segment points from the sub-level
        auto start  = pointers[rv];
        auto end    = pointers[rv + 1];
        unsigned numberOfPoints  = end - start;
        
        DgmOctree::NeighboursSet pointsInSegment;
        try
        {
            pointsInSegment.resize(numberOfPoints);
        }
        catch (const std::bad_alloc&)
        {
            return false;
        }

        DgmOctree::NeighboursSet::iterator it = pointsInSegment.begin();
        for (unsigned i = 0; i < numberOfPoints; ++i, ++it)
        {
            unsigned pointIndex = clusters[start + i];
            it->point = subData->getPoint(pointIndex);
            it->pointIndex = pointIndex;
        }

        //the Neighbourhood is shared by all the characteristics
        CCCoreLib::DgmOctreeReferenceCloud neighboursCloud(&pointsInSegment, numberOfPoints);
        CCCoreLib::Neighbourhood Z(&neighboursCloud);

        for (const Request& r : requests)
        {
            ScalarType value = NAN_VALUE;
            switch (r.c)
            {
                case GAT::Feature:
                    if (numberOfPoints > 3)
                        value = static_cast<ScalarType>(Z.computeFeature(static_cast<CCCoreLib::Neighbourhood::GeomFeature>(r.subOption)));
                    break;

                case GAT::Curvature:
					if (numberOfPoints > 5)
						value = Z.computeCurvature(*queryPoint, static_cast<CCCoreLib::Neighbourhood::CurvatureType>(r.subOption));
					break;

                case GAT::LocalDensity:
					value = static_cast<ScalarType>(numberOfPoints);
					break;

                case GAT::Roughness:
					if (numberOfPoints > 3)
					{
						// value = ZR.computeRoughness(*queryPoint, roughnessUpDir);
                        value = Z.computeRoughness(*queryPoint, nullptr);
					}
					break;

                case GAT::MomentOrder1:
				{
					value = Z.computeMomentOrder1(*queryPoint);
				}
					break;

				case GAT::Normal:
				{
					const CCVector3* N = Z.getLSPlaneNormal();
					if (N)
					{
						if (r.subOption == 0)
						{
							value = N->x;
						}
						else if (r.subOption == 1)
						{
							value = N->y;
						}
						else if (r.subOption == 2)
						{
							value = N->z;
						}
					}
				}
				break;

                default:
					assert(false);
					return false;
            }

            r.sf->setValue(rv, value);
        }
    }
    return true;
}

std::vector<float> NAG::getSubSize(int high, int low)
{
    // Compute the number of `low`-level elements contained in each
    //  `high`-level segment.

    // The sizes are computed in a bottom-up approach using the
    // `superIndex` at each level, starting from the `low`-level
    // sizes.

    // Seed: level `low+1` already knows the size of its children
    std::vector<float> subSizes = (*this)[low + 1]->nodeSize;

    for (int level = low + 1; level < high; ++level)
    {       
        std::vector<float> next((*this)[level + 1]->numNodes(), 0.0f);
        for (size_t i = 0; i < (*this)[level]->numNodes(); ++i)
            next[(*this)[level]->superIndex[i]] += subSizes[i];
        subSizes = std::move(next);
    }
    return subSizes;
}

bool NAG::addLevel(Data* data)
{
    push_back(data);
    if (!(size() > 1))
    {
        // If this was the first level added, we don't need to do anything else
        return true;
    }

    const std::string levelName = "Level " + std::to_string(size() - 1);
    data->setName(levelName.c_str());
    
    // Every time a new level is added, we need to create the scalar field
    // to map the labels at the base level (d0) for visualization purposes.
    int sfIdx = (*this)[0]->getScalarFieldIndexByName(levelName);
    if (sfIdx < 0)
    {
        sfIdx = (*this)[0]->addScalarField(levelName);
    }
    if (sfIdx < 0)
    {
        return false;
    }
    (*this)[0]->setCurrentDisplayedScalarField(sfIdx);
    ccScalarField::Shared sf = (*this)[0]->getCCScalarField(sfIdx);

    // map partition labels from the new level to the base level's scalar field
    int high = static_cast<int>(size()) - 1;
    int low = 0; // always map from the base level
    std::vector<int> superIndex = getSuperIndex(high, low);
    for (unsigned i = 0; i < (*this)[0]->size(); ++i)
    {
        sf->setValue(i, static_cast<ScalarType>(superIndex[i]));
    }
    sf->computeMinAndMax();

    return true;
}