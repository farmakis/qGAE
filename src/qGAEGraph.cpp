//##########################################################################
//#                                                                        #
//#                CLOUDCOMPARE PLUGIN: qGAE                               #
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
#include "qGAEFeatures.h"

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
    assert(low >= -1 && high < static_cast<int>(size()) && low < high);

    
    // start with the superIndex of the lowest level
    std::vector<int32_t> superIndex;
    if (low == -1 && (*this)[0]->hasSub())
    {
        // if the low level is the original points (not member of the NAG), 
        // we start directly with the sub vector that maps them to the level-0 nodes (voxels)
        superIndex = (*this)[0]->sub;
    }
    else
    {
        superIndex = (*this)[low]->superIndex;
    }

    for (int level = low + 1; level < high; ++level) 
    {
        for (std::size_t i = 0; i < superIndex.size(); ++i)
        {
            superIndex[i] = (*this)[level]->superIndex[superIndex[i]];
        }
    }
    return superIndex;
}

bool NAG::setLevelFeatures(const std::vector<Features::FeatureData>& selectedFeatures, int level)
{
    if (level >= numLevels() || level < 0)
    {
        assert(false);
        return false;
    }

    Data* data = (*this)[level];
    data->features.clear();
    data->features.reserve(selectedFeatures.size());

    for (Features::FeatureData ftData : selectedFeatures)
    {
        QString sfName = GetFeatureSFName(ftData.f, ftData.subOption);
        if (sfName.isEmpty())
        {
            return false;
        }

        int sfIdx = data->addField(sfName.toStdString());
        if (sfIdx < 0)
        {
            ccLog::Warning(QString("Failed to add scalar field for feature '%1' at level %2").arg(sfName).arg(level));
            return false;
        }

        // complete the feaure data structure
        ftData.sf = data->getCCScalarField(sfIdx).get();
        ftData.sfIdx = sfIdx;  

        data->features.push_back(ftData);
    }

    return true;
}

bool NAG::computeLevelFeatures(int level, int subLevel/*=0*/, const ccProgressDialog* pDlg/*=nullptr*/)
{
    // Recover the level Data object we will be working on
    if (level < 0 || level >= static_cast<int>(size()) || subLevel < -1 || subLevel > level)
    {
        assert(false);
        return false;
    }
    Data* data = (*this)[level];

    const ccPointCloud* sourcePoints = nullptr;
    if (subLevel == -1)
    {
        sourcePoints = (*this)[0]->getOriginalCloud();
    }
    else
    {
        sourcePoints = (*this)[subLevel];
    }

    if (!sourcePoints)
    {
        assert(false);
        return false;
    }

    // helper arrays
    std::vector<int32_t> superIndex = getSuperIndex(level, subLevel); 
    std::vector<int32_t> pointers(data->size() + 1, 0);
    std::vector<int32_t> clusters(sourcePoints->size()); // low-level indices grouped by high-level segment

    // get super index histogram
    for (unsigned i = 0; i < sourcePoints->size(); ++i)
        ++pointers[superIndex[i] + 1];

    // prefix sum -> start of each segment
    for (unsigned i = 0; i < data->size(); ++i)
        pointers[i + 1] += pointers[i];

    // scatter (counting sort, stable: indices stay ascending inside a segment)
    std::vector<int32_t> cursor(pointers.begin(), pointers.end() - 1);
    for (unsigned i = 0; i < superIndex.size(); ++i)
        clusters[cursor[superIndex[i]]++] = i;

    for (unsigned rv = 0; rv < data->size(); ++rv)
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
            it->point = sourcePoints->getPoint(pointIndex);
            it->pointIndex = pointIndex;
        }

        //the Neighbourhood is shared by all the characteristics
        CCCoreLib::DgmOctreeReferenceCloud neighboursCloud(&pointsInSegment, numberOfPoints);
        CCCoreLib::Neighbourhood Z(&neighboursCloud);

        for (const Features::FeatureData& fd : data->features)
        {
            ScalarType value = NAN_VALUE;
            switch (fd.f)
            {
                case Features::Feature::Eigen:
                    if (numberOfPoints > 3)
                        value = static_cast<ScalarType>(Z.computeFeature(static_cast<CCCoreLib::Neighbourhood::GeomFeature>(fd.subOption)));
                    break;

                // case Features::Feature::Curvature:
				// 	if (numberOfPoints > 5)
				// 		value = Z.computeCurvature(*queryPoint, static_cast<CCCoreLib::Neighbourhood::CurvatureType>(r.subOption));
				// 	break;

                case Features::Feature::Density:
					value = static_cast<ScalarType>(numberOfPoints);
					break;

				case Features::Feature::Normal:
				{
					const CCVector3* N = Z.getLSPlaneNormal();
					if (N)
                        switch (fd.subOption)
			            {
                        case Features::NormalAxis::Nx:
                            value = N->x;
                            break;
                        case Features::NormalAxis::Ny:
                            value = N->y;
                            break;
                        case Features::NormalAxis::Nz:
                            value = N->z;
                            break;
                        default:
                            assert(false);
                            return false;
                        }
                    break;
				}
				break;

                default:
					assert(false);
					return false;
            }

            fd.sf->setValue(rv, value);
        }
    }
    return true;
}

bool NAG::mapFeaturesToLowerLevel(int high, int low, const ccProgressDialog* pDlg/*=nullptr*/)
{
    // Can only map features down to the base level (level 0)
    // points are only accessible as source for feature computation
    assert(low >= 0 && high < static_cast<int>(size()));

    Data* data = (*this)[high];
    Data* dataLow = (*this)[low];

    for (const Features::FeatureData& fd : data->features)
    {
        int sfIdx;
        ccScalarField::Shared sfLow;
        if (high == low)
        {
            // if the high and low levels are the same, we don't need to add a scalar field
            // just activate the existing scalar fields
            sfIdx = fd.sfIdx;
            sfLow = data->getCCScalarField(sfIdx);
        }
        else
        {
            ccScalarField::Shared sfHigh = data->getCCScalarField(fd.sfIdx);

            // add scalar field to the base level with a suffix to avoid name clashes between levels
            QString sfName = GetFeatureSFName(fd.f, fd.subOption);
            sfName += QString(" (L%1)").arg(high);

            sfIdx = dataLow->addField(sfName.toStdString());
            if (sfIdx < 0)
            {
                ccLog::Warning(QString("Failed to add scalar field for feature '%1' at level %2").arg(sfName).arg(low));
                return false;
            }

            sfLow = dataLow->getCCScalarField(sfIdx);
            const std::vector<int32_t>& superIndex = getSuperIndex(high, low);

            for (unsigned i = 0; i < dataLow->size(); ++i)
            {
                ScalarType value = sfHigh->getValue(superIndex[i]);
                sfLow->setValue(i, value);
            }
        }

        sfLow->computeMinAndMax();
        dataLow->setCurrentDisplayedScalarField(sfIdx);
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