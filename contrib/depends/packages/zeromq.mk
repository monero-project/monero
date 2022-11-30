package=zeromq
$(package)_version=4.3.2
$(package)_download_path=https://github.com/zeromq/libzmq/releases/download/v$($(package)_version)/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=ebd7b5c830d6428956b67a0454a7f8cbed1de74b3b01e5c33c5378e22740f763
$(package)_dependencies=sodium
$(package)_patches=b3123a2fd1e77cbdceb5ee7a70e796063b5ee5b9.patch 87b81926aaaea70c84d5a5ea6eda982b3425ceeb.patch

define $(package)_set_vars
  $(package)_config_opts=--without-docs --enable-static=yes --enable-shared=no --with-libsodium=yes --with-pgm=no --with-norm=no --disable-perf --disable-Werror --disable-drafts --enable-option-checking
  $(package)_config_opts_linux=--with-pic
  $(package)_cxxflags_linux=-std=c++14
  $(package)_cxxflags_mingw32=-std=c++14
  $(package)_cxxflags_darwin=-std=c++14 -fvisibility=default
endef

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/b3123a2fd1e77cbdceb5ee7a70e796063b5ee5b9.patch && \
  patch -p1 < $($(package)_patch_dir)/87b81926aaaea70c84d5a5ea6eda982b3425ceeb.patch && \
  ./autogen.sh
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
