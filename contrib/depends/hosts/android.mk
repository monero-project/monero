ANDROID_API=21

ifeq ($(host_arch),arm)
host_toolchain=armv7a-linux-androideabi$(ANDROID_API)-
else ifeq ($(host_arch),aarch64)
host_toolchain=aarch64-linux-android$(ANDROID_API)-
endif

clear_guix_env=env -u C_INCLUDE_PATH -u CPLUS_INCLUDE_PATH \
                   -u OBJC_INCLUDE_PATH -u OBJCPLUS_INCLUDE_PATH -u CPATH \
                   -u LIBRARY_PATH

android_CC=$(clear_guix_env) $(build_prefix)/bin/$(host_toolchain)clang
android_CXX=$(clear_guix_env) $(build_prefix)/bin/$(host_toolchain)clang++

android_AR=llvm-ar
android_RANLIB=llvm-ranlib

android_CFLAGS=-pipe
android_CXXFLAGS=$(android_CFLAGS)
android_ARFLAGS=crsD

android_release_CFLAGS=-O2
android_release_CXXFLAGS=$(android_release_CFLAGS)

android_debug_CFLAGS=-g -O0
android_debug_CXXFLAGS=$(android_debug_CFLAGS)

android_native_toolchain=android_ndk

# CMake 3.24 fails to detect API level for Android, even if set explicitly in
# toolchain.cmake. It also tries to manually construct paths to clang(++), but we
# want it to always use the options defined here. It's easier to just pretend
# we're a generic Linux target, than to hack around it.
android_cmake_system=Linux
