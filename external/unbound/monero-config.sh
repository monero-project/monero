#!/bin/bash

./configure --prefix=${CMAKE_FIND_ROOT_PATH} --build=${GCC_PREFIX} --host=${GCC_PREFIX} --disable-shared --enable-static --sysconfdir=${CMAKE_FIND_ROOT_PATH}/etc --localstatedir=${CMAKE_FIND_ROOT_PATH}/var --sbindir=${CMAKE_FIND_ROOT_PATH}/bin --disable-gost --disable-rpath --with-libevent=no --with-libexpat=${CMAKE_FIND_ROOT_PATH} --without-pyunbound --without-pythonmodule --with-ssl=${CMAKE_FIND_ROOT_PATH} --without-pthreads --with-libunbound-only
