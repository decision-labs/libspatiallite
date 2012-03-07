/*

 check_shp_load_3d.c -- SpatiaLite Test Case

 Author: Brad Hards <bradh@frogmouth.net>

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
 
Portions created by the Initial Developer are Copyright (C) 2011
the Initial Developer. All Rights Reserved.

Contributor(s):
Brad Hards <bradh@frogmouth.net>

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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "sqlite3.h"
#include "spatialite.h"

int main (int argc, char *argv[])
{
    int ret;
    sqlite3 *handle;
    char *err_msg = NULL;
    int row_count;
    const char *sql;
    char **results;
    int rows;
    int columns;
    double tic;

    spatialite_init (0);
    ret = sqlite3_open_v2 (":memory:", &handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK) {
	fprintf(stderr, "cannot open in-memory database: %s\n", sqlite3_errmsg (handle));
	sqlite3_close(handle);
	return -1;
    }
    
    ret = sqlite3_exec (handle, "SELECT InitSpatialMetadata()", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -2;
    }

    ret = load_shapefile (handle, "shp/merano-3d/polygons", "polygons", "CP1252", 25832, 
			  "geom", 0, 1, 1, 0, &row_count, err_msg);
    if (!ret) {
        fprintf (stderr, "load_shapefile() error: %s\n", err_msg);
	sqlite3_close(handle);
	return -3;
    }

    ret = update_layer_statistics (handle, "polygons", "geom");
    if (!ret) {
        fprintf (stderr, "update_layer_statistics() error\n", err_msg);
	sqlite3_close(handle);
	return -8;
    }

    ret = load_shapefile (handle, "shp/merano-3d/roads", "roads", "CP1252", 25832, 
			  "geom", 0, 0, 1, 0, &row_count, err_msg);
    if (!ret) {
        fprintf (stderr, "load_shapefile() error: %s\n", err_msg);
	sqlite3_close(handle);
	return -4;
    }

    ret = load_shapefile (handle, "shp/merano-3d/points", "points", "CP1252", 25832, 
			  "geom", 0, 0, 1, 0, &row_count, err_msg);
    if (!ret) {
        fprintf (stderr, "load_shapefile() error: %s\n", err_msg);
	sqlite3_close(handle);
	return -5;
    }
    
    ret = sqlite3_exec (handle, "INSERT INTO polygons (COL_0, DATUM, HAUSNR) VALUES (1250000, 0.1, 'alpha')", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "INSERT polygons (1) error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -14;
    }
    ret = sqlite3_exec (handle, "INSERT INTO polygons (COL_0, DATUM, HAUSNR) VALUES (1250000, 0.1, 'alpha')", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "INSERT polygons (1) error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -15;
    }
    ret = sqlite3_exec (handle, "INSERT INTO polygons (COL_0, DATUM, TEXT_I) VALUES (1250000, 0.1, 'alpha')", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "INSERT polygons (1) error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -16;
    }

    ret = sqlite3_exec (handle, "CREATE TABLE polyg_xy (pk_elem INTEGER PRIMARY KEY AUTOINCREMENT, COL_0 INTEGER, DATUM DOUBLE, TEXT_I TEXT, BLOB_I BLOB)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "CREATE polyg_xy error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -17;
    }
    ret = sqlite3_exec (handle, "SELECT AddGeometryColumn('polyg_xy', 'geom', 25832, 'POLYGON', 'XY')", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "AddGeomety polyg_xy error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -17;
    }
    ret = sqlite3_exec (handle, "INSERT INTO polyg_xy (pk_elem, COL_0, DATUM, TEXT_I, BLOB_I, geom) SELECT NULL, COL_0, DATUM, 'alpha', zeroblob(20), CastToXY(geom) FROM polygons WHERE geom IS NOT NULL", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "feeding polyg_xy (1) error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -18;
    }
    ret = sqlite3_exec (handle, "INSERT INTO polyg_xy (pk_elem, COL_0, DATUM, TEXT_I, BLOB_I, geom) SELECT NULL, COL_0, DATUM, 'alpha', zeroblob(20), CastToXY(geom) FROM polygons WHERE geom IS NOT NULL", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "feeding polyg_xy (2) error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -19;
    }
    ret = sqlite3_exec (handle, "CREATE TABLE polyg_xym (pk_elem INTEGER PRIMARY KEY AUTOINCREMENT, COL_0 INTEGER, DATUM DOUBLE, TEXT_I TEXT, BLOB_I BLOB)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "CREATE polyg_xym error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -20;
    }
    ret = sqlite3_exec (handle, "SELECT AddGeometryColumn('polyg_xym', 'geom', 25832, 'POLYGON', 'XYM')", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "AddGeometry polyg_xym error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -21;
    }
    ret = sqlite3_exec (handle, "INSERT INTO polyg_xym (pk_elem, COL_0, DATUM, TEXT_I, BLOB_I, geom) SELECT NULL, COL_0, DATUM, 'alpha', zeroblob(20), CastToXYM(geom) FROM polygons WHERE geom IS NOT NULL", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "feeding polyg_xym (1) error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -22;
    }
    ret = sqlite3_exec (handle, "INSERT INTO polyg_xym (pk_elem, COL_0, DATUM, TEXT_I, BLOB_I, geom) SELECT NULL, COL_0, DATUM, 'alpha', zeroblob(20), CastToXYM(geom) FROM polygons WHERE geom IS NOT NULL", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "feeding polyg_xym (2) error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -23;
    }
    ret = sqlite3_exec (handle, "CREATE TABLE polyg_xyz (pk_elem INTEGER PRIMARY KEY AUTOINCREMENT, COL_0 INTEGER, DATUM DOUBLE, TEXT_I TEXT, BLOB_I BLOB)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "CREATE polyg_xyz error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -24;
    }
    ret = sqlite3_exec (handle, "SELECT AddGeometryColumn('polyg_xyz', 'geom', 25832, 'POLYGON', 'XYZ')", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "AddGeometry polyg_xyz error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -25;
    }
    ret = sqlite3_exec (handle, "INSERT INTO polyg_xyz (pk_elem, COL_0, DATUM, TEXT_I, BLOB_I, geom) SELECT NULL, COL_0, DATUM, 'alpha', zeroblob(20), CastToXYZ(geom) FROM polygons WHERE geom IS NOT NULL", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "feeding polyg_xyz (1) error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -26;
    }
    ret = sqlite3_exec (handle, "INSERT INTO polyg_xyz (pk_elem, COL_0, DATUM, TEXT_I, BLOB_I, geom) SELECT NULL, COL_0, DATUM, 'alpha', zeroblob(20), CastToXYZ(geom) FROM polygons WHERE geom IS NOT NULL", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "feeding polyg_xyz (2) error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -27;
    }

    ret = sqlite3_exec (handle, "CREATE TABLE roads_xyz (pk_elem INTEGER PRIMARY KEY AUTOINCREMENT, COL_0 INTEGER, DATUM DOUBLE, TEXT_I TEXT, BLOB_I BLOB)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "CREATE roads_xyz error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -28;
    }
    ret = sqlite3_exec (handle, "SELECT AddGeometryColumn('roads_xyz', 'geom', 25832, 'LINESTRING', 'XYZ')", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "AddGeomety roads_xyz error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -29;
    }
    ret = sqlite3_exec (handle, "INSERT INTO roads_xyz (pk_elem, COL_0, DATUM, TEXT_I, BLOB_I, geom) SELECT NULL, COL_0, DATUM, 'alpha', zeroblob(20), CastToXYZ(geom) FROM roads WHERE geom IS NOT NULL", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "feeding roads_xyz (1) error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -30;
    }
    ret = sqlite3_exec (handle, "INSERT INTO roads_xyz (pk_elem, COL_0, DATUM, TEXT_I, BLOB_I, geom) SELECT NULL, COL_0, DATUM, 'alpha', zeroblob(20), CastToXYZ(geom) FROM roads WHERE geom IS NOT NULL", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "feeding roads_xyz (2) error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -31;
    }
    ret = sqlite3_exec (handle, "CREATE TABLE roads_xym (pk_elem INTEGER PRIMARY KEY AUTOINCREMENT, COL_0 INTEGER, DATUM DOUBLE, TEXT_I TEXT, BLOB_I BLOB)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "CREATE roads_xym error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -32;
    }
    ret = sqlite3_exec (handle, "SELECT AddGeometryColumn('roads_xym', 'geom', 25832, 'LINESTRING', 'XYM')", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "AddGeometry roads_xym error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -33;
    }
    ret = sqlite3_exec (handle, "INSERT INTO roads_xym (pk_elem, COL_0, DATUM, TEXT_I, BLOB_I, geom) SELECT NULL, COL_0, DATUM, 'alpha', zeroblob(20), CastToXYM(geom) FROM roads WHERE geom IS NOT NULL", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "feeding roads_xym (1) error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -34;
    }
    ret = sqlite3_exec (handle, "INSERT INTO roads_xym (pk_elem, COL_0, DATUM, TEXT_I, BLOB_I, geom) SELECT NULL, COL_0, DATUM, 'alpha', zeroblob(20), CastToXYM(geom) FROM roads WHERE geom IS NOT NULL", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "feeding roads_xym (2) error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -35;
    }
    ret = sqlite3_exec (handle, "CREATE TABLE roads_xyzm (pk_elem INTEGER PRIMARY KEY AUTOINCREMENT, COL_0 INTEGER, DATUM DOUBLE, TEXT_I TEXT, BLOB_I BLOB)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "CREATE roads_xyzm error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -36;
    }
    ret = sqlite3_exec (handle, "SELECT AddGeometryColumn('roads_xyzm', 'geom', 25832, 'LINESTRING', 'XYZM')", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "AddGeometry roads_xyzm error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -37;
    }
    ret = sqlite3_exec (handle, "INSERT INTO roads_xyzm (pk_elem, COL_0, DATUM, TEXT_I, BLOB_I, geom) SELECT NULL, COL_0, DATUM, 'alpha', zeroblob(20), CastToXYZM(geom) FROM roads WHERE geom IS NOT NULL", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "feeding roads_xyzm (1) error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -38;
    }
    ret = sqlite3_exec (handle, "INSERT INTO roads_xyzm (pk_elem, COL_0, DATUM, TEXT_I, BLOB_I, geom) SELECT NULL, COL_0, DATUM, 'alpha', zeroblob(20), CastToXYZM(geom) FROM roads WHERE geom IS NOT NULL", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "feeding roads_xyzm (2) error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -39;
    }

    ret = sqlite3_exec (handle, "CREATE TABLE points_xyz (pk_elem INTEGER PRIMARY KEY AUTOINCREMENT, COL_0 INTEGER, DATUM DOUBLE, TEXT_I TEXT, BLOB_I BLOB)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "CREATE points_xyz error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -40;
    }
    ret = sqlite3_exec (handle, "SELECT AddGeometryColumn('points_xyz', 'geom', 25832, 'POINT', 'XYZ')", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "AddGeomety points_xyz error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -41;
    }
    ret = sqlite3_exec (handle, "INSERT INTO points_xyz (pk_elem, COL_0, DATUM, TEXT_I, BLOB_I, geom) SELECT NULL, COL_0, DATUM, 'alpha', zeroblob(20), CastToXYZ(geom) FROM points WHERE geom IS NOT NULL", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "feeding points_xyz (1) error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -42;
    }
    ret = sqlite3_exec (handle, "INSERT INTO points_xyz (pk_elem, COL_0, DATUM, TEXT_I, BLOB_I, geom) SELECT NULL, COL_0, DATUM, 'alpha', zeroblob(20), CastToXYZ(geom) FROM points WHERE geom IS NOT NULL", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "feeding points_xyz (2) error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -43;
    }
    ret = sqlite3_exec (handle, "CREATE TABLE points_xym (pk_elem INTEGER PRIMARY KEY AUTOINCREMENT, COL_0 INTEGER, DATUM DOUBLE, TEXT_I TEXT, BLOB_I BLOB)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "CREATE points_xym error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -44;
    }
    ret = sqlite3_exec (handle, "SELECT AddGeometryColumn('points_xym', 'geom', 25832, 'POINT', 'XYM')", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "AddGeometry points_xym error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -45;
    }
    ret = sqlite3_exec (handle, "INSERT INTO points_xym (pk_elem, COL_0, DATUM, TEXT_I, BLOB_I, geom) SELECT NULL, COL_0, DATUM, 'alpha', zeroblob(20), CastToXYM(geom) FROM points WHERE geom IS NOT NULL", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "feeding points_xym (1) error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -46;
    }
    ret = sqlite3_exec (handle, "INSERT INTO points_xym (pk_elem, COL_0, DATUM, TEXT_I, BLOB_I, geom) SELECT NULL, COL_0, DATUM, 'alpha', zeroblob(20), CastToXYM(geom) FROM points WHERE geom IS NOT NULL", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "feeding points_xym (2) error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -47;
    }
    ret = sqlite3_exec (handle, "CREATE TABLE points_xyzm (pk_elem INTEGER PRIMARY KEY AUTOINCREMENT, COL_0 INTEGER, DATUM DOUBLE, TEXT_I TEXT, BLOB_I BLOB)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "CREATE points_xyzm error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -48;
    }
    ret = sqlite3_exec (handle, "SELECT AddGeometryColumn('points_xyzm', 'geom', 25832, 'POINT', 'XYZM')", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "AddGeometry points_xyzm error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -49;
    }
    ret = sqlite3_exec (handle, "INSERT INTO points_xyzm (pk_elem, COL_0, DATUM, TEXT_I, BLOB_I, geom) SELECT NULL, COL_0, DATUM, 'alpha', zeroblob(20), CastToXYZM(geom) FROM points WHERE geom IS NOT NULL", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "feeding points_xyzm (1) error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -50;
    }
    ret = sqlite3_exec (handle, "INSERT INTO points_xyzm (pk_elem, COL_0, DATUM, TEXT_I, BLOB_I, geom) SELECT NULL, COL_0, DATUM, 'alpha', zeroblob(20), CastToXYZM(geom) FROM points WHERE geom IS NOT NULL", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "feeding points_xyzm (2) error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -51;
    }

    elementary_geometries (handle, "points", "geom", "elem_point", "pk_elem", "mul_id");
    elementary_geometries (handle, "roads", "geom", "elem_linestring", "pk_elem", "mul_id");
    elementary_geometries (handle, "polygons", "geom", "elem_poly", "pk_elem", "mul_id");

    elementary_geometries (handle, "polyg_xy", "geom", "elem_poly_xy", "pk_elem1", "mul_id");
    elementary_geometries (handle, "polyg_xym", "geom", "elem_poly_xym", "pk_elem1", "mul_id");
    elementary_geometries (handle, "polyg_xyz", "geom", "elem_poly_xyz", "pk_elem1", "mul_id");

    elementary_geometries (handle, "roads_xyz", "geom", "elem_roads_xy", "pk_elem1", "mul_id");
    elementary_geometries (handle, "roads_xym", "geom", "elem_roads_xym", "pk_elem1", "mul_id");
    elementary_geometries (handle, "roads_xyzm", "geom", "elem_roads_xyz", "pk_elem1", "mul_id");

    elementary_geometries (handle, "points_xyz", "geom", "elem_points_xy", "pk_elem1", "mul_id");
    elementary_geometries (handle, "points_xym", "geom", "elem_points_xym", "pk_elem1", "mul_id");
    elementary_geometries (handle, "points_xyzm", "geom", "elem_points_xyz", "pk_elem1", "mul_id");

    remove_duplicated_rows(handle, "polyg_xy");
    remove_duplicated_rows(handle, "polyg_xyz");
    remove_duplicated_rows(handle, "polyg_xym");
    remove_duplicated_rows(handle, "roads_xyz");
    remove_duplicated_rows(handle, "roads_xym");
    remove_duplicated_rows(handle, "roads_xyzm");
    remove_duplicated_rows(handle, "points_xyz");
    remove_duplicated_rows(handle, "points_xym");
    remove_duplicated_rows(handle, "points_xyzm");

    ret = update_layer_statistics (handle, NULL, NULL);
    if (!ret) {
        fprintf (stderr, "update_layer_statistics() error\n", err_msg);
	sqlite3_close(handle);
	return -8;
    }

    sql = "SELECT row_count, extent_min_x, extent_max_y "
          "FROM layer_statistics "
          "WHERE raster_layer = 0 AND table_name LIKE 'roads' "
          "AND geometry_column LIKE 'geom'";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "Error: %s\n", err_msg);
        sqlite3_free (err_msg);
        return -9;
    }
    if (rows != 1) {
        fprintf (stderr, "Unexpected num of rows for LAYER_STATISTICS.\n");
        return  -10;
    }
    if (atoi(results[columns+0]) != 18) {
        fprintf (stderr, "Unexpected error: LAYER_STATISTICS bad result row_count: %s\n", results[columns+0]);
        return -11;
    }
    tic = fabs(atof(results[columns+1]) - 666057.648017325);
    if (tic >= 0.00000002) {
        fprintf (stderr, "Unexpected error: LAYER_STATISTICS bad result extent_min_x: %s\n", results[columns+1]);
        return -12;
    }
    tic = fabs(atof(results[columns+2]) - 5170671.31627796);
    if (tic >= 0.0000002) {
        fprintf (stderr, "Unexpected error: LAYER_STATISTICS bad result extent_max_y: %s\n", results[columns+2]);
        return -13;
    }
    sqlite3_free_table (results);

    ret = sqlite3_exec (handle, "DROP TABLE polygons", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "DROP polygons error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -6;
    }
    
    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "sqlite3_close() error: %s\n", sqlite3_errmsg (handle));
	return -7;
    }

    spatialite_cleanup();
    return 0;
}
