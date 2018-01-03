#!/bin/bash
# Ubuntu basic setup file as of January 2018
# Ubuntu Server 16.04.3 LTS
# Run as sudo

apt-get update
apt-get install -y build-essential cmake pkg-config 
apt-get install -y libboost-all-dev libssl-dev libzmq3-dev 
apt-get install -y libunbound-dev libsodium-dev libminiupnpc-dev 
apt-get install -y libunwind8-dev liblzma-dev libreadline6-dev
apt-get install -y libldns-dev libexpat1-dev libgtest-dev 
apt-get install -y doxygen graphviz
apt-get dist-upgrade

