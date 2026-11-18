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

#include "qGAEProcess.h"

//local
#include "qGAEDialog.h"
#include "qGAEFeatureDialog.h"
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
    
    const std::vector<double> regularizationList        = dlg.getRegularization();
    const std::vector<double> spatialWeightList         = dlg.getSpatialWeight();
    const std::vector<int>    cutoffList                = dlg.getCutoff();

    ccProgressDialog pDlg(true, parentWidget);
    pDlg.setAutoClose(false);

    ccProgressDialog pOctreeDlg(allowDialogs, parentWidget);
    pOctreeDlg.setAutoClose(false);

    //Duration: initialization
	QElapsedTimer voxelTimer;

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

        size_t numLevels = regularizationList.size();        
        NAG nag;

        voxelTimer.start();

        VoxelPartition voxelizer(dlg.getResolution(), dlg.getConnectivity());
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

        qint64 time_ms = voxelTimer.elapsed();

        //Upadate display
        cloud->setEnabled(false);
        data->setName(QObject::tr("%1 (VoxelGrid at Octree level %2)").arg(cloud->getName()).arg(dlg.getResolution()));
        data->showSF(true);
        data->prepareDisplayForRefresh();
        app->addToDB(data);

        if (app)
        {
            app->dispToConsole(QObject::tr("[GAE] Created voxel grid at octree level %1: %2 s").arg(dlg.getResolution()).arg(time_ms / 1000.0, 0, 'f', 3),
                ccMainAppInterface::STD_CONSOLE_MESSAGE);
        }
 
        int projectLevel = 0;
        PCP::Parameters params;
        for (int level = 0; level < numLevels; ++level)
        {
            // some parallel cut pursuit params (Level-1 values; Level-2 values are set later)
            params.regularization   = static_cast<float>(regularizationList[level]);
            params.spatialWeight    = static_cast<float>(spatialWeightList[level]);
            params.cutoff           = cutoffList[level];

            // the iterative process starts here
            if (!nag.setLevelFeatures(dlg.getSelectedFeatures(), level))
            {
                errorMessage = QString("Failed to set level features at level %1").arg(level);
                return false;
            }
            if (!nag.computeLevelFeatures(level,  dlg.getFeatureLevel(), &pDlg))
            {
                errorMessage = QString("Failed to compute segment features at level %1").arg(level);
                return false;
            }
            if (!nag.mapFeaturesToLowerLevel(level, projectLevel, &pDlg))
            {
                errorMessage = QString("Failed to map features from level %1 to level %2").arg(level).arg(projectLevel);
                return false;
            }
            CutPursuitPartition partition(params);
            Data* d1 = partition.Process(*nag[level], errorMessage, &pDlg);
            if (!d1)
            {
                return false;
            }
            if(!nag.addLevel(d1))
            {
                errorMessage = "Failed to add level (SF memory allocation issue)";
                return false;
            }
        
            // handling display and output
            data->addChild(d1);
            d1->setName(QObject::tr("%1 (Graph Level %2)").arg(cloud->getName()).arg(level+1));
            d1->showSF(true);
            app->refreshAll();

            // //we display computation timing only if no error occurred!
            // if (app)
            // {
            //     app->dispToConsole(QObject::tr("[GAE] Created %1 segments in cloud '%2': %3 s").arg(d1->numNodes()).arg(cloudName).arg(time_ms / 1000.0, 0, 'f', 3),
            //         ccMainAppInterface::STD_CONSOLE_MESSAGE);
            //     app->dispToConsole(QObject::tr("[GAE] Created %1 segments in cloud '%2': %3 s").arg(d2->numNodes()).arg(cloudName).arg(time_ms / 1000.0, 0, 'f', 3),
            //         ccMainAppInterface::STD_CONSOLE_MESSAGE);
            // }
        }
    }
    
    return true;
}
