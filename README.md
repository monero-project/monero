# Loki

<p align="center">
    <a href="https://github.com/loki-project/loki/commits/dev"><img alt="pipeline status" src="https://gitlab.com/lokiproject/loki/badges/dev/pipeline.svg" /></a>
</p>

Copyright (c) 2018 The Loki Project.   
Portions Copyright (c) 2014-2019 The Monero Project.   
Portions Copyright (c) 2012-2013 The Cryptonote developers.

## Development resources

- Web: [loki.network](https://loki.network)
- Telegram: [t.me/LokiCommunity](https://t.me/LokiCommunity)
- Mail: [team@loki.network](mailto:team@loki.network)
- GitHub: [https://github.com/loki-project/loki](https://github.com/loki-project/loki)
- Discord: [https://discord.gg/67GXfD6](https://discord.gg/67GXfD6)

## Vulnerability disclosure

- Check out our [Vulnerability Response Process](https://loki-project.github.io/loki-docs/Contributing/VULNERABILITY_RESPONSE_LOKI), encourages prompt disclosure of any Vulnerabilities

## Information

Loki is a private cryptocurrency based on Monero. Loki currently offers an incentivised full node layer, over the coming months we will be looking to support a secondary p2p network (Lokinet) and a messenger that offers private communications based on the Signal protocol (Loki Messenger).

More information on the project can be found on the website and in the whitepaper.

Loki is an open source project, and we encourage contributions from anyone with something to offer. For more information on contributing, please contact team@loki.network

## Compiling Loki from source

### Dependencies

The following table summarizes the tools and libraries required to build. A
few of the libraries are also included in this repository (marked as
"Vendored"). By default, the build uses the library installed on the system,
and ignores the vendored sources. However, if no library is found installed on
the system, then the vendored source will be built and used. The vendored
sources are also used for statically-linked builds because distribution
packages often include only shared library binaries (`.so`) but not static
library archives (`.a`).

| Dep          | Min. version  | Vendored | Debian/Ubuntu pkg  | Arch pkg     | Fedora            | Optional | Purpose        |
| ------------ | ------------- | -------- | ------------------ | ------------ | ----------------- | -------- | -------------- |
| GCC          | 4.7.3         | NO       | `build-essential`  | `base-devel` | `gcc`             | NO       |                |
| CMake        | 3.5           | NO       | `cmake`            | `cmake`      | `cmake`           | NO       |                |
| pkg-config   | any           | NO       | `pkg-config`       | `base-devel` | `pkgconf`         | NO       |                |
| Boost        | 1.58          | NO       | `libboost-all-dev` | `boost`      | `boost-devel`     | NO       | C++ libraries  |
| OpenSSL      | basically any | NO       | `libssl-dev`       | `openssl`    | `openssl-devel`   | NO       | sha256 sum     |
| libzmq       | 3.0.0         | NO       | `libzmq3-dev`      | `zeromq`     | `cppzmq-devel`    | NO       | ZeroMQ library |
| OpenPGM      | ?             | NO       | `libpgm-dev`       | `libpgm`     | `openpgm-devel`   | NO       | For ZeroMQ     |
| libnorm[2]   | ?             | NO       | `libnorm-dev`      |              |               `   | YES      | For ZeroMQ     |
| libunbound   | 1.4.16        | YES      | `libunbound-dev`   | `unbound`    | `unbound-devel`   | NO       | DNS resolver   |
| libsodium    | ?             | NO       | `libsodium-dev`    | `libsodium`  | `libsodium-devel` | NO       | cryptography   |
| libunwind    | any           | NO       | `libunwind8-dev`   | `libunwind`  | `libunwind-devel` | YES      | Stack traces   |
| liblzma      | any           | NO       | `liblzma-dev`      | `xz`         | `xz-devel`        | YES      | For libunwind  |
| libreadline  | 6.3.0         | NO       | `libreadline6-dev` | `readline`   | `readline-devel`  | YES      | Input editing  |
| ldns         | 1.6.17        | NO       | `libldns-dev`      | `ldns`       | `ldns-devel`      | YES      | SSL toolkit    |
| expat        | 1.1           | NO       | `libexpat1-dev`    | `expat`      | `expat-devel`     | YES      | XML parsing    |
| GTest        | 1.5           | YES      | `libgtest-dev`[1]  | `gtest`      | `gtest-devel`     | YES      | Test suite     |
| Doxygen      | any           | NO       | `doxygen`          | `doxygen`    | `doxygen`         | YES      | Documentation  |
| Graphviz     | any           | NO       | `graphviz`         | `graphviz`   | `graphviz`        | YES      | Documentation  |


[1] On Debian/Ubuntu `libgtest-dev` only includes sources and headers. You must
build the library binary manually. This can be done with the following command ```sudo apt-get install libgtest-dev && cd /usr/src/gtest && sudo cmake . && sudo make && sudo mv libg* /usr/lib/ ```
[2] libnorm-dev is needed if your zmq library was built with libnorm, and not needed otherwise

Install all dependencies at once on Debian/Ubuntu:

``` sudo apt update && sudo apt install build-essential cmake pkg-config libboost-all-dev libssl-dev libzmq3-dev libunbound-dev libsodium-dev libunwind8-dev liblzma-dev libreadline6-dev libldns-dev libexpat1-dev doxygen graphviz libpgm-dev```

Install all dependencies at once on macOS with the provided Brewfile:
``` brew update && brew bundle --file=contrib/brew/Brewfile ```

FreeBSD one liner for required to build dependencies
```pkg install git gmake cmake pkgconf boost-libs cppzmq libsodium```

### Cloning the repository

Clone recursively to pull-in needed submodule(s):

`$ git clone --recursive https://github.com/loki-project/loki`

If you already have a repo cloned, initialize and update:

`$ cd loki && git submodule init && git submodule update`

### Build instructions

Loki uses the CMake build system and a top-level [Makefile](Makefile) that
invokes cmake commands as needed.

#### On Linux and macOS

* Install the dependencies
* Change to the root of the source code directory, change to the most recent release branch, and build:

    ```bash
    cd loki
    git checkout master
    make
    ```

    *Optional*: If your machine has several cores and enough memory, enable
    parallel build by running `make -j<number of threads>` instead of `make`. For
    this to be worthwhile, the machine should have one core and about 2GB of RAM
    available per thread.

    *Note*: If cmake can not find zmq.hpp file on macOS, installing `zmq.hpp` from
    https://github.com/zeromq/cppzmq to `/usr/local/include` should fix that error.

    *Note*: The instructions above will compile the most stable release of the
    Loki software. If you would like to use and test the most recent software,
    use ```git checkout master```. The master branch may contain updates that are
    both unstable and incompatible with release software, though testing is always
    encouraged.

* The resulting executables can be found in `build/release/bin`

* Add `PATH="$PATH:$HOME/loki/build/release/bin"` to `.profile`

* Run Loki with `lokid --detach`

* **Optional**: build and run the test suite to verify the binaries:

    ```bash
    make release-test
    ```

    *NOTE*: `core_tests` test may take a few hours to complete.

* **Optional**: to build binaries suitable for debugging:

    ```bash
    make debug
    ```

* **Optional**: to build statically-linked binaries:

    ```bash
    make release-static
    ```

Dependencies need to be built with -fPIC. Static libraries usually aren't, so you may have to build them yourself with -fPIC. Refer to their documentation for how to build them.

* **Optional**: build documentation in `doc/html` (omit `HAVE_DOT=YES` if `graphviz` is not installed):

    ```bash
    HAVE_DOT=YES doxygen Doxyfile
    ```

#### On the Raspberry Pi

Tested on a Raspberry Pi Zero with a clean install of minimal Raspbian Stretch (2017-09-07 or later) from https://www.raspberrypi.org/downloads/raspbian/. If you are using Raspian Jessie, [please see note in the following section](#note-for-raspbian-jessie-users).

* `apt-get update && apt-get upgrade` to install all of the latest software

* Install the dependencies for Loki from the 'Debian' column in the table above.

* Increase the system swap size:

    ```bash
    sudo /etc/init.d/dphys-swapfile stop  
    sudo nano /etc/dphys-swapfile  
    CONF_SWAPSIZE=2048
    sudo /etc/init.d/dphys-swapfile start
    ```

* If using an external hard disk without an external power supply, ensure it gets enough power to avoid hardware issues when syncing, by adding the line "max_usb_current=1" to /boot/config.txt

* Clone Loki and checkout the most recent release version:

    ```bash
    git clone https://github.com/loki-project/loki.git
    cd loki
    git checkout master
    ```

* Build:

    ```bash
    make release
    ```

* Wait 4-6 hours

* The resulting executables can be found in `build/release/bin`

* Add `PATH="$PATH:$HOME/loki/build/release/bin"` to `.profile`

* Run Loki with `lokid --detach`

* You may wish to reduce the size of the swap file after the build has finished, and delete the boost directory from your home directory

#### *Note for Raspbian Jessie users:*

If you are using the older Raspbian Jessie image, compiling Loki is a bit more complicated. The version of Boost available in the Debian Jessie repositories is too old to use with Loki, and thus you must compile a newer version yourself. The following explains the extra steps, and has been tested on a Raspberry Pi 2 with a clean install of minimal Raspbian Jessie.

* As before, `apt-get update && apt-get upgrade` to install all of the latest software, and increase the system swap size

    ```bash
    sudo /etc/init.d/dphys-swapfile stop
    sudo nano /etc/dphys-swapfile
    CONF_SWAPSIZE=2048
    sudo /etc/init.d/dphys-swapfile start
    ```


* Then, install the dependencies for Loki except `libunwind` and `libboost-all-dev`

* Install the latest version of boost (this may first require invoking `apt-get remove --purge libboost*` to remove a previous version if you're not using a clean install):

    ```bash
    cd
    wget https://sourceforge.net/projects/boost/files/boost/1.64.0/boost_1_64_0.tar.bz2
    tar xvfo boost_1_64_0.tar.bz2
    cd boost_1_64_0
    ./bootstrap.sh
    sudo ./b2
    ```

* Wait ~8 hours

    ```bash    
    sudo ./bjam cxxflags=-fPIC cflags=-fPIC -a install
    ```

* Wait ~4 hours

* From here, follow the [general Raspberry Pi instructions](#on-the-raspberry-pi) from the "Clone loki and checkout most recent release version" step.

#### On Windows:

Binaries for Windows are built on Windows using the MinGW toolchain within
[MSYS2 environment](https://www.msys2.org). The MSYS2 environment emulates a
POSIX system. The toolchain runs within the environment and *cross-compiles*
binaries that can run outside of the environment as a regular Windows
application.

**Preparing the build environment**

* Download and install the [MSYS2 installer](https://www.msys2.org), either the 64-bit (x86_64) or the 32-bit (i686) package, depending on your system.
* Note: Installation must be on the C drive and root directory as result of [Monero issue 3167](https://github.com/monero-project/monero/issues/3167).
* Open the MSYS shell via the `MSYS2 MSYS` shortcut in the Start Menu or "C:\msys64\msys2_shell.cmd -msys"
* Update packages using pacman:  

    ```bash
    pacman -Syu
    ```

* Exit the MSYS shell using Alt+F4 when you get a warning stating: "terminate MSYS2 without returning to shell and check for updates again/for example close your terminal window instead of calling exit"

    ```bash
    pacman -Syu
    ```

* Update packages again using pacman: 

        pacman -Syu  

* Install dependencies:

    To build for 64-bit Windows:

    ```bash
    pacman -S mingw-w64-x86_64-toolchain make mingw-w64-x86_64-cmake mingw-w64-x86_64-boost mingw-w64-x86_64-openssl mingw-w64-x86_64-zeromq mingw-w64-x86_64-libsodium mingw-w64-x86_64-hidapi
    ```

    To build for 32-bit Windows:

    ```bash
    pacman -S mingw-w64-i686-toolchain make mingw-w64-i686-cmake mingw-w64-i686-boost mingw-w64-i686-openssl mingw-w64-i686-zeromq mingw-w64-i686-libsodium mingw-w64-i686-hidapi
    ```

* Close and reopen the MSYS MinGW shell via `MSYS2 MinGW 64-bit` shortcut on
  64-bit Windows or `MSYS2 MinGW 32-bit` shortcut on 32-bit Windows. Note 
  that if you are running 64-bit Windows, you will have both 64-bit and
  32-bit MinGW shells.

**Cloning**

* To git clone, run:

    ```bash
    git clone --recursive https://github.com/loki-project/loki.git
    ```

**Building**

* Change to the cloned directory, run:
	
    ```bash
    cd loki
    ```

* If you would like a specific [version/tag](https://github.com/loki-project/loki/tags), do a git checkout for that version. eg. 'v3.0.6'. If you don't care about the version and just want binaries from master, skip this step:
	
    ```bash
    git checkout v3.0.6
    ```

* If you are on a 64-bit system, run:

    ```bash
    make release-static-win64
    ```

* If you are on a 32-bit system, run:

    ```bash
    make release-static-win32
    ```

* The resulting executables can be found in `build/<MinGW version>/<loki version>/release/bin`

* **Optional**: to build Windows binaries suitable for debugging on a 64-bit system, run:

    ```bash
    make debug-static-win64
    ```

* **Optional**: to build Windows binaries suitable for debugging on a 32-bit system, run:

    ```bash
    make debug-static-win32
    ```

* The resulting executables can be found in `build/<MinGW version>/<loki version>/debug/bin`

### On FreeBSD:

The project can be built from scratch by following instructions for Linux above(but use `gmake` instead of `make`). If you are running loki in a jail you need to add the flag: `allow.sysvipc=1` to your jail configuration, otherwise lmdb will throw the error message: `Failed to open lmdb environment: Function not implemented`.

We expect to add Loki into the ports tree in the near future, which will aid in managing installations using ports or packages.

### On OpenBSD:

#### OpenBSD < 6.2

This has been tested on OpenBSD 5.8.

You will need to add a few packages to your system. `pkg_add db cmake gcc gcc-libs g++ gtest`.

The doxygen and graphviz packages are optional and require the xbase set.

The Boost package has a bug that will prevent librpc.a from building correctly. In order to fix this, you will have to Build boost yourself from scratch. Follow the directions here (under "Building Boost"):
https://github.com/bitcoin/bitcoin/blob/master/doc/build-openbsd.md

You will have to add the serialization, date_time, and regex modules to Boost when building as they are needed by Loki.

To build: `env CC=egcc CXX=eg++ CPP=ecpp DEVELOPER_LOCAL_TOOLS=1 BOOST_ROOT=/path/to/the/boost/you/built make release-static-64`

#### OpenBSD 6.2 and 6.3

You will need to add a few packages to your system. `pkg_add cmake zeromq libiconv`.

The doxygen and graphviz packages are optional and require the xbase set.


Build the Boost library using clang. This guide is derived from: https://github.com/bitcoin/bitcoin/blob/master/doc/build-openbsd.md

We assume you are compiling with a non-root user and you have `doas` enabled.

Note: do not use the boost package provided by OpenBSD, as we are installing boost to `/usr/local`.

```bash
# Create boost building directory
mkdir ~/boost
cd ~/boost

# Fetch boost source
ftp -o boost_1_64_0.tar.bz2 https://netcologne.dl.sourceforge.net/project/boost/boost/1.64.0/boost_1_64_0.tar.bz2

# MUST output: (SHA256) boost_1_64_0.tar.bz2: OK
echo "7bcc5caace97baa948931d712ea5f37038dbb1c5d89b43ad4def4ed7cb683332 boost_1_64_0.tar.bz2" | sha256 -c
tar xfj boost_1_64_0.tar.bz2

# Fetch and apply boost patches, required for OpenBSD
ftp -o boost_test_impl_execution_monitor_ipp.patch https://raw.githubusercontent.com/openbsd/ports/bee9e6df517077a7269ff0dfd57995f5c6a10379/devel/boost/patches/patch-boost_test_impl_execution_monitor_ipp
ftp -o boost_config_platform_bsd_hpp.patch https://raw.githubusercontent.com/openbsd/ports/90658284fb786f5a60dd9d6e8d14500c167bdaa0/devel/boost/patches/patch-boost_config_platform_bsd_hpp

# MUST output: (SHA256) boost_config_platform_bsd_hpp.patch: OK
echo "1f5e59d1154f16ee1e0cc169395f30d5e7d22a5bd9f86358f738b0ccaea5e51d boost_config_platform_bsd_hpp.patch" | sha256 -c
# MUST output: (SHA256) boost_test_impl_execution_monitor_ipp.patch: OK
echo "30cec182a1437d40c3e0bd9a866ab5ddc1400a56185b7e671bb3782634ed0206 boost_test_impl_execution_monitor_ipp.patch" | sha256 -c

cd boost_1_64_0
patch -p0 < ../boost_test_impl_execution_monitor_ipp.patch
patch -p0 < ../boost_config_platform_bsd_hpp.patch

# Start building boost
echo 'using clang : : c++ : <cxxflags>"-fvisibility=hidden -fPIC" <linkflags>"" <archiver>"ar" <striper>"strip"  <ranlib>"ranlib" <rc>"" : ;' > user-config.jam
./bootstrap.sh --without-icu --with-libraries=chrono,filesystem,program_options,system,thread,test,date_time,regex,serialization,locale --with-toolset=clang
./b2 toolset=clang cxxflags="-stdlib=libc++" linkflags="-stdlib=libc++" -sICONV_PATH=/usr/local
doas ./b2 -d0 runtime-link=shared threadapi=pthread threading=multi link=static variant=release --layout=tagged --build-type=complete --user-config=user-config.jam -sNO_BZIP2=1 -sICONV_PATH=/usr/local --prefix=/usr/local install
```

Build the cppzmq bindings.

We assume you are compiling with a non-root user and you have `doas` enabled.

```bash
# Create cppzmq building directory
mkdir ~/cppzmq
cd ~/cppzmq

# Fetch cppzmq source
ftp -o cppzmq-4.2.3.tar.gz https://github.com/zeromq/cppzmq/archive/v4.2.3.tar.gz

# MUST output: (SHA256) cppzmq-4.2.3.tar.gz: OK
echo "3e6b57bf49115f4ae893b1ff7848ead7267013087dc7be1ab27636a97144d373 cppzmq-4.2.3.tar.gz" | sha256 -c
tar xfz cppzmq-4.2.3.tar.gz

# Start building cppzmq
cd cppzmq-4.2.3
mkdir build
cd build
cmake ..
doas make install
```

Build Loki:
```bash
env DEVELOPER_LOCAL_TOOLS=1 BOOST_ROOT=/usr/local make release-static
```

#### OpenBSD >= 6.4

You will need to add a few packages to your system. `pkg_add cmake gmake zeromq cppzmq libiconv boost`.

The doxygen and graphviz packages are optional and require the xbase set.

Build loki: `env DEVELOPER_LOCAL_TOOLS=1 BOOST_ROOT=/usr/local gmake release-static`

Note: you may encounter the following error, when compiling the latest version of loki as a normal user:

```
LLVM ERROR: out of memory
c++: error: unable to execute command: Abort trap (core dumped)
```

Then you need to increase the data ulimit size to 2GB and try again: `ulimit -d 2000000`

### On Solaris:

The default Solaris linker can't be used, you have to install GNU ld, then run cmake manually with the path to your copy of GNU ld:

```bash
mkdir -p build/release
cd build/release
cmake -DCMAKE_LINKER=/path/to/ld -D CMAKE_BUILD_TYPE=Release ../..
cd ../..
```

Then you can run make as usual.

### On Linux for Android (using docker):

```bash
# Build image (for ARM 32-bit)
docker build -f utils/build_scripts/android32.Dockerfile -t loki-android .
# Build image (for ARM 64-bit)
docker build -f utils/build_scripts/android64.Dockerfile -t loki-android .
# Create container
docker create -it --name loki-android loki-android bash
# Get binaries
docker cp loki-android:/src/build/release/bin .
```

### Building portable statically linked binaries

By default, in either dynamically or statically linked builds, binaries target the specific host processor on which the build happens and are not portable to other processors. Portable binaries can be built using the following targets:

* ```make release-static-linux-x86_64``` builds binaries on Linux on x86_64 portable across POSIX systems on x86_64 processors
* ```make release-static-linux-i686``` builds binaries on Linux on x86_64 or i686 portable across POSIX systems on i686 processors
* ```make release-static-linux-armv8``` builds binaries on Linux portable across POSIX systems on armv8 processors
* ```make release-static-linux-armv7``` builds binaries on Linux portable across POSIX systems on armv7 processors
* ```make release-static-linux-armv6``` builds binaries on Linux portable across POSIX systems on armv6 processors
* ```make release-static-win64``` builds binaries on 64-bit Windows portable across 64-bit Windows systems
* ```make release-static-win32``` builds binaries on 64-bit or 32-bit Windows portable across 32-bit Windows systems

### Cross Compiling

You can also cross-compile static binaries on Linux for Windows and macOS with the `depends` system.

* ```make depends target=x86_64-linux-gnu``` for 64-bit linux binaries.
* ```make depends target=x86_64-w64-mingw32``` for 64-bit windows binaries.
  * Requires: `python3 g++-mingw-w64-x86-64 wine1.6 bc`
* ```make depends target=x86_64-apple-darwin11``` for macOS binaries.
  * Requires: `cmake imagemagick libcap-dev librsvg2-bin libz-dev libbz2-dev libtiff-tools python-dev`
* ```make depends target=i686-linux-gnu``` for 32-bit linux binaries.
  * Requires: `g++-multilib bc`
* ```make depends target=i686-w64-mingw32``` for 32-bit windows binaries.
  * Requires: `python3 g++-mingw-w64-i686`
* ```make depends target=arm-linux-gnueabihf``` for armv7 binaries.
  * Requires: `g++-arm-linux-gnueabihf`
* ```make depends target=aarch64-linux-gnu``` for armv8 binaries.
  * Requires: `g++-aarch64-linux-gnu`

The required packages are the names for each toolchain on apt. Depending on your distro, they may have different names.

Using `depends` might also be easier to compile Loki on Windows than using MSYS. Activate Windows Subsystem for Linux (WSL) with a distro (for example Ubuntu), install the apt build-essentials and follow the `depends` steps as depicted above.

The produced binaries still link libc dynamically. If the binary is compiled on a current distribution, it might not run on an older distribution with an older installation of libc. Passing `-DBACKCOMPAT=ON` to cmake will make sure that the binary will run on systems having at least libc version 2.17.

## Installing Loki from a package

**DISCLAIMER: These packages are not part of this repository or maintained by this project's contributors, and as such, do not go through the same review process to ensure their trustworthiness and security.**

Packages are available for

* Docker

    ```bash
    # Build using all available cores
    docker build -t loki-daemon-image .
    
    # or build using a specific number of cores (reduce RAM requirement)
    docker build --build-arg NPROC=1 -t loki .
    
    # either run in foreground
    docker run -it -v /loki/chain:/root/.loki -v /loki/wallet:/wallet -p 22022:22022 loki
    
    # or in background
    docker run -it -d -v /loki/chain:/root/.loki -v /loki/wallet:/wallet -p 22022:22022 loki
    ```

* The build needs 3 GB space.
* Wait one hour or more. For docker, the collect_from_docker_container.sh script will automate downloading the binaries from the docker container.

## Running lokid

The build places the binary in `bin/` sub-directory within the build directory
from which cmake was invoked (repository root by default). To run in
foreground:

```bash
./bin/lokid
```

To list all available options, run `./bin/lokid --help`.  Options can be
specified either on the command line or in a configuration file passed by the
`--config-file` argument.  To specify an option in the configuration file, add
a line with the syntax `argumentname=value`, where `argumentname` is the name
of the argument without the leading dashes, for example `log-level=1`.

To run in background:

```bash
./bin/lokid --log-file lokid.log --detach
```

To run as a systemd service, copy
[lokid.service](utils/systemd/lokid.service) to `/etc/systemd/system/` and
[lokid.conf](utils/conf/lokid.conf) to `/etc/`. The [example
service](utils/systemd/lokid.service) assumes that the user `loki` exists
and its home is the data directory specified in the [example
config](utils/conf/lokid.conf).

If you're on Mac, you may need to add the `--max-concurrency 1` option to
loki-wallet-cli, and possibly lokid, if you get crashes refreshing.

## Internationalization

See [README.i18n.md](README.i18n.md).

## Using Tor

> There is a new, still experimental, [integration with Tor](ANONYMITY_NETWORKS.md). The
> feature allows connecting over IPv4 and Tor simulatenously - IPv4 is used for
> relaying blocks and relaying transactions received by peers whereas Tor is
> used solely for relaying transactions received over local RPC. This provides
> privacy and better protection against surrounding node (sybil) attacks.

While Loki isn't made to integrate with Tor, it can be used wrapped with torsocks, by
setting the following configuration parameters and environment variables:

* `--p2p-bind-ip 127.0.0.1` on the command line or `p2p-bind-ip=127.0.0.1` in
  lokid.conf to disable listening for connections on external interfaces.
* `--no-igd` on the command line or `no-igd=1` in lokid.conf to disable IGD
  (UPnP port forwarding negotiation), which is pointless with Tor.
* `DNS_PUBLIC=tcp` or `DNS_PUBLIC=tcp://x.x.x.x` where x.x.x.x is the IP of the
  desired DNS server, for DNS requests to go over TCP, so that they are routed
  through Tor. When IP is not specified, lokid uses the default list of
  servers defined in [src/common/dns_utils.cpp](src/common/dns_utils.cpp).
* `TORSOCKS_ALLOW_INBOUND=1` to tell torsocks to allow lokid to bind to interfaces
   to accept connections from the wallet. On some Linux systems, torsocks
   allows binding to localhost by default, so setting this variable is only
   necessary to allow binding to local LAN/VPN interfaces to allow wallets to
   connect from remote hosts. On other systems, it may be needed for local wallets
   as well.
* Do NOT pass `--detach` when running through torsocks with systemd, (see
  [utils/systemd/lokid.service](utils/systemd/lokid.service) for details).
* If you use the wallet with a Tor daemon via the loopback IP (eg, 127.0.0.1:9050),
  then use `--untrusted-daemon` unless it is your own hidden service.

Example command line to start lokid through Tor:

```bash
DNS_PUBLIC=tcp torsocks lokid --p2p-bind-ip 127.0.0.1 --no-igd
```

### Using Tor on Tails

TAILS ships with a very restrictive set of firewall rules. Therefore, you need
to add a rule to allow this connection too, in addition to telling torsocks to
allow inbound connections. Full example:

```bash
sudo iptables -I OUTPUT 2 -p tcp -d 127.0.0.1 -m tcp --dport 22023 -j ACCEPT
DNS_PUBLIC=tcp torsocks ./lokid --p2p-bind-ip 127.0.0.1 --no-igd --rpc-bind-ip 127.0.0.1 \
    --data-dir /home/amnesia/Persistent/your/directory/to/the/blockchain
```

## Debugging

This section contains general instructions for debugging failed installs or problems encountered with Loki. First ensure you are running the latest version built from the Github repo.

### Obtaining stack traces and core dumps on Unix systems

We generally use the tool `gdb` (GNU debugger) to provide stack trace functionality, and `ulimit` to provide core dumps in builds which crash or segfault.

* To use `gdb` in order to obtain a stack trace for a build that has stalled:

Run the build.

Once it stalls, enter the following command:

```bash
gdb /path/to/lokid `pidof lokid`
```

Type `thread apply all bt` within gdb in order to obtain the stack trace

* If however the core dumps or segfaults:

Enter `ulimit -c unlimited` on the command line to enable unlimited filesizes for core dumps

Enter `echo core | sudo tee /proc/sys/kernel/core_pattern` to stop cores from being hijacked by other tools

Run the build.

When it terminates with an output along the lines of "Segmentation fault (core dumped)", there should be a core dump file in the same directory as lokid. It may be named just `core`, or `core.xxxx` with numbers appended.

You can now analyse this core dump with `gdb` as follows:

```bash
gdb /path/to/lokid /path/to/dumpfile`
```

Print the stack trace with `bt`

#### To run Loki within gdb:

Type `gdb /path/to/lokid`

Pass command-line options with `--args` followed by the relevant arguments

Type `run` to run lokid

### Analysing memory corruption

There are two tools available:

#### ASAN

Configure Loki with the -D SANITIZE=ON cmake flag, eg:

```bash
cd build/debug && cmake -D SANITIZE=ON -D CMAKE_BUILD_TYPE=Debug ../..
```

You can then run the loki tools normally. Performance will typically halve.

#### valgrind

Install valgrind and run as `valgrind /path/to/lokid`. It will be very slow.

### LMDB

Instructions for debugging suspected blockchain corruption as per @HYC

There is an `mdb_stat` command in the LMDB source that can print statistics about the database but it's not routinely built. This can be built with the following command:

```bash
cd ~/loki/external/db_drivers/liblmdb && make
```

The output of `mdb_stat -ea <path to blockchain dir>` will indicate inconsistencies in the blocks, block_heights and block_info table.

The output of `mdb_dump -s blocks <path to blockchain dir>` and `mdb_dump -s block_info <path to blockchain dir>` is useful for indicating whether blocks and block_info contain the same keys.

These records are dumped as hex data, where the first line is the key and the second line is the data.
