#!/bin/bash -e

VER_DOTS=$1

if [ -z $VER_DOTS ]; then
	echo "Please supply the boost version with dots as the argument, for example:"
	echo "1.58.0"
	echo "1.76.0"
	exit 1
fi

TMP="build/boost"
DEST="/usr/local"
#TMP="/tmp/build/boost" # For local testing
#DEST="/tmp/local"

VER_UNDR=${VER_DOTS//./_} # Replaces . with _
WGET_OPTS="--timeout=15 --waitretry=5 --tries=5"
PROC=3
BOOST_NAME="boost_${VER_UNDR}"
BOOST_ARCHIVE="$BOOST_NAME.tar.bz2"

mkdir -pv "$TMP" && cd "$TMP"

BOOST_DIR="$(pwd)/$BOOST_NAME"

# TODO: verify signatures!

if [ ! -f "$BOOST_ARCHIVE" ]; then
	if (! wget $WGET_OPTS https://boostorg.jfrog.io/artifactory/main/release/${VER_DOTS}/source/$BOOST_ARCHIVE); then
		# Perhaps the file is on sourceforge?
		wget $WGET_OPTS http://downloads.sourceforge.net/project/boost/boost/${VER_DOTS}/$BOOST_ARCHIVE?use_mirror=autoselect -O "$BOOST_ARCHIVE"
	fi
else
	echo "$BOOST_ARCHIVE already downloaded."
fi

if [ ! -d "$BOOST_NAME" ]; then
	echo "Extracting $BOOST_ARCHIVE ..."
	tar -xf "$BOOST_ARCHIVE"
else
	echo "$BOOST_ARCHIVE already extracted."
fi


export PATH=/usr/lib/ccache:$PATH
export CFLAGS="-std=c++11 -fPIC"
export CXXFLAGS=$CFLAGS

cd $BOOST_NAME/tools/build
./bootstrap.sh #--with-libraries=$boost_config_libraries # --with-libraries= is not backward compatible
./b2 install --prefix=b2dir
./b2 ../bcp
cd ../..

B2_CALL="tools/build/b2dir/bin/b2 \
threading=multi runtime-link=shared --link=shared --layout=system --build-type=minimal -j $PROC \
--with-program_options \
--with-serialization \
--with-filesystem \
--with-system \
--with-date_time \
--with-thread \
--with-chrono \
--with-regex \
--with-test \
--with-locale"

# Library list taken from:
# contrib/depends/packages/boost.mk
# --with-LIBRARY is backward compatible

$B2_CALL      --prefix="${DEST}" --build-dir="${TMP}/boost-build" stage
echo "Installing boost:"
sudo $B2_CALL --prefix="${DEST}" --build-dir="${TMP}/boost-build" install

