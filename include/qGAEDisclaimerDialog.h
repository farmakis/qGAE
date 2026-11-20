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

#include <ui_qGAEDisclaimerDialog.h>

#include <QDialog>

class ccMainAppInterface;

//! Dialog for displaying the disclaimer
class qGAEDisclaimerDialog : public QDialog, public Ui::GAEDisclaimerDialog
{
public:
	qGAEDisclaimerDialog(QWidget* parent = nullptr);
	~qGAEDisclaimerDialog();
	
	static bool show(ccMainAppInterface* app);
	
private:
	//whether disclaimer has already been displayed (and accepted) or not	
	static bool s_disclaimerAccepted;
	
	std::unique_ptr<Ui::GAEDisclaimerDialog> m_ui;
};
