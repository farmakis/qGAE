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
#include <ui_qGAEVoxelDialog.h>

// Qt
#include <QDialog>

// System
#include <vector>

//! Dialog for selecting the voxel grid setup
class qGAEVoxelDialog : public QDialog
    , public Ui::GAEVoxelDialog
{
	Q_OBJECT

public:
	//! Default constructor
	explicit qGAEVoxelDialog(QWidget* parent = nullptr);

    int getResolution() const;
	//! Voxel graph connectivity (6, 14 or 26)
	int getConnectivity() const;
	
};