//##########################################################################
//#                                                                        #
//#                CLOUDCOMPARE PLUGIN: qOBPA                              #
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

//! OBPA plugin
/** Graph-based discontinuity surface segmentation plugin.
**/
class qOBPA : public QObject, public ccStdPluginInterface
{
	Q_OBJECT
	Q_INTERFACES( ccPluginInterface ccStdPluginInterface )

	Q_PLUGIN_METADATA( IID "cccorp.cloudcompare.plugin.OBPA" FILE "../info.json" )

public:
	explicit qOBPA( QObject* parent = nullptr );
	~qOBPA() override = default;

	// Inherited from ccStdPluginInterface
	void onNewSelection( const ccHObject::Container& selectedEntities ) override;
	QList<QAction*> getActions() override;

private:
	void doAction();

	//! Default action
	QAction* m_action;
};
