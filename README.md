# Monero

Copyright (c) 2014-2016, The Monero Project

[![Build Status](https://travis-ci.org/monero-project/bitmonero.svg?branch=master)](https://travis-ci.org/monero-project/bitmonero)

## Development Resources

- Web: [getmonero.org](https://getmonero.org)
- Forum: [forum.getmonero.org](https://forum.getmonero.org)
- Mail: [dev@getmonero.org](mailto:dev@getmonero.org)
- GitHub: [https://github.com/monero-project/bitmonero](https://github.com/monero-project/bitmonero)
- IRC: [#monero-dev on Freenode](irc://chat.freenode.net/#monero-dev)

## Introduction

Monero is a private, secure, untraceable, decentralised digital currency. You are your bank, you control your funds, and nobody can trace your transfers unless you allow them to do so.

**Privacy:** Monero uses a cryptographically sound system to allow you to send and receive funds without your transactions being easily revealed on the blockchain (the ledger of transactions that everyone has). This ensures that your purchases, receipts, and all transfers remain absolutely private by default.

**Security:** Using the power of a distributed peer-to-peer consensus network, every transaction on the network is cryptographically secured. Individual wallets have a 24 word mnemonic seed that is only displayed once, and can be written down to backup the wallet. Wallet files are encrypted with a passphrase to ensure they are useless if stolen.

**Untraceability:** By taking advantage of ring signatures, a special property of a certain type of cryptography, Monero is able to ensure that transactions are not only untraceable, but have an optional measure of ambiguity that ensures that transactions cannot easily be tied back to an individual user or computer.

## About this Project

This is the core implementation of Monero. It is open source and completely free to use without restrictions, except for those specified in the license agreement below. There are no restrictions on anyone creating an alternative implementation of Monero that uses the protocol and network in a compatible manner.

As with many development projects, the repository on Github is considered to be the "staging" area for the latest changes. Before changes are merged into that branch on the main repository, they are tested by individual developers in their own branches, submitted as a pull request, and then subsequently tested by contributors who focus on testing and code reviews. That having been said, the repository should be carefully considered before using it in a production environment, unless there is a patch in the repository for a particular show-stopping issue you are experiencing. It is generally a better idea to use a tagged release for stability.

**Anyone is welcome to contribute to Monero's codebase!** If you have a fix or code change, feel free to submit is as a pull request directly to the "master" branch. In cases where the change is relatively small or does not affect other parts of the codebase it may be merged in immediately by any one of the collaborators. On the other hand, if the change is particularly large or complex, it is expected that it will be discussed at length either well in advance of the pull request being submitted, or even directly on the pull request.

## Supporting the Project

Monero development can be supported directly through donations.

Both Monero and Bitcoin donations can be made to donate.getmonero.org if using a client that supports the [OpenAlias](https://openalias.org) standard

The Monero donation address is: `44AFFq5kSiGBoZ4NMDwYtN18obc8AemS33DBLWs3H7otXft3XjrpDtQGv7SqSsaBYBb98uNbr2VBBEt7f2wfn3RVGQBEP3A` (viewkey: `f359631075708155cc3d92a32b75a7d02a5dcf27756707b47a2b31b21c389501`)

The Bitcoin donation address is: `1KTexdemPdxSBcG55heUuTjDRYqbC5ZL8H`

Core development funding and/or some supporting services are also graciously provided by sponsors:

[<img width="80" src="https://static.getmonero.org/images/sponsors/mymonero.png"/>](https://mymonero.com)
[<img width="150" src="https://static.getmonero.org/images/sponsors/kitware.png?1"/>](http://kitware.com)
[<img width="100" src="https://static.getmonero.org/images/sponsors/dome9.png"/>](http://dome9.com)
[<img width="150" src="https://static.getmonero.org/images/sponsors/araxis.png"/>](http://araxis.com)
[<img width="150" src="https://static.getmonero.org/images/sponsors/jetbrains.png"/>](http://www.jetbrains.com/)
[<img width="150" src="https://static.getmonero.org/images/sponsors/navicat.png"/>](http://www.navicat.com/)
[<img width="150" src="https://static.getmonero.org/images/sponsors/symas.png"/>](http://www.symas.com/)

There are also several mining pools that kindly donate a portion of their fees, [a list of them can be found on our Bitcointalk post](https://bitcointalk.org/index.php?topic=583449.0).

## License

See [LICENSE](LICENSE).

## Compiling Monero

### Overview:

Dependencies:

* GCC `>=4.7.3`
* CMake `>=3.0.0`
* pkg-config
* libunbound `>=1.4.16` (note: Unbound is not a dependency, libunbound is)
* libevent `>=2.0`
* libgtest `>=1.5`
* Boost `>=1.58`
* BerkeleyDB `>=4.8` (note: on Ubuntu this means installing libdb-dev and libdb++-dev)
* libunwind (optional, for stack trace on exception)
* miniupnpc (optional, for NAT punching)

Additional dependencies for statically-linked build:

* ldns `>=1.6.17`
* expat `>=1.1`
* bison or yacc

Additional dependencies for building documentation:

* Doxygen
* graphviz

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

The instructions above should provide enough detail.

### On OS X:

**Basic Process:**

The project can be built from scratch by following instructions for Unix and Linux above.

**Alternate Process:**

Alternatively, it can be built in an easier and more automated fashion using Homebrew:

* Ensure Homebrew is installed, it can be found at http://brew.sh
* Add the repository to brew: `brew tap sammy007/cryptonight`
* Build Monero: `brew install bitmonero --build-from-source`

### On Windows:

Dependencies:

* mingw-w64
* msys2
* CMake `>=3.0.0`
* libunbound `>=1.4.16` (note: Unbound is not a dependency, libunbound is)
* Boost `>=1.58`
* BerkeleyDB `>=4.8`

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
* Install dependencies: `pacman -S mingw-w64-x86_64-gcc make mingw-w64-x86_64-cmake mingw-w64-x86_64-expat mingw-w64-x86_64-boost`

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

### On OpenBSD:

This has been tested on OpenBSD 5.8.

You will need to add a few packages to your system. `pkg_add db cmake gcc gcc-libs g++ miniupnpc gtest`.

The doxygen and graphviz packages are optional and require the xbase set.

The Boost package has a bug that will prevent librpc.a from building correctly. In order to fix this, you will have to Build boost yourself from scratch. Follow the directions here (under "Building Boost"):
https://github.com/bitcoin/bitcoin/blob/master/doc/build-openbsd.md

You will have to add the serialization, date_time, and regex modules to Boost when building as they are needed by Monero.

To build: `env CC=egcc CXX=eg++ CPP=ecpp DEVELOPER_LOCAL_TOOLS=1 BOOST_ROOT=/path/to/the/boost/you/built make release-static-64`

## Building Documentation

Monero developer documentation uses Doxygen, and is currently a work-in-progress.

Dependencies: Doxygen 1.8.0 or later, Graphviz 2.28 or later (optional).

* To build, change to the root of the source code directory, and run `doxygen Doxyfile`
* If you have installed Graphviz, you can also generate in-doc diagrams by instead running `HAVE_DOT=YES doxygen Doxyfile`
* The output will be built in doc/html/

## Running bitmonerod

The build places the binary in `bin/` sub-directory within the build directory
from which cmake was invoked (repository root by default). To run in
foreground:

    ./bin/bitmonerod

To run in background:

    ./bin/bitmonerod --log-file bitmonerod.log --detach

To list all available options, run `./bin/bitmonerod --help`.  Options can be
specified either on the command line or in a configuration file passed by the
`--config-file` argument.  To specify an option in the configuration file, add
a line with the syntax `argumentname=value`, where `argumentname` is the name
of the argument without the leading dashes, for example `log-level=1`.

## Internationalization

See README.i18n

## Using Tor

While Monero isn't made to integrate with Tor, it can be used wrapped with torsocks, if you add --p2p-bind-ip 127.0.0.1 to the bitmonerod command line. You also want to set DNS requests to go over TCP, so they'll be routed through Tor, by setting DNS_PUBLIC=tcp. You may also disable IGD (UPnP port forwarding negotiation), which is pointless with Tor. To allow local connections from the wallet, add TORSOCKS_ALLOW_INBOUND=1. Example:

`DNS_PUBLIC=tcp TORSOCKS_ALLOW_INBOUND=1 torsocks bitmonerod --p2p-bind-ip 127.0.0.1 --no-igd`

TAILS ships with a very restrictive set of firewall rules. Therefore, you need to add a rule to allow this connection too, in addition to telling torsocks to allow inbound connections. Full example:

`sudo iptables -I OUTPUT 2 -p tcp -d 127.0.0.1 -m tcp --dport 18081 -j ACCEPT`

`DNS_PUBLIC=tcp TORSOCKS_ALLOW_INBOUND=1 torsocks ./bitmonerod --p2p-bind-ip 127.0.0.1 --no-igd --rpc-bind-ip 127.0.0.1 --data-dir /home/amnesia/Persistent/your/directory/to/the/blockchain`

`./simplewallet`

## Using readline

While bitmonerod and simplewallet do not use readline directly, most of the functionality can be obtained by running them via rlwrap. This allows command recall, edit capabilities, etc. It does not give autocompletion without an extra completion file, however. To use rlwrap, simply prepend `rlwrap` to the command line, eg:

`rlwrap bin/simplewallet --wallet-file /path/to/wallet`
