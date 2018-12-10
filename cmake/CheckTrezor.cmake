OPTION(USE_DEVICE_TREZOR "Trezor support compilation" ON)
OPTION(USE_DEVICE_TREZOR_LIBUSB "Trezor LibUSB compilation" ON)
OPTION(USE_DEVICE_TREZOR_UDP_RELEASE "Trezor UdpTransport in release mode" OFF)

# Use Trezor master switch
if (USE_DEVICE_TREZOR)
    # Protobuf is required to build protobuf messages for Trezor
    include(FindProtobuf OPTIONAL)
    find_package(Protobuf)
    if(NOT Protobuf_FOUND)
        message(STATUS "Could not find Protobuf")
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

# Try to build protobuf messages
if(Protobuf_FOUND AND USE_DEVICE_TREZOR AND TREZOR_PYTHON)
    set(ENV{PROTOBUF_INCLUDE_DIRS} "${Protobuf_INCLUDE_DIRS}")
    set(ENV{PROTOBUF_PROTOC_EXECUTABLE} "${Protobuf_PROTOC_EXECUTABLE}")
    execute_process(COMMAND ${TREZOR_PYTHON} tools/build_protob.py WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../src/device_trezor/trezor RESULT_VARIABLE RET OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
    if(RET)
        message(WARNING "Trezor protobuf messages could not be regenerated (err=${RET}, python ${PYTHON})."
                "OUT: ${OUT}, ERR: ${ERR}."
                "Please read src/device_trezor/trezor/tools/README.md")
    else()
        message(STATUS "Trezor protobuf messages regenerated ${OUT}")
        set(DEVICE_TREZOR_READY 1)
        add_definitions(-DDEVICE_TREZOR_READY=1)

        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            add_definitions(-DTREZOR_DEBUG=1)
        endif()

        if(USE_DEVICE_TREZOR_UDP_RELEASE)
            add_definitions(-DWITH_DEVICE_TREZOR_UDP_RELEASE=1)
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
    endif()
endif()
