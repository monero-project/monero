cmake_minimum_required (VERSION 2.6)
project (otshell CXX)

# Add executable

if(APPLE AND POLICY CMP0042)
	cmake_policy(SET CMP0042 NEW)
endif()

file(GLOB otshell_utils_sources # All files in directory:
	"*.h"
	"*.hpp"
	"*.cpp"
)

add_library (otshell_utils ${otshell_utils_sources})
set_target_properties (otshell_utils PROPERTIES OUTPUT_NAME "otshell_utils")
#target_link_libraries (upnpc-static ${LDLIBS}) # to add used libs
