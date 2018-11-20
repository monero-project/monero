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
#  LIBEDIT_FOUND             Version of readline found is libedit, not GNU readline!
#  Readline_INCLUDE_DIR      The readline include directories. 
#  Readline_LIBRARY          The readline library.
#  GNU_READLINE_LIBRARY      The GNU readline library or empty string.
#  LIBEDIT_LIBRARY           The libedit library or empty string.

find_path(Readline_ROOT_DIR
    NAMES include/readline/readline.h
    HINTS ENV READLINE_ROOT_DIR
    PATHS /usr/local/opt/readline/ /opt/local/ /usr/local/ /usr/
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

find_library(Termcap_LIBRARY
  NAMES tinfo termcap ncurses ncursesw cursesw curses
)

if(Readline_INCLUDE_DIR AND Readline_LIBRARY)
  set(READLINE_FOUND TRUE)
else(Readline_INCLUDE_DIR AND Readline_LIBRARY)
  FIND_LIBRARY(Readline_LIBRARY NAMES readline PATHS Readline_ROOT_DIR)
  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(Readline DEFAULT_MSG Readline_INCLUDE_DIR Readline_LIBRARY )
  MARK_AS_ADVANCED(Readline_INCLUDE_DIR Readline_LIBRARY)
endif(Readline_INCLUDE_DIR AND Readline_LIBRARY)

mark_as_advanced(
    Readline_ROOT_DIR
    Readline_INCLUDE_DIR
    Readline_LIBRARY
)

set(CMAKE_REQUIRED_INCLUDES ${Readline_INCLUDE_DIR})
set(CMAKE_REQUIRED_LIBRARIES ${Readline_LIBRARY})

include(CheckFunctionExists)
check_function_exists(rl_copy_text HAVE_COPY_TEXT)
check_function_exists(rl_filename_completion_function HAVE_COMPLETION_FUNCTION)

if(NOT HAVE_COMPLETION_FUNCTION)
  if (Readline_LIBRARY)
    set(CMAKE_REQUIRED_LIBRARIES ${Readline_LIBRARY} ${Termcap_LIBRARY})
  endif(Readline_LIBRARY)
  check_function_exists(rl_copy_text HAVE_COPY_TEXT_TC)
  check_function_exists(rl_filename_completion_function HAVE_COMPLETION_FUNCTION_TC)
  set(HAVE_COMPLETION_FUNCTION ${HAVE_COMPLETION_FUNCTION_TC})
  set(HAVE_COPY_TEXT ${HAVE_COPY_TEXT_TC})
  if(HAVE_COMPLETION_FUNCTION)
    set(Readline_LIBRARY ${Readline_LIBRARY} ${Termcap_LIBRARY})
  endif(HAVE_COMPLETION_FUNCTION)
endif(NOT HAVE_COMPLETION_FUNCTION)

set(LIBEDIT_LIBRARY "")
set(GNU_READLINE_LIBRARY "")

if(HAVE_COMPLETION_FUNCTION AND HAVE_COPY_TEXT)
  set(GNU_READLINE_FOUND TRUE)
  set(GNU_READLINE_LIBRARY ${Readline_LIBRARY})
elseif(READLINE_FOUND AND NOT HAVE_COPY_TEXT)
  set(LIBEDIT_FOUND TRUE)
  set(LIBEDIT_LIBRARY ${Readline_LIBRARY})
endif(HAVE_COMPLETION_FUNCTION AND HAVE_COPY_TEXT)

