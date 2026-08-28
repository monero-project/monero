package=native_abseil
$(package)_version=20260817.0
$(package)_download_path=https://github.com/abseil/abseil-cpp/releases/download/$($(package)_version)/
$(package)_file_name=abseil-cpp-$($(package)_version).tar.gz
$(package)_sha256_hash=f7e05179df39c45434cad433f5783840bb3788ef322976f9138bc6b72b3a107d

define $(package)_set_vars
  $(package)_cxxflags+=-std=c++17
endef

define $(package)_config_cmds
  $($(package)_cmake) .
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
