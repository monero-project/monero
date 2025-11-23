# --- License Block (Omitted for brevity, but remains intact in the file) ---
# Copyright (c) 2014-2024, The Monero Project
# ...

# Function to write the version tag into the C++ source file.
# The VERSIONTAG is set as a CACHE variable to persist across CMake runs.
function (write_version tag)
    # Set the version tag. Removed 'FORCE' to respect user cache modifications.
    set(VERSIONTAG "${tag}" CACHE STRING "The tag portion of the Monero software version")
    
    # Configure the version file template into the binary directory.
    configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/../src/version.cpp.in" 
        "${CMAKE_BINARY_DIR}/version.cpp"
    )
endfunction ()

# --- Version Determination Logic ---

# 1. Try to find Git executable without being mandatory.
find_package(Git QUIET)

# Check for tarball/export condition first, as indicated by the placeholder string.
if ("$Format:$" STREQUAL "")
    # Condition when the source code is likely from a tarball or git archive export.
    message(STATUS "Source appears to be from a tarball. Using release version.")
    set(VERSION_IS_RELEASE "true")
    write_version("release")
    
# Check if Git was successfully found (using the standard variable name: Git_FOUND).
elseif (Git_FOUND)
    message(STATUS "Found Git executable: ${Git_EXECUTABLE}")
    
    # Include the custom Git version script (assuming it defines get_version_tag_from_git).
    include(GitVersion)
    get_version_tag_from_git("${Git_EXECUTABLE}")
    
    # Write the version obtained from Git (VERSIONTAG is set by get_version_tag_from_git).
    write_version("${VERSIONTAG}")
    
# Fallback: Git not found and not a recognized tarball/export.
else()
    message(WARNING "Git was not found! Version tag set to 'unknown'.")
    set(VERSION_IS_RELEASE "false")
    write_version("unknown")
endif ()

# Create a custom target that forces the build system to check the dependencies 
# (i.e., regenerate version.cpp) every time the target is built.
add_custom_target(genversion ALL
    DEPENDS "${CMAKE_BINARY_DIR}/version.cpp"
)
