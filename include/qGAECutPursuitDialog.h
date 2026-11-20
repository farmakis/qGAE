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

// Ui
#include <ui_qGAECutPursuitDialog.h>

// Qt
#include <QDialog>

//! Dialog for selecting the voxel grid setup
class qGAECutPursuitDialog : public QDialog
    , public Ui::GAECutPursuitDialog
{
	Q_OBJECT

public:
	//! Default constructor
	explicit qGAECutPursuitDialog(QWidget* parent = nullptr);

    //! Cut Pursuit partition parameters
	double getRegularization() const;
	double getSpatialWeight() const;
	int    getCutoff() const;
	
};