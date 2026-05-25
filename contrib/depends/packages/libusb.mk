package=libusb
$(package)_version=1.0.30
$(package)_download_path=https://github.com/libusb/libusb/releases/download/v$($(package)_version)
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=fea36f34f9156400209595e300840767ab1a385ede1dc7ee893015aea9c6dbaf

define $(package)_set_vars
  $(package)_config_opts=--disable-shared --disable-udev
endef

# Remove blobs
define $(package)_preprocess_cmds
  rm -rf tests/fuzz/corpus && \
  rm doc/libusb.png && \
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub .
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
