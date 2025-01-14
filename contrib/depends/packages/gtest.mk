package=gtest
$(package)_version=1.15.2
$(package)_download_path=https://github.com/google/googletest/archive/
$(package)_download_file=v$($(package)_version).tar.gz
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926

define $(package)_set_vars
  $(package)_config_opts := -DBUILD_GMOCK=OFF
endef

define $(package)_config_cmds
  $($(package)_cmake) -S . -B .
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
