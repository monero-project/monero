package=sodium
$(package)_version=1.0.16
$(package)_download_path=https://github.com/jedisct1/libsodium/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=0c14604bbeab2e82a803215d65c3b6e74bb28291aaee6236d65c699ccfe1a98c

define $(package)_set_vars
$(package)_config_opts=--enable-static
$(package)_config_opts+=--prefix=$(host_prefix)
endef

define $(package)_config_cmds
  ./autogen.sh &&\
  $($(package)_autoconf) $($(package)_config_opts)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
