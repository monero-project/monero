#!/bin/bash 
set -x
export LD_LIBRARY_PATH=$HOME/.local/lib:$HOME/.local/lib64
./build/release/src/bitmonerod $@


