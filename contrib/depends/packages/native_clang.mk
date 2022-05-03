package=native_clang
$(package)_version=3.7.1
$(package)_download_path=http://llvm.org/releases/$($(package)_version)
$(package)_download_file=clang+llvm-$($(package)_version)-x86_64-linux-gnu-ubuntu-14.04.tar.xz
$(package)_file_name=clang-llvm-$($(package)_version)-x86_64-linux-gnu-ubuntu-14.04.tar.xz
$(package)_sha256_hash=99b28a6b48e793705228a390471991386daa33a9717cd9ca007fcdde69608fd9

define $(package)_extract_cmds
  echo $($(package)_sha256_hash) $($(package)_source) | sha256sum -c &&\
  mkdir -p toolchain/bin toolchain/lib/clang/3.5/include && \
  tar --strip-components=1 -C toolchain -xf $($(package)_source) && \
  rm -f toolchain/lib/libc++abi.so* && \
  echo "#!/bin/sh" > toolchain/bin/$(host)-dsymutil && \
  echo "exit 0" >> toolchain/bin/$(host)-dsymutil && \
  chmod +x toolchain/bin/$(host)-dsymutil
endef

define $(package)_stage_cmds
  cd $($(package)_extract_dir)/toolchain && \
  mkdir -p $($(package)_staging_prefix_dir)/lib/clang/$($(package)_version)/include && \
  mkdir -p $($(package)_staging_prefix_dir)/bin $($(package)_staging_prefix_dir)/include && \
  cp bin/clang $($(package)_staging_prefix_dir)/bin/ &&\
  cp -P bin/clang++ $($(package)_staging_prefix_dir)/bin/ &&\
  cp lib/libLTO.so $($(package)_staging_prefix_dir)/lib/ && \
  cp -rf lib/clang/$($(package)_version)/include/* $($(package)_staging_prefix_dir)/lib/clang/$($(package)_version)/include/ && \
  cp bin/llvm-dsymutil $($(package)_staging_prefix_dir)/bin/$(host)-dsymutil && \
  if `test -d include/c++/`; then cp -rf include/c++/ $($(package)_staging_prefix_dir)/include/; fi && \
  if `test -d lib/c++/`; then cp -rf lib/c++/ $($(package)_staging_prefix_dir)/lib/; fi
endef
