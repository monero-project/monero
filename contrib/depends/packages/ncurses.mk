package=ncurses
$(package)_version=6.1
$(package)_download_path=https://ftp.gnu.org/gnu/ncurses
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=aa057eeeb4a14d470101eff4597d5833dcef5965331be3528c08d99cebaa0d17
$(package)_patches=fallback.c

define $(package)_set_vars
  $(package)_build_opts=CC="$($(package)_cc)"
  $(package)_config_env=AR="$($(package)_ar)" RANLIB="$($(package)_ranlib)" CC="$($(package)_cc)" ARFLAGS=$($(package)_arflags) cf_cv_ar_flags=""
  $(package)_config_env_darwin=RANLIB="$(host_prefix)/native/bin/x86_64-apple-darwin11-ranlib" AR="$(host_prefix)/native/bin/x86_64-apple-darwin11-ar" CC="$(host_prefix)/native/bin/$($(package)_cc)"
  $(package)_config_opts=--prefix=$(host_prefix)
  $(package)_config_opts+=--disable-shared
  $(package)_config_opts+=--with-build-cc=gcc
  $(package)_config_opts+=--without-debug
  $(package)_config_opts+=--without-ada
  $(package)_config_opts+=--without-cxx-binding
  $(package)_config_opts+=--without-cxx
  $(package)_config_opts+=--without-ticlib
  $(package)_config_opts+=--without-tic
  $(package)_config_opts+=--without-progs
  $(package)_config_opts+=--without-tests
  $(package)_config_opts+=--without-tack
  $(package)_config_opts+=--without-manpages
  $(package)_config_opts+=--with-termlib=tinfo
  $(package)_config_opts+=--disable-tic-depends
  $(package)_config_opts+=--disable-big-strings
  $(package)_config_opts+=--disable-ext-colors
  $(package)_config_opts+=--enable-pc-files
  $(package)_config_opts+=--host=$(HOST)
  $(pacakge)_config_opts+=--without-shared
  $(pacakge)_config_opts+=--without-pthread
  $(pacakge)_config_opts+=--disable-rpath
  $(pacakge)_config_opts+=--disable-colorfgbg
  $(pacakge)_config_opts+=--disable-ext-mouse
  $(pacakge)_config_opts+=--disable-symlinks
  $(pacakge)_config_opts+=--enable-warnings
  $(pacakge)_config_opts+=--enable-assertions
  $(package)_config_opts+=--with-default-terminfo-dir=/etc/_terminfo_
  $(package)_config_opts+=--with-terminfo-dirs=/etc/_terminfo_
  $(pacakge)_config_opts+=--enable-database
  $(pacakge)_config_opts+=--enable-sp-funcs
  $(pacakge)_config_opts+=--disable-term-driver
  $(pacakge)_config_opts+=--enable-interop
  $(pacakge)_config_opts+=--enable-widec
  $(package)_build_opts=CFLAGS="$($(package)_cflags) $($(package)_cppflags) -fPIC"
endef

define $(package)_preprocess_cmds
  cp $($(package)_patch_dir)/fallback.c ncurses
endef

define $(package)_config_cmds
  ./configure $($(package)_config_opts)
endef

define $(package)_build_cmds
  $(MAKE) $($(package)_build_opts) V=1
endef

define $(package)_stage_cmds
  $(MAKE) install.libs DESTDIR=$($(package)_staging_dir)
endef

