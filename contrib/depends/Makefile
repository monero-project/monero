.NOTPARALLEL :

# Pattern rule to print variables, e.g. make print-all_packages
print-%: FORCE
	@echo '$($*)'

SOURCES_PATH ?= $(BASEDIR)/sources
BASE_CACHE ?= $(BASEDIR)/built
FALLBACK_DOWNLOAD_PATH ?= https://downloads.getmonero.org/depends-sources

BUILD = $(shell ./config.guess)
HOST ?= $(BUILD)
PATCHES_PATH = $(BASEDIR)/patches
BASEDIR = $(CURDIR)
HASH_LENGTH:=11
DOWNLOAD_CONNECT_TIMEOUT:=30
DOWNLOAD_RETRIES:=5
HOST_ID_SALT ?= salt
BUILD_ID_SALT ?= salt

host:=$(BUILD)
ifneq ($(HOST),)
host:=$(HOST)
host_toolchain:=$(HOST)-
endif

ifneq ($(DEBUG),)
release_type=debug
else
release_type=release
endif

ifneq ($(TESTS),)
build_tests=ON
release_type=debug
else
build_tests=OFF
endif

ifeq ($(release_type),debug)
cmake_release_type=Debug
else
cmake_release_type=Release
endif

base_build_dir=$(BASEDIR)/work/build
base_staging_dir=$(BASEDIR)/work/staging
base_download_dir=$(BASEDIR)/work/download
canonical_host:=$(shell ./config.sub $(HOST))
build:=$(shell ./config.sub $(BUILD))

build_arch =$(firstword $(subst -, ,$(build)))
build_vendor=$(word 2,$(subst -, ,$(build)))
full_build_os:=$(subst $(build_arch)-$(build_vendor)-,,$(build))
build_os:=$(findstring linux,$(full_build_os))
build_os+=$(findstring darwin,$(full_build_os))
build_os:=$(strip $(build_os))
ifeq ($(build_os),)
build_os=$(full_build_os)
endif

host_arch=$(firstword $(subst -, ,$(canonical_host)))
host_vendor=$(word 2,$(subst -, ,$(canonical_host)))
full_host_os:=$(subst $(host_arch)-$(host_vendor)-,,$(canonical_host))
host_os:=$(findstring android,$(full_host_os))
ifeq ($(host_os),)
host_os:=$(findstring linux,$(full_host_os))
endif
host_os+=$(findstring darwin,$(full_host_os))
host_os+=$(findstring freebsd,$(full_host_os))
host_os+=$(findstring mingw32,$(full_host_os))
host_os:=$(strip $(host_os))
ifeq ($(host_os),)
host_os=$(full_host_os)
endif

$(host_arch)_$(host_os)_prefix=$(BASEDIR)/$(host)
$(host_arch)_$(host_os)_host=$(host)
host_prefix=$($(host_arch)_$(host_os)_prefix)
build_prefix=$(host_prefix)/native

AT_$(V):=
AT_:=@
AT:=$(AT_$(V))

all: install

include hosts/$(host_os).mk
include hosts/default.mk
include builders/$(build_os).mk
include builders/default.mk
include packages/packages.mk

ifeq ($(GUIX_ENVIRONMENT),)
build_id_string:=$(BUILD_ID_SALT)
build_id_string+=$(shell $(build_CC) --version 2>/dev/null)
build_id_string+=$(shell $(build_AR) --version 2>/dev/null)
build_id_string+=$(shell $(build_CXX) --version 2>/dev/null)
build_id_string+=$(shell $(build_RANLIB) --version 2>/dev/null)
build_id_string+=$(shell $(build_STRIP) --version 2>/dev/null)

$(host_arch)_$(host_os)_id_string:=$(HOST_ID_SALT)
$(host_arch)_$(host_os)_id_string+=$(shell $(host_CC) --version 2>/dev/null)
$(host_arch)_$(host_os)_id_string+=$(shell $(host_AR) --version 2>/dev/null)
$(host_arch)_$(host_os)_id_string+=$(shell $(host_CXX) --version 2>/dev/null)
$(host_arch)_$(host_os)_id_string+=$(shell $(host_RANLIB) --version 2>/dev/null)
$(host_arch)_$(host_os)_id_string+=$(shell $(host_STRIP) --version 2>/dev/null)
else
build_id_string:=$(realpath $(GUIX_ENVIRONMENT))
$(host_arch)_$(host_os)_id_string:=$(realpath $(GUIX_ENVIRONMENT))
endif

packages += $($(host_arch)_$(host_os)_packages) $($(host_os)_packages)
native_packages += $($(host_arch)_$(host_os)_native_packages) $($(host_os)_native_packages)

all_packages = $(packages) $(native_packages)

meta_depends = Makefile funcs.mk builders/default.mk hosts/default.mk hosts/$(host_os).mk builders/$(build_os).mk

$(host_arch)_$(host_os)_native_toolchain?=$($(host_os)_native_toolchain)

include funcs.mk

toolchain_path=$($($(host_arch)_$(host_os)_native_toolchain)_prefixbin)
final_build_id_long+=:[sha256sum]:$(shell $(build_SHA256SUM) toolchain.cmake.in)
final_build_id+=$(shell echo -n "$(final_build_id_long)" | $(build_SHA256SUM) | cut -c-$(HASH_LENGTH))
$(host_prefix)/.stamp_$(final_build_id): $(native_packages) $(packages)
	$(AT)rm -rf $(@D)
	$(AT)mkdir -p $(@D)
	$(AT)echo copying packages: $^
	$(AT)echo to: $(@D)
	$(AT)cd $(@D); $(foreach package,$^, tar xf $($(package)_cached); )
	$(AT)touch $@

$(host_prefix)/share/toolchain.cmake : toolchain.cmake.in $(host_prefix)/.stamp_$(final_build_id)
	$(AT)@mkdir -p $(@D)
	$(AT)sed -e 's|@HOST@|$(host)|' \
            -e 's|@CC@|$(host_CC)|' \
            -e 's|@CXX@|$(host_CXX)|' \
            -e 's|@AR@|$(toolchain_path)$(host_AR)|' \
            -e 's|@RANLIB@|$(toolchain_path)$(host_RANLIB)|' \
            -e 's|@NM@|$(toolchain_path)$(host_NM)|' \
            -e 's|@STRIP@|$(toolchain_path)$(host_STRIP)|' \
            -e 's|@build_os@|$(build_os)|' \
            -e 's|@host_os@|$(host_os)|' \
            -e 's|@CFLAGS@|$(strip $(host_CFLAGS) $(host_$(release_type)_CFLAGS))|' \
            -e 's|@CXXFLAGS@|$(strip $(host_CXXFLAGS) $(host_$(release_type)_CXXFLAGS))|' \
            -e 's|@CPPFLAGS@|$(strip $(host_CPPFLAGS) $(host_$(release_type)_CPPFLAGS))|' \
            -e 's|@LDFLAGS@|$(strip $(host_LDFLAGS) $(host_$(release_type)_LDFLAGS))|' \
            -e 's|@allow_host_packages@|$(ALLOW_HOST_PACKAGES)|' \
            -e 's|@debug@|$(DEBUG)|' \
            -e 's|@release_type@|$(cmake_release_type)|' \
            -e 's|@build_tests@|$(build_tests)|' \
            -e 's|@cmake_system_name@|$($(host_os)_cmake_system)|' \
            -e 's|@prefix@|$($(host_arch)_$(host_os)_prefix)|'\
            -e 's|@arch@|$(host_arch)|'\
            $< > $@
	$(AT)touch $@

define check_or_remove_cached
  mkdir -p $(BASE_CACHE)/$(host)/$(package) && cd $(BASE_CACHE)/$(host)/$(package); \
  $(build_SHA256SUM) -c $($(package)_cached_checksum) >/dev/null 2>/dev/null || \
  ( rm -f $($(package)_cached_checksum); \
    if test -f "$($(package)_cached)"; then echo "Checksum mismatch for $(package). Forcing rebuild.."; rm -f $($(package)_cached_checksum) $($(package)_cached); fi )
endef

define check_or_remove_sources
  mkdir -p $($(package)_source_dir); cd $($(package)_source_dir); \
  test -f $($(package)_fetched) && ( $(build_SHA256SUM) -c $($(package)_fetched) >/dev/null 2>/dev/null || \
    ( echo "Checksum missing or mismatched for $(package) source. Forcing re-download."; \
      rm -f $($(package)_all_sources) $($(1)_fetched))) || true
endef

check-packages:
	@$(foreach package,$(all_packages),$(call check_or_remove_cached,$(package));)
check-sources:
	@$(foreach package,$(all_packages),$(call check_or_remove_sources,$(package));)

$(host_prefix)/share/toolchain.cmake: check-packages

check-packages: check-sources

install: check-packages $(host_prefix)/share/toolchain.cmake

download-one: check-sources $(all_sources)

download-osx:
	@$(MAKE) -s HOST=x86_64-apple-darwin download-one
download-linux:
	@$(MAKE) -s HOST=x86_64-unknown-linux-gnu download-one
download-win:
	@$(MAKE) -s HOST=x86_64-w64-mingw32 download-one
download-freebsd:
	@$(MAKE) -s HOST=x86_64-unknown-freebsd download-one
download-android:
	@$(MAKE) -s HOST=aarch64-linux-android download-one
download: download-osx download-linux download-win download-freebsd download-android

 $(foreach package,$(all_packages),$(eval $(call ext_add_stages,$(package))))

.PHONY: install cached download-one download-osx download-linux download-win download-freebsd download-android download check-packages check-sources
.PHONY: FORCE
