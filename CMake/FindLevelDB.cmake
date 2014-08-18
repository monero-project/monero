
# Look for the header file.
find_path(LEVELDB_INCLUDE NAMES leveldb/db.h PATHS $ENV{LEVELDB_ROOT}/include /opt/local/include /usr/local/include /usr/include DOC "Path in which the file leveldb/db.h is located." )
mark_as_advanced(LEVELDB_INCLUDE)

# Look for the library.
find_library(LEVELDB_LIBS NAMES leveldb PATHS /usr/lib $ENV{LEVELDB_ROOT}/lib DOC "Path to leveldb library." )
mark_as_advanced(LEVELDB_LIBS)

# Copy the results to the output variables.
if (LEVELDB_INCLUDE AND LEVELDB_LIBS)
  message(STATUS "Found leveldb in ${LEVELDB_INCLUDE} ${LEVELDB_LIBS}")
  set(LEVELDB_FOUND 1)
  include(CheckCXXSourceCompiles)
  set(CMAKE_REQUIRED_LIBRARY ${LEVELDB_LIBS} pthread)
  set(CMAKE_REQUIRED_INCLUDES ${LEVELDB_INCLUDE})
 else ()
   set(LEVELDB_FOUND 0)
 endif ()

 # Report the results.
 if (NOT LEVELDB_FOUND)
   set(LEVELDB_DIR_MESSAGE "LEVELDB was not found. Make sure LEVELDB_LIBS and LEVELDB_INCLUDE are set.")
   if (LEVELDB_FIND_REQUIRED)
     message(FATAL_ERROR "${LEVELDB_DIR_MESSAGE}")
   elseif (NOT LEVELDB_FIND_QUIETLY)
     message(STATUS "${LEVELDB_DIR_MESSAGE}")
   endif ()
 endif ()
