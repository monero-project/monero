package=darwin_sdk
$(package)_version=12.2
$(package)_download_path=https://bitcoincore.org/depends-sources/sdks
$(package)_file_name=Xcode-12.2-12B45b-extracted-SDK-with-libcxx-headers.tar.gz
$(package)_sha256_hash=df75d30ecafc429e905134333aeae56ac65fac67cb4182622398fd717df77619

# Prevent clang from including readline headers from the SDK. We statically link
# our own version of readline.

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/SDK &&\
  rm -rf usr/include/readline && \
  mv * $($(package)_staging_prefix_dir)/SDK
endef
