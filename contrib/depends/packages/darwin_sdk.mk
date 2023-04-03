package=darwin_sdk
$(package)_version=12.3
$(package)_download_path=https://github.com/joseluisq/macosx-sdks/releases/download/$($(package)_version)/
$(package)_file_name=MacOSX$($(package)_version).sdk.tar.xz
$(package)_sha256_hash=3abd261ceb483c44295a6623fdffe5d44fc4ac2c872526576ec5ab5ad0f6e26c

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_dir)/$(host_prefix)/native/SDK &&\
  mv * $($(package)_staging_dir)/$(host_prefix)/native/SDK &&\
  export SDKROOT=$($($(package)_staging_dir)/$(host_prefix)/native/SDK)
endef