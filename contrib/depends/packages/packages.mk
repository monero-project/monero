ifeq ($(host_os),android)
packages:=boost openssl zeromq libiconv
else
packages:=boost openssl zeromq lexpat dns libiconv hidapi protobuf libusb
endif

native_packages := native_ccache native_protobuf

android_native_packages = android_ndk
android_packages = ncurses readline sodium

darwin_native_packages = native_biplist native_ds_store native_mac_alias
darwin_packages = sodium-darwin ncurses readline 

linux_packages = eudev ncurses readline sodium
qt_packages = qt

ifeq ($(build_tests),ON)
packages += gtest
endif

ifneq ($(host_arch),riscv64)
linux_packages += unwind
endif

ifeq ($(host_os),mingw32)
packages += icu4c
packages += sodium
endif

ifneq ($(build_os),darwin)
darwin_native_packages += native_cctools native_cdrkit native_libdmg-hfsplus
endif

