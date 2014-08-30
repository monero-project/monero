# Monero

Copyright (c) 2014, The Monero Project

## Development Resources

Web: [monero.cc](http://monero.cc)  
Mail: [dev@monero.cc](mailto:dev@monero.cc)  
Github (staging): [https://github.com/monero-project/bitmonero](https://github.com/monero-project/bitmonero)  
Github (development): [http://github.com/monero-project/bitmonero/tree/development](http://github.com/monero-project/bitmonero/tree/development)  
IRC: [#monero-dev on Freenode](irc://chat.freenode.net/#monero-dev)

## Introduction

Monero is a private, secure, untraceable currency. You are your bank, you control your funds, and nobody can trace your transfers.

**Privacy:** Monero uses a cryptographically sound system to allow you to send and receive funds without your transactions being easily revealed on the blockchain (the ledger of transactions that everyone has). This ensures that your purchases, receipts, and all transfers remain absolutely private by default.

**Security:** Using the power of a distributed peer-to-peer consensus network, every transaction on the network is cryptographically secured. Individual wallets have a 24 word mnemonic that is only displayed once, and can be written down to backup the wallet. Wallet files are encrypted with a passphrase to ensure they are useless if stolen.

**Untraceability:** By taking advantage of ring signatures, a special property of certain types of cryptography, Monero is able to ensure that transactions are not only untraceable, but have an optional measure of ambiguity that ensures that transactions cannot easily be tied back to an individual user or computer.

## About this Project

This is the core implementation of Monero. It is open source and completely free to use without restrictions, except for those specified in the license agreement below. There are no restrictions on anyone creating an alternative implementation of Monero that uses the protocol and network in a compatible manner.

As with many development projects, the repository on Github is considered to be the "staging" area for the latest changes. Before changes are merged into that branch on the main repository, they are tested by individual developers, committed to the "development" branch, and then subsequently tested by contributors who focus on thorough testing and code reviews. That having been said, the repository should be carefully considered before using it in a production environment, unless there is a patch in the repository for a particular show-stopping issue you are experiencing. It is generally a better idea to use a tagged release for stability.

Anyone is able to contribute to Monero. If you have a fix or code change, feel free to submit is as a pull request directly to the "development" branch. In cases where the change is relatively small or does not affect other parts of the codebase it may be merged in immediately by any one of the collaborators. On the other hand, if the change is particularly large or complex, it is expected that it will be discussed at length either well in advance of the pull request being submitted, or even directly on the pull request.

## License

Copyright (c) 2014, The Monero Project

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Parts of the project are originally copyright (c) 2012-2013 The Cryptonote developers

## Compiling Monero

### On Unix and Linux:

Dependencies: GCC 4.7.3 or later, CMake 2.8.6 or later, and Boost 1.53 or later (except 1.54, [more details here](http://goo.gl/RrCFmA)).

**Basic Process:**

* To build, change to the root of the source code directory, and run `make`.
* The resulting executables can be found in build/release/src.

**Advanced options:**

* Parallel build: run `make -j<number of threads>` instead of `make`.
* Debug build: run `make build-debug`.
* Test suite: run `make test-release` to run tests in addition to building. Running `make test-debug` will do the same to the debug version.
* Building with Clang: it may be possible to use Clang instead of GCC, but this may not work everywhere. To build, run `export CC=clang CXX=clang++` before running `make`.

### On OS X:

The project can be built from scratch by following instructions for Unix and Linux above.

Alternatively, it can be built in an easier and more automated fashion using Homebrew:

* Ensure Homebrew is installed, it can be found at http://brew.sh
* Add the repository to brew: `brew tap sammy007/cryptonight`
* Build Monero: `brew install --HEAD bitmonero`

### On Windows:

Dependencies: MSVC 2013 or later, CMake 2.8.6 or later, and Boost 1.53 or later (except 1.54, [more details here](http://goo.gl/RrCFmA)).

* To build, change to the root of the source code directory, and run these commands:
```
mkdir build
cd build
cmake -G "Visual Studio 12 Win64" -DBOOST_ROOT="c:\folder\to\boost_1_55_0" -DBOOST_LIBRARYDIR="c:\folder\to\boost_1_55_0\lib64-msvc-12.0"
msbuild Project.sln /p:Configuration=Release
```
* If you don't have your path environment variable configured with the VS paths, you may may want to run `C:\program files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat` (or equivalent) to temporarily set the environment variables.

## Building Documentation

Monero developer documentation uses Doxygen, and is currently a work-in-progress.

Dependencies: Doxygen 1.8.0 or later, Graphviz 2.28 or later (optional).

* To build, change to the root of the source code directory, and run `doxygen Doxyfile`
* If you have installed Graphviz, you can also generate in-doc diagrams by instead running `HAVE_DOT=YES doxygen Doxyfile`
* The output will be built in doc/html/