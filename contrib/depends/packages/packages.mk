packages:=boost openssl zeromq cppzmq expat ldns readline libiconv hidapi protobuf libusb
native_packages := native_ccache native_protobuf

darwin_native_packages = native_biplist native_ds_store native_mac_alias
darwin_packages = sodium-darwin

linux_packages = eudev
qt_packages = qt

ifeq ($(build_tests),ON)
packages += gtest
endif

ifeq ($(host_os),linux)
packages += unwind
packages += sodium
endif
ifeq ($(host_os),mingw32)
packages += icu4c
packages += sodium
endif

ifneq ($(build_os),darwin)
darwin_native_packages += native_cctools native_cdrkit native_libdmg-hfsplus
endif

