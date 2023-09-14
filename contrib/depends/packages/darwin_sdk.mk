package=darwin_sdk
#$(package)_version=11.1
#$(package)_download_path=https://github.com/phracker/MacOSX-SDKs/releases/download/11.3/
#$(package)_file_name=MacOSX$($(package)_version).sdk.tar.xz
#$(package)_sha256_hash=68797baaacb52f56f713400de306a58a7ca00b05c3dc6d58f0a8283bcac721f8

#$(package)_version=11.3
#$(package)_download_path=https://github.com/phracker/MacOSX-SDKs/releases/download/11.3/
#$(package)_file_name=MacOSX$($(package)_version).sdk.tar.xz
#$(package)_sha256_hash=cd4f08a75577145b8f05245a2975f7c81401d75e9535dcffbb879ee1deefcbf4

#$(package)_version=12.3
#$(package)_download_path=https://github.com/joseluisq/macosx-sdks/releases/download/12.3/
#$(package)_file_name=MacOSX$($(package)_version).sdk.tar.xz
#$(package)_sha256_hash=3abd261ceb483c44295a6623fdffe5d44fc4ac2c872526576ec5ab5ad0f6e26c

#$(package)_version=12.3
#$(package)_download_path=https://github.com/roblabla/MacOSX-SDKs/releases/download/12.x/
#$(package)_file_name=MacOSX$($(package)_version).sdk.tar.xz
#$(package)_sha256_hash=5a7dc597666f07d11bc5cabe37f1c88b63b19de2931e6a2fb31de3154f38dd43

# Use this 13.1 - works
$(package)_version=13.1
$(package)_download_path=https://github.com/roblabla/MacOSX-SDKs/releases/download/13.1/
$(package)_file_name=MacOSX$($(package)_version).sdk.tar.xz
$(package)_sha256_hash=5dbb5c44961f14141d7eaa989ccd57098589c9d74eafbc807bb5beeb3c0540de

# TODO:
# https://github.com/mozilla/gecko-dev/blob/585fe519f14ca8f241370573a902fc6d53cf8ac6/taskcluster/ci/toolchain/macos-sdk.yml#L49
#             - https://swdist.apple.com/content/downloads/38/50/012-70652-A_2ZHIRHCHLN/f8b81s6tzlzrj0z67ynydjx4x1lwhr08ab/CLTools_macOSNMOS_SDK.pkg
#            - 06f4a045854c456a553a5ee6acf678fbe26c06296fc68054ae918c206134aa20
#            - Library/Developer/CommandLineTools/SDKs/MacOSX13.0.sdk
# https://github.com/mozilla/gecko-dev/blob/585fe519f14ca8f241370573a902fc6d53cf8ac6/taskcluster/scripts/misc/unpack-sdk.py
# https://github.com/mozilla/gecko-dev/blob/585fe519f14ca8f241370573a902fc6d53cf8ac6/python/mozbuild/mozpack/macpkg.py

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_dir)/$(host_prefix)/native/SDK &&\
  mv * $($(package)_staging_dir)/$(host_prefix)/native/SDK
endef
