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
#include "ccGeomFeaturesDlg.h"

//Qt
#include <QElapsedTimer>
#include <QMessageBox>

//System
#include <cmath>
#include <numeric>

using namespace PCP;

int RES_FACTOR_NORMALS = 3; // resolution factor for normals computation 

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
    bool useRGB = false;

    // per-level parameter lists (validated to be of the same length)
    if (!dlg.validateLists(errorMessage))
    {
        return false;
    }
    
    int connectivity                             = dlg.getConnectivity();
    int octreeLevel                              = dlg.getResolution();
    const std::vector<double> regularizationList = dlg.getRegularization();
    const std::vector<double> spatialWeightList  = dlg.getSpatialWeight();
    const std::vector<int>    cutoffList         = dlg.getCutoff();

    // some parallel cut pursuit params (Level-1 values; Level-2 values are set later)
	PCP::Parameters params;
    params.regularization   = static_cast<float>(regularizationList[0]);
    params.spatialWeight    = static_cast<float>(spatialWeightList[0]);
    params.cutoff           = cutoffList[0];

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

        int numLevels = regularizationList.size();        
        NAG nag;

        VoxelPartition voxelizer(octreeLevel, connectivity);
        Data* data = voxelizer.Process(cloud, errorMessage, &pDlg);
        if (!data)
        {
            return false;
        }
        if(!nag.addLevel(data))
        {
            errorMessage = "Failed to add level to NAG (SF memory allocation issue)";
            return false;
        }
 

        int targetLevel = 0; // TODO: set the intended target level
        for (int level = 0; level < numLevels; ++level)
        {
            // the iterative process starts here
            if (!nag.setLevelFeatures(dlg.getSelectedFeatures(), level))
            {
                errorMessage = QString("Failed to set level features at level %1").arg(level);
                return false;
            }
            if (!nag.computeLevelFeatures(level, targetLevel, &pDlg))
            {
                errorMessage = QString("Failed to compute segment features at level %1").arg(level);
                return false;
            }
            CutPursuitPartition partition(params);
            if(!nag.addLevel(partition.Process(data, errorMessage, &pDlg)))
            {
                errorMessage = "Failed to add level (SF memory allocation issue)";
                return false;
            }
        }

        // // this will be coniditioned by if (dlg.partitionMethod == CutPursuit) when we support multiple partition methods
        // CutPursuitPartition partition(params);
        // Data* d1 = partition.Process(*d0, errorMessage, &pDlg);
        // if (!d1)
        // {
        //     return false;
        // }
        // if(!nag.addLevel(d1))
        // {
        //     errorMessage = "Failed to add level to NAG (SF memory allocation issue)";
        //     return false;
        // }

        // // ############################################################################
        // // ############################################################################
        // // ############################################################################
        
        // // Compute segment features
        // requests.clear(); // 'requests' is a std::vector (not a pointer): reuse it for the segment-level features
        // const char* levelNames[3] = { "Nx (L1)", "Ny (L1)", "Nz (L1)" };

        // for (int k = 0; k < 3; ++k)
        // {
        //     int idx = d1->addField(levelNames[k]);
        //     if (idx < 0) { /* error, memory */ break; }

        //     GAT::MultiGeomCharacteristicRequest r;
        //     r.c         = GAT::Normal;
        //     r.subOption = k;
        //     r.sf        = d1->getCCScalarField(idx).get();
        //     r.sfIdx     = idx;
        //     requests.push_back(r);
        // }

        // if (!nag.computeLevelFeatures(requests, 1, 0, &pDlg))
        // {
        //     errorMessage = "Failed to compute segment features at level 1";
        //     return false;
        // }

        // // create the higher level scalar fields in lower to host feature values of segments
        // for (unsigned sfIndx = 0; sfIndx < d1->numFeatures(); ++sfIndx)
        // {
        //     auto sfName = d1->getScalarFieldName(sfIndx);
        //     int sfIdxHigh = d1->getScalarFieldIndexByName(sfName);
        //     ccScalarField::Shared sfHigh = d1->getCCScalarField(sfIdxHigh);

        //     // we create/activate Level-2's label scalar field in the target level
        //     int sfIdxLow = d0->getScalarFieldIndexByName(sfName);
        //     if (sfIdxLow < 0)
        //     {
        //         sfIdxLow = d0->addScalarField(sfName);
        //     }
        //     if (sfIdxLow < 0)
        //     {
        //         app->dispToConsole(QObject::tr("[GAE] Couldn't allocate a new scalar field for computing higher level partition! Try to free some memory ..."), ccMainAppInterface::ERR_CONSOLE_MESSAGE);
        //         break;
        //     }
        //     d0->setCurrentScalarField(sfIdxLow);

        //     ccScalarField::Shared sfLow = d0->getCCScalarField(sfIdxLow);

        //     // map the higher segment values to the corresponding lower points
        //     for (int rv = 0; rv < d1->numNodes(); ++rv)
        //     {
        //         ScalarType value = sfHigh->getValue(rv);

        //         // get segment points
        //         auto start  = d1->subPointer[rv];
        //         auto end    = d1->subPointer[rv + 1];
        //         unsigned pointCount  = end - start;

        //         for (unsigned i = 0; i < pointCount; ++i)
        //         {
        //             sfLow->setValue(d0->clusterList[start + i], value);
        //         }
        //     }
        //     sfLow->computeMinAndMax();
        // }

        // //Segment one more level up to create a second level of segments (Level-2)
        // // this will be coniditioned by if (dlg.partitionMethod == CutPursuit) when we support multiple partition methods
        // params.regularization = static_cast<float>(regularizationList[1]);
        // params.spatialWeight  = static_cast<float>(spatialWeightList[1]);
        // params.cutoff         = cutoffList[1];
        
        // CutPursuitPartition partition2(params);
        // Data* d2 = partition2.Process(*d1, errorMessage, &pDlg);
        // if (!d2)
        // {
        //     return false;
        // }
        // if (!nag.addLevel(d2))
        // {
        //     errorMessage = "Failed to add level to NAG (SF memory allocation issue)";
        //     return false;
        // }

        // //Upadate display
        // cloud->setEnabled(false);

        // d0->setName(QObject::tr("%1 (res.:%2 - reg:%3 - cutoff:%4)").arg(cloud->getName()).arg(voxelSize).arg(regularizationList[0]).arg(cutoffList[0]));
        // d0->showSF(true);
        // d0->prepareDisplayForRefresh();

        // app->addToDB(d0);
        // // app->addToDB(d1);
        // // app->addToDB(d2);
        // app->refreshAll();

        // qint64 time_ms = timer.elapsed();
        // //we display block volume computation timing only if no error occurred!
        // if (app)
        // {
        //     app->dispToConsole(QObject::tr("[GAE] Created %1 segments in cloud '%2': %3 s").arg(d1->numNodes()).arg(cloudName).arg(time_ms / 1000.0, 0, 'f', 3),
        //         ccMainAppInterface::STD_CONSOLE_MESSAGE);
        //     app->dispToConsole(QObject::tr("[GAE] Created %1 segments in cloud '%2': %3 s").arg(d2->numNodes()).arg(cloudName).arg(time_ms / 1000.0, 0, 'f', 3),
        //         ccMainAppInterface::STD_CONSOLE_MESSAGE);
        // }
    }
    
    return true;
}
