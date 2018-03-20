package=ldns
$(package)_version=1.6.17
$(package)_download_path=http://www.nlnetlabs.nl/downloads/ldns/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=8b88e059452118e8949a2752a55ce59bc71fa5bc414103e17f5b6b06f9bcc8cd
$(package)_dependencies=openssl

define $(package)_set_vars
  $(package)_config_opts=--disable-shared --enable-static --disable-dane-ta-usage --with-drill 
  $(package)_config_opts=--with-ssl=$(host_prefix) 
  $(package)_config_opts_release=--disable-debug-mode
  $(package)_config_opts_linux=--with-pic
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
endef
