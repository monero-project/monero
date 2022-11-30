package=boost
$(package)_version=1.77.0
$(package)_download_path=https://boostorg.jfrog.io/artifactory/main/release/$($(package)_version)/source/
$(package)_file_name=boost_$(subst .,_,$($(package)_version)).tar.bz2
$(package)_sha256_hash=fc9f85fc030e233142908241af7a846e60630aa7388de9a5fafb1f3a26840854
$(package)_dependencies=libiconv

define $(package)_set_vars
$(package)_config_opts_release=variant=release
$(package)_config_opts_debug=variant=debug
$(package)_config_opts=--layout=tagged --build-type=complete --user-config=user-config.jam
$(package)_config_opts+=threading=multi link=static -sNO_BZIP2=1 -sNO_ZLIB=1
$(package)_config_opts_linux=target-os=linux threadapi=pthread runtime-link=static
$(package)_config_opts_darwin=target-os=darwin runtime-link=static
$(package)_config_opts_mingw32=binary-format=pe target-os=windows threadapi=win32 runtime-link=static
$(package)_config_opts_x86_64=architecture=x86 address-model=64
$(package)_config_opts_i686=architecture=x86 address-model=32
$(package)_config_opts_aarch64=address-model=64
$(package)_config_opts_armv7a=address-model=32
$(package)_config_opts_i686_android=address-model=32
$(package)_config_opts_aarch64_android=address-model=64
$(package)_config_opts_x86_64_android=address-model=64
$(package)_config_opts_armv7a_android=address-model=32
ifneq (,$(findstring clang,$($(package)_cxx)))
   $(package)_toolset_$(host_os)=clang
else
   $(package)_toolset_$(host_os)=gcc
endif
$(package)_archiver_$(host_os)=$($(package)_ar)
$(package)_config_libraries=chrono,filesystem,program_options,system,thread,test,date_time,regex,serialization,locale,atomic
$(package)_cxxflags+=-std=c++17 -fvisibility=hidden
$(package)_cxxflags_linux=-fPIC
endef

define $(package)_preprocess_cmds
  echo "using $($(package)_toolset_$(host_os)) : : $($(package)_cxx) : <cxxflags>\"$($(package)_cxxflags) $($(package)_cppflags)\" <linkflags>\"$($(package)_ldflags)\" <archiver>\"$(boost_archiver_$(host_os))\" <arflags>\"$($(package)_arflags)\" <striper>\"$(host_STRIP)\" <ranlib>\"$(host_RANLIB)\" <rc>\"$(host_WINDRES)\" : ;" > user-config.jam
endef

define $(package)_config_cmds
  ./bootstrap.sh --without-icu --with-libraries=$(boost_config_libraries) --with-toolset=$($(package)_toolset_$(host_os))
endef

define $(package)_build_cmds
  ./b2 -d2 -j2 -d1 --prefix=$($(package)_staging_prefix_dir) $($(package)_config_opts) toolset=$($(package)_toolset_$(host_os)) stage
endef

define $(package)_stage_cmds
  ./b2 -d0 -j4 --prefix=$($(package)_staging_prefix_dir) $($(package)_config_opts) toolset=$($(package)_toolset_$(host_os)) install
endef
