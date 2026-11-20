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

#include "qGAEVoxelDialog.h"

// CCCoreLib
#include <DgmOctree.h>

qGAEVoxelDialog::qGAEVoxelDialog(QWidget* parent /*=nullptr*/)
    : QDialog(parent)
    , Ui::GAEVoxelDialog()
{
	setupUi(this);

	octreeLevelSpinBox->setMaximum(CCCoreLib::DgmOctree::MAX_OCTREE_LEVEL);
}

void qGAEVoxelDialog::setCloud(ccGenericPointCloud* cloud)
{
	octreeLevelSpinBox->setCloud(cloud);
}

int qGAEVoxelDialog::getResolution() const
{
	return octreeLevelSpinBox->value();
}

int qGAEVoxelDialog::getConnectivity() const
{
	return connectivityComboBox->currentText().toInt();
}