packages:=boost openssl libevent zeromq cppzmq zlib expat ldns cppzmq readline libiconv qt
native_packages := native_ccache

wallet_packages=bdb

darwin_native_packages = native_biplist native_ds_store native_mac_alias
darwin_packages += sodium-darwin

ifeq ($(host_os),linux)
packages += pcsc-lite
packages += unwind
packages += sodium
endif
ifeq ($(host_os),mingw32)
packages += icu4c
packages += sodium
endif

ifneq ($(build_os),darwin)
darwin_native_packages += native_cctools native_cdrkit native_libdmg-hfsplus
packages += readline
endif

