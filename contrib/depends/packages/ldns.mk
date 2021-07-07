package=ldns
$(package)_version=1.7.1
$(package)_download_path=https://www.nlnetlabs.nl/downloads/$(package)/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=8ac84c16bdca60e710eea75782356f3ac3b55680d40e1530d7cea474ac208229
$(package)_dependencies=openssl

define $(package)_set_vars
  $(package)_config_opts=--disable-shared --enable-static --with-drill
  $(package)_config_opts+=--with-ssl=$(host_prefix)
  $(package)_config_opts_release=--disable-debug-mode
  $(package)_config_opts_linux=--with-pic
endef

define $(package)_preprocess_cmds
   cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub .
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install-h install-lib
endef

define $(package)_postprocess_cmds
  rm lib/*.la
endef

