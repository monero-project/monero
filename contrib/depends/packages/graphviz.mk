package=graphviz
$(package)_version=2.40.1
$(package)_download_path=www.graphviz.org/pub/graphviz/stable/SOURCES/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=ca5218fade0204d59947126c38439f432853543b0818d9d728c589dfe7f3a421

define $(package)_preprocess_cmds
  ./autogen.sh
endef

define $(package)_set_vars
  $(package)_config_opts=--disable-shared --enable-multibye --without-purify --without-curses
  $(package)_config_opts_release=--disable-debug-mode
  $(package)_config_opts_linux=--with-pic
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
endef
