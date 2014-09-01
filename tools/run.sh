#!/bin/bash 
set -x
./build/src/bitmonerod --p2p-bind-port 15080 --rpc-bind-port 15081   --add-exclusive-node   192.168.0.66:18080       $@ 
#1./build/src/bitmonerod --p2p-bind-port 15080 --rpc-bind-port 15081   --add-exclusive-node   192.168.0.66:17080         --add-exclusive-node   192.168.0.57:18080    --add-exclusive-node   192.168.0.66:18080       --add-exclusive-node   192.168.0.51:18080 --add-exclusive-node   192.168.0.60:18080   $@ 

