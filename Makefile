# Copyright (c) 2014-2024, The Monero Project
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are
# permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this list of
#    conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this list
#    of conditions and the following disclaimer in the documentation and/or other
#    materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors may be
#    used to endorse or promote products derived from this software without specific
#    prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
# THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
# THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

ANDROID_STANDALONE_TOOLCHAIN_PATH ?= /usr/local/toolchain

dotgit=$(shell ls -d .git/config)
ifneq ($(dotgit), .git/config)
  USE_SINGLE_BUILDDIR=1
endif

subbuilddir:=$(shell echo  `uname | sed -e 's|[:/\\ \(\)]|_|g'`/`git branch | grep '\* ' | cut -f2- -d' '| sed -e 's|[:/\\ \(\)]|_|g'`)
ifeq ($(USE_SINGLE_BUILDDIR),)
  builddir := build/"$(subbuilddir)"
  topdir   := ../../../..
  deldirs  := $(builddir)
else
  builddir := build
  topdir   := ../..
  deldirs  := $(builddir)/debug $(builddir)/release $(builddir)/fuzz
endif

all: release-all

depends:
	cd contrib/depends && $(MAKE) HOST=$(target) && cd ../.. && mkdir -p build/$(target)/release
	cd build/$(target)/release && USE_DEVICE_TREZOR_MANDATORY=1 cmake -DCMAKE_TOOLCHAIN_FILE=$(CURDIR)/contrib/depends/$(target)/share/toolchain.cmake ../../.. && $(MAKE)

cmake-debug:
	mkdir -p $(builddir)/debug
	cd $(builddir)/debug && cmake -D CMAKE_BUILD_TYPE=Debug $(topdir)

debug: cmake-debug
	cd $(builddir)/debug && cmake --build .

# Temporarily disable some tests:
#  * libwallet_api_tests fail (Issue #895)
debug-test:
	mkdir -p $(builddir)/debug
	cd $(builddir)/debug && cmake -D BUILD_TESTS=ON -D CMAKE_BUILD_TYPE=Debug $(topdir) &&  $(MAKE) && $(MAKE) ARGS="-E libwallet_api_tests" test

debug-test-asan:
	mkdir -p $(builddir)/debug
	cd $(builddir)/debug && cmake -D BUILD_TESTS=ON -D SANITIZE=ON -D CMAKE_BUILD_TYPE=Debug $(topdir) &&  $(MAKE) && $(MAKE) ARGS="-E libwallet_api_tests" test

debug-test-trezor:
	mkdir -p $(builddir)/debug
	cd $(builddir)/debug && cmake -D BUILD_TESTS=ON -D TREZOR_DEBUG=ON -D CMAKE_BUILD_TYPE=Debug $(topdir) &&  $(MAKE) && $(MAKE) ARGS="-E libwallet_api_tests" test

debug-all:
	mkdir -p $(builddir)/debug
	cd $(builddir)/debug && cmake -D BUILD_TESTS=ON -D BUILD_SHARED_LIBS=OFF -D CMAKE_BUILD_TYPE=Debug $(topdir) && $(MAKE)

cmake-release:
	mkdir -p $(builddir)/release
	cd $(builddir)/release && cmake -D CMAKE_BUILD_TYPE=Release $(topdir)

release: cmake-release
	cd $(builddir)/release && cmake --build .

release-test:
	mkdir -p $(builddir)/release
	cd $(builddir)/release && cmake -D BUILD_TESTS=ON -D CMAKE_BUILD_TYPE=Release $(topdir) && $(MAKE) && $(MAKE) test

release-all:
	mkdir -p $(builddir)/release
	cd $(builddir)/release && cmake -D BUILD_TESTS=ON -D CMAKE_BUILD_TYPE=Release $(topdir) && $(MAKE)

release-static:
	mkdir -p $(builddir)/release
	cd $(builddir)/release && cmake -D STATIC=ON -D ARCH="default" -D CMAKE_BUILD_TYPE=Release $(topdir) && cmake --build .

coverage:
	mkdir -p $(builddir)/debug
	cd $(builddir)/debug && cmake -D BUILD_TESTS=ON -D CMAKE_BUILD_TYPE=Debug -D COVERAGE=ON $(topdir) && $(MAKE) && $(MAKE) test

fuzz:
	mkdir -p $(builddir)/fuzz
	cd $(builddir)/fuzz && cmake -D STATIC=ON -D SANITIZE=ON -D BUILD_TESTS=ON -D CMAKE_C_COMPILER=afl-gcc -D CMAKE_CXX_COMPILER=afl-g++ -D ARCH="x86-64" -D CMAKE_BUILD_TYPE=fuzz -D BUILD_TAG="linux-x64" $(topdir) && $(MAKE)

clean:
	@echo "WARNING: Back-up your wallet if it exists within ./"$(deldirs)"!" ; \
    read -r -p "This will destroy the build directory, continue (y/N)?: " CONTINUE; \
	[ $$CONTINUE = "y" ] || [ $$CONTINUE = "Y" ] || (echo "Exiting."; exit 1;)
	rm -rf $(deldirs)

clean-all:
	@echo "WARNING: Back-up your wallet if it exists within ./build!" ; \
	read -r -p "This will destroy all build directories, continue (y/N)?: " CONTINUE; \
	[ $$CONTINUE = "y" ] || [ $$CONTINUE = "Y" ] || (echo "Exiting."; exit 1;)
	rm -rf ./build

tags:
	ctags -R --sort=1 --c++-kinds=+p --fields=+iaS --extra=+q --language-force=C++ src contrib tests/gtest

.PHONY: all cmake-debug debug debug-test debug-all cmake-release release release-test release-all clean tags
