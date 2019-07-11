package=gtest
$(package)_version=1.8.1
$(package)_download_path=https://github.com/google/googletest/archive/
$(package)_file_name=release-$($(package)_version).tar.gz
$(package)_sha256_hash=9bf1fe5182a604b4135edc1a425ae356c9ad15e9b23f9f12a02e80184c3a249c
$(package)_cxxflags=-std=c++11
$(package)_cxxflags_linux=-fPIC

define $(package)_config_cmds
  cd googletest && \
    CC="$(host_prefix)/native/bin/$($(package)_cc)" \
    CXX="$(host_prefix)/native/bin/$($(package)_cxx)" \
    AR="$(host_prefix)/native/bin/$($(package)_ar)" \
    RANLIB="$(host_prefix)/native/bin/$($(package)_ranlib)" \
    LIBTOOL="$(host_prefix)/native/bin/$($(package)_libtool)" \
    CXXFLAGS="$($(package)_cxxflags)" \
    CCFLAGS="$($(package)_ccflags)" \
    CPPFLAGS="$($(package)_cppflags)" \
    CFLAGS="$($(package)_cflags) $($(package)_cppflags)" \
    LDLAGS="$($(package)_ldflags)" \
  cmake -DCMAKE_INSTALL_PREFIX=$(build_prefix) \
    -DTOOLCHAIN_PREFIX=$(host_toolchain) \
    -DCMAKE_AR="$(host_prefix)/native/bin/$($(package)_ar)" \
    -DCMAKE_RANLIB="$(host_prefix)/native/bin/$($(package)_ranlib)" \
    -DCMAKE_CXX_FLAGS_DEBUG=ON
endef
# -DCMAKE_TOOLCHAIN_FILE=$(HOST)/share/toolchain.cmake

define $(package)_build_cmds
  cd googletest && CC="$(host_prefix)/native/bin/$($(package)_cc)" $(MAKE)
endef

define $(package)_stage_cmds
  mkdir $($(package)_staging_prefix_dir)/lib $($(package)_staging_prefix_dir)/include &&\
  cp googletest/libgtest.a $($(package)_staging_prefix_dir)/lib/ &&\
  cp googletest/libgtest_main.a $($(package)_staging_prefix_dir)/lib/ &&\
  cp -a googletest/include/* $($(package)_staging_prefix_dir)/include/
endef
