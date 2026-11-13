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

#include <ui_qGAEDialog.h>

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

	double getResolution() const;
	//! Per-level parameter lists (comma-separated in the dialog)
	std::vector<double> getRegularization() const;
	std::vector<double> getSpatialWeight() const;
	std::vector<int>    getCutoff() const;

	//! Checks that the three lists are valid and of the same length (returns the error otherwise)
	bool validateLists(QString& error) const;

	void loadParamsFromPersistentSettings();
	void saveParamsToPersistentSettings();

private:
	ccMainAppInterface* m_app;
};
