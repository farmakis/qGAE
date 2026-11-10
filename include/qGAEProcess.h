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

//Local
#include "qGAEDialog.h"

//qCC
#include "ccPointCloud.h"

class ccMainAppInterface;

//! GAE process
/** See "Graph-Based Discontinuity Surface Isolation from 3D Point Clouds", 
    Farmakis, I.; Dewez, T.; Dyson, C., 2027, 
    ISPRS Journal of Photogrammetry and Remote Sensing"
**/
class qGAEProcess
{
public:
	
	static bool Compute(const qGAEDialog& dlg,
						QString& errorMessage,
						ccPointCloud*& outputCloud,
						ccHObject*& outputGroup,
						bool allowDialogs,
						QWidget* parentWidget = nullptr,
						ccMainAppInterface* app = nullptr);

};