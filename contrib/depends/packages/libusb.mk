package=libusb
$(package)_version=1.0.26
$(package)_download_path=https://github.com/libusb/libusb/releases/download/v$($(package)_version)
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=12ce7a61fc9854d1d2a1ffe095f7b5fac19ddba095c259e6067a46500381b5a5
$(package)_patches=fix_osx_avail.patch compat.cpp

#$(package)_version=1.0.22
#$(package)_download_path=https://github.com/libusb/libusb/releases/download/v$($(package)_version)
#$(package)_file_name=$(package)-$($(package)_version).tar.bz2
#$(package)_sha256_hash=75aeb9d59a4fdb800d329a545c2e6799f732362193b465ea198f2aa275518157

#$(package)_version=1.0.25
#$(package)_download_path=https://github.com/libusb/libusb/releases/download/v$($(package)_version)
#$(package)_file_name=$(package)-$($(package)_version).tar.bz2
#$(package)_sha256_hash=8a28ef197a797ebac2702f095e81975e2b02b2eeff2774fa909c78a74ef50849

# Works with OSX.SDK 11.1, clang-9
#$(package)_version=1.0.23
#$(package)_download_path=https://github.com/libusb/libusb/releases/download/v$($(package)_version)
#$(package)_file_name=$(package)-$($(package)_version).tar.bz2
#$(package)_sha256_hash=db11c06e958a82dac52cf3c65cb4dd2c3f339c8a988665110e0d24d19312ad8d

#define $(package)_preprocess_cmds
#  patch -p1 < $($(package)_patch_dir)/fix_osx_avail.patch &&\
#  autoreconf -i
#endef
#cat $(PATCHES_PATH)/../libs/osx-cross-compile/compat.cpp libusb/os/darwin_usb.c.o > libusb/os/darwin_usb.c &&\

define $(package)_preprocess_cmds
  cp libusb/os/darwin_usb.c libusb/os/darwin_usb.c.o &&\
  cat $($(package)_patch_dir)/compat.cpp libusb/os/darwin_usb.c.o > libusb/os/darwin_usb.c &&\
  autoreconf -i
endef

define $(package)_set_vars
  $(package)_config_opts=--disable-shared
  $(package)_config_opts_linux=--with-pic --disable-udev
  $(package)_config_opts_mingw32=--disable-udev
  $(package)_config_opts_darwin=--disable-udev
endef

ifneq ($(host_os),darwin)
  define $(package)_config_cmds
    cp -f $(BASEDIR)/config.guess config.guess &&\
    cp -f $(BASEDIR)/config.sub config.sub &&\
    $($(package)_autoconf) AR_FLAGS=$($(package)_arflags)
  endef
else
  define $(package)_config_cmds
    $($(package)_autoconf) AR_FLAGS=$($(package)_arflags)
  endef
endif

define $(package)_build_cmd
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds  cp -f lib/libusb-1.0.a lib/libusb.a
endef
