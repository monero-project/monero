ANDROID_API=21

ifeq ($(host_arch),arm)
host_toolchain=arm-linux-androideabi-
endif

android_CC=$(host_toolchain)clang
android_CXX=$(host_toolchain)clang++
android_RANLIB=:

android_CFLAGS=-pipe
android_CXXFLAGS=$(android_CFLAGS)
android_ARFLAGS=crsD

android_release_CFLAGS=-O2
android_release_CXXFLAGS=$(android_release_CFLAGS)

android_debug_CFLAGS=-g -O0
android_debug_CXXFLAGS=$(android_debug_CFLAGS)

android_native_toolchain=android_ndk

