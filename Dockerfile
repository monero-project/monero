FROM stagex/pallet-gcc-gnu-gnu:sx2026.05.0@sha256:c06b4e5e490d7fdc77951510b5256f889a8bd51c6fa9d6c41c7f6c4cea88f35c AS builder
COPY --from=stagex/core-curl:sx2026.05.0@sha256:7a95abfe88eea0a7afd614d219e0b0f11fd77ce257046489baa0fbbf2fc6c088 . /
COPY --from=stagex/core-openssl:sx2026.05.0@sha256:9a8cac9cacc5fdcbd09f0cf36b1895f71c02fb92fc9806fae824de1d18f60bb0 . /
COPY --from=stagex/core-ca-certificates:sx2026.05.0@sha256:7773dae6630aa3bdcc82cfec6c9265c0c501aaf0af67cc73631b09e1cff1b094 . /
COPY --from=stagex/user-patch:sx2026.05.0@sha256:1d4428893f0ea9abfabc1fb5e365c5593fe10c6ed8ffc592d6528157a4299942 . /
COPY --from=stagex/core-cmake:sx2026.05.0@sha256:ac023f4f1dfb1f7fb649c63de4f54f072ca206f43311807b1e1fd21edecaf8fe . /
COPY --from=stagex/core-ncurses:sx2026.05.0@sha256:90cc5d029c5073405f9db39c88b9509b8959bbd8f19d8cd02c20e9350cc40254 . /

ENV TARGET="x86_64-pc-linux-musl"
WORKDIR /monero
COPY . .

RUN make -C contrib/depends -j$(nproc) download-linux NO_WALLET=1 NO_READLINE=1

RUN make -C contrib/depends -j$(nproc) HOST="${TARGET}" NO_WALLET=1 NO_READLINE=1

RUN cmake --toolchain "contrib/depends/${TARGET}/share/toolchain.cmake" -S . -B build \
        -DSTACK_TRACE=OFF \
        -DBUILD_WALLET=OFF \
        -DUSE_READLINE=OFF \
        -DUSE_DEVICE_TREZOR=OFF \
        -DSTATIC_FLAGS="-static-pie" && \
    cmake --build build --target daemon --parallel $(nproc)

FROM scratch

COPY --from=builder /monero/build/bin/monerod /

EXPOSE 18080
EXPOSE 18081

ENTRYPOINT ["/monerod"]
CMD ["--p2p-bind-ip=0.0.0.0", "--p2p-bind-port=18080", "--rpc-bind-ip=0.0.0.0", "--rpc-bind-port=18081", "--non-interactive", "--confirm-external-bind"]
