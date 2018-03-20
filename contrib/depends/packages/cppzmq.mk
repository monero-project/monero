package=cppzmq
$(package)_version=4.2.3
$(package)_download_path=https://github.com/zeromq/cppzmq/archive/
$(package)_file_name=v$($(package)_version).tar.gz
$(package)_sha256_hash=3e6b57bf49115f4ae893b1ff7848ead7267013087dc7be1ab27636a97144d373
$(package)_dependencies=zeromq

define $(package)_config_cmds
  echo $(build_pefix) &&\
  cmake -DCMAKE_INSTALL_PREFIX=$(build_prefix)/../ -DCMAKE_PREFIX_PATH=$(build_prefix)/../include -DZeroMQ_STATIC_LIBRARY=$(build_prefix)/lib -DPC_LIBZMQ_LIBDIR=$(build_prefix)/lib -DPC_LIBZMQ_LIBRARY_DIRS=$(build_prefix)/lib CMakeLists.txt
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) &&\
  echo $(host_prefix)/include/ &&\
  echo $($(package)_staging_prefix_dir) &&\
  mkdir $($(package)_staging_prefix_dir)/include &&\
  cp zmq.hpp $($(package)_staging_prefix_dir)/include
endef

define $(package)_postprocess_cmds
  rm -rf bin share
endef
