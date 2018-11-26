# - Find libusb for portable USB support
# This module will find libusb as published by
#  http://libusb.sf.net and
#  http://libusb-win32.sf.net
#
# It will use PkgConfig if present and supported, else search
# it on its own. If the LibUSB_ROOT_DIR environment variable
# is defined, it will be used as base path.
# The following standard variables get defined:
#  LibUSB_FOUND:        true if LibUSB was found
#  LibUSB_HEADER_FILE:  the location of the C header file
#  LibUSB_INCLUDE_DIRS: the directory that contains the include file
#  LibUSB_LIBRARIES:    the library
# source: https://github.com/IntelRealSense/librealsense

include ( CheckLibraryExists )
include ( CheckIncludeFile )

find_package ( PkgConfig )
if ( PKG_CONFIG_FOUND )
    pkg_check_modules ( PKGCONFIG_LIBUSB libusb-1.0 )
    if ( NOT PKGCONFIG_LIBUSB_FOUND )
        pkg_check_modules ( PKGCONFIG_LIBUSB libusb )
    endif ( NOT PKGCONFIG_LIBUSB_FOUND )
endif ( PKG_CONFIG_FOUND )

if ( PKGCONFIG_LIBUSB_FOUND )
    set ( LibUSB_INCLUDE_DIRS ${PKGCONFIG_LIBUSB_INCLUDE_DIRS} )
    foreach ( i ${PKGCONFIG_LIBUSB_LIBRARIES} )
        string ( REGEX MATCH "[^-]*" ibase "${i}" )
        find_library ( ${ibase}_LIBRARY
                NAMES ${i}
                PATHS ${PKGCONFIG_LIBUSB_LIBRARY_DIRS}
                )
        if ( ${ibase}_LIBRARY )
            list ( APPEND LibUSB_LIBRARIES ${${ibase}_LIBRARY} )
        endif ( ${ibase}_LIBRARY )
        mark_as_advanced ( ${ibase}_LIBRARY )
    endforeach ( i )

else ( PKGCONFIG_LIBUSB_FOUND )
    find_file ( LibUSB_HEADER_FILE
            NAMES
            libusb.h usb.h
            PATHS
            $ENV{ProgramFiles}/LibUSB-Win32
            $ENV{LibUSB_ROOT_DIR}
            PATH_SUFFIXES
            include
            libusb-1.0
            include/libusb-1.0
            )
    mark_as_advanced ( LibUSB_HEADER_FILE )
    get_filename_component ( LibUSB_INCLUDE_DIRS "${LibUSB_HEADER_FILE}" PATH )

    if ( ${CMAKE_SYSTEM_NAME} STREQUAL "Windows" )
        # LibUSB-Win32 binary distribution contains several libs.
        # Use the lib that got compiled with the same compiler.
        if ( MSVC )
            if ( WIN32 )
                set ( LibUSB_LIBRARY_PATH_SUFFIX lib/msvc )
            else ( WIN32 )
                set ( LibUSB_LIBRARY_PATH_SUFFIX lib/msvc_x64 )
            endif ( WIN32 )
        elseif ( BORLAND )
            set ( LibUSB_LIBRARY_PATH_SUFFIX lib/bcc )
        elseif ( CMAKE_COMPILER_IS_GNUCC )
            set ( LibUSB_LIBRARY_PATH_SUFFIX lib/gcc )
        endif ( MSVC )
    endif ( ${CMAKE_SYSTEM_NAME} STREQUAL "Windows" )

    find_library ( usb_LIBRARY
            NAMES
            usb-1.0 libusb usb
            PATHS
            $ENV{ProgramFiles}/LibUSB-Win32
            $ENV{LibUSB_ROOT_DIR}
            PATH_SUFFIXES
            ${LibUSB_LIBRARY_PATH_SUFFIX}
            )
    mark_as_advanced ( usb_LIBRARY )
    if ( usb_LIBRARY )
        set ( LibUSB_LIBRARIES ${usb_LIBRARY} )
    endif ( usb_LIBRARY )

endif ( PKGCONFIG_LIBUSB_FOUND )

if ( LibUSB_INCLUDE_DIRS AND LibUSB_LIBRARIES )
    set ( LibUSB_FOUND true )
endif ( LibUSB_INCLUDE_DIRS AND LibUSB_LIBRARIES )

if ( LibUSB_FOUND )
    set ( CMAKE_REQUIRED_INCLUDES "${LibUSB_INCLUDE_DIRS}" )
    check_include_file ( "${LibUSB_HEADER_FILE}" LibUSB_FOUND )
endif ( LibUSB_FOUND )

if ( LibUSB_FOUND )
    check_library_exists ( "${LibUSB_LIBRARIES}" usb_open "" LibUSB_FOUND )
    check_library_exists ( "${LibUSB_LIBRARIES}" libusb_get_device_list "" LibUSB_VERSION_1.0 )
    check_library_exists ( "${LibUSB_LIBRARIES}" libusb_get_port_numbers "" LibUSB_VERSION_1.0.16 )

    # Library 1.0.16+ compilation test.
    # The check_library_exists does not work well on Apple with shared libs.
    if (APPLE OR LibUSB_VERSION_1.0.16)
        if (APPLE)
            if(DEPENDS)
                list(APPEND TEST_COMPILE_EXTRA_LIBRARIES "-framework Foundation -framework IOKit")
            else()
                find_library(COREFOUNDATION CoreFoundation)
                find_library(IOKIT IOKit)
                list(APPEND TEST_COMPILE_EXTRA_LIBRARIES ${IOKIT})
                list(APPEND TEST_COMPILE_EXTRA_LIBRARIES ${COREFOUNDATION})
            endif()
        endif()
        if (WIN32)
            list(APPEND TEST_COMPILE_EXTRA_LIBRARIES setupapi)
        endif()
        list(APPEND TEST_COMPILE_EXTRA_LIBRARIES ${LibUSB_LIBRARIES})

        try_compile(LibUSB_COMPILE_TEST_PASSED
                ${CMAKE_BINARY_DIR}
                "${CMAKE_SOURCE_DIR}/cmake/test-libusb-version.c"
                CMAKE_FLAGS
                    "-DINCLUDE_DIRECTORIES=${LibUSB_INCLUDE_DIRS}"
                    "-DLINK_DIRECTORIES=${LibUSB_LIBRARIES}"
                LINK_LIBRARIES ${TEST_COMPILE_EXTRA_LIBRARIES}
                OUTPUT_VARIABLE OUTPUT)
        unset(TEST_COMPILE_EXTRA_LIBRARIES)
        message(STATUS "LibUSB Compilation test: ${LibUSB_COMPILE_TEST_PASSED}")
    endif()
endif ( LibUSB_FOUND )

if ( NOT LibUSB_FOUND )
    if ( NOT LibUSB_FIND_QUIETLY )
        message ( STATUS "LibUSB not found, try setting LibUSB_ROOT_DIR environment variable." )
    endif ( NOT LibUSB_FIND_QUIETLY )
    if ( LibUSB_FIND_REQUIRED )
        message ( FATAL_ERROR "" )
    endif ( LibUSB_FIND_REQUIRED )
endif ( NOT LibUSB_FOUND )
