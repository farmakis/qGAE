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

// Qt
#include <QString>

// CCCoreLib
#include <Neighbourhood.h>
#include <GeometricalAnalysisTools.h>

// System
#include <vector>

class ccScalarField;

//! Geometric characteristic (with sub option), as selected in the UI
struct GeomCharacteristicWithSubOption
{
	GeomCharacteristicWithSubOption(CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic c = CCCoreLib::GeometricalAnalysisTools::Feature, int option = 0)
	    : charac(c)
	    , subOption(option)
	{
	}

	bool operator==(const GeomCharacteristicWithSubOption& other) const
	{
		return charac == other.charac && subOption == other.subOption;
	}

	CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic charac;        //!< Geometric characteristic
	int                                                     subOption = 0; //!< Sub option (if any)
};

//! Set of GeomCharacteristicWithSubOption instances
using GeomCharacteristicWithSubOptionSet = std::vector<GeomCharacteristicWithSubOption>;

QString GetFeatureSFName(CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic feature, int subOption);
