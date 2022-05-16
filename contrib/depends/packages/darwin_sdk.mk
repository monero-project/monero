package=darwin_sdk
$(package)_version=11.1
$(package)_download_path=https://github.com/phracker/MacOSX-SDKs/releases/download/11.3/
$(package)_file_name=MacOSX$($(package)_version).sdk.tar.xz
$(package)_sha256_hash=68797baaacb52f56f713400de306a58a7ca00b05c3dc6d58f0a8283bcac721f8

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_dir)/$(host_prefix)/native/SDK &&\
  mv * $($(package)_staging_dir)/$(host_prefix)/native/SDK
endef
