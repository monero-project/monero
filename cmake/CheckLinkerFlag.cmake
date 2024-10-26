include(CheckCCompilerFlag)

macro(CHECK_LINKER_FLAG flag VARIABLE)
  if(NOT DEFINED "${VARIABLE}")
    if(NOT CMAKE_REQUIRED_QUIET)
      message(STATUS "Looking for ${flag} linker flag")
    endif()

    set(_cle_source ${CMAKE_SOURCE_DIR}/cmake/CheckLinkerFlag.c)

    set(saved_CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
    set(CMAKE_C_FLAGS "${flag}")
    try_compile(${VARIABLE}
      ${CMAKE_BINARY_DIR}
      ${_cle_source}
      COMPILE_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS} ${flag}
      CMAKE_FLAGS
      "-DCMAKE_EXE_LINKER_FLAGS=${flag}"
      OUTPUT_VARIABLE OUTPUT)
    unset(_cle_source)
    set(CMAKE_C_FLAGS ${saved_CMAKE_C_FLAGS})
    unset(saved_CMAKE_C_FLAGS)

    if ("${OUTPUT}" MATCHES "warning.*ignored")
      set(${VARIABLE} 0)
    endif()

    if(${VARIABLE})
      if(NOT CMAKE_REQUIRED_QUIET)
        message(STATUS "Looking for ${flag} linker flag - found")
      endif()
      set(${VARIABLE} 1 CACHE INTERNAL "Have linker flag ${flag}")
      file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
        "Determining if the ${flag} linker flag is supported "
        "passed with the following output:\n"
        "${OUTPUT}\n\n")
    else()
      if(NOT CMAKE_REQUIRED_QUIET)
        message(STATUS "Looking for ${flag} linker flag - not found")
      endif()
      set(${VARIABLE} "" CACHE INTERNAL "Have linker flag ${flag}")
      file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
        "Determining if the ${flag} linker flag is supported "
        "failed with the following output:\n"
        "${OUTPUT}\n\n")
    endif()
  endif()
endmacro()
