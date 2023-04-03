package=libiconv
$(package)_version=1.15
$(package)_download_path=https://ftp.gnu.org/gnu/libiconv
$(package)_file_name=libiconv-$($(package)_version).tar.gz
$(package)_sha256_hash=ccf536620a45458d26ba83887a983b96827001e92a13847b45e4925cc8913178
$(package)_patches=fix-whitespace.patch

define $(package)_set_vars
  $(package)_config_opts=--disable-nls
  $(package)_config_opts=--enable-static
  $(package)_config_opts=--disable-shared
  $(package)_config_opts_linux=--with-pic
endef

define $(package)_preprocess_cmds
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub build-aux/ &&\
  patch -p1 < $($(package)_patch_dir)/fix-whitespace.patch
endef

define $(package)_config_cmds
  $($(package)_autoconf) AR_FLAGS=$($(package)_arflags)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
endef
