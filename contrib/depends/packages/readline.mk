package=readline
$(package)_version=6.3
$(package)_download_path=ftp://ftp.cwru.edu/pub/bash/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=56ba6071b9462f980c5a72ab0023893b65ba6debb4eeb475d7a563dc65cafd43
$(package)_patches=readline-1.patch

define $(package)_set_vars
  $(package)_build_opts=CC="$($(package)_cc)"
  $(package)_config_env=AR="$($(package)_ar)" RANLIB="$($(package)_ranlib)" CC="$($(package)_cc)"
  $(package)_config_opts=--prefix=$(host_prefix)
  $(package)_config_opts+=--disable-shared --enable-multibye --without-purify --without-curses
  $(package)_config_opts_release=--disable-debug-mode
  $(package)_build_opts=CFLAGS="$($(package)_cflags) $($(package)_cppflags) -fPIC"
endef

define $(package)_config_cmds
  patch -p1 < $($(package)_patch_dir)/readline-1.patch &&\
  export bash_cv_have_mbstate_t=yes &&\
  export bash_cv_wcwidth_broken=yes &&\
  ./configure $($(package)_config_opts)
endef

define $(package)_build_cmds
  $(MAKE) $($(package)_build_opts)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
endef
