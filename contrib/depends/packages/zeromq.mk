# --- Package Definition ---
package=zeromq
$(package)_version=4.3.5
$(package)_download_path=https://github.com/zeromq/libzmq/releases/download/v$($(package)_version)/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=6653ef5910f17954861fe72332e68b03ca6e4d9c7160eb3a8de5a5a913bfab43

# --- Build Variable Configuration ---
define $(package)_set_vars
  # Security & Size Optimization:
  # --disable-shared: Static linking prevents DLL hijacking and reduces runtime overhead.
  # --disable-curve: Disabling Curve to reduce external dependencies (libsodium).
  # --without-documentation: Reduces build time and artifact size.
  $(package)_config_opts=--without-documentation --disable-shared --without-libsodium --disable-curve --enable-static
  $(package)_cxxflags=-std=c++11 -fPIC
endef



# --- Pre-processing Phase ---
define $(package)_preprocess_cmds
  # Ensures cross-compilation compatibility by updating config scripts
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub config
endef

# --- Configuration Phase ---
define $(package)_config_cmds
  # Executes Autoconf with custom AR_FLAGS for consistent static library creation
  $($(package)_autoconf) AR_FLAGS=$($(package)_arflags)
endef

# --- Build Phase ---
define $(package)_build_cmds
  # Only build the core library object to save resources
  $(MAKE) src/libzmq.la
endef

# --- Staging (Installation) Phase ---
define $(package)_stage_cmds
  # Selective installation: Only headers, static libs, and pkgconfig data
  $(MAKE) DESTDIR=$($(package)_staging_dir) install-libLTLIBRARIES install-includeHEADERS install-pkgconfigDATA
endef



# --- Post-processing Phase ---
define $(package)_postprocess_cmds
  # Security Hardening: Remove unused binaries and development metadata (.la files)
  # This prevents leaking build-path information and reduces the package footprint.
  rm -rf bin share && \
  rm -f lib/*.la
endef
