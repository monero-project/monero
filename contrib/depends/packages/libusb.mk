package=libusb
$(package)_version=1.0.26
$(package)_download_path=https://github.com/libusb/libusb/releases/download/v$($(package)_version)
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=12ce7a61fc9854d1d2a1ffe095f7b5fac19ddba095c259e6067a46500381b5a5

define $(package)_preprocess_cmds
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
