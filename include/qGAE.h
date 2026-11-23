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

#pragma once

#include "ccStdPluginInterface.h"

#include "qGAEDialog.h"
#include "qGAEProcess.h"

//! GAE plugin
/** Graph Analysis Engine: Point cloud analysis as a multi-level graph.
**/
class qGAE : public QObject, public ccStdPluginInterface
{
	Q_OBJECT
	Q_INTERFACES( ccPluginInterface ccStdPluginInterface )

	Q_PLUGIN_METADATA( IID "cccorp.cloudcompare.plugin.GAE" FILE "../info.json" )

public:
	explicit qGAE( QObject* parent = nullptr );
	~qGAE() override = default;

	// Inherited from ccStdPluginInterface
	void onNewSelection( const ccHObject::Container& selectedEntities ) override;
	QList<QAction*> getActions() override;

private:

	//Begin GAE 
	bool launch();

	void doAction();
	void onCreateVoxelGrid();
	void onComputeFeatures();
	void onRunPartition();
	void onSetRandomRGB();
	void onUndo();
	void popLastLevel();
	void updateFeatureLevelLimit();

	//! Default action
	QAction* m_action;
	//! Non-modal dialog (kept alive between runs)
	qGAEDialog* m_dlg = nullptr;
	//! State shared between hierarchical abstractions
	qGAESession m_session;
};
