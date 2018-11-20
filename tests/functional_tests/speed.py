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

"""Test speed of various procedures

Test the following RPCs:
    - generateblocks
    - transfer
    - [TODO: many tests still need to be written]

"""


import time
from time import sleep
from decimal import Decimal

from test_framework.daemon import Daemon
from test_framework.wallet import Wallet


class SpeedTest():
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        daemon = Daemon()
        wallet = Wallet()

        destinations = wallet.make_uniform_destinations('44AFFq5kSiGBoZ4NMDwYtN18obc8AemS33DBLWs3H7otXft3XjrpDtQGv7SqSsaBYBb98uNbr2VBBEt7f2wfn3RVGQBEP3A',1,3)

        self._test_speed_generateblocks(daemon=daemon, blocks=70)
        for i in range(1, 10):
            while wallet.get_balance()['unlocked_balance'] == 0:
                print('Waiting for wallet to refresh...')
                sleep(1)
            self._test_speed_transfer_split(wallet=wallet)
        self._test_speed_generateblocks(daemon=daemon, blocks=10)

    def _test_speed_generateblocks(self, daemon, blocks):
        print('Test speed of block generation')
        start = time.time()

        res = daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', blocks)
            # wallet seed: velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted

        print('generating ', blocks, 'blocks took: ', time.time() - start, 'seconds')

    def _test_speed_transfer_split(self, wallet):
        print('Test speed of transfer')
        start = time.time()

        destinations = wallet.make_uniform_destinations('44AFFq5kSiGBoZ4NMDwYtN18obc8AemS33DBLWs3H7otXft3XjrpDtQGv7SqSsaBYBb98uNbr2VBBEt7f2wfn3RVGQBEP3A',1)
        res = wallet.transfer_split(destinations)

        print('generating tx took: ', time.time() - start, 'seconds')


if __name__ == '__main__':
    SpeedTest().run_test()
