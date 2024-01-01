#!/usr/bin/env python3

# Copyright (c) 2019-2024, The Monero Project
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

from __future__ import print_function
import time

"""Test peer baning RPC calls

Test the following RPCs:
    - set_bans
    - get_bans

"""

from framework.daemon import Daemon

class BanTest():
    def run_test(self):
        print('Testing bans')

        daemon = Daemon()
        res = daemon.get_bans()
        assert 'bans' not in res or len(res.bans) == 0

        daemon.set_bans([{'host': '1.2.3.4', 'ban': True, 'seconds': 100}])
        res = daemon.get_bans()
        assert len(res.bans) == 1
        assert res.bans[0].host == '1.2.3.4'
        assert res.bans[0].seconds >= 98 and res.bans[0].seconds <= 100 # allow for slow RPC

        daemon.set_bans([{'host': '5.6.7.8', 'ban': True, 'seconds': 100}])
        res = daemon.get_bans()
        assert len(res.bans) == 2
        for i in range(2):
          assert res.bans[i].host == '1.2.3.4' or res.bans[i].host == '5.6.7.8'
          assert res.bans[i].seconds >= 7 and res.bans[0].seconds <= 100 # allow for slow RPC

        daemon.set_bans([{'host': '1.2.3.4', 'ban': False}])
        res = daemon.get_bans()
        assert len(res.bans) == 1
        assert res.bans[0].host == '5.6.7.8'
        assert res.bans[0].seconds >= 98 and res.bans[0].seconds <= 100 # allow for slow RPC

        time.sleep(2)

        res = daemon.get_bans()
        assert len(res.bans) == 1
        assert res.bans[0].host == '5.6.7.8'
        assert res.bans[0].seconds >= 96 and res.bans[0].seconds <= 98 # allow for slow RPC

        daemon.set_bans([{'host': '3.4.5.6', 'ban': False}])
        res = daemon.get_bans()
        assert len(res.bans) == 1
        assert res.bans[0].host == '5.6.7.8'
        assert res.bans[0].seconds >= 96 and res.bans[0].seconds <= 98 # allow for slow RPC

        daemon.set_bans([{'host': '3.4.5.6', 'ban': True, 'seconds': 2}])
        res = daemon.get_bans()
        assert len(res.bans) == 2
        for i in range(2):
          assert res.bans[i].host == '5.6.7.8' or res.bans[i].host == '3.4.5.6'
          if res.bans[i].host == '5.6.7.8':
            assert res.bans[i].seconds >= 96 and res.bans[0].seconds <= 98 # allow for slow RPC
          else:
            assert res.bans[i].seconds >= 1 and res.bans[0].seconds <= 2 # allow for slow RPC

        time.sleep(2)
        res = daemon.get_bans()
        assert len(res.bans) == 1
        assert res.bans[0].host == '5.6.7.8'
        assert res.bans[0].seconds >= 94 and res.bans[0].seconds <= 96 # allow for slow RPC

        daemon.set_bans([{'host': '5.6.7.8', 'ban': True, 'seconds': 20}])
        res = daemon.get_bans()
        assert len(res.bans) == 1
        assert res.bans[0].host == '5.6.7.8'
        assert res.bans[0].seconds >= 18 and res.bans[0].seconds <= 20 # allow for slow RPC

        daemon.set_bans([{'host': '5.6.7.8', 'ban': True, 'seconds': 200}])
        res = daemon.get_bans()
        assert len(res.bans) == 1
        assert res.bans[0].host == '5.6.7.8'
        assert res.bans[0].seconds >= 198 and res.bans[0].seconds <= 200 # allow for slow RPC

        daemon.set_bans([{'host': '5.6.7.8', 'ban': False}])
        res = daemon.get_bans()
        assert 'bans' not in res or len(res.bans) == 0


if __name__ == '__main__':
    BanTest().run_test()
