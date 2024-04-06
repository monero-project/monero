# - Try to find libunwind
# Once done this will define
#
#  LIBUNWIND_FOUND - system has libunwind
#  LIBUNWIND_INCLUDE_DIR - the libunwind include directory
#  LIBUNWIND_LIBRARIES - Link these to use libunwind
#  LIBUNWIND_DEFINITIONS - Compiler switches required for using libunwind

# Copyright (c) 2006, Alexander Dymo, <adymo@kdevelop.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

find_path(LIBUNWIND_INCLUDE_DIR libunwind.h
  /usr/include
  /usr/local/include
)

find_library(LIBUNWIND_LIBRARIES NAMES unwind )
if(NOT LIBUNWIND_LIBRARIES STREQUAL "LIBUNWIND_LIBRARIES-NOTFOUND")
  if (CMAKE_COMPILER_IS_GNUCC)
    set(LIBUNWIND_LIBRARIES "gcc_eh;${LIBUNWIND_LIBRARIES}")
  endif()
endif()

# some versions of libunwind need liblzma, and we don't use pkg-config
# so we just look whether liblzma is installed, and add it if it is.
# It might not be actually needed, but doesn't hurt if it is not.
# We don't need any headers, just the lib, as it's privately needed.
message(STATUS "looking for liblzma")
find_library(LIBLZMA_LIBRARIES lzma )
if(NOT LIBLZMA_LIBRARIES STREQUAL "LIBLZMA_LIBRARIES-NOTFOUND")
  message(STATUS "liblzma found")
  set(LIBUNWIND_LIBRARIES "${LIBUNWIND_LIBRARIES};${LIBLZMA_LIBRARIES}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libunwind "Could not find libunwind" LIBUNWIND_INCLUDE_DIR LIBUNWIND_LIBRARIES)
# show the LIBUNWIND_INCLUDE_DIR and LIBUNWIND_LIBRARIES variables only in the advanced view
mark_as_advanced(LIBUNWIND_INCLUDE_DIR LIBUNWIND_LIBRARIES )

