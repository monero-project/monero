package=libiconv
$(package)_version=1.15
$(package)_download_path=https://ftp.gnu.org/gnu/libiconv
$(package)_file_name=libiconv-$($(package)_version).tar.gz
$(package)_sha256_hash=ccf536620a45458d26ba83887a983b96827001e92a13847b45e4925cc8913178

define $(package)_config_cmds
  $($(package)_autoconf) --disable-nls --enable-static --disable-shared
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
