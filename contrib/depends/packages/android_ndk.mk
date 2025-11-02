package=android_ndk
$(package)_version=29
$(package)_download_path=https://dl.google.com/android/repository/
$(package)_file_name=android-ndk-r$($(package)_version)-linux.zip
$(package)_sha256_hash=4abbbcdc842f3d4879206e9695d52709603e52dd68d3c1fff04b3b5e7a308ecf

define $(package)_extract_cmds
  echo $($(package)_sha256_hash) $($(1)_source_dir)/$($(package)_file_name) | sha256sum -c &&\
  unzip -q $($(1)_source_dir)/$($(package)_file_name)
endef

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir) && \
  mv android-ndk-r$($(package)_version)/toolchains/llvm/prebuilt/linux-x86_64/* $($(package)_staging_prefix_dir)
endef
