package=native_abseil
$(package)_version=20260107.1
$(package)_download_path=https://github.com/abseil/abseil-cpp/releases/download/$($(package)_version)/
$(package)_file_name=abseil-cpp-$($(package)_version).tar.gz
$(package)_sha256_hash=4314e2a7cbac89cac25a2f2322870f343d81579756ceff7f431803c2c9090195

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
