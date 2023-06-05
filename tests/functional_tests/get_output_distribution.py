#!/usr/bin/env python3

# Copyright (c) 2019-2023, The Monero Project
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

"""Test get_output_distribution RPC
"""

from __future__ import print_function
from framework.daemon import Daemon
from framework.wallet import Wallet

import json

class GetOutputDistributionTest():
    def run_test(self):
        self.reset()
        self.create()
        self.test_get_output_distribution()
        self.test_get_rct_coinbase_output_distribution()

    def reset(self):
        print('Resetting blockchain')
        daemon = Daemon()
        res = daemon.get_height()
        daemon.pop_blocks(res.height - 1)
        daemon.flush_txpool()

    def create(self):
        self.wallet = Wallet()
        # close the wallet if any, will throw if none is loaded
        try: self.wallet.close_wallet()
        except: pass
        res = self.wallet.restore_deterministic_wallet(seed = 'velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted')

    def test_get_output_distribution(self):
        print("Test get_output_distribution")

        daemon = Daemon()

        res = daemon.get_output_distribution([0], 0, 0)
        assert len(res.distributions) == 1
        d = res.distributions[0]
        assert d.amount == 0
        assert d.base == 0
        assert d.binary == False
        assert len(d.distribution) == 1
        assert d.distribution[0] == 0

        res = daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 1)

        res = daemon.get_output_distribution([0], 0, 0)
        assert len(res.distributions) == 1
        d = res.distributions[0]
        assert d.amount == 0
        assert d.base == 0
        assert d.binary == False
        assert len(d.distribution) == 2
        assert d.distribution[0] == 0
        assert d.distribution[1] == 1

        res = daemon.pop_blocks(1)

        res = daemon.get_output_distribution([0], 0, 0)
        assert len(res.distributions) == 1
        d = res.distributions[0]
        assert d.amount == 0
        assert d.base == 0
        assert d.binary == False
        assert len(d.distribution) == 1
        assert d.distribution[0] == 0

        res = daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 3)

        res = daemon.get_output_distribution([0], 0, 0, cumulative = True)
        assert len(res.distributions) == 1
        d = res.distributions[0]
        assert d.amount == 0
        assert d.base == 0
        assert d.binary == False
        assert len(d.distribution) == 4
        assert d.distribution[0] == 0
        assert d.distribution[1] == 1
        assert d.distribution[2] == 2
        assert d.distribution[3] == 3

        # extend
        res = daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 80)

        res = daemon.get_output_distribution([0], 0, 0, cumulative = True)
        assert len(res.distributions) == 1
        d = res.distributions[0]
        assert d.amount == 0
        assert d.base == 0
        assert d.binary == False
        assert len(d.distribution) == 84
        for h in range(len(d.distribution)):
            assert d.distribution[h] == h

        # pop and replace, this will do through the "trim and extend" path
        res = daemon.pop_blocks(2)
        self.wallet.refresh()
        dst = {'address': '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 'amount': 1000000000000}
        self.wallet.transfer([dst])
        res = daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 1)
        for step in range(3): # the second will be cached, the third will also be cached, but we get it in non-cumulative mode
            res = daemon.get_output_distribution([0], 0, 0, cumulative = step < 3)
            assert len(res.distributions) == 1
            d = res.distributions[0]
            assert d.amount == 0
            assert d.base == 0
            assert d.binary == False
            assert len(d.distribution) == 83
            for h in range(len(d.distribution)):
                assert d.distribution[h] == (h if step < 3 else 1) + (2 if h == len(d.distribution) - 1 else 0)

        # start at 0, end earlier
        res = daemon.get_output_distribution([0], 0, 40, cumulative = True)
        assert len(res.distributions) == 1
        d = res.distributions[0]
        assert d.amount == 0
        assert d.base == 0
        assert d.binary == False
        assert len(d.distribution) == 41
        for h in range(len(d.distribution)):
            assert d.distribution[h] == h

        # start after 0, end earlier
        res = daemon.get_output_distribution([0], 10, 20, cumulative = True)
        assert len(res.distributions) == 1
        d = res.distributions[0]
        assert d.amount == 0
        assert d.base == 9
        assert d.binary == False
        assert len(d.distribution) == 11
        for h in range(len(d.distribution)):
            assert d.distribution[h] == 10 + h

        # straddling up
        res = daemon.get_output_distribution([0], 15, 25, cumulative = True)
        assert len(res.distributions) == 1
        d = res.distributions[0]
        assert d.amount == 0
        assert d.base == 14
        assert d.binary == False
        assert len(d.distribution) == 11
        for h in range(len(d.distribution)):
            assert d.distribution[h] == 15 + h

        # straddling down
        res = daemon.get_output_distribution([0], 8, 18, cumulative = True)
        assert len(res.distributions) == 1
        d = res.distributions[0]
        assert d.amount == 0
        assert d.base == 7
        assert d.binary == False
        assert len(d.distribution) == 11
        for h in range(len(d.distribution)):
            assert d.distribution[h] == 8 + h

        # encompassing
        res = daemon.get_output_distribution([0], 5, 20, cumulative = True)
        assert len(res.distributions) == 1
        d = res.distributions[0]
        assert d.amount == 0
        assert d.base == 4
        assert d.binary == False
        assert len(d.distribution) == 16
        for h in range(len(d.distribution)):
            assert d.distribution[h] == 5 + h

        # single
        res = daemon.get_output_distribution([0], 2, 2, cumulative = True)
        assert len(res.distributions) == 1
        d = res.distributions[0]
        assert d.amount == 0
        assert d.base == 1
        assert d.binary == False
        assert len(d.distribution) == 1
        assert d.distribution[0] == 2

        # a non existent amount
        res = daemon.get_output_distribution([1], 0, 0)
        assert len(res.distributions) == 1
        d = res.distributions[0]
        assert d.amount == 1
        assert d.base == 0
        assert d.binary == False
        assert len(d.distribution) == 83
        for h in range(len(d.distribution)):
            assert d.distribution[h] == 0

    def test_get_rct_coinbase_output_distribution(self):
        print("Test get_rct_coinbase_output_distribution")

        daemon = Daemon()

        # Check that we have some RCT blocks/transactions to test against
        chain_height = daemon.get_height()['height']
        top_header = daemon.get_block_header_by_height(chain_height - 1)
        top_major_version = top_header['block_header']['major_version']
        assert top_major_version >= 4

        # Fetch the cumulative RCT coinbase distribution
        res = daemon.get_output_distribution(get_rct_coinbase = True, cumulative = True, binary=False)
        dist_info = res['rct_coinbase_distribution']
        amount = dist_info['amount']
        begin_height = dist_info['start_height']
        base = dist_info['base']
        dist = dist_info['distribution']
        end_height = begin_height + len(dist)
        assert amount == 0
        assert begin_height < chain_height
        assert end_height == chain_height
        assert base == 0
        assert len(dist) > 0
        assert len(dist) == chain_height - begin_height # Check to_height=0 grabs whole chain after v4 fork
        assert all((x >= 0 for x in dist)) # Check all elements of dist positive
        assert all((dist[i - 1] <= dist[i] for i in range(1, len(dist)))) # Check cumulative

        print("Found {} total RCT coinbase outputs".format(dist[-1]))

        # Check that the non-cumulative distribution is valid to corresponding cumulative
        res = daemon.get_output_distribution(get_rct_coinbase = True, cumulative = False, binary=False)
        non_cum_dist = res['rct_coinbase_distribution']['distribution']
        assert len(non_cum_dist) == len(dist)
        assert dist[0] == non_cum_dist[0]
        assert all(((dist[i] - dist[i - 1]) == non_cum_dist[i] for i in range(1, len(dist))))

        # Scan the whole chain from begin_height and check values of RCT coinbase distribution
        # for each block.
        for h in range(begin_height, chain_height):
            dist_index = h - begin_height
            res = daemon.get_block(height = h)
            block_body = json.loads(res['json'])
            miner_tx = block_body['miner_tx']
            is_rct_miner = miner_tx['version'] >= 2
            if not is_rct_miner:
                assert non_cum_dist[dist_index] == 0
            else:
                assert non_cum_dist[dist_index] == len(miner_tx['vout'])

class Guard:
    def __enter__(self):
        for i in range(4):
            Wallet(idx = i).auto_refresh(False)
    def __exit__(self, exc_type, exc_value, traceback):
        for i in range(4):
            Wallet(idx = i).auto_refresh(True)

if __name__ == '__main__':
    with Guard() as guard:
        GetOutputDistributionTest().run_test()
