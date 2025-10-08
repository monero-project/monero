package=boost
$(package)_version=1.84.0
$(package)_download_path=https://archives.boost.io/release/$($(package)_version)/source/
$(package)_file_name=$(package)_$(subst .,_,$($(package)_version)).tar.bz2
$(package)_sha256_hash=cc4b893acf645c9d4b698e9a0f08ca8846aa5d6c68275c14c3e7949c24109454

define $(package)_set_vars
$(package)_config_opts_release=variant=release
$(package)_config_opts_debug=variant=debug
$(package)_config_opts+=--layout=system --user-config=user-config.jam
$(package)_config_opts+=threading=multi link=static -sNO_BZIP2=1 -sNO_ZLIB=1
$(package)_config_opts_linux=threadapi=pthread runtime-link=shared
$(package)_config_opts_android=threadapi=pthread runtime-link=static target-os=android
$(package)_config_opts_darwin=runtime-link=shared target-os=darwin
$(package)_config_opts_mingw32=binary-format=pe target-os=windows threadapi=win32 runtime-link=static
$(package)_config_opts_x86_64_mingw32=address-model=64
$(package)_config_opts_i686_mingw32=address-model=32
$(package)_config_opts_i686_linux=address-model=32 architecture=x86
$(package)_toolset_$(host_os)=gcc
$(package)_archiver_$(host_os)=$($(package)_ar)
$(package)_config_libraries_$(host_os)="chrono,filesystem,program_options,system,thread,test,date_time,regex,serialization"
$(package)_config_libraries_mingw32="chrono,filesystem,program_options,system,thread,test,date_time,regex,serialization,locale"
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
