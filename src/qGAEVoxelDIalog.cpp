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

// Qt
#include <QDialogButtonBox>
#include <QPushButton>

qGAEVoxelDialog::qGAEVoxelDialog(QWidget* parent /*=nullptr*/)
    : QDialog(parent)
    , Ui::GAEVoxelDialog()
{
	setupUi(this);

	connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

int qGAEVoxelDialog::getResolution() const
{
	return intSpinBoxResolution->value();
}

int qGAEVoxelDialog::getConnectivity() const
{
	return comboBoxConnectivity->currentText().toInt();
}