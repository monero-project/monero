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

from __future__ import print_function
import time

"""Test daemon mining RPC calls

Test the following RPCs:
    - start_mining
    - stop_mining
    - mining_status
"""

from framework.daemon import Daemon
from framework.wallet import Wallet

class MiningTest():
    def run_test(self):
        self.reset()
        self.create()
        self.mine()

    def reset(self):
        print('Resetting blockchain')
        daemon = Daemon()
        daemon.pop_blocks(1000)
        daemon.flush_txpool()

    def create(self):
        print('Creating wallet')
        wallet = Wallet()
        # close the wallet if any, will throw if none is loaded
        try: wallet.close_wallet()
        except: pass
        res = wallet.restore_deterministic_wallet(seed = 'velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted')

    def mine(self):
        print("Test mining")

        daemon = Daemon()
        wallet = Wallet()

        # check info/height/balance before generating blocks
        res_info = daemon.get_info()
        prev_height = res_info.height
        res_getbalance = wallet.get_balance()
        prev_balance = res_getbalance.balance

        res_status = daemon.mining_status()

        res = daemon.start_mining('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', threads_count = 1)

        res_status = daemon.mining_status()
        assert res_status.active == True
        assert res_status.threads_count == 1
        assert res_status.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
        assert res_status.is_background_mining_enabled == False
        assert res_status.block_reward >= 600000000000

        # wait till we mined a few of them
        timeout = 5
        timeout_height = prev_height
        while True:
            time.sleep(1)
            res_info = daemon.get_info()
            height = res_info.height
            if height >= prev_height + 5:
                break
            if height > timeout_height:
              timeout = 5
              timeout_height = height
            else:
              timeout -= 1
            assert timeout >= 0

        res = daemon.stop_mining()
        res_status = daemon.mining_status()
        assert res_status.active == False

        res_info = daemon.get_info()
        new_height = res_info.height

        wallet.refresh()
        res_getbalance = wallet.get_balance()
        balance = res_getbalance.balance
        assert balance >= prev_balance + (new_height - prev_height) * 600000000000

        res = daemon.start_mining('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', threads_count = 1, do_background_mining = True)
        res_status = daemon.mining_status()
        assert res_status.active == True
        assert res_status.threads_count == 1
        assert res_status.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
        assert res_status.is_background_mining_enabled == True
        assert res_status.block_reward >= 600000000000

        # don't wait, might be a while if the machine is busy, which it probably is
        res = daemon.stop_mining()
        res_status = daemon.mining_status()
        assert res_status.active == False


if __name__ == '__main__':
    MiningTest().run_test()
