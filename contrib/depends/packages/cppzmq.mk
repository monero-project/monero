package=cppzmq
$(package)_version=4.4.1
$(package)_download_path=https://github.com/zeromq/cppzmq/archive/
$(package)_file_name=v$($(package)_version).tar.gz
$(package)_sha256_hash=117fc1ca24d98dbe1a60c072cde13be863d429134907797f8e03f654ce679385
$(package)_dependencies=zeromq

define $(package)_stage_cmds
  mkdir $($(package)_staging_prefix_dir)/include &&\
  cp zmq.hpp $($(package)_staging_prefix_dir)/include
endef

define $(package)_postprocess_cmds
  rm -rf bin share
endef
