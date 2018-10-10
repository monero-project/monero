package=sodium-darwin
$(package)_version=1.0.15
$(package)_download_path=https://download.libsodium.org/libsodium/releases/
$(package)_file_name=libsodium-$($(package)_version).tar.gz
$(package)_sha256_hash=fb6a9e879a2f674592e4328c5d9f79f082405ee4bb05cb6e679b90afe9e178f4

define $(package)_set_vars
$(package)_build_opts_darwin=OS=Darwin LIBTOOL="$($(package)_libtool)"
$(package)_config_opts=--enable-static --disable-shared
$(package)_config_opts+=--prefix=$(host_prefix)
endef

define $(package)_config_cmds
  ./autogen.sh &&\
  $($(package)_autoconf) $($(package)_config_opts) RANLIB="$(host_prefix)/native/bin/x86_64-apple-darwin11-ranlib" AR="$(host_prefix)/native/bin/x86_64-apple-darwin11-ar" CC="$(host_prefix)/native/bin/$($(package)_cc)"
endef

define $(package)_build_cmds
  echo "path is problematic here" &&\
  make
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
