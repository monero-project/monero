native_packages := native_abseil native_protobuf
packages := boost openssl zeromq expat unbound sodium abseil protobuf

linux_native_packages :=
linux_packages := ncurses readline eudev libusb hidapi

mingw32_native_packages :=
mingw32_packages := hidapi

freebsd_native_packages := freebsd_base
freebsd_packages := ncurses readline hidapi

ifneq ($(build_os),darwin)
darwin_native_packages := darwin_sdk native_cctools native_libtapi
endif
darwin_packages := ncurses readline hidapi

android_native_packages := android_ndk
android_packages := ncurses readline

ifeq ($(build_tests),ON)
packages += gtest
endif
