FROM stagex/pallet-clang-gnu-busybox

# Todo: minimalize & pin

COPY --from=stagex/core-bash . /
COPY --from=stagex/core-lld . /

# CMake
COPY --from=stagex/core-cmake . /
COPY --from=stagex/core-openssl . /

COPY --from=stagex/core-git . /
COPY --from=stagex/core-bzip2 . /

# Needed for android
COPY --from=stagex/user-glibc . /

ENTRYPOINT ["/bin/env"]
