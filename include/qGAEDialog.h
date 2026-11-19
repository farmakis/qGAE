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

#include <ui_qGAEDialog.h>

#include "qGAEFeatures.h"
#include "qGAEFeatureDialog.h"
#include "qGAEVoxelDialog.h"

//Qt
#include <QSettings>
#include <vector>

class ccMainAppInterface;

//! Dialog for the GAE plugin
class qGAEDialog : public QDialog, public Ui::GAEDialog
{
	Q_OBJECT

public:
	//! Default constructor
	explicit qGAEDialog( ccMainAppInterface* app );

	int getResolution() const;
	//! Voxel graph connectivity (6, 14 or 26)
	int getConnectivity() const;
	//! Partition parameters (for the next level)
	double getRegularization() const;
	double getSpatialWeight() const;
	int    getCutoff() const;

	//! Enables/disables the buttons according to the session state
	void setSessionActive( bool active );

	//! Features selected in the feature dialog
	GeomCharacteristicWithSubOptionSet getSelectedFeatures() const;
	//! Level at which the features are computed
	int getFeatureLevel() const;
	//! Restricts the feature level (must be below the level being processed)
	void setMaxFeatureLevel( int maxLevel );

	void loadParamsFromPersistentSettings();
	void saveParamsToPersistentSettings();

Q_SIGNALS:
	void voxelRequested();
	void featuresRequested();
	void runRequested();
	void undoRequested();

private:
	ccMainAppInterface* m_app;
	qGAEFeatureDialog*  m_featureDialog;
	qGAEVoxelDialog*  m_voxelDialog;
};
