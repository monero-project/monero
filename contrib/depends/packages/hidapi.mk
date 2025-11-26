package=hidapi
$(package)_version=0.15.0
$(package)_download_path=https://github.com/libusb/hidapi/archive/refs/tags
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=5d84dec684c27b97b921d2f3b73218cb773cf4ea915caee317ac8fc73cef8136
$(package)_linux_dependencies=libusb eudev

define $(package)_set_vars
$(package)_config_opts=--enable-static --disable-shared
$(package)_config_opts+=--prefix=$(host_prefix)
$(package)_config_opts_linux+=libudev_LIBS="-L$(host_prefix)/lib -ludev"
$(package)_config_opts_linux+=libudev_CFLAGS=-I$(host_prefix)/include
$(package)_config_opts_linux+=libusb_LIBS="-L$(host_prefix)/lib -lusb-1.0"
$(package)_config_opts_linux+=libusb_CFLAGS=-I$(host_prefix)/include/libusb-1.0
$(package)_config_opts_linux+=--with-pic
endef

define $(package)_preprocess_cmds
  ./bootstrap
endef

define $(package)_config_cmds
  $($(package)_autoconf) AR_FLAGS=$($(package)_arflags)
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

