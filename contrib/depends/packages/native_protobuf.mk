package=native_protobuf
$(package)_version=31.1
$(package)_download_path=https://github.com/protocolbuffers/protobuf/releases/download/v$($(package)_version)
$(package)_file_name=protobuf-$($(package)_version).tar.gz
$(package)_sha256_hash=12bfd76d27b9ac3d65c00966901609e020481b9474ef75c7ff4601ac06fa0b82
$(package)_dependencies=native_abseil

define $(package)_set_vars
  $(package)_cxxflags+=-std=c++17
  $(package)_config_opts=-Dprotobuf_BUILD_TESTS=OFF
  $(package)_config_opts+=-Dprotobuf_ABSL_PROVIDER=package
  $(package)_config_opts+=-Dprotobuf_BUILD_SHARED_LIBS=OFF
  $(package)_config_opts+=-Dprotobuf_WITH_ZLIB=OFF
endef

# Remove blobs
define $(package)_preprocess_cmds
  rm -rf examples docs php/src/GPBMetadata compatibility objectivec/Tests csharp/keys php/tests src/google/protobuf/testdata csharp/src/Google.Protobuf.Test
endef

define $(package)_config_cmds
  $($(package)_cmake)
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
