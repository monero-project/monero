package=libusb
$(package)_version=1.0.9
$(package)_download_path=http://sourceforge.net/projects/libusb/files/libusb-1.0/libusb-1.0.9/
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=e920eedc2d06b09606611c99ec7304413c6784cba6e33928e78243d323195f9b

define $(package)_preprocess_cmds
  autoreconf -i
endef

define $(package)_set_vars
  $(package)_config_opts=--disable-shared
  $(package)_config_opts_linux=--with-pic
endef

define $(package)_config_cmds
  cp -f $(BASEDIR)/config.guess config.guess &&\
  cp -f $(BASEDIR)/config.sub config.sub &&\
  $($(package)_autoconf)
endef

define $(package)_build_cmd
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds                                                                                                          cp -f lib/libusb-1.0.a lib/libusb.a
endef
