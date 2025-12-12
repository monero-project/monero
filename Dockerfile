FROM stagex/pallet-gcc-gnu-gnu:sx2025.10.0@sha256:dbf70eae3f75fdaf72d76b7b55684362910d085f470ab850fdd81cd96871e350 as builder
COPY --from=stagex/core-curl:sx2025.10.0@sha256:bc8bab43d96a9167fbb85022ea773644a45ef335e7a9b747f203078973fa988e . /
COPY --from=stagex/core-openssl:sx2025.10.0@sha256:d6487f0cb15f4ee02b420c717cb9abd85d73043c0bb3a2c6ce07688b23c1df07 . /
COPY --from=stagex/core-ca-certificates:sx2025.10.0@sha256:d135f1189e9b232eb7316626bf7858534c5540b2fc53dced80a4c9a95f26493e . /
COPY --from=stagex/user-patch:sx2025.10.0@sha256:cbcc7a5cbbee974634c933d08ece441e91d6907673ce2310e5f61ef63b7d1bb5 . /
COPY --from=stagex/core-cmake:sx2025.10.0@sha256:e6cea8207c83860aa49f57c509740f12588d043a00bf26261b18dccc4f03a59d . /

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
