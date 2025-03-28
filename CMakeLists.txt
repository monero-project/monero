# Copyright (c) 2014-2024, The Monero Project
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are
# permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this list of
#    conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this list
#    of conditions and the following disclaimer in the documentation and/or other
#    materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors may be
#    used to endorse or promote products derived from this software without specific
#    prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
# THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
# THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

cmake_minimum_required(VERSION 3.5)
message(STATUS "CMake version ${CMAKE_VERSION}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(INSERT CMAKE_MODULE_PATH 0
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckLinkerFlag)
include(CheckLibraryExists)
include(CheckFunctionExists)
if (POLICY CMP0148)
    cmake_policy(SET CMP0148 OLD) # https://cmake.org/cmake/help/latest/policy/CMP0148.html
endif()
include(FindPythonInterp)

if (IOS)
    INCLUDE(CmakeLists_IOS.txt)
endif()

project(monero)

option (USE_CCACHE "Use ccache if a usable instance is found" ON)
if (USE_CCACHE)
	include(FindCcache) # Has to be included after the project() macro, to be able to read the CXX variable.
else()
	message(STATUS "ccache deselected")
endif()
option (USE_COMPILATION_TIME_PROFILER "Use compilation time profiler (for CLang >= 9 only)" OFF)
if (USE_COMPILATION_TIME_PROFILER)
	if (NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
		message(FATAL_ERROR "The flag USE_COMPILATION_TIME_PROFILER is meant to be set only for CLang compiler!")
	endif()
	add_compile_options("-ftime-trace")
endif()

if (${CMAKE_VERSION} VERSION_GREATER "3.0.0" AND CMAKE_MAKE_PROGRAM MATCHES "ninja")
  set(MONERO_PARALLEL_COMPILE_JOBS "" CACHE STRING "The maximum number of concurrent compilation jobs.")
  if (MONERO_PARALLEL_COMPILE_JOBS)
    set_property(GLOBAL APPEND PROPERTY JOB_POOLS compile_job_pool=${MONERO_PARALLEL_COMPILE_JOBS})
    set(CMAKE_JOB_POOL_COMPILE compile_job_pool)
  endif ()

  set(MONERO_PARALLEL_LINK_JOBS "" CACHE STRING "The maximum number of concurrent link jobs.")
  if (MONERO_PARALLEL_LINK_JOBS)
    set_property(GLOBAL APPEND PROPERTY JOB_POOLS link_job_pool=${MONERO_PARALLEL_LINK_JOBS})
    set(CMAKE_JOB_POOL_LINK link_job_pool)
  endif ()
endif ()
  
option (USE_CLANG_TIDY_C   "Lint the code with clang-tidy - variant C"   OFF)
option (USE_CLANG_TIDY_CXX "Lint the code with clang-tidy - variant C++" OFF)
if (USE_CLANG_TIDY_C AND USE_CLANG_TIDY_CXX)
	message(FATAL_ERROR "Enabling both USE_CLANG_TIDY_C and USE_CLANG_TIDY_CXX simultaneously crashes clang-tidy.")
endif()
if (USE_CLANG_TIDY_C OR USE_CLANG_TIDY_CXX)
	include(SetClangTidy)
endif()
if (USE_CLANG_TIDY_C)
	monero_clang_tidy("C")
elseif (USE_CLANG_TIDY_CXX)
	monero_clang_tidy("CXX")
endif()

enable_language(C ASM)

# Require C11/C++11 and disable extensions for all targets
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function (die msg)
  if (NOT WIN32)
    string(ASCII 27 Esc)
    set(ColourReset "${Esc}[m")
    set(BoldRed     "${Esc}[1;31m")
  else ()
    set(ColourReset "")
    set(BoldRed     "")
  endif ()

  message(FATAL_ERROR "${BoldRed}${msg}${ColourReset}")
endfunction ()

function (add_c_flag_if_supported flag var)
  # Prepending the flag with -Werror will only add the flag,
  # if it doesn't result in generation of a warning of using a flag unknown to the compiler.
  set(TMP "-Werror ${flag}")
  string(REGEX REPLACE "[- ]" "_" supported ${TMP}_c)
  check_c_compiler_flag(${TMP} ${supported})
  if(${${supported}})
    set(${var} "${${var}} ${flag}" PARENT_SCOPE)
  endif()
endfunction()

function (add_cxx_flag_if_supported flag var)
  set(TMP "-Werror ${flag}")
  string(REGEX REPLACE "[- ]" "_" supported ${TMP}_cxx)
  check_cxx_compiler_flag(${TMP} ${supported})
  if(${${supported}})
    set(${var} "${${var}} ${flag}" PARENT_SCOPE)
  endif()
endfunction()

function (add_linker_flag_if_supported flag var)
  string(REPLACE "-" "_" supported ${flag}_ld)
  string(REPLACE "," "_" supported ${flag}_ld)
  check_linker_flag(${flag} ${supported})
  if(${${supported}})
    set(${var} "${${var}} ${flag}" PARENT_SCOPE)
  endif()
endfunction()

function (add_definition_if_function_found function var)
  string(REPLACE "-" "_" supported ${function}_function)
  check_function_exists(${function} ${supported})
  if(${${supported}})
    add_definitions("-D${var}")
  endif()
endfunction()

function (add_definition_if_library_exists library function header var)
  string(REPLACE "-" "_" supported ${function}_library)
  check_library_exists(${library} ${function} ${header} ${supported})
  if(${${supported}})
    add_definitions("-D${var}")
  endif()
endfunction()

option(RELINK_TARGETS "Relink targets, when just a dependant .so changed, but not its header?" OFF)
function (monero_set_target_no_relink target)
  if (RELINK_TARGETS MATCHES OFF)
    # Will not relink the target, when just its dependant .so has changed, but not it's interface
    set_target_properties("${target}" PROPERTIES LINK_DEPENDS_NO_SHARED true)
  endif()
endfunction()

option(STRIP_TARGETS "Strip symbols from targets?" OFF)
function (monero_set_target_strip target)
  if (STRIP_TARGETS)
    set_target_properties("${target}" PROPERTIES LINK_FLAGS_RELEASE -s)
    set_target_properties("${target}" PROPERTIES LINK_FLAGS_DEBUG -s)
    # Stripping from Debug might make sense if you're low on disk space, but want to test if debug version builds properly.
  endif()
endfunction()

function (monero_add_minimal_executable name)
  source_group("${name}"
    FILES
      ${ARGN})

  add_executable("${name}"
    ${ARGN})
    monero_set_target_no_relink("${name}")
    monero_set_target_strip    ("${name}")
endfunction()

# Finds all headers in a directory and its subdirs, to be able to search for them and autosave in IDEs.
#
# Parameters:
# - headers_found:    Output variable, which will hold the found headers
# - module_root_dir:  The search path for the headers. Typically it will be the module's root dir, so "${CMAKE_CURRENT_SOURCE_DIR}" or a derivative of it.
macro (monero_find_all_headers headers_found module_root_dir)
  file(GLOB ${headers_found}
           "${module_root_dir}/*.h*"    # h* will include hpps as well.
           "${module_root_dir}/**/*.h*" # Any number of subdirs will be included.
           "${module_root_dir}/*.inl"   # .inl is typically template code and is being treated as headers (it's being included).
           "${module_root_dir}/**/*.inl"
)
endmacro()

# Function to forbid undefined symbols and also verify
# 1) Test project with all types of libraries and without undefined symbols can compile successfully
# 2) Test project with all types of libraries and undefined symbols can not compile successfully
function(forbid_undefined_symbols)
  unset(TMP)
  # https://www.unix.com/man-page/linux/1/ld, --no-undefined, Report unresolved symbol references from regular object files.
  add_linker_flag_if_supported(-Wl,--no-undefined TMP)
  # https://www.unix.com/man-page/osx/1/ld/, -undefined, Specifies how undefined symbols are to be treated.
  add_linker_flag_if_supported(-Wl,-undefined,error TMP)
  string(APPEND CMAKE_SHARED_LINKER_FLAGS ${TMP})
  string(APPEND CMAKE_MODULE_LINKER_FLAGS ${TMP})
  set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} PARENT_SCOPE)
  set(CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS} PARENT_SCOPE)
  set(TEST_PROJECT "${CMAKE_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/CMakeTmp/test_project")
  foreach(EXPECT IN ITEMS TRUE FALSE)
    file(REMOVE_RECURSE "${TEST_PROJECT}")
    file(MAKE_DIRECTORY "${TEST_PROJECT}")
    file(WRITE "${TEST_PROJECT}/CMakeLists.txt"
      [=[
cmake_minimum_required(VERSION 3.5)
project(test)
option(EXPECT_SUCCESS "" ON)
file(WRITE "${CMAKE_SOURCE_DIR}/incorrect_source.cpp" "void undefined_symbol(); void symbol() { undefined_symbol(); }")
if (EXPECT_SUCCESS)
  file(APPEND "${CMAKE_SOURCE_DIR}/incorrect_source.cpp" " void undefined_symbol() {}; ")
endif()
add_library(l0 SHARED incorrect_source.cpp)
add_library(l1 MODULE incorrect_source.cpp)
add_library(l2 STATIC incorrect_source.cpp)
add_library(l3 OBJECT incorrect_source.cpp)
]=]
    )
    try_compile(SUCCESS "${TEST_PROJECT}/build" "${TEST_PROJECT}" test
      CMAKE_FLAGS
      "-DCMAKE_SHARED_LINKER_FLAGS=${CMAKE_SHARED_LINKER_FLAGS}"
      "-DCMAKE_MODULE_LINKER_FLAGS=${CMAKE_MODULE_LINKER_FLAGS}"
      "-DEXPECT_SUCCESS=${EXPECT}"
    )
    if (NOT ${SUCCESS} STREQUAL ${EXPECT})
      message(FATAL_ERROR "Undefined symbols test failure: expect(${EXPECT}), success(${SUCCESS})")
    endif()
    file(REMOVE_RECURSE "${TEST_PROJECT}")
  endforeach()
endfunction()
if (NOT (CMAKE_SYSTEM_NAME MATCHES "kOpenBSD.*|OpenBSD.*") AND NOT OSSFUZZ)
  forbid_undefined_symbols()
endif()

if (MINGW)
  function(export_all_symbols)
    unset(TMP)
    add_linker_flag_if_supported(-Wl,--export-all-symbols TMP)
    string(APPEND CMAKE_SHARED_LINKER_FLAGS ${TMP})
    string(APPEND CMAKE_MODULE_LINKER_FLAGS ${TMP})
    set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} PARENT_SCOPE)
    set(CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS} PARENT_SCOPE)
  endfunction()
  export_all_symbols()
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
  message(STATUS "Setting default build type: ${CMAKE_BUILD_TYPE}")
endif()
string(TOLOWER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_LOWER)

# ARCH defines the target architecture, either by an explicit identifier or
# one of the following two keywords. By default, ARCH a value of 'native':
# target arch = host arch, binary is not portable. When ARCH is set to the
# string 'default', no -march arg is passed, which creates a binary that is
# portable across processors in the same family as host processor.  In cases
# when ARCH is not set to an explicit identifier, cmake's builtin is used
# to identify the target architecture, to direct logic in this cmake script.
# Since ARCH is a cached variable, it will not be set on first cmake invocation.
if (NOT ARCH_ID)
if (NOT ARCH OR ARCH STREQUAL "" OR ARCH STREQUAL "native" OR ARCH STREQUAL "default")
  if(CMAKE_SYSTEM_PROCESSOR STREQUAL "")
    set(CMAKE_SYSTEM_PROCESSOR ${CMAKE_HOST_SYSTEM_PROCESSOR})
  endif()
  # On Windows, CMake sets CMAKE{_HOST,}_SYSTEM_PROCESSOR to the value of the
  # PROCESSOR_ARCHITECTURE environment variable. On some systems it may be set to
  # AMD64. Lowercase it to ensure checks for "x86_64" or "amd64" work as expected.
  string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" ARCH_ID)
else()
  set(ARCH_ID "${ARCH}")
endif()
endif()
string(TOLOWER "${ARCH_ID}" ARM_ID)
string(SUBSTRING "${ARM_ID}" 0 3 ARM_TEST)
if (ARM_TEST STREQUAL "arm")
  set(ARM 1)
  string(SUBSTRING "${ARM_ID}" 0 5 ARM_TEST)
  if (ARM_TEST STREQUAL "armv6")
    set(ARM6 1)
  endif()
  if (ARM_TEST STREQUAL "armv7")
    set(ARM7 1)
  endif()
endif()

if (ARM_ID STREQUAL "aarch64" OR ARM_ID STREQUAL "arm64" OR ARM_ID STREQUAL "armv8-a")
  set(ARM 1)
  set(ARM8 1)
  set(ARCH "armv8-a")
endif()

if(ARCH_ID STREQUAL "ppc64le")
  set(PPC64LE 1)
  set(PPC64   0)
  set(PPC     0)
endif()

if(ARCH_ID STREQUAL "powerpc64" OR ARCH_ID STREQUAL "ppc64")
  set(PPC64LE 0)
  set(PPC64   1)
  set(PPC     0)
endif()

if(ARCH_ID STREQUAL "powerpc" OR ARCH_ID STREQUAL "ppc")
  set(PPC64LE 0)
  set(PPC64   0)
  set(PPC     1)
endif()

if(ARCH_ID STREQUAL "s390x")
  set(S390X 1)
endif()

if(ARCH_ID STREQUAL "riscv64")
set(RISCV   1)
set(RISCV64 1)
endif()

if(ARCH_ID STREQUAL "riscv32")
set(RISCV   1)
set(RISCV32 1)
endif()

if(ARCH_ID STREQUAL "loongarch64")
set(LOONGARCH   1)
set(LOONGARCH64 1)
endif()

if(WIN32 OR ARM OR PPC64LE OR PPC64 OR PPC)
  set(OPT_FLAGS_RELEASE "-O2")
else()
  set(OPT_FLAGS_RELEASE "-Ofast")
endif()

# BUILD_TAG is used to select the build type to check for a new version
if(BUILD_TAG)
  message(STATUS "Building build tag ${BUILD_TAG}")
  add_definitions("-DBUILD_TAG=${BUILD_TAG}")
else()
  message(STATUS "Building without build tag")
endif()

option(MANUAL_SUBMODULES "Skip submodule up-to-date checks" OFF)
if(NOT MANUAL_SUBMODULES)
  find_package(Git)
  if(GIT_FOUND)
    function (check_submodule relative_path)
      execute_process(COMMAND git rev-parse "HEAD" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${relative_path} OUTPUT_VARIABLE localHead)
      execute_process(COMMAND git rev-parse "HEAD:${relative_path}" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE checkedHead)
      string(COMPARE EQUAL "${localHead}" "${checkedHead}" upToDate)
      if (upToDate)
        message(STATUS "Submodule '${relative_path}' is up-to-date")
      else()
        message(FATAL_ERROR "Submodule '${relative_path}' is not up-to-date. Please update all submodules with\ngit submodule update --init --force\nor run cmake with -DMANUAL_SUBMODULES=1\n")
      endif()
    endfunction ()
    
    message(STATUS "Checking submodules")
    check_submodule(external/miniupnp)
    check_submodule(external/rapidjson)
    check_submodule(external/randomx)
    check_submodule(external/supercop)
  endif()
endif()

set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG ${OPT_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG ${OPT_FLAGS_RELEASE}")

option(PER_BLOCK_CHECKPOINT "Enable per-block checkpoint" ON)
if(PER_BLOCK_CHECKPOINT)
  add_definitions("-DPER_BLOCK_CHECKPOINT")
endif()

list(INSERT CMAKE_MODULE_PATH 0
  "${CMAKE_SOURCE_DIR}/cmake")

option(BOOST_IGNORE_SYSTEM_PATHS "Ignore boost system paths for local boost installation" OFF)
if (BOOST_IGNORE_SYSTEM_PATHS)
  set(Boost_NO_SYSTEM_PATHS TRUE)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
enable_testing()

option(BUILD_DOCUMENTATION "Build the Doxygen documentation." ON)
option(BUILD_TESTS "Build tests." OFF)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(DEFAULT_BUILD_DEBUG_UTILITIES ON)
else()
  set(DEFAULT_BUILD_DEBUG_UTILITIES OFF)
endif()
option(BUILD_DEBUG_UTILITIES "Build debug utilities." ${DEFAULT_BUILD_DEBUG_UTILITIES})

if(OSSFUZZ)
  message(STATUS "Using OSS-Fuzz fuzzing system")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DOSSFUZZ")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOSSFUZZ")
endif()

# Check whether we're on a 32-bit or 64-bit system
if(CMAKE_SIZEOF_VOID_P EQUAL "8")
  set(DEFAULT_BUILD_64 ON)
else()
  set(DEFAULT_BUILD_64 OFF)
endif()
option(BUILD_64 "Build for 64-bit? 'OFF' builds for 32-bit." ${DEFAULT_BUILD_64})

if(BUILD_64)
  set(ARCH_WIDTH "64")
else()
  set(ARCH_WIDTH "32")
endif()
message(STATUS "Building for a ${ARCH_WIDTH}-bit system")

# Check if we're on FreeBSD so we can exclude the local miniupnpc (it should be installed from ports instead)
# CMAKE_SYSTEM_NAME checks are commonly known, but specifically taken from libsdl's CMakeLists
if(CMAKE_SYSTEM_NAME MATCHES "kFreeBSD.*|FreeBSD")
  set(FREEBSD TRUE)
endif()

# Check if we're on DragonFly BSD. See the README.md for build instructions.
if(CMAKE_SYSTEM_NAME MATCHES "DragonFly.*")
  set(DRAGONFLY TRUE)
endif()

# Check if we're on OpenBSD. See the README.md for build instructions.
if(CMAKE_SYSTEM_NAME MATCHES "kOpenBSD.*|OpenBSD.*")
  set(OPENBSD TRUE)
elseif(CMAKE_SYSTEM_NAME MATCHES "kNetBSD.*|NetBSD.*")
  set(NETBSD TRUE)
elseif(CMAKE_SYSTEM_NAME MATCHES ".*BSDI.*")
  set(BSDI TRUE)
endif()

include_directories(external/rapidjson/include external/easylogging++ src contrib/epee/include external external/supercop/include)

if(MINGW)
  set(DEFAULT_STATIC true)
else()
  set(DEFAULT_STATIC false)
endif()
option(STATIC "Link libraries statically" ${DEFAULT_STATIC})

# This is a CMake built-in switch that concerns internal libraries
set(BUILD_SHARED_LIBS_DEFAULT OFF)
if (NOT STATIC AND CMAKE_BUILD_TYPE_LOWER STREQUAL "debug")
	set(BUILD_SHARED_LIBS_DEFAULT ON)
endif()
option(BUILD_SHARED_LIBS "Build internal libraries as shared" ${BUILD_SHARED_LIBS_DEFAULT})

if (BUILD_SHARED_LIBS)
  message(STATUS "Building internal libraries with position independent code")
  add_definitions("-DBUILD_SHARED_LIBS")
else()
  message(STATUS "Building internal libraries as static")
endif()
set(PIC_FLAG "-fPIC")

if(STATIC)
  set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DZMQ_STATIC")
endif()

option(SANITIZE "Use ASAN memory sanitizer" OFF)
if(SANITIZE)
  message(STATUS "Using ASAN")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif()

# Set default blockchain storage location:
# memory was the default in Cryptonote before Monero implemented LMDB, it still works but is unnecessary.
# set(DATABASE memory)
set(DATABASE lmdb)
message(STATUS "Using LMDB as default DB type")
set(BLOCKCHAIN_DB DB_LMDB)
add_definitions("-DDEFAULT_DB_TYPE=\"lmdb\"")
add_definitions("-DBLOCKCHAIN_DB=${BLOCKCHAIN_DB}")

# Can't install hook in static build on OSX, because OSX linker does not support --wrap
# On ARM, having libunwind package (with .so's only) installed breaks static link.
# When possible, avoid stack tracing using libunwind in favor of using easylogging++.
if (APPLE OR NETBSD)
  set(DEFAULT_STACK_TRACE OFF)
  set(LIBUNWIND_LIBRARIES "")
elseif (DEPENDS AND NOT LINUX)
  set(DEFAULT_STACK_TRACE OFF)
  set(LIBUNWIND_LIBRARIES "")
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT MINGW)
  set(DEFAULT_STACK_TRACE ON)
  set(STACK_TRACE_LIB "easylogging++") # for diag output only
  set(LIBUNWIND_LIBRARIES "")
elseif (ARM)
  set(DEFAULT_STACK_TRACE OFF)
  set(LIBUNWIND_LIBRARIES "")
else()
  find_package(Libunwind)
  if(LIBUNWIND_FOUND)
    set(DEFAULT_STACK_TRACE ON)
    set(STACK_TRACE_LIB "libunwind") # for diag output only
  else()
    set(DEFAULT_STACK_TRACE OFF)
    set(LIBUNWIND_LIBRARIES "")
  endif()
endif()

option(STACK_TRACE "Install a hook that dumps stack on exception" ${DEFAULT_STACK_TRACE})

if(STACK_TRACE)
  message(STATUS "Stack trace on exception enabled (using ${STACK_TRACE_LIB})")
else()
  message(STATUS "Stack trace on exception disabled")
endif()

if (UNIX AND NOT APPLE)
  # Note that at the time of this writing the -Wstrict-prototypes flag added below will make this fail
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads)
  add_c_flag_if_supported(-pthread CMAKE_C_FLAGS)
  add_cxx_flag_if_supported(-pthread CMAKE_CXX_FLAGS)
endif()

# Handle OpenSSL, used for sha256sum on binary updates and light wallet ssl http
if (CMAKE_SYSTEM_NAME MATCHES "(SunOS|Solaris)")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthreads")
endif ()

if (APPLE AND NOT IOS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=default")
  if (NOT OPENSSL_ROOT_DIR)
      EXECUTE_PROCESS(COMMAND brew --prefix openssl
        OUTPUT_VARIABLE OPENSSL_ROOT_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "Using OpenSSL found at ${OPENSSL_ROOT_DIR}")
  endif()
endif()

find_package(OpenSSL REQUIRED)
message(STATUS "Using OpenSSL include dir at ${OPENSSL_INCLUDE_DIR}")
include_directories(${OPENSSL_INCLUDE_DIR})
if(STATIC AND NOT IOS)
  if(UNIX)
    set(OPENSSL_LIBRARIES "${OPENSSL_LIBRARIES};${CMAKE_DL_LIBS};${CMAKE_THREAD_LIBS_INIT}")
  endif()
endif()

if (WIN32)
  list(APPEND OPENSSL_LIBRARIES ws2_32 crypt32)
endif()

find_package(HIDAPI)

add_definition_if_library_exists(c memset_s "string.h" HAVE_MEMSET_S)
add_definition_if_library_exists(c explicit_bzero "strings.h" HAVE_EXPLICIT_BZERO)
add_definition_if_function_found(strptime HAVE_STRPTIME)

add_definitions(-DAUTO_INITIALIZE_EASYLOGGINGPP)

set(MONERO_GENERATED_HEADERS_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated_include")
include_directories(${MONERO_GENERATED_HEADERS_DIR})

# As of OpenBSD 6.8, -march=<anything> breaks the build
function(set_default_arch)
  if (OPENBSD)
    set(ARCH default)
  else()
    set(ARCH native)
  endif()

  set(ARCH ${ARCH} CACHE STRING "CPU to build for: -march value or 'default' to not pass -march at all")
endfunction()

if (NOT ARCH)
    set_default_arch()
endif()

option(COVERAGE "Enable profiling for test coverage report" OFF)
if(COVERAGE)
    message(STATUS "Building with profiling for test coverage report")
endif()
macro (monero_enable_coverage)
  if(COVERAGE)
    foreach(COV_FLAG -fprofile-arcs -ftest-coverage --coverage)
      set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${COV_FLAG}")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COV_FLAG}")
    endforeach()
  endif()
endmacro()

function (monero_add_library name)
    monero_add_library_with_deps(NAME "${name}" SOURCES ${ARGN})
endfunction()

function (monero_add_library_with_deps)
  cmake_parse_arguments(MONERO_ADD_LIBRARY "" "NAME" "DEPENDS;SOURCES" ${ARGN})
  source_group("${MONERO_ADD_LIBRARY_NAME}" FILES ${MONERO_ADD_LIBRARY_SOURCES})

  # Define a ("virtual") object library and an actual library that links those
  # objects together. The virtual libraries can be arbitrarily combined to link
  # any subset of objects into one library archive. This is used for releasing
  # libwallet, which combines multiple components.
  set(objlib obj_${MONERO_ADD_LIBRARY_NAME})
  add_library(${objlib} OBJECT ${MONERO_ADD_LIBRARY_SOURCES})
  add_library("${MONERO_ADD_LIBRARY_NAME}" $<TARGET_OBJECTS:${objlib}>)
  monero_set_target_no_relink("${MONERO_ADD_LIBRARY_NAME}")
  monero_set_target_strip    ("${MONERO_ADD_LIBRARY_NAME}")
  if (MONERO_ADD_LIBRARY_DEPENDS)
    add_dependencies(${objlib} ${MONERO_ADD_LIBRARY_DEPENDS})
  endif()
  set_property(TARGET "${MONERO_ADD_LIBRARY_NAME}" PROPERTY FOLDER "libs")
  target_compile_definitions(${objlib}
    PRIVATE $<TARGET_PROPERTY:${MONERO_ADD_LIBRARY_NAME},INTERFACE_COMPILE_DEFINITIONS>)
endfunction ()

# Generate header for embedded translations
# Generate header for embedded translations, use target toolchain if depends, otherwise use the
# lrelease and lupdate binaries from the host
include(ExternalProject)
ExternalProject_Add(generate_translations_header
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/translations"
  BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/translations"
  STAMP_DIR ${LRELEASE_PATH}
  CMAKE_ARGS -DLRELEASE_PATH=${LRELEASE_PATH}
  INSTALL_COMMAND ${CMAKE_COMMAND} -E echo "")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/translations")
add_subdirectory(external)

# Final setup for libunbound
include_directories(${UNBOUND_INCLUDE_DIR})

# Final setup for easylogging++
include_directories(${EASYLOGGING_INCLUDE})
link_directories(${EASYLOGGING_LIBRARY_DIRS})

# Final setup for liblmdb
include_directories(${LMDB_INCLUDE})

# Final setup for libunwind
include_directories(${LIBUNWIND_INCLUDE})
link_directories(${LIBUNWIND_LIBRARY_DIRS})

# Final setup for hid
if (HIDAPI_FOUND) 
  message(STATUS "Using HIDAPI include dir at ${HIDAPI_INCLUDE_DIR}")
  add_definitions(-DHAVE_HIDAPI)
  include_directories(${HIDAPI_INCLUDE_DIR})
  link_directories(${LIBHIDAPI_LIBRARY_DIRS})
else()
  message(STATUS "Could not find HIDAPI")
endif()

# Trezor support check
include(CheckTrezor)

  include(TestCXXAcceptsFlag)
  message(STATUS "Building on ${CMAKE_SYSTEM_PROCESSOR} for ${ARCH}")
  if(ARCH STREQUAL "default")
    set(ARCH_FLAG "")
  elseif(PPC64LE)
    set(ARCH_FLAG "-mcpu=power8")
  elseif(PPC64)
    set(ARCH_FLAG "-mcpu=970")
  elseif(PPC)
    set(ARCH_FLAG "-mcpu=7400")
  elseif(IOS AND ARCH STREQUAL "arm64")
    message(STATUS "IOS: Changing arch from arm64 to armv8")
    set(ARCH_FLAG "-march=armv8")
  else()
    set(ARCH_FLAG "-march=${ARCH}")
    if(ARCH STREQUAL "native")
      check_c_compiler_flag(-march=native CC_SUPPORTS_MARCH_NATIVE)
      if (NOT CC_SUPPORTS_MARCH_NATIVE)
        check_c_compiler_flag(-mtune=native CC_SUPPORTS_MTUNE_NATIVE)
        if (CC_SUPPORTS_MTUNE_NATIVE)
          set(ARCH_FLAG "-mtune=${ARCH}")
        else()
          set(ARCH_FLAG "")
        endif()
      endif()
    endif()
  endif()

  option(NO_AES "Explicitly disable AES support" ${NO_AES})

  if(NO_AES)
    message(STATUS "AES support explicitly disabled")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_AES")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNO_AES")
  elseif(NOT ARM AND NOT PPC64LE AND NOT PPC64 AND NOT PPC AND NOT S390X AND NOT RISCV AND NOT LOONGARCH)
    message(STATUS "AES support enabled")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -maes")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -maes")
  elseif(PPC64LE OR PPC64 OR PPC)
    message(STATUS "AES support not available on POWER")
  elseif(S390X)
    message(STATUS "AES support not available on s390x")
  elseif(RISCV)
    message(STATUS "AES support not available on RISC-V")
  elseif(LOONGARCH)
    message(STATUS "AES support not available on LOONGARCH")
  elseif(ARM6)
    message(STATUS "AES support not available on ARMv6")
  elseif(ARM7)
    message(STATUS "AES support not available on ARMv7")
  elseif(ARM8)
    CHECK_CXX_ACCEPTS_FLAG("-march=${ARCH}+crypto" ARCH_PLUS_CRYPTO)
    if(ARCH_PLUS_CRYPTO)
      message(STATUS "Crypto extensions enabled for ARMv8")
      set(ARCH_FLAG "-march=${ARCH}+crypto")
    else()
      message(STATUS "Crypto extensions unavailable on your ARMv8 device")
    endif()
  else()
    message(STATUS "AES support disabled")
  endif()

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ARCH_FLAG}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ARCH_FLAG}")

  set(WARNINGS "-Wall -Wextra -Wpointer-arith -Wundef -Wvla -Wwrite-strings -Wno-error=extra -Wno-error=deprecated-declarations -Wno-unused-parameter -Wno-error=unused-variable -Wno-error=undef -Wno-error=uninitialized")
  if(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(ARM)
      set(WARNINGS "${WARNINGS} -Wno-error=inline-asm")
    endif()
  else()
    set(WARNINGS "${WARNINGS} -Wlogical-op -Wno-error=maybe-uninitialized -Wno-error=cpp")
  endif()
  if(MINGW)
    set(WARNINGS "${WARNINGS} -Wno-error=unused-value -Wno-error=unused-but-set-variable")
    set(MINGW_FLAG "${MINGW_FLAG} -DWIN32_LEAN_AND_MEAN")
    set(Boost_THREADAPI win32)
    # mingw doesn't support LTO (multiple definition errors at link time)
    set(USE_LTO_DEFAULT false)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--stack,10485760")
    if(NOT BUILD_64)
      add_definitions(-DWINVER=0x0600 -D_WIN32_WINNT=0x0600)
    endif()
  endif()
  set(C_WARNINGS "-Waggregate-return -Wnested-externs -Wold-style-definition -Wstrict-prototypes")
  set(CXX_WARNINGS "-Wno-reorder -Wno-missing-field-initializers")
  try_compile(STATIC_ASSERT_RES "${CMAKE_CURRENT_BINARY_DIR}/static-assert" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/test-static-assert.c" CMAKE_FLAGS -DCMAKE_C_STANDARD=11)
  if(STATIC_ASSERT_RES)
    set(STATIC_ASSERT_FLAG "")
  else()
    set(STATIC_ASSERT_FLAG "-Dstatic_assert=_Static_assert")
  endif()

  try_compile(STATIC_ASSERT_CPP_RES "${CMAKE_CURRENT_BINARY_DIR}/static-assert" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/test-static-assert.cpp" CMAKE_FLAGS -DCMAKE_CXX_STANDARD=11)
  if(STATIC_ASSERT_CPP_RES)
    set(STATIC_ASSERT_CPP_FLAG "")
  else()
    set(STATIC_ASSERT_CPP_FLAG "-Dstatic_assert=_Static_assert")
  endif()

  monero_enable_coverage()
  # With GCC 6.1.1 the compiled binary malfunctions due to aliasing. Until that
  # is fixed in the code (Issue #847), force compiler to be conservative.
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing")

  # if those don't work for your compiler, single it out where appropriate
  if(CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT OPENBSD)
    set(C_SECURITY_FLAGS "${C_SECURITY_FLAGS} -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=1")
    set(CXX_SECURITY_FLAGS "${CXX_SECURITY_FLAGS} -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=1")
  endif()

  # warnings
  add_c_flag_if_supported(-Wformat C_SECURITY_FLAGS)
  add_cxx_flag_if_supported(-Wformat CXX_SECURITY_FLAGS)
  add_c_flag_if_supported(-Wformat-security C_SECURITY_FLAGS)
  add_cxx_flag_if_supported(-Wformat-security CXX_SECURITY_FLAGS)

  # -fstack-protector
  if (NOT OPENBSD AND NOT (WIN32 AND (CMAKE_C_COMPILER_ID STREQUAL "GNU" AND CMAKE_C_COMPILER_VERSION VERSION_LESS 9.1)))
    add_c_flag_if_supported(-fstack-protector C_SECURITY_FLAGS)
    add_cxx_flag_if_supported(-fstack-protector CXX_SECURITY_FLAGS)
    add_c_flag_if_supported(-fstack-protector-strong C_SECURITY_FLAGS)
    add_cxx_flag_if_supported(-fstack-protector-strong CXX_SECURITY_FLAGS)
  endif()

  # New in GCC 8.2
  if (NOT OPENBSD AND NOT (WIN32 AND (CMAKE_C_COMPILER_ID STREQUAL "GNU" AND CMAKE_C_COMPILER_VERSION VERSION_LESS 9.1)))
    add_c_flag_if_supported(-fcf-protection=full C_SECURITY_FLAGS)
    add_cxx_flag_if_supported(-fcf-protection=full CXX_SECURITY_FLAGS)
  endif()
  if (NOT WIN32 AND NOT OPENBSD)
    add_c_flag_if_supported(-fstack-clash-protection C_SECURITY_FLAGS)
    add_cxx_flag_if_supported(-fstack-clash-protection CXX_SECURITY_FLAGS)
  endif()

  # Removed in GCC 9.1 (or before ?), but still accepted, so spams the output
  if (NOT (CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 9.1))
    add_c_flag_if_supported(-mmitigate-rop C_SECURITY_FLAGS)
    add_cxx_flag_if_supported(-mmitigate-rop CXX_SECURITY_FLAGS)
  endif()

  # linker
  if (NOT SANITIZE AND NOT OSSFUZZ AND NOT (WIN32 AND (CMAKE_C_COMPILER_ID STREQUAL "GNU" AND (CMAKE_C_COMPILER_VERSION VERSION_LESS 9.1 OR NOT STATIC))))
    # PIE executables randomly crash at startup with ASAN
    # Windows binaries die on startup with PIE when compiled with GCC <9.x
    # Windows dynamically-linked binaries die on startup with PIE regardless of GCC version
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
      # Clang does not support -pie flag
      add_linker_flag_if_supported("-Wl,-pie" LD_SECURITY_FLAGS)
    else()
      add_linker_flag_if_supported("-pie" LD_SECURITY_FLAGS)
    endif()
  endif()
  add_linker_flag_if_supported(-Wl,-z,relro LD_SECURITY_FLAGS)
  add_linker_flag_if_supported(-Wl,-z,now LD_SECURITY_FLAGS)
  add_linker_flag_if_supported(-Wl,-z,noexecstack noexecstack_SUPPORTED)
  if (noexecstack_SUPPORTED)
    set(LD_SECURITY_FLAGS "${LD_SECURITY_FLAGS} -Wl,-z,noexecstack")
  endif()
  add_linker_flag_if_supported(-Wl,-z,noexecheap noexecheap_SUPPORTED)
  if (noexecheap_SUPPORTED)
    set(LD_SECURITY_FLAGS "${LD_SECURITY_FLAGS} -Wl,-z,noexecheap")
  endif()

  # some windows linker bits
  if (WIN32)
    add_linker_flag_if_supported(-Wl,--dynamicbase LD_SECURITY_FLAGS)
    add_linker_flag_if_supported(-Wl,--nxcompat LD_SECURITY_FLAGS)
    add_linker_flag_if_supported(-Wl,--high-entropy-va LD_SECURITY_FLAGS)
  endif()

  # Warnings, that when ignored are so severe, that they can segfault or even UB any application.
  # Treat them as errors.
  add_c_flag_if_supported(  -Werror=switch      C_SECURITY_FLAGS)
  add_cxx_flag_if_supported(-Werror=switch      CXX_SECURITY_FLAGS)
  add_c_flag_if_supported(  -Werror=return-type C_SECURITY_FLAGS)
  add_cxx_flag_if_supported(-Werror=return-type CXX_SECURITY_FLAGS)

  message(STATUS "Using C security hardening flags: ${C_SECURITY_FLAGS}")
  message(STATUS "Using C++ security hardening flags: ${CXX_SECURITY_FLAGS}")
  message(STATUS "Using linker security hardening flags: ${LD_SECURITY_FLAGS}")

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE ${MINGW_FLAG} ${STATIC_ASSERT_FLAG} ${WARNINGS} ${C_WARNINGS} ${PIC_FLAG} ${C_SECURITY_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GNU_SOURCE ${MINGW_FLAG} ${STATIC_ASSERT_CPP_FLAG} ${WARNINGS} ${CXX_WARNINGS} ${PIC_FLAG} ${CXX_SECURITY_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LD_SECURITY_FLAGS}")

  # With GCC 6.1.1 the compiled binary malfunctions due to aliasing. Until that
  # is fixed in the code (Issue #847), force compiler to be conservative.
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing")

  if(ARM)
    message(STATUS "Setting FPU Flags for ARM Processors")

    #NB NEON hardware does not fully implement the IEEE 754 standard for floating-point arithmetic
    #Need custom assembly code to take full advantage of NEON SIMD

    #Cortex-A5/9  -mfpu=neon-fp16
    #Cortex-A7/15 -mfpu=neon-vfpv4
    #Cortex-A8    -mfpu=neon
    #ARMv8  	  -FP and SIMD on by default for all ARM8v-A series, NO -mfpu setting needed

    #For custom -mtune, processor IDs for ARMv8-A series:
    #0xd04 - Cortex-A35
    #0xd07 - Cortex-A57
    #0xd08 - Cortex-A72
    #0xd03 - Cortex-A73

    if(NOT ARM8)
      CHECK_CXX_ACCEPTS_FLAG(-mfpu=vfp3-d16 CXX_ACCEPTS_VFP3_D16)
      CHECK_CXX_ACCEPTS_FLAG(-mfpu=vfp4 CXX_ACCEPTS_VFP4)
      CHECK_CXX_ACCEPTS_FLAG(-mfloat-abi=hard CXX_ACCEPTS_MFLOAT_HARD)
      CHECK_CXX_ACCEPTS_FLAG(-mfloat-abi=softfp CXX_ACCEPTS_MFLOAT_SOFTFP)
    endif()

    if(ARM8)
      CHECK_CXX_ACCEPTS_FLAG(-mfix-cortex-a53-835769 CXX_ACCEPTS_MFIX_CORTEX_A53_835769)
      CHECK_CXX_ACCEPTS_FLAG(-mfix-cortex-a53-843419 CXX_ACCEPTS_MFIX_CORTEX_A53_843419)
    endif()

    if(ARM6)
      message(STATUS "Selecting VFP for ARMv6")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=vfp")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=vfp")
      if(DEPENDS)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -marm")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -marm")
      endif()
    endif(ARM6)

    if(ARM7)
      if(CXX_ACCEPTS_VFP3_D16 AND NOT CXX_ACCEPTS_VFP4)
        message(STATUS "Selecting VFP3 for ARMv7")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=vfp3-d16")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=vfp3-d16")
      endif()

      if(CXX_ACCEPTS_VFP4)
        message(STATUS "Selecting VFP4 for ARMv7")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=vfp4")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=vfp4")
      endif()

      if(CXX_ACCEPTS_MFLOAT_HARD)
        message(STATUS "Setting Hardware ABI for Floating Point")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfloat-abi=hard")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfloat-abi=hard")
      endif()

      if(CXX_ACCEPTS_MFLOAT_SOFTFP AND NOT CXX_ACCEPTS_MFLOAT_HARD)
        message(STATUS "Setting Software ABI for Floating Point")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfloat-abi=softfp")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfloat-abi=softfp")
      endif()
    endif(ARM7)

    if(ARM8)
      if(CXX_ACCEPTS_MFIX_CORTEX_A53_835769)
        message(STATUS "Enabling Cortex-A53 workaround 835769")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfix-cortex-a53-835769")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfix-cortex-a53-835769")
      endif()

      if(CXX_ACCEPTS_MFIX_CORTEX_A53_843419)
        message(STATUS "Enabling Cortex-A53 workaround 843419")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfix-cortex-a53-843419")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfix-cortex-a53-843419")
      endif()
    endif(ARM8)

  endif(ARM)

  # random crash on startup when asan is on if pie is enabled
  if(NOT SANITIZE AND ANDROID AND NOT BUILD_GUI_DEPS STREQUAL "ON" OR IOS)
    #From Android 5: "only position independent executables (PIE) are supported" 
    message(STATUS "Enabling PIE executable")
    set(PIC_FLAG "")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIE")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_CXX_FLAGS} -fPIE -pie")
  endif()

  if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=default -DGTEST_HAS_TR1_TUPLE=0")
    if(ARM)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-aligned-allocation")
    endif()
  endif()

  set(DEBUG_FLAGS "-g3")

  # At least some CLANGs default to not enough for monero
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=900")

  if(NOT DEFINED USE_LTO_DEFAULT)
    set(USE_LTO_DEFAULT false)
  endif()
  set(USE_LTO ${USE_LTO_DEFAULT} CACHE BOOL "Use Link-Time Optimization (Release mode only)")

  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # There is a clang bug that does not allow to compile code that uses AES-NI intrinsics if -flto is enabled, so explicitly disable
    set(USE_LTO false)
  endif()


  if(USE_LTO)
    set(RELEASE_FLAGS "${RELEASE_FLAGS} -flto")
    if(STATIC)
      set(RELEASE_FLAGS "${RELEASE_FLAGS} -ffat-lto-objects")
    endif()
    # Since gcc 4.9 the LTO format is non-standard (slim), so we need the gcc-specific ar and ranlib binaries
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9.0) AND NOT OPENBSD AND NOT DRAGONFLY)
      # When invoking cmake on distributions on which gcc's binaries are prefixed
      # with an arch-specific triplet, the user must specify -DCHOST=<prefix>
      if (DEFINED CHOST)
        set(CMAKE_AR "${CHOST}-gcc-ar")
        set(CMAKE_RANLIB "${CHOST}-gcc-ranlib")
      else()
        set(CMAKE_AR "gcc-ar")
        set(CMAKE_RANLIB "gcc-ranlib")
      endif()
    endif()
  endif()

  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${DEBUG_FLAGS}")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${DEBUG_FLAGS}")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${RELEASE_FLAGS}")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${RELEASE_FLAGS}")

  if(STATIC)
    # STATIC already configures most deps to be linked in statically,
    # here we make more deps static if the platform permits it
    if (MINGW)
      # On Windows, this is as close to fully-static as we get:
      # this leaves only deps on /c/Windows/system32/*.dll
      set(STATIC_FLAGS "-static")
    elseif (NOT (APPLE OR FREEBSD OR OPENBSD OR DRAGONFLY))
      # On Linux, we don't support fully static build, but these can be static
      set(STATIC_FLAGS "-static-libgcc -static-libstdc++")
    endif()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${STATIC_FLAGS} ")
  endif()

set(OLD_LIB_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
set(Boost_NO_BOOST_CMAKE ON)
if(STATIC)
  if(MINGW)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
  endif()

  set(Boost_USE_STATIC_LIBS ON)
  set(Boost_USE_STATIC_RUNTIME ON)
endif()

# Find Boost headers
set(BOOST_MIN_VER 1.62)
find_package(Boost ${BOOST_MIN_VER} QUIET REQUIRED)

if(NOT Boost_FOUND)
  die("Could not find Boost libraries, please make sure you have installed Boost or libboost-all-dev (>=${BOOST_MIN_VER}) or the equivalent")
elseif(Boost_FOUND)
  message(STATUS "Found Boost Version: ${Boost_VERSION_STRING}")

  set(BOOST_COMPONENTS filesystem thread date_time chrono serialization program_options)
  if (WIN32)
    list(APPEND BOOST_COMPONENTS locale)
  endif()

  # Boost System is header-only since 1.69
  if (Boost_VERSION_STRING VERSION_LESS 1.69.0)
    list(APPEND BOOST_COMPONENTS system)
  endif()

  # Boost Regex is header-only since 1.77
  if (Boost_VERSION_STRING VERSION_LESS 1.77.0)
    list(APPEND BOOST_COMPONENTS regex)
  endif()

  message(STATUS "Boost components: ${BOOST_COMPONENTS}")

  # Find required Boost libraries
  find_package(Boost ${BOOST_MIN_VER} QUIET REQUIRED COMPONENTS ${BOOST_COMPONENTS})
  set(CMAKE_FIND_LIBRARY_SUFFIXES ${OLD_LIB_SUFFIXES})
endif()

add_definitions(-DBOOST_ASIO_ENABLE_SEQUENTIAL_STRAND_ALLOCATION)
add_definitions(-DBOOST_NO_AUTO_PTR)
add_definitions(-DBOOST_UUID_DISABLE_ALIGNMENT) # This restores UUID's std::has_unique_object_representations property

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
if(MINGW)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wa,-mbig-obj")
  set(EXTRA_LIBRARIES mswsock;ws2_32;iphlpapi;crypt32;bcrypt)
  if(NOT DEPENDS)
    set(ICU_LIBRARIES icuio icuin icuuc icudt icutu iconv)
  endif()
elseif(APPLE OR OPENBSD OR ANDROID)
  set(EXTRA_LIBRARIES "")
elseif(FREEBSD)
  set(EXTRA_LIBRARIES execinfo elf)
elseif(DRAGONFLY)
  find_library(COMPAT compat)
  set(EXTRA_LIBRARIES execinfo ${COMPAT})
elseif(CMAKE_SYSTEM_NAME MATCHES "(SunOS|Solaris)")
  set(EXTRA_LIBRARIES socket nsl resolv)
elseif(NOT DEPENDS)
  find_library(RT rt)
  find_library(Z z)
  set(EXTRA_LIBRARIES ${RT} ${Z})
endif()

list(APPEND EXTRA_LIBRARIES ${CMAKE_DL_LIBS})

if (HIDAPI_FOUND OR LibUSB_COMPILE_TEST_PASSED)
  if (APPLE)
    if(DEPENDS)
      list(APPEND EXTRA_LIBRARIES "-framework Foundation -framework AppKit -framework IOKit -framework Security")
    else()
      find_library(COREFOUNDATION CoreFoundation)
      find_library(APPKIT AppKit)
      find_library(IOKIT IOKit)
      list(APPEND EXTRA_LIBRARIES ${APPKIT})
      list(APPEND EXTRA_LIBRARIES ${IOKIT})
      list(APPEND EXTRA_LIBRARIES ${COREFOUNDATION})
    endif()
  endif()
  if (WIN32)
    list(APPEND EXTRA_LIBRARIES setupapi)
  endif()
endif()

if(NOT OPENBSD)
  option(USE_READLINE "Build with GNU readline support." ON)
endif()
if(USE_READLINE AND NOT DEPENDS)
  find_package(Readline)
  if(READLINE_FOUND AND GNU_READLINE_FOUND)
    add_definitions(-DHAVE_READLINE)
    include_directories(${Readline_INCLUDE_DIR})
    message(STATUS "Found readline library at: ${Readline_ROOT_DIR}")
    set(EPEE_READLINE epee_readline)
  else()
    message(STATUS "Could not find GNU readline library so building without readline support")
  endif()
elseif(USE_READLINE AND DEPENDS AND NOT MINGW)
  find_path(Readline_INCLUDE_PATH readline/readline.h)
  find_library(Readline_LIBRARY readline)
  find_library(Terminfo_LIBRARY tinfo)
  set(Readline_LIBRARY "${Readline_LIBRARY};${Terminfo_LIBRARY}")
  set(GNU_READLINE_LIBRARY ${Readline_LIBRARY})
  add_definitions(-DHAVE_READLINE)
  set(EPEE_READLINE epee_readline)
endif()

if(STATIC)
  set(sodium_USE_STATIC_LIBS ON)
endif()
find_package(Sodium REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(libzmq REQUIRED IMPORTED_TARGET libzmq)

include(external/supercop/functions.cmake) # place after setting flags and before src directory inclusion
add_subdirectory(contrib)
add_subdirectory(src)

find_package(PythonInterp)
if(BUILD_TESTS)
  message(STATUS "Building tests")
  add_subdirectory(tests)
else()
  message(STATUS "Not building tests")
endif()

if(BUILD_DEBUG_UTILITIES)
  message(STATUS "Building debug utilities")
else()
  message(STATUS "Not building debug utilities")
endif()

if(BUILD_DOCUMENTATION)
  set(DOC_GRAPHS "YES" CACHE STRING "Create dependency graphs (needs graphviz)")
  set(DOC_FULLGRAPHS "NO" CACHE STRING "Create call/callee graphs (large)")

  find_program(DOT_PATH dot)

  if (DOT_PATH STREQUAL "DOT_PATH-NOTFOUND")
    message("Doxygen: graphviz not found - graphs disabled")
    set(DOC_GRAPHS "NO")
  endif()

  find_package(Doxygen)
  if(DOXYGEN_FOUND)
    configure_file("cmake/Doxyfile.in" "Doxyfile" @ONLY)
    configure_file("cmake/Doxygen.extra.css.in" "Doxygen.extra.css" @ONLY)
    add_custom_target(doc
      ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen.." VERBATIM)
  endif()
endif()

# when ON - will install libwallet_merged into "lib"
option(BUILD_GUI_DEPS "Build GUI dependencies." OFF)

find_package(PythonInterp)
find_program(iwyu_tool_path NAMES iwyu_tool.py iwyu_tool)
if (iwyu_tool_path AND PYTHONINTERP_FOUND)
  add_custom_target(iwyu
    COMMAND "${PYTHON_EXECUTABLE}" "${iwyu_tool_path}" -p "${CMAKE_BINARY_DIR}" -- --no_fwd_decls
    COMMENT "Running include-what-you-use tool"
    VERBATIM
  )
endif()
