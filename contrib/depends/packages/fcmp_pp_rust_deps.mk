package=fcmp_pp_rust_deps
$(package)_version=0.0.0
$(package)_download_path=https://featherwallet.org/files/sources
$(package)_file_name=fcmp_pp_rust-$($(package)_version)-deps.tar.gz
$(package)_sha256_hash=9a7c20d5571c0a82fe570122b4166dd4223672baaa1e25e52eb2426eefe3bc02

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/cargo &&\
  mv * $($(package)_staging_prefix_dir)/cargo
endef
