package=native_clang
#$(package)_version=9.0.0
#$(package)_download_path=https://releases.llvm.org/$($(package)_version)
#$(package)_download_file=clang+llvm-$($(package)_version)-x86_64-linux-gnu-ubuntu-18.04.tar.xz
#$(package)_file_name=clang-llvm-$($(package)_version)-x86_64-linux-gnu-ubuntu-18.04.tar.xz
#$(package)_sha256_hash=a23b082b30c128c9831dbdd96edad26b43f56624d0ad0ea9edec506f5385038d

#$(package)_version=10.0.0
#$(package)_download_path=https://github.com/llvm/llvm-project/releases/download/llvmorg-$($(package)_version)
#$(package)_download_file=clang+llvm-$($(package)_version)-x86_64-linux-gnu-ubuntu-18.04.tar.xz
#$(package)_file_name=clang-llvm-$($(package)_version)-x86_64-linux-gnu-ubuntu-18.04.tar.xz
#$(package)_sha256_hash=b25f592a0c00686f03e3b7db68ca6dc87418f681f4ead4df4745a01d9be63843

#$(package)_version=11.0.0
#$(package)_download_path=https://github.com/llvm/llvm-project/releases/download/llvmorg-$($(package)_version)
#$(package)_download_file=clang+llvm-$($(package)_version)-x86_64-linux-gnu-ubuntu-20.04.tar.xz
#$(package)_file_name=clang-llvm-$($(package)_version)-x86_64-linux-gnu-ubuntu-20.04.tar.xz
#$(package)_sha256_hash=829f5fb0ebda1d8716464394f97d5475d465ddc7bea2879c0601316b611ff6db

$(package)_version=12.0.0
$(package)_download_path=https://github.com/llvm/llvm-project/releases/download/llvmorg-$($(package)_version)
$(package)_download_file=clang+llvm-$($(package)_version)-x86_64-linux-gnu-ubuntu-20.04.tar.xz
$(package)_file_name=clang-llvm-$($(package)_version)-x86_64-linux-gnu-ubuntu-20.04.tar.xz
$(package)_sha256_hash=a9ff205eb0b73ca7c86afc6432eed1c2d49133bd0d49e47b15be59bbf0dd292e

#$(package)_version=14.0.0
#$(package)_download_path=https://github.com/llvm/llvm-project/releases/download/llvmorg-$($(package)_version)
#$(package)_download_file=clang+llvm-$($(package)_version)-x86_64-linux-gnu-ubuntu-18.04.tar.xz
#$(package)_file_name=clang-llvm-$($(package)_version)-x86_64-linux-gnu-ubuntu-18.04.tar.xz
#$(package)_sha256_hash=61582215dafafb7b576ea30cc136be92c877ba1f1c31ddbbd372d6d65622fef5


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
  cp bin/dsymutil $($(package)_staging_prefix_dir)/bin/$(host)-dsymutil && \
  if `test -d include/c++/`; then cp -rf include/c++/ $($(package)_staging_prefix_dir)/include/; fi && \
  if `test -d lib/c++/`; then cp -rf lib/c++/ $($(package)_staging_prefix_dir)/lib/; fi
endef
