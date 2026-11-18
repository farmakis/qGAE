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

	try
	{
		// addOption(Option(densityCheckBox, Features::Feature::Density, 0));
		addOption(Option(eigSumCheckBox, Features::Feature::Eigen, CCCoreLib::Neighbourhood::GeomFeature::EigenValuesSum));
		addOption(Option(eigOmnivarianceCheckBox, Features::Feature::Eigen, CCCoreLib::Neighbourhood::GeomFeature::Omnivariance));
		addOption(Option(eigenentropyCheckBox, Features::Feature::Eigen, CCCoreLib::Neighbourhood::GeomFeature::EigenEntropy));
		addOption(Option(eigAnisotropyCheckBox, Features::Feature::Eigen, CCCoreLib::Neighbourhood::GeomFeature::Anisotropy));
		addOption(Option(eigPlanarityCheckBox, Features::Feature::Eigen, CCCoreLib::Neighbourhood::GeomFeature::Planarity));
		addOption(Option(eigLinearityCheckBox, Features::Feature::Eigen, CCCoreLib::Neighbourhood::GeomFeature::Linearity));
		addOption(Option(eigPCA1CheckBox, Features::Feature::Eigen, CCCoreLib::Neighbourhood::GeomFeature::PCA1));
		addOption(Option(eigPCA2CheckBox, Features::Feature::Eigen, CCCoreLib::Neighbourhood::GeomFeature::PCA2));
		addOption(Option(eigSurfaceVarCheckBox, Features::Feature::Eigen, CCCoreLib::Neighbourhood::GeomFeature::SurfaceVariation));
		addOption(Option(eigSphericityCheckBox, Features::Feature::Eigen, CCCoreLib::Neighbourhood::GeomFeature::Sphericity));
		addOption(Option(eigVerticalityCheckBox, Features::Feature::Eigen, CCCoreLib::Neighbourhood::GeomFeature::Verticality));
		addOption(Option(eigenvalue1CheckBox, Features::Feature::Eigen, CCCoreLib::Neighbourhood::GeomFeature::EigenValue1));
		addOption(Option(eigenvalue2CheckBox, Features::Feature::Eigen, CCCoreLib::Neighbourhood::GeomFeature::EigenValue2));
		addOption(Option(eigenvalue3CheckBox, Features::Feature::Eigen, CCCoreLib::Neighbourhood::GeomFeature::EigenValue3));
		addOption(Option(degreeOfPlanarityCheckBox, Features::Feature::Eigen, CCCoreLib::Neighbourhood::GeomFeature::DegreeOfPlanarity));
		addOption(Option(degreeOfLinearityCheckBox, Features::Feature::Eigen, CCCoreLib::Neighbourhood::GeomFeature::DegreeOfLinearity));
		addOption(Option(normalXCheckBox, Features::Feature::Normal, Features::NormalAxis::Nx));
		addOption(Option(normalYCheckBox, Features::Feature::Normal, Features::NormalAxis::Ny));
		addOption(Option(normalZCheckBox, Features::Feature::Normal, Features::NormalAxis::Nz));
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
		ccLog::Warning(QString("Internal error: null checkbox for feature '%1'").arg(static_cast<int>(option.ft.f)));
		assert(false);
		return;
	}
	if (std::find(m_options.begin(), m_options.end(), option) != m_options.end())
	{
		ccLog::Warning(QString("Internal error: duplicate option for feature '%1'").arg(static_cast<int>(option.ft.f)));
		assert(false);
		return;
	}
	m_options.push_back(option);
}

void qGAEFeatureDialog::setSelectedFeatures(const std::vector<Features::FeatureData>& features)
{
	reset();

	for (Features::FeatureData f : features)
	{
		auto it = std::find(m_options.begin(), m_options.end(), Option(nullptr, f));
		if (it != m_options.end())
		{
			it->checkBox->setChecked(true);
		}
	}
}

std::vector<Features::FeatureData> qGAEFeatureDialog::getSelectedFeatures() const
{
	std::vector<Features::FeatureData> features;
	for (const Option& opt : m_options)
	{
		assert(opt.checkBox);
        if (opt.checkBox && opt.checkBox->isChecked())
		{
			features.push_back(opt.ft);
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
