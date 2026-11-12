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

#include "qGAEProcess.h"

//local
#include "qGAEDialog.h"
#include "qGAELevel.h"
#include "qGAEGraph.h"
#include "qGAEPartition.h"

//qCC_plugins
#include <ccMainAppInterface.h>

//qPCP
#include <Graph.h>
#include <Partition.h>

//CCCoreLib
#include <CloudSamplingTools.h>
#include <DgmOctreeReferenceCloud.h>
#include <GeometricalAnalysisTools.h>
#include <Neighbourhood.h>

//qCC_db
#include <ccBackgroundTask.h>
#include <ccNormalVectors.h>
#include <ccOctree.h>
#include <ccPointCloud.h>
#include <ccPointCloudInterpolator.h>
#include <ccProgressDialog.h>
#include <ccScalarField.h>

//Qt
#include <QElapsedTimer>
#include <QMessageBox>

//System
#include <cmath>
#include <numeric>

using namespace PCP;

int RES_FACTOR_NORMALS = 3; // resolution factor for normals computation 
//! Default name for GAE scalar fields
static const char LEVEL_1_SF_NAME[] = "Level 1";
static const char LEVEL_2_SF_NAME[] = "Level 2";


bool qGAEProcess::Compute(const qGAEDialog& dlg,
                                QString& errorMessage,
                                ccPointCloud*& outputCloud,
                                ccHObject*& outputGroup,
                                bool allowDialogs,
                                QWidget* parentWidget,
                                ccMainAppInterface* app)
{
    //get the selected entities
    const ccHObject::Container& selectedEntities = app->getSelectedEntities();

    //check if there is at least one point cloud in the selection
    std::vector<ccPointCloud*> clouds;
    for ( ccHObject* entity : selectedEntities )
    {
        if ( entity && entity->isA( CC_TYPES::POINT_CLOUD ) )
        {
            clouds.push_back( ccHObjectCaster::ToPointCloud(entity) );
        }
    }

    if ( clouds.empty() )
    {
        errorMessage = "No point cloud has been selected!";
        return false;
    }

    // get the parameters from the dialog
    double resolution = dlg.getResolution();
    double minPlanarity = dlg.getMinPlanarity();
    bool useRGB = false;

    // some parallel cut pursuit params
	PCP::Parameters params;
    params.knn            = 26;
    params.knnRadius      = resolution;
    params.regularization = dlg.getRegularization();
    params.spatialWeight  = 0.00001f;
    params.cutoff         = dlg.getCutoff();

    ccProgressDialog pDlg(true, parentWidget);
    pDlg.setAutoClose(false);

    ccProgressDialog pOctreeDlg(allowDialogs, parentWidget);
    pOctreeDlg.setAutoClose(false);

    //Duration: initialization
	QElapsedTimer initTimer;
	initTimer.start();

    for (ccPointCloud* cloud : clouds)
	{
        //Duration: volume computation
        QElapsedTimer timer;
        timer.start();

        auto cloudName = cloud->getName();

        ccOctree::Shared theOctree = cloud->getOctree();
        if (!theOctree)
        {
            theOctree = cloud->computeOctree(&pOctreeDlg);
            if (!theOctree)
            {
                app->dispToConsole(QObject::tr("[GAE] Couldn't compute octree for cloud '%1'!").arg(cloud->getName()), ccMainAppInterface::ERR_CONSOLE_MESSAGE);
                break;
            }
        }

        VoxelPartition voxelizer(params.knnRadius, params.knn);
        Data* d1 = voxelizer.Process(cloud, errorMessage, &pDlg);
        if (!d1)
        {
            errorMessage = "Failed to build the voxel graph";
            return false;
        }

        using GAT = CCCoreLib::GeometricalAnalysisTools;
        GAT::MultiGeomCharacteristicRequestSet requests;
        const char* names[3] = { "Nx", "Ny", "Nz" };

        for (int k = 0; k < 3; ++k)
        {
            int idx = d1->getScalarFieldIndexByName(names[k]);
            if (idx < 0) idx = d1->addScalarField(names[k]);
            if (idx < 0) { /* error, memory */ break; }

            GAT::MultiGeomCharacteristicRequest r;
            r.c         = GAT::Normal;
            r.subOption = k;
            r.sf        = d1->getCCScalarField(idx).get();
            r.sfIdx     = idx;
            requests.push_back(r);
        }

        PointCoordinateType radius = static_cast<PointCoordinateType>(resolution * RES_FACTOR_NORMALS);

        GAT::ErrorCode err = GAT::ComputeCharacteristics(requests,
                                                         d1,
                                                         radius,
                                                         nullptr,
                                                         &pDlg,
                                                         d1->getOctree().data());

        if (err != GAT::NoError) 
        { 
            delete d1;
            errorMessage = "Failed to compute feature space";
            return false;
        }
        for (auto& r : requests) 
        {
            d1->getCCScalarField(r.sfIdx)->computeMinAndMax();
        }

        // this will be coniditioned by if (dlg.partitionMethod == CutPursuit) when we support multiple partition methods
        CutPursuitPartition partition(params);
        Data* d2 = partition.Process(*d1, errorMessage, &pDlg);
        if (!d2)
        {
            errorMessage = "Failed to compute partition";
            return false;
        }
        
        // we create/activate Level-1's label scalar field
        int sfIdx = d1->getScalarFieldIndexByName(LEVEL_1_SF_NAME);
        if (sfIdx < 0)
        {
            sfIdx = d1->addScalarField(LEVEL_1_SF_NAME);
        }
        if (sfIdx < 0)
        {
            app->dispToConsole(QObject::tr("[GAE] Couldn't allocate a new scalar field for computing higher level partition! Try to free some memory ..."), ccMainAppInterface::ERR_CONSOLE_MESSAGE);
            break;
        }
        d1->setCurrentScalarField(sfIdx);

        // Assign component index to each point
        ccScalarField::Shared sf = d1->getCCScalarField(sfIdx);
        for (unsigned i = 0; i < d1->size(); ++i)
        {
            sf->setValue(i, static_cast<ScalarType>(d1->superIndex[i]));
        }
        sf->computeMinAndMax();

        // ############################################################################
        // ############################################################################
        // ############################################################################
        
        // Compute segment features
        requests.clear(); // 'requests' is a std::vector (not a pointer): reuse it for the segment-level features
        const char* levelNames[3] = { "Nx (L1)", "Ny (L1)", "Nz (L1)" };

        for (int k = 0; k < 3; ++k)
        {
            int idx = d2->getScalarFieldIndexByName(levelNames[k]);
            if (idx < 0) idx = d2->addScalarField(levelNames[k]);
            if (idx < 0) { /* error, memory */ break; }

            GAT::MultiGeomCharacteristicRequest r;
            r.c         = GAT::Normal;
            r.subOption = k;
            r.sf        = d2->getCCScalarField(idx).get();
            r.sfIdx     = idx;
            requests.push_back(r);
        }

        for (int rv = 0; rv < d2->numNodes(); ++rv)
        {
            ScalarType value = CCCoreLib::NAN_VALUE;

            // get segment points
            auto start  = d2->subPointer[rv];
            auto end    = d2->subPointer[rv + 1];
            unsigned pointCount  = end - start;
            
            DgmOctree::NeighboursSet pointsInSegment;
            try
            {
                pointsInSegment.resize(pointCount);
            }
            catch (const std::bad_alloc&)
            {
                return false;
            }

            DgmOctree::NeighboursSet::iterator it = pointsInSegment.begin();
            for (unsigned i = 0; i < pointCount; ++i, ++it)
            {
                DgmOctree::PointDescriptor pointInfo;
                it->point = d1->getPoint(d1->clusterList[start + i]);
                it->pointIndex = d1->clusterList[start + i];
            }

            //the Neighbourhood is shared by all the characteristics
            CCCoreLib::DgmOctreeReferenceCloud neighboursCloud(&pointsInSegment, pointCount);
            CCCoreLib::Neighbourhood Z(&neighboursCloud);

            const CCVector3* N = Z.getLSPlaneNormal();
            for (const GAT::MultiGeomCharacteristicRequest& r : requests)
	        {
                value = CCCoreLib::NAN_VALUE;
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
                r.sf->setValue(rv, value);
            }
        }

        // create the higher level scalar fields in lower to host feature values of segments
        for (unsigned sfIndx = 0; sfIndx < d2->numFeatures(); ++sfIndx)
        {
            auto sfName = d2->getScalarFieldName(sfIndx);
            int sfIdxHigh = d2->getScalarFieldIndexByName(sfName);
            ccScalarField::Shared sfHigh = d2->getCCScalarField(sfIdxHigh);

            // we create/activate Level-2's label scalar field in the target level
            int sfIdxLow = d1->getScalarFieldIndexByName(sfName);
            if (sfIdxLow < 0)
            {
                sfIdxLow = d1->addScalarField(sfName);
            }
            if (sfIdxLow < 0)
            {
                app->dispToConsole(QObject::tr("[GAE] Couldn't allocate a new scalar field for computing higher level partition! Try to free some memory ..."), ccMainAppInterface::ERR_CONSOLE_MESSAGE);
                break;
            }
            d1->setCurrentScalarField(sfIdxLow);

            ccScalarField::Shared sfLow = d1->getCCScalarField(sfIdxLow);

            // map the higher segment values to the corresponding lower points
            for (int rv = 0; rv < d2->numNodes(); ++rv)
            {
                ScalarType value = sfHigh->getValue(rv);

                // get segment points
                auto start  = d2->subPointer[rv];
                auto end    = d2->subPointer[rv + 1];
                unsigned pointCount  = end - start;

                for (unsigned i = 0; i < pointCount; ++i)
                {
                    sfLow->setValue(d1->clusterList[start + i], value);
                }
            }
            sfLow->computeMinAndMax();
        }

        //Segment one more level up to create a second level of segments (Level-2)
        // this will be coniditioned by if (dlg.partitionMethod == CutPursuit) when we support multiple partition methods
        // CutPursuitPartition partition(params);
        Data* d3 = partition.Process(*d2, errorMessage, &pDlg);
        if (!d3)
        {
            errorMessage = "Failed to compute partition";
            return false;
        }
        
        // we create/activate Level-2's label scalar field to base cloud
        sfIdx = d1->getScalarFieldIndexByName(LEVEL_2_SF_NAME);
        if (sfIdx < 0)
        {
            sfIdx = d1->addScalarField(LEVEL_2_SF_NAME);
        }
        if (sfIdx < 0)
        {
            app->dispToConsole(QObject::tr("[GAE] Couldn't allocate a new scalar field for computing higher level partition! Try to free some memory ..."), ccMainAppInterface::ERR_CONSOLE_MESSAGE);
            break;
        }
        d1->setCurrentScalarField(sfIdx);

        // Assign L3 component index to each base point
        // first get the great superindex (will be a method of NAG)
        ccScalarField::Shared sf1 = d1->getCCScalarField(sfIdx);

        int low = 0;
        int high = 2;
        std::vector<Data*> dataList = { d1, d2, d3 };

        std::vector<int32_t> superIndex(dataList[low]->numNodes());
        std::iota(superIndex.begin(), superIndex.end(), 0); // identity: each base node starts as its own index
        for (int level = low; level < high; ++level) 
        {
            const std::vector<int>& si = dataList[level]->superIndex;
            for (std::size_t i = 0; i < superIndex.size(); ++i)
            {
                superIndex[i] = si[superIndex[i]];
            }
        }

        for (unsigned i = 0; i < d1->size(); ++i)
        {
            sf1->setValue(i, static_cast<ScalarType>(superIndex[i]));
        }
        sf1->computeMinAndMax();

        //Upadate display
        cloud->setEnabled(false);

        d1->setName(QObject::tr("%1 (res.:%2 - reg:%2 - cutoff:%5)").arg(cloud->getName()).arg(resolution).arg(params.regularization).arg(params.cutoff));
        d1->setCurrentDisplayedScalarField(sfIdx);
        d1->showSF(true);
        d1->prepareDisplayForRefresh();

        app->addToDB(d1);
        app->refreshAll();

        qint64 time_ms = timer.elapsed();
        //we display block volume computation timing only if no error occurred!
        if (app)
        {
            app->dispToConsole(QObject::tr("[GAE] Created %1 segments in cloud '%2': %3 s").arg(d2->numNodes()).arg(cloudName).arg(time_ms / 1000.0, 0, 'f', 3),
                ccMainAppInterface::STD_CONSOLE_MESSAGE);
            app->dispToConsole(QObject::tr("[GAE] Created %1 segments in cloud '%2': %3 s").arg(d3->numNodes()).arg(cloudName).arg(time_ms / 1000.0, 0, 'f', 3),
                ccMainAppInterface::STD_CONSOLE_MESSAGE);
        }
    }
    
    return true;
}
