#!/usr/bin/env bash

mkdir bin

triple=x86_64-unknown-freebsd11

for tool in clang clang++; do
  tool_path=bin/${triple}-${tool}
  cat > "$tool_path" <<EOF
#!/bin/sh
exec env -u C_INCLUDE_PATH -u CPLUS_INCLUDE_PATH \
                    -u OBJC_INCLUDE_PATH -u OBJCPLUS_INCLUDE_PATH -u CPATH \
                    -u LIBRARY_PATH \
                    $tool --sysroot=${host_prefix}/native/toolchain --prefix=${host_prefix}/native/toolchain/bin "\$@" --target=${triple}
EOF
  chmod +x "$tool_path"
done
