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

#pragma once

//Local
#include "qGAEDialog.h"
#include "qGAEGraph.h"

//qCC
#include "ccPointCloud.h"

class ccMainAppInterface;

//! State kept between two 'Run' presses.
/** The NAG doesn't own its levels: the Data objects belong to the DB tree
    (level 0 is added to the DB, level N+1 is a child of level N).
    nag[0] is always the voxel grid. **/
struct qGAESession
{
	ccPointCloud* cloud   = nullptr; //!< original cloud
	NAG           nag;               //!< levels (not owned)
	unsigned      cloudId = 0;       //!< unique IDs, to detect deletion from the DB
	unsigned      voxelId = 0;
	unsigned      lastId  = 0;       //!< unique ID of the last level
	int           featuresLevel = -1; //!< level for which features were computed (-1: none)

	bool isActive() const { return cloud && !nag.empty(); }
	void reset() { cloud = nullptr; nag.clear(); cloudId = voxelId = lastId = 0; featuresLevel = -1; }
};

//! GAE process
/** See "Graph-Based Discontinuity Surface Isolation from 3D Point Clouds", 
    Farmakis, I.; Dewez, T.; Dyson, C., 2027, 
    ISPRS Journal of Photogrammetry and Remote Sensing"
**/
class qGAEProcess
{
public:
	
	static bool CreateVoxelGrid(const qGAEDialog& dlg,
								qGAESession& session,
								QString& errorMessage,
								bool allowDialogs,
								QWidget* parentWidget = nullptr,
								ccMainAppInterface* app = nullptr);
	
	static bool ComputeFeatures(const qGAEDialog& dlg,
								qGAESession& session,
								QString& errorMessage,
								bool allowDialogs,
								QWidget* parentWidget = nullptr,
								ccMainAppInterface* app = nullptr);
		
	static bool RunPartition(const qGAEDialog& dlg,
								qGAESession& session,
								QString& errorMessage,
								bool allowDialogs,
								QWidget* parentWidget = nullptr,
								ccMainAppInterface* app = nullptr);

};