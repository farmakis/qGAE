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

#include "qGAEDialog.h"
#include "qGAEFeatures.h"

//CCPluginAPI
#include <ccMainAppInterface.h>

//Qt
#include <QMainWindow>
#include <QPushButton>


qGAEDialog::qGAEDialog( ccMainAppInterface* app )
	: QDialog( app ? app->getMainWindow() : nullptr )
	, Ui::GAEDialog()
	, m_app( app )
{
	setupUi( this );

	m_featureDialog = new qGAEFeatureDialog( this );
	connect( voxelButton, &QPushButton::clicked, this, &qGAEDialog::voxelRequested );
	connect( featureButton, &QPushButton::clicked, this, [this]()
	{
		if ( m_featureDialog->exec() == QDialog::Accepted && !m_featureDialog->getSelectedFeatures().empty() )
		{
			Q_EMIT featuresRequested();
		}
	} );
	connect( runButton,   &QPushButton::clicked, this, &qGAEDialog::runRequested );
	connect( resetButton, &QPushButton::clicked, this, &qGAEDialog::resetRequested );

	loadParamsFromPersistentSettings();
}

GeomCharacteristicWithSubOptionSet qGAEDialog::getSelectedFeatures() const
{
	return m_featureDialog->getSelectedFeatures();
}

int qGAEDialog::getFeatureLevel() const
{
	return m_featureDialog->getLevel();
}

void qGAEDialog::setMaxFeatureLevel(int maxLevel)
{
	m_featureDialog->setMaxLevel(maxLevel);
}

int qGAEDialog::getResolution() const
{
	return intSpinBoxResolution->value();
}

int qGAEDialog::getConnectivity() const
{
	return comboBoxConnectivity->currentText().toInt();
}

double qGAEDialog::getRegularization() const
{
	return doubleSpinBoxRegularization->value();
}

double qGAEDialog::getSpatialWeight() const
{
	return doubleSpinBoxSpatialWeight->value();
}

int qGAEDialog::getCutoff() const
{
	return intSpinBoxCutoff->value();
}

void qGAEDialog::setSessionActive(bool active)
{
	// resolution/connectivity only matter when (re)creating the voxel grid
	voxelButton->setEnabled(!active);
	intSpinBoxResolution->setEnabled(!active);
	comboBoxConnectivity->setEnabled(!active);
	runButton->setEnabled(active);
	featureButton->setEnabled(active);
	resetButton->setEnabled(active);
}

void qGAEDialog::loadParamsFromPersistentSettings()
{
	QSettings settings("qGAE");

	//read parameters
	int resolution 		  = settings.value("Resolution", intSpinBoxResolution->value()).toInt();
	double regularization = settings.value("Regularization", doubleSpinBoxRegularization->value()).toDouble();
	double spatialWeight  = settings.value("SpatialWeight", doubleSpinBoxSpatialWeight->value()).toDouble();
	int cutoff            = settings.value("Cutoff", intSpinBoxCutoff->value()).toInt();

	//apply parameters
	intSpinBoxResolution->setValue(resolution);
	doubleSpinBoxRegularization->setValue(regularization);
	doubleSpinBoxSpatialWeight->setValue(spatialWeight);
	intSpinBoxCutoff->setValue(cutoff);
}

void qGAEDialog::saveParamsToPersistentSettings()
{
	QSettings settings("qGAE");
	//save parameters
	settings.setValue("Resolution", intSpinBoxResolution->value());
	settings.setValue("Regularization", doubleSpinBoxRegularization->value());
	settings.setValue("SpatialWeight", doubleSpinBoxSpatialWeight->value());
	settings.setValue("Cutoff", intSpinBoxCutoff->value());
}