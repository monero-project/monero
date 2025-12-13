package=android_ndk
$(package)_version=27d
$(package)_download_path=https://dl.google.com/android/repository/
$(package)_file_name=android-ndk-r$($(package)_version)-linux.zip
$(package)_sha256_hash=601246087a682d1944e1e16dd85bc6e49560fe8b6d61255be2829178c8ed15d9

define $(package)_extract_cmds
  echo $($(package)_sha256_hash) $($(1)_source_dir)/$($(package)_file_name) | sha256sum -c &&\
  unzip -q $($(1)_source_dir)/$($(package)_file_name)
endef

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir) && \
  mv android-ndk-r$($(package)_version)/toolchains/llvm/prebuilt/linux-x86_64/* $($(package)_staging_prefix_dir)
endef
