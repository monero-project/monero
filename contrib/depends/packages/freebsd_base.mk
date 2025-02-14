package=freebsd_base
$(package)_version=12.3
$(package)_download_path=https://archive.freebsd.org/old-releases/amd64/$($(package)_version)-RELEASE/
$(package)_download_file=base.txz
$(package)_file_name=freebsd-base-$($(package)_version).txz
$(package)_sha256_hash=e85b256930a2fbc04b80334106afecba0f11e52e32ffa197a88d7319cf059840

define $(package)_extract_cmds
  echo $($(package)_sha256_hash) $($(1)_source_dir)/$($(package)_file_name) | sha256sum -c &&\
  tar xf $($(1)_source_dir)/$($(package)_file_name) ./lib/ ./usr/lib/ ./usr/include/
endef

# Prevent clang from including OpenSSL headers from the system base. We
# statically link our own version of OpenSSL.

define $(package)_stage_cmds
  mkdir $($(package)_staging_prefix_dir) &&\
  rm -rf usr/include/openssl &&\
  mv lib usr $($(package)_staging_prefix_dir)
endef
