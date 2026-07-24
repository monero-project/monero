package=freebsd_base
$(package)_version=15.1
$(package)_download_path=https://download.freebsd.org/releases/amd64/$($(package)_version)-RELEASE/
$(package)_download_file=base.txz
$(package)_file_name=freebsd-base-$($(package)_version).txz
$(package)_sha256_hash=3768988b151c20f965679062b065c63a977d6bbb9f47fd83695ec2c40790c18f

define $(package)_extract_cmds
  echo $($(package)_sha256_hash) $($(1)_source_dir)/$($(package)_file_name) | sha256sum -c &&\
  tar xf $($(1)_source_dir)/$($(package)_file_name) ./lib/ ./usr/lib/ ./usr/include/
endef

# Prevent clang from including OpenSSL headers from the system base. We
# statically link our own version of OpenSSL.

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/sysroot &&\
  rm -rf usr/include/openssl &&\
  mv lib usr $($(package)_staging_prefix_dir)/sysroot
endef
