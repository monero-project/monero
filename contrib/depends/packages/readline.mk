package=readline
$(package)_version=8.3
$(package)_download_path=https://ftp.gnu.org/gnu/readline
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=fe5383204467828cd495ee8d1d3c037a7eba1389c22bc6a041f627976f9061cc
$(package)_dependencies=ncurses

define $(package)_set_vars
  $(package)_build_opts=CC="$($(package)_cc)"
  $(package)_config_opts+=--prefix=$(host_prefix)
  $(package)_config_opts+=--exec-prefix=$(host_prefix)
  $(package)_config_opts+=--host=$(HOST)
  $(package)_config_opts+=--disable-shared --with-curses
  $(package)_config_opts_release=--disable-debug-mode
  $(package)_build_opts=CFLAGS="$($(package)_cflags) $($(package)_cppflags) -fPIC"
endef

define $(package)_preprocess_cmds
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub support
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE) $($(package)_build_opts)
endef

define $(package)_stage_cmds
  $(MAKE) install DESTDIR=$($(package)_staging_dir) prefix=$(host_prefix) exec-prefix=$(host_prefix)
endef

