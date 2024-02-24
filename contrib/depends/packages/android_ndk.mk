package=android_ndk
$(package)_version=18b
$(package)_download_path=https://dl.google.com/android/repository/
$(package)_file_name=android-ndk-r$($(package)_version)-linux-x86_64.zip
$(package)_sha256_hash=4f61cbe4bbf6406aa5ef2ae871def78010eed6271af72de83f8bd0b07a9fd3fd
$(package)_patches=api_definition.patch

define $(package)_set_vars
$(package)_config_opts_arm=--arch arm
$(package)_config_opts_aarch64=--arch arm64
endef

define $(package)_extract_cmds
  echo $($(package)_sha256_hash) $($(1)_source_dir)/$($(package)_file_name) | sha256sum -c &&\
  unzip -q $($(1)_source_dir)/$($(package)_file_name)
endef

define $(package)_preprocess_cmds
  cd android-ndk-r$($(package)_version) && \
  patch -p1 < $($(package)_patch_dir)/api_definition.patch
endef

define $(package)_stage_cmds
  android-ndk-r$($(package)_version)/build/tools/make_standalone_toolchain.py --api 21 \
    --install-dir $(build_prefix) --stl=libc++ $($(package)_config_opts) &&\
  mv $(build_prefix) $($(package)_staging_dir)/$(host_prefix)
endef

