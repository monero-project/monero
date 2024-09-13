package=native_abseil
$(package)_version=20260107.0
$(package)_download_path=https://github.com/abseil/abseil-cpp/releases/download/$($(package)_version)/
$(package)_file_name=abseil-cpp-$($(package)_version).tar.gz
$(package)_sha256_hash=4c124408da902be896a2f368042729655709db5e3004ec99f57e3e14439bc1b2

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
