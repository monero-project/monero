package=protobuf
$(package)_version=$(native_$(package)_version)
$(package)_download_path=$(native_$(package)_download_path)
$(package)_file_name=$(native_$(package)_file_name)
$(package)_sha256_hash=$(native_$(package)_sha256_hash)
$(package)_dependencies=abseil

define $(package)_set_vars
  $(package)_cxxflags+=-std=c++17
  $(package)_config_opts=-Dprotobuf_ABSL_PROVIDER=package
  $(package)_config_opts+=-Dprotobuf_BUILD_TESTS=OFF
  $(package)_config_opts+=-Dprotobuf_BUILD_SHARED_LIBS=OFF
  $(package)_config_opts+=-Dprotobuf_BUILD_PROTOC_BINARIES=OFF
  $(package)_config_opts+=-Dprotobuf_WITH_ZLIB=OFF
  $(package)_config_opts+=-Dprotobuf_BUILD_LIBUPB=OFF
endef

# Remove blobs
define $(package)_preprocess_cmds
  rm -rf docs php/src/GPBMetadata compatibility objectivec/Tests csharp/keys php/tests src/google/protobuf/testdata csharp/src/Google.Protobuf.Test
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
