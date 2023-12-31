#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(libspatialite, 5.1.1-rc0, a.furieri@lqt.it)
AC_LANG(C)
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign no-define])
AM_MAINTAINER_MODE
AC_CONFIG_HEADERS(./fakeconfig.h ./config.h ./src/headers/spatialite/gaiaconfig.h ./config-msvc.h ./src/headers/spatialite/gaiaconfig-msvc.h)

# enabling huge-file support (64 bit file pointers)
AH_TEMPLATE([_LARGE_FILE],
            [Must be defined in order to enable huge-file support.])
AC_DEFINE(_LARGE_FILE)
AH_TEMPLATE([_FILE_OFFSET_BITS],
            [Must be =64 in order to enable huge-file support.])
AC_DEFINE(_FILE_OFFSET_BITS, 64)
AH_TEMPLATE([_LARGEFILE_SOURCE],
            [Must be defined in order to enable huge-file support.])
AC_DEFINE(_LARGEFILE_SOURCE)

# disabling debug support
AH_TEMPLATE([NDEBUG],
            [Must be defined in order to disable debug mode.])
AC_DEFINE(NDEBUG)

# config depending options
AH_TEMPLATE([OMIT_GEOS],
            [Should be defined in order to disable GEOS support.])
AH_TEMPLATE([GEOS_ADVANCED],
            [Should be defined in order to enable GEOS_ADVANCED support.])
AH_TEMPLATE([GEOS_REENTRANT],
            [Should be defined in order to enable GEOS_REENTRANT (fully thread-safe).])
AH_TEMPLATE([GEOS_ONLY_REENTRANT],
            [Should be defined in order to fully disable GEOS non-thread-safe API.])
AH_TEMPLATE([GEOS_370],
            [Should be defined in order to enable GEOS_370 support.])
AH_TEMPLATE([GEOS_390],
            [Should be defined in order to enable GEOS_390 support.])
AH_TEMPLATE([GEOS_3100],
            [Should be defined in order to enable GEOS_3100 support.])
AH_TEMPLATE([GEOS_3110],
            [Should be defined in order to enable GEOS_3110 support.])
AH_TEMPLATE([PROJ_NEW],
            [Should be defined in order to enable PROJ.6 support.])
AH_TEMPLATE([ENABLE_RTTOPO],
            [Should be defined in order to enable RTTOPO support.])
AH_TEMPLATE([ENABLE_GCP],
            [Should be defined in order to enable GCP support.])
AH_TEMPLATE([OMIT_PROJ],
            [Should be defined in order to disable PROJ.4 support.])
AH_TEMPLATE([OMIT_ICONV],
            [Should be defined in order to disable ICONV support.])
AH_TEMPLATE([OMIT_MATHSQL],
            [Should be defined in order to disable MATHSQL support.])
AH_TEMPLATE([OMIT_EPSG],
            [Should be defined in order to disable EPSG full support.])
AH_TEMPLATE([OMIT_KNN],
            [Should be defined in order to disable KNN support.])
AH_TEMPLATE([OMIT_FREEXL],
            [Should be defined in order to disable FREEXL support.])
AH_TEMPLATE([ENABLE_LIBXML2],
            [Should be defined in order to enable LIBXML2 support.])
AH_TEMPLATE([ENABLE_MINIZIP],
            [Should be defined in order to enable MiniZIP support.])
AH_TEMPLATE([ENABLE_GEOPACKAGE],
            [Should be defined in order to enable GeoPackage support.])
AH_TEMPLATE([SPATIALITE_TARGET_CPU],
            [Should contain a text-string describing the intended target CPU])
AH_TEMPLATE([SPATIALITE_VERSION],
            [the Version of this package.])
AH_TEMPLATE([HAVE_DECL_SQLITE_INDEX_CONSTRAINT_LIKE],
            [depending on SQLite library version.])

# Checks for header files.
AC_CHECK_HEADERS(stdlib.h,, [AC_MSG_ERROR([cannot find stdlib.h, bailing out])])
AC_CHECK_HEADERS(stdio.h,, [AC_MSG_ERROR([cannot find stdio.h, bailing out])])
AC_CHECK_HEADERS(string.h,, [AC_MSG_ERROR([cannot find string.h, bailing out])])
AC_CHECK_HEADERS(memory.h,, [AC_MSG_ERROR([cannot find memory.h, bailing out])])
AC_CHECK_HEADERS(math.h,, [AC_MSG_ERROR([cannot find math.h, bailing out])])
AC_CHECK_HEADERS(float.h,, [AC_MSG_ERROR([cannot find float.h, bailing out])])
AC_CHECK_HEADERS(fcntl.h,, [AC_MSG_ERROR([cannot find fcntl.h, bailing out])])
AC_CHECK_HEADERS(inttypes.h,, [AC_MSG_ERROR([cannot find inttypes.h, bailing out])])
AC_CHECK_HEADERS(stddef.h,, [AC_MSG_ERROR([cannot find stddef.h, bailing out])])
AC_CHECK_HEADERS(stdint.h,, [AC_MSG_ERROR([cannot find stdint.h, bailing out])])
AC_CHECK_HEADERS(sys/time.h,, [AC_MSG_ERROR([cannot find sys/time.h, bailing out])])
AC_CHECK_HEADERS(unistd.h,, [AC_MSG_ERROR([cannot find unistd.h, bailing out])])
AC_CHECK_HEADERS(sqlite3.h,, [AC_MSG_ERROR([cannot find sqlite3.h, bailing out])])
AC_CHECK_HEADERS(sqlite3ext.h,, [AC_MSG_ERROR([cannot find sqlite3ext.h, bailing out])])
AC_CHECK_HEADERS(zlib.h,, [AC_MSG_ERROR([cannot find zlib.h, bailing out])])


# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
LT_INIT(win32-dll)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_C_VOLATILE

# Checks for library functions.
AC_FUNC_LSTAT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MEMCMP
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([memset sqrt strcasecmp strerror strncasecmp strstr fdatasync ftruncate getcwd gettimeofday localtime_r memmove strerror])

# Checks for installed libraries
# AC_CHECK_LIB(sqlite3,sqlite3_prepare_v2,,AC_MSG_ERROR(['libsqlite3' is required but it doesn't seem to be installed on this system.]),-lm)
AC_CHECK_LIB(z,inflateInit_,,AC_MSG_ERROR(['libz' is required but it doesn't seem to be installed on this system.]),-lm)

AC_CONFIG_FILES([Makefile \
		src/Makefile \
		src/headers/Makefile \
		src/gaiaaux/Makefile \
		src/gaiaexif/Makefile \
		src/gaiageo/Makefile \
		src/gaiageo/flex/Makefile \
		src/gaiageo/lemon/Makefile \
		src/gaiageo/lemon/lemon_src/Makefile \
		src/geopackage/Makefile \
		src/spatialite/Makefile \
		src/shapefiles/Makefile \
		src/dxf/Makefile \
		src/md5/Makefile \
		src/control_points/Makefile \
		src/cutter/Makefile \
		src/topology/Makefile \
		src/srsinit/Makefile \
		src/srsinit/epsg_update/Makefile \
		src/stored_procedures/Makefile \
		src/connection_cache/Makefile \
		src/connection_cache/generator/Makefile \
		src/virtualtext/Makefile \
		src/wfs/Makefile \
		test/Makefile \
		test/sql_stmt_tests/Makefile \
		test/sql_stmt_geos_tests/Makefile \
		test/sql_stmt_geos370_tests/Makefile \
		test/sql_stmt_geos390_tests/Makefile \
		test/sql_stmt_geos3100_tests/Makefile \
		test/sql_stmt_geos3110_tests/Makefile \
		test/sql_stmt_geos_380/Makefile \
		test/sql_stmt_geos_non380/Makefile \
		test/sql_stmt_geos_390/Makefile \
		test/sql_stmt_geos_non390/Makefile \
		test/sql_stmt_geos_391/Makefile \
		test/sql_stmt_geos_3100/Makefile \
		test/sql_stmt_geos_non3100/Makefile \
		test/sql_stmt_geos_3110/Makefile \
		test/sql_stmt_geos_non3110/Makefile \
		test/sql_stmt_geos_3120/Makefile \
		test/sql_stmt_geos_non3120/Makefile \
		test/sql_stmt_geosadvanced_tests/Makefile \
		test/sql_stmt_geopackage_tests/Makefile \
		test/sql_stmt_gpkg_epsg492_tests/Makefile \
		test/sql_stmt_gpkg_epsg493_tests/Makefile \
		test/sql_stmt_gpkg_epsg600_tests/Makefile \
		test/sql_stmt_proj_tests/Makefile \
		test/sql_stmt_proj492_tests/Makefile \
		test/sql_stmt_proj493_tests/Makefile \
		test/sql_stmt_proj600_tests/Makefile \
		test/sql_stmt_proj720_tests/Makefile \
		test/sql_stmt_proj910_tests/Makefile \
		test/sql_stmt_proj600security_tests/Makefile \
		test/sql_stmt_mathsql_tests/Makefile \
		test/sql_stmt_rtgeom_tests/Makefile \
		test/sql_stmt_rttopo_tests/Makefile \
		test/sql_stmt_libxml2_tests/Makefile \
		test/sql_stmt_libxml2_rttopo_tests/Makefile \
		test/sql_stmt_security_tests/Makefile \
		test/sql_stmt_xmlsec_tests/Makefile \
		test/sql_stmt_freexl_tests/Makefile \
		test/sql_stmt_cache_tests/Makefile \
		test/sql_stmt_gpkgcache_tests/Makefile \
		test/sql_stmt_nocache_tests/Makefile \
		test/sql_stmt_gpkgnocache_tests/Makefile \
		test/sql_stmt_voronoj1_tests/Makefile \
		test/sql_stmt_voronoj2_tests/Makefile \
		test/sql_stmt_sequence_tests/Makefile \
		test/sql_stmt_routing_tests/Makefile \
		test/sql_stmt_logfile_tests/Makefile \
		test/sql_stmt_iconv_tests/Makefile \
		test/sql_stmt_proc_tests/Makefile \
		test/sql_stmt_point_geom/Makefile \
		test/sql_stmt_tiny_point/Makefile \
		test/sql_stmt_postgres_tests/Makefile \
		test/sql_stmt_renamenew_tests/Makefile \
		test/sql_stmt_renameold_tests/Makefile \
		test/sql_stmt_bufoptsnew_tests/Makefile \
		test/sql_stmt_bufoptsold_tests/Makefile \
		test/sql_stmt_minizip/Makefile \
		test/sql_stmt_zip_proj6/Makefile \
		test/sql_stmt_rtgeom_390/Makefile \
		test/sql_stmt_rtgeom_non390/Makefile \
		test/sql_stmt_voronoj_390/Makefile \
		test/sql_stmt_voronoj_non390/Makefile \
		examples/Makefile \
		Doxyfile \
		spatialite.pc])

# exporting the TARGET_CPU string
splite_cpu=`$CC -dumpmachine`
AC_DEFINE_UNQUOTED([SPATIALITE_TARGET_CPU], ["$splite_cpu"])

# exporting the VERSION string
AC_DEFINE_UNQUOTED([SPATIALITE_VERSION], ["$PACKAGE_VERSION"])

#-----------------------------------------------------------------------
#   --enable-mathsql
#
AC_ARG_ENABLE(mathsql, [AS_HELP_STRING(
  [--enable-mathsql], [enables SQL math functions [default=yes]])],
  [], [enable_mathsql=yes])
if test x"$enable_mathsql" != "xyes"; then
  AC_DEFINE(OMIT_MATHSQL)
fi
#-----------------------------------------------------------------------

PKG_CHECK_MODULES([SQLITE3], [sqlite3], , AC_MSG_ERROR(['libsqlite3' is required but it doesn't seem to be installed on this system.]))
AC_SUBST(SQLITE3_LIBS)

#-----------------------------------------------------------------------
#   --enable-proj
#
AC_ARG_ENABLE(proj, [AS_HELP_STRING(
  [--enable-proj], [enables PROJ.4 inclusion [default=yes]])],
  [], [enable_proj=yes])
if test x"$enable_proj" != "xno"; then
  AC_CHECK_HEADERS(proj.h, [proj_new_incl=1], 
	[AC_CHECK_HEADERS(proj_api.h, [proj_new_incl=0], [AC_MSG_ERROR([cannot find proj_api.h, bailing out])])])
  AC_SEARCH_LIBS(proj_normalize_for_visualization, proj, [proj_new_lib=1],
	[AC_SEARCH_LIBS(pj_init_plus, proj, [proj_new_lib=0], [AC_MSG_ERROR(['libproj' is required but it doesn't seem to be installed on this system.])], [-lm -lpthread -lsqlite3])])
  if test $proj_new_incl -eq 1 && test $proj_new_lib -eq 1; then
    AC_DEFINE(PROJ_NEW)
  fi
else
  AC_DEFINE(OMIT_PROJ)
fi
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
#   --enable-iconv
#
AC_ARG_ENABLE(iconv, [AS_HELP_STRING(
  [--enable-iconv], [enables ICONV inclusion [default=yes]])],
  [], [enable_iconv=yes])
if test x"$enable_iconv" != "xno"; then
  AC_CHECK_HEADERS(iconv.h,, [AC_MSG_ERROR([cannot find iconv.h, bailing out])])
# on some systems "iconv()" lives in libc. On others it lives in libiconv
#  on older systems "libiconv()" lives in libiconv
  AC_SEARCH_LIBS(iconv,iconv,,
    AC_SEARCH_LIBS(libiconv,iconv,,AC_MSG_ERROR(['libiconv' is required but it doesn't seem to be installed on this system.]),))
  AC_SEARCH_LIBS(locale_charset,charset,,
    AC_SEARCH_LIBS(nl_langinfo,c,,AC_MSG_ERROR(['libcharset' is required but it doesn't seem to be installed on this system.]),))
else
  AC_DEFINE(OMIT_ICONV)
fi

#-----------------------------------------------------------------------
#   --enable-freexl
#
AC_ARG_ENABLE(freexl, [AS_HELP_STRING(
  [--enable-freexl], [enables FreeXL inclusion [default=yes]])],
  [], [enable_freexl=yes])
if test x"$enable_freexl" != "xno"; then
  AC_CHECK_HEADERS(freexl.h,, [AC_MSG_ERROR([cannot find freexl.h, bailing out])])
  AC_SEARCH_LIBS(freexl_open_ods,freexl,,AC_MSG_ERROR([could not find 'freexl' (or obsolete 'freexl' < v.2.0.0 found).]),-lm)
else
  AC_DEFINE(OMIT_FREEXL)
fi

#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
#   --enable-epsg
#
AC_ARG_ENABLE(epsg, [AS_HELP_STRING(
  [--enable-epsg], [enables full EPSG dataset support [default=yes]])],
  [], [enable_epsg=yes])
if test x"$enable_epsg" != "xyes"; then
  AC_DEFINE(OMIT_EPSG)
fi
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
#   --enable-geos
#
AC_ARG_ENABLE(geos, [AS_HELP_STRING(
  [--enable-geos], [enables GEOS inclusion [default=yes]])],
  [], [enable_geos=yes])
if test x"$enable_geos" != "xno"; then
  #-----------------------------------------------------------------------
  #   --with-geosconfig
  #
  AC_ARG_WITH([geosconfig],
        [AS_HELP_STRING([--with-geosconfig=FILE], [specify an alternative geos-config file])],
	[GEOSCONFIG="$withval"], [GEOSCONFIG=""])
  if test "x$GEOSCONFIG" = "x"; then
        # GEOSCONFIG was not specified, so search within the current path
        AC_PATH_PROG([GEOSCONFIG], [geos-config])	
        # If we couldn't find geos-config, display an error
        if test "x$GEOSCONFIG" = "x"; then
                AC_MSG_ERROR([could not find geos-config within the current path. You may need to try re-running configure with a --with-geosconfig parameter.])
        fi
  else
        # GEOSCONFIG was specified; display a message to the user
        if test "x$GEOSCONFIG" = "xyes"; then
                AC_MSG_ERROR([you must specify a parameter to --with-geosconfig, e.g. --with-geosconfig=/path/to/geos-config])
        else
                if test -f $GEOSCONFIG; then
                        AC_MSG_RESULT([Using user-specified geos-config file: $GEOSCONFIG])
                else
                        AC_MSG_ERROR([the user-specified geos-config file $GEOSCONFIG does not exist])
                fi     
        fi
  fi
  # Extract the linker and include flags
  GEOS_LDFLAGS=`$GEOSCONFIG --ldflags`
  GEOS_CFLAGS=-I`$GEOSCONFIG --includes`
  AC_SUBST([GEOS_LDFLAGS])
  AC_SUBST([GEOS_CFLAGS])	
  # Ensure that we can parse geos_c.h
  CPPFLAGS_SAVE="$CPPFLAGS"
  CPPFLAGS="$GEOS_CFLAGS"
  AC_CHECK_HEADERS([geos_c.h],, [AC_MSG_ERROR([could not find geos_c.h - you may need to specify the directory of a geos-config file using --with-geosconfig])])
  CPPFLAGS="$CPPFLAGS_SAVE"	
  # Ensure we can link against libgeos_c
  LIBS_SAVE="$LIBS"
  LIBS="$GEOS_LDFLAGS"
  AC_SEARCH_LIBS(GEOSCoveredBy,geos_c,,AC_MSG_ERROR([could not find libgeos_c (or obsolete 'libgeos_c' < v.3.3.0 found) - you may need to specify the directory of a geos-config file using --with-geosconfig]))
  LIBS="$LIBS_SAVE"
  LIBS="$LIBS $GEOS_LDFLAGS -lgeos_c"

  #-----------------------------------------------------------------------
  #   --enable-controlpoints
  #
  AC_ARG_ENABLE(gcp, [AS_HELP_STRING(
	  [--enable-gcp], [enables Control Points (from Grass GIS) [default=yes]])],
	  [], [enable_gcp=yes])
  if test x"$enable_gcp" != "xno"; then
	  AC_DEFINE(ENABLE_GCP)
  fi

  #-----------------------------------------------------------------------
  #   --enable-geosadvanced
  #
  AC_ARG_ENABLE(geosadvanced, [AS_HELP_STRING(
	  [--enable-geosadvanced], [enables GEOS advanced features [default=yes]])],
	  [], [enable_geosadvanced=yes])
  if test x"$enable_geosadvanced" != "xno"; then
	  AC_SEARCH_LIBS(GEOSDelaunayTriangulation,geos_c,,AC_MSG_ERROR(['libgeos_c' (>= v.3.4.0) is required but it doesn't seem to be installed on this system. You may need to try re-running configure with a --disable-geosadvanced parameter.]))
	  AC_DEFINE(GEOS_ADVANCED)
  fi

  #-----------------------------------------------------------------------
  #   --enable-geosreentrant
  #
  AC_ARG_ENABLE(geosreentrant, [AS_HELP_STRING(
	  [--enable-geosreentrant], [enables GEOS reentrant (fully thread safe) [default=yes]])],
	  [], [enable_geosreentrant=yes])
  if test x"$enable_geosreentrant" != "xno"; then
	  AC_SEARCH_LIBS(GEOSContext_setErrorMessageHandler_r,geos_c,,AC_MSG_ERROR(['libgeos_c' (>= v.3.5.0) is required but it doesn't seem to be installed on this system. You may need to try re-running configure with a --disable-geosreentrant parameter.]))
	  AC_DEFINE(GEOS_REENTRANT)
  fi

  #-----------------------------------------------------------------------
  #   --with-geosonlyreentrant
  #
  AC_ARG_WITH(geosonlyreentrant, [AS_HELP_STRING(
	  [--with-geosonlyreentrant], [completely disables GEOS non-thread safe API [default=no]])],
	  [], [with_geosonlyreentrant=no])
  if test x"$with_geosonlyreentrant" != "xno"; then
	  AC_DEFINE(GEOS_ONLY_REENTRANT)
  fi

  #-----------------------------------------------------------------------
  #   --enable-geos370
  #
  AC_ARG_ENABLE(geos370, [AS_HELP_STRING(
	  [--enable-geos370], [enables GEOS 3.7.0 features [default=yes]])],
	  [], [enable_geos370=yes])
  if test x"$enable_geos370" != "xno"; then
	  AC_SEARCH_LIBS(GEOSFrechetDistance_r,geos_c,,AC_MSG_ERROR(['libgeos_c' (>= v.3.7.0) is required but it doesn't seem to be installed on this system. You may need to try re-running configure with a --disable-geos370 parameter.]))
	  AC_DEFINE(GEOS_370)
  fi
  
  #-----------------------------------------------------------------------
  #   --enable-geos390
  #
  AC_ARG_ENABLE(geos390, [AS_HELP_STRING(
	  [--enable-geos390], [enables GEOS 3.9.0 features [default=yes]])],
	  [], [enable_geos390=$enable_geos370])
  if test x"$enable_geos390" != "xno"; then
	  AC_SEARCH_LIBS(GEOSLargestEmptyCircle_r,geos_c,,AC_MSG_ERROR(['libgeos_c' (>= v.3.9.0) is required but it doesn't seem to be installed on this system. You may need to try re-running configure with a --disable-geos390 parameter.]))
	  AC_DEFINE(GEOS_390)
  fi

  #-----------------------------------------------------------------------
  #   --enable-geos3100
  #
  AC_ARG_ENABLE(geos3100, [AS_HELP_STRING(
	  [--enable-geos3100], [enables GEOS 3.10.0 features [default=yes]])],
	  [], [enable_geos3100=$enable_geos390])
  if test x"$enable_geos3100" != "xno"; then
	  AC_SEARCH_LIBS(GEOSConstrainedDelaunayTriangulation_r,geos_c,,AC_MSG_ERROR(['libgeos_c' (>= v.3.10.0) is required but it doesn't seem to be installed on this system. You may need to try re-running configure with a --disable-geos3100 parameter.]))
	  AC_DEFINE(GEOS_3100)
  fi
  
  #-----------------------------------------------------------------------
  #   --enable-geos3110
  #
  AC_ARG_ENABLE(geos3110, [AS_HELP_STRING(
	  [--enable-geos3100], [enables GEOS 3.11.0 features [default=yes]])],
	  [], [enable_geos3110=$enable_geos3100])
  if test x"$enable_geos3110" != "xno"; then
	  AC_SEARCH_LIBS(GEOSHilbertCode,geos_c,,AC_MSG_ERROR(['libgeos_c' (>= v.3.11.0) is required but it doesn't seem to be installed on this system. You may need to try re-running configure with a --disable-geos3110 parameter.]))
	  AC_DEFINE(GEOS_3110)
  fi

  #-----------------------------------------------------------------------
  #   --enable-rttopo
  #
  AC_ARG_ENABLE(rttopo, [AS_HELP_STRING(
	  [--enable-rttopo], [enables RTTOPO support [default=yes]])],
	  [], [enable_rttopo=yes])
  if test x"$enable_rttopo" != "xno"; then
	  AC_CHECK_HEADERS(librttopo.h,, [AC_MSG_ERROR([cannot find librttopo.h, bailing out])])
	  AC_SEARCH_LIBS(rtt_AddLineNoFace,rttopo,,AC_MSG_ERROR(['librttopo' (>= v.1.1.0) is required but it doesn't seem to be installed on this system. You may need to try re-running configure with a --disable-rttopo parameter.]))
	  AC_DEFINE(ENABLE_RTTOPO)
  fi
else
  AC_DEFINE(OMIT_GEOS)
fi

#-----------------------------------------------------------------------
#   --enable-libxml2
#
AC_ARG_ENABLE(libxml2, [AS_HELP_STRING(
  [--enable-libxml2], [enables libxml2 inclusion [default=yes]])],
  [], [enable_libxml2=yes])
if test x"$enable_libxml2" != "xno"; then
  PKG_CHECK_MODULES([LIBXML2], [libxml-2.0], , AC_MSG_ERROR(['libxml2' is required but it doesn't seem to be installed on this system.]))
  AC_SUBST(LIBXML2_CFLAGS)
  AC_SUBST(LIBXML2_LIBS)
  AC_DEFINE(ENABLE_LIBXML2)
fi

#-----------------------------------------------------------------------
#   --enable-minizip
#
AC_ARG_ENABLE(minizip, [AS_HELP_STRING(
  [--enable-minizip], [enables MiniZIP inclusion [default=yes]])],
  [], [enable_minizip=yes])
if test x"$enable_minizip" != "xno"; then
  AC_CHECK_HEADERS(minizip/unzip.h,, [AC_MSG_ERROR([cannot find minizip/unzip.h, bailing out])])
  AC_SEARCH_LIBS(unzLocateFile, minizip,,AC_MSG_ERROR(['libminizip' is required but it doesn't seem to be installed on this system.]))
  AC_DEFINE(ENABLE_MINIZIP)
fi

#-----------------------------------------------------------------------
#   --enable-geopackage
#
AC_ARG_ENABLE(geopackage, [AS_HELP_STRING(
  [--enable-geopackage], [enables GeoPackage support [default=yes]])],
  [], [enable_geopackage=yes])
if test x"$enable_geopackage" != "xno"; then
  AC_DEFINE(ENABLE_GEOPACKAGE)
fi
AM_CONDITIONAL([ENABLE_GEOPACKAGE], [test x"$enable_geopackage" = "xyes"])

#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
#    --enable-gcov
#
AC_ARG_ENABLE(gcov, AS_HELP_STRING([--enable-gcov],[turn on code coverage analysis tools]))
if test "x$enable_gcov" = "xyes"; then
    dnl Check for -fprofile-arcs and -ftest-coverage option
    CFLAGS=$CFLAGS" -fprofile-arcs -ftest-coverage -g"
    GCOV_FLAGS="-lgcov"
fi
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
#   --enable-examples
#
AC_ARG_ENABLE(examples, [AS_HELP_STRING(
       [--enable-examples], [enables building examples [default=yes]])],
       [], [enable_examples=yes])
AM_CONDITIONAL(ENABLE_EXAMPLES, [test x"$enable_examples" != "xno"])
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
#   --enable-module-only
#
AC_ARG_ENABLE(module-only, [AS_HELP_STRING(
       [--enable-module-only], [builds only mod_spatialite alone [default=no]])],
       [], [enable_module_only=no])
AM_CONDITIONAL(MODULE_ONLY, [test x"$enable_module_only" != "xno"])
#-----------------------------------------------------------------------

# Checking for MinGW
AM_CONDITIONAL([MINGW], [test "$target_alias" = "mingw32"])
# Checking for Mac OsX
AM_CONDITIONAL([MACOSX], [test "$target_alias" = "macosx"])
# Checking for Android
AM_CONDITIONAL([ANDROID], [test "$target_alias" = "android"])

# testing for sqlite 3-10 or later
AC_CHECK_DECL([SQLITE_INDEX_CONSTRAINT_LIKE],
  [AC_DEFINE(HAVE_DECL_SQLITE_INDEX_CONSTRAINT_LIKE)],[],[[#include <sqlite3.h>]])

AC_OUTPUT

#-----------------------------------------------------------------------
# printing an eventual message reporting about GPL escalation
#-----------------------------------------------------------------------
gpl_escalation=no;
if test x"$enable_rttopo" != "xno"; then
    gpl_escalation=yes
fi
if test x"$enable_gcp" != "xno"; then
    gpl_escalation=yes
fi
if test x"$gpl_escalation" != xno; then
    echo 
    echo
    echo "=============================================================="
    echo "                        IMPORTANT NOTICE"
    echo "=============================================================="
    echo "You have selected --enable-rttopo and/or --enable-gcp"
    echo
    echo "Both modules strictly depend on code released under the GPLv2+"
    echo "license, wich takes precedence over any other license."
    echo "Consequently the copy of libspatialite you are going to build"
    echo "if configured this way *must* be released under the GPLv2+ license."
    echo
    echo "If you wish better preserving the initial MPL tri-license you"
    echo "simply have to reconfigure by specifying the following options:"
    echo "        --disable-rttopo --disable-gcp"
    echo "=============================================================="
  
fi
