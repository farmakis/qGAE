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

#include "qGAEFeatureDialog.h"

// qCC_db
#include <ccLog.h>

// CCCoreLib
#include <Neighbourhood.h>
#include <GeometricalAnalysisTools.h>

// Qt
#include <QDialogButtonBox>
#include <QPushButton>

// System
#include <algorithm>
#include <cassert>

qGAEFeatureDialog::qGAEFeatureDialog(QWidget* parent /*=nullptr*/)
    : QDialog(parent)
    , Ui::GAEFeatureDialog()
{
	setupUi(this);

	connect(buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &qGAEFeatureDialog::reset);
	connect(normalsGroupBox, &QGroupBox::toggled, this, &qGAEFeatureDialog::onNormalsToggled);

	try
	{
		// addOption(Option(densityCheckBox, CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic::LocalDensity, 0));
		addOption(Option(eigSumCheckBox, CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic::Feature, CCCoreLib::Neighbourhood::GeomFeature::EigenValuesSum));
		addOption(Option(eigOmnivarianceCheckBox, CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic::Feature, CCCoreLib::Neighbourhood::GeomFeature::Omnivariance));
		addOption(Option(eigenentropyCheckBox, CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic::Feature, CCCoreLib::Neighbourhood::GeomFeature::EigenEntropy));
		addOption(Option(eigAnisotropyCheckBox, CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic::Feature, CCCoreLib::Neighbourhood::GeomFeature::Anisotropy));
		addOption(Option(eigPlanarityCheckBox, CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic::Feature, CCCoreLib::Neighbourhood::GeomFeature::Planarity));
		addOption(Option(eigLinearityCheckBox, CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic::Feature, CCCoreLib::Neighbourhood::GeomFeature::Linearity));
		addOption(Option(eigPCA1CheckBox, CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic::Feature, CCCoreLib::Neighbourhood::GeomFeature::PCA1));
		addOption(Option(eigPCA2CheckBox, CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic::Feature, CCCoreLib::Neighbourhood::GeomFeature::PCA2));
		addOption(Option(eigSurfaceVarCheckBox, CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic::Feature, CCCoreLib::Neighbourhood::GeomFeature::SurfaceVariation));
		addOption(Option(eigSphericityCheckBox, CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic::Feature, CCCoreLib::Neighbourhood::GeomFeature::Sphericity));
		addOption(Option(eigVerticalityCheckBox, CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic::Feature, CCCoreLib::Neighbourhood::GeomFeature::Verticality));
		addOption(Option(eigenvalue1CheckBox, CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic::Feature, CCCoreLib::Neighbourhood::GeomFeature::EigenValue1));
		addOption(Option(eigenvalue2CheckBox, CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic::Feature, CCCoreLib::Neighbourhood::GeomFeature::EigenValue2));
		addOption(Option(eigenvalue3CheckBox, CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic::Feature, CCCoreLib::Neighbourhood::GeomFeature::EigenValue3));
		addOption(Option(degreeOfPlanarityCheckBox, CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic::Feature, CCCoreLib::Neighbourhood::GeomFeature::DegreeOfPlanarity));
		addOption(Option(degreeOfLinearityCheckBox, CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic::Feature, CCCoreLib::Neighbourhood::GeomFeature::DegreeOfLinearity));
		addOption(Option(normalXCheckBox, CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic::Normal, CCCoreLib::Neighbourhood::NormalComponent::NormalX));
		addOption(Option(normalYCheckBox, CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic::Normal, CCCoreLib::Neighbourhood::NormalComponent::NormalY));
		addOption(Option(normalZCheckBox, CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic::Normal, CCCoreLib::Neighbourhood::NormalComponent::NormalZ));
	}
	catch (const std::bad_alloc&)
	{
		ccLog::Warning("[qGAEFeatureDialog] Not enough memory!");
	}
}

void qGAEFeatureDialog::addOption(const Option& option)
{
	if (option.checkBox == nullptr)
	{
		ccLog::Warning(QString("Internal error: null checkbox for feature '%1'").arg(static_cast<int>(option.gc.charac)));
		assert(false);
		return;
	}
	if (std::find(m_options.begin(), m_options.end(), option) != m_options.end())
	{
		ccLog::Warning(QString("Internal error: duplicate option for feature '%1'").arg(static_cast<int>(option.gc.charac)));
		assert(false);
		return;
	}
	m_options.push_back(option);
}

void qGAEFeatureDialog::onNormalsToggled()
{
	normalXCheckBox->setChecked(normalsGroupBox->isChecked());
	normalYCheckBox->setChecked(normalsGroupBox->isChecked());
	normalZCheckBox->setChecked(normalsGroupBox->isChecked());
}

void qGAEFeatureDialog::setSelectedFeatures(const GeomCharacteristicWithSubOptionSet& features)
{
	reset();

	for (const GeomCharacteristicWithSubOption& f : features)
	{
		auto it = std::find(m_options.begin(), m_options.end(), Option(nullptr, f.charac, f.subOption));
		if (it != m_options.end())
		{
			it->checkBox->setChecked(true);
		}
	}
}

GeomCharacteristicWithSubOptionSet qGAEFeatureDialog::getSelectedFeatures() const
{
	GeomCharacteristicWithSubOptionSet features;
	for (const Option& opt : m_options)
	{
		assert(opt.checkBox);
        if (opt.checkBox && opt.checkBox->isChecked())
		{
			features.push_back(opt.gc);
		}
	}
	return features;
}

int qGAEFeatureDialog::getLevel() const
{
	return levelSpinBox->value();
}

void qGAEFeatureDialog::setLevel(int level)
{
	levelSpinBox->setValue(level);
}

void qGAEFeatureDialog::setMaxLevel(int maxLevel)
{
	// QSpinBox clamps the current value automatically
	levelSpinBox->setMaximum(std::max(maxLevel, levelSpinBox->minimum()));
}

void qGAEFeatureDialog::reset()
{
	for (const Option& opt : m_options)
	{
		opt.checkBox->setChecked(false);
	}
}
