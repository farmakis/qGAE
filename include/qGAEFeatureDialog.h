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

#include "qGAEFeatures.h"

// Ui
#include <ui_qGAEFeatureDialog.h>

// Qt
#include <QDialog>

// System
#include <vector>

//! Dialog for selecting the GAE features and their level
class qGAEFeatureDialog : public QDialog
    , public Ui::GAEFeatureDialog
{
	Q_OBJECT

  public:
	//! Default constructor
	explicit qGAEFeatureDialog(QWidget* parent = nullptr);
	//! Sets selected features
	void setSelectedFeatures(const std::vector<Features::FeatureData>& features);
	//! Returns selected features
	std::vector<Features::FeatureData> getSelectedFeatures() const;

	//! Returns	the level at which the features will be computed
	int getLevel() const;
	//! Sets the level
	void setLevel(int level);
	//! Sets the maximum selectable level (the current value is clamped)
	void setMaxLevel(int maxLevel);

	//! Reset the whole dialog
	void reset();

  protected:
	//! Composite structure for a geometric characteristic option and its corresponding checkbox
	struct Option
	{
		Option(QCheckBox* cb, Features::Feature f, int option)
		    : checkBox(cb)
		    , ft{f, option, nullptr, -1}
		{
		}

		Option(QCheckBox* cb, const Features::FeatureData& featureData)
		    : checkBox(cb)
		    , ft(featureData)
		{
		}

		//! Equality operator
		bool operator==(const Option& other) const
		{
			return ft == other.ft;
		}

		QCheckBox*              checkBox = nullptr; //!< Corresponding checkbox
		Features::FeatureData   ft;                 //!< GAE feature (with sub option)
	};

	//! Add option
	void addOption(const Option& option);

	//! List of all available feature options
	std::vector<Option> m_options;
};