//##########################################################################
//#                                                                        #
//#                CLOUDCOMPARE PLUGIN: qGAE                              #
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

#include "qGAELevel.h"
#include "qGAEFeatures.h"


//! Nested Acyclic Graph structure. In practise a list of Data objects of graphs
//! with super indices to map from lower to higher levels.
class NAG : public std::vector<Data*>
{
    public:
        NAG() = default;
        explicit NAG(std::vector<Data*> dataList);

        //! Returns the super index mapping from level `low` to level `high`
        std::vector<int32_t> getSuperIndex(int high, int low = 0);

        bool addLevel(Data* data);

        int numLevels() const { return static_cast<int>(size()); }

        bool setLevelFeatures(const CCCoreLib::GeometricalAnalysisTools::MultiGeomCharacteristicRequestSet& selectedFeatures, int level);

        bool computeLevelFeatures(int level, int subLevel = 0, const ccProgressDialog* pDlg = nullptr);

        bool mapFeaturesToLowerLevel(int high, int low, const ccProgressDialog* pDlg = nullptr);

    private:
        std::vector<float> getSubSize(int high, int low = 0);
};