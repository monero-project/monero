package=icu4c
$(package)_version=73.1
$(package)_download_path=https://github.com/unicode-org/icu/releases/download/release-73-1/
$(package)_file_name=$(package)-73_1-src.tgz
$(package)_sha256_hash=a457431de164b4aa7eca00ed134d00dfbf88a77c6986a10ae7774fc076bb8c45
$(package)_patches=icu-001-dont-build-static-dynamic-twice.patch

define $(package)_set_vars
  $(package)_build_opts=CFLAGS="$($(package)_cflags) $($(package)_cppflags) -DU_USING_ICU_NAMESPACE=0 -DU_STATIC_IMPLEMENTATION -fPIC -DENABLE_STATIC=YES -DPGKDATA_MODE=static"
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
