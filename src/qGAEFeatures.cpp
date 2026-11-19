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
#include "qGAEDialog.h"

// CCCoreLib
#include <Neighbourhood.h>
#include <GeometricalAnalysisTools.h>


QString GetFeatureSFName(CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic feature, int subOption)
{
    // generate the right SF name
    QString sfName;

    switch (feature)
    {
    case CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic::Feature:
    {
        switch (subOption)
        {
        case CCCoreLib::Neighbourhood::GeomFeature::EigenValuesSum:
            sfName = "Eigenvalues sum";
            break;
        case CCCoreLib::Neighbourhood::GeomFeature::Omnivariance:
            sfName = "Omnivariance";
            break;
        case CCCoreLib::Neighbourhood::GeomFeature::EigenEntropy:
            sfName = "Eigenentropy";
            break;
        case CCCoreLib::Neighbourhood::GeomFeature::Anisotropy:
            sfName = "Anisotropy";
            break;
        case CCCoreLib::Neighbourhood::GeomFeature::Planarity:
            sfName = "Planarity";
            break;
        case CCCoreLib::Neighbourhood::GeomFeature::Linearity:
            sfName = "Linearity";
            break;
        case CCCoreLib::Neighbourhood::GeomFeature::PCA1:
            sfName = "PCA1";
            break;
        case CCCoreLib::Neighbourhood::GeomFeature::PCA2:
            sfName = "PCA2";
            break;
        case CCCoreLib::Neighbourhood::GeomFeature::SurfaceVariation:
            sfName = "Surface variation";
            break;
        case CCCoreLib::Neighbourhood::GeomFeature::Sphericity:
            sfName = "Sphericity";
            break;
        case CCCoreLib::Neighbourhood::GeomFeature::Verticality:
            sfName = "Verticality";
            break;
        case CCCoreLib::Neighbourhood::GeomFeature::EigenValue1:
            sfName = "1st eigenvalue";
            break;
        case CCCoreLib::Neighbourhood::GeomFeature::EigenValue2:
            sfName = "2nd eigenvalue";
            break;
        case CCCoreLib::Neighbourhood::GeomFeature::EigenValue3:
            sfName = "3rd eigenvalue";
            break;
        case CCCoreLib::Neighbourhood::GeomFeature::DegreeOfPlanarity:
            sfName = "Degree of planarity (M)";
            break;
        case CCCoreLib::Neighbourhood::GeomFeature::DegreeOfLinearity:
            sfName = "Degree of linearity (K)";
            break;
        default:
            assert(false);
            return {};
        }
    }
    break;

    case CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic::Normal:
    {
        switch (subOption)
        {
        case CCCoreLib::Neighbourhood::NormalComponent::NormalX:
            sfName = "Nx";
            break;
        case CCCoreLib::Neighbourhood::NormalComponent::NormalY:
            sfName = "Ny";
            break;
        case CCCoreLib::Neighbourhood::NormalComponent::NormalZ:
            sfName = "Nz";
            break;
        default:
            assert(false);
            return {};
        }
    }
    break;

    case CCCoreLib::GeometricalAnalysisTools::GeomCharacteristic::LocalDensity:
        sfName = "Density";
        break;

    default:
        assert(false);
        return {};
    }

    return sfName;
}