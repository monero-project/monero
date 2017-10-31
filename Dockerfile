FROM ubuntu:16.04

ENV SRC_DIR /usr/local/src/electroneum

RUN set -x \
  && buildDeps=' \
      ca-certificates \
      cmake \
      g++ \
      git \
      libboost1.58-all-dev \
      libssl-dev \
      make \
      pkg-config \
  ' \
  && apt-get -qq update \
  && apt-get -qq --no-install-recommends install $buildDeps

RUN git clone https://github.com/electroneum/electroneum.git $SRC_DIR
WORKDIR $SRC_DIR
RUN make -j$(nproc) release-static

RUN cp build/release/bin/* /usr/local/bin/ \
  \
  && rm -r $SRC_DIR \
  && apt-get -qq --auto-remove purge $buildDeps

# Contains the blockchain
VOLUME /root/.electroneum

# Generate your wallet via accessing the container and run:
# cd /wallet
# electroneum-wallet-cli
VOLUME /wallet

ENV LOG_LEVEL 0
ENV P2P_BIND_IP 0.0.0.0
ENV P2P_BIND_PORT 26967
ENV RPC_BIND_IP 127.0.0.1
ENV RPC_BIND_PORT 26968

EXPOSE 26967
EXPOSE 26968

CMD electroneumd --log-level=$LOG_LEVEL --p2p-bind-ip=$P2P_BIND_IP --p2p-bind-port=$P2P_BIND_PORT --rpc-bind-ip=$RPC_BIND_IP --rpc-bind-port=$RPC_BIND_PORT
