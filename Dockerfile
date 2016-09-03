FROM debian:testing
MAINTAINER eiabea <developer@eiabea.com>

# Install clone dependencies
RUN set -e && \
  apt-get update -q && \
  apt-get install -q -y --no-install-recommends ca-certificates git && \
  git clone https://github.com/monero-project/monero.git src && \
  apt-get purge -y git && \
  apt-get clean -q -y && \
  apt-get autoclean -q -y && \
  apt-get autoremove -q -y

WORKDIR /src

# Install make dependencies
RUN set -e && \
  apt-get update -q && \
  apt-get install -q -y --no-install-recommends build-essential ca-certificates g++ gcc cmake \
  pkg-config libunbound2 libevent-2.0-5 libgtest-dev libboost-all-dev libdb5.3++-dev libdb5.3-dev libssl-dev && \
  make -j 4 && \
  apt-get purge -y g++ gcc cmake pkg-config && \
  apt-get clean -q -y && \
  apt-get autoclean -q -y && \
  apt-get autoremove -q -y && \
  mkdir /monero && \
  mv /src/build/release/bin/* /monero && \
  rm -rf /src

WORKDIR /monero

# Contains the blockchain
VOLUME /root/.bitmonero

# Generate your wallet via accessing the container and run:
# cd /wallet
# /./bitmonero/simplewallet
VOLUME /wallet

ENV LOG_LEVEL 0
ENV P2P_BIND_IP 0.0.0.0
ENV P2P_BIND_PORT 18080
ENV RPC_BIND_IP 127.0.0.1
ENV RPC_BIND_PORT 18081

EXPOSE 18080
EXPOSE 18081

CMD ./monerod --log-level=$LOG_LEVEL --p2p-bind-ip=$P2P_BIND_IP --p2p-bind-port=$P2P_BIND_PORT --rpc-bind-ip=$RPC_BIND_IP --rpc-bind-port=$RPC_BIND_PORT