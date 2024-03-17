# Function for setting default options default values via env vars
function(_trezor_default_val val_name val_default)
    if(NOT DEFINED ENV{${val_name}})
        set(ENV{${val_name}} ${val_default})
    endif()
endfunction()

# Define default options via env vars
_trezor_default_val(USE_DEVICE_TREZOR ON)
_trezor_default_val(USE_DEVICE_TREZOR_MANDATORY OFF)
_trezor_default_val(USE_DEVICE_TREZOR_PROTOBUF_TEST ON)
_trezor_default_val(USE_DEVICE_TREZOR_LIBUSB ON)
_trezor_default_val(USE_DEVICE_TREZOR_UDP_RELEASE OFF)
_trezor_default_val(USE_DEVICE_TREZOR_DEBUG OFF)
_trezor_default_val(TREZOR_DEBUG OFF)

# Main options
OPTION(USE_DEVICE_TREZOR "Trezor support compilation" $ENV{USE_DEVICE_TREZOR})
OPTION(USE_DEVICE_TREZOR_MANDATORY "Trezor compilation is mandatory, fail build if Trezor support cannot be compiled" $ENV{USE_DEVICE_TREZOR_MANDATORY})
OPTION(USE_DEVICE_TREZOR_PROTOBUF_TEST "Trezor Protobuf test" $ENV{USE_DEVICE_TREZOR_PROTOBUF_TEST})
OPTION(USE_DEVICE_TREZOR_LIBUSB "Trezor LibUSB compilation" $ENV{USE_DEVICE_TREZOR_LIBUSB})
OPTION(USE_DEVICE_TREZOR_UDP_RELEASE "Trezor UdpTransport in release mode" $ENV{USE_DEVICE_TREZOR_UDP_RELEASE})
OPTION(USE_DEVICE_TREZOR_DEBUG "Trezor Debugging enabled" $ENV{USE_DEVICE_TREZOR_DEBUG})
OPTION(TREZOR_DEBUG "Main Trezor debugging switch" $ENV{TREZOR_DEBUG})

# Helper function to fix cmake < 3.6.0 FindProtobuf variables
function(_trezor_protobuf_fix_vars)
    if(${CMAKE_VERSION} VERSION_LESS "3.6.0")
        foreach(UPPER
                PROTOBUF_SRC_ROOT_FOLDER
                PROTOBUF_IMPORT_DIRS
                PROTOBUF_DEBUG
                PROTOBUF_LIBRARY
                PROTOBUF_PROTOC_LIBRARY
                PROTOBUF_INCLUDE_DIR
                PROTOBUF_PROTOC_EXECUTABLE
                PROTOBUF_LIBRARY_DEBUG
                PROTOBUF_PROTOC_LIBRARY_DEBUG
                PROTOBUF_LITE_LIBRARY
                PROTOBUF_LITE_LIBRARY_DEBUG
                )
            if (DEFINED ${UPPER})
                string(REPLACE "PROTOBUF_" "Protobuf_" Camel ${UPPER})
                if (NOT DEFINED ${Camel})
                    set(${Camel} ${${UPPER}} PARENT_SCOPE)
                endif()
            endif()
        endforeach()
    endif()
endfunction()

macro(trezor_fatal_msg msg)
    if ($ENV{USE_DEVICE_TREZOR_MANDATORY})
        message(FATAL_ERROR
                "${msg}\n"
                "==========================================================================\n"
                "[ERROR] To compile without Trezor support, set USE_DEVICE_TREZOR=OFF. "
                "It is possible both via cmake variable and environment variable, e.g., "
                "`USE_DEVICE_TREZOR=OFF make release`\n"
                "For more information, please check src/device_trezor/README.md\n"
        )
    else()
        message(WARNING
                "${msg}\n"
                "==========================================================================\n"
                "[WARNING] Trezor support cannot be compiled! Skipping Trezor compilation. \n"
                "For more information, please check src/device_trezor/README.md\n")
        set(USE_DEVICE_TREZOR OFF)
        return()  # finish this cmake file processing (as this is macro).
    endif()
endmacro()

# Use Trezor master switch
if (USE_DEVICE_TREZOR)
    # Protobuf is required to build protobuf messages for Trezor
    include(FindProtobuf OPTIONAL)

    # PkgConfig works better with new Protobuf
    find_package(PkgConfig QUIET)
    pkg_check_modules(PROTOBUF protobuf)

    if (NOT Protobuf_FOUND)
        FIND_PACKAGE(Protobuf CONFIG)
    endif()
    if (NOT Protobuf_FOUND)
        FIND_PACKAGE(Protobuf)
    endif()

    _trezor_protobuf_fix_vars()

    # Early fail for optional Trezor support
    if(NOT Protobuf_FOUND AND NOT Protobuf_LIBRARY AND NOT Protobuf_PROTOC_EXECUTABLE AND NOT Protobuf_INCLUDE_DIR)
        trezor_fatal_msg("Trezor: Could not find Protobuf")
    elseif(${CMAKE_CXX_STANDARD} LESS 17 AND ${Protobuf_VERSION} GREATER 21)
        trezor_fatal_msg("Trezor: Unsupported Protobuf version ${Protobuf_VERSION} with C++ ${CMAKE_CXX_STANDARD}. Please, use Protobuf v21.")
    elseif(NOT Protobuf_LIBRARY)
        trezor_fatal_msg("Trezor: Protobuf library not found: ${Protobuf_LIBRARY}")
        unset(Protobuf_FOUND)
    elseif(NOT Protobuf_PROTOC_EXECUTABLE OR NOT EXISTS "${Protobuf_PROTOC_EXECUTABLE}")
        trezor_fatal_msg("Trezor: Protobuf executable not found: ${Protobuf_PROTOC_EXECUTABLE}")
        unset(Protobuf_FOUND)
    elseif(NOT Protobuf_INCLUDE_DIR OR NOT EXISTS "${Protobuf_INCLUDE_DIR}")
        trezor_fatal_msg("Trezor: Protobuf include dir not found: ${Protobuf_INCLUDE_DIR}")
        unset(Protobuf_FOUND)
    else()
        message(STATUS "Trezor: Protobuf lib: ${Protobuf_LIBRARY}, inc: ${Protobuf_INCLUDE_DIR}, protoc: ${Protobuf_PROTOC_EXECUTABLE}")
        set(Protobuf_INCLUDE_DIRS ${Protobuf_INCLUDE_DIR})
        set(Protobuf_FOUND 1)  # override found if all required info was provided by variables
    endif()

    if(TREZOR_DEBUG)
        set(USE_DEVICE_TREZOR_DEBUG 1)
        message(STATUS "Trezor: debug build enabled")
    endif()

    # Compile debugging support (for tests)
    if (USE_DEVICE_TREZOR_DEBUG)
        add_definitions(-DWITH_TREZOR_DEBUGGING=1)
    endif()
else()
    message(STATUS "Trezor: support disabled by USE_DEVICE_TREZOR")
endif()

if(Protobuf_FOUND AND USE_DEVICE_TREZOR)
    if (NOT "$ENV{TREZOR_PYTHON}" STREQUAL "")
        set(TREZOR_PYTHON "$ENV{TREZOR_PYTHON}" CACHE INTERNAL "Copied from environment variable TREZOR_PYTHON")
    else()
        find_package(Python QUIET COMPONENTS Interpreter)  # cmake 3.12+
        if(Python_Interpreter_FOUND)
            set(TREZOR_PYTHON "${Python_EXECUTABLE}")
        endif()
    endif()

    if(NOT TREZOR_PYTHON)
        find_package(PythonInterp)
        if(PYTHONINTERP_FOUND AND PYTHON_EXECUTABLE)
            set(TREZOR_PYTHON "${PYTHON_EXECUTABLE}")
        endif()
    endif()

    if(NOT TREZOR_PYTHON)
        trezor_fatal_msg("Trezor: Python not found")
    endif()
endif()

# Protobuf compilation test
if(Protobuf_FOUND AND USE_DEVICE_TREZOR AND TREZOR_PYTHON)
    execute_process(COMMAND ${Protobuf_PROTOC_EXECUTABLE} -I "${CMAKE_CURRENT_LIST_DIR}" -I "${Protobuf_INCLUDE_DIR}" "${CMAKE_CURRENT_LIST_DIR}/test-protobuf.proto" --cpp_out ${CMAKE_BINARY_DIR} RESULT_VARIABLE RET OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
    if(RET)
        trezor_fatal_msg("Trezor: Protobuf test generation failed: ${OUT} ${ERR}")
    endif()

    if(ANDROID)
        set(CMAKE_TRY_COMPILE_LINKER_FLAGS "${CMAKE_TRY_COMPILE_LINKER_FLAGS} -llog")
        set(CMAKE_TRY_COMPILE_LINK_LIBRARIES "${CMAKE_TRY_COMPILE_LINK_LIBRARIES} log")
    endif()

    if(USE_DEVICE_TREZOR_PROTOBUF_TEST)
        if(PROTOBUF_LDFLAGS)
            set(PROTOBUF_TRYCOMPILE_LINKER "${PROTOBUF_LDFLAGS}")
        else()
            set(PROTOBUF_TRYCOMPILE_LINKER "${Protobuf_LIBRARY}")
        endif()
        
        try_compile(Protobuf_COMPILE_TEST_PASSED
            "${CMAKE_BINARY_DIR}"
            SOURCES
            "${CMAKE_BINARY_DIR}/test-protobuf.pb.cc"
            "${CMAKE_CURRENT_LIST_DIR}/test-protobuf.cpp"
            CMAKE_FLAGS
            CMAKE_EXE_LINKER_FLAGS ${CMAKE_TRY_COMPILE_LINKER_FLAGS}
            "-DINCLUDE_DIRECTORIES=${Protobuf_INCLUDE_DIR};${CMAKE_BINARY_DIR}"
            "-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}"
            LINK_LIBRARIES "${PROTOBUF_TRYCOMPILE_LINKER}" ${CMAKE_TRY_COMPILE_LINK_LIBRARIES}
            OUTPUT_VARIABLE OUTPUT
        )
        if(NOT Protobuf_COMPILE_TEST_PASSED)
            trezor_fatal_msg("Trezor: Protobuf Compilation test failed: ${OUTPUT}.")
        endif()
    else ()
        message(STATUS "Trezor: Protobuf Compilation test skipped, build may fail later")
    endif()
endif()

# Try to build protobuf messages
if(Protobuf_FOUND AND USE_DEVICE_TREZOR AND TREZOR_PYTHON)
    set(ENV{PROTOBUF_INCLUDE_DIRS} "${Protobuf_INCLUDE_DIR}")
    set(ENV{PROTOBUF_PROTOC_EXECUTABLE} "${Protobuf_PROTOC_EXECUTABLE}")
    set(TREZOR_PROTOBUF_PARAMS "")
    if (USE_DEVICE_TREZOR_DEBUG)
        set(TREZOR_PROTOBUF_PARAMS "--debug")
    endif()
    
    execute_process(COMMAND ${TREZOR_PYTHON} tools/build_protob.py ${TREZOR_PROTOBUF_PARAMS} WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../src/device_trezor/trezor RESULT_VARIABLE RET OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
    if(RET)
        trezor_fatal_msg("Trezor: protobuf messages could not be regenerated (err=${RET}, python ${PYTHON})."
                "OUT: ${OUT}, ERR: ${ERR}."
                "Please read src/device_trezor/trezor/tools/README.md")
    endif()

    message(STATUS "Trezor: protobuf messages regenerated out: \"${OUT}.\"")
    set(DEVICE_TREZOR_READY 1)
    add_definitions(-DDEVICE_TREZOR_READY=1)
    add_definitions(-DPROTOBUF_INLINE_NOT_IN_HEADERS=0)

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_definitions(-DTREZOR_DEBUG=1)
    endif()

    if(USE_DEVICE_TREZOR_UDP_RELEASE)
        message(STATUS "Trezor: UDP transport enabled (emulator)")
        add_definitions(-DUSE_DEVICE_TREZOR_UDP_RELEASE=1)
    endif()

    if (Protobuf_INCLUDE_DIR)
        include_directories(${Protobuf_INCLUDE_DIR})
    endif()

    # LibUSB support, check for particular version
    # Include support only if compilation test passes
    if (USE_DEVICE_TREZOR_LIBUSB)
        find_package(LibUSB)
    endif()

    if (LibUSB_COMPILE_TEST_PASSED)
        add_definitions(-DHAVE_TREZOR_LIBUSB=1)
        if(LibUSB_INCLUDE_DIRS)
            include_directories(${LibUSB_INCLUDE_DIRS})
        endif()
    endif()

    set(TREZOR_LIBUSB_LIBRARIES "")
    if(LibUSB_COMPILE_TEST_PASSED)
        list(APPEND TREZOR_LIBUSB_LIBRARIES ${LibUSB_LIBRARIES} ${LIBUSB_DEP_LINKER})
        message(STATUS "Trezor: compatible LibUSB found at: ${LibUSB_INCLUDE_DIRS}")
    elseif(USE_DEVICE_TREZOR_LIBUSB AND NOT ANDROID)
        trezor_fatal_msg("Trezor: LibUSB not found or test failed, please install libusb-1.0.26")
    endif()

    if (BUILD_GUI_DEPS)
        set(TREZOR_DEP_LIBS "")
        set(TREZOR_DEP_LINKER "")

        if (Protobuf_LIBRARY)
            list(APPEND TREZOR_DEP_LIBS ${Protobuf_LIBRARY})
            string(APPEND TREZOR_DEP_LINKER " -lprotobuf")
        endif()

        if (TREZOR_LIBUSB_LIBRARIES)
            list(APPEND TREZOR_DEP_LIBS ${TREZOR_LIBUSB_LIBRARIES})
            string(APPEND TREZOR_DEP_LINKER " -lusb-1.0 ${LIBUSB_DEP_LINKER}")
        endif()
    endif()
endif()
