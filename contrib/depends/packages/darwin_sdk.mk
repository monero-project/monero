package=darwin_sdk
$(package)_version=15.0
$(package)_download_path=https://bitcoincore.org/depends-sources/sdks
$(package)_file_name=Xcode-15.0-15A240d-extracted-SDK-with-libcxx-headers.tar.gz
$(package)_sha256_hash=c0c2e7bb92c1fee0c4e9f3a485e4530786732d6c6dd9e9f418c282aa6892f55d

# Prevent clang from including readline headers from the SDK. We statically link
# our own version of readline.

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/SDK &&\
  rm -rf usr/include/readline && \
  mv * $($(package)_staging_prefix_dir)/SDK
endef
