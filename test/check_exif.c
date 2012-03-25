
/*

 check_exif.c -- SpatiaLite Test Case

 Author: Sandro Furieri <a.furieri@lqt.it>

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

#include <spatialite/gaiaexif.h>

static sqlite3_int64 
get_pixel_x(gaiaExifTagListPtr tag_list, int *ok)
{
/* trying to retrieve the ExifImageWidth */
    gaiaExifTagPtr tag = tag_list->First;
    *ok = 0;

    while (tag)
    {
        if (tag->TagId == 0xA002)
        {
          /* ok, this one is the ExifImageWidth tag */
            if (tag->Type == 3 && tag->Count == 1)
            {
                *ok = 1;
                return *(tag->ShortValues + 0);
            } 
            else if (tag->Type == 4 && tag->Count == 1)
            {
                *ok = 1;
                return *(tag->LongValues + 0);
            }
        }
        tag = tag->Next;
    }
    return 0;
}

static sqlite3_int64 
get_pixel_y(gaiaExifTagListPtr tag_list, int *ok)
{
/* trying to retrieve the ExifImageLength */
    gaiaExifTagPtr tag = tag_list->First;
    *ok = 0;

    while (tag)
    {
        if (tag->TagId == 0xA003)
        {
          /* ok, this one is the ExifImageLength tag */
            if (tag->Type == 3 && tag->Count == 1)
            {
                *ok = 1;
                return *(tag->ShortValues + 0);
            } 
            else if (tag->Type == 4 && tag->Count == 1)
            {
                *ok = 1;
                return *(tag->LongValues + 0);
            }
        }
        tag = tag->Next;
    }
    return 0;
}

static void 
get_make(gaiaExifTagListPtr tag_list, const char **str, int *ok)
{
/* trying to retrieve the Make */
    gaiaExifTagPtr tag = tag_list->First;
    *ok = 0;

    while (tag)
    {
        if (tag->TagId == 0x010F)
        {
        /* ok, this one is the Make tag */ 
            if (tag->Type == 2)
            {
                *ok = 1;
                *str = tag->StringValue;
                return;
            }
        }
      tag = tag->Next;
    }
    return;
}

static void 
get_model(gaiaExifTagListPtr tag_list, const char **str, int *ok)
{
/* trying to retrieve the Model */
    gaiaExifTagPtr tag = tag_list->First;
    *ok = 0;

    while (tag)
    {
        if (tag->TagId == 0x0110)
        {
        /* ok, this one is the Model tag */ 
            if (tag->Type == 2)
            {
                *ok = 1;
                *str = tag->StringValue;
                return;
            }
        }
      tag = tag->Next;
    }
    return;
}

static void 
get_date(gaiaExifTagListPtr tag_list, const char **str, int *ok)
{
/* trying to retrieve the Date */
    gaiaExifTagPtr tag = tag_list->First;
    *ok = 0;

    while (tag)
    {
        if (tag->TagId == 0x9003)
        {
        /* ok, this one is the Make tag */ 
            if (tag->Type == 2)
            {
                *ok = 1;
                *str = tag->StringValue;
                return;
            }
        }
      tag = tag->Next;
    }
    return;
}

int main (int argc, char *argv[])
{
    int ret;
    sqlite3 *handle;
    char *err_msg = NULL;
    FILE *fl;
    int sz = 0;
    int rd;
    int ok;
    int i;
    int len;
    char tag_name[128];
    char human[8192];
    gaiaExifTagPtr pT;
    sqlite3_int64 val64;
    const char *result;
    unsigned char *blob = NULL;
    gaiaExifTagListPtr tag_list = NULL;

    spatialite_init (0);
    
    fl = fopen("sql_stmt_tests/DSC_1467.JPG", "rb");
    if (!fl)
    if (ret != SQLITE_OK) {
	fprintf(stderr, "cannot open EXIF-JPEG image: DSC_1467.JPG\n");
	return -1;
    }
    if (fseek(fl, 0, SEEK_END) == 0)
        sz = ftell(fl);
    if (sz <= 14) {
	fprintf(stderr, "invalid size EXIF-JPEG image: DSC_1467.JPG\n");
	return -2;
    }
    
    blob = (unsigned char *) malloc(sz);
    rewind(fl);
    rd = fread(blob, 1, sz, fl);
    if (rd != sz) {
	fprintf(stderr, "read error EXIF-JPEG image: DSC_1467.JPG\n");
	return -3;
    }
    
    tag_list = gaiaGetExifTags(blob, sz);
    if (tag_list == NULL) {
	fprintf(stderr, "cannot extract EXIF tags from JPEG image: DSC_1467.JPG\n");
	return -4;
    }

    val64 = get_pixel_x(tag_list, &ok);
    if (!ok) {
	fprintf(stderr, "DSC_1467.JPG: PixelX tag not found\n");
	return -5;
    }
    if (val64 != 2128) {
	fprintf(stderr, "DSC_1467.JPG: PixelX unexpected value: %d\n", (int)val64);
	return -6;
    }

    val64 = get_pixel_y(tag_list, &ok); 
    if (!ok) {
	fprintf(stderr, "DSC_1467.JPG: PixelY tag not found\n");
	return -7;
    }
    if (val64 != 1416) {
	fprintf(stderr, "DSC_1467.JPG: PixelY unexpected value: %d\n", (int)val64);
	return -8;
    }
  
    get_make(tag_list, &result, &ok);
    if (!ok) {
	fprintf(stderr, "DSC_1467.JPG: Make tag not found\n");
	return -9;
    }
    if (strcmp(result, "NIKON CORPORATION") != 0) {
	fprintf(stderr, "DSC_1467.JPG: Make unexpected value: %s|\n", result);
	return -10;
    };

    get_model(tag_list, &result, &ok);
    if (!ok) {
	fprintf(stderr, "DSC_1467.JPG: Model tag not found\n");
	return -11;
    }
    if (strcmp(result, "NIKON D700") != 0) {
	fprintf(stderr, "DSC_1467.JPG: Model unexpected value: %s|\n", result);
	return -12;
    };

    get_date(tag_list, &result, &ok);
    if (!ok) {
	fprintf(stderr, "DSC_1467.JPG: Date tag not found\n");
	return -13;
    }
    if (strcmp(result, "2011:03:26 11:01:13") != 0) {
	fprintf(stderr, "DSC_1467.JPG: Date unexpected value: %s|\n", result);
	return -14;
    };
            
    for (i = 0; i < gaiaGetExifTagsCount(tag_list); i++)
    {
        pT = gaiaGetExifTagByPos(tag_list, i);
        if (pT)
        {
            gaiaExifTagGetName(pT, tag_name, 128);
            gaiaExifTagGetValueType(pT);
            gaiaIsExifGpsTag(pT);
            gaiaExifTagGetValueType(pT);
            gaiaExifTagGetNumValues(pT);
            gaiaExifTagGetHumanReadable (pT, human, 8190, &ok);
        }
    }

    pT = gaiaGetExifTagById (tag_list, 0x0112);
    if (pT == NULL) {
	fprintf(stderr, "DSC_1467.JPG: tag Orientation not found: %s|\n", result);
	return -15;
    };

    pT = gaiaGetExifTagByName (tag_list, "YCbCrPositioning");
    if (pT == NULL) {
	fprintf(stderr, "DSC_1467.JPG: tag YCbCrPositioning not found: %s|\n", result);
	return -16;
    };

    for (i = 0x0000; i < 0xffff; i++)
    {
        gaiaExifTag tag;
        tag.Gps = 0;
        tag.TagId = i;
        gaiaExifTagGetName(&tag, tag_name, 128);
    }
    for (i = 0x0000; i < 0xffff; i++)
    {
        gaiaExifTag tag;
        tag.Gps = 1;
        tag.TagId = i;
        gaiaExifTagGetName(&tag, tag_name, 128);
    }

    if (blob)
        free(blob);
    if (tag_list)
        gaiaExifTagsFree(tag_list);
    fclose(fl);
    
    spatialite_cleanup();
    
    return 0;
}