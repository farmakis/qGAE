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

#include <ui_qOBPADialog.h>

//Qt
#include <QSettings>

class ccMainAppInterface;

//! Dialog for the OBPA plugin
class qOBPADialog : public QDialog, public Ui::OBPADialog
{
	Q_OBJECT

public:
	//! Default constructor
	explicit qOBPADialog( ccMainAppInterface* app );

	double getResolution() const;
	double getMinPlanarity() const;
	double getRegularization() const;
	int    getCutoff() const;

	void loadParamsFromPersistentSettings();
	void saveParamsToPersistentSettings();

private:
	ccMainAppInterface* m_app;
};
