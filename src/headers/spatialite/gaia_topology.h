/*
 gaia_topology.h -- Gaia common support for Topology
  
 version 4.3, 2015 July 15

 Author: Sandro Furieri a.furieri@lqt.it

 ------------------------------------------------------------------------------
 
 Version: MPL 1.1/GPL 2.0/LGPL 2.1
 
 The contents of this file are subject to the Mozilla Public License Version
 1.1 (the "License"); you may not use this file except in compliance with
 the License. You may obtain a copy of the License at
 http://www.mozilla.org/MPL/
 
Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
for the specific language governing rights and limitations under the
License.

The Original Code is the SpatiaLite library

The Initial Developer of the Original Code is Alessandro Furieri
 
Portions created by the Initial Developer are Copyright (C) 2015
the Initial Developer. All Rights Reserved.

Contributor(s):

Alternatively, the contents of this file may be used under the terms of
either the GNU General Public License Version 2 or later (the "GPL"), or
the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
in which case the provisions of the GPL or the LGPL are applicable instead
of those above. If you wish to allow use of your version of this file only
under the terms of either the GPL or the LGPL, and not to allow others to
use your version of this file under the terms of the MPL, indicate your
decision by deleting the provisions above and replace them with the notice
and other provisions required by the GPL or the LGPL. If you do not delete
the provisions above, a recipient may use your version of this file under
the terms of any one of the MPL, the GPL or the LGPL.
 
*/


/**
 \file gaia_topology.h

 Topology handling functions and constants 
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS
/* stdio.h included for FILE objects. */
#include <stdio.h>
#ifdef DLL_EXPORT
#define GAIATOPO_DECLARE __declspec(dllexport)
#else
#define GAIATOPO_DECLARE extern
#endif
#endif

#ifndef _GAIATOPO_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _GAIATOPO_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/**
 Typedef for Topology Accessor Object (opaque, hidden)

 \sa GaiaTopologyAccessorPtr
 */
    typedef struct gaia_topology_accessor GaiaTopologyAccessor;
/**
 Typedef for Topology Accessor Object pointer (opaque, hidden)

 \sa GaiaTopologyAccessor
 */
    typedef struct gaia_topology_accessor *GaiaTopologyAccessorPtr;

/**
 returns the last Topology exception (if any)

 \param topo_name unique name identifying the Topology.
 
 \return pointer to the last Topology error message: may be NULL if
 no Topology error is currently pending.
 */
    GAIATOPO_DECLARE const char *gaiaTopologyGetLastException (const char
							       *topo_name);

/**
 creates a new Topology and all related DB objects

 \param handle pointer to the current DB connection.
 \param topo_name unique name identifying the Topology.
 \param srid a spatial reference identifier.
 \param tolerance a tolerance factor measuren in the same units defined
        by the associated SRID.
 \param has_z boolean: if TRUE this Topology supports 3D (XYZ).

 \return 0 on failure: any other value on success.

 \sa gaiaTopologyDrop
 */
    GAIATOPO_DECLARE int gaiaTopologyCreate (sqlite3 * handle,
					     const char *topo_name, int srid,
					     double tolerance, int has_z);

/**
 completely drops an already existing Topology and removes all related DB objects

 \param handle pointer to the current DB connection.
 \param topo_name unique name identifying the Topology.

 \return 0 on failure: any other value on success.

 \sa gaiaTopologyCreate
 */
    GAIATOPO_DECLARE int gaiaTopologyDrop (sqlite3 * handle,
					   const char *topo_name);

/**
 creates an opaque Topology Accessor object starting from its DB configuration

 \param handle pointer to the current DB connection.
 \param cache pointer to the opaque Cache Object supporting the DB connection
 \param topo_name unique name identifying the Topology.

 \return the pointer to newly created Topology Accessor Object: NULL on failure.

 \sa gaiaTopologyCreate, gaiaTopologyDestroy, gaiaTopologyFromCache, 
 gaiaGetTopology
 
 \note you are responsible to destroy (before or after) any allocated 
 Topology Accessor Object. The Topology Accessor once created will be
 preserved within the internal connection cache for future references.
 */
    GAIATOPO_DECLARE GaiaTopologyAccessorPtr gaiaTopologyFromDBMS (sqlite3 *
								   handle,
								   const void
								   *cache,
								   const char
								   *topo_name);

/**
 retrieves a Topology configuration from DB

 \param handle pointer to the current DB connection.
 \param cache pointer to the opaque Cache Object supporting the DB connection
 \param topo_name unique name identifying the Topology.
 \param topology_name on completion will point to the real Topology name.
 \param srid on completion will contain the Topology SRID.
 \param tolerance on completion will contain the tolerance argument.
 \param has_z on completion will report if the Topology is of the 3D type. 

 \return 1 on success: NULL on failure.

 \sa gaiaTopologyCreate, gaiaTopologyDestroy, gaiaTopologyFromCache, 
 gaiaGetTopology
 */
    GAIATOPO_DECLARE int gaiaReadTopologyFromDBMS (sqlite3 *
						   handle,
						   const char
						   *topo_name,
						   char **topology_name,
						   int *srid, double *tolerance,
						   int *has_z);

/**
 retrieves an already defined opaque Topology Accessor object from the
 internal connection cache

 \param cache pointer to the opaque Cache Object supporting the DB connection
 \param topo_name unique name identifying the Topology.

 \return pointer to an already existing Topology Accessor Object: NULL on failure.

 \sa gaiaTopologyCreate, gaiaTopologyDestroy, gaiaTopologyFromDBMS, 
 gaiaGetTopology
 */
    GAIATOPO_DECLARE GaiaTopologyAccessorPtr gaiaTopologyFromCache (const void
								    *cache,
								    const char
								    *topo_name);

/**
 will attempt to return a reference to a Topology Accessor object

 \param handle pointer to the current DB connection.
 \param cache pointer to the opaque Cache Object supporting the DB connection
 \param topo_name unique name identifying the Topology.

 \return pointer to Topology Accessor Object: NULL on failure.

 \sa gaiaTopologyCreate, gaiaTopologyDestroy, gaiaTopologyFromCache,
 gaiaTopologyFromDBMS
 
 \note if a corresponding Topology Accessor Object is already defined
 will return a pointer to such Objet. Otherwise an attempt will be made
 in order to create a new Topology Accessor object starting from its DB 
 configuration.
 */
    GAIATOPO_DECLARE GaiaTopologyAccessorPtr gaiaGetTopology (sqlite3 *
							      handle,
							      const void
							      *cache,
							      const char
							      *topo_name);

/**
 destroys a Topology Accessor object and any related memory allocation

 \param ptr pointer to the Topology Accessor Object to be destroyed.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE void gaiaTopologyDestroy (GaiaTopologyAccessorPtr ptr);

/**
 Adds an isolated node into the Topology

 \param ptr pointer to the Topology Accessor Object.
 \param face the unique identifier of containing face or -1 for "unknown".
 \param pt pointer to the Node Geometry.
 \param skip_checks boolean: if TRUE skips consistency checks
 (coincident nodes, crossing edges, actual face containement)

 \return the ID of the inserted Node; a negative number on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE sqlite3_int64 gaiaAddIsoNode (GaiaTopologyAccessorPtr ptr,
						   sqlite3_int64 face,
						   gaiaPointPtr pt,
						   int skip_checks);

/**
 Moves an isolated node in a Topology from one point to another

 \param ptr pointer to the Topology Accessor Object.
 \param node the unique identifier of node.
 \param pt pointer to the Node Geometry.

 \return 1 on success; 0 on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE int gaiaMoveIsoNode (GaiaTopologyAccessorPtr ptr,
					  sqlite3_int64 node, gaiaPointPtr pt);

/**
 Removes an isolated node from a Topology

 \param ptr pointer to the Topology Accessor Object.
 \param node the unique identifier of node.

 \return 1 on success; 0 on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE int gaiaRemIsoNode (GaiaTopologyAccessorPtr ptr,
					 sqlite3_int64 node);

/**
 Adds an isolated edge into the Topology

 \param ptr pointer to the Topology Accessor Object.
 \param start_node the Start Node's unique identifier.
 \param end_node the End Node unique identifier.
 \param ln pointer to the Edge Geometry.

 \return the ID of the inserted Edge; a negative number on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE sqlite3_int64 gaiaAddIsoEdge (GaiaTopologyAccessorPtr ptr,
						   sqlite3_int64 start_node,
						   sqlite3_int64 end_node,
						   gaiaLinestringPtr ln);

/**
 Changes the shape of an Edge without affecting the Topology structure

 \param ptr pointer to the Topology Accessor Object.
 \param edge_id the Edge unique identifier.
 \param ln pointer to the Edge Geometry.

 \return 1 on success; 0 on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE int gaiaChangeEdgeGeom (GaiaTopologyAccessorPtr ptr,
					     sqlite3_int64 edge_id,
					     gaiaLinestringPtr ln);

/**
 Split an edge by a node, modifying the original edge and adding a new one.

 \param ptr pointer to the Topology Accessor Object.
 \param edge the unique identifier of the edge to be split.
 \param pt pointer to the Node Geometry.
 \param skip_checks boolean: if TRUE skips consistency checks
 (coincident node, point not on edge...)

 \return the ID of the inserted Node; a negative number on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE sqlite3_int64 gaiaModEdgeSplit (GaiaTopologyAccessorPtr
						     ptr, sqlite3_int64 edge,
						     gaiaPointPtr pt,
						     int skip_checks);

/**
 Split an edge by a node, replacing it with two new edges.

 \param ptr pointer to the Topology Accessor Object.
 \param edge the unique identifier of the edge to be split.
 \param pt pointer to the Node Geometry.
 \param skip_checks boolean: if TRUE skips consistency checks
 (coincident node, point not on edge...)

 \return the ID of the inserted Node; a negative number on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE sqlite3_int64 gaiaNewEdgesSplit (GaiaTopologyAccessorPtr
						      ptr, sqlite3_int64 edge,
						      gaiaPointPtr pt,
						      int skip_checks);

/**
 Adds a new edge possibly splitting and modifying a face.

 \param ptr pointer to the Topology Accessor Object.
 \param start_node the unique identifier of the starting node.
 \param end_node the unique identifier of the ending node.
 \param ln pointer to the Edge Geometry.
 \param skip_checks boolean: if TRUE skips consistency checks
 (curve being simple and valid, start/end nodes, consistency 
 actual face containement)

 \return the ID of the inserted Edge; a negative number on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE sqlite3_int64 gaiaAddEdgeModFace (GaiaTopologyAccessorPtr
						       ptr,
						       sqlite3_int64 start_node,
						       sqlite3_int64 end_node,
						       gaiaLinestringPtr ln,
						       int skip_checks);

/**
 Adds a new edge possibly splitting a face (replacing with two new faces).

 \param ptr pointer to the Topology Accessor Object.
 \param start_node the unique identifier of the starting node.
 \param end_node the unique identifier of the ending node.
 \param ln pointer to the Edge Geometry.
 \param skip_checks boolean: if TRUE skips consistency checks
 (curve being simple and valid, start/end nodes, consistency 
 actual face containement)

 \return the ID of the inserted Edge; a negative number on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE sqlite3_int64 gaiaAddEdgeNewFaces (GaiaTopologyAccessorPtr
							ptr,
							sqlite3_int64
							start_node,
							sqlite3_int64 end_node,
							gaiaLinestringPtr ln,
							int skip_checks);

/**
 Removes an edge, and if the removed edge separated two faces, delete one
 of them and modify the other to take the space of both.

 \param ptr pointer to the Topology Accessor Object.
 \param edge_id the unique identifier of the edge to be removed.

 \return the ID of face that takes up the space previously occupied 
 by the removed edge; a negative number on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE sqlite3_int64 gaiaRemEdgeModFace (GaiaTopologyAccessorPtr
						       ptr,
						       sqlite3_int64 edge_id);

/**
 Removes an edge, and if the removed edge separated two faces, delete the
 original faces and replace them with a new face.

 \param ptr pointer to the Topology Accessor Object.
 \param edge_id the unique identifier of the edge to be removed.

 \return the ID of the created face; a negative number on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE sqlite3_int64 gaiaRemEdgeNewFace (GaiaTopologyAccessorPtr
						       ptr,
						       sqlite3_int64 edge_id);

/**
 Heal two edges by removing the node connecting them, modifying the
 first edge and removing the second edge

 \param ptr pointer to the Topology Accessor Object.
 \param edge_id1 the unique identifier of the first edge to be healed.
 \param edge_id2 the unique identifier of the second edge to be healed.

 \return the ID of the removed node.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE sqlite3_int64 gaiaModEdgeHeal (GaiaTopologyAccessorPtr
						    ptr,
						    sqlite3_int64 edge_id1,
						    sqlite3_int64 edge_id2);

/**
 Heal two edges by removing the node connecting them, deleting both edges
 and replacing them with an edge whose orientation is the same of the
 first edge provided

 \param ptr pointer to the Topology Accessor Object.
 \param edge_id1 the unique identifier of the first edge to be healed.
 \param edge_id2 the unique identifier of the second edge to be healed.

 \return the ID of the removed node.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE sqlite3_int64 gaiaNewEdgeHeal (GaiaTopologyAccessorPtr
						    ptr,
						    sqlite3_int64 edge_id1,
						    sqlite3_int64 edge_id2);

/**
 Return the geometry of a Topology Face

 \param ptr pointer to the Topology Accessor Object.
 \param face the unique identifier of the face.

 \return pointer to Geomtry (polygon); NULL on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE gaiaGeomCollPtr
	gaiaGetFaceGeometry (GaiaTopologyAccessorPtr ptr, sqlite3_int64 face);

/**
 Updates a temporary table containing the ordered list of Edges
 (in counterclockwise order) for every Face.
 EdgeIDs are signed value; a negative ID intends reverse direction.

 \param ptr pointer to the Topology Accessor Object.
 \param face the unique identifier of the face.

 \return 1 on success; 0 on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE int
	gaiaGetFaceEdges (GaiaTopologyAccessorPtr ptr, sqlite3_int64 face);


#ifdef __cplusplus
}
#endif


#endif				/* _GAIATOPO_H */