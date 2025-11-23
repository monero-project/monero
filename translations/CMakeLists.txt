# Copyright (c) 2017-2025, The Monero Project
# ... (License header kept intact) ...

# Bumped to 3.16 for CONFIGURE_DEPENDS and better generator expressions.
cmake_minimum_required(VERSION 3.16)

project(translations)

# -----------------------------------------------------------------------------
# 1. Handle Cross-Compilation & Generator Tool
# -----------------------------------------------------------------------------
# We need a tool 'generate_translations_header' to embed translations.
# If cross-compiling, we must import a natively built version of this tool.

if(CMAKE_CROSSCOMPILING)
  message(STATUS "Cross-compiling: Importing native generator tool.")
  set(IMPORT_EXECUTABLES "${CMAKE_CURRENT_BINARY_DIR}/ImportExecutables.cmake" CACHE FILEPATH "Path to the native build export file")
  include(${IMPORT_EXECUTABLES})
else()
  # Build the generator natively
  add_executable(generate_translations_header generate_translations_header.c)
endif()

# -----------------------------------------------------------------------------
# 2. Locate lrelease (Qt Linguist Tool)
# -----------------------------------------------------------------------------
# Try finding the tool via standard paths or user override.

if(NOT defined LRELEASE_PATH OR LRELEASE_PATH STREQUAL "")
  find_program(LRELEASE NAMES lrelease lrelease-qt5)
else()
  set(LRELEASE ${LRELEASE_PATH}/lrelease)
endif()

if(NOT EXISTS ${LRELEASE})
  message(WARNING "lrelease not found. Translation files will not be built.")
  # Create a dummy target or variable to prevent build failure later
  set(qm_files "")
else()
  # Verify lrelease works
  execute_process(COMMAND ${LRELEASE} -version
    RESULT_VARIABLE lrelease_ret
    OUTPUT_QUIET ERROR_QUIET)
    
  if(NOT lrelease_ret EQUAL "0")
    message(WARNING "lrelease found but not working. Translations skipped.")
    set(qm_files "")
  else()
    # -------------------------------------------------------------------------
    # 3. Process Translation Files (.ts -> .qm)
    # -------------------------------------------------------------------------
    # CONFIGURE_DEPENDS ensures CMake re-runs if you add a new .ts file.
    file(GLOB ts_files CONFIGURE_DEPENDS
