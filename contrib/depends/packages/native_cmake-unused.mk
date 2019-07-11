package=native_cmake
$(package)_version=3.14.0
$(package)_version_dot=v3.14
$(package)_download_path=https://cmake.org/files/$($(package)_version_dot)/
$(package)_file_name=cmake-$($(package)_version).tar.gz
$(package)_sha256_hash=aa76ba67b3c2af1946701f847073f4652af5cbd9f141f221c97af99127e75502

define $(package)_set_vars
$(package)_config_opts=
endef

define $(package)_config_cmds
  ./bootstrap &&\
  ./configure $($(package)_config_opts)
endef

define $(package)_build_cmd
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
