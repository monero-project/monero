package=expat
$(package)_version=2.6.0
$(package)_download_path=https://github.com/libexpat/libexpat/releases/download/R_$(subst .,_,$($(package)_version))/
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=ff60e6a6b6ce570ae012dc7b73169c7fdf4b6bf08c12ed0ec6f55736b78d85ba

define $(package)_set_vars
$(package)_config_opts=--disable-shared --without-docbook --without-tests --without-examples
$(package)_config_opts+=--enable-option-checking --without-xmlwf --with-pic
$(package)_config_opts+=--prefix=$(host_prefix)
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  rm -rf share lib/cmake lib/*.la
endef

