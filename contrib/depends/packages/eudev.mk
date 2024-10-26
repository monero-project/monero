package=eudev
$(package)_version=3.2.14
$(package)_download_path=https://github.com/eudev-project/eudev/releases/download/v$($(package)_version)/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=8da4319102f24abbf7fff5ce9c416af848df163b29590e666d334cc1927f006f

define $(package)_set_vars
  $(package)_config_opts=--disable-gudev --disable-introspection --disable-hwdb --disable-manpages --disable-shared
endef

define $(package)_config_cmds
  $($(package)_autoconf) AR_FLAGS=$($(package)_arflags)
endef

define $(package)_build_cmd
  $(MAKE)
endef

define $(package)_preprocess_cmds
  cd $($(package)_build_subdir); autoreconf -f -i
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  rm lib/*.la
endef
