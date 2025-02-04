native_packages := native_protobuf
packages := boost openssl zeromq unbound sodium protobuf

ifneq ($(host_os),android)
packages += libusb
endif

ifneq ($(host_os),freebsd)
ifneq ($(host_os),android)
packages += hidapi
endif
endif

ifneq ($(host_os),mingw32)
packages += ncurses readline
endif

linux_native_packages :=
linux_packages :=

freebsd_native_packages := freebsd_base
freebsd_packages :=

ifneq ($(build_os),darwin)
darwin_native_packages := darwin_sdk native_cctools native_libtapi
endif
darwin_packages :=

android_native_packages := android_ndk
android_packages :=
