package=darwin_sdk
$(package)_version=26.1.1
$(package)_download_path=https://bitcoincore.org/depends-sources/sdks
$(package)_file_name=Xcode-26.1.1-17B100-extracted-SDK-with-libcxx-headers.tar
$(package)_sha256_hash=9600fa93644df674ee916b5e2c8a6ba8dacf631996a65dc922d003b98b5ea3b1

# Prevent clang from including readline headers from the SDK. We statically link
# our own version of readline.

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/SDK &&\
  rm -rf usr/include/readline && \
  mv * $($(package)_staging_prefix_dir)/SDK
endef
