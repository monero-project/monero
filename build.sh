#!/bin/bash

mkdir -p build
cd build && ccmake ..
make $@
