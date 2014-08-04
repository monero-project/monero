cmake_minimum_required (VERSION 2.6)
project (otshell CXX)

# Options

option(WITH_TERMCOLORS "Build with support for unix terminal console colors VT100" ON)

if (WITH_TERMCOLORS)
	add_definitions( -DCFG_WITH_TERMCOLORS )
endif ()

# Add executable

file(GLOB otshell_utils_sources # All files in directory:
	"*.h"
	"*.hpp"
	"*.cpp"
)

add_library (otshell_utils STATIC ${otshell_utils_sources})
set_target_properties (otshell_utils PROPERTIES OUTPUT_NAME "otshell_utils")
#target_link_libraries (upnpc-static ${LDLIBS}) # to add used libs
