package=gtest
$(package)_version=1.14.0
$(package)_download_path=https://github.com/google/googletest/archive/
$(package)_download_file=v$($(package)_version).tar.gz
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=8ad598c73ad796e0d8280b082cebd82a630d73e73cd3c70057938a6501bba5d7
$(package)_cxxflags=-std=c++14
$(package)_cxxflags_linux=-fPIC

define $(package)_config_cmds
  $($(package)_cmake) -S . -B .
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  mkdir $($(package)_staging_prefix_dir)/lib $($(package)_staging_prefix_dir)/include &&\
  cp lib/libgtest.a $($(package)_staging_prefix_dir)/lib/ &&\
  cp lib/libgtest_main.a $($(package)_staging_prefix_dir)/lib/ &&\
  cp -a googletest/include/* $($(package)_staging_prefix_dir)/include/
endef
