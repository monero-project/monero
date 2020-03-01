
cmake_minimum_required(VERSION 2.8)

set(Boost_USE_MULTITHREADED ON)

include_directories(.)
include_directories(../../include)

find_package(Boost COMPONENTS system filesystem thread date_time chrono regex)
include_directories( ${Boost_INCLUDE_DIRS} )

IF (MSVC)
	add_definitions( "/W3 /D_CRT_SECURE_NO_WARNINGS /wd4996 /wd4345  /nologo /D_WIN32_WINNT=0x0600 /DWIN32_LEAN_AND_MEAN /bigobj" )
	include_directories(SYSTEM platform/msvc)
ELSE()
	# set stuff for other systems
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wall")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wno-reorder")
ENDIF()


# Add folders to filters
file(GLOB_RECURSE SRC        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
			     ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
			     ${CMAKE_CURRENT_SOURCE_DIR}/*.inl
			     ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

source_group(general  FILES ${SRC})


add_executable(tests ${SRC} )
target_link_libraries( tests ${Boost_LIBRARIES} )

