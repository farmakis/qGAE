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

//qCC_plugins
#include <ccMainAppInterface.h>

//qPCP
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
    params.knn           = 26;
    params.knnRadius     = resolution;
    params.regularization = dlg.getRegularization();
    params.spatialWeight = 0.00001f;
    params.cutoff        = dlg.getCutoff();

    ccProgressDialog pDlg(parentWidget);
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

        auto subsampled = CCCoreLib::CloudSamplingTools::resampleCloudSpatially(cloud,
                                                                                static_cast<PointCoordinateType>(resolution),
                                                                                CCCoreLib::CloudSamplingTools::SFModulationParams(false),
                                                                                theOctree.data(),
                                                                                &pDlg);
        if (!subsampled)
        {
            app->dispToConsole(QObject::tr("[GAE] Failed to subsample cloud '%1'!").arg(cloud->getName()), ccMainAppInterface::ERR_CONSOLE_MESSAGE);
            break;
        }

        ccPointCloud* pc = cloud->partialClone(subsampled);
        delete subsampled;
        subsampled = nullptr;
        if (!pc)
        {
            app->dispToConsole(QObject::tr("[GAE] Not enough memory to subsample cloud '%1'!").arg(cloud->getName()), ccMainAppInterface::ERR_CONSOLE_MESSAGE);
            break;
        }

        Data* d1 = new Data(*pc);
        delete pc;
        pc = nullptr;
        if (!d1->isValid())
        {
            delete d1;
            app->dispToConsole(QObject::tr("[GAE] Not enough memory to create the graph for cloud '%1'!").arg(cloud->getName()), ccMainAppInterface::ERR_CONSOLE_MESSAGE);
            break;
        }

        ccOctree::Shared theSubOctree = d1->getOctree();
        if (!theSubOctree)
        {
            theSubOctree = d1->computeOctree(&pOctreeDlg);
            if (!theSubOctree)
            {
                app->dispToConsole(QObject::tr("[GAE] Couldn't compute octree for cloud '%1'!").arg(d1->getName()), ccMainAppInterface::ERR_CONSOLE_MESSAGE);
                break;
            }
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
                                                         theSubOctree.data());

        if (err != GAT::NoError) 
        { 
            errorMessage = "Failed to compute feature space";
            return false;
        }
        for (auto& r : requests) 
        {
            d1->getCCScalarField(r.sfIdx)->computeMinAndMax();
        }

        app->addToDB(d1);
        app->refreshAll();
        
        // // we create/activate Level-1's label scalar field
        // int sfIdx = pc->getScalarFieldIndexByName(LEVEL_1_SF_NAME);
        // if (sfIdx < 0)
        // {
        //     sfIdx = pc->addScalarField(LEVEL_1_SF_NAME);
        // }
        // if (sfIdx < 0)
        // {
        //     app->dispToConsole(QObject::tr("[GAE] Couldn't allocate a new scalar field for computing higher level partition! Try to free some memory ..."), ccMainAppInterface::ERR_CONSOLE_MESSAGE);
        //     break;
        // }
        // pc->setCurrentScalarField(sfIdx);

        // // Assign component index to each point
        // ccScalarField::Shared sf = pc->getCCScalarField(sfIdx);
        // for (unsigned i = 0; i < pc->size(); ++i)
        // {
        //     sf->setValue(i, static_cast<ScalarType>(components[i]));
        // }
        // sf->computeMinAndMax();

        // ############################################################################
        // ############################################################################
        // ############################################################################
        
        // // Compute segment normals
        // requests.clear(); // 'requests' is a std::vector (not a pointer): reuse it for the segment-level features
        // const char* levelNames[3] = { "Nx (L1)", "Ny (L1)", "Nz (L1)" };

        // for (int k = 0; k < 3; ++k)
        // {
        //     int idx = d1->getScalarFieldIndexByName(levelNames[k]);
        //     if (idx < 0) idx = pc->addScalarField(levelNames[k]);
        //     if (idx < 0) { /* error, memory */ break; }

        //     GAT::MultiGeomCharacteristicRequest r;
        //     r.c         = GAT::Normal;
        //     r.subOption = k;
        //     r.sf        = pc->getCCScalarField(idx).get();
        //     r.sfIdx     = idx;
        //     requests.push_back(r);
        // }

        // for (int segment = 0; segment < rV; ++segment)
        // {
        //     ScalarType value = CCCoreLib::NAN_VALUE;

        //     // get segment points
        //     auto start  = componentsOffset[segment];
        //     auto end    = componentsOffset[segment + 1];
        //     unsigned pointCount  = end - start;
            
        //     DgmOctree::NeighboursSet pointsInSegment;
        //     try
        //     {
        //         pointsInSegment.resize(pointCount);
        //     }
        //     catch (const std::bad_alloc&)
        //     {
        //         return false;
        //     }

        //     DgmOctree::NeighboursSet::iterator it = pointsInSegment.begin();
        //     for (unsigned i = 0; i < pointCount; ++i, ++it)
        //     {
        //         DgmOctree::PointDescriptor pointInfo;
        //         it->point = pc->getPoint(componentsList[start + i]);
        //         it->pointIndex = componentsList[start + i];
        //     }

        //     //the Neighbourhood is shared by all the characteristics
        //     CCCoreLib::DgmOctreeReferenceCloud neighboursCloud(&pointsInSegment, pointCount);
        //     CCCoreLib::Neighbourhood Z(&neighboursCloud);

        //     const CCVector3* N = Z.getLSPlaneNormal();
        //     for (const GAT::MultiGeomCharacteristicRequest& r : requests)
	    //     {
        //         value = CCCoreLib::NAN_VALUE;
        //         if (N)
        //         {
        //             if (r.subOption == 0)
        //             {
        //                 value = N->x;
        //             }
        //             else if (r.subOption == 1)
        //             {
        //                 value = N->y;
        //             }
        //             else if (r.subOption == 2)
        //             {
        //                 value = N->z;
        //             }
        //         }
        //         for (unsigned i = 0; i < pointCount; ++i)
        //         {
        //             r.sf->setValue(componentsList[start + i], value);
        //         }
        //     }
            
        // }

        // for (auto& r : requests) 
        // {
        //     pc->getCCScalarField(r.sfIdx)->computeMinAndMax();
        // }

        // // we create/activate Level-2's label scalar field
        // sfIdx = pc->getScalarFieldIndexByName(LEVEL_2_SF_NAME);
        // if (sfIdx < 0)
        // {
        //     sfIdx = pc->addScalarField(LEVEL_2_SF_NAME);
        // }
        // if (sfIdx < 0)
        // {
        //     app->dispToConsole(QObject::tr("[GAE] Couldn't allocate a new scalar field for computing higher level partition! Try to free some memory ..."), ccMainAppInterface::ERR_CONSOLE_MESSAGE);
        //     break;
        // }
        // pc->setCurrentScalarField(sfIdx);

        // //Upadate display
        // cloud->setEnabled(false);

        // pc->setName(QObject::tr("%1 (res.:%2 - reg:%2 - cutoff:%5)").arg(cloud->getName()).arg(resolution).arg(params.regularization).arg(params.cutoff));
        // pc->setCurrentDisplayedScalarField(sfIdx);
        // pc->showSF(true);
        // pc->prepareDisplayForRefresh();

        // app->addToDB(pc);
        // app->refreshAll();

        // qint64 time_ms = timer.elapsed();
        // //we display block volume computation timing only if no error occurred!
        // if (app)
        // {
        //     app->dispToConsole(QObject::tr("[GAE] Detected %1 facets in cloud '%2': %3 s").arg(rV).arg(cloudName).arg(time_ms / 1000.0, 0, 'f', 3),
        //         ccMainAppInterface::STD_CONSOLE_MESSAGE);
        // }
    }
    
    return true;
}
