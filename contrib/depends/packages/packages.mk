packages:=boost openssl libevent zeromq cppzmq zlib expat ldns unbound cppzmq readline libiconv icu4c
native_packages := native_ccache

wallet_packages=bdb

upnp_packages=miniupnpc

darwin_native_packages = native_biplist native_ds_store native_mac_alias

ifeq ($(host_os),linux)
packages += unwind
endif
ifeq ($(host_os),darwin11)
package += unwind
endif

ifneq ($(build_os),darwin)
darwin_native_packages += native_cctools native_cdrkit native_libdmg-hfsplus
packages += readline
endif


