package=libusb
$(package)_version=1.0.22
$(package)_download_path=https://sourceforge.net/projects/libusb/files/libusb-1.0/libusb-$($(package)_version)/
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=75aeb9d59a4fdb800d329a545c2e6799f732362193b465ea198f2aa275518157

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
