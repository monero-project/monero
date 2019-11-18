freebsd_CC=clang-8
freebsd_CXX=clang++-8
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

