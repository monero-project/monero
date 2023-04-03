package=native_ds_store
$(package)_version=1.3.0
$(package)_download_path=https://github.com/al45tair/ds_store/archive/
$(package)_file_name=v$($(package)_version).tar.gz
$(package)_sha256_hash=78999ca6fd6a9b2b08a5dc04503d98d7a4337ada80132856c1fb963608841ca0
$(package)_install_libdir=$(build_prefix)/lib/python3/dist-packages
$(package)_dependencies=native_biplist


define $(package)_build_cmds
    python3 setup.py build
endef

define $(package)_stage_cmds
    mkdir -p $($(package)_install_libdir) && \
    python3 setup.py install --root=$($(package)_staging_dir) --prefix=$(build_prefix) --install-lib=$($(package)_install_libdir)
endef
