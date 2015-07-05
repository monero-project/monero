# - Try to find Berkeley DB
# Once done this will define
#
#  BERKELEY_DB_FOUND - system has Berkeley DB
#  BERKELEY_DB_INCLUDE_DIR - the Berkeley DB include directory
#  BERKELEY_DB_LIBRARIES - Link these to use Berkeley DB
#  BERKELEY_DB_DEFINITIONS - Compiler switches required for using Berkeley DB

# Copyright (c) 2006, Alexander Dymo, <adymo@kdevelop.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

find_path(BERKELEY_DB_INCLUDE_DIR db_cxx.h
  /usr/include/db4
  /usr/local/include/db4
)

find_library(BERKELEY_DB_LIBRARIES NAMES db_cxx )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Berkeley "Could not find Berkeley DB >= 4.1" BERKELEY_DB_INCLUDE_DIR BERKELEY_DB_LIBRARIES)
# show the BERKELEY_DB_INCLUDE_DIR and BERKELEY_DB_LIBRARIES variables only in the advanced view
mark_as_advanced(BERKELEY_DB_INCLUDE_DIR BERKELEY_DB_LIBRARIES )

