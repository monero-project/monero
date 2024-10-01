package=darwin_sdk
$(package)_version=12.2
$(package)_download_path=https://bitcoincore.org/depends-sources/sdks
$(package)_file_name=Xcode-12.2-12B45b-extracted-SDK-with-libcxx-headers.tar.gz
$(package)_sha256_hash=df75d30ecafc429e905134333aeae56ac65fac67cb4182622398fd717df77619
$(package)_patches=setup.sh

define $(package)_config_cmds
   env host_prefix="$(host_prefix)" host="$(host)" bash $($(package)_patch_dir)/setup.sh
endef

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_dir)/$(host_prefix)/native/SDK &&\
  rm -rf usr/include/readline && \
  mv bin $($(package)_staging_dir)/$(host_prefix)/native/ && \
  mv * $($(package)_staging_dir)/$(host_prefix)/native/SDK
endef
