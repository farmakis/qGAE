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

// qCC_db
#include <ccProgressDialog.h>

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

bool NAG::setLevelFeatures(const GeomCharacteristicWithSubOptionSet& selectedFeatures, int level)
{
    if (level >= numLevels() || level < 0)
    {
        assert(false);
        return false;
    }

    Data* data = (*this)[level];
    deleteAllScalarFields(level); // clean existing scalar fields of that level first
    data->features.reserve(selectedFeatures.size());

    for (const GeomCharacteristicWithSubOption& f : selectedFeatures)
    {
        QString sfName = GetFeatureSFName(f.charac, f.subOption);
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

        // populate the request structure (except for ApproxLocalDensity which was handled separately)
        CCCoreLib::GeometricalAnalysisTools::MultiGeomCharacteristicRequest request;
        {
            request.c         = f.charac;
            request.subOption = f.subOption;
            request.sf        = (data && sfIdx >= 0 ? data->getCCScalarField(sfIdx).get() : nullptr);
            request.sfIdx     = sfIdx;
        } 

        data->features.push_back(request);
    }

    return true;
}

bool NAG::computeLevelFeatures(int level, int subLevel/*=0*/, const ccProgressDialog* pDlg/*=nullptr*/)
{
    const int RES_FACTOR_NORMALS = 3; // factor to multiply the voxel size to get the radius for normal estimation

    // Recover the level Data object we will be working on
    if (level < 0 || level >= static_cast<int>(size()) || subLevel < -1 || subLevel > level)
    {
        assert(false);
        return false;
    }
    Data* data = (*this)[level];

    if (level == subLevel)
    {
        // we compute features directly on the level's points (needs octree)
        ccOctree::Shared theSubOctree = data->getOctree();
        if (!theSubOctree)
        {
            theSubOctree = data->computeOctree(const_cast<ccProgressDialog*>(pDlg));
            if (!theSubOctree)
            {
                // app->dispToConsole(QObject::tr("[GAE] Couldn't compute octree for level '%1'!").arg(level), ccMainAppInterface::ERR_CONSOLE_MESSAGE);
                return false;
            }
        }

        using GAT = CCCoreLib::GeometricalAnalysisTools;
        PointCoordinateType radius = (*this)[level]->voxelSize * RES_FACTOR_NORMALS;
        GAT::ErrorCode err = GAT::ComputeCharacteristics(data->features,
                                                         (*this)[level],
                                                         radius,
                                                         nullptr,
                                                         const_cast<ccProgressDialog*>(pDlg),
                                                         theSubOctree.data());
        if (err != GAT::NoError) 
        { 
            // errorMessage = "Failed to compute feature space";
            return false;
        }

        return true;
    }

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

        for (const CCCoreLib::GeometricalAnalysisTools::MultiGeomCharacteristicRequest& f : data->features)
        {
            ScalarType value = NAN_VALUE;
            switch (f.c)
            {
                case CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic::Feature:
                    if (numberOfPoints > 3)
                        value = static_cast<ScalarType>(Z.computeFeature(static_cast<CCCoreLib::Neighbourhood::GeomFeature>(f.subOption)));
                    break;

                // case Features::Feature::Curvature:
				// 	if (numberOfPoints > 5)
				// 		value = Z.computeCurvature(*queryPoint, static_cast<CCCoreLib::Neighbourhood::CurvatureType>(r.subOption));
				// 	break;

                case CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic::LocalDensity:
					value = static_cast<ScalarType>(numberOfPoints);
					break;

				case CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic::Normal:
				{
					const CCVector3* N = Z.getLSPlaneNormal();
					if (N)
                        switch (f.subOption)
			            {
                        case CCCoreLib::Neighbourhood::NormalComponent::NormalX:
                            value = N->x;
                            break;
                        case CCCoreLib::Neighbourhood::NormalComponent::NormalY:
                            value = N->y;
                            break;
                        case CCCoreLib::Neighbourhood::NormalComponent::NormalZ:
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

            f.sf->setValue(rv, value);
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

    for (const CCCoreLib::GeometricalAnalysisTools::MultiGeomCharacteristicRequest& f : data->features)
    {
        int sfIdx;
        ccScalarField::Shared sfLow;
        if (high == low)
        {
            // if the high and low levels are the same, we don't need to add a scalar field
            // just activate the existing scalar fields
            sfIdx = f.sfIdx;
            sfLow = data->getCCScalarField(sfIdx);
        }
        else
        {
            ccScalarField::Shared sfHigh = data->getCCScalarField(f.sfIdx);

            // add scalar field to the base level with a suffix to avoid name clashes between levels
            QString sfName = GetFeatureSFName(f.c, f.subOption);
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

void NAG::deleteAllScalarFields(int level)
{
    Data* data = (*this)[level];
    // skip if there are no features to delete on that level
    if (data->features.empty())
    {
        return;
    }

    // first delete the labels on the base level
    int sfIdx = (*this)[0]->getScalarFieldIndexByName(QString("Level %1").arg(level).toStdString());
    if (sfIdx >= 0)
    {
        (*this)[0]->deleteScalarField(sfIdx);
    }

    // then find the features that were mapped to lower levels
    for (CCCoreLib::GeometricalAnalysisTools::MultiGeomCharacteristicRequest f: data->features)
    {
        QString sfName = GetFeatureSFName(f.c, f.subOption);
        sfName += QString(" (L%1)").arg(level);

        for (int i = 0; i < numLevels() - 1; ++i)
        {
            int sfIdx = (*this)[i]->getScalarFieldIndexByName(sfName.toStdString());
            if (sfIdx >= 0)
            {
                (*this)[i]->deleteScalarField(sfIdx);
            }
        }
    }

    // finally delete on the current level
    data->deleteAllScalarFields();
    data->features.clear();
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