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

#pragma once

// qCC_db
#include <ccPointCloud.h>

// CCCoreLib
#include "GeometricalAnalysisTools.h"

// Qt
#include <QString>

using GAT = CCCoreLib::GeometricalAnalysisTools;

//! Point cloud with N reserved points and helpers to compute sub and super lebel features
class Data : public ccPointCloud
{
    public:
        //! Constructor: reserves memory for N points
        explicit Data(unsigned N, const QString& name = QString());

        //! Constructor: copies points, scalar fields, colors and normals of an existing cloud
        explicit Data(const ccPointCloud& pc, const QString& name = QString());

        //! Adds (or retrieves) a scalar field by name, and reserves/resizes it
        /** \return the scalar field index, or -1 on failure **/
        int addField(const QString& name);

        //! Whether the point memory was successfully reserved
        bool isValid() const { return m_valid; }

        // bool computeFeatures(GAT::MultiGeomCharacteristicRequestSet requests);

    protected:
        bool m_valid = false;

    public:
        std::vector<int32_t> superIndex;
        // std::vector<int32_t> compList;
        // std::vector<int32_t> compOffset;
};

// class Segment : CCCoreLib::Neighboirhood
// {
//     public:
//         explicit Segment()
// }