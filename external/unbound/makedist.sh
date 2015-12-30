#!/bin/sh

# Build unbound distribution tar from the SVN repository.
# 
# Copyright (c) 2007, NLnet Labs. All rights reserved.
# 
# This software is open source.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
# Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
# 
# Neither the name of the NLNET LABS nor the names of its contributors may
# be used to endorse or promote products derived from this software without
# specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Abort script on unexpected errors.
set -e

# Remember the current working directory.
cwd=`pwd`

# Utility functions.
usage () {
    cat >&2 <<EOF
Usage $0: [-h] [-s] [-d SVN_root] [-w ...args...]
Generate a distribution tar file for unbound.

    -h           This usage information.
    -s           Build a snapshot distribution file.  The current date is
                 automatically appended to the current unbound version number.
    -rc <nr>     Build a release candidate, the given string will be added
                 to the version number 
                 (which will then be unbound-<version>rc<number>)
    -d SVN_root  Retrieve the unbound source from the specified repository.
                 Detected from svn working copy if not specified.
    -wssl openssl.xx.tar.gz Also build openssl from tarball for windows dist.
    -wxp expat.xx.tar.gz Also build expat from tarball for windows dist.
    -w ...       Build windows binary dist. last args passed to configure.
EOF
    exit 1
}

info () {
    echo "$0: info: $1"
}

error () {
    echo "$0: error: $1" >&2
    exit 1
}

question () {
    printf "%s (y/n) " "$*"
    read answer
    case "$answer" in
        [Yy]|[Yy][Ee][Ss])
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

# Only use cleanup and error_cleanup after generating the temporary
# working directory.
cleanup () {
    info "Deleting temporary working directory."
    cd $cwd && rm -rf $temp_dir
}

error_cleanup () {
    echo "$0: error: $1" >&2
    cleanup
    exit 1
}

replace_text () {
    (cp "$1" "$1".orig && \
        sed -e "s/$2/$3/g" < "$1".orig > "$1" && \
        rm "$1".orig) || error_cleanup "Replacement for $1 failed."
}

replace_all () {
    info "Updating '$1' with the version number."
    replace_text "$1" "@version@" "$version"
    info "Updating '$1' with today's date."
    replace_text "$1" "@date@" "`date +'%b %e, %Y'`"
}

replace_version () {
    local v1=`echo $2 | sed -e 's/^.*\..*\.//'`
    local v2=`echo $3 | sed -e 's/^.*\..*\.//'`
    replace_text "$1" "VERSION_MICRO\],\[$v1" "VERSION_MICRO\],\[$v2"
}
    
check_svn_root () {
    # Check if SVNROOT is specified.
    if [ -z "$SVNROOT" ]; then
	if svn info 2>&1 | grep "not a working copy" >/dev/null; then
		if test -z "$SVNROOT"; then
		    error "SVNROOT must be specified (using -d)"
		fi
	else
	     eval `svn info | grep 'URL:' | sed -e 's/URL: /url=/' | head -1`
	     SVNROOT="$url"
	fi
    fi
}

create_temp_dir () {
    # Creating temp directory
    info "Creating temporary working directory"
    temp_dir=`mktemp -d unbound-dist-XXXXXX`
    info "Directory '$temp_dir' created."
    cd $temp_dir
}

# pass filename as $1 arg.
# creates file.sha1 and file.sha256
storehash () {
    case $OSTYPE in
        linux*)
                sha=`sha1sum $1 |  awk '{ print $1 }'`
                sha256=`sha256sum $1 |  awk '{ print $1 }'`
                ;;
        freebsd*)
                sha=`sha1 $1 |  awk '{ print $5 }'`
                sha256=`sha256 $1 |  awk '{ print $5 }'`
                ;;
	*)
		# in case $OSTYPE is gone.
		case `uname` in
		Linux*)
		  sha=`sha1sum $1 |  awk '{ print $1 }'`
		  sha256=`sha256sum $1 |  awk '{ print $1 }'`
		  ;;
		FreeBSD*)
		  sha=`sha1 $1 |  awk '{ print $5 }'`
		  sha256=`sha256 $1 |  awk '{ print $5 }'`
		  ;;
		*)
		  sha=`sha1sum $1 |  awk '{ print $1 }'`
		  sha256=`sha256sum $1 |  awk '{ print $1 }'`
		  ;;
		esac
                ;;
    esac
    echo $sha > $1.sha1
    echo $sha256 > $1.sha256
    echo "hash of $1.{sha1,sha256}"
    echo "sha1 $sha"
    echo "sha256 $sha256"
}


SNAPSHOT="no"
RC="no"
DOWIN="no"
WINSSL=""
WINEXPAT=""

# Parse the command line arguments.
while [ "$1" ]; do
    case "$1" in
        "-h")
            usage
            ;;
        "-d")
            SVNROOT="$2"
            shift
            ;;
        "-s")
            SNAPSHOT="yes"
            ;;
        "-wssl")
	    WINSSL="$2"
	    shift
	    ;;
        "-wxp")
	    WINEXPAT="$2"
	    shift
	    ;;
        "-w")
            DOWIN="yes"
	    shift
	    break
            ;;
        "-rc")
            RC="$2"
            shift
            ;;
        *)
            error "Unrecognized argument -- $1"
            ;;
    esac
    shift
done

if [ "$DOWIN" = "yes" ]; then
    # detect crosscompile, from Fedora13 at this point.
    if test "`uname`" = "Linux"; then 
	info "Crosscompile windows dist"
        cross="yes"
	configure="mingw32-configure"
	strip="i686-w64-mingw32-strip"
	makensis="makensis"	# from mingw32-nsis package
	# flags for crosscompiled dependency libraries
	cross_flag=""

	check_svn_root
	create_temp_dir

	# crosscompile openssl for windows.
	if test -n "$WINSSL"; then
		info "Cross compile $WINSSL"
		info "winssl tar unpack"
		(cd ..; gzip -cd $WINSSL) | tar xf - || error_cleanup "tar unpack of $WINSSL failed"
		sslinstall="`pwd`/sslinstall"
		cd openssl-* || error_cleanup "no openssl-X dir in tarball"
		# configure for crosscompile, without CAPI because it fails
		# cross-compilation and it is not used anyway
		# before 1.0.1i need --cross-compile-prefix=i686-w64-mingw32-
		sslflags="no-asm -DOPENSSL_NO_CAPIENG mingw"
		info "winssl: Configure $sslflags"
		CC=i686-w64-mingw32-gcc AR=i686-w64-mingw32-ar RANLIB=i686-w64-mingw32-ranlib ./Configure --prefix="$sslinstall" $sslflags || error_cleanup "OpenSSL Configure failed"
		info "winssl: make"
		make || error_cleanup "OpenSSL crosscompile failed"
		# only install sw not docs, which take a long time.
		info "winssl: make install_sw"
		make install_sw || error_cleanup "OpenSSL install failed"
		cross_flag="$cross_flag --with-ssl=$sslinstall"
		cd ..
	fi

	if test -n "$WINEXPAT"; then
		info "Cross compile $WINEXPAT"
		info "wxp: tar unpack"
		(cd ..; gzip -cd $WINEXPAT) | tar xf - || error_cleanup "tar unpack of $WINEXPAT failed"
		wxpinstall="`pwd`/wxpinstall"
		cd expat-* || error_cleanup "no expat-X dir in tarball"
		info "wxp: configure"
		mingw32-configure --prefix="$wxpinstall" --exec-prefix="$wxpinstall" --bindir="$wxpinstall/bin" --includedir="$wxpinstall/include" --mandir="$wxpinstall/man" --libdir="$wxpinstall/lib"  || error_cleanup "libexpat configure failed"
		#info "wxp: make"
		#make || error_cleanup "libexpat crosscompile failed"
		info "wxp: make installlib"
		make installlib || error_cleanup "libexpat install failed"
		cross_flag="$cross_flag --with-libexpat=$wxpinstall"
		cd ..
	fi

        info "SVNROOT  is $SVNROOT"
	info "Exporting source from SVN."
	svn export "$SVNROOT" unbound || error_cleanup "SVN command failed"
	cd unbound || error_cleanup "Unbound not exported correctly from SVN"

	# on a re-configure the cache may no longer be valid...
	if test -f mingw32-config.cache; then rm mingw32-config.cache; fi
    else 
	cross="no"	# mingw and msys
	cross_flag=""
	configure="./configure"
	strip="strip"
	makensis="c:/Program Files/NSIS/makensis.exe" # http://nsis.sf.net
    fi

    # version gets compiled into source, edit the configure to set  it
    version=`./configure --version | head -1 | awk '{ print $3 }'` \
	|| error_cleanup "Cannot determine version number."
    if [ "$RC" != "no" -o "$SNAPSHOT" != "no" ]; then
    	if [ "$RC" != "no" ]; then
    		version2=`echo $version | sed -e 's/rc.*$//' -e 's/_20.*$//'`
		version2=`echo $version2 | sed -e 's/rc.*//'`"rc$RC"
	fi
    	if [ "$SNAPSHOT" != "no" ]; then
    		version2=`echo $version | sed -e 's/rc.*$//' -e 's/_20.*$//'`
    		version2="${version2}_`date +%Y%m%d`"
	fi
	replace_version "configure.ac" "$version" "$version2"
    	version="$version2"
    	info "Rebuilding configure script (autoconf) snapshot."
    	autoconf || error_cleanup "Autoconf failed."
    	autoheader || error_cleanup "Autoheader failed."
    	rm -r autom4te* || echo "ignored"
    fi

    # procedure for making unbound installer on mingw. 
    info "Creating windows dist unbound $version"
    info "Calling configure"
    echo "$configure"' --enable-debug --enable-static-exe '"$* $cross_flag"
    $configure --enable-debug --enable-static-exe $* $cross_flag \
	|| error_cleanup "Could not configure"
    info "Calling make"
    make || error_cleanup "Could not make"
    info "Make complete"

    info "Unbound version: $version"
    file="unbound-$version.zip"
    rm -f $file
    info "Creating $file"
    mkdir tmp.$$
    $strip unbound.exe
    $strip anchor-update.exe
    $strip unbound-control.exe
    $strip unbound-host.exe
    $strip unbound-anchor.exe
    $strip unbound-checkconf.exe
    $strip unbound-service-install.exe
    $strip unbound-service-remove.exe
    cd tmp.$$
    cp ../doc/example.conf ../doc/Changelog .
    cp ../unbound.exe ../unbound-anchor.exe ../unbound-host.exe ../unbound-control.exe ../unbound-checkconf.exe ../unbound-service-install.exe ../unbound-service-remove.exe ../LICENSE ../winrc/unbound-control-setup.cmd ../winrc/unbound-website.url ../winrc/service.conf ../winrc/README.txt ../contrib/create_unbound_ad_servers.cmd ../contrib/warmup.cmd ../contrib/unbound_cache.cmd .
    # zipfile
    zip ../$file LICENSE README.txt unbound.exe unbound-anchor.exe unbound-host.exe unbound-control.exe unbound-checkconf.exe unbound-service-install.exe unbound-service-remove.exe unbound-control-setup.cmd example.conf service.conf unbound-website.url create_unbound_ad_servers.cmd warmup.cmd unbound_cache.cmd Changelog
    info "Testing $file"
    (cd .. ; zip -T $file )
    # installer
    info "Creating installer"
    quadversion=`cat ../config.h | grep RSRC_PACKAGE_VERSION | sed -e 's/#define RSRC_PACKAGE_VERSION //' -e 's/,/\\./g'`
    cat ../winrc/setup.nsi | sed -e 's/define VERSION.*$/define VERSION "'$version'"/' -e 's/define QUADVERSION.*$/define QUADVERSION "'$quadversion'"/' > ../winrc/setup_ed.nsi
    "$makensis" ../winrc/setup_ed.nsi
    info "Created installer"
    cd ..
    rm -rf tmp.$$
    mv winrc/unbound_setup_$version.exe .
    if test "$cross" = "yes"; then
	    mv unbound_setup_$version.exe $cwd/.
	    mv unbound-$version.zip $cwd/.
	    cleanup
    fi
    storehash unbound_setup_$version.exe
    storehash unbound-$version.zip
    ls -lG unbound_setup_$version.exe
    ls -lG unbound-$version.zip
    info "Done"
    exit 0
fi

check_svn_root

# Start the packaging process.
info "SVNROOT  is $SVNROOT"
info "SNAPSHOT is $SNAPSHOT"

#question "Do you wish to continue with these settings?" || error "User abort."

create_temp_dir

info "Exporting source from SVN."
svn export "$SVNROOT" unbound || error_cleanup "SVN command failed"

cd unbound || error_cleanup "Unbound not exported correctly from SVN"

info "Adding libtool utils (libtoolize)."
libtoolize -c --install || libtoolize -c || error_cleanup "Libtoolize failed."

info "Building configure script (autoreconf)."
autoreconf || error_cleanup "Autoconf failed."

rm -r autom4te* || error_cleanup "Failed to remove autoconf cache directory."

info "Building lexer and parser."
echo "#include \"config.h\"" > util/configlexer.c || error_cleanup "Failed to create configlexer"
echo "#include \"util/configyyrename.h\"" >> util/configlexer.c || error_cleanup "Failed to create configlexer"
flex -i -t util/configlexer.lex >> util/configlexer.c  || error_cleanup "Failed to create configlexer"
if test -x `which bison` 2>&1; then YACC=bison; else YACC=yacc; fi
$YACC -y -d -o util/configparser.c util/configparser.y || error_cleanup "Failed to create configparser"

find . -name .c-mode-rc.el -exec rm {} \;
find . -name .cvsignore -exec rm {} \;
rm makedist.sh || error_cleanup "Failed to remove makedist.sh."

info "Determining Unbound version."
version=`./configure --version | head -1 | awk '{ print $3 }'` || \
    error_cleanup "Cannot determine version number."

info "Unbound version: $version"

RECONFIGURE="no"

if [ "$RC" != "no" ]; then
    info "Building Unbound release candidate $RC."
    version2="${version}rc$RC"
    info "Version number: $version2"

    replace_version "configure.ac" "$version" "$version2"
    version="$version2"
    RECONFIGURE="yes"
fi

if [ "$SNAPSHOT" = "yes" ]; then
    info "Building Unbound snapshot."
    version2="${version}_`date +%Y%m%d`"
    info "Snapshot version number: $version2"

    replace_version "configure.ac" "$version" "$version2"
    version="$version2"
    RECONFIGURE="yes"
fi

if [ "$RECONFIGURE" = "yes" ]; then
    info "Rebuilding configure script (autoconf) snapshot."
    autoreconf || error_cleanup "Autoconf failed."
    rm -r autom4te* || error_cleanup "Failed to remove autoconf cache directory."
fi

replace_all doc/README
replace_all doc/unbound.8.in
replace_all doc/unbound.conf.5.in
replace_all doc/unbound-checkconf.8.in
replace_all doc/unbound-control.8.in
replace_all doc/unbound-anchor.8.in
replace_all doc/unbound-host.1.in
replace_all doc/example.conf.in
replace_all doc/libunbound.3.in

info "Renaming Unbound directory to unbound-$version."
cd ..
mv unbound unbound-$version || error_cleanup "Failed to rename unbound directory."

tarfile="../unbound-$version.tar.gz"

if [ -f $tarfile ]; then
    (question "The file $tarfile already exists.  Overwrite?" \
        && rm -f $tarfile) || error_cleanup "User abort."
fi

info "Creating tar unbound-$version.tar.gz"
tar czf ../unbound-$version.tar.gz unbound-$version || error_cleanup "Failed to create tar file."

cleanup

storehash unbound-$version.tar.gz
echo "create unbound-$version.tar.gz.asc with:"
echo "  gpg --armor --detach-sign unbound-$version.tar.gz"
echo "  gpg --armor --detach-sign unbound-$version.zip"
echo "  gpg --armor --detach-sign unbound_setup_$version.exe"

info "Unbound distribution created successfully."

