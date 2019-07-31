package=icu4c
$(package)_version=55.1
$(package)_download_path=https://github.com/TheCharlatan/icu4c/archive
$(package)_file_name=55.1.tar.gz
$(package)_sha256_hash=1f912c54035533fb4268809701d65c7468d00e292efbc31e6444908450cc46ef
$(package)_patches=icu-001-dont-build-static-dynamic-twice.patch

define $(package)_set_vars
  $(package)_build_opts=CFLAGS="$($(package)_cflags) $($(package)_cppflags) -DU_USING_ICU_NAMESPACE=0 -DU_STATIC_IMPLEMENTATION -DU_COMBINED_IMPLEMENTATION -fPIC -DENABLE_STATIC=YES -DPGKDATA_MODE=static"
endef

define $(package)_config_cmds
  patch -p1 < $($(package)_patch_dir)/icu-001-dont-build-static-dynamic-twice.patch &&\
  mkdir builda &&\
  mkdir buildb &&\
  cd builda &&\
  sh ../source/runConfigureICU Linux &&\
  make &&\
  cd ../buildb &&\
  sh ../source/runConfigureICU MinGW --enable-static=yes --disable-shared --disable-layout --disable-layoutex --disable-tests --disable-samples --prefix=$(host_prefix) --with-cross-build=`pwd`/../builda &&\
  $(MAKE) $($(package)_build_opts)
endef

define $(package)_stage_cmds
  cd buildb &&\
  $(MAKE) $($(package)_build_opts) DESTDIR=$($(package)_staging_dir) install lib/*
endef
