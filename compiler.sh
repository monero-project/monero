#!/bin/bash
# Copyright (c) 2014-2018, The Monero Project, ClockworX
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are
# permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this list of
#    conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this list
#    of conditions and the following disclaimer in the documentation and/or other
#    materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors may be
#    used to endorse or promote products derived from this software without specific
#    prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
# THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
# THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
 
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
apt-get install -y qtbase5-dev qt5-default qtdeclarative5-dev 
apt-get install -y qml-module-qtquick-controls qml-module-qtquick-xmllistmodel 
apt-get install -y qttools5-dev-tools qml-module-qtquick-dialogs 
apt-get install -y qml-module-qt-labs-settings libqt5qml-graphicaleffects
apt-get install -y qtmultimedia5-dev qml-module-qtmultimedia libzbar-dev
apt-get dist-upgrade -y


