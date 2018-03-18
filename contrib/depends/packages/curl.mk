package=curl
$(package)_version=7.55.0
$(package)_download_path=https://curl.haxx.se/download/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=dae1b1be34f5983e8d46917f2bdbb2335aecd0e57f777f4c32213da6a8050a80

define $(package)_set_vars
  $(package)_config_opts=--disable-shared
  $(package)_config_opts+= --enable-static
  $(package)_config_opts_release+=--disable-debug-mode
  $(package)_config_opts_linux+=--with-pic
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
endef
