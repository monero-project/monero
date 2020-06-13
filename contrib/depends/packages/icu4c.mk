package=icu4c
$(package)_version=55.1
$(package)_download_path=https://github.com/TheCharlatan/icu4c/archive
$(package)_file_name=55.1.tar.gz
$(package)_sha256_hash=1f912c54035533fb4268809701d65c7468d00e292efbc31e6444908450cc46ef
$(package)_patches=icu-001-dont-build-static-dynamic-twice.patch

define $(package)_set_vars
  $(package)_build_opts=CFLAGS="$($(package)_cflags) $($(package)_cppflags) -DU_USING_ICU_NAMESPACE=0 --std=gnu++0x -DU_STATIC_IMPLEMENTATION -DU_COMBINED_IMPLEMENTATION -fPIC"
endef

define $(package)_config_cmds
  patch -p1 < $($(package)_patch_dir)/icu-001-dont-build-static-dynamic-twice.patch &&\
  mkdir builda &&\
  mkdir buildb &&\
  cd builda &&\
  sh ../source/runConfigureICU Linux &&\
  make &&\
  cd ../buildb &&\
  sh ../source/$($(package)_autoconf) --enable-static=yes --enable-shared=yes --disable-layoutex --prefix=$(host_prefix) --with-cross-build=`pwd`/../builda &&\
  $(MAKE) $($(package)_build_opts)
endef

#define $(package)_build_cmds
#  cd source &&\
   $(MAKE) $($((package)_build_opts) `nproc`
#endef

define $(package)_stage_cmds
  cd buildb &&\
  $(MAKE) $($(package)_build_opts) DESTDIR=$($(package)_staging_dir) install lib/*
endef
