package=openssl
$(package)_version=3.5.4
$(package)_download_path=https://github.com/openssl/openssl/releases/download/openssl-$($(package)_version)
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=967311f84955316969bdb1d8d4b983718ef42338639c621ec4c34fddef355e99
$(package)_patches=fix-android.patch

define $(package)_set_vars
$(package)_config_env=AR="$($(package)_ar)" RANLIB="$($(package)_ranlib)" CC="$($(package)_cc)"
$(package)_config_env_android=ANDROID_NDK_ROOT="$(host_prefix)/native" PATH="$(host_prefix)/native/bin"
$(package)_build_env_android=ANDROID_NDK_ROOT="$(host_prefix)/native"
$(package)_config_opts=--prefix=$(host_prefix) --openssldir=$(host_prefix)/etc/openssl --libdir=$(host_prefix)/lib
$(package)_config_opts+=no-apps
$(package)_config_opts+=no-capieng
$(package)_config_opts+=no-dso
$(package)_config_opts+=no-dtls1
$(package)_config_opts+=no-ec_nistp_64_gcc_128
$(package)_config_opts+=no-gost
$(package)_config_opts+=no-md2
$(package)_config_opts+=no-rc5
$(package)_config_opts+=no-rdrand
$(package)_config_opts+=no-rfc3779
$(package)_config_opts+=no-sctp
$(package)_config_opts+=no-shared
$(package)_config_opts+=no-ssl-trace
$(package)_config_opts+=no-ssl3
$(package)_config_opts+=no-tests
$(package)_config_opts+=no-unit-test
$(package)_config_opts+=no-weak-ssl-ciphers
$(package)_config_opts+=no-winstore
$(package)_config_opts+=no-zlib
$(package)_config_opts+=no-zlib-dynamic
$(package)_config_opts_linux=-fPIC -Wa,--noexecstack
$(package)_config_opts_freebsd=-fPIC -Wa,--noexecstack
$(package)_config_opts_x86_64_linux=linux-x86_64
$(package)_config_opts_i686_linux=linux-generic32
$(package)_config_opts_arm_linux=linux-generic32
$(package)_config_opts_aarch64_linux=linux-generic64
$(package)_config_opts_arm_android=--static android-arm
$(package)_config_opts_aarch64_android=--static android-arm64
$(package)_config_opts_aarch64_darwin=darwin64-arm64-cc
$(package)_config_opts_riscv64_linux=linux64-riscv64
$(package)_config_opts_loongarch64_linux=linux-generic64
$(package)_config_opts_mipsel_linux=linux-generic32
$(package)_config_opts_mips_linux=linux-generic32
$(package)_config_opts_powerpc_linux=linux-generic32
$(package)_config_opts_x86_64_darwin=darwin64-x86_64-cc
$(package)_config_opts_x86_64_mingw32=mingw64
$(package)_config_opts_i686_mingw32=mingw
$(package)_config_opts_x86_64_freebsd=BSD-x86_64
endef

define $(package)_preprocess_cmds
  sed -i.old 's|crypto ssl apps util tools fuzz providers doc|crypto ssl util tools providers|' build.info && \
  patch -p1 < $($(package)_patch_dir)/fix-android.patch && \
  rm -rf doc demos apps test
endef

define $(package)_config_cmds
  ./Configure $($(package)_config_opts) ARFLAGS=$($(package)_arflags)
endef

define $(package)_build_cmds
  $(MAKE) build_libs
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install_sw
endef

define $(package)_postprocess_cmds
  rm -rf share bin etc
endef
