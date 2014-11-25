cmake_minimum_required (VERSION 2.6)

project (miniupnpc C)
set (MINIUPNPC_VERSION 1.9)
set (MINIUPNPC_API_VERSION 10)

option (UPNPC_BUILD_STATIC "Build static library" TRUE)
option (UPNPC_BUILD_SHARED "Build shared library" TRUE)
option (NO_GETADDRINFO "Define NO_GETADDRINFO" FALSE)

mark_as_advanced (NO_GETADDRINFO)

if (NO_GETADDRINFO)
  add_definitions (-DNO_GETADDRINFO)
endif ()

if (NOT WIN32)
  add_definitions (-DMINIUPNPC_SET_SOCKET_TIMEOUT)
  add_definitions (-D_BSD_SOURCE -D_POSIX_C_SOURCE=201112)
else ()
  add_definitions (-D_WIN32_WINNT=0x0501) # XP or higher for getnameinfo and friends
endif ()

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  add_definitions (-D_DARWIN_C_SOURCE)
endif ()

configure_file (miniupnpcstrings.h.cmake ${CMAKE_BINARY_DIR}/miniupnpcstrings.h)
include_directories (${CMAKE_BINARY_DIR})

set (MINIUPNPC_SOURCES
  igd_desc_parse.c
  miniupnpc.c
  minixml.c
  minisoap.c
  miniwget.c
  upnpc.c
  upnpcommands.c
  upnpreplyparse.c
  upnperrors.c
  connecthostport.c
  portlistingparse.c
  receivedata.c
)

if (NOT WIN32)
  set (MINIUPNPC_SOURCES ${MINIUPNPC_SOURCES} minissdpc.c)
endif ()

if (WIN32)
  set_source_files_properties (${MINIUPNPC_SOURCES} PROPERTIES
                                                    COMPILE_DEFINITIONS MINIUPNP_STATICLIB
                                                    COMPILE_DEFINITIONS MINIUPNP_EXPORTS
  )
endif ()

if (WIN32)
#  find_library (WINSOCK2_LIBRARY NAMES ws2_32 WS2_32 Ws2_32)
#  find_library (IPHLPAPI_LIBRARY NAMES iphlpapi)
  set(WINSOCK2_LIBRARY ws2_32)
  set(IPHLPAPI_LIBRARY iphlpapi)
  set (LDLIBS ${WINSOCK2_LIBRARY} ${IPHLPAPI_LIBRARY} ${LDLIBS})
endif ()

if (NOT UPNPC_BUILD_STATIC AND NOT UPNPC_BUILD_SHARED)
  message (FATAL "Both shared and static libraries are disabled!")
endif ()

if (UPNPC_BUILD_STATIC)
  add_library (upnpc-static STATIC ${MINIUPNPC_SOURCES})
  set_target_properties (upnpc-static PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
  set_target_properties (upnpc-static PROPERTIES OUTPUT_NAME "miniupnpc")
  target_link_libraries (upnpc-static ${LDLIBS})
  set (UPNPC_INSTALL_TARGETS ${UPNPC_INSTALL_TARGETS} upnpc-static)
  set (UPNPC_LIBRARY_TARGET upnpc-static)
endif ()

if (UPNPC_BUILD_SHARED)
  add_library (upnpc-shared SHARED ${MINIUPNPC_SOURCES})
  set_target_properties (upnpc-shared PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
  set_target_properties (upnpc-shared PROPERTIES OUTPUT_NAME "miniupnpc")
  set_target_properties (upnpc-shared PROPERTIES VERSION ${MINIUPNPC_VERSION})
  set_target_properties (upnpc-shared PROPERTIES SOVERSION ${MINIUPNPC_API_VERSION})
  target_link_libraries (upnpc-shared ${LDLIBS})
  set (UPNPC_INSTALL_TARGETS ${UPNPC_INSTALL_TARGETS} upnpc-shared)
  set (UPNPC_LIBRARY_TARGET upnpc-shared)
endif ()

install (TARGETS ${UPNPC_INSTALL_TARGETS}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib${LIB_SUFFIX}
  ARCHIVE DESTINATION lib${LIB_SUFFIX}
)
install (FILES
  miniupnpc.h
  miniwget.h
  upnpcommands.h
  igd_desc_parse.h
  upnpreplyparse.h
  upnperrors.h
  miniupnpctypes.h
  portlistingparse.h
  declspec.h
  DESTINATION include/miniupnpc
)

# vim: ts=2:sw=2
