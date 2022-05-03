OSX_MIN_VERSION=10.8
OSX_SDK_VERSION=11.1
OSX_SDK=$(SDK_PATH)/MacOSX$(OSX_SDK_VERSION).sdk
LD64_VERSION=609
ifeq (aarch64, $(host_arch))
CC_target=arm64-apple-darwin11
else
CC_target=$(host)
endif
darwin_CC=clang -target $(CC_target) -mmacosx-version-min=$(OSX_MIN_VERSION) --sysroot $(OSX_SDK) -mlinker-version=$(LD64_VERSION) -B$(host_prefix)/native/bin/$(host)-
darwin_CXX=clang++ -target $(CC_target) -mmacosx-version-min=$(OSX_MIN_VERSION) --sysroot $(OSX_SDK) -mlinker-version=$(LD64_VERSION) -stdlib=libc++ -B$(host_prefix)/native/bin/$(host)-

darwin_CFLAGS=-pipe
darwin_CXXFLAGS=$(darwin_CFLAGS)
darwin_ARFLAGS=cr

darwin_release_CFLAGS=-O1
darwin_release_CXXFLAGS=$(darwin_release_CFLAGS)

darwin_debug_CFLAGS=-O1
darwin_debug_CXXFLAGS=$(darwin_debug_CFLAGS)

darwin_native_toolchain=native_cctools
