
cmake_minimum_required(VERSION 3.5)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(Boost_USE_MULTITHREADED ON)
set(NAME tests_epee)

include_directories(.)

IF (MSVC)
	add_definitions( "/W3 /D_CRT_SECURE_NO_WARNINGS /wd4996 /wd4345  /nologo /D_WIN32_WINNT=0x0600 /DWIN32_LEAN_AND_MEAN /bigobj" )
	include_directories(SYSTEM platform/msvc)
ELSE()
	# set stuff for other systems
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-reorder")
ENDIF()


set(SRC
	main.cpp
)

monero_find_all_headers(EPEE_TESTS_HEADERS_PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
monero_add_minimal_executable(${NAME} ${SRC} ${EPEE_TESTS_HEADERS_PUBLIC})
target_link_libraries(${NAME}
	epee
	${Boost_SYSTEM_LIBRARY}
	${CMAKE_THREAD_LIBS_INIT}
)

add_test(${NAME} ${NAME})

