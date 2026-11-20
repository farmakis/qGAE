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

#include "qGAECutPursuitDialog.h"

// Qt
#include <QDialogButtonBox>
#include <QPushButton>

qGAECutPursuitDialog::qGAECutPursuitDialog(QWidget* parent /*=nullptr*/)
    : QDialog(parent)
    , Ui::GAECutPursuitDialog()
{
	setupUi(this);
}

double qGAECutPursuitDialog::getRegularization() const
{
	return regularizationDoubleSpinBox->value();
}

double qGAECutPursuitDialog::getSpatialWeight() const
{
	return spatialWeightDoubleSpinBox->value();
}

int qGAECutPursuitDialog::getCutoff() const
{
	return cutoffIntSpinBox->value();
}