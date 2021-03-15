package=zlib
$(package)_version=1.2.11
$(package)_download_path=https://zlib.net
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1

# Here all that we do, is copying the content of the package to a fixed directory,
# so that Boost can find it.
define $(package)_build_cmds
  mkdir -p $(BASEDIR)/sources/$(package) && cp * -rf $(BASEDIR)/sources/$(package)
endef

