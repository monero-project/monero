FROM debian:jessie

RUN apt-get update && apt-get install -y unzip automake build-essential curl file pkg-config git python libtool

WORKDIR /opt/android
## INSTALL ANDROID SDK
RUN curl -s -O http://dl.google.com/android/android-sdk_r24.4.1-linux.tgz \
    && tar --no-same-owner -xzf android-sdk_r24.4.1-linux.tgz \
    && rm -f android-sdk_r24.4.1-linux.tgz

## INSTALL ANDROID NDK
ENV ANDROID_NDK_REVISION 14
RUN curl -s -O https://dl.google.com/android/repository/android-ndk-r${ANDROID_NDK_REVISION}-linux-x86_64.zip \
    && unzip android-ndk-r${ANDROID_NDK_REVISION}-linux-x86_64.zip \
    && rm -f android-ndk-r${ANDROID_NDK_REVISION}-linux-x86_64.zip

ENV WORKDIR /opt/android
ENV ANDROID_SDK_ROOT ${WORKDIR}/android-sdk-linux
ENV ANDROID_NDK_ROOT ${WORKDIR}/android-ndk-r${ANDROID_NDK_REVISION}

## INSTALL BOOST
ENV BOOST_VERSION 1_62_0
ENV BOOST_VERSION_DOT 1.62.0
RUN curl -s -L -o  boost_${BOOST_VERSION}.tar.bz2 https://sourceforge.net/projects/boost/files/boost/${BOOST_VERSION_DOT}/boost_${BOOST_VERSION}.tar.bz2/download \
    && tar -xvf boost_${BOOST_VERSION}.tar.bz2 \
    && rm -f /usr/boost_${BOOST_VERSION}.tar.bz2 \
    && cd boost_${BOOST_VERSION} \
    && ./bootstrap.sh

ENV TOOLCHAIN_DIR ${WORKDIR}/toolchain-arm
RUN ${ANDROID_NDK_ROOT}/build/tools/make_standalone_toolchain.py \
         --arch arm \
         --api 21 \
         --install-dir $TOOLCHAIN_DIR \
         --stl=libc++
ENV PATH $TOOLCHAIN_DIR/arm-linux-androideabi/bin:$TOOLCHAIN_DIR/bin:$PATH

## Build BOOST
RUN cd boost_${BOOST_VERSION} \
    && ./b2 --build-type=minimal link=static runtime-link=static --with-chrono --with-date_time --with-filesystem --with-program_options --with-regex --with-serialization --with-system --with-thread --build-dir=android32 --stagedir=android32 toolset=clang threading=multi threadapi=pthread target-os=android stage

#INSTALL cmake (avoid 3.7 : https://github.com/android-ndk/ndk/issues/254)
ENV CMAKE_VERSION 3.6.3
RUN cd /usr \
    && curl -s -O https://cmake.org/files/v3.6/cmake-${CMAKE_VERSION}-Linux-x86_64.tar.gz \
    && tar -xzf /usr/cmake-${CMAKE_VERSION}-Linux-x86_64.tar.gz \
    && rm -f /usr/cmake-${CMAKE_VERSION}-Linux-x86_64.tar.gz
ENV PATH /usr/cmake-${CMAKE_VERSION}-Linux-x86_64/bin:$PATH

#Note : we build openssl because the default lacks DSA1

# download, configure and make Zlib
ENV ZLIB_VERSION 1.2.11
RUN curl -s -O http://zlib.net/zlib-${ZLIB_VERSION}.tar.gz \
    && tar -xzf zlib-${ZLIB_VERSION}.tar.gz \
    && rm zlib-${ZLIB_VERSION}.tar.gz \
    && mv zlib-${ZLIB_VERSION} zlib \
    && cd zlib && CC=clang CXX=clang++ ./configure --static \
    && make
# open ssl
ENV OPENSSL_VERSION 1.0.2j
RUN curl -s -O https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz \
    && tar -xzf openssl-${OPENSSL_VERSION}.tar.gz \
    && rm openssl-${OPENSSL_VERSION}.tar.gz \
    && cd openssl-${OPENSSL_VERSION} \
    && sed -i -e "s/mandroid/target\ armv7\-none\-linux\-androideabi/" Configure \
    && CC=clang CXX=clang++ \
           ./Configure android-armv7 \
           no-asm \
           no-shared --static \
           --with-zlib-include=${WORKDIR}/zlib/include --with-zlib-lib=${WORKDIR}/zlib/lib \
    && make build_crypto build_ssl \
    && cd .. && mv openssl-${OPENSSL_VERSION}  openssl

# ZMQ
RUN git clone https://github.com/zeromq/zeromq4-1.git \
    && git clone https://github.com/zeromq/cppzmq.git \
    && cd zeromq4-1 \
    && ./autogen.sh \
    && CC=clang CXX=clang++ ./configure --host=arm-none-linux-gnueabi \
    && make

RUN ln -s /opt/android/openssl/libcrypto.a /opt/android/openssl/libssl.a ${TOOLCHAIN_DIR}/arm-linux-androideabi/lib/armv7-a

RUN git clone https://github.com/monero-project/monero.git \
    && cd monero \
    && mkdir -p build/release \
    && CC=clang CXX=clang++ \
         BOOST_ROOT=${WORKDIR}/boost_${BOOST_VERSION} BOOST_LIBRARYDIR=${WORKDIR}/boost_${BOOST_VERSION}/android32/lib/ \
         OPENSSL_ROOT_DIR=${WORKDIR}/openssl/ \
         CMAKE_INCLUDE_PATH=${WORKDIR}/cppzmq/ \
         CMAKE_LIBRARY_PATH=${WORKDIR}/zeromq4-1/.libs \
         CXXFLAGS="-I ${WORKDIR}/zeromq4-1/include/" \
         make release-static-android
