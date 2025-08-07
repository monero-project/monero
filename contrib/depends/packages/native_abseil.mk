package=native_abseil
$(package)_version=20250512.0
$(package)_download_path=https://github.com/abseil/abseil-cpp/archive/refs/tags/
$(package)_download_file=$($(package)_version).tar.gz
$(package)_file_name=abseil-$($(package)_version).tar.gz
$(package)_sha256_hash=7262daa7c1711406248c10f41026d685e88223bc92817d16fb93c19adb57f669

define $(package)_set_vars
  $(package)_cxxflags+=-std=c++17
endef

define $(package)_config_cmds
  $($(package)_cmake)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
