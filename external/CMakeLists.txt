# --- Dependency Management ---

## MiniUPnP:
# The project aims to use the in-tree miniupnpc source code for consistency,
# especially when building statically, to minimize external dependency issues.

message(STATUS "Using in-tree miniupnpc source code.")

# Set internal variables to control the in-tree build of miniupnp.
# We set them once and avoid using 'FORCE' to respect potential user overrides.
if (NOT DEFINED UPNPC_NO_INSTALL)
    set(UPNPC_NO_INSTALL TRUE CACHE BOOL "Disable miniupnp installation")
endif()
if (NOT DEFINED UPNPC_BUILD_SHARED)
    # Ensure static library is built, which is generally preferred for linking.
    set(UPNPC_BUILD_SHARED OFF CACHE BOOL "Disable building shared library")
endif()

# Add the miniupnpc source directory to the build.
add_subdirectory(miniupnp/miniupnpc)

# Configure the resulting static library target.
set_property(TARGET libminiupnpc-static PROPERTY FOLDER "external")
# Set Position Independent Code (PIC) ON for robust linking, especially with shared libraries.
set_property(TARGET libminiupnpc-static PROPERTY POSITION_INDEPENDENT_CODE ON)

# Suppress specific warnings for the external library to keep the build log clean.
# Using target_compile_options is the modern CMake way.
target_compile_options(libminiupnpc-static PRIVATE
    -Wno-undef
    -Wno-unused-result
    -Wno-unused-value
)

# Apply NetBSD-specific definition if compiling on that system.
if (CMAKE_SYSTEM_NAME MATCHES "NetBSD")
    target_compile_options(libminiupnpc-static PRIVATE
        -D_NETBSD_SOURCE
    )
endif()

# Export the library name for use in the parent scope or other modules.
set(UPNP_LIBRARIES "libminiupnpc-static" PARENT_SCOPE)

# --- Libunbound Dependency Check ---

# Attempt to find the external libunbound library.
find_package(Unbound)

if(NOT Unbound_FOUND)
    # Using message(FATAL_ERROR) is the standard CMake way to stop the build.
    message(FATAL_ERROR "Could not find libunbound. Please ensure the library and headers are installed (e.g., libunbound-dev).")
else()
    message(STATUS "Found libunbound: includes in ${UNBOUND_INCLUDE_DIR}")

    # Note: If Unbound_FOUND is true, CMake guarantees UNBOUND_LIBRARIES is set
    # if the library was found. The explicit check is mostly redundant but kept
    # for defensive programming style if a custom FindUnbound.cmake is used.
    if(NOT UNBOUND_LIBRARIES)
        message(FATAL_ERROR "Found libunbound includes, but could not find the library files. Check your linker paths.")
    endif()
endif()

# --- Internal Subdirectories ---

# Add other project components.
add_subdirectory(db_drivers)
add_subdirectory(easylogging++)
add_subdirectory(qrcodegen)
# RandomX is often a heavy component and is marked EXCLUDE_FROM_ALL.
add_subdirectory(randomx EXCLUDE_FROM_ALL)
