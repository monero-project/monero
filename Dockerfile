# Multistage docker build, requires docker 17.05

# builder stage
FROM ubuntu:20.04 as builder

RUN set -ex && \
    apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get --no-install-recommends --yes install \
        automake \
        autotools-dev \
        bsdmainutils \
        build-essential \
        ca-certificates \
        ccache \
        cmake \
        curl \
        git \
        make \
        libtool \
        pkg-config \
        gperf \
        libusb-1.0-0-dev \
        libhidapi-dev \
        libprotobuf-dev \
        protobuf-compiler \
        libssl-dev \
        libunbound-dev \
        libboost-all-dev \
        libsodium-dev \
        libzmq3-dev && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

ARG NPROC
RUN set -ex && \
    git submodule init && \
    git submodule update && \
    echo "Submodules initialized and updated" && \
    rm -rf build && \
    mkdir build && \
    cd build && \
    cmake .. -DARCH="default" -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release && \
    echo "CMake configuration completed" && \
    if [ -z "$NPROC" ] ; then make -j$(nproc) ; else make -j$NPROC ; fi && \
    echo "Build completed"

# runtime stage
FROM ubuntu:20.04

RUN set -ex && \
    apt-get update && \
    apt-get --no-install-recommends --yes install ca-certificates && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

COPY --from=builder /src/build /usr/local/bin/

# Create monero user
RUN adduser --system --group --disabled-password monero

# Create monero user
RUN adduser --system --group --disabled-password monero && \
    mkdir -p /wallet /home/monero/.bitmonero && \
    chown -R monero:monero /home/monero/.bitmonero && \
    chown -R monero:monero /wallet

# Contains the blockchain
VOLUME /home/monero/.bitmonero

# Generate your wallet via accessing the container and run:
# cd /wallet
# monero-wallet-cli
VOLUME /wallet

EXPOSE 18080
EXPOSE 18081

# switch to user monero
USER monero

ENTRYPOINT ["monerod"]
CMD ["--p2p-bind-ip=0.0.0.0", "--p2p-bind-port=18080", "--rpc-bind-ip=0.0.0.0", "--rpc-bind-port=18081", "--non-interactive", "--confirm-external-bind"]