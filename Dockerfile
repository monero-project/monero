FROM stagex/pallet-gcc-gnu-gnu:sx2026.01.0@sha256:d03941bcc6efc36f3b4840d9f6fd74cbecbedc181257e51401c704acaa3cfce8 as builder
COPY --from=stagex/core-curl:sx2026.01.0@sha256:4a1df45afd9226852857b4b7f39d3ab7cf52a0fb284e3052b0bcb9d8293e9462 . /
COPY --from=stagex/core-openssl:sx2026.01.0@sha256:4ecf4f42ad958a25d4622b246aacdbccd523aacbb8ce036f387d39e8a17b9840 . /
COPY --from=stagex/core-ca-certificates:sx2026.01.0@sha256:90d28341b8002c1930616e3fb1d20927ab07e7bab2533a6348c572a32c8c3d00 . /
COPY --from=stagex/user-patch:sx2026.01.0@sha256:c1fab61b5ccefa16333d716e0b086167a84863f75d9df78e1e88c13ea1b00be7 . /
COPY --from=stagex/core-cmake:sx2026.01.0@sha256:2135313a387610b1365b38cdf83fa16db1f49aec7dccd04a41243ca355c7c4bb . /
COPY --from=stagex/core-ncurses:sx2026.01.0@sha256:7ac2ad988862989d1da45fffb21c4218c2c1766e7cdbfb4a4479ac57cff6bb6b . /

ENV TARGET="x86_64-pc-linux-musl"
WORKDIR monero
COPY . .

RUN make -C contrib/depends -j$(nproc) download-linux NO_WALLET=1 NO_READLINE=1

RUN make -C contrib/depends -j$(nproc) HOST="${TARGET}" NO_WALLET=1 NO_READLINE=1

RUN cmake --toolchain "contrib/depends/${TARGET}/share/toolchain.cmake" -S . -B build \
        -DSTACK_TRACE=OFF \
        -DBUILD_WALLET=OFF \
        -DUSE_READLINE=OFF \
        -DCMAKE_EXE_LINKER_FLAGS="-static-pie" && \
    cmake --build build --target daemon --parallel $(nproc)

FROM scratch

COPY --from=builder /monero/build/bin/monerod /

EXPOSE 18080
EXPOSE 18081

ENTRYPOINT ["/monerod"]
CMD ["--p2p-bind-ip=0.0.0.0", "--p2p-bind-port=18080", "--rpc-bind-ip=0.0.0.0", "--rpc-bind-port=18081", "--non-interactive", "--confirm-external-bind"]
