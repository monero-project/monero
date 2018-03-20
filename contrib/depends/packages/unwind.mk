package=unwind
$(package)_version=1.2
$(package)_download_path=http://download.savannah.nongnu.org/releases/libunwind
$(package)_file_name=lib$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=1de38ffbdc88bd694d10081865871cd2bfbb02ad8ef9e1606aee18d65532b992

define $(package)_config_cmds
 $($(package)_autoconf) --disable-shared --enable-static
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
endef
