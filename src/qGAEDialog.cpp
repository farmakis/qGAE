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

//CCPluginAPI
#include <ccMainAppInterface.h>

//Qt
#include <QMainWindow>


qGAEDialog::qGAEDialog( ccMainAppInterface* app )
	: QDialog( app ? app->getMainWindow() : nullptr )
	, Ui::GAEDialog()
	, m_app( app )
{
	setupUi( this );
	
	loadParamsFromPersistentSettings();
}

int qGAEDialog::getResolution() const
{
	return intSpinBoxResolution->value();
}

int qGAEDialog::getConnectivity() const
{
	return comboBoxConnectivity->currentText().toInt();
}

// parses a comma-separated list; returns an empty vector if any item is invalid
static std::vector<double> ParseDoubleList(const QString& text)
{
	std::vector<double> values;
	const QStringList   items = text.split(',', Qt::SkipEmptyParts);
	for (const QString& item : items)
	{
		bool         ok = false;
		const double v  = item.trimmed().toDouble(&ok);
		if (!ok)
		{
			return {};
		}
		values.push_back(v);
	}
	return values;
}

std::vector<double> qGAEDialog::getRegularization() const
{
	return ParseDoubleList(lineEditRegularization->text());
}

std::vector<double> qGAEDialog::getSpatialWeight() const
{
	return ParseDoubleList(lineEditSpatialWeight->text());
}

std::vector<int> qGAEDialog::getCutoff() const
{
	std::vector<int> values;
	for (double v : ParseDoubleList(lineEditCutoff->text()))
	{
		values.push_back(static_cast<int>(v));
	}
	return values;
}

bool qGAEDialog::validateLists(QString& errorMessage) const
{
	const std::vector<double> reg = getRegularization();
	const std::vector<double> sw = getSpatialWeight();
	const std::vector<int>    cut = getCutoff();

	if (reg.empty() || sw.empty() || cut.empty())
	{
		errorMessage = tr("Regularization, spatial weight and cutoff must be non-empty comma-separated lists of numbers");
		return false;
	}
	if (reg.size() != sw.size() || reg.size() != cut.size())
	{
		errorMessage = tr("Regularization, spatial weight and cutoff lists must have the same length");
		return false;
	}
	for (double v : reg)
	{
		if (v < 0.0) { errorMessage = tr("Regularization values must be >= 0"); return false; }
	}
	for (double v : sw )
	{
		if (v <= 0.0) { errorMessage = tr("Spatial weight values must be > 0"); return false; }
		if (v > 1.0) { errorMessage = tr("Spatial weight values must be <= 1"); return false; }
	}
	for (int v : cut)
	{
		if (v < 1) { errorMessage = tr("Cutoff values must be a > 0"); return false; }
	}
	return true;
}

void qGAEDialog::loadParamsFromPersistentSettings()
{
	QSettings settings("qGAE");

	//read parameters
	int resolution = settings.value("Resolution", intSpinBoxResolution->value()).toInt();
	QString regularization = settings.value("RegularizationList", lineEditRegularization->text()).toString();
	QString spatialWeight  = settings.value("SpatialWeightList", lineEditSpatialWeight->text()).toString();
	QString cutoff         = settings.value("CutoffList", lineEditCutoff->text()).toString();

	//apply parameters
	intSpinBoxResolution->setValue(static_cast<int>(resolution));
	lineEditRegularization->setText(regularization);
	lineEditSpatialWeight->setText(spatialWeight);
	lineEditCutoff->setText(cutoff);
}

void qGAEDialog::saveParamsToPersistentSettings()
{
	QSettings settings("qGAE");
	//save parameters
	settings.setValue("Resolution", intSpinBoxResolution->value());
	settings.setValue("RegularizationList", lineEditRegularization->text());
	settings.setValue("SpatialWeightList", lineEditSpatialWeight->text());
	settings.setValue("CutoffList", lineEditCutoff->text());
}