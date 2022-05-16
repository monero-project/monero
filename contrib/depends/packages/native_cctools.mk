package=native_cctools
$(package)_version=04663295d0425abfac90a42440a7ec02d7155fea
$(package)_download_path=https://github.com/tpoechtrager/cctools-port/archive
$(package)_download_file=$($(package)_version).tar.gz
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=70a7189418c2086d20c299c5d59250cf5940782c778892ccc899c66516ed240e
$(package)_build_subdir=cctools
$(package)_dependencies=native_clang native_libtapi

define $(package)_set_vars
$(package)_config_opts=--target=$(host) --disable-lto-support --with-libtapi=$(host_prefix)
$(package)_ldflags+=-Wl,-rpath=\\$$$$$$$$\$$$$$$$$ORIGIN/../lib
$(package)_cc=$(host_prefix)/native/bin/clang
$(package)_cxx=$(host_prefix)/native/bin/clang++
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install && \
  cp $($(package)_extract_dir)/cctools/misc/install_name_tool $($(package)_staging_prefix_dir)/bin/
endef
