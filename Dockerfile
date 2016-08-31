FROM debian:testing
MAINTAINER eiabea <developer@eiabea.com>

RUN set -e && \
  apt-get update -q && \
  apt-get install -q -y --no-install-recommends build-essential ca-certificates git g++ gcc cmake pkg-config libunbound2 libevent-2.0-5 libgtest-dev libboost-all-dev libdb5.3++-dev libdb5.3-dev libssl-dev

ENV GIT_BRANCH master

RUN git clone --branch $GIT_BRANCH https://github.com/monero-project/bitmonero.git
WORKDIR /bitmonero

ENV BUILD_THREADS 4
RUN make -j $BUILD_THREADS

WORKDIR /bitmonero/build/release/bin

VOLUME /root/.bitmonero

ENV LOG_LEVEL 0
ENV P2P_BIND_IP 0.0.0.0
ENV P2P_BIND_PORT 18080
ENV RPC_BIND_IP 127.0.0.1
ENV RPC_BIND_PORT 18081

CMD ./bitmonerod --log-level=$LOG_LEVEL --p2p-bind-ip=$P2P_BIND_IP --p2p-bind-port=$P2P_BIND_PORT --rpc-bind-ip=$RPC_BIND_IP --rpc-bind-port=$RPC_BIND_PORT
