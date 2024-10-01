package=freebsd_base
$(package)_version=12.3
$(package)_download_path=https://ci-mirrors.rust-lang.org/rustc
$(package)_download_file=2022-05-06-freebsd-$($(package)_version)-amd64-base.txz
$(package)_file_name=freebsd-base-$($(package)_version).txz
$(package)_sha256_hash=e85b256930a2fbc04b80334106afecba0f11e52e32ffa197a88d7319cf059840
$(package)_patches=setup.sh

define $(package)_extract_cmds
  echo $($(package)_sha256_hash) $($(1)_source_dir)/$($(package)_file_name) | sha256sum -c &&\
  tar xf $($(1)_source_dir)/$($(package)_file_name) ./lib/ ./usr/lib/ ./usr/include/
endef

define $(package)_config_cmds
   env host_prefix="$(host_prefix)" bash $($(package)_patch_dir)/setup.sh
endef

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_dir)/$(host_prefix)/native/toolchain &&\
  rm -rf usr/include/openssl &&\
  mv lib usr $($(package)_staging_dir)/$(host_prefix)/native/toolchain &&\
  mv bin $($(package)_staging_dir)/$(host_prefix)/native/
endef
