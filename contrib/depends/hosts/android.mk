ANDROID_API=24

ifeq ($(host_arch),arm)
host_toolchain=armv7a-linux-androideabi$(ANDROID_API)-
else ifeq ($(host_arch),aarch64)
host_toolchain=aarch64-linux-android$(ANDROID_API)-
endif

clear_guix_env=env -u C_INCLUDE_PATH -u CPLUS_INCLUDE_PATH \
                   -u OBJC_INCLUDE_PATH -u OBJCPLUS_INCLUDE_PATH -u CPATH \
                   -u LIBRARY_PATH

ifeq ($(STAGEX_ENVIRONMENT),)
android_CC=$(clear_guix_env) $(build_prefix)/bin/$(host_toolchain)clang
android_CXX=$(clear_guix_env) $(build_prefix)/bin/$(host_toolchain)clang++
else
# Use clang provided by stagex
android_ndk_clang_version=21
android_clang_target=$(patsubst %-,%,$(host_toolchain))
android_clang_common=--target=$(android_clang_target) \
                     --sysroot=$(build_prefix)/sysroot \
                     -resource-dir=$(build_prefix)/lib/clang/$(android_ndk_clang_version)

android_CC=$(clear_guix_env) /usr/bin/clang $(android_clang_common)
android_CXX=$(clear_guix_env) /usr/bin/clang++ $(android_clang_common)
endif

android_AR=llvm-ar
android_RANLIB=llvm-ranlib

android_CFLAGS=-pipe -std=$(C_STANDARD)
android_CXXFLAGS=-pipe -std=$(CXX_STANDARD)
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
