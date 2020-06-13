package=expat
$(package)_version=2.2.4
$(package)_download_path=https://downloads.sourceforge.net/project/expat/expat/$($(package)_version)
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=03ad85db965f8ab2d27328abcf0bc5571af6ec0a414874b2066ee3fdd372019e

define $(package)_set_vars
$(package)_config_opts=--enable-static
$(package)_config_opts+=--prefix=$(host_prefix)
endef

define $(package)_config_cmds
  $($(package)_autoconf) $($(package)_config_opts)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
