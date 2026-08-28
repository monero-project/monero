package=native_protobuf
$(package)_version=36.0
$(package)_download_path=https://github.com/protocolbuffers/protobuf/releases/download/v$($(package)_version)
$(package)_file_name=protobuf-$($(package)_version).tar.gz
$(package)_sha256_hash=399931c793f4ac6db81045b00b06dd07c877b48aeecf36c797f65c541fb533e7
$(package)_dependencies=native_abseil

define $(package)_set_vars
  $(package)_config_opts=-Dprotobuf_BUILD_TESTS=OFF
  $(package)_config_opts+=-Dprotobuf_ABSL_PROVIDER=package
  $(package)_config_opts+=-Dprotobuf_BUILD_SHARED_LIBS=OFF
  $(package)_config_opts+=-Dprotobuf_WITH_ZLIB=OFF
  $(package)_cxxflags+=-g0
endef

# Remove blobs
define $(package)_preprocess_cmds
  rm -rf examples docs php/src/GPBMetadata compatibility objectivec/Tests csharp/keys php/tests src/google/protobuf/testdata csharp/src/Google.Protobuf.Test
endef

define $(package)_config_cmds
  $($(package)_cmake) .
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  rm -rf lib64
endef
