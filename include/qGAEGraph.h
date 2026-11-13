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

//! Nested Acyclic Graph structure. In practise a list of Data objects of graphs
//! with super indices to map from lower to higher levels.
class NAG : public std::vector<Data*>
{
    public:
        NAG() = default;
        explicit NAG(std::vector<Data*> dataList);

        //! Returns the super index mapping from level `low` to level `high`
        std::vector<int> getSuperIndex(int high, int low = 0);

        bool addLevel(Data* data);
};