packages:=boost openssl zeromq libiconv

native_packages := native_ccache

hardware_packages := hidapi protobuf libusb
hardware_native_packages := native_protobuf

android_native_packages = android_ndk
android_packages = ncurses readline sodium

darwin_native_packages = native_biplist native_ds_store native_mac_alias $(hardware_native_packages)
darwin_packages = sodium ncurses readline $(hardware_packages)

# not really native...
freebsd_native_packages = freebsd_base
freebsd_packages = ncurses readline sodium

linux_packages = eudev ncurses readline sodium $(hardware_packages)
linux_native_packages = $(hardware_native_packages)
qt_packages = qt

ifeq ($(build_tests),ON)
packages += gtest
endif

ifneq ($(host_arch),riscv64)
linux_packages += unwind
endif

mingw32_packages = icu4c sodium $(hardware_packages)
mingw32_native_packages = $(hardware_native_packages)

ifneq ($(build_os),darwin)
darwin_native_packages += native_cctools native_cdrkit native_libdmg-hfsplus
endif

