package=android_ndk
$(package)_version=27
$(package)_download_path=https://dl.google.com/android/repository/
$(package)_file_name=android-ndk-r$($(package)_version)-linux.zip
$(package)_sha256_hash=2f17eb8bcbfdc40201c0b36e9a70826fcd2524ab7a2a235e2c71186c302da1dc

define $(package)_extract_cmds
  echo $($(package)_sha256_hash) $($(1)_source_dir)/$($(package)_file_name) | sha256sum -c &&\
  unzip -q $($(1)_source_dir)/$($(package)_file_name)
endef

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/toolchain && \
  mv android-ndk-r$($(package)_version)/toolchains/llvm/prebuilt/linux-x86_64/* $($(package)_staging_prefix_dir)/toolchain
endef
