OSX_MIN_VERSION=11
OSX_SDK_VERSION=11.0
XCODE_VERSION=12.2
XCODE_BUILD_ID=12B45b
LD64_VERSION=711

OSX_SDK=$(host_prefix)/native/SDK

darwin_native_toolchain=darwin_sdk

clang_prog=$(shell $(SHELL) $(.SHELLFLAGS) "command -v clang")
clangxx_prog=$(shell $(SHELL) $(.SHELLFLAGS) "command -v clang++")
llvm_config_prog=$(shell $(SHELL) $(.SHELLFLAGS) "command -v llvm-config")

darwin_AR=$(shell $(SHELL) $(.SHELLFLAGS) "command -v llvm-ar")
darwin_DSYMUTIL=$(shell $(SHELL) $(.SHELLFLAGS) "command -v dsymutil")
darwin_NM=$(shell $(SHELL) $(.SHELLFLAGS) "command -v llvm-nm")
darwin_OBJDUMP=$(shell $(SHELL) $(.SHELLFLAGS) "command -v llvm-objdump")
darwin_RANLIB=$(shell $(SHELL) $(.SHELLFLAGS) "command -v llvm-ranlib")
darwin_STRIP=$(shell $(SHELL) $(.SHELLFLAGS) "command -v llvm-strip")
darwin_LIBTOOL=$(shell $(SHELL) $(.SHELLFLAGS) "command -v llvm-libtool-darwin")

# Flag explanations:
#
#     -mlinker-version
#
#         Ensures that modern linker features are enabled. See here for more
#         details: https://github.com/bitcoin/bitcoin/pull/19407.
#
#     -B$(build_prefix)/bin
#
#         Explicitly point to our binaries (e.g. cctools) so that they are
#         ensured to be found and preferred over other possibilities.
#
#     -isysroot$(OSX_SDK) -nostdlibinc
#
#         Disable default include paths built into the compiler as well as
#         those normally included for libc and libc++. The only path that
#         remains implicitly is the clang resource dir.
#
#     -iwithsysroot / -iframeworkwithsysroot
#
#         Adds the desired paths from the SDK
#

darwin_CC=env -u C_INCLUDE_PATH -u CPLUS_INCLUDE_PATH \
              -u OBJC_INCLUDE_PATH -u OBJCPLUS_INCLUDE_PATH -u CPATH \
              -u LIBRARY_PATH \
            $(clang_prog) --target=$(host) \
              -B$(build_prefix)/bin \
              -isysroot$(OSX_SDK) -nostdlibinc \
              -iwithsysroot/usr/include -iframeworkwithsysroot/System/Library/Frameworks

darwin_CXX=env -u C_INCLUDE_PATH -u CPLUS_INCLUDE_PATH \
               -u OBJC_INCLUDE_PATH -u OBJCPLUS_INCLUDE_PATH -u CPATH \
               -u LIBRARY_PATH \
             $(clangxx_prog) --target=$(host) \
               -B$(build_prefix)/bin \
               -isysroot$(OSX_SDK) -nostdlibinc \
               -iwithsysroot/usr/include/c++/v1 \
               -iwithsysroot/usr/include -iframeworkwithsysroot/System/Library/Frameworks

darwin_CFLAGS=-pipe -std=c11 -mmacosx-version-min=$(OSX_MIN_VERSION) -mlinker-version=$(LD64_VERSION)
darwin_CXXFLAGS=-pipe -std=c++17 -mmacosx-version-min=$(OSX_MIN_VERSION) -mlinker-version=$(LD64_VERSION)
darwin_LDFLAGS=-Wl,-platform_version,macos,$(OSX_MIN_VERSION),$(OSX_SDK_VERSION) -Wl,-no_adhoc_codesign -fuse-ld=lld
darwin_ARFLAGS=cr

darwin_release_CFLAGS=-O2
darwin_release_CXXFLAGS=$(darwin_release_CFLAGS)

darwin_debug_CFLAGS=-O1
darwin_debug_CXXFLAGS=$(darwin_debug_CFLAGS)

darwin_cmake_system=Darwin
