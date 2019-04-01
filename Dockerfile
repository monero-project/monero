# Multistage docker build, requires docker 17.05

# builder stage
FROM ubuntu:16.04 as builder

RUN set -ex && \
    apt-get update && \
    apt-get --no-install-recommends --yes install \
        ca-certificates \
        cmake \
        g++ \
        make \
        pkg-config \
        graphviz \
        doxygen \
        git \
        curl \
        libtool-bin \
        autoconf \
        automake \
        bzip2 \
        xsltproc \
        gperf \
        unzip

WORKDIR /usr/local

ENV CFLAGS='-fPIC'
ENV CXXFLAGS='-fPIC'

#Cmake
ARG CMAKE_VERSION=3.14.0
ARG CMAKE_VERSION_DOT=v3.14
ARG CMAKE_HASH=aa76ba67b3c2af1946701f847073f4652af5cbd9f141f221c97af99127e75502
RUN set -ex \
    && curl -s -O https://cmake.org/files/${CMAKE_VERSION_DOT}/cmake-${CMAKE_VERSION}.tar.gz \
    && echo "${CMAKE_HASH}  cmake-${CMAKE_VERSION}.tar.gz" | sha256sum -c \
    && tar -xzf cmake-${CMAKE_VERSION}.tar.gz \
    && cd cmake-${CMAKE_VERSION} \
    && ./configure \
    && make \
    && make install

## Boost
ARG BOOST_VERSION=1_69_0
ARG BOOST_VERSION_DOT=1.69.0
ARG BOOST_HASH=8f32d4617390d1c2d16f26a27ab60d97807b35440d45891fa340fc2648b04406
RUN set -ex \
    && curl -s -L -o  boost_${BOOST_VERSION}.tar.bz2 https://dl.bintray.com/boostorg/release/${BOOST_VERSION_DOT}/source/boost_${BOOST_VERSION}.tar.bz2 \
    && echo "${BOOST_HASH}  boost_${BOOST_VERSION}.tar.bz2" | sha256sum -c \
    && tar -xvf boost_${BOOST_VERSION}.tar.bz2 \
    && cd boost_${BOOST_VERSION} \
    && ./bootstrap.sh \
    && ./b2 --build-type=minimal link=static runtime-link=static --with-chrono --with-date_time --with-filesystem --with-program_options --with-regex --with-serialization --with-system --with-thread --with-locale threading=multi threadapi=pthread cflags="$CFLAGS" cxxflags="$CXXFLAGS" stage
ENV BOOST_ROOT /usr/local/boost_${BOOST_VERSION}

# OpenSSL
ARG OPENSSL_VERSION=1.1.1b
ARG OPENSSL_HASH=5c557b023230413dfb0756f3137a13e6d726838ccd1430888ad15bfb2b43ea4b
RUN set -ex \
    && curl -s -O https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz \
    && echo "${OPENSSL_HASH}  openssl-${OPENSSL_VERSION}.tar.gz" | sha256sum -c \
    && tar -xzf openssl-${OPENSSL_VERSION}.tar.gz \
    && cd openssl-${OPENSSL_VERSION} \
    && ./Configure linux-x86_64 no-shared --static "$CFLAGS" \
    && make build_generated \
    && make libcrypto.a \
    && make install
ENV OPENSSL_ROOT_DIR=/usr/local/openssl-${OPENSSL_VERSION}

# ZMQ
ARG ZMQ_VERSION=v4.3.1
ARG ZMQ_HASH=2cb1240db64ce1ea299e00474c646a2453a8435b
RUN set -ex \
    && git clone https://github.com/zeromq/libzmq.git -b ${ZMQ_VERSION} \
    && cd libzmq \
    && test `git rev-parse HEAD` = ${ZMQ_HASH} || exit 1 \
    && ./autogen.sh \
    && ./configure --enable-static --disable-shared \
    && make \
    && make install \
    && ldconfig

# zmq.hpp
ARG CPPZMQ_VERSION=v4.3.0
ARG CPPZMQ_HASH=213da0b04ae3b4d846c9abc46bab87f86bfb9cf4
RUN set -ex \
    && git clone https://github.com/zeromq/cppzmq.git -b ${CPPZMQ_VERSION} \
    && cd cppzmq \
    && test `git rev-parse HEAD` = ${CPPZMQ_HASH} || exit 1 \
    && mv *.hpp /usr/local/include

# Readline
ARG READLINE_VERSION=8.0
ARG READLINE_HASH=e339f51971478d369f8a053a330a190781acb9864cf4c541060f12078948e461
RUN set -ex \
    && curl -s -O https://ftp.gnu.org/gnu/readline/readline-${READLINE_VERSION}.tar.gz \
    && echo "${READLINE_HASH}  readline-${READLINE_VERSION}.tar.gz" | sha256sum -c \
    && tar -xzf readline-${READLINE_VERSION}.tar.gz \
    && cd readline-${READLINE_VERSION} \
    && ./configure \
    && make \
    && make install

# Sodium
ARG SODIUM_VERSION=1.0.17
ARG SODIUM_HASH=b732443c442239c2e0184820e9b23cca0de0828c
RUN set -ex \
    && git clone https://github.com/jedisct1/libsodium.git -b ${SODIUM_VERSION} \
    && cd libsodium \
    && test `git rev-parse HEAD` = ${SODIUM_HASH} || exit 1 \
    && ./autogen.sh \
    && ./configure \
    && make \
    && make check \
    && make install

# Udev
ARG UDEV_VERSION=v3.2.7
ARG UDEV_HASH=4758e346a14126fc3a964de5831e411c27ebe487
RUN set -ex \
    && git clone https://github.com/gentoo/eudev -b ${UDEV_VERSION} \
    && cd eudev \
    && test `git rev-parse HEAD` = ${UDEV_HASH} || exit 1 \
    && ./autogen.sh \
    && ./configure --disable-gudev --disable-introspection --disable-hwdb --disable-manpages --disable-shared \
    && make \
    && make install

# Libusb
ARG USB_VERSION=v1.0.22
ARG USB_HASH=0034b2afdcdb1614e78edaa2a9e22d5936aeae5d
RUN set -ex \
    && git clone https://github.com/libusb/libusb.git -b ${USB_VERSION} \
    && cd libusb \
    && test `git rev-parse HEAD` = ${USB_HASH} || exit 1 \
    && ./autogen.sh \
    && ./configure --disable-shared \
    && make \
    && make install

# Hidapi
ARG HIDAPI_VERSION=hidapi-0.8.0-rc1
ARG HIDAPI_HASH=40cf516139b5b61e30d9403a48db23d8f915f52c
RUN set -ex \
    && git clone https://github.com/signal11/hidapi -b ${HIDAPI_VERSION} \
    && cd hidapi \
    && test `git rev-parse HEAD` = ${HIDAPI_HASH} || exit 1 \
    && ./bootstrap \
    && ./configure --enable-static --disable-shared \
    && make \
    && make install

# Protobuf
ARG PROTOBUF_VERSION=v3.7.0
ARG PROTOBUF_HASH=582743bf40c5d3639a70f98f183914a2c0cd0680
RUN set -ex \
    && git clone https://github.com/protocolbuffers/protobuf -b ${PROTOBUF_VERSION} \
    && cd protobuf \
    && test `git rev-parse HEAD` = ${PROTOBUF_HASH} || exit 1 \
    && git submodule update --init --recursive \
    && ./autogen.sh \
    && ./configure --enable-static --disable-shared \
    && make \
    && make install \
    && ldconfig

WORKDIR /src
COPY . .

ENV USE_SINGLE_BUILDDIR=1
ARG NPROC
RUN set -ex && \
    git submodule init && git submodule update && \
    rm -rf build && \
    if [ -z "$NPROC" ] ; \
    then make -j$(nproc) release-static ; \
    else make -j$NPROC release-static ; \
    fi

# runtime stage
FROM ubuntu:16.04

RUN set -ex && \
    apt-get update && \
    apt-get --no-install-recommends --yes install ca-certificates && \
    apt-get clean && \
    rm -rf /var/lib/apt
COPY --from=builder /src/build/release/bin /usr/local/bin/

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

ENTRYPOINT ["monerod", "--p2p-bind-ip=0.0.0.0", "--p2p-bind-port=18080", "--rpc-bind-ip=0.0.0.0", "--rpc-bind-port=18081", "--non-interactive", "--confirm-external-bind"]

