package=native_abseil
$(package)_version=20240722.0
$(package)_download_path=https://github.com/abseil/abseil-cpp/archive/refs/tags/
$(package)_download_file=$($(package)_version).tar.gz
$(package)_file_name=abseil-$($(package)_version).tar.gz
$(package)_sha256_hash=f50e5ac311a81382da7fa75b97310e4b9006474f9560ac46f54a9967f07d4ae3

define $(package)_config_cmds
  $($(package)_cmake)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
