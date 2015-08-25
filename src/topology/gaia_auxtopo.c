/*

 gaia_auxtopo.c -- implementation of the Topology module methods
    
 version 4.3, 2015 July 15

 Author: Sandro Furieri a.furieri@lqt.it

 -----------------------------------------------------------------------------
 
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

/*
 
CREDITS:

this module has been completely funded by:
Regione Toscana - Settore Sistema Informativo Territoriale ed Ambientale
(Topology support) 

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#ifdef POSTGIS_2_2		/* only if TOPOLOGY is enabled */

#include <spatialite/sqlite.h>
#include <spatialite/debug.h>
#include <spatialite/gaiageo.h>
#include <spatialite/gaia_topology.h>
#include <spatialite/gaiaaux.h>

#include <spatialite.h>
#include <spatialite_private.h>

#include <liblwgeom.h>
#include <liblwgeom_topo.h>

#include "topology_private.h"

#define GAIA_UNUSED() if (argc || argv) argc = argc;

SPATIALITE_PRIVATE void
free_internal_cache_topologies (void *firstTopology)
{
/* destroying all Topologies registered into the Internal Connection Cache */
    struct gaia_topology *p_topo = (struct gaia_topology *) firstTopology;
    struct gaia_topology *p_topo_n;

    while (p_topo != NULL)
      {
	  p_topo_n = p_topo->next;
	  gaiaTopologyDestroy ((GaiaTopologyAccessorPtr) p_topo);
	  p_topo = p_topo_n;
      }
}

static int
do_create_topologies (sqlite3 * handle)
{
/* attempting to create the Topologies table (if not already existing) */
    const char *sql;
    char *err_msg = NULL;
    int ret;

    sql = "CREATE TABLE IF NOT EXISTS topologies (\n"
	"\ttopology_name TEXT NOT NULL PRIMARY KEY,\n"
	"\tsrid INTEGER NOT NULL,\n"
	"\ttolerance DOUBLE NOT NULL,\n"
	"\thas_z INTEGER NOT NULL,\n"
	"\tnext_node_id INTEGER NOT NULL DEFAULT 1,\n"
	"\tnext_edge_id INTEGER NOT NULL DEFAULT 1,\n"
	"\tnext_face_id INTEGER NOT NULL DEFAULT 1,\n"
	"\tCONSTRAINT topo_srid_fk FOREIGN KEY (srid) "
	"REFERENCES spatial_ref_sys (srid))";
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE topologies - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating Topologies triggers */
    sql = "CREATE TRIGGER IF NOT EXISTS topology_name_insert\n"
	"BEFORE INSERT ON 'topologies'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on topologies violates constraint: "
	"topology_name value must not contain a single quote')\n"
	"WHERE NEW.topology_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'insert on topologies violates constraint: "
	"topology_name value must not contain a double quote')\n"
	"WHERE NEW.topology_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'insert on topologies violates constraint: "
	"topology_name value must be lower case')\n"
	"WHERE NEW.topology_name <> lower(NEW.topology_name);\nEND";
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER IF NOT EXISTS topology_name_update\n"
	"BEFORE UPDATE OF 'topology_name' ON 'topologies'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'update on topologies violates constraint: "
	"topology_name value must not contain a single quote')\n"
	"WHERE NEW.topology_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'update on topologies violates constraint: "
	"topology_name value must not contain a double quote')\n"
	"WHERE NEW.topology_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'update on topologies violates constraint: "
	"topology_name value must be lower case')\n"
	"WHERE NEW.topology_name <> lower(NEW.topology_name);\nEND";
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
check_new_topology (sqlite3 * handle, const char *topo_name)
{
/* testing if some already defined DB object forbids creating the new Topology */
    char *sql;
    char *prev;
    char *table;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    const char *value;
    int error = 0;

/* testing if the same Topology is already defined */
    sql = sqlite3_mprintf ("SELECT Count(*) FROM topologies WHERE "
			   "Lower(topology_name) = Lower(%Q)", topo_name);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) != 0)
		    error = 1;
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;

/* testing if some table/geom is already defined in geometry_columns */
    sql = sqlite3_mprintf ("SELECT Count(*) FROM geometry_columns WHERE");
    prev = sql;
    table = sqlite3_mprintf ("%s_node", topo_name);
    sql =
	sqlite3_mprintf
	("%s (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'geom')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_edge", topo_name);
    sql =
	sqlite3_mprintf
	("%s OR (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'geom')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) != 0)
		    error = 1;
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;

/* testing if some table is already defined */
    sql = sqlite3_mprintf ("SELECT Count(*) FROM sqlite_master WHERE");
    prev = sql;
    table = sqlite3_mprintf ("%s_node", topo_name);
    sql = sqlite3_mprintf ("%s Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_edge", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_face", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("idx_%s_node_geom", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("idx_%s_edge_geom", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("idx_%s_face_rtree", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) != 0)
		    error = 1;
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;

    return 1;
}

static int
do_create_face (sqlite3 * handle, const char *topo_name)
{
/* attempting to create the Topology Face table */
    char *sql;
    char *table;
    char *xtable;
    char *trigger;
    char *xtrigger;
    char *rtree;
    char *xrtree;
    char *err_msg = NULL;
    int ret;

/* creating the main table */
    table = sqlite3_mprintf ("%s_face", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" (\n"
			   "\tface_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "\tmin_x DOUBLE,\n\tmin_y DOUBLE,\n"
			   "\tmax_x DOUBLE,\n\tmax_y DOUBLE)", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE topology-FACE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating the exotic spatial index */
    table = sqlite3_mprintf ("idx_%s_face_rtree", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE VIRTUAL TABLE \"%s\" USING RTree "
			   "(id_face, x_min, x_max, y_min, y_max)", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE topology-FACE Spatiale index - error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "face_insert" trigger */
    trigger = sqlite3_mprintf ("%s_face_insert", topo_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_face", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    rtree = sqlite3_mprintf ("idx_%s_face_rtree", topo_name);
    xrtree = gaiaDoubleQuotedSql (rtree);
    sqlite3_free (rtree);
    sql = sqlite3_mprintf ("CREATE TRIGGER \"%s\" AFTER INSERT ON \"%s\"\n"
			   "FOR EACH ROW BEGIN\n"
			   "\tUPDATE topologies SET next_face_id = NEW.face_id + 1 "
			   "WHERE Lower(topology_name) = Lower(%Q) AND next_face_id < NEW.face_id + 1;\n"
			   "\tINSERT OR REPLACE INTO \"%s\" (id_face, x_min, x_max, y_min, y_max) "
			   "VALUES (NEW.face_id, NEW.min_x, NEW.max_x, NEW.min_y, NEW.max_y);\n"
			   "DELETE FROM \"%s\" WHERE id_face = NEW.face_id AND NEW.face_id = 0;\n"
			   "END", xtrigger, xtable, topo_name, xrtree, xrtree);
    free (xtrigger);
    free (xtable);
    free (xrtree);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TRIGGER topology-FACE next INSERT - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "face_update_id" trigger */
    trigger = sqlite3_mprintf ("%s_face_update_id", topo_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_face", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("CREATE TRIGGER \"%s\" AFTER UPDATE OF face_id ON \"%s\"\n"
	 "FOR EACH ROW BEGIN\n"
	 "\tUPDATE topologies SET next_face_id = NEW.face_id + 1 "
	 "WHERE Lower(topology_name) = Lower(%Q) AND next_face_id < NEW.face_id + 1;\n"
	 "END", xtrigger, xtable, topo_name);
    free (xtrigger);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TRIGGER topology-FACE next UPDATE - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "face_update_mbr" trigger */
    trigger = sqlite3_mprintf ("%s_face_update_mbr", topo_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_face", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    rtree = sqlite3_mprintf ("idx_%s_face_rtree", topo_name);
    xrtree = gaiaDoubleQuotedSql (rtree);
    sql =
	sqlite3_mprintf
	("CREATE TRIGGER \"%s\" AFTER UPDATE OF min_x, min_y, max_x, max_y ON \"%s\"\n"
	 "FOR EACH ROW BEGIN\n"
	 "\tINSERT OR REPLACE INTO \"%s\" (id_face, x_min, x_max, y_min, y_max) "
	 "VALUES (NEW.face_id, NEW.min_x, NEW.max_x, NEW.min_y, NEW.max_y);\n"
	 "DELETE FROM \"%s\" WHERE id_face = NEW.face_id AND NEW.face_id = 0;\n"
	 "END", xtrigger, xtable, xrtree, xrtree);
    free (xtrigger);
    free (xtable);
    free (xrtree);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TRIGGER topology-FACE next UPDATE - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "face_deleter" trigger */
    trigger = sqlite3_mprintf ("%s_face_delete", topo_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_face", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    rtree = sqlite3_mprintf ("idx_%s_face_rtree", topo_name);
    xrtree = gaiaDoubleQuotedSql (rtree);
    sql = sqlite3_mprintf ("CREATE TRIGGER \"%s\" AFTER DELETE ON \"%s\"\n"
			   "FOR EACH ROW BEGIN\n"
			   "\tDELETE FROM \"%s\" WHERE id_face = OLD.face_id;\n"
			   "END", xtrigger, xtable, xrtree);
    free (xtrigger);
    free (xtable);
    free (xrtree);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TRIGGER topology-FACE next UPDATE - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* inserting the default World Face */
    table = sqlite3_mprintf ("%s_face", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" VALUES (0, NULL, NULL, NULL, NULL)", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("INSERT WorldFACE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_create_node (sqlite3 * handle, const char *topo_name, int srid, int has_z)
{
/* attempting to create the Topology Node table */
    char *sql;
    char *table;
    char *xtable;
    char *xconstraint;
    char *xmother;
    char *trigger;
    char *xtrigger;
    char *err_msg = NULL;
    int ret;

/* creating the main table */
    table = sqlite3_mprintf ("%s_node", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_node_face_fk", topo_name);
    xconstraint = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_face", topo_name);
    xmother = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" (\n"
			   "\tnode_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "\tcontaining_face INTEGER,\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (containing_face) "
			   "REFERENCES \"%s\" (face_id))", xtable, xconstraint,
			   xmother);
    free (xtable);
    free (xconstraint);
    free (xmother);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE topology-NODE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "next_node_ins" trigger */
    trigger = sqlite3_mprintf ("%s_node_next_ins", topo_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_node", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TRIGGER \"%s\" AFTER INSERT ON \"%s\"\n"
			   "FOR EACH ROW BEGIN\n"
			   "\tUPDATE topologies SET next_node_id = NEW.node_id + 1 "
			   "WHERE Lower(topology_name) = Lower(%Q) AND next_node_id < NEW.node_id + 1;\n"
			   "END", xtrigger, xtable, topo_name);
    free (xtrigger);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TRIGGER topology-NODE next INSERT - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "next_node_upd" trigger */
    trigger = sqlite3_mprintf ("%s_node_next_upd", topo_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_node", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("CREATE TRIGGER \"%s\" AFTER UPDATE OF node_id ON \"%s\"\n"
	 "FOR EACH ROW BEGIN\n"
	 "\tUPDATE topologies SET next_node_id = NEW.node_id + 1 "
	 "WHERE Lower(topology_name) = Lower(%Q) AND next_node_id < NEW.node_id + 1;\n"
	 "END", xtrigger, xtable, topo_name);
    free (xtrigger);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TRIGGER topology-NODE next UPDATE - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating the Node Geometry */
    table = sqlite3_mprintf ("%s_node", topo_name);
    sql =
	sqlite3_mprintf
	("SELECT AddGeometryColumn(%Q, 'geom', %d, 'POINT', %Q, 1)", table,
	 srid, has_z ? "XYZ" : "XY");
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("AddGeometryColumn topology-NODE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating a Spatial Index supporting Node Geometry */
    table = sqlite3_mprintf ("%s_node", topo_name);
    sql = sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'geom')", table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CreateSpatialIndex topology-NODE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "containing_face" */
    table = sqlite3_mprintf ("%s_node", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_node_contface", topo_name);
    xconstraint = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (containing_face)",
			 xconstraint, xtable);
    free (xtable);
    free (xconstraint);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX node-contface - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_create_edge (sqlite3 * handle, const char *topo_name, int srid, int has_z)
{
/* attempting to create the Topology Edge table */
    char *sql;
    char *table;
    char *xtable;
    char *xconstraint1;
    char *xconstraint2;
    char *xconstraint3;
    char *xconstraint4;
    char *xconstraint5;
    char *xconstraint6;
    char *xnodes;
    char *xfaces;
    char *xedges;
    char *trigger;
    char *xtrigger;
    char *err_msg = NULL;
    int ret;

/* creating the main table */
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge_node_start_fk", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge_node_end_fk", topo_name);
    xconstraint2 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge_face_left_fk", topo_name);
    xconstraint3 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge_face_right_fk", topo_name);
    xconstraint4 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge_edge_left_fk", topo_name);
    xconstraint5 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge_edge_right_fk", topo_name);
    xconstraint6 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_node", topo_name);
    xnodes = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_face", topo_name);
    xfaces = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xedges = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" (\n"
			   "\tedge_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "\tstart_node INTEGER NOT NULL,\n"
			   "\tend_node INTEGER NOT NULL,\n"
			   "\tnext_left_edge INTEGER NOT NULL,\n"
			   "\tabs_next_left_edge INTEGER,\n"
			   "\tnext_right_edge INTEGER NOT NULL,\n"
			   "\tabs_next_right_edge INTEGER,\n"
			   "\tleft_face INTEGER NOT NULL,\n"
			   "\tright_face INTEGER NOT NULL,\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (start_node) "
			   "REFERENCES \"%s\" (node_id),\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (end_node) "
			   "REFERENCES \"%s\" (node_id),\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (abs_next_left_edge) "
			   "REFERENCES \"%s\" (edge_id) DEFERRABLE INITIALLY DEFERRED,\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (abs_next_right_edge) "
			   "REFERENCES \"%s\" (edge_id) DEFERRABLE INITIALLY DEFERRED,\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (left_face) "
			   "REFERENCES \"%s\" (face_id),\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (right_face) "
			   "REFERENCES \"%s\" (face_id))",
			   xtable, xconstraint1, xnodes, xconstraint2, xnodes,
			   xconstraint5, xedges, xconstraint6, xedges,
			   xconstraint3, xfaces, xconstraint4, xfaces);
    free (xtable);
    free (xconstraint1);
    free (xconstraint2);
    free (xconstraint3);
    free (xconstraint4);
    free (xconstraint5);
    free (xconstraint6);
    free (xnodes);
    free (xfaces);
    free (xedges);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE topology-EDGE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "next_edge_ins" trigger */
    trigger = sqlite3_mprintf ("%s_edge_next_ins", topo_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TRIGGER \"%s\" AFTER INSERT ON \"%s\"\n"
			   "FOR EACH ROW BEGIN\n"
			   "\tUPDATE topologies SET next_edge_id = NEW.edge_id + 1 "
			   "WHERE Lower(topology_name) = Lower(%Q) AND next_edge_id < NEW.edge_id + 1;\n"
			   "END", xtrigger, xtable, topo_name);
    free (xtrigger);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TRIGGER topology-EDGE next INSERT - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "next_edge_upd" trigger */
    trigger = sqlite3_mprintf ("%s_edge_next_upd", topo_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("CREATE TRIGGER \"%s\" AFTER UPDATE OF edge_id ON \"%s\"\n"
	 "FOR EACH ROW BEGIN\n"
	 "\tUPDATE topologies SET next_edge_id = NEW.edge_id + 1 "
	 "WHERE Lower(topology_name) = Lower(%Q) AND next_edge_id < NEW.edge_id + 1;\n"
	 "END", xtrigger, xtable, topo_name);
    free (xtrigger);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TRIGGER topology-EDGE next UPDATE - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "abs_next_ins" trigger */
    trigger = sqlite3_mprintf ("%s_abs_next_ins", topo_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("CREATE TRIGGER \"%s\" AFTER INSERT ON \"%s\"\n"
	 "FOR EACH ROW BEGIN\n"
	 "UPDATE \"%s\" SET abs_next_left_edge = ABS(NEW.next_left_edge), "
	 "abs_next_right_edge = ABS(NEW.next_right_edge) WHERE edge_id = NEW.edge_id;\n"
	 "END", xtrigger, xtable, xtable);
    free (xtrigger);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TRIGGER topology-EDGE abs_next INSERT - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "abs_next_upd" trigger */
    trigger = sqlite3_mprintf ("%s_abs_next_upd", topo_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("CREATE TRIGGER \"%s\" AFTER UPDATE OF next_left_edge, next_right_edge, "
	 "abs_next_left_edge, abs_next_right_edge ON \"%s\"\n"
	 "FOR EACH ROW BEGIN\n"
	 "UPDATE \"%s\" SET abs_next_left_edge = ABS(NEW.next_left_edge), "
	 "abs_next_right_edge = ABS(NEW.next_right_edge) WHERE edge_id = NEW.edge_id;\n"
	 "END", xtrigger, xtable, xtable);
    free (xtrigger);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TRIGGER topology-EDGE abs_next UPDATE - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating the Edge Geometry */
    table = sqlite3_mprintf ("%s_edge", topo_name);
    sql =
	sqlite3_mprintf
	("SELECT AddGeometryColumn(%Q, 'geom', %d, 'LINESTRING', %Q, 1)",
	 table, srid, has_z ? "XYZ" : "XY");
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("AddGeometryColumn topology-EDGE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating a Spatial Index supporting Edge Geometry */
    table = sqlite3_mprintf ("%s_edge", topo_name);
    sql = sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'geom')", table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CreateSpatialIndex topology-EDGE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "start_node" */
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_start_node", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (start_node)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX edge-startnode - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "end_node" */
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_end_node", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (end_node)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX edge-endnode - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "left_face" */
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_edge_leftface", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (left_face)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX edge-leftface - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "right_face" */
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_edge_rightface", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (right_face)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX edge-rightface - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "next_left_edge" */
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_edge_nextleftedge", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (next_left_edge)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE INDEX edge-nextleftedge - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "next_right_edge" */
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_edge_nextrightedge", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (next_right_edge)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE INDEX edge-nextrightedge - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

GAIATOPO_DECLARE int
gaiaTopologyCreate (sqlite3 * handle, const char *topo_name, int srid,
		    double tolerance, int has_z)
{
/* attempting to create a new Topology */
    int ret;
    char *sql;

/* creating the Topologies table (just in case) */
    if (!do_create_topologies (handle))
	return 0;

/* testing for forbidding objects */
    if (!check_new_topology (handle, topo_name))
	return 0;

/* creating the Topology own Tables */
    if (!do_create_face (handle, topo_name))
	goto error;
    if (!do_create_node (handle, topo_name, srid, has_z))
	goto error;
    if (!do_create_edge (handle, topo_name, srid, has_z))
	goto error;

/* registering the Topology */
    sql = sqlite3_mprintf ("INSERT INTO topologies (topology_name, "
			   "srid, tolerance, has_z) VALUES (Lower(%Q), %d, %f, %d)",
			   topo_name, srid, tolerance, has_z);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;

    return 1;

  error:
    return 0;
}

static int
check_existing_topology (sqlite3 * handle, const char *topo_name,
			 int full_check)
{
/* testing if a Topology is already defined */
    char *sql;
    char *prev;
    char *table;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    const char *value;
    int error = 0;

/* testing if the Topology is already defined */
    sql = sqlite3_mprintf ("SELECT Count(*) FROM topologies WHERE "
			   "Lower(topology_name) = Lower(%Q)", topo_name);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) != 1)
		    error = 1;
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;
    if (!full_check)
	return 1;

/* testing if all table/geom are correctly defined in geometry_columns */
    sql = sqlite3_mprintf ("SELECT Count(*) FROM geometry_columns WHERE");
    prev = sql;
    table = sqlite3_mprintf ("%s_node", topo_name);
    sql =
	sqlite3_mprintf
	("%s (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'geom')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_edge", topo_name);
    sql =
	sqlite3_mprintf
	("%s OR (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'geom')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) != 2)
		    error = 1;
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;

/* testing if all tables are already defined */
    sql =
	sqlite3_mprintf
	("SELECT Count(*) FROM sqlite_master WHERE type = 'table' AND (");
    prev = sql;
    table = sqlite3_mprintf ("%s_node", topo_name);
    sql = sqlite3_mprintf ("%s Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_edge", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_face", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("idx_%s_node_geom", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("idx_%s_edge_geom", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("idx_%s_face_rtree", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q))", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) != 6)
		    error = 1;
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;

    return 1;
}

static int
do_drop_topo_face (sqlite3 * handle, const char *topo_name)
{
/* attempting to drop the Topology-Face table */
    char *sql;
    char *table;
    char *xtable;
    char *err_msg = NULL;
    int ret;

/* dropping the main table */
    table = sqlite3_mprintf ("%s_face", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DROP TABLE IF EXISTS \"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("DROP topology-face - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* dropping the corresponding Spatial Index */
    table = sqlite3_mprintf ("idx_%s_%s_rtree", topo_name);
    sql = sqlite3_mprintf ("DROP TABLE IF EXISTS \"%s\"", table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("DROP SpatialIndex topology-face - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_drop_topo_table (sqlite3 * handle, const char *topo_name, const char *which)
{
/* attempting to drop some Topology table */
    char *sql;
    char *table;
    char *xtable;
    char *err_msg = NULL;
    int ret;

    if (strcmp (which, "face") == 0)
	return do_drop_topo_face (handle, topo_name);

/* disabling the corresponding Spatial Index */
    table = sqlite3_mprintf ("%s_%s", topo_name, which);
    sql = sqlite3_mprintf ("SELECT DisableSpatialIndex(%Q, 'geom')", table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("DisableSpatialIndex topology-%s - error: %s\n", which, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* discarding the Geometry column */
    table = sqlite3_mprintf ("%s_%s", topo_name, which);
    sql = sqlite3_mprintf ("SELECT DiscardGeometryColumn(%Q, 'geom')", table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("DisableGeometryColumn topology-%s - error: %s\n", which,
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* dropping the main table */
    table = sqlite3_mprintf ("%s_%s", topo_name, which);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DROP TABLE IF EXISTS \"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("DROP topology-%s - error: %s\n", which, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* dropping the corresponding Spatial Index */
    table = sqlite3_mprintf ("idx_%s_%s_geom", topo_name, which);
    sql = sqlite3_mprintf ("DROP TABLE IF EXISTS \"%s\"", table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("DROP SpatialIndex topology-%s - error: %s\n", which, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_get_topology (sqlite3 * handle, const char *topo_name, char **topology_name,
		 int *srid, double *tolerance, int *has_z)
{
/* retrieving a Topology configuration */
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    int ok = 0;
    char *xtopology_name = NULL;
    int xsrid;
    double xtolerance;
    int xhas_z;

/* preparing the SQL query */
    sql =
	sqlite3_mprintf
	("SELECT topology_name, srid, tolerance, has_z FROM topologies WHERE "
	 "Lower(topology_name) = Lower(%Q)", topo_name);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SELECT FROM topologys error: \"%s\"\n",
			sqlite3_errmsg (handle));
	  return 0;
      }

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int ok_name = 0;
		int ok_srid = 0;
		int ok_tolerance = 0;
		int ok_z = 0;
		if (sqlite3_column_type (stmt, 0) == SQLITE_TEXT)
		  {
		      const char *str =
			  (const char *) sqlite3_column_text (stmt, 0);
		      if (xtopology_name != NULL)
			  free (xtopology_name);
		      xtopology_name = malloc (strlen (str) + 1);
		      strcpy (xtopology_name, str);
		      ok_name = 1;
		  }
		if (sqlite3_column_type (stmt, 1) == SQLITE_INTEGER)
		  {
		      xsrid = sqlite3_column_int (stmt, 1);
		      ok_srid = 1;
		  }
		if (sqlite3_column_type (stmt, 2) == SQLITE_FLOAT)
		  {
		      xtolerance = sqlite3_column_double (stmt, 2);
		      ok_tolerance = 1;
		  }
		if (sqlite3_column_type (stmt, 3) == SQLITE_INTEGER)
		  {
		      xhas_z = sqlite3_column_int (stmt, 3);
		      ok_z = 1;
		  }
		if (ok_name && ok_srid && ok_tolerance && ok_z)
		  {
		      ok = 1;
		      break;
		  }
	    }
	  else
	    {
		spatialite_e
		    ("step: SELECT FROM topologies error: \"%s\"\n",
		     sqlite3_errmsg (handle));
		sqlite3_finalize (stmt);
		return 0;
	    }
      }
    sqlite3_finalize (stmt);

    if (ok)
      {
	  *topology_name = xtopology_name;
	  *srid = xsrid;
	  *tolerance = xtolerance;
	  *has_z = xhas_z;
	  return 1;
      }

    if (xtopology_name != NULL)
	free (xtopology_name);
    return 0;
}

GAIATOPO_DECLARE GaiaTopologyAccessorPtr
gaiaGetTopology (sqlite3 * handle, const void *cache, const char *topo_name)
{
/* attempting to get a reference to some Topology Accessor Object */
    GaiaTopologyAccessorPtr accessor;

/* attempting to retrieve an alredy cached definition */
    accessor = gaiaTopologyFromCache (cache, topo_name);
    if (accessor != NULL)
	return accessor;

/* attempting to create a new Topology Accessor */
    accessor = gaiaTopologyFromDBMS (handle, cache, topo_name);
    return accessor;
}

GAIATOPO_DECLARE GaiaTopologyAccessorPtr
gaiaTopologyFromCache (const void *p_cache, const char *topo_name)
{
/* attempting to retrieve an already defined Topology Accessor Object from the Connection Cache */
    struct gaia_topology *ptr;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == 0)
	return NULL;

    ptr = (struct gaia_topology *) (cache->firstTopology);
    while (ptr != NULL)
      {
	  /* checking for an already registered Topology */
	  if (strcasecmp (topo_name, ptr->topology_name) == 0)
	      return (GaiaTopologyAccessorPtr) ptr;
	  ptr = ptr->next;
      }
    return NULL;
}

GAIATOPO_DECLARE int
gaiaReadTopologyFromDBMS (sqlite3 *
			  handle,
			  const char
			  *topo_name, char **topology_name, int *srid,
			  double *tolerance, int *has_z)
{
/* testing for existing DBMS objects */
    if (!check_existing_topology (handle, topo_name, 1))
	return 0;

/* retrieving the Topology configuration */
    if (!do_get_topology
	(handle, topo_name, topology_name, srid, tolerance, has_z))
	return 0;
    return 1;
}

GAIATOPO_DECLARE GaiaTopologyAccessorPtr
gaiaTopologyFromDBMS (sqlite3 * handle, const void *p_cache,
		      const char *topo_name)
{
/* attempting to create a Topology Accessor Object into the Connection Cache */
    LWT_BE_CALLBACKS *callbacks;
    struct gaia_topology *ptr;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == 0)
	return NULL;

/* allocating and initializing the opaque object */
    ptr = malloc (sizeof (struct gaia_topology));
    ptr->db_handle = handle;
    ptr->cache = cache;
    ptr->topology_name = NULL;
    ptr->srid = -1;
    ptr->tolerance = 0;
    ptr->has_z = 0;
    ptr->inside_lwt_callback = 0;
    ptr->last_error_message = NULL;
    ptr->lwt_iface = lwt_CreateBackendIface ((const LWT_BE_DATA *) ptr);
    ptr->prev = cache->lastTopology;
    ptr->next = NULL;

    callbacks = malloc (sizeof (LWT_BE_CALLBACKS));
    callbacks->lastErrorMessage = callback_lastErrorMessage;
    callbacks->topoGetSRID = callback_topoGetSRID;
    callbacks->topoGetPrecision = callback_topoGetPrecision;
    callbacks->topoHasZ = callback_topoHasZ;
    callbacks->createTopology = NULL;
    callbacks->loadTopologyByName = callback_loadTopologyByName;
    callbacks->freeTopology = callback_freeTopology;
    callbacks->getNodeById = callback_getNodeById;
    callbacks->getNodeWithinDistance2D = callback_getNodeWithinDistance2D;
    callbacks->insertNodes = callback_insertNodes;
    callbacks->getEdgeById = callback_getEdgeById;
    callbacks->getEdgeWithinDistance2D = callback_getEdgeWithinDistance2D;
    callbacks->getNextEdgeId = callback_getNextEdgeId;
    callbacks->insertEdges = callback_insertEdges;
    callbacks->updateEdges = callback_updateEdges;
    callbacks->getFaceById = callback_getFaceById;
    callbacks->getFaceContainingPoint = callback_getFaceContainingPoint;
    callbacks->deleteEdges = callback_deleteEdges;
    callbacks->getNodeWithinBox2D = callback_getNodeWithinBox2D;
    callbacks->getEdgeWithinBox2D = callback_getEdgeWithinBox2D;
    callbacks->getEdgeByNode = callback_getEdgeByNode;
    callbacks->updateNodes = callback_updateNodes;
    callbacks->insertFaces = callback_insertFaces;
    callbacks->updateFacesById = callback_updateFacesById;
    callbacks->deleteFacesById = callback_deleteFacesById;
    callbacks->getRingEdges = callback_getRingEdges;
    callbacks->updateEdgesById = callback_updateEdgesById;
    callbacks->getEdgeByFace = callback_getEdgeByFace;
    callbacks->getNodeByFace = callback_getNodeByFace;
    callbacks->updateNodesById = callback_updateNodesById;
    callbacks->deleteNodesById = callback_deleteNodesById;
    callbacks->updateTopoGeomEdgeSplit = callback_updateTopoGeomEdgeSplit;
    callbacks->updateTopoGeomFaceSplit = callback_updateTopoGeomFaceSplit;
    callbacks->checkTopoGeomRemEdge = callback_checkTopoGeomRemEdge;
    callbacks->updateTopoGeomFaceHeal = callback_updateTopoGeomFaceHeal;
    callbacks->checkTopoGeomRemNode = callback_checkTopoGeomRemNode;
    callbacks->updateTopoGeomEdgeHeal = callback_updateTopoGeomEdgeHeal;
    ptr->callbacks = callbacks;

    lwt_BackendIfaceRegisterCallbacks (ptr->lwt_iface, callbacks);
    ptr->lwt_topology = lwt_LoadTopology (ptr->lwt_iface, topo_name);
    if (ptr->lwt_topology == NULL)
	goto invalid;

/* creating the SQL prepared statements */
    ptr->stmt_getNodeWithinDistance2D =
	do_create_stmt_getNodeWithinDistance2D ((GaiaTopologyAccessorPtr) ptr);
    ptr->stmt_insertNodes =
	do_create_stmt_insertNodes ((GaiaTopologyAccessorPtr) ptr);
    ptr->stmt_getEdgeWithinDistance2D =
	do_create_stmt_getEdgeWithinDistance2D ((GaiaTopologyAccessorPtr) ptr);
    ptr->stmt_getNextEdgeId =
	do_create_stmt_getNextEdgeId ((GaiaTopologyAccessorPtr) ptr);
    ptr->stmt_setNextEdgeId =
	do_create_stmt_setNextEdgeId ((GaiaTopologyAccessorPtr) ptr);
    ptr->stmt_insertEdges =
	do_create_stmt_insertEdges ((GaiaTopologyAccessorPtr) ptr);
    ptr->stmt_getFaceContainingPoint_1 =
	do_create_stmt_getFaceContainingPoint_1 ((GaiaTopologyAccessorPtr) ptr);
    ptr->stmt_getFaceContainingPoint_2 =
	do_create_stmt_getFaceContainingPoint_2 ((GaiaTopologyAccessorPtr) ptr);
    ptr->stmt_deleteEdges = NULL;
    ptr->stmt_getNodeWithinBox2D =
	do_create_stmt_getNodeWithinBox2D ((GaiaTopologyAccessorPtr) ptr);
    ptr->stmt_getEdgeWithinBox2D =
	do_create_stmt_getEdgeWithinBox2D ((GaiaTopologyAccessorPtr) ptr);
    ptr->stmt_updateNodes = NULL;
    ptr->stmt_insertFaces =
	do_create_stmt_insertFaces ((GaiaTopologyAccessorPtr) ptr);
    ptr->stmt_updateFacesById =
	do_create_stmt_updateFacesById ((GaiaTopologyAccessorPtr) ptr);
    ptr->stmt_deleteFacesById =
	do_create_stmt_deleteFacesById ((GaiaTopologyAccessorPtr) ptr);
    ptr->stmt_deleteNodesById =
	do_create_stmt_deleteNodesById ((GaiaTopologyAccessorPtr) ptr);
    ptr->stmt_getRingEdges =
	do_create_stmt_getRingEdges ((GaiaTopologyAccessorPtr) ptr);

    return (GaiaTopologyAccessorPtr) ptr;

  invalid:
    ptr->stmt_getNodeWithinDistance2D = NULL;
    ptr->stmt_insertNodes = NULL;
    ptr->stmt_getEdgeWithinDistance2D = NULL;
    ptr->stmt_getNextEdgeId = NULL;
    ptr->stmt_setNextEdgeId = NULL;
    ptr->stmt_insertEdges = NULL;
    ptr->stmt_getFaceContainingPoint_1 = NULL;
    ptr->stmt_getFaceContainingPoint_2 = NULL;
    ptr->stmt_deleteEdges = NULL;
    ptr->stmt_getNodeWithinBox2D = NULL;
    ptr->stmt_getEdgeWithinBox2D = NULL;
    ptr->stmt_updateNodes = NULL;
    ptr->stmt_insertFaces = NULL;
    ptr->stmt_updateFacesById = NULL;
    ptr->stmt_deleteFacesById = NULL;
    ptr->stmt_deleteNodesById = NULL;
    ptr->stmt_getRingEdges = NULL;
    gaiaTopologyDestroy ((GaiaTopologyAccessorPtr) ptr);
    return NULL;
}

GAIATOPO_DECLARE void
gaiaTopologyDestroy (GaiaTopologyAccessorPtr topo_ptr)
{
/* destroying a Topology Accessor Object */
    struct gaia_topology *prev;
    struct gaia_topology *next;
    struct splite_internal_cache *cache;
    struct gaia_topology *ptr = (struct gaia_topology *) topo_ptr;
    if (ptr == NULL)
	return;

    prev = ptr->prev;
    next = ptr->next;
    cache = (struct splite_internal_cache *) (ptr->cache);
    if (ptr->lwt_topology != NULL)
	lwt_FreeTopology ((LWT_TOPOLOGY *) (ptr->lwt_topology));
    if (ptr->lwt_iface != NULL)
	lwt_FreeBackendIface ((LWT_BE_IFACE *) (ptr->lwt_iface));
    if (ptr->callbacks != NULL)
	free (ptr->callbacks);

    if (ptr->topology_name != NULL)
	free (ptr->topology_name);
    if (ptr->last_error_message != NULL)
	free (ptr->last_error_message);
    if (ptr->stmt_getNodeWithinDistance2D != NULL)
	sqlite3_finalize (ptr->stmt_getNodeWithinDistance2D);
    if (ptr->stmt_insertNodes != NULL)
	sqlite3_finalize (ptr->stmt_insertNodes);
    if (ptr->stmt_getEdgeWithinDistance2D != NULL)
	sqlite3_finalize (ptr->stmt_getEdgeWithinDistance2D);
    if (ptr->stmt_getNextEdgeId != NULL)
	sqlite3_finalize (ptr->stmt_getNextEdgeId);
    if (ptr->stmt_setNextEdgeId != NULL)
	sqlite3_finalize (ptr->stmt_setNextEdgeId);
    if (ptr->stmt_insertEdges != NULL)
	sqlite3_finalize (ptr->stmt_insertEdges);
    if (ptr->stmt_getFaceContainingPoint_1 != NULL)
	sqlite3_finalize (ptr->stmt_getFaceContainingPoint_1);
    if (ptr->stmt_getFaceContainingPoint_2 != NULL)
	sqlite3_finalize (ptr->stmt_getFaceContainingPoint_2);
    if (ptr->stmt_deleteEdges != NULL)
	sqlite3_finalize (ptr->stmt_deleteEdges);
    if (ptr->stmt_getNodeWithinBox2D != NULL)
	sqlite3_finalize (ptr->stmt_getNodeWithinBox2D);
    if (ptr->stmt_getEdgeWithinBox2D != NULL)
	sqlite3_finalize (ptr->stmt_getEdgeWithinBox2D);
    if (ptr->stmt_updateNodes != NULL)
	sqlite3_finalize (ptr->stmt_updateNodes);
    if (ptr->stmt_insertFaces != NULL)
	sqlite3_finalize (ptr->stmt_insertFaces);
    if (ptr->stmt_updateFacesById != NULL)
	sqlite3_finalize (ptr->stmt_updateFacesById);
    if (ptr->stmt_deleteFacesById != NULL)
	sqlite3_finalize (ptr->stmt_deleteFacesById);
    if (ptr->stmt_deleteNodesById != NULL)
	sqlite3_finalize (ptr->stmt_deleteNodesById);
    if (ptr->stmt_getRingEdges != NULL)
	sqlite3_finalize (ptr->stmt_getRingEdges);
    free (ptr);

/* unregistering from the Internal Cache double linked list */
    if (prev != NULL)
	prev->next = next;
    if (next != NULL)
	next->prev = prev;
    if (cache->firstTopology == ptr)
	cache->firstTopology = next;
    if (cache->lastTopology == ptr)
	cache->lastTopology = prev;
}

TOPOLOGY_PRIVATE void
gaiatopo_reset_last_error_msg (GaiaTopologyAccessorPtr accessor)
{
/* resets the last Topology error message */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return;

    if (topo->last_error_message != NULL)
	free (topo->last_error_message);
    topo->last_error_message = NULL;
}

TOPOLOGY_PRIVATE void
gaiatopo_set_last_error_msg (GaiaTopologyAccessorPtr accessor, const char *msg)
{
/* sets the last Topology error message */
    int len;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

    spatialite_e ("%s\n", msg);
    if (topo == NULL)
	return;

    if (topo->last_error_message != NULL)
	return;

    len = strlen (msg);
    topo->last_error_message = malloc (len + 1);
    strcpy (topo->last_error_message, msg);
}

TOPOLOGY_PRIVATE const char *
gaiatopo_get_last_exception (GaiaTopologyAccessorPtr accessor)
{
/* returns the last Topology error message */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return NULL;

    return topo->last_error_message;
}

GAIATOPO_DECLARE int
gaiaTopologyDrop (sqlite3 * handle, const char *topo_name)
{
/* attempting to drop an already existing Topology */
    int ret;
    char *sql;

/* creating the Topologies table (just in case) */
    if (!do_create_topologies (handle))
	return 0;

/* testing for existing DBMS objects */
    if (!check_existing_topology (handle, topo_name, 0))
	return 0;

/* dropping the Topology own Tables */
    if (!do_drop_topo_table (handle, topo_name, "edge"))
	goto error;
    if (!do_drop_topo_table (handle, topo_name, "node"))
	goto error;
    if (!do_drop_topo_table (handle, topo_name, "face"))
	goto error;

/* unregistering the Topology */
    sql =
	sqlite3_mprintf
	("DELETE FROM topologies WHERE Lower(topology_name) = Lower(%Q)",
	 topo_name);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;

    return 1;

  error:
    return 0;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaAddIsoNode (GaiaTopologyAccessorPtr accessor,
		sqlite3_int64 face, gaiaPointPtr pt, int skip_checks)
{
/* LWT wrapper - AddIsoNode */
    sqlite3_int64 ret;
    int has_z = 0;
    LWPOINT *lw_pt;
    POINTARRAY *pa;
    POINT4D point;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    if (pt->DimensionModel == GAIA_XY_Z || pt->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    pa = ptarray_construct (has_z, 0, 1);
    point.x = pt->X;
    point.y = pt->Y;
    if (has_z)
	point.z = pt->Z;
    ptarray_set_point4d (pa, 0, &point);
    lw_pt = lwpoint_construct (topo->srid, NULL, pa);

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret =
	lwt_AddIsoNode ((LWT_TOPOLOGY *) (topo->lwt_topology), face, lw_pt,
			skip_checks);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    lwpoint_free (lw_pt);
    return ret;
}

GAIATOPO_DECLARE int
gaiaMoveIsoNode (GaiaTopologyAccessorPtr accessor,
		 sqlite3_int64 node, gaiaPointPtr pt)
{
/* LWT wrapper - MoveIsoNode */
    int ret;
    int has_z = 0;
    LWPOINT *lw_pt;
    POINTARRAY *pa;
    POINT4D point;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    if (pt->DimensionModel == GAIA_XY_Z || pt->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    pa = ptarray_construct (has_z, 0, 1);
    point.x = pt->X;
    point.y = pt->Y;
    if (has_z)
	point.z = pt->Z;
    ptarray_set_point4d (pa, 0, &point);
    lw_pt = lwpoint_construct (topo->srid, NULL, pa);

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret = lwt_MoveIsoNode ((LWT_TOPOLOGY *) (topo->lwt_topology), node, lw_pt);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    lwpoint_free (lw_pt);
    if (ret == 0)
	return 1;
    return 0;
}

GAIATOPO_DECLARE int
gaiaRemIsoNode (GaiaTopologyAccessorPtr accessor, sqlite3_int64 node)
{
/* LWT wrapper - RemIsoNode */
    int ret;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret = lwt_RemoveIsoNode ((LWT_TOPOLOGY *) (topo->lwt_topology), node);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    if (ret == 0)
	return 1;
    return 0;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaAddIsoEdge (GaiaTopologyAccessorPtr accessor,
		sqlite3_int64 start_node, sqlite3_int64 end_node,
		gaiaLinestringPtr ln)
{
/* LWT wrapper - AddIsoEdge */
    sqlite3_int64 ret;
    LWLINE *lw_line;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    lw_line = gaia_convert_linestring_to_lwline (ln, topo->srid, topo->has_z);

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret =
	lwt_AddIsoEdge ((LWT_TOPOLOGY *) (topo->lwt_topology), start_node,
			end_node, lw_line);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    lwline_free (lw_line);
    return ret;
}

GAIATOPO_DECLARE int
gaiaChangeEdgeGeom (GaiaTopologyAccessorPtr accessor,
		    sqlite3_int64 edge_id, gaiaLinestringPtr ln)
{
/* LWT wrapper - ChangeEdgeGeom  */
    int ret;
    LWLINE *lw_line;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    lw_line = gaia_convert_linestring_to_lwline (ln, topo->srid, topo->has_z);

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret =
	lwt_ChangeEdgeGeom ((LWT_TOPOLOGY *) (topo->lwt_topology), edge_id,
			    lw_line);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    lwline_free (lw_line);
    if (ret == 0)
	return 1;
    return 0;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaModEdgeSplit (GaiaTopologyAccessorPtr accessor,
		  sqlite3_int64 edge, gaiaPointPtr pt, int skip_checks)
{
/* LWT wrapper - ModEdgeSplit */
    sqlite3_int64 ret;
    int has_z = 0;
    LWPOINT *lw_pt;
    POINTARRAY *pa;
    POINT4D point;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    if (pt->DimensionModel == GAIA_XY_Z || pt->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    pa = ptarray_construct (has_z, 0, 1);
    point.x = pt->X;
    point.y = pt->Y;
    if (has_z)
	point.z = pt->Z;
    ptarray_set_point4d (pa, 0, &point);
    lw_pt = lwpoint_construct (topo->srid, NULL, pa);

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret =
	lwt_ModEdgeSplit ((LWT_TOPOLOGY *) (topo->lwt_topology), edge, lw_pt,
			  skip_checks);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    lwpoint_free (lw_pt);
    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaNewEdgesSplit (GaiaTopologyAccessorPtr accessor,
		   sqlite3_int64 edge, gaiaPointPtr pt, int skip_checks)
{
/* LWT wrapper - NewEdgesSplit */
    sqlite3_int64 ret;
    int has_z = 0;
    LWPOINT *lw_pt;
    POINTARRAY *pa;
    POINT4D point;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    if (pt->DimensionModel == GAIA_XY_Z || pt->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    pa = ptarray_construct (has_z, 0, 1);
    point.x = pt->X;
    point.y = pt->Y;
    if (has_z)
	point.z = pt->Z;
    ptarray_set_point4d (pa, 0, &point);
    lw_pt = lwpoint_construct (topo->srid, NULL, pa);

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret =
	lwt_NewEdgesSplit ((LWT_TOPOLOGY *) (topo->lwt_topology), edge, lw_pt,
			   skip_checks);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    lwpoint_free (lw_pt);
    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaAddEdgeModFace (GaiaTopologyAccessorPtr accessor,
		    sqlite3_int64 start_node, sqlite3_int64 end_node,
		    gaiaLinestringPtr ln, int skip_checks)
{
/* LWT wrapper - AddEdgeModFace */
    sqlite3_int64 ret;
    LWLINE *lw_line;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    lw_line = gaia_convert_linestring_to_lwline (ln, topo->srid, topo->has_z);

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret =
	lwt_AddEdgeModFace ((LWT_TOPOLOGY *) (topo->lwt_topology), start_node,
			    end_node, lw_line, skip_checks);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    lwline_free (lw_line);
    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaAddEdgeNewFaces (GaiaTopologyAccessorPtr accessor,
		     sqlite3_int64 start_node, sqlite3_int64 end_node,
		     gaiaLinestringPtr ln, int skip_checks)
{
/* LWT wrapper - AddEdgeNewFaces */
    sqlite3_int64 ret;
    LWLINE *lw_line;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    lw_line = gaia_convert_linestring_to_lwline (ln, topo->srid, topo->has_z);

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret =
	lwt_AddEdgeNewFaces ((LWT_TOPOLOGY *) (topo->lwt_topology), start_node,
			     end_node, lw_line, skip_checks);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    lwline_free (lw_line);
    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaRemEdgeNewFace (GaiaTopologyAccessorPtr accessor, sqlite3_int64 edge_id)
{
/* LWT wrapper - RemEdgeNewFace */
    sqlite3_int64 ret;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret = lwt_RemEdgeNewFace ((LWT_TOPOLOGY *) (topo->lwt_topology), edge_id);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaRemEdgeModFace (GaiaTopologyAccessorPtr accessor, sqlite3_int64 edge_id)
{
/* LWT wrapper - RemEdgeModFace */
    sqlite3_int64 ret;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret = lwt_RemEdgeModFace ((LWT_TOPOLOGY *) (topo->lwt_topology), edge_id);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaNewEdgeHeal (GaiaTopologyAccessorPtr accessor, sqlite3_int64 edge_id1,
		 sqlite3_int64 edge_id2)
{
/* LWT wrapper - NewEdgeHeal */
    sqlite3_int64 ret;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret =
	lwt_NewEdgeHeal ((LWT_TOPOLOGY *) (topo->lwt_topology), edge_id1,
			 edge_id2);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaModEdgeHeal (GaiaTopologyAccessorPtr accessor, sqlite3_int64 edge_id1,
		 sqlite3_int64 edge_id2)
{
/* LWT wrapper - ModEdgeHeal */
    sqlite3_int64 ret;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret =
	lwt_ModEdgeHeal ((LWT_TOPOLOGY *) (topo->lwt_topology), edge_id1,
			 edge_id2);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    return ret;
}

GAIATOPO_DECLARE gaiaGeomCollPtr
gaiaGetFaceGeometry (GaiaTopologyAccessorPtr accessor, sqlite3_int64 face)
{
/* LWT wrapper - GetFaceGeometry */
    LWGEOM *result = NULL;
    LWPOLY *lwpoly;
    int has_z = 0;
    POINTARRAY *pa;
    POINT4D pt4d;
    int iv;
    int ib;
    double x;
    double y;
    double z;
    gaiaGeomCollPtr geom;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;
    int dimension_model;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    if (topo->inside_lwt_callback == 0)
      {
	  /* locking the semaphore if not already inside a callback context */
	  splite_lwgeom_semaphore_lock ();
	  splite_lwgeomtopo_init ();
	  gaiaResetLwGeomMsg ();
      }

    result = lwt_GetFaceGeometry ((LWT_TOPOLOGY *) (topo->lwt_topology), face);

    if (topo->inside_lwt_callback == 0)
      {
	  /* unlocking the semaphore if not already inside a callback context */
	  splite_lwgeom_init ();
	  splite_lwgeom_semaphore_unlock ();
      }

    if (result == NULL)
	return NULL;

/* converting the result as a Gaia Geometry */
    lwpoly = (LWPOLY *) result;
    pa = lwpoly->rings[0];
    if (FLAGS_GET_Z (pa->flags))
	has_z = 1;
    if (has_z)
      {
	  dimension_model = GAIA_XY_Z;
	  geom = gaiaAllocGeomCollXYZ ();
      }
    else
      {
	  dimension_model = GAIA_XY;
	  geom = gaiaAllocGeomColl ();
      }
    pg = gaiaAddPolygonToGeomColl (geom, pa->npoints, lwpoly->nrings - 1);
    rng = pg->Exterior;
    for (iv = 0; iv < pa->npoints; iv++)
      {
	  /* copying Exterior Ring vertices */
	  getPoint4d_p (pa, iv, &pt4d);
	  x = pt4d.x;
	  y = pt4d.y;
	  if (has_z)
	      z = pt4d.z;
	  else
	      z = 0.0;
	  if (dimension_model == GAIA_XY_Z)
	    {
		gaiaSetPointXYZ (rng->Coords, iv, x, y, z);
	    }
	  else
	    {
		gaiaSetPoint (rng->Coords, iv, x, y);
	    }
      }
    for (ib = 1; ib < lwpoly->nrings; ib++)
      {
	  has_z = 0;
	  pa = lwpoly->rings[ib];
	  if (FLAGS_GET_Z (pa->flags))
	      has_z = 1;
	  rng = gaiaAddInteriorRing (pg, ib - 1, pa->npoints);
	  for (iv = 0; iv < pa->npoints; iv++)
	    {
		/* copying Exterior Ring vertices */
		getPoint4d_p (pa, iv, &pt4d);
		x = pt4d.x;
		y = pt4d.y;
		if (has_z)
		    z = pt4d.z;
		else
		    z = 0.0;
		if (dimension_model == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (rng->Coords, iv, x, y, z);
		  }
		else
		  {
		      gaiaSetPoint (rng->Coords, iv, x, y);
		  }
	    }
      }
    geom->DeclaredType = GAIA_POLYGON;
    geom->Srid = topo->srid;
    return geom;
}

static int
do_check_create_faceedges_table (GaiaTopologyAccessorPtr accessor)
{
/* attemtping to create or validate the target table */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    int exists = 0;
    int ok_face_id = 0;
    int ok_sequence = 0;
    int ok_edge_id = 0;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

/* testing for an already existing table */
    table = sqlite3_mprintf ("%s_face_edges_temp", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("PRAGMA TEMP.table_info(\"%s\")", xtable);
    free (xtable);
    ret =
	sqlite3_get_table (topo->db_handle, sql, &results, &rows, &columns,
			   &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("ST_GetFaceEdges exception: %s", errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
      {
	  const char *name = results[(i * columns) + 1];
	  const char *type = results[(i * columns) + 2];
	  const char *notnull = results[(i * columns) + 3];
	  const char *dflt_value = results[(i * columns) + 4];
	  const char *pk = results[(i * columns) + 5];
	  if (strcmp (name, "face_id") == 0 && strcmp (type, "INTEGER") == 0
	      && strcmp (notnull, "1") == 0 && dflt_value == NULL
	      && strcmp (pk, "1") == 0)
	      ok_face_id = 1;
	  if (strcmp (name, "sequence") == 0 && strcmp (type, "INTEGER") == 0
	      && strcmp (notnull, "1") == 0 && dflt_value == NULL
	      && strcmp (pk, "2") == 0)
	      ok_sequence = 1;
	  if (strcmp (name, "edge_id") == 0 && strcmp (type, "INTEGER") == 0
	      && strcmp (notnull, "1") == 0 && dflt_value == NULL
	      && strcmp (pk, "0") == 0)
	      ok_edge_id = 1;
	  exists = 1;
      }
    sqlite3_free_table (results);
    if (ok_face_id && ok_sequence && ok_edge_id)
	return 1;		/* already existing and valid */

    if (exists)
	return 0;		/* already existing but invalid */

/* attempting to create the table */
    table = sqlite3_mprintf ("%s_face_edges_temp", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("CREATE TEMP TABLE \"%s\" (\n\tface_id INTEGER NOT NULL,\n"
	 "\tsequence INTEGER NOT NULL,\n\tedge_id INTEGER NOT NULL,\n"
	 "\tCONSTRAINT pk_topo_facee_edges PRIMARY KEY (face_id, sequence))",
	 xtable);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("ST_GetFaceEdges exception: %s", errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  sqlite3_free (errMsg);
	  return 0;
      }

    return 1;
}

static int
do_populate_faceedges_table (GaiaTopologyAccessorPtr accessor,
			     sqlite3_int64 face, LWT_ELEMID * edges,
			     int num_edges)
{
/* populating the target table */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    int i;
    sqlite3_stmt *stmt = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

/* deleting all rows belonging to Face (if any) */
    table = sqlite3_mprintf ("%s_face_edges_temp", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DELETE FROM \"%s\" WHERE face_id = ?", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("ST_GetFaceEdges exception: %s",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, face);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  char *msg = sqlite3_mprintf ("ST_GetFaceEdges exception: %s",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }
    sqlite3_finalize (stmt);
    stmt = NULL;

/* preparing the INSERT statement */
    table = sqlite3_mprintf ("%s_face_edges_temp", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (face_id, sequence, edge_id) VALUES (?, ?, ?)",
	 xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("ST_GetFaceEdges exception: %s",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }
    for (i = 0; i < num_edges; i++)
      {
	  /* inserting all Face/Edges */
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_int64 (stmt, 1, face);
	  sqlite3_bind_int (stmt, 2, i + 1);
	  sqlite3_bind_int64 (stmt, 3, *(edges + i));
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		char *msg = sqlite3_mprintf ("ST_GetFaceEdges exception: %s",
					     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

GAIATOPO_DECLARE int
gaiaGetFaceEdges (GaiaTopologyAccessorPtr accessor, sqlite3_int64 face)
{
/* LWT wrapper - GetFaceEdges */
    LWT_ELEMID *edges = NULL;
    int num_edges;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    /* locking the semaphore */
    splite_lwgeom_semaphore_lock ();
    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();

    num_edges =
	lwt_GetFaceEdges ((LWT_TOPOLOGY *) (topo->lwt_topology), face, &edges);

    /* unlocking the semaphore */
    splite_lwgeom_init ();
    splite_lwgeom_semaphore_unlock ();

    if (num_edges < 0)
	return 0;

    if (num_edges > 0)
      {
	  /* attemtping to create or validate the target table */
	  if (!do_check_create_faceedges_table (accessor))
	    {
		lwfree (edges);
		return 0;
	    }

	  /* populating the target table */
	  if (!do_populate_faceedges_table (accessor, face, edges, num_edges))
	    {
		lwfree (edges);
		return 0;
	    }
      }
    lwfree (edges);
    return 1;
}

#endif /* end TOPOLOGY conditionals */
