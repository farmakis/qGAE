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

//! Forces the viewer to show the current scalar field of the voxel grid (level 0)
static void RefreshVoxelLevelDisplay(qGAESession& session, ccMainAppInterface* app)
{
    if (!app || session.nag.empty())
    {
        return;
    }

    Data* voxels = session.nag[0];
    voxels->showSF(true);
    voxels->colorsHaveChanged();                 // the SF colors (VBO) are outdated
    voxels->prepareDisplayForRefresh_recursive(); // flags the window for refresh
    app->updateUI();                              // properties panel (current SF, color scale)
    app->refreshAll();                            // redraws the flagged windows
}

bool qGAEProcess::CreateVoxelGrid(const qGAEDialog& dlg,
                                qGAESession& session,
                                QString& errorMessage,
                                bool allowDialogs,
                                QWidget* parentWidget,
                                ccMainAppInterface* app)
{
    //get the selected entities
    const ccHObject::Container& selectedEntities = app->getSelectedEntities();

    //check if a valid point cloud has been selected
    ccPointCloud* cloud = selectedEntities.empty() ? nullptr : ccHObjectCaster::ToPointCloud(selectedEntities[0]);
    if ( !cloud )
    {
        errorMessage = "No valid point cloud has been selected!";
        return false;
    }

    ccProgressDialog pDlg(true, parentWidget);
    pDlg.setAutoClose(false);

    ccProgressDialog pOctreeDlg(allowDialogs, parentWidget);
    pOctreeDlg.setAutoClose(false);

    //Duration: initialization
	QElapsedTimer voxelTimer;

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
            errorMessage = QObject::tr("Couldn't compute octree for cloud '%1'!").arg(cloud->getName());
            return false;
        }
    }
    
    voxelTimer.start();

    NAG nag;
    VoxelPartition voxelizer(dlg.getResolution(), dlg.getConnectivity());
    Data* data = voxelizer.Process(cloud, errorMessage, &pDlg);
    if (!data)
    {
        return false;
    }
    if(!nag.addLevel(data))
    {
        delete data;
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

    session.cloud   = cloud;
    session.cloudId = cloud->getUniqueID();
    session.voxelId = data->getUniqueID();
    session.lastId  = data->getUniqueID();
    session.nag     = std::move(nag);

    return true;
}

bool qGAEProcess::ComputeFeatures(const qGAEDialog& dlg,
                                qGAESession& session,
                                QString& errorMessage,
                                bool allowDialogs,
                                QWidget* parentWidget,
                                ccMainAppInterface* app)
{
    if (!session.isActive())
    {
        errorMessage = "No graph available: create a level first";
        return false;
    }

    // the levels live in the DB: make sure the user didn't delete them
    ccHObject* root = app->dbRootObject();
    if (!root || !root->find(session.cloudId) || !root->find(session.voxelId) || !root->find(session.lastId))
    {
        session.reset();
        errorMessage = "The voxel grid or a level was removed from the DB: please start over";
        return false;
    }
    ccPointCloud* cloud = session.cloud;

    ccProgressDialog pDlg(true, parentWidget);
    pDlg.setAutoClose(false);

    //Duration: features computation
    QElapsedTimer featuresTimer;
    featuresTimer.start();

    int level = session.nag.numLevels() - 1;
    int projectLevel = 0;
    if (dlg.getFeatureLevel() > level - 1)
    {
        errorMessage = QString("Feature level must be at most %1").arg(level - 1);
        return false;
    }
    if (!session.nag.setLevelFeatures(dlg.getSelectedFeatures(), level))
    {
        errorMessage = QString("Failed to set level features at level %1").arg(level);
        return false;
    }
    if (!session.nag.computeLevelFeatures(level,  dlg.getFeatureLevel(), &pDlg))
    {
        errorMessage = QString("Failed to compute segment features at level %1").arg(level);
        return false;
    }
    if (!session.nag.mapFeaturesToLowerLevel(level, projectLevel, &pDlg))
    {
        errorMessage = QString("Failed to map features from level %1 to level %2").arg(level).arg(projectLevel);
        return false;
    }

    qint64 time_ms = featuresTimer.elapsed();
    session.featuresLevel = level;
    RefreshVoxelLevelDisplay(session, app);

    //we display computation timing only if no error occurred!
    if (app)
    {
        app->dispToConsole(QObject::tr("[GAE] Computed level %1 features: %2 s").arg(level+1).arg(time_ms / 1000.0, 0, 'f', 3),
            ccMainAppInterface::STD_CONSOLE_MESSAGE);
    }

    return true;
}

bool qGAEProcess::RunPartition(const qGAEDialog& dlg,
                                qGAESession& session,
                                QString& errorMessage,
                                bool allowDialogs,
                                QWidget* parentWidget,
                                ccMainAppInterface* app)
{
    if (!session.isActive())
    {
        errorMessage = "No graph available: create a level first";
        return false;
    }

    // the levels live in the DB: make sure the user didn't delete them
    ccHObject* root = app->dbRootObject();
    if (!root || !root->find(session.cloudId) || !root->find(session.voxelId) || !root->find(session.lastId))
    {
        session.reset();
        errorMessage = "The voxel grid or a level was removed from the DB: please start over";
        return false;
    }
    ccPointCloud* cloud = session.cloud;

    if (session.featuresLevel != session.nag.numLevels() - 1)
    {
        errorMessage = "Features of the current level have not been computed yet";
        return false;
    }

    // get the parameters from the dialog
    const double regularization = dlg.getRegularization();
    const double spatialWeight  = dlg.getSpatialWeight();
    const int    cutoff         = dlg.getCutoff();

    ccProgressDialog pDlg(true, parentWidget);
    pDlg.setAutoClose(false);

    //Duration: volume computation
    QElapsedTimer partitionTimer;
    partitionTimer.start();

    auto cloudName = cloud->getName();
    
    int level = session.nag.numLevels() - 1;
    PCP::Parameters params;
    params.regularization   = static_cast<float>(regularization);
    params.spatialWeight    = static_cast<float>(spatialWeight);
    params.cutoff           = cutoff;

    // the process starts here
    CutPursuitPartition partition(params);
    Data* data = partition.Process(*session.nag[level], errorMessage, &pDlg);
    if (!data)
    {
        return false;
    }
    if(!session.nag.addLevel(data))
    {
        delete data;
        errorMessage = "Failed to add level (SF memory allocation issue)";
        return false;
    }

    qint64 time_ms = partitionTimer.elapsed();

    // handling display and output
    session.nag[level]->addChild(data);
    session.lastId = data->getUniqueID();
    data->setName(QObject::tr("%1 (Graph Level %2)").arg(cloud->getName()).arg(level+1));
    data->showSF(true);
    RefreshVoxelLevelDisplay(session, app);

    //we display computation timing only if no error occurred!
    if (app)
    {
        app->dispToConsole(QObject::tr("[GAE] Created level %1 with '%2' segments in cloud: %3 s").arg(level+1).arg(data->numNodes()).arg(time_ms / 1000.0, 0, 'f', 3),
            ccMainAppInterface::STD_CONSOLE_MESSAGE);
    }
    
    return true;
}
