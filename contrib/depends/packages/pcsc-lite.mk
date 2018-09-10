package=pcsc-lite
$(package)_version=1.8.23
$(package)_download_path=https://pcsclite.apdu.fr/files
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=5a27262586eff39cfd5c19aadc8891dd71c0818d3d629539bd631b958be689c9

define $(package)_set_vars
  $(package)_build_opts=CC="$($(package)_cc)"
  $(package)_config_env=AR="$($(package)_ar)" RANLIB="$($(package)_ranlib)" CC="$($(package)_cc)"
  $(package)_config_opts=--prefix=$(host_prefix)
  $(package)_config_opts_release=--disable-debug-mode --disable-libsystemd --disable-libudev --enable-static --disable-shared --disable-libusb
  $(package)_build_opts=CFLAGS="$($(package)_cflags) $($(package)_cppflags) -fPIC"
endef

define $(package)_config_cmds
  ./bootstrap &&\
  $($(package)_autoconf) $($(package)_config_opts)
endef

define $(package)_build_cmds
  $(MAKE) $($(package)_build_opts)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
