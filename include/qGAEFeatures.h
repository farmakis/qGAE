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

class ccScalarField;
class Features
{
public:

    enum Feature	{	Eigen,
                        Density, 
                        Normal
                    };

    enum NormalAxis {   Nx = 1,
                        Ny,
                        Nz
                    };

    struct FeatureData
    {
        Feature f;
        int subOption = 0;
        ccScalarField* sf = nullptr;
        int sfIdx = -1;

        bool operator==(const FeatureData& other) const	
        {		
            return f == other.f && subOption == other.subOption;	
        }
    };
};

QString GetFeatureSFName(Features::Feature feature, int subOption);
