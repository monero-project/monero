#!/bin/bash -e
# Format all sources with specific version of clang-format
#
# --check parameter can be used to check if the sources are
# properly formatted without modifying them. This is useful
# in a CI environment.
#
# Since clang-format can be installed either as clang-format
# or clang-format-4.0, we need to figure out which the current
# system has.

CLANG_FORMAT=$(which clang-format-4.0)

# if clang-format-4.0 was not found, use regular clang-format.
if [ -z $CLANG_FORMAT ]; then
  CLANG_FORMAT=$(which clang-format)
fi

if [ -z $CLANG_FORMAT ]; then
  echo "Unable to find clang-format or clang-format-4.0!"
  exit 1
fi

if ! [[ $($CLANG_FORMAT --version 2>&1) =~ "clang-format version 4.0" ]]; then
  echo "clang-format version 4.0 not found!"
  exit 1
fi

if [ "$1" == "--check" ]; then
  ARGUMENTS="-output-replacements-xml"
fi

# Run clang-format for all .cpp and .h files found in subdirectories.
# Excluding 3rd party files.
#
# If --check was passed, then clang is instructed to print all the changes it *would* do.
# That output is grepped of replacement tags to see if there is anything to change.
# If something was found, an error will be returned to indicate that sources are not
# properly formatted.

! find -L . -iregex '.*\.[ch]\(pp\)?' \
       -not -iregex '.*contrib.*' \
       -not -iregex '.*external.*' \
       | xargs -I filename $CLANG_FORMAT $ARGUMENTS --style=file -i filename \
       | grep "</replacement>"
exit $?

