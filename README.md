# Monero

Copyright (c) 2014-2015, The Monero Project

## Development Resources

Web: [getmonero.org](https://getmonero.org)  
Forum: [forum.getmonero.org](https://forum.getmonero.org)  
Mail: [dev@getmonero.org](mailto:dev@getmonero.org)  
Github (staging): [https://github.com/monero-project/bitmonero](https://github.com/monero-project/bitmonero)  
Github (development): [http://github.com/monero-project/bitmonero/tree/development](http://github.com/monero-project/bitmonero/tree/development)  
IRC: [#monero-dev on Freenode](irc://chat.freenode.net/#monero-dev)

## Introduction

Monero is a private, secure, untraceable currency. You are your bank, you control your funds, and nobody can trace your transfers unless you decide so.

**Privacy:** Monero uses a cryptographically sound system to allow you to send and receive funds without your transactions being easily revealed on the blockchain (the ledger of transactions that everyone has). This ensures that your purchases, receipts, and all transfers remain absolutely private by default.

**Security:** Using the power of a distributed peer-to-peer consensus network, every transaction on the network is cryptographically secured. Individual wallets have a 24 word mnemonic seed that is only displayed once, and can be written down to backup the wallet. Wallet files are encrypted with a passphrase to ensure they are useless if stolen.

**Untraceability:** By taking advantage of ring signatures, a special property of a certain type of cryptography, Monero is able to ensure that transactions are not only untraceable, but have an optional measure of ambiguity that ensures that transactions cannot easily be tied back to an individual user or computer.

## About this Project

This is the core implementation of Monero. It is open source and completely free to use without restrictions, except for those specified in the license agreement below. There are no restrictions on anyone creating an alternative implementation of Monero that uses the protocol and network in a compatible manner.

As with many development projects, the repository on Github is considered to be the "staging" area for the latest changes. Before changes are merged into that branch on the main repository, they are tested by individual developers, committed to the "development" branch, and then subsequently tested by contributors who focus on thorough testing and code reviews. That having been said, the repository should be carefully considered before using it in a production environment, unless there is a patch in the repository for a particular show-stopping issue you are experiencing. It is generally a better idea to use a tagged release for stability.

Anyone is welcome to contribute to Monero. If you have a fix or code change, feel free to submit is as a pull request directly to the "development" branch. In cases where the change is relatively small or does not affect other parts of the codebase it may be merged in immediately by any one of the collaborators. On the other hand, if the change is particularly large or complex, it is expected that it will be discussed at length either well in advance of the pull request being submitted, or even directly on the pull request.

## Supporting the Project

Monero development can be supported directly through donations.

Both Monero and Bitcoin donations can be made to donate.getmonero.org if using a client that supports the [OpenAlias](https://openalias.org) standard

The Monero donation address is: 46BeWrHpwXmHDpDEUmZBWZfoQpdc6HaERCNmx1pEYL2rAcuwufPN9rXHHtyUA4QVy66qeFQkn6sfK8aHYjA3jk3o1Bv16em (viewkey: e422831985c9205238ef84daf6805526c14d96fd7b059fe68c7ab98e495e5703)

The Bitcoin donation address is: 1FhnVJi2V1k4MqXm2nHoEbY5LV7FPai7bb

Core development funding and/or some supporting services are also graciously provided by sponsors:

[![MyMonero](https://static.getmonero.org/images/sponsors/mymonero.png)](https://mymonero.com) [![Kitware](https://static.getmonero.org/images/sponsors/kitware.png?1)](http://kitware.com) [![Dome9](https://static.getmonero.org/images/sponsors/dome9.png)](http://dome9.com) [![Araxis](https://static.getmonero.org/images/sponsors/araxis.png)](http://araxis.com) [![JetBrains](https://static.getmonero.org/images/sponsors/jetbrains.png)](http://www.jetbrains.com/) [![Navicat](https://static.getmonero.org/images/sponsors/navicat.png)](http://www.navicat.com/)

There are also several mining pools that kindly donate a portion of their fees, [a list of them can be found on our Bitcointalk post](https://bitcointalk.org/index.php?topic=583449.0).

## License

Copyright (c) 2014-2015, The Monero Project

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Parts of the project are originally copyright (c) 2012-2013 The Cryptonote developers

## Compiling Monero

### Overview:

Dependencies: GCC 4.7.3 or later, CMake 2.8.6 or later, libunbound 1.4.16 or later (note: Unbound is not a dependency, libunbound is), libevent 2.0 or later, libgtest 1.5 or later, and Boost 1.53 or later (except 1.54, [more details here](http://goo.gl/RrCFmA)).
Static Build Additional Dependencies: ldns 1.6.17 or later, expat 1.1 or later, bison or yacc

**Basic Process:**

* Install the dependencies (see below for more detailed instructions for your OS)
* To build, change to the root of the source code directory, and run `make`. Please note that Windows systems follow a slightly different process, outlined below.
* The resulting executables can be found in `build/release/bin` or `build/debug/bin`, depending on what you're building.

**Advanced options:**

* Parallel build: run `make -j<number of threads>` instead of `make`.
* Statically linked release build: run `make release-static`.
* Debug build: run `make debug`.
* Test suite: run `make release-test` to run tests in addition to building. Running `make debug-test` will do the same to the debug version.

**Makefile Targets for Static Builds:**

For static builds there are a number of Makefile targets to make the build process easier.

* ```make release-static-win64``` builds statically for 64-bit Windows systems
* ```make release-static-win32``` builds statically for 32-bit Windows systems
* ```make release-static-64``` the default, builds statically for 64-bit non-Windows systems
* ```make release-static-32``` builds statically for 32-bit non-Windows systems
* ```make release-static-arm6``` builds statically for ARMv6 devices, such as the Raspberry Pi

### On Linux:

Note: These compile instructions are up to date as of commit 0fdc75 20151014

## Ubuntu

```sudo apt-get update```
```sudo apt-get upgrade -y```
```sudo apt-get install -y git gcc-4.9 cmake libunbound2 libevent-2.0-5 libgtest-dev libboost1.55-all-dev libevent-dev libssl-dev build-essential pkg-config miniupnpc libdb++-dev libdb-dev```
```git clone https://github.com/monero-project/bitmonero.git```
```cd bitmonero```
```make```

## Mint

```sudo apt-get update```
```sudo apt-get install -y git gcc-4.9 cmake libunbound2 libevent-2.0-5 libgtest-dev libboost1.55-dev libboost1.55-all-dev libunbound-dev build-essential libssl-dev libdb++-dev libdb-dev```
```git clone https://github.com/monero-project/bitmonero.git```
```cd bitmonero```
```make```

## Archlinux

```pacman -Sy; pacman -S boost boost-libs git gcc autoconf automake cmake db unbound gtest ldns expat bison pkg-config; git clone https://github.com/libevent/libevent; cd libevent;./autogen.sh;./configure --prefix=/usr;make;make install; cd; git clone https://github.com/monero-project/bitmonero; cd bitmonero; make release-static```

## Centos

Update repository meta data and install required packages.
```yum update; yum groupinstall "Development Tools"; yum install wget cmake unbound-libs libevent-devel expat-devel bison-devel libdb-devel libdb-cxx-devel openssl-devel libicu libicu-devel zlib-devel zlib python-devel bzip2-devel texinfo boost-*```

Enable EPEL repository and install the googletest library.
```wget http://dl.fedoraproject.org/pub/epel/7/x86_64/e/epel-release-7-5.noarch.rpm; rpm -ivh epel-release-7-5.noarch.rpm; yum --enablerepo=epel install gtest-devel```

Install ldns library from source, RPM is version 1.6.16, which is outdated.
```wget http://www.nlnetlabs.nl/downloads/ldns/ldns-1.6.17.tar.gz; tar xvfz ldns-1.6.17.tar.gz; cd ldns-1.6.17; ./configure --prefix=/usr --libdir=/usr/lib64; make; make install```

Optional step: updating Boost libraries. RHEL 7 / CentOS 7 repository offers Boost 1.53 which is the minimal Boost release Monero requires.
If you would like to upgrade to Boost 1.55, here is how you do it.
First, we remove boost-* RPMs we had installed in the first step, installing them was necessary to get initial dependencies added to the system.
And we build from source.
```yum remove boost-*```
```cd; wget "http://sourceforge.net/projects/boost/files/boost/1.55.0/boost_1_55_0.tar.gz"; tar xvfz boost_1_55_0.tar.gz; cd boost_1_55_0```
```./bootstrap.sh --prefix=/usr --libdir=/usr/lib64 --with-icu --with-libraries=atomic,date_time,exception,filesystem,iostreams,locale,program_options,regex,serialization,signals,system,test,thread,timer,log; ./b2; ./b2 install```

Now we compile Monero.
```cd; git clone https://github.com/monero-project/bitmonero; cd bitmonero; make```

## OpenSUSE 13.2.

Install required packages from repository
```zypper install gcc gcc-c++ cmake libunbound2 libevent libevent-devel googletest-devel libldns1 expat libexpat-devel bison libicu-devel libicu53_1 git wget zlib-devel python-devel libopenssl-devel```

OpenSUSE installs Boost 1.54 to /usr/lib64 if you use KDE environment, we must keep Boost 1.54 for KDE and install required Boost release (1.53, 1.55+) from source to /usr/lib
```cd; wget "http://sourceforge.net/projects/boost/files/boost/1.55.0/boost_1_55_0.tar.gz"; tar xvfz boost_1_55_0.tar.gz; cd boost_1_55_0; ./bootstrap.sh --prefix=/usr --libdir=/usr/lib --with-icu --with-libraries=atomic,date_time,exception,filesystem,iostreams,locale,program_options,regex,serialization,signals,system,test,thread,timer,log; ./b2; ./b2 install```

Install Berkeley DB 5.3
```wget http://download.oracle.com/berkeley-db/db-5.3.28.tar.gz; tar xvfz db-5.3.28.tar.gz; cd db-5.3.28/build_unix/; ../dist/configure --enable-cxx --with-pic --enable-dbm --prefix=/usr; make; make install```

Compile Monero.
```cd; git clone https://github.com/monero-project/bitmonero; cd bitmonero; make```

## Magei

Install required packages.
```urpmi -a boost; urpmi libboost-devel cmake expat gcc gcc-c++ make git bison libdb5.3-devel libevent-devel expat-devel bison-devel libicu-devel libstdc++-devel ldns-devel gtest-devel libapr-devel libuuid-devel libunimrcp-deps```

Fix a berkeley db header file.
```ln -s /usr/include/db53/db_cxx.h /usr/include/db_cxx.h```

Install libunbound.
```wget http://www.unbound.net/downloads/unbound-latest.tar.gz; tar xvfz unbound-latest.tar.gz; cd unbound-1*; ./configure; make; make install```

Now we compile Monero.
```cd; git clone https://github.com/monero-project/bitmonero; cd bitmonero; make```

## Slackware 

Note: If you use Slackware 14.1 not CURRENT tree, you must compile Boost 1.55+ yourself instead of installing from packages (see below). The other installation steps apply.
You need to install required packages from Slackware repository:
'D' series: cmake, gcc, gcc-g++, bison, git, libtool, autoconf, automake, m4, make
'L' series:  boost, expat, libevent, icu4c

Install googletest library.
```cd; wget https://googletest.googlecode.com/files/gtest-1.7.0.zip; unzip gtest-1.7.0.zip; cd gtest-1.7.0; ./configure; make; cp -R include/gtest /usr/include; cp lib/.libs/* /usr/lib64/```

Install libunbound.
```cd; wget http://www.unbound.net/downloads/unbound-latest.tar.gz; tar xvfz unbound-latest.tar.gz; cd unbound-1*; ./configure; make; make install```

Install ldns.
```cd; wget https://www.nlnetlabs.nl/downloads/ldns/ldns-1.6.17.tar.gz; tar xvfz ldns-1.6.17.tar.gz; cd ldns-1.6.17; ./configure --prefix=/usr --libdir=/usr/lib64; make; make install```

Install Berkeley DB 5.3
```cd; wget http://download.oracle.com/berkeley-db/db-5.3.28.tar.gz; tar xvfz db-5.3.28.tar.gz; cd db-5.3.28/build_unix/;../dist/configure --enable-cxx --with-pic --enable-dbm --prefix=/usr; make; make install```

Now we compile Monero.
```cd; git clone https://github.com/monero-project/bitmonero; cd bitmonero; make```

Only if you use Slackware 14.1, before building Monero compile Boost 1.55+, do not use Boost 1.54 from the Slackware 14.1 repository.
```cd; wget "http://sourceforge.net/projects/boost/files/boost/1.55.0/boost_1_55_0.tar.gz"; tar xvfz boost_1_55_0.tar.gz; cd boost_1_55_0; ./bootstrap.sh --prefix=/usr --libdir=/usr/lib64 --with-icu --with-libraries=atomic,date_time,exception,filesystem,iostreams,locale,program_options,regex,serialization,signals,system,test,thread,timer,log;./b2; ./b2 install```

## Gentoo

Update portage and install required dependencies.
```emerge --sync; emerge --ask dev-libs/icu dev-libs/libevent dev-cpp/gtest dev-util/cmake dev-libs/expat sys-devel/bison dev-vcs/git net-libs/ldns ```

Now we need to install Boost 1.55+.
I didn't have luck building from Gentoo 'dev-libs/boost' package. To tell the truth, the libraries built without a hitch, but Monero failed to pick them for some reason and thus, I had to unmerge them and use Boost 1.55 from sourceforge. 
If you decide to go with the Gentoo supplied Boost 1.56 package, be sure to have the flag USE="icu", but I recommend to download Boost 1.55 from sourceforge, it's tested and works.

```cd; wget "http://sourceforge.net/projects/boost/files/boost/1.55.0/boost_1_55_0.tar.gz"; tar xvfz boost_1_55_0.tar.gz; cd boost_1_55_0; ./bootstrap.sh --prefix=/usr --libdir=/usr/lib64 --with-icu --with-libraries=atomic,date_time,exception,filesystem,iostreams,locale,program_options,regex,serialization,signals,system,test,thread,timer,log;./b2; ./b2 install```

Install Berkeley DB 5.3
```cd; wget http://download.oracle.com/berkeley-db/db-5.3.28.tar.gz; tar xvfz db-5.3.28.tar.gz; cd db-5.3.28/build_unix/;../dist/configure --enable-cxx --with-pic --enable-dbm --prefix=/usr; make; make install```

Install libunbound from github.
```cd; git clone https://github.com/jedisct1/unbound; cd unbound; ./configure --prefix=/usr --libdir=/usr/lib64; make; make install```

Compile Monero.
```cd; git clone https://github.com/monero-project/bitmonero```

Here we replace unbound source in bitmonero/external/unbound with the github source. For some reason only Gentoo had this problem, unbound-1.5.5+ has the fix. 
I am not a dev though, don't ask me why. My goal was to get Monero to compile on most popular Linux distributions and save you time figuring it out on your own.
If your compilation fails: "unbound/util/net_help.c:656:3: error: unknown type name ^ ^ EC_KEY ^ ^ EC_KEY *ecdh = EC_KEY_new_by_curve_name (NID_X9_62_prime256v1)", fear no more.
Let's fetch unbound source and copy cmake files.
```cd bitmonero/external; mv unbound unbound.old; git clone https://github.com/jedisct1/unbound; cp unbound.old/CMakeLists.txt unbound.old/config.h.cmake.in unbound.old/configure_checks.cmake unbound```

We're ready to compile Monero, let's go.
```cd ~/bitmonero; make```

## Sabayon.
Update and install required packages.
```equo update; equo install sys-devel/gcc dev-libs/icu dev-cpp/gtest dev-util/cmake dev-libs/expat sys-devel/bison dev-vcs/git net-libs/ldns sys-libs/db:4.8```

Remove Boost if already installed. Compile and install Boost from sourceforge.
```equo remove boost; cd; wget "http://sourceforge.net/projects/boost/files/boost/1.55.0/boost_1_55_0.tar.gz"; tar xvfz boost_1_55_0.tar.gz; cd boost_1_55_0```
```./bootstrap.sh --prefix=/usr --libdir=/usr/lib64 --with-icu --with-libraries=all;./b2; ./b2 install```

Install libunbound from github.
```cd; git clone https://github.com/jedisct1/unbound; cd unbound; ./configure --prefix=/usr --libdir=/usr/lib64; make; make install```


Install libevent.
```cd; git clone https://github.com/libevent/libevent; cd libevent; ./autogen.sh; ./configure --prefix=/usr; make; make install```


Compile Monero.
```cd; git clone https://github.com/monero-project/bitmonero; cd bitmonero; make release-static```

## Lunar Linux.
Update the system (optional) and package list.
```lunar update; lin moonbase```

Install required software.
```lin curl icu4c; lin cmake expat bison db```

The rest of required software is not available in Moonbase or Monero doesn't build with it, fetch from relevant sources.

Install Git.
```cd; wget --no-check-certificate https://github.com/git/git/archive/v2.5.2.tar.gz; tar xvfz v2.5.2.tar.gz; cd git-2.5.2; make; make install```

Install googletest library.
```cd; wget http://googletest.googlecode.com/files/gtest-1.7.0.zip; unzip gtest-1.7.0.zip; cd gtest-1.7.0; ./configure; make; cp -R include/gtest /usr/include; cp lib/.libs/* /usr/lib64/```

Compile and install Boost 1.55 from sourceforge.
```cd; wget "http://sourceforge.net/projects/boost/files/boost/1.55.0/boost_1_55_0.tar.gz"; tar xvfz boost_1_55_0.tar.gz; cd boost_1_55_0; ./bootstrap.sh --prefix=/usr --libdir=/usr/lib64 --with-icu --with-libraries=all; ./b2; ./b2 install```

Install libunbound from github.
```cd; GIT_SSL_NO_VERIFY=true /root/bin/git clone https://github.com/jedisct1/unbound; cd unbound; ./configure --prefix=/usr --libdir=/usr/lib64; make; make install```

Install ldns.
```cd; wget --no-check-certificate https://www.nlnetlabs.nl/downloads/ldns/ldns-1.6.17.tar.gz; tar xvfz ldns-1.6.17.tar.gz; cd ldns-1.6.17; ./configure --prefix=/usr --libdir=/usr/lib64; make; make install```

Install libevent.
```cd; GIT_SSL_NO_VERIFY=true /root/bin/git clone https://github.com/libevent/libevent; cd libevent; ./autogen.sh; ./configure --prefix=/usr; make; make install```

Compile Monero.
```cd; GIT_SSL_NO_VERIFY=true /root/bin/git clone https://github.com/monero-project/bitmonero; cd bitmonero; make release-static```




### On OS X:

The project can be built from scratch by following instructions for Unix and Linux above.

Alternatively, it can be built in an easier and more automated fashion using Homebrew:

* Ensure Homebrew is installed, it can be found at http://brew.sh
* Add the repository to brew: `brew tap sammy007/cryptonight`
* Build Monero: `brew install bitmonero --build-from-source`

## OS X Instructions

Install homebrew
```ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"```

Install various packages needed to build the Monero daemon and wallet.
```brew install git boost cmake libevent miniupnpc  pkg-config```

Clone the bitmonero repository to your computer using Git.
```git clone https://github.com/monero-project/bitmonero.git bitmonero```

Compile
```cd bitmonero```
```make release```

### On Windows:

Dependencies: mingw-w64, msys2, CMake 2.8.6 or later, libunbound 1.4.16 or later (note: Unbound is not a dependency, libunbound is), and Boost 1.53 or 1.55 (except 1.54, [more details here](http://goo.gl/RrCFmA)), BerkeleyDB 4.8 or later (note: on Ubuntu this means installing libdb-dev and libdb++-dev).

**Preparing the Build Environment**

* Download the [MSYS2 installer](http://msys2.github.io), 64-bit or 32-bit as needed, and run it.
* Use the shortcut associated with your architecture to launch the MSYS2 environment. On 64-bit systems that would be the MinGW-w64 Win64 Shell shortcut. Note that if you are running 64-bit Windows, you will have both 64-bit and 32-bit environments.
* Update the packages in your MSYS2 install:
```
pacman -Sy
pacman -Su --ignoregroup base
pacman -Su
```
* For those of you already familiar with pacman, you can run the normal `pacman -Syu` to update, but you may get errors and need to restart MSYS2 if pacman's dependencies are updated.
* Install dependencies: `pacman -S mingw-w64-x86_64-gcc make mingw-w64-x86_64-cmake mingw-w64-x86_64-unbound mingw-w64-x86_64-boost`
* If you are planning to build statically you will also need to install: `pacman -S mingw-w64-x86_64-ldns mingw-w64-x86_64-expat` (note that these are likely already installed by the unbound dependency installation above)

**Building**

* From the root of the source code directory run:
```
mkdir build
cd build
```
* If you are on a 64-bit system, run:
```
cmake -G "MSYS Makefiles" -D CMAKE_BUILD_TYPE=Release -D ARCH="x86-64" -D BUILD_64=ON -D CMAKE_TOOLCHAIN_FILE=../cmake/64-bit-toolchain.cmake -D MSYS2_FOLDER=c:/msys64 ..
```
* If you are on a 32-bit system, run:
```
cmake -G "MSYS Makefiles" -D CMAKE_BUILD_TYPE=Release -D ARCH="i686" -D BUILD_64=OFF -D CMAKE_TOOLCHAIN_FILE=../cmake/32-bit-toolchain.cmake -D MSYS2_FOLDER=c:/msys32 ..
```
* You can now run `make` to have it build
* The resulting executables can be found in `build/release/bin` or `build/debug/bin`, depending on what you're building.

If you installed MSYS2 in a folder other than c:/msys64, make the appropriate substitution above.

**Advanced options:**

* Parallel build: run `make -j<number of threads>` instead of `make`.
* Statically linked release build: run `make release-static`.
* Debug build: run `make debug`.
* Test suite: run `make release-test` to run tests in addition to building. Running `make debug-test` will do the same to the debug version.

### On FreeBSD:

The project can be built from scratch by following instructions for Unix and Linux above.

We expect to add Monero into the ports tree in the near future, which will aid in managing installations using ports or packages.

## Building Documentation

Monero developer documentation uses Doxygen, and is currently a work-in-progress.

Dependencies: Doxygen 1.8.0 or later, Graphviz 2.28 or later (optional).

* To build, change to the root of the source code directory, and run `doxygen Doxyfile`
* If you have installed Graphviz, you can also generate in-doc diagrams by instead running `HAVE_DOT=YES doxygen Doxyfile`
* The output will be built in doc/html/

## Internationalization

See README.i18n

