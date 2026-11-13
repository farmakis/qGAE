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

#include "qGAEGraph.h"

//system
#include <utility>
#include <vector>


NAG::NAG(std::vector<Data*> dataList)
    : std::vector<Data*>(std::move(dataList))
{
}

std::vector<int> NAG::getSuperIndex(int high, int low)
{ 
    assert(low >= 0 || high < static_cast<int>(size()) || low < high);

    // start with the superIndex of the lowest level
    std::vector<int> superIndex = (*this)[low]->superIndex;
    for (int level = low + 1; level < high; ++level) 
    {
        for (std::size_t i = 0; i < superIndex.size(); ++i)
        {
            superIndex[i] = (*this)[level]->superIndex[superIndex[i]];
        }
    }
    return superIndex;
}

bool NAG::addLevel(Data* data)
{
    push_back(data);
    if (!(size() > 1))
    {
        // If this was the first level added, we don't need to do anything else
        return true;
    }

    const std::string levelName = "Level " + std::to_string(size() - 1);
    data->setName(levelName.c_str());
    
    // Every time a new level is added, we need to create the scalar field
    // to map the labels at the base level (d0) for visualization purposes.
    int sfIdx = (*this)[0]->getScalarFieldIndexByName(levelName);
    if (sfIdx < 0)
    {
        sfIdx = (*this)[0]->addScalarField(levelName);
    }
    if (sfIdx < 0)
    {
        return false;
    }
    (*this)[0]->setCurrentDisplayedScalarField(sfIdx);
    ccScalarField::Shared sf = (*this)[0]->getCCScalarField(sfIdx);

    // map partition labels from the new level to the base level's scalar field
    int high = static_cast<int>(size()) - 1;
    int low = 0; // always map from the base level
    std::vector<int> superIndex = getSuperIndex(high, low);
    for (unsigned i = 0; i < (*this)[0]->size(); ++i)
    {
        sf->setValue(i, static_cast<ScalarType>(superIndex[i]));
    }
    sf->computeMinAndMax();

    return true;
}