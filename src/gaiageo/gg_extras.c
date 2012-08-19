/*

 gg_extras.c -- Gaia extra functions support
    
 version 4.0, 2012 August 19

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
 
Portions created by the Initial Developer are Copyright (C) 2012
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

#include "config.h"

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>

#include "config.h"

#include <spatialite/sqlite.h>
#include <spatialite/gaiageo.h>

static void
auxGridSnapPoint (int dimension_model, gaiaPointPtr pt, gaiaGeomCollPtr result,
		  double origin_x, double origin_y, double origin_z,
		  double origin_m, double size_x, double size_y, double size_z,
		  double size_m)
{
/* snapping a Point to a regular Grid */
    double x = pt->X;
    double y = pt->Y;
    double z = 0.0;
    double m = 0.0;
    int has_z = 0;
    int has_m = 0;
    gaiaPointPtr ptx;

    if (pt == NULL || result == NULL)
	return;
    if (dimension_model == GAIA_XY_Z || dimension_model == GAIA_XY_Z_M)
	has_z = 1;
    if (dimension_model == GAIA_XY_M || dimension_model == GAIA_XY_Z_M)
	has_m = 1;
    if (has_z)
	z = pt->Z;
    if (has_m)
	m = pt->M;

/* snapping coords to the given grid */
    if (size_x > 0.0)
	x = rint ((x - origin_x) / size_x) * size_x + origin_x;
    if (size_y > 0.0)
	y = rint ((y - origin_y) / size_y) * size_y + origin_y;
    if (has_z && size_z > 0.0)
	z = rint ((z - origin_z) / size_z) * size_z + origin_z;
    if (has_m && size_m > 0.0)
	m = rint ((m - origin_m) / size_m) * size_m + origin_m;

    ptx = result->FirstPoint;
    while (ptx)
      {
	  /* checking if already defined */
	  if (has_z && has_m)
	    {
		if (ptx->X == x && ptx->Y == y && ptx->Z == z && ptx->M == m)
		    return;
	    }
	  else if (has_z)
	    {
		if (ptx->X == x && ptx->Y == y && ptx->Z == z)
		    return;
	    }
	  else if (has_m)
	    {
		if (ptx->X == x && ptx->Y == y && ptx->M == m)
		    return;
	    }
	  else
	    {
		if (ptx->X == x && ptx->Y == y)
		    return;
	    }
	  ptx = ptx->Next;
      }

/* inserting the snapped Point into the result Geometry */
    if (has_z && has_m)
	gaiaAddPointToGeomCollXYZM (result, x, y, z, m);
    else if (has_z)
	gaiaAddPointToGeomCollXYZ (result, x, y, z);
    else if (has_m)
	gaiaAddPointToGeomCollXYM (result, x, y, m);
    else
	gaiaAddPointToGeomColl (result, x, y);
}

static void
auxGridSnapLinestring (gaiaLinestringPtr ln, gaiaGeomCollPtr result,
		       double origin_x, double origin_y, double origin_z,
		       double origin_m, double size_x, double size_y,
		       double size_z, double size_m)
{
/* snapping a Linestring to a regular Grid */
    double x;
    double y;
    double z;
    double m;
    int has_z = 0;
    int has_m = 0;
    int iv;
    gaiaDynamicLinePtr dyn;
    gaiaPointPtr pt;
    gaiaLinestringPtr lnx;
    int count = 0;

    if (ln == NULL || result == NULL)
	return;
    if (ln->DimensionModel == GAIA_XY_Z || ln->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    if (ln->DimensionModel == GAIA_XY_M || ln->DimensionModel == GAIA_XY_Z_M)
	has_m = 1;
    dyn = gaiaAllocDynamicLine ();

    for (iv = 0; iv < ln->Points; iv++)
      {
	  /* snapping each Vertex to the given grid */
	  int to_be_inserted = 0;
	  z = 0.0;
	  m = 0.0;
	  if (has_z && has_m)
	    {
		gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
	    }
	  else if (has_z)
	    {
		gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
	    }
	  else if (has_m)
	    {
		gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
	    }
	  else
	    {
		gaiaGetPoint (ln->Coords, iv, &x, &y);
	    }
	  /* snapping coords to the given grid */
	  if (size_x > 0.0)
	      x = rint ((x - origin_x) / size_x) * size_x + origin_x;
	  if (size_y > 0.0)
	      y = rint ((y - origin_y) / size_y) * size_y + origin_y;
	  if (has_z && size_z > 0.0)
	      z = rint ((z - origin_z) / size_z) * size_z + origin_z;
	  if (has_m && size_m > 0.0)
	      m = rint ((m - origin_m) / size_m) * size_m + origin_m;

	  if (dyn->Last == NULL)
	      to_be_inserted = 1;
	  else
	    {
		/* skipping repeated points */
		pt = dyn->Last;
		if (has_z && has_m)
		  {
		      if (pt->X == x && pt->Y == y && pt->Z == z && pt->M == m)
			  ;
		      else
			  to_be_inserted = 1;
		  }
		else if (has_z)
		  {
		      if (pt->X == x && pt->Y == y && pt->Z == z)
			  ;
		      else
			  to_be_inserted = 1;
		  }
		else if (has_m)
		  {
		      if (pt->X == x && pt->Y == y && pt->M == m)
			  ;
		      else
			  to_be_inserted = 1;
		  }
		else
		  {
		      if (pt->X == x && pt->Y == y)
			  ;
		      else
			  to_be_inserted = 1;
		  }
	    }
	  if (to_be_inserted)
	    {
		if (has_z && has_m)
		    gaiaAppendPointZMToDynamicLine (dyn, x, y, z, m);
		else if (has_z)
		    gaiaAppendPointZToDynamicLine (dyn, x, y, z);
		else if (has_m)
		    gaiaAppendPointMToDynamicLine (dyn, x, y, m);
		else
		    gaiaAppendPointToDynamicLine (dyn, x, y);
	    }
      }

/* checking for validity */
    pt = dyn->First;
    while (pt)
      {
	  /* counting how many points are there */
	  count++;
	  pt = pt->Next;
      }
    if (count < 2)
      {
	  /* skipping any collapsed line */
	  gaiaFreeDynamicLine (dyn);
	  return;
      }

/* inserting into the result Geometry */
    lnx = gaiaAddLinestringToGeomColl (result, count);
    iv = 0;
    pt = dyn->First;
    while (pt)
      {
	  /* copying points */
	  if (lnx->DimensionModel == GAIA_XY_Z)
	    {
		gaiaSetPointXYZ (lnx->Coords, iv, pt->X, pt->Y, pt->Z);
	    }
	  else if (lnx->DimensionModel == GAIA_XY_M)
	    {
		gaiaSetPointXYM (lnx->Coords, iv, pt->X, pt->Y, pt->M);
	    }
	  else if (lnx->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaSetPointXYZM (lnx->Coords, iv, pt->X, pt->Y, pt->Z, pt->M);
	    }
	  else
	    {
		gaiaSetPoint (lnx->Coords, iv, pt->X, pt->Y);
	    }
	  iv++;
	  pt = pt->Next;
      }
    gaiaFreeDynamicLine (dyn);
}

static gaiaDynamicLinePtr
auxGridSnapRing (gaiaRingPtr rng, double origin_x, double origin_y,
		 double origin_z, double origin_m, double size_x, double size_y,
		 double size_z, double size_m)
{
/* snapping a Ring to a regular Grid */
    double x;
    double y;
    double z;
    double m;
    int has_z = 0;
    int has_m = 0;
    int iv;
    gaiaDynamicLinePtr dyn;
    gaiaPointPtr pt0;
    gaiaPointPtr pt;
    int count = 0;

    if (rng == NULL)
	return NULL;
    if (rng->DimensionModel == GAIA_XY_Z || rng->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    if (rng->DimensionModel == GAIA_XY_M || rng->DimensionModel == GAIA_XY_Z_M)
	has_m = 1;
    dyn = gaiaAllocDynamicLine ();

    for (iv = 0; iv < rng->Points; iv++)
      {
	  /* snapping each Vertex to the given grid */
	  int to_be_inserted = 0;
	  z = 0.0;
	  m = 0.0;
	  if (has_z && has_m)
	    {
		gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
	    }
	  else if (has_z)
	    {
		gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
	    }
	  else if (has_m)
	    {
		gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
	    }
	  else
	    {
		gaiaGetPoint (rng->Coords, iv, &x, &y);
	    }
	  /* snapping coords to the given grid */
	  if (size_x > 0.0)
	      x = rint ((x - origin_x) / size_x) * size_x + origin_x;
	  if (size_y > 0.0)
	      y = rint ((y - origin_y) / size_y) * size_y + origin_y;
	  if (has_z && size_z > 0.0)
	      z = rint ((z - origin_z) / size_z) * size_z + origin_z;
	  if (has_m && size_m > 0.0)
	      m = rint ((m - origin_m) / size_m) * size_m + origin_m;

	  if (dyn->Last == NULL)
	      to_be_inserted = 1;
	  else
	    {
		/* skipping repeated points */
		pt = dyn->Last;
		if (has_z && has_m)
		  {
		      if (pt->X == x && pt->Y == y && pt->Z == z && pt->M == m)
			  ;
		      else
			  to_be_inserted = 1;
		  }
		else if (has_z)
		  {
		      if (pt->X == x && pt->Y == y && pt->Z == z)
			  ;
		      else
			  to_be_inserted = 1;
		  }
		else if (has_m)
		  {
		      if (pt->X == x && pt->Y == y && pt->M == m)
			  ;
		      else
			  to_be_inserted = 1;
		  }
		else
		  {
		      if (pt->X == x && pt->Y == y)
			  ;
		      else
			  to_be_inserted = 1;
		  }
	    }
	  if (to_be_inserted)
	    {
		if (has_z && has_m)
		    gaiaAppendPointZMToDynamicLine (dyn, x, y, z, m);
		else if (has_z)
		    gaiaAppendPointZToDynamicLine (dyn, x, y, z);
		else if (has_m)
		    gaiaAppendPointMToDynamicLine (dyn, x, y, m);
		else
		    gaiaAppendPointToDynamicLine (dyn, x, y);
	    }
      }
/* ensuring for Ring closure */
    pt0 = dyn->First;
    pt = dyn->Last;
    if (has_z && has_m)
      {
	  if (pt0->X == pt->X && pt0->Y == pt->Y && pt0->Z == pt->Z
	      && pt0->M == pt->M)
	      ;
	  else
	      gaiaAppendPointZMToDynamicLine (dyn, pt->X, pt->Y, pt->Z, pt->M);
      }
    else if (has_z)
      {
	  if (pt0->X == pt->X && pt0->Y == pt->Y && pt0->Z == pt->Z)
	      ;
	  else
	      gaiaAppendPointZToDynamicLine (dyn, pt->X, pt->Y, pt->Z);
      }
    else if (has_m)
      {
	  if (pt0->X == pt->X && pt0->Y == pt->Y && pt0->M == pt->M)
	      ;
	  else
	      gaiaAppendPointMToDynamicLine (dyn, pt->X, pt->Y, pt->M);
      }
    else
      {
	  if (pt0->X == pt->X && pt0->Y == pt->Y)
	      ;
	  else
	      gaiaAppendPointToDynamicLine (dyn, pt->X, pt->Y);
      }

/* checking for validity */
    pt = dyn->First;
    while (pt)
      {
	  /* counting how many points are there */
	  count++;
	  pt = pt->Next;
      }
    if (count < 4)
      {
	  /* skipping any collapsed ring */
	  gaiaFreeDynamicLine (dyn);
	  return NULL;
      }
    return dyn;
}

static void
auxGridSnapPolygon (gaiaPolygonPtr pg, gaiaGeomCollPtr result, double origin_x,
		    double origin_y, double origin_z, double origin_m,
		    double size_x, double size_y, double size_z, double size_m)
{
/* snapping a Polygon to a regular Grid */
    int ib;
    int holes = 0;
    int count;
    int next_hole = 0;
    int iv;
    gaiaRingPtr rng;
    gaiaPolygonPtr pgx;
    gaiaPointPtr pt;
    gaiaDynamicLinePtr rng_ext;
    gaiaDynamicLinePtr dyn;
    gaiaDynamicLinePtr *rng_ints = NULL;

    if (pg == NULL || result == NULL)
	return;
/* snapping the Exterior Ring */
    rng = pg->Exterior;
    rng_ext =
	auxGridSnapRing (rng, origin_x, origin_y, origin_z, origin_m, size_x,
			 size_y, size_z, size_m);
    if (rng_ext == NULL)	/* skipping any collaped Polygon */
	return;

    if (pg->NumInteriors)
      {
	  /* snapping any Interior Ring */
	  rng_ints = malloc (sizeof (gaiaRingPtr *) * pg->NumInteriors);
	  for (ib = 0; ib < pg->NumInteriors; ib++)
	    {
		rng = pg->Interiors + ib;
		*(rng_ints + ib) =
		    auxGridSnapRing (rng, origin_x, origin_y, origin_z,
				     origin_m, size_x, size_y, size_z, size_m);
		if (*(rng_ints + ib) != NULL)
		    holes++;
	    }
      }

/* inserting into the result Geometry */
    pt = rng_ext->First;
    count = 0;
    while (pt)
      {
	  /* counting how many points are in the Exterior Ring */
	  count++;
	  pt = pt->Next;
      }
    pgx = gaiaAddPolygonToGeomColl (result, count, holes);
    rng = pgx->Exterior;
    iv = 0;
    pt = rng_ext->First;
    while (pt)
      {
	  /* copying Exterior Ring points */
	  if (rng->DimensionModel == GAIA_XY_Z)
	    {
		gaiaSetPointXYZ (rng->Coords, iv, pt->X, pt->Y, pt->Z);
	    }
	  else if (rng->DimensionModel == GAIA_XY_M)
	    {
		gaiaSetPointXYM (rng->Coords, iv, pt->X, pt->Y, pt->M);
	    }
	  else if (rng->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaSetPointXYZM (rng->Coords, iv, pt->X, pt->Y, pt->Z, pt->M);
	    }
	  else
	    {
		gaiaSetPoint (rng->Coords, iv, pt->X, pt->Y);
	    }
	  iv++;
	  pt = pt->Next;
      }
    if (holes > 0)
      {
	  /* setting up any not-collapsed Hole */
	  for (ib = 0; ib < pg->NumInteriors; ib++)
	    {
		if (*(rng_ints + ib) == NULL)
		    continue;
		dyn = *(rng_ints + ib);
		pt = dyn->First;
		count = 0;
		while (pt)
		  {
		      /* counting how many points are in the Exterior Ring */
		      count++;
		      pt = pt->Next;
		  }
		rng = gaiaAddInteriorRing (pgx, next_hole++, count);
		iv = 0;
		pt = dyn->First;
		while (pt)
		  {
		      /* copying Interior Ring points */
		      if (rng->DimensionModel == GAIA_XY_Z)
			{
			    gaiaSetPointXYZ (rng->Coords, iv, pt->X, pt->Y,
					     pt->Z);
			}
		      else if (rng->DimensionModel == GAIA_XY_M)
			{
			    gaiaSetPointXYM (rng->Coords, iv, pt->X, pt->Y,
					     pt->M);
			}
		      else if (rng->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaSetPointXYZM (rng->Coords, iv, pt->X, pt->Y,
					      pt->Z, pt->M);
			}
		      else
			{
			    gaiaSetPoint (rng->Coords, iv, pt->X, pt->Y);
			}
		      iv++;
		      pt = pt->Next;
		  }
	    }
      }

/* memory clean-up */
    gaiaFreeDynamicLine (rng_ext);
    if (rng_ints)
      {
	  for (ib = 0; ib < pg->NumInteriors; ib++)
	    {
		dyn = *(rng_ints + ib);
		if (dyn)
		    gaiaFreeDynamicLine (dyn);
	    }
	  free (rng_ints);
      }
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaSnapToGrid (gaiaGeomCollPtr geom, double origin_x, double origin_y,
		double origin_z, double origin_m, double size_x, double size_y,
		double size_z, double size_m)
{
/* creating a Geometry snapped to a regular Grid */
    gaiaGeomCollPtr result;
    int pts = 0;
    int lns = 0;
    int pgs = 0;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;

    if (!geom)
	return NULL;

/* creating the output Geometry */
    if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaAllocGeomCollXYZ ();
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaAllocGeomCollXYM ();
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaAllocGeomCollXYZM ();
    else
	result = gaiaAllocGeomColl ();

/* snapping elementary Geometries to the given Grid */
    pt = geom->FirstPoint;
    while (pt)
      {
	  /* snapping POINTs */
	  auxGridSnapPoint (geom->DimensionModel, pt, result, origin_x,
			    origin_y, origin_z, origin_m, size_x, size_y,
			    size_z, size_m);
	  pt = pt->Next;
      }
    ln = geom->FirstLinestring;
    while (ln)
      {
	  /* snapping LINESTRINGs */
	  auxGridSnapLinestring (ln, result, origin_x, origin_y, origin_z,
				 origin_m, size_x, size_y, size_z, size_m);
	  ln = ln->Next;
      }
    pg = geom->FirstPolygon;
    while (pg)
      {
	  /* snapping POLYGONs */
	  auxGridSnapPolygon (pg, result, origin_x, origin_y, origin_z,
			      origin_m, size_x, size_y, size_z, size_m);
	  pg = pg->Next;
      }

/* validating the output Geometry */
    pt = result->FirstPoint;
    while (pt)
      {
	  /* counting how many POINTs are there */
	  pts++;
	  pt = pt->Next;
      }
    ln = result->FirstLinestring;
    while (ln)
      {
	  /* counting how many LINESTRINGs are there */
	  lns++;
	  ln = ln->Next;
      }
    pg = result->FirstPolygon;
    while (pg)
      {
	  /* counting how many POLYGONs are there */
	  pgs++;
	  pg = pg->Next;
      }
    if (pts == 0 && lns == 0 && pgs == 0)
      {
	  /* empty result */
	  gaiaFreeGeomColl (result);
	  return NULL;
      }

/* final adjustment */
    result->Srid = geom->Srid;
    if (pts == 1 && lns == 0 && pgs == 0)
      {
	  if (geom->DeclaredType == GAIA_GEOMETRYCOLLECTION)
	      result->DeclaredType = GAIA_GEOMETRYCOLLECTION;
	  else if (geom->DeclaredType == GAIA_MULTIPOINT)
	      result->DeclaredType = GAIA_MULTIPOINT;
	  else
	      result->DeclaredType = GAIA_POINT;
      }
    else if (pts == 0 && lns == 1 && pgs == 0)
      {
	  if (geom->DeclaredType == GAIA_GEOMETRYCOLLECTION)
	      result->DeclaredType = GAIA_GEOMETRYCOLLECTION;
	  else if (geom->DeclaredType == GAIA_MULTILINESTRING)
	      result->DeclaredType = GAIA_MULTILINESTRING;
	  else
	      result->DeclaredType = GAIA_LINESTRING;
      }
    else if (pts == 0 && lns == 0 && pgs == 1)
      {
	  if (geom->DeclaredType == GAIA_GEOMETRYCOLLECTION)
	      result->DeclaredType = GAIA_GEOMETRYCOLLECTION;
	  else if (geom->DeclaredType == GAIA_MULTIPOLYGON)
	      result->DeclaredType = GAIA_MULTIPOLYGON;
	  else
	      result->DeclaredType = GAIA_POLYGON;
      }
    else if (pts > 1 && lns == 0 && pgs == 0)
      {
	  if (geom->DeclaredType == GAIA_GEOMETRYCOLLECTION)
	      result->DeclaredType = GAIA_GEOMETRYCOLLECTION;
	  else
	      result->DeclaredType = GAIA_MULTIPOINT;
      }
    else if (pts == 0 && lns > 1 && pgs == 0)
      {
	  if (geom->DeclaredType == GAIA_GEOMETRYCOLLECTION)
	      result->DeclaredType = GAIA_GEOMETRYCOLLECTION;
	  else
	      result->DeclaredType = GAIA_MULTILINESTRING;
      }
    else if (pts == 0 && lns == 0 && pgs > 1)
      {
	  if (geom->DeclaredType == GAIA_GEOMETRYCOLLECTION)
	      result->DeclaredType = GAIA_GEOMETRYCOLLECTION;
	  else
	      result->DeclaredType = GAIA_MULTIPOLYGON;
      }
    else
	result->DeclaredType = GAIA_GEOMETRYCOLLECTION;
    return result;
}
