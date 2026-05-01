FROM stagex/pallet-gcc-gnu-gnu:sx2026.03.0@sha256:53b3f603878fd72024d839477fe4889636a998df22e4f56031e49c48afad960c as builder
COPY --from=stagex/core-curl:sx2026.03.0@sha256:213387cf483d9d407081d06376239d87476b4a002339154b936b1f8c0f1ce4bc . /
COPY --from=stagex/core-openssl:sx2026.03.0@sha256:decacae13674bebbeaef3ce79c5d1159ae8495f272f8fa7e5db660c84ab37d1d . /
COPY --from=stagex/core-ca-certificates:sx2026.03.0@sha256:4d901fb99baee5f461ede66e37edf2505d9d367222feded99e80892b1940f710 . /
COPY --from=stagex/user-patch:sx2026.03.0@sha256:dafe08ac952eb49aa48821684b1402de03a132dbbf7a814916a189af4c13e363 . /
COPY --from=stagex/core-cmake:sx2026.03.0@sha256:c6fa4b854d0e91ca15a4d04d568d97cd82d70e2c1637570ca1b09b68e09029c2 . /
COPY --from=stagex/core-ncurses:sx2026.03.0@sha256:080694dbb4da85bb43e0772f3d42dc4a396e77b8e047a5020b9c266e9bd18212 . /

ENV TARGET="x86_64-pc-linux-musl"
WORKDIR monero
COPY . .

RUN make -C contrib/depends -j$(nproc) download-linux NO_WALLET=1 NO_READLINE=1

RUN make -C contrib/depends -j$(nproc) HOST="${TARGET}" NO_WALLET=1 NO_READLINE=1

RUN cmake --toolchain "contrib/depends/${TARGET}/share/toolchain.cmake" -S . -B build \
        -DSTACK_TRACE=OFF \
        -DBUILD_WALLET=OFF \
        -DUSE_READLINE=OFF \
        -DSTATIC_FLAGS="-static-pie" && \
    cmake --build build --target daemon --parallel $(nproc)

FROM scratch

COPY --from=builder /monero/build/bin/monerod /

EXPOSE 18080
EXPOSE 18081

ENTRYPOINT ["/monerod"]
CMD ["--p2p-bind-ip=0.0.0.0", "--p2p-bind-port=18080", "--rpc-bind-ip=0.0.0.0", "--rpc-bind-port=18081", "--non-interactive", "--confirm-external-bind"]
