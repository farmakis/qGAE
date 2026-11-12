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

//system
#include <cstdint>
#include <vector>

//CCCoreLib
#include <GenericProgressCallback.h>

//qPCP
#include <Partition.h>
#include <CutPursuit.h>

//Qt
#include <QMessageBox>

class Data;
class ccPointCloud;

//! Paritions a point cloud into voxels
class VoxelPartition
{
    public:
        //! Constructor
        explicit VoxelPartition(double voxelSize, int connectivity = 26);

        //! Run the process
        //! \return a new Data object owned by the caller (nullptr on failure)
        Data* Process(ccPointCloud* cloud, QString& errorMessage, CCCoreLib::GenericProgressCallback* progressCb=nullptr);

    protected:
        double m_voxelSize;
        int m_connectivity = 26; //!< connectivity of the voxel graph (6, 18 or 26)
};

//! Partitions a graph contained in a `Data` object using cut-pursuit.
class CutPursuitPartition
{
    public:
        //! Constructor
        explicit CutPursuitPartition(PCP::Parameters params);

        //! Run the process
        //! \return a new Data object (reduced graph) owned by the caller (nullptr on failure)
        Data* Process(Data& data, QString& errorMessage, CCCoreLib::GenericProgressCallback* progressCb=nullptr);

    protected:
        float               m_regularization;                   //!< regularization strength
        float               m_spatialWeight;                    //!< weight of spatial coordinates in the feature space
        int32_t             m_cutoff;                           //!< minimum component weight
        int32_t             m_D;                                //!< number of dimensions of the feature space
        std::vector<float>  m_Y;                                //!< feature matrix (size = number of points * D, row-major)
        float               m_difTol                = 0.01f;    //!< stopping criterion on relative energy difference
        int                 m_iterations            = 10;       //!< maximum number of cut-pursuit iterations
        int                 m_K                     = 2;        //!< number of alternative labels in each split
        int                 m_splitIterNum          = 2;        //!< number of iterations in each split
        float               m_splitDampRatio        = 0.7f;     //!< damping ratio of the split
        int                 m_kmppInitNum           = 3;        //!< number of k-means++ initializations
        int                 m_kmppIterNum           = 3;        //!< number of k-means++ iterations
        int                 m_verbose               = 1000;     //!< verbosity level
        bool                m_balanceParallelSplit  = false;    //!< balance the parallel split
        bool                m_computeTime           = true;     //!< compute and report timings
        bool                m_computeObj            = false;    //!< compute the objective value
        bool                m_computeDif            = false;    //!< compute the relative difference
        int                 m_maxNumThreads         = -1;       //!< maximum number of threads (-1: automatic)
};
