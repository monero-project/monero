package=hidapi
$(package)_version=0.9.0
$(package)_download_path=https://github.com/libusb/hidapi/archive
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=630ee1834bdd5c5761ab079fd04f463a89585df8fcae51a7bfe4229b1e02a652
$(package)_linux_dependencies=libusb eudev

define $(package)_set_vars
$(package)_config_opts=--enable-static --disable-shared
$(package)_config_opts+=--prefix=$(host_prefix)
$(package)_config_opts_darwin+=RANLIB="$(host_prefix)/native/bin/x86_64-apple-darwin11-ranlib" AR="$(host_prefix)/native/bin/x86_64-apple-darwin11-ar" CC="$(host_prefix)/native/bin/$($(package)_cc)"
$(package)_config_opts_linux+=libudev_LIBS="-L$(host_prefix)/lib -ludev"
$(package)_config_opts_linux+=libudev_CFLAGS=-I$(host_prefix)/include
$(package)_config_opts_linux+=libusb_LIBS="-L$(host_prefix)/lib -lusb-1.0"
$(package)_config_opts_linux+=libusb_CFLAGS=-I$(host_prefix)/include/libusb-1.0
$(package)_config_opts_linux+=--with-pic
endef

define $(package)_config_cmds
  ./bootstrap &&\
  $($(package)_autoconf) $($(package)_config_opts) AR_FLAGS=$($(package)_arflags)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  rm lib/*.la
endef

