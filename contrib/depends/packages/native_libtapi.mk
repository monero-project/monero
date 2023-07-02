package=native_libtapi
$(package)_version=664b8414f89612f2dfd35a9b679c345aa5389026
$(package)_download_path=https://github.com/tpoechtrager/apple-libtapi/archive
$(package)_download_file=$($(package)_version).tar.gz
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=62e419c12d1c9fad67cc1cd523132bc00db050998337c734c15bc8d73cc02b61
$(package)_patches=no_embed_git_rev.patch

define $(package)_preprocess_cmds
  patch -p1 -i $($(package)_patch_dir)/no_embed_git_rev.patch
endef

define $(package)_build_cmds
  CC=$(clang_prog) CXX=$(clangxx_prog) INSTALLPREFIX=$($(package)_staging_prefix_dir) ./build.sh
endef

define $(package)_stage_cmds
  ./install.sh
endef
