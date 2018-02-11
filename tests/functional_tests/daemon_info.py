#!/usr/bin/env python3

# Copyright (c) 2018 The Monero Project
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

"""Test daemon RPC calls

Test the following RPCs:
    - get_info
    - hard_fork_info

"""

from __future__ import print_function
import os
from framework.daemon import Daemon

class DaemonGetInfoTest():
    def run_test(self):
        self._test_hardfork_info()
        self._test_get_info()

    def _test_hardfork_info(self):
        print('Test hard_fork_info')

        daemon = Daemon()
        res = daemon.hard_fork_info()

        # hard_fork version should be set at height 1
        assert 'earliest_height' in res.keys()
        #assert res['earliest_height'] == 1;
        assert res.earliest_height == 1

    def _test_get_info(self):
        print('Test get_info')

        daemon = Daemon()
        res = daemon.get_info()

        # difficulty should be set to 1 for this test
        assert 'difficulty' in res.keys()
        assert res.difficulty == int(os.environ['DIFFICULTY'])

        # nettype should not be TESTNET
        assert 'testnet' in res.keys()
        assert res.testnet == False;

        # nettype should not be STAGENET
        assert 'stagenet' in res.keys()
        assert res.stagenet == False;

        # nettype should be FAKECHAIN
        assert 'nettype' in res.keys()
        assert res.nettype == "fakechain";

        # free_space should be > 0
        assert 'free_space' in res.keys()
        assert res.free_space > 0

        # height should be greater or equal to 1
        assert 'height' in res.keys()
        assert res.height >= 1


if __name__ == '__main__':
    DaemonGetInfoTest().run_test()
