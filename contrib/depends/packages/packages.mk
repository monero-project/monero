native_packages :=
packages := boost openssl zeromq unbound sodium

ifneq ($(host_os),mingw32)
packages += ncurses readline
endif

wallet_native_packages := native_protobuf
wallet_packages = protobuf

ifneq ($(host_os),android)
wallet_packages += libusb
endif

ifneq ($(host_os),freebsd)
ifneq ($(host_os),android)
wallet_packages += hidapi
endif
endif

linux_native_packages :=
linux_packages :=

freebsd_native_packages := freebsd_base
freebsd_packages :=

ifneq ($(build_os),darwin)
darwin_native_packages := darwin_sdk
endif
darwin_packages :=

android_native_packages := android_ndk
android_packages :=
