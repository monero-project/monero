# Locate miniupnp library
# This module defines
#  MINIUPNP_FOUND, if false, do not try to link to miniupnp
#  MINIUPNP_LIBRARY, the miniupnp variant
#  MINIUPNP_INCLUDE_DIR, where to find miniupnpc.h and family)
#  MINIUPNPC_VERSION_1_7_OR_HIGHER, set if we detect the version of miniupnpc is 1.7 or higher
#

# Sekreta/Kovri provides miniupnpc from the same upstream source as Monero, and uses its own targets.
if (NOT BUILD_KOVRI)
  find_path(MINIUPNP_INCLUDE_DIR miniupnpc.h
    HINTS $ENV{MINIUPNP_INCLUDE_DIR}
    PATH_SUFFIXES miniupnpc)

  set(MINIUPNP_SHORT_LIB libminiupnpc.a)
  if (NOT STATIC)
    set(MINIUPNP_SHORT_LIB miniupnpc)
  endif()

  find_library(MINIUPNP_LIBRARY ${MINIUPNP_SHORT_LIB}
    HINTS $ENV{MINIUPNP_LIBRARY})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(
    MiniUPnPc DEFAULT_MSG
    MINIUPNP_INCLUDE_DIR
    MINIUPNP_LIBRARY)

  # TODO(unassigned): outdated, higher requisite to match API changes
  if (MINIUPNPC_FOUND)
    file(STRINGS "${MINIUPNP_INCLUDE_DIR}/miniupnpc.h" MINIUPNPC_API_VERSION_STR REGEX "^#define[\t ]+MINIUPNPC_API_VERSION[\t ]+[0-9]+")
    if (MINIUPNPC_API_VERSION_STR MATCHES "^#define[\t ]+MINIUPNPC_API_VERSION[\t ]+([0-9]+)")
      set(MINIUPNPC_API_VERSION "${CMAKE_MATCH_1}")
      if (${MINIUPNPC_API_VERSION} GREATER "10" OR ${MINIUPNPC_API_VERSION} EQUAL "10")
        message(STATUS "Found miniupnpc API version " ${MINIUPNPC_API_VERSION})
        set(MINIUPNP_FOUND true)
        set(MINIUPNPC_VERSION_1_7_OR_HIGHER true)
      endif()
    endif()
  endif()
  mark_as_advanced(MINIUPNP_INCLUDE_DIR MINIUPNP_LIBRARY)
endif()
