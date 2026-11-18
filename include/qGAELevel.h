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

// local
#include "qGAEFeatures.h"

// qCC_db
#include <ccPointCloud.h>
#include <ccScalarField.h>

// system
#include <cstdint>
#include <vector>

// Qt
#include <QString>


//! Represents graph using a point cloud to manage nodes and their features
//! with additional properties for the edges
class Data : public ccPointCloud
{
    public:
        //! Constructor: reserves memory for N points
        explicit Data(unsigned N, const QString& name = QString());

        //! Constructor: copies points, scalar fields, colors and normals of an existing cloud
        explicit Data(const ccPointCloud& pc, const QString& name = QString());

        //! Adds (or retrieves) a scalar field by name, and reserves/resizes it
        /** \return the scalar field index, or -1 on failure **/
        int addField(const std::string& name);

        //! Whether the point memory was successfully reserved
        bool isValid() const { return m_valid; }

        //! Whethere has a sub index to the original points
        bool hasSub() const { return !sub.empty() || m_subPtr != nullptr; }

        void linkToOriginalPoints(const ccPointCloud* cloud);

        //! Returns the original (finer) point cloud this level was built from (if any)
        const ccPointCloud* getOriginalCloud() const { return m_subPtr; }

        /** getter functions */ 

        //! Returns the number of nodes in the graph
        int32_t numNodes() const { return static_cast<int32_t>(size()); }
        //! Returns the number of edges in the graph
        int32_t numEdges() const { return static_cast<int32_t>(edges.size() / 2); }
        //! Returns the number of scalar fields stored in this cloud
        size_t numFeatures() const { return features.size(); }
        //! Returns a pointer to the scalar field that holds the feature by index in the `features` vector
        ccScalarField* getFeature(const int index);

        /* convert edge list to forward-star representation */
		/* adapted from the edge_list_to_forward_star().cpp file of the
		 * grid-graph project by Hugo Raguet (https://github.com/1a7r0ch3/grid-graph) */
        void edgeListToForwardStar(int32_t* firstEdge, int32_t* target, int32_t* reindex) const;
        /* uses the cloud size as V and the 'edges' member (pairs of vertex indices) as E */
        /* firstEdge is an array of length V + 1, already allocated;
		 * reindex is the permutation indices so that all edges starting from a
		 * same vertex are consecutive, array of length E, already allocated;
		 * adj_vertices can be thus deduced from the edges by permuting the ending
		 * vertices according to reindex */

    protected:
        bool m_valid = false;
        const ccPointCloud* m_subPtr = nullptr; //!< point cloud to hold the original points

    public:
        std::vector<int32_t> edges;
        std::vector<float> nodeSize;
        std::vector<float> edgeAttr;
        std::vector<int32_t> sub; //stores the index of the current partition to the lower points
        std::vector<int32_t> superIndex;
        std::vector<int32_t> clusterList; //stores nodes indices per cluster in the graph
        std::vector<int32_t> subPointer;  //points to the start of the cluster in the sub levels clusterList

        /* set of requests for computing features */
        std::vector<Features::FeatureData> features;
};

// class Segment : CCCoreLib::Neighboirhood
// {
//     public:
//         explicit Segment()s
// }