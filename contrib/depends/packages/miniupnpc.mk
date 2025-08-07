package=miniupnpc
$(package)_version=2.3.3
$(package)_download_path=https://miniupnp.tuxfamily.org/files/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=d52a0afa614ad6c088cc9ddff1ae7d29c8c595ac5fdd321170a05f41e634bd1a
$(package)_build_subdir=build

define $(package)_set_vars
  $(package)_config_opts := -DUPNPC_BUILD_SAMPLE=OFF
  $(package)_config_opts += -DUPNPC_BUILD_SHARED=OFF
  $(package)_config_opts += -DUPNPC_BUILD_STATIC=ON
  $(package)_config_opts += -DUPNPC_BUILD_TESTS=OFF
  $(package)_config_opts_mingw32 := -DMINIUPNPC_TARGET_WINDOWS_VERSION=0x0601
endef

define $(package)_config_cmds
  $($(package)_cmake) -S .. -B .
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  cmake --install . --prefix $($(package)_staging_prefix_dir)
endef
