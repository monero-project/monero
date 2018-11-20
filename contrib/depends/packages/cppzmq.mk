package=cppzmq
$(package)_version=4.2.3
$(package)_download_path=https://github.com/zeromq/cppzmq/archive/
$(package)_file_name=v$($(package)_version).tar.gz
$(package)_sha256_hash=3e6b57bf49115f4ae893b1ff7848ead7267013087dc7be1ab27636a97144d373
$(package)_dependencies=zeromq

define $(package)_stage_cmds
  mkdir $($(package)_staging_prefix_dir)/include &&\
  cp zmq.hpp $($(package)_staging_prefix_dir)/include
endef

define $(package)_postprocess_cmds
  rm -rf bin share
endef
