package=native_abseil
$(package)_version=20250814.1
$(package)_download_path=https://github.com/abseil/abseil-cpp/archive/refs/tags/
$(package)_download_file=$($(package)_version).tar.gz
$(package)_file_name=abseil-$($(package)_version).tar.gz
$(package)_sha256_hash=1692f77d1739bacf3f94337188b78583cf09bab7e420d2dc6c5605a4f86785a1

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
