package=hidapi
$(package)_version=0.15.0
$(package)_download_path=https://github.com/libusb/hidapi/archive/refs/tags
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=5d84dec684c27b97b921d2f3b73218cb773cf4ea915caee317ac8fc73cef8136
$(package)_linux_dependencies=libusb

# -DHIDAPI_NO_ICONV=ON
#
#   `FindIconv.cmake` in CMake 3.16 fails to detect iconv for riscv64, arm, and aarch64 linux targets.
#   Disable it if we're not building in a release environment.

define $(package)_set_vars
  $(package)_config_opts := -DBUILD_SHARED_LIBS=OFF
  $(package)_config_opts += -DHIDAPI_WITH_HIDRAW=OFF
  ifeq ($(GUIX_ENVIRONMENT),)
  $(package)_config_opts += -DHIDAPI_NO_ICONV=ON
  endif
endef

# Remove blobs
define $(package)_preprocess_cmds
  rm -rf documentation testgui windows/test/data m4
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
