package=abseil
$(package)_version=$(native_$(package)_version)
$(package)_download_path=$(native_$(package)_download_path)
$(package)_download_file=$(native_$(package)_download_file)
$(package)_file_name=$(native_$(package)_file_name)
$(package)_sha256_hash=$(native_$(package)_sha256_hash)
$(package)_patches=no_librt.patch

define $(package)_set_vars
  $(package)_cxxflags+=-std=c++17
endef

define $(package)_preprocess_cmds
   patch -p1 -i $($(package)_patch_dir)/no_librt.patch
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
