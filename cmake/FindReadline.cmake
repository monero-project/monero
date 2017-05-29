# - Try to find readline include dirs and libraries 
#
# Usage of this module as follows:
#
#     find_package(Readline)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  Readline_ROOT_DIR         Set this variable to the root installation of
#                            readline if the module has problems finding the
#                            proper installation path.
#
# Variables defined by this module:
#
#  READLINE_FOUND            System has readline, include and lib dirs found
#  GNU_READLINE_FOUND        Version of readline found is GNU readline, not libedit!
#  Readline_INCLUDE_DIR      The readline include directories. 
#  Readline_LIBRARY          The readline library.

find_path(Readline_ROOT_DIR
    NAMES include/readline/readline.h
    PATHS /opt/local/ /usr/local/ /usr/
    NO_DEFAULT_PATH
)

find_path(Readline_INCLUDE_DIR
    NAMES readline/readline.h
    PATHS ${Readline_ROOT_DIR}/include
    NO_DEFAULT_PATH
)

find_library(Readline_LIBRARY
    NAMES readline
    PATHS ${Readline_ROOT_DIR}/lib
    NO_DEFAULT_PATH
)

if(Readline_INCLUDE_DIR AND Readline_LIBRARY AND Ncurses_LIBRARY)
  set(READLINE_FOUND TRUE)
else(Readline_INCLUDE_DIR AND Readline_LIBRARY AND Ncurses_LIBRARY)
  FIND_LIBRARY(Readline_LIBRARY NAMES readline PATHS Readline_ROOT_DIR)
  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(Readline DEFAULT_MSG Readline_INCLUDE_DIR Readline_LIBRARY )
  MARK_AS_ADVANCED(Readline_INCLUDE_DIR Readline_LIBRARY)
endif(Readline_INCLUDE_DIR AND Readline_LIBRARY AND Ncurses_LIBRARY)

mark_as_advanced(
    Readline_ROOT_DIR
    Readline_INCLUDE_DIR
    Readline_LIBRARY
)

set(CMAKE_REQUIRED_INCLUDES ${Readline_INCLUDE_DIR})
set(CMAKE_REQUIRED_LIBRARIES ${Readline_LIBRARY})
INCLUDE(CheckCXXSourceCompiles) 
CHECK_CXX_SOURCE_COMPILES(
"
#include <stdio.h>
#include <readline/readline.h>
int
main()
{
  char * s  = rl_copy_text(0, 0);
}
" GNU_READLINE_FOUND)
