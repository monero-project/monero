package=android_ndk
$(package)_version=30-beta3
$(package)_download_path=https://dl.google.com/android/repository/
$(package)_file_name=android-ndk-r$($(package)_version)-linux.zip
$(package)_sha256_hash=2698cca1e9f161048ecd84e1e70a556e1aa00b78409473d4c1e87969d40c3efc

# We temporarily use a release candidate of the NDK because the include headers in earlier versions use
# compiler builtins that are not supported by the version of Clang provided by StageX.
# Todo: update the NDK before cutting a release.

define $(package)_extract_cmds
  echo $($(package)_sha256_hash) $($(1)_source_dir)/$($(package)_file_name) | sha256sum -c &&\
  unzip -q $($(1)_source_dir)/$($(package)_file_name)
endef

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir) && \
  mv android-ndk-r$($(package)_version)/toolchains/llvm/prebuilt/linux-x86_64/* $($(package)_staging_prefix_dir) && \
  touch $($(package)_staging_prefix_dir)/sysroot/usr/include/stdc-predef.h
endef

ifneq ($(STAGEX_ENVIRONMENT),)
define $(package)_postprocess_cmds
  rm -rf bin
endef
endif
