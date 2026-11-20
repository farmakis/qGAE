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

#include "qGAEDisclaimerDialog.h"
#include "ui_qGAEDisclaimerDialog.h"

//qCC_plugins
#include <ccMainAppInterface.h>

//Qt
#include <QMainWindow>

bool qGAEDisclaimerDialog::s_disclaimerAccepted = false;


qGAEDisclaimerDialog::qGAEDisclaimerDialog(QWidget *parent)
    : QDialog(parent)
    , m_ui(std::make_unique<Ui::GAEDisclaimerDialog>())
{
    m_ui->setupUi( this );
}

qGAEDisclaimerDialog::~qGAEDisclaimerDialog() = default;

bool qGAEDisclaimerDialog::show(ccMainAppInterface *app)
{
	if ( !s_disclaimerAccepted )
	{
		//if the user "cancels" it, then he refuses the disclaimer
		s_disclaimerAccepted = qGAEDisclaimerDialog(app ? app->getMainWindow() : 0).exec();
	}

	return s_disclaimerAccepted;
}
