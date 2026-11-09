//##########################################################################
//#                                                                        #
//#                CLOUDCOMPARE PLUGIN: qOBPA                              #
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

#include "qOBPALevel.h"


Data::Data(unsigned N, const QString& name)
    : ccPointCloud(name)
{
    m_valid = reserve(N);
}

Data::Data(const ccPointCloud& pc, const QString& name)
    : ccPointCloud(name.isEmpty() ? pc.getName() : name)
{
    // appending to an empty cloud copies points and all associated fields
    *this += const_cast<ccPointCloud*>(&pc);
    m_valid = (size() == pc.size());
}

int Data::addField(const QString& name)
{
    const std::string stdName = name.toStdString();
    int idx = getScalarFieldIndexByName(stdName);
    if (idx >= 0)
    {
        return idx;
    }

    // the scalar field is allocated with the currently reserved point count
    return addScalarField(stdName);
}

// bool Data::computeFeatures(GAT::MultiGeomCharacteristicRequestSet requests)
// {
//     std::string label = "Geometric characteristics computation";
// }