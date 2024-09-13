package=native_abseil
$(package)_version=20260526.0
$(package)_download_path=https://github.com/abseil/abseil-cpp/releases/download/$($(package)_version)/
$(package)_file_name=abseil-cpp-$($(package)_version).tar.gz
$(package)_sha256_hash=6e1aee535473414164bf83e4ebc40240dec71a4701f8a642d906e95bea1aea0c

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
