package=boost
$(package)_version=1.91.0-1
$(package)_download_path=https://github.com/boostorg/boost/releases/download/boost-$($(package)_version)
$(package)_file_name=boost-$($(package)_version)-b2-nodocs.tar.gz
$(package)_sha256_hash=b5a3d1490118e012f8b12688240d981bcdfcd009fd35bc70d120fbc907df4f7c

define $(package)_set_vars
$(package)_config_opts_release=variant=release
$(package)_config_opts_debug=variant=debug
$(package)_config_opts+=--layout=system --user-config=user-config.jam
$(package)_config_opts+=threading=multi link=static -sNO_BZIP2=1 -sNO_ZLIB=1
$(package)_config_opts+=runtime-link=static
$(package)_config_opts_linux=threadapi=pthread
$(package)_config_opts_android=threadapi=pthread target-os=android
$(package)_config_opts_darwin=target-os=darwin
$(package)_config_opts_mingw32=binary-format=pe target-os=windows threadapi=win32
$(package)_config_opts_x86_64_mingw32=address-model=64
$(package)_config_opts_i686_mingw32=address-model=32
$(package)_config_opts_i686_linux=address-model=32 architecture=x86
$(package)_toolset_$(host_os)=gcc
$(package)_archiver_$(host_os)=$($(package)_ar)
$(package)_config_libraries_$(host_os)="chrono,filesystem,program_options,thread,test,serialization"
$(package)_config_libraries_mingw32="chrono,filesystem,program_options,thread,test,serialization,locale"
$(package)_cxxflags_linux+=-fPIC
$(package)_cxxflags_freebsd+=-fPIC
$(package)_cxxflags_darwin+=-ffile-prefix-map=$($(package)_extract_dir)=/usr
endef

define $(package)_preprocess_cmds
  echo "using $(boost_toolset_$(host_os)) : : $($(package)_cxx) : <cxxflags>\"$($(package)_cxxflags) $($(package)_cppflags)\" <linkflags>\"$($(package)_ldflags)\" <archiver>\"$(boost_archiver_$(host_os))\" <arflags>\"$($(package)_arflags)\" <striper>\"$(host_STRIP)\"  <ranlib>\"$(host_RANLIB)\" <rc>\"$(host_WINDRES)\" : ;" > user-config.jam
endef

define $(package)_config_cmds
  ./bootstrap.sh --without-icu --with-libraries=$(boost_config_libraries_$(host_os))
endef

define $(package)_build_cmds
  ./b2 -d2 -j2 -d1 --prefix=$($(package)_staging_prefix_dir) $($(package)_config_opts) stage
endef

define $(package)_stage_cmds
  ./b2 -d0 -j4 --prefix=$($(package)_staging_prefix_dir) $($(package)_config_opts) install
endef

ifeq ($(release_type),release)
define $(package)_postprocess_cmds
  cd include/boost && \
  find . -mindepth 1 -maxdepth 1 -type d \
    ! -name algorithm \
    ! -name align \
    ! -name any \
    ! -name archive \
    ! -name asio \
    ! -name assert \
    ! -name atomic \
    ! -name bimap \
    ! -name bind \
    ! -name chrono \
    ! -name circular_buffer \
    ! -name concept \
    ! -name config \
    ! -name container \
    ! -name container_hash \
    ! -name core \
    ! -name date_time \
    ! -name describe \
    ! -name detail \
    ! -name endian \
    ! -name exception \
    ! -name filesystem \
    ! -name format \
    ! -name function \
    ! -name function_types \
    ! -name functional \
    ! -name fusion \
    ! -name integer \
    ! -name intrusive \
    ! -name io \
    ! -name iostreams \
    ! -name iterator \
    ! -name lambda \
    ! -name lexical_cast \
    ! -name locale \
    ! -name logic \
    ! -name math \
    ! -name move \
    ! -name mp11 \
    ! -name mpl \
    ! -name multi_index \
    ! -name multiprecision \
    ! -name numeric \
    ! -name optional \
    ! -name phoenix \
    ! -name predef \
    ! -name preprocessor \
    ! -name program_options \
    ! -name proto \
    ! -name range \
    ! -name ratio \
    ! -name regex \
    ! -name serialization \
    ! -name smart_ptr \
    ! -name spirit \
    ! -name system \
    ! -name thread \
    ! -name tuple \
    ! -name type_index \
    ! -name type_traits \
    ! -name typeof \
    ! -name unordered \
    ! -name utility \
    ! -name uuid \
    ! -name variant \
    ! -name variant2 \
    ! -name winapi \
    -exec rm -rf {} +
endef
endif
