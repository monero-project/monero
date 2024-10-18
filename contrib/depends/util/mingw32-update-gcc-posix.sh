#!/bin/sh -e

# Sets mingw cross compiler to POSIX version.

TOOLCHAIN="$1"

if [ -z "$TOOLCHAIN" ]; then
	echo "Please supply the toolchain as the first argument, for example:"
	echo "x86_64-w64-mingw32"
	echo "i686-w64-mingw32"
	exit 1
fi

if (! which "${TOOLCHAIN}"-g++-posix); then
	echo "Toolchain '${TOOLCHAIN}' is not installed!"
	exit 1
fi

update-alternatives --set "${TOOLCHAIN}"-g++ $(which "${TOOLCHAIN}"-g++-posix)
update-alternatives --set "${TOOLCHAIN}"-gcc $(which "${TOOLCHAIN}"-gcc-posix)

echo "Toolchain '$TOOLCHAIN' is set to POSIX successfuly."
