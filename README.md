# Equilibria

A Private and Decentralized Oracle Solution

### Private
Every transaction you make is private and secure via ring signatures and stealth addresses.

### Open Source
A community project with public code. Anyone can contribute ideas.

### Borderless
Equilibria can be used by anyone with access to the internet.

### Oracle Clusters
Equilibria is a hybrid PoW/PoS decentralized oracle node network

### High Utility 
Use-cases span from stablecoins to eSports

### For announcements:
[Website](https://equilibria.network)
[Twitter](https://twitter.com/EquilibriaCC)  
[Discord](https://discord.gg/pDyfUTs)


# General Information
Copyright (c) 2014-2019 The Monero Project.   
Copyright (c) 2018-2019 Project Triton/Equilibria.
Portions Copyright (c) 2012-2013 The Cryptonote developers.

## Current maintainer
- Harrison Hesslink (yodeh,krakn)

**Anyone is welcome to contribute to Equilibria's codebase!** If you have a fix or code change, feel free to submit it as a pull request directly to the "master" branch. In cases where the change is relatively small or does not affect other parts of the codebase it may be merged in immediately by any one of the collaborators. On the other hand, if the change is particularly large or complex, it is expected that it will be discussed at length either well in advance of the pull request being submitted, or even directly on the pull request.


See [LICENSE](LICENSE).

## Contributing

If you want to help out, see [CONTRIBUTING](CONTRIBUTING.md) for a set of guidelines.

## Compiling Equilibria from source

### Dependencies

The following table summarizes the tools and libraries required to build. A
few of the libraries are also included in this repository (marked as
"Vendored"). By default, the build uses the library installed on the system,
and ignores the vendored sources. However, if no library is found installed on
the system, then the vendored source will be built and used. The vendored
sources are also used for statically-linked builds because distribution
packages often include only shared library binaries (`.so`) but not static
library archives (`.a`).

| Dep          | Min. version  | Vendored | Debian/Ubuntu pkg    | Arch pkg     | Void pkg           | Fedora pkg          | Optional | Purpose         |
| ------------ | ------------- | -------- | -------------------- | ------------ | ------------------ | ------------------- | -------- | --------------- |
| GCC          | 4.7.3         | NO       | `build-essential`    | `base-devel` | `base-devel`       | `gcc`               | NO       |                 |
| CMake        | 3.5           | NO       | `cmake`              | `cmake`      | `cmake`            | `cmake`             | NO       |                 |
| pkg-config   | any           | NO       | `pkg-config`         | `base-devel` | `base-devel`       | `pkgconf`           | NO       |                 |
| Boost        | 1.58          | NO       | `libboost-all-dev`   | `boost`      | `boost-devel`      | `boost-devel`       | NO       | C++ libraries   |
| OpenSSL      | basically any | NO       | `libssl-dev`         | `openssl`    | `libressl-devel`   | `openssl-devel`     | NO       | sha256 sum      |
| libzmq       | 3.0.0         | NO       | `libzmq3-dev`        | `zeromq`     | `zeromq-devel`     | `zeromq-devel`      | NO       | ZeroMQ library  |
| OpenPGM      | ?             | NO       | `libpgm-dev`         | `libpgm`     |                    | `openpgm-devel`     | NO       | For ZeroMQ      |
| libnorm[2]   | ?             | NO       | `libnorm-dev`        |              |                    |                     | YES      | For ZeroMQ      |
| libunbound   | 1.4.16        | YES      | `libunbound-dev`     | `unbound`    | `unbound-devel`    | `unbound-devel`     | NO       | DNS resolver    |
| libsodium    | ?             | NO       | `libsodium-dev`      | `libsodium`  | `libsodium-devel`  | `libsodium-devel`   | NO       | cryptography    |
| libunwind    | any           | NO       | `libunwind8-dev`     | `libunwind`  | `libunwind-devel`  | `libunwind-devel`   | YES      | Stack traces    |
| liblzma      | any           | NO       | `liblzma-dev`        | `xz`         | `liblzma-devel`    | `xz-devel`          | YES      | For libunwind   |
| libreadline  | 6.3.0         | NO       | `libreadline6-dev`   | `readline`   | `readline-devel`   | `readline-devel`    | YES      | Input editing   |
| ldns         | 1.6.17        | NO       | `libldns-dev`        | `ldns`       | `libldns-devel`    | `ldns-devel`        | YES      | SSL toolkit     |
| expat        | 1.1           | NO       | `libexpat1-dev`      | `expat`      | `expat-devel`      | `expat-devel`       | YES      | XML parsing     |
| GTest        | 1.5           | YES      | `libgtest-dev`[1]    | `gtest`      | `gtest-devel`      | `gtest-devel`       | YES      | Test suite      |
| Doxygen      | any           | NO       | `doxygen`            | `doxygen`    | `doxygen`          | `doxygen`           | YES      | Documentation   |
| Graphviz     | any           | NO       | `graphviz`           | `graphviz`   | `graphviz`         | `graphviz`          | YES      | Documentation   |
| lrelease     | ?             | NO       | `qttools5-dev-tools` | `qt5-tools`  | `qt5-tools`        | `qt5-linguist`      | YES      | Translations    |
| libhidapi    | ?             | NO       | `libhidapi-dev`      | `hidapi`     | `hidapi-devel`     | `hidapi-devel`      | YES      | Hardware wallet |
| libusb       | ?             | NO       | `libusb-dev`         | `libusb`     | `libusb-devel`     | `libusb-devel`      | YES      | Hardware wallet |
| libprotobuf  | ?             | NO       | `libprotobuf-dev`    | `protobuf`   | `protobuf-devel`   | `protobuf-devel`    | YES      | Hardware wallet |
| protoc       | ?             | NO       | `protobuf-compiler`  | `protobuf`   | `protobuf`         | `protobuf-compiler` | YES      | Hardware wallet |


[1] On Debian/Ubuntu `libgtest-dev` only includes sources and headers. You must
build the library binary manually. This can be done with the following command ```sudo apt-get install libgtest-dev && cd /usr/src/gtest && sudo cmake . && sudo make && sudo mv libg* /usr/lib/ ```
[2] libnorm-dev is needed if your zmq library was built with libnorm, and not needed otherwise

Install all dependencies at once on Debian/Ubuntu:

``` sudo apt update && sudo apt install build-essential cmake pkg-config libboost-all-dev libssl-dev libzmq3-dev libunbound-dev libsodium-dev libunwind8-dev liblzma-dev libreadline6-dev libldns-dev libexpat1-dev doxygen graphviz libpgm-dev qttools5-dev-tools libhidapi-dev libusb-dev libprotobuf-dev protobuf-compiler ```

Install all dependencies at once on macOS with the provided Brewfile:
``` brew update && brew bundle --file=contrib/brew/Brewfile ```

FreeBSD one liner for required to build dependencies
```pkg install git gmake cmake pkgconf boost-libs libzmq libsodium```

### Cloning the repository

Clone recursively to pull-in needed submodule(s):

`$ git clone --recursive https://github.com/EquilibriaCC/Equilibria.git equilibria`

If you already have a repo cloned, initialize and update:

`$ cd equilibria && git submodule init && git submodule update`

### Build instructions

Equilibria uses the CMake build system and a top-level [Makefile](Makefile) that
invokes cmake commands as needed.

#### On Linux and macOS

* Install the dependencies
* Change to the root of the source code directory, change to the most recent release branch, and build:

        cd equilibria
        git checkout v8.0.0
        make

    *Optional*: If your machine has several cores and enough memory, enable
    parallel build by running `make -j<number of threads>` instead of `make`. For
    this to be worthwhile, the machine should have one core and about 2GB of RAM
    available per thread.

    *Note*: The instructions above will compile the most stable release of the
    Equilibria software. If you would like to use and test the most recent software,
    use ```git checkout master```. The master branch may contain updates that are
    both unstable and incompatible with release software, though testing is always
    encouraged.

* The resulting executables can be found in `build/release/bin`

* Add `PATH="$PATH:$HOME/equilibria/build/Linux/release/bin"` to `.profile`

* Run Equilibria with `daemond --detach`

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

* Install the dependencies for Monero from the 'Debian' column in the table above.

* Increase the system swap size:

    ```bash
    sudo /etc/init.d/dphys-swapfile stop  
    sudo nano /etc/dphys-swapfile  
    CONF_SWAPSIZE=2048
    sudo /etc/init.d/dphys-swapfile start
    ```

* If using an external hard disk without an external power supply, ensure it gets enough power to avoid hardware issues when syncing, by adding the line "max_usb_current=1" to /boot/config.txt

* Clone monero and checkout the most recent release version:

    ```bash
    git clone https://github.com/monero-project/monero.git
    cd Equilibria
    git checkout tags/v6.0.2
    ```

* Build:

    ```bash
    make release
    ```

* Wait 4-6 hours

* The resulting executables can be found in `build/release/bin`

* Add `PATH="$PATH:$HOME/monero/build/release/bin"` to `.profile`

* Run Monero with `daemon --detach`

* You may wish to reduce the size of the swap file after the build has finished, and delete the boost directory from your home directory

#### *Note for Raspbian Jessie users:*

If you are using the older Raspbian Jessie image, compiling Monero is a bit more complicated. The version of Boost available in the Debian Jessie repositories is too old to use with Monero, and thus you must compile a newer version yourself. The following explains the extra steps, and has been tested on a Raspberry Pi 2 with a clean install of minimal Raspbian Jessie.

* As before, `apt-get update && apt-get upgrade` to install all of the latest software, and increase the system swap size

    ```bash
    sudo /etc/init.d/dphys-swapfile stop
    sudo nano /etc/dphys-swapfile
    CONF_SWAPSIZE=2048
    sudo /etc/init.d/dphys-swapfile start
    ```


* Then, install the dependencies for Monero except `libunwind` and `libboost-all-dev`

* Install the latest version of boost (this may first require invoking `apt-get remove --purge libboost*-dev` to remove a previous version if you're not using a clean install):

    ```bash
    cd
    wget https://sourceforge.net/projects/boost/files/boost/1.72.0/boost_1_72_0.tar.bz2
    tar xvfo boost_1_72_0.tar.bz2
    cd boost_1_72_0
    ./bootstrap.sh
    sudo ./b2
    ```

* Wait ~8 hours

    ```bash    
    sudo ./bjam cxxflags=-fPIC cflags=-fPIC -a install
    ```

* Wait ~4 hours

* From here, follow the [general Raspberry Pi instructions](#on-the-raspberry-pi) from the "Clone monero and checkout most recent release version" step.

#### On Windows:

Binaries for Windows are built on Windows using the MinGW toolchain within
[MSYS2 environment](https://www.msys2.org). The MSYS2 environment emulates a
POSIX system. The toolchain runs within the environment and *cross-compiles*
binaries that can run outside of the environment as a regular Windows
application.

**Preparing the build environment**

* Download and install the [MSYS2 installer](https://www.msys2.org), either the 64-bit or the 32-bit package, depending on your system.
* Open the MSYS shell via the `MSYS2 Shell` shortcut
* Update packages using pacman:  

    ```bash
    pacman -Syu
    ```

* Exit the MSYS shell using Alt+F4  
* Edit the properties for the `MSYS2 Shell` shortcut changing "msys2_shell.bat" to "msys2_shell.cmd -mingw64" for 64-bit builds or "msys2_shell.cmd -mingw32" for 32-bit builds
* Restart MSYS shell via modified shortcut and update packages again using pacman:  

    ```bash
    pacman -Syu
    ```


* Install dependencies:

    To build for 64-bit Windows:

    ```bash
    pacman -S mingw-w64-x86_64-toolchain make mingw-w64-x86_64-cmake mingw-w64-x86_64-boost mingw-w64-x86_64-openssl mingw-w64-x86_64-zeromq mingw-w64-x86_64-libsodium mingw-w64-x86_64-hidapi
    ```

    To build for 32-bit Windows:

    ```bash
    pacman -S mingw-w64-i686-toolchain make mingw-w64-i686-cmake mingw-w64-i686-boost mingw-w64-i686-openssl mingw-w64-i686-zeromq mingw-w64-i686-libsodium mingw-w64-i686-hidapi
    ```

* Open the MingW shell via `MinGW-w64-Win64 Shell` shortcut on 64-bit Windows
  or `MinGW-w64-Win64 Shell` shortcut on 32-bit Windows. Note that if you are
  running 64-bit Windows, you will have both 64-bit and 32-bit MinGW shells.

**Cloning**

* To git clone, run:

        git clone --recursive https://github.com/EquilibriaCC/Equilibria.git equilibria

**Building**

* Change to the cloned directory, run:

        cd equilibria

        git checkout v6.0.2

* If you are on a 64-bit system, run:

    ```bash
    make release-static-win64
    ```

* If you are on a 32-bit system, run:

    ```bash
    make release-static-win32
    ```

* The resulting executables can be found in `build/release/bin`

* **Optional**: to build Windows binaries suitable for debugging on a 64-bit system, run:

    ```bash
    make debug-static-win64
    ```

* **Optional**: to build Windows binaries suitable for debugging on a 32-bit system, run:

    ```bash
    make debug-static-win32
    ```

* The resulting executables can be found in `build/debug/bin`
