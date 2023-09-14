package=zeromq
$(package)_version=4.3.4
$(package)_download_path=https://github.com/zeromq/libzmq/releases/download/v$($(package)_version)/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=c593001a89f5a85dd2ddf564805deb860e02471171b3f204944857336295c3e5
$(package)_patches=06aba27b04c5822cb88a69677382a0f053367143.patch

define $(package)_set_vars
  $(package)_config_opts=--without-documentation --disable-shared --without-libsodium --disable-curve
  $(package)_config_opts_linux=--with-pic
  $(package)_config_opts_freebsd=--with-pic
  $(package)_cxxflags=-std=c++11
  $(package)_cxxflags_darwin=-Wno-error=deprecated-declarations
endef

#  $(package)_build_opts=CC=$($(package)_cc)
#  $(package)_build_opts=CFLAGS="$($(package)_cflags) $($(package)_cppflags) -fPIC"
#  $(package)_build_opts_darwin=-Wno-error=deprecated-copy -Wno-error=pessimizing-move

#ifneq ($(host_os),darwin)
#  CC_EXTRA=-Wno-error=deprecated-declarations -Wno-error=deprecated-copy -Wno-error=pessimizing-move
#else
#  CC_EXTRA=
#endif

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/06aba27b04c5822cb88a69677382a0f053367143.patch
endef

define $(package)_config_cmds
  $($(package)_autoconf) AR_FLAGS=$($(package)_arflags)
endef

#$(MAKE) $($(package)_build_opts) $(CC_EXTRA) src/libzmq.la
#$(MAKE) $(CC_EXTRA) src/libzmq.la

#define $(package)_build_cmds
#  echo "Compiling with $(CC_EXTRA)" && \
#  $(MAKE) $(CC_EXTRA) src/libzmq.la
#endef

define $(package)_build_cmds
  V=1 $(MAKE) src/libzmq.la
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install-libLTLIBRARIES install-includeHEADERS install-pkgconfigDATA
endef

define $(package)_postprocess_cmds
  rm -rf bin share &&\
  rm lib/*.la
endef

