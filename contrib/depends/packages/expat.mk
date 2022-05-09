package=expat
$(package)_version=2.4.1
$(package)_download_path=https://github.com/libexpat/libexpat/releases/download/R_2_4_1
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=2f9b6a580b94577b150a7d5617ad4643a4301a6616ff459307df3e225bcfbf40

define $(package)_set_vars
$(package)_config_opts=--enable-static
$(package)_config_opts=--disable-shared
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
  rm lib/*.la
endef

