# This file is part of MXE. See LICENSE.md for licensing information.

# 2.8.9 is Debian Wheezy version
cmake_minimum_required(VERSION 2.8.9)

# use default C and CXX languages
project(mxe)

# see cmake --help-policy <cmp> for details
cmake_policy(SET CMP0017 NEW)
if (POLICY CMP0020)
    cmake_policy(SET CMP0020 NEW)
endif()

# so we can find pkg-test.cmake files to include
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../.. ${CMAKE_MODULE_PATH})

include(${PKG}-test)
