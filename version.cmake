function (write_static_version_inl hash)
  set(VERSIONTAG "${hash}")
  configure_file("src/version.inl.in" "version/version.inl")
  add_custom_target(version ALL)
endfunction ()

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/version")
file(REMOVE "${CMAKE_BINARY_DIR}/version/version.inl")
find_package(Git QUIET)
if ("$Format:$" STREQUAL "")
  # We're in a tarball; use hard-coded variables.
  write_static_version_inl("release")
elseif (GIT_FOUND OR Git_FOUND)
  message(STATUS "Found Git: ${GIT_EXECUTABLE}")
  set(extra_output)
  if (CMAKE_GENERATOR MATCHES "Ninja")
    # Ninja will not rerun the command every time if the file doesn't change,
    # so inject this bogus output so that it always runs.
    set(extra_output "${CMAKE_SOURCE_DIR}/.force-git-version-check")
  endif ()
  add_custom_command(
    OUTPUT            "${CMAKE_BINARY_DIR}/version/version.inl"
                      ${extra_output}
    COMMAND           "${CMAKE_COMMAND}"
                      "-D" "GIT=${GIT_EXECUTABLE}"
                      "-D" "TO=${CMAKE_BINARY_DIR}/version/version.inl"
                      "-P" "src/version.cmake"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
  add_custom_target(version ALL
    DEPENDS "${CMAKE_BINARY_DIR}/version/version.inl")
else()
  message(STATUS "WARNING: Git was not found!")
  write_static_version_inl("unknown")
endif ()
