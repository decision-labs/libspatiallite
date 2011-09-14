/*

 shape_utf8_2.c -- SpatiaLite Test Case

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

#include "sqlite3.h"
#include "spatialite.h"

int main (int argc, char *argv[])
{
    int ret;
    sqlite3 *handle;
    char *dbname = __FILE__"test.db";
    char *kmlname = __FILE__"test.kml";
    char *err_msg = NULL;
    int row_count;

    spatialite_init (0);
    ret = sqlite3_open_v2 (dbname, &handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK) {
	fprintf(stderr, "cannot open '%s': %s\n", dbname, sqlite3_errmsg (handle));
	sqlite3_close(handle);
	return -1;
    }
    
    ret = sqlite3_exec (handle, "SELECT InitSpatialMetadata()", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	unlink(dbname);
	return -2;
    }
    
    ret = load_shapefile (handle, "./shp/taiwan/hystoric", "hystoric", "UTF8", 4326, 
			  "col1", 1, 0, 1, 0, &row_count, err_msg);
    if (!ret) {
        fprintf (stderr, "load_shapefile() error for shp/taiwan/hystoric: %s\n", err_msg);
	sqlite3_close(handle);
	unlink(dbname);
	return -3;
    }
    if (row_count != 15) {
	fprintf (stderr, "unexpected row count for shp/taiwan/hystoric: %i\n", row_count);
	sqlite3_close(handle);
	unlink(dbname);
	return -4;
    }

    ret = load_shapefile (handle, "./shp/taiwan/leisure", "leisure", "UTF8", 4326, 
			  "col1", 1, 0, 1, 0, &row_count, err_msg);
    if (!ret) {
        fprintf (stderr, "load_shapefile() error for shp/taiwan/leisure: %s\n", err_msg);
	sqlite3_close(handle);
	unlink(dbname);
	return -5;
    }
    if (row_count != 5) {
	fprintf (stderr, "unexpected row count for shp/taiwan/leisure: %i\n", row_count);
	sqlite3_close(handle);
	unlink(dbname);
	return -6;
    }

    ret = load_shapefile (handle, "./shp/taiwan/route", "route", "UTF8", 4326, 
			  "col1", 1, 0, 1, 0, &row_count, err_msg);
    if (!ret) {
        fprintf (stderr, "load_shapefile() error for shp/taiwan/route: %s\n", err_msg);
	sqlite3_close(handle);
	unlink(dbname);
	return -7;
    }
    if (row_count != 4) {
	fprintf (stderr, "unexpected row count for shp/taiwan/route: %i\n", row_count);
	sqlite3_close(handle);
	unlink(dbname);
	return -8;
    }

    if (is_kml_constant (handle, "route", "name")) {
	fprintf(stderr, "unexpected result for is_kml_constant (1)\n");
	return -9;
    }
    if (! is_kml_constant (handle, "route", "foo")) {
	fprintf(stderr, "unexpected result for is_kml_constant (2)\n");
	return -10;
    }
    
    ret = dump_kml (handle, "route", "col1", kmlname, NULL, NULL, 10);
    if (!ret) {
        fprintf (stderr, "load_shapefile() error for shp/taiwan/route: %s\n", err_msg);
	sqlite3_close(handle);
	unlink(dbname);
	return -11;
    }
    unlink(kmlname);

    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "sqlite3_close() error: %s\n", sqlite3_errmsg (handle));
	unlink(dbname);
	return -12;
    }
    
    unlink(dbname);
    spatialite_cleanup();
    sqlite3_reset_auto_extension();
    return 0;
}
