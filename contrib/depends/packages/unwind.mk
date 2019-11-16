package=unwind
$(package)_version=1.2
$(package)_download_path=https://download.savannah.nongnu.org/releases/libunwind
$(package)_file_name=lib$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=1de38ffbdc88bd694d10081865871cd2bfbb02ad8ef9e1606aee18d65532b992
$(package)_patches=fix_obj_order.patch

define $(package)_preprocess_cmds
  patch -p0 < $($(package)_patch_dir)/fix_obj_order.patch
endef

define $(package)_config_cmds
  cp -f $(BASEDIR)/config.guess config/config.guess &&\
  cp -f $(BASEDIR)/config.sub config/config.sub &&\
  $($(package)_autoconf) --disable-shared --enable-static AR_FLAGS=$($(package)_arflags)
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

