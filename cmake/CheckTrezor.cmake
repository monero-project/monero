OPTION(USE_DEVICE_TREZOR "Trezor support compilation" ON)
OPTION(USE_DEVICE_TREZOR_LIBUSB "Trezor LibUSB compilation" ON)
OPTION(USE_DEVICE_TREZOR_UDP_RELEASE "Trezor UdpTransport in release mode" OFF)
OPTION(USE_DEVICE_TREZOR_DEBUG "Trezor Debugging enabled" OFF)
OPTION(TREZOR_DEBUG "Main trezor debugging switch" OFF)

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

# Use Trezor master switch
if (USE_DEVICE_TREZOR)
    # Protobuf is required to build protobuf messages for Trezor
    include(FindProtobuf OPTIONAL)
    find_package(Protobuf)
    _trezor_protobuf_fix_vars()

    # Protobuf handling the cache variables set in docker.
    if(NOT Protobuf_FOUND AND NOT Protobuf_LIBRARY AND NOT Protobuf_PROTOC_EXECUTABLE AND NOT Protobuf_INCLUDE_DIR)
        message(STATUS "Could not find Protobuf")
    elseif(NOT Protobuf_LIBRARY OR NOT EXISTS "${Protobuf_LIBRARY}")
        message(STATUS "Protobuf library not found: ${Protobuf_LIBRARY}")
        unset(Protobuf_FOUND)
    elseif(NOT Protobuf_PROTOC_EXECUTABLE OR NOT EXISTS "${Protobuf_PROTOC_EXECUTABLE}")
        message(STATUS "Protobuf executable not found: ${Protobuf_PROTOC_EXECUTABLE}")
        unset(Protobuf_FOUND)
    elseif(NOT Protobuf_INCLUDE_DIR OR NOT EXISTS "${Protobuf_INCLUDE_DIR}")
        message(STATUS "Protobuf include dir not found: ${Protobuf_INCLUDE_DIR}")
        unset(Protobuf_FOUND)
    else()
        message(STATUS "Protobuf lib: ${Protobuf_LIBRARY}, inc: ${Protobuf_INCLUDE_DIR}, protoc: ${Protobuf_PROTOC_EXECUTABLE}")
        set(Protobuf_INCLUDE_DIRS ${Protobuf_INCLUDE_DIR})
        set(Protobuf_FOUND 1)  # override found if all rquired info was provided by variables
    endif()

    if(TREZOR_DEBUG)
        set(USE_DEVICE_TREZOR_DEBUG 1)
    endif()

    # Compile debugging support (for tests)
    if (USE_DEVICE_TREZOR_DEBUG)
        add_definitions(-DWITH_TREZOR_DEBUGGING=1)
    endif()
else()
    message(STATUS "Trezor support disabled by USE_DEVICE_TREZOR")
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
        message(STATUS "Trezor: Python not found")
    endif()
endif()

# Protobuf compilation test
if(Protobuf_FOUND AND USE_DEVICE_TREZOR AND TREZOR_PYTHON)
    execute_process(COMMAND ${Protobuf_PROTOC_EXECUTABLE} -I "${CMAKE_CURRENT_LIST_DIR}" -I "${Protobuf_INCLUDE_DIR}" "${CMAKE_CURRENT_LIST_DIR}/test-protobuf.proto" --cpp_out ${CMAKE_BINARY_DIR} RESULT_VARIABLE RET OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
    if(RET)
        message(STATUS "Protobuf test generation failed: ${OUT} ${ERR}")
    endif()

    try_compile(Protobuf_COMPILE_TEST_PASSED
        "${CMAKE_BINARY_DIR}"
        SOURCES
        "${CMAKE_BINARY_DIR}/test-protobuf.pb.cc"
        "${CMAKE_CURRENT_LIST_DIR}/test-protobuf.cpp"
        CMAKE_FLAGS
        "-DINCLUDE_DIRECTORIES=${Protobuf_INCLUDE_DIR};${CMAKE_BINARY_DIR}"
        "-DCMAKE_CXX_STANDARD=11"
        LINK_LIBRARIES ${Protobuf_LIBRARY}
        OUTPUT_VARIABLE OUTPUT
    )
    if(NOT Protobuf_COMPILE_TEST_PASSED)
        message(STATUS "Protobuf Compilation test failed: ${OUTPUT}.")
    endif()
endif()

# Try to build protobuf messages
if(Protobuf_FOUND AND USE_DEVICE_TREZOR AND TREZOR_PYTHON AND Protobuf_COMPILE_TEST_PASSED)
    set(ENV{PROTOBUF_INCLUDE_DIRS} "${Protobuf_INCLUDE_DIR}")
    set(ENV{PROTOBUF_PROTOC_EXECUTABLE} "${Protobuf_PROTOC_EXECUTABLE}")
    set(TREZOR_PROTOBUF_PARAMS "")
    if (USE_DEVICE_TREZOR_DEBUG)
        set(TREZOR_PROTOBUF_PARAMS "--debug")
    endif()
    
    execute_process(COMMAND ${TREZOR_PYTHON} tools/build_protob.py ${TREZOR_PROTOBUF_PARAMS} WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../src/device_trezor/trezor RESULT_VARIABLE RET OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
    if(RET)
        message(WARNING "Trezor protobuf messages could not be regenerated (err=${RET}, python ${PYTHON})."
                "OUT: ${OUT}, ERR: ${ERR}."
                "Please read src/device_trezor/trezor/tools/README.md")
    else()
        message(STATUS "Trezor protobuf messages regenerated out: \"${OUT}.\"")
        set(DEVICE_TREZOR_READY 1)
        add_definitions(-DDEVICE_TREZOR_READY=1)
        add_definitions(-DPROTOBUF_INLINE_NOT_IN_HEADERS=0)

        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            add_definitions(-DTREZOR_DEBUG=1)
        endif()

        if(USE_DEVICE_TREZOR_UDP_RELEASE)
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
            message(STATUS "Trezor compatible LibUSB found at: ${LibUSB_INCLUDE_DIRS}")
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
endif()
