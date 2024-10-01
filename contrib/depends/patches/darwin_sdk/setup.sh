#!/usr/bin/env bash

mkdir bin

target=${host}

OSX_SDK=${host_prefix}/native/SDK

for tool in clang clang++; do
  tool_path=bin/${target}-${tool}
  cat > "$tool_path" <<EOF
#!/bin/sh
exec env -u C_INCLUDE_PATH -u CPLUS_INCLUDE_PATH \
         -u OBJC_INCLUDE_PATH -u OBJCPLUS_INCLUDE_PATH -u CPATH \
         -u LIBRARY_PATH \
            ${tool} --target=${target} \
                          -B${host_prefix}/native/bin \
                          -isysroot${OSX_SDK} -nostdlibinc \
                          -iwithsysroot/usr/include \
                          -iframeworkwithsysroot/System/Library/Frameworks \
                          -pipe -mmacosx-version-min=11 -mlinker-version=711 -Wl,-platform_version,macos,11,11.0 -Wl,-no_adhoc_codesign -fuse-ld=lld "\$@"
EOF
  chmod +x "$tool_path"
done
