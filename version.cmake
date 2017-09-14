function (write_static_version_inl hash)
  set(VERSIONTAG "${hash}")
  configure_file("src/version.cpp.in" "version/version.cpp")
endfunction ()

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/version")
file(REMOVE "${CMAKE_BINARY_DIR}/version/version.cpp")
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
    OUTPUT            "${CMAKE_BINARY_DIR}/version/version.cpp"
                      ${extra_output}
    COMMAND           "${CMAKE_COMMAND}"
                      "-D" "GIT=${GIT_EXECUTABLE}"
                      "-D" "TO=${CMAKE_BINARY_DIR}/version/version.cpp"
                      "-P" "src/version.cmake"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
else()
  message(STATUS "WARNING: Git was not found!")
  write_static_version_inl("unknown")
endif ()
add_custom_target(version ALL
  DEPENDS "${CMAKE_BINARY_DIR}/version/version.cpp")
