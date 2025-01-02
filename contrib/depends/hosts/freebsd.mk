FREEBSD_VERSION=13.4

clang_prog=$(shell $(SHELL) $(.SHELLFLAGS) "command -v clang")
clangxx_prog=$(shell $(SHELL) $(.SHELLFLAGS) "command -v clang++")

# We append the FreeBSD version to the target to ensure clang sets __FreeBSD__

freebsd_CC=env -u C_INCLUDE_PATH -u CPLUS_INCLUDE_PATH \
               -u OBJC_INCLUDE_PATH -u OBJCPLUS_INCLUDE_PATH -u CPATH \
               -u LIBRARY_PATH $(clang_prog) --target=$(host)$(FREEBSD_VERSION) --sysroot=$(host_prefix)/native -iwithsysroot/usr/include
freebsd_CXX=env -u C_INCLUDE_PATH -u CPLUS_INCLUDE_PATH \
                -u OBJC_INCLUDE_PATH -u OBJCPLUS_INCLUDE_PATH -u CPATH \
                -u LIBRARY_PATH $(clangxx_prog) --target=$(host)$(FREEBSD_VERSION) -stdlib=libc++ --sysroot=$(host_prefix)/native \
                -iwithsysroot/usr/include/c++/v1 -iwithsysroot/usr/include

freebsd_AR=ar
freebsd_RANLIB=ranlib
freebsd_NM=nm

freebsd_CFLAGS=-pipe
freebsd_CXXFLAGS=$(freebsd_CFLAGS)
freebsd_ARFLAGS=cr

freebsd_release_CFLAGS=-O2
freebsd_release_CXXFLAGS=$(freebsd_release_CFLAGS)

freebsd_debug_CFLAGS=-g -O0
freebsd_debug_CXXFLAGS=$(freebsd_debug_CFLAGS)

freebsd_native_toolchain=freebsd_base

freebsd_cmake_system=FreeBSD
