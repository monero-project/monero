package=libusb
$(package)_version=1.0.27
$(package)_download_path=https://github.com/libusb/libusb/releases/download/v$($(package)_version)
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=ffaa41d741a8a3bee244ac8e54a72ea05bf2879663c098c82fc5757853441575

define $(package)_preprocess_cmds
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub . && \
  autoreconf -i
endef

define $(package)_set_vars
  $(package)_config_opts=--disable-shared
  $(package)_config_opts_linux=--with-pic --disable-udev
  $(package)_config_opts_mingw32=--disable-udev
  $(package)_config_opts_darwin=--disable-udev
  $(package)_config_opts_freebsd=--with-pic --disable-udev
endef

define $(package)_config_cmds
  $($(package)_autoconf) AR_FLAGS=$($(package)_arflags)
endef

define $(package)_build_cmd
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  cp -f lib/libusb-1.0.a lib/libusb.a
endef
