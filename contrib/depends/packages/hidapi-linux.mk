package=hidapi-linux
$(package)_version=0.8.0-rc1
$(package)_download_path=https://github.com/signal11/hidapi/archive
$(package)_file_name=hidapi-$($(package)_version).tar.gz
$(package)_sha256_hash=3c147200bf48a04c1e927cd81589c5ddceff61e6dac137a605f6ac9793f4af61
$(package)_dependencies=libusb eudev

define $(package)_set_vars
$(package)_config_opts=--enable-static
$(package)_config_opts+=--prefix=$(host_prefix)
$(package)_config_opts+=libudev_LIBS="-L$(host_prefix)/lib -ludev"
$(package)_config_opts+=libudev_CFLAGS=-I$(host_prefix)/include
$(package)_config_opts+=libusb_LIBS="-L$(host_prefix)/lib -lusb-1.0"
$(package)_config_opts+=libusb_CFLAGS=-I$(host_prefix)/include/libusb-1.0
endef

define $(package)_config_cmds
  ./bootstrap &&\
  $($(package)_autoconf) $($(package)_config_opts)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
