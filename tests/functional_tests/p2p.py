#!/usr/bin/env python3

# Copyright (c) 2018-2023, The Monero Project

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

"""Test daemon P2P
"""

from framework.daemon import Daemon
from framework.wallet import Wallet

class P2PTest():
    def run_test(self):
        self.reset()
        self.create()
        self.mine(80)
        self.test_p2p_reorg()
        txid = self.test_p2p_tx_propagation()
        self.test_p2p_block_propagation(txid)

    def reset(self):
        print('Resetting blockchain')
        daemon = Daemon()
        res = daemon.get_height()
        daemon.pop_blocks(res.height - 1)
        daemon.flush_txpool()

    def create(self):
        print('Creating wallet')
        seed = 'velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted'
        self.wallet = Wallet(idx = 4)
        # close the wallet if any, will throw if none is loaded
        try: self.wallet.close_wallet()
        except: pass
        res = self.wallet.restore_deterministic_wallet(seed = seed)

    def mine(self, blocks):
        assert blocks >= 1

        print("Generating", blocks, 'blocks')

        daemon = Daemon(idx = 2)

        # generate blocks
        res_generateblocks = daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', blocks)

    def test_p2p_reorg(self):
        print('Testing P2P reorg')
        daemon2 = Daemon(idx = 2)
        daemon3 = Daemon(idx = 3)

        # give sync some time
        time.sleep(1)

        res = daemon2.get_info()
        height = res.height
        assert height > 0
        top_block_hash = res.top_block_hash
        assert len(top_block_hash) == 64

        res = daemon3.get_info()
        assert res.height == height
        assert res.top_block_hash == top_block_hash

        # disconnect daemons and mine separately on both
        daemon2.out_peers(0)
        daemon3.out_peers(0)

        res = daemon2.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 2)
        res = daemon3.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 3)

        res = daemon2.get_info()
        assert res.height == height + 2
        daemon2_top_block_hash = res.top_block_hash
        assert daemon2_top_block_hash != top_block_hash
        res = daemon3.get_info()
        assert res.height == height + 3
        daemon3_top_block_hash = res.top_block_hash
        assert daemon3_top_block_hash != top_block_hash
        assert daemon3_top_block_hash != daemon2_top_block_hash

        # reconnect, daemon2 will now switch to daemon3's chain
        daemon2.out_peers(8)
        daemon3.out_peers(8)
        time.sleep(10)
        res = daemon2.get_info()
        assert res.height == height + 3
        assert res.top_block_hash == daemon3_top_block_hash

        # disconect, mine on daemon2 again more than daemon3
        daemon2.out_peers(0)
        daemon3.out_peers(0)

        res = daemon2.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 3)
        res = daemon3.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 2)

        res = daemon2.get_info()
        assert res.height == height + 6
        daemon2_top_block_hash = res.top_block_hash
        assert daemon2_top_block_hash != top_block_hash
        res = daemon3.get_info()
        assert res.height == height + 5
        daemon3_top_block_hash = res.top_block_hash
        assert daemon3_top_block_hash != top_block_hash
        assert daemon3_top_block_hash != daemon2_top_block_hash

        # reconnect, daemon3 will now switch to daemon2's chain
        daemon2.out_peers(8)
        daemon3.out_peers(8)
        time.sleep(5)
        res = daemon3.get_info()
        assert res.height == height + 6
        assert res.top_block_hash == daemon2_top_block_hash

        # disconnect and mine a lot on daemon3
        daemon2.out_peers(0)
        daemon3.out_peers(0)
        res = daemon3.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 500)

        # reconnect and wait for sync
        daemon2.out_peers(8)
        daemon3.out_peers(8)
        loops = 100
        while True:
            res2 = daemon2.get_info()
            res3 = daemon3.get_info()
            if res2.top_block_hash == res3.top_block_hash:
                break
            time.sleep(10)
            loops -= 1
            assert loops >= 0

        # create a transaction which both daemons have in their pool
        dst = {'address': '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 'amount': 1000000000000}
        res = self.wallet.transfer([dst])
        txid = res.tx_hash
        time.sleep(5)
        for daemon in [daemon2, daemon3]:
            res = daemon.get_transaction_pool_hashes()
            assert len(res.tx_hashes) == 1
            assert res.tx_hashes[0] == txid

        # disconnect nodes and mine on daemon2, mining alt blocks containing that mempool tx
        daemon2.out_peers(0)
        daemon3.out_peers(0)
        daemon2.pop_blocks(2)
        daemon2.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 5)

        # assert that the tx is in daemon2's blockchain now, instead of the mempool
        res = daemon2.get_transactions([txid])
        assert len(res.txs) == 1
        tx_details = res.txs[0]
        assert not tx_details.in_pool
        daemon2_tx_height = tx_details.block_height

        # reconnect, daemon3 will now switch to daemon2's chain
        daemon2.out_peers(8)
        daemon3.out_peers(8)
        time.sleep(5)
        res = daemon2.get_info()
        daemon2_top_block_hash = res.height
        daemon2_height = res.top_block_hash
        res = daemon3.get_info()
        daemon3_top_block_hash = res.height
        daemon3_height = res.top_block_hash
        assert daemon2_height == daemon3_height
        assert daemon2_top_block_hash == daemon3_top_block_hash

        # assert that the tx is in daemon3's blockchain now
        res = daemon3.get_transactions([txid])
        assert len(res.txs) == 1
        tx_details = res.txs[0]
        assert not tx_details.in_pool
        assert tx_details.block_height == daemon2_tx_height

    def test_p2p_tx_propagation(self):
        print('Testing P2P tx propagation')
        daemon2 = Daemon(idx = 2)
        daemon3 = Daemon(idx = 3)

        for daemon in [daemon2, daemon3]:
            res = daemon.get_transaction_pool_hashes()
            assert not 'tx_hashes' in res or len(res.tx_hashes) == 0

        self.wallet.refresh()
        res = self.wallet.get_balance()

        dst = {'address': '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 'amount': 1000000000000}
        res = self.wallet.transfer([dst])
        assert len(res.tx_hash) == 32*2
        txid = res.tx_hash

        time.sleep(5)

        for daemon in [daemon2, daemon3]:
            res = daemon.get_transaction_pool_hashes()
            assert len(res.tx_hashes) == 1
            assert res.tx_hashes[0] == txid

        return txid

    def test_p2p_block_propagation(self, daemon2_mempool_txid):
        print('Testing P2P block propagation')
        daemon2 = Daemon(idx = 2)
        daemon3 = Daemon(idx = 3)

        # check precondition: txid in daemon2 mempool
        res = daemon2.get_transaction_pool_hashes()
        assert daemon2_mempool_txid in res.tx_hashes

        res = daemon2.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 1)
        block_height = res.height

        # wait until both are synced
        loops = 5
        while True:
            res2 = daemon2.get_info()
            res3 = daemon3.get_info()
            if res2.top_block_hash == res3.top_block_hash:
                break
            time.sleep(5)
            loops -= 1
            assert loops >= 0

        # check the tx is in both daemons
        for daemon in [daemon2, daemon3]:
            res = daemon.get_transaction_pool_hashes()
            assert not 'tx_hashes' in res or len(res.tx_hashes) == 0

            res = daemon.get_transactions([daemon2_mempool_txid])
            assert len(res.txs) == 1
            tx_details = res.txs[0]
            assert not tx_details.in_pool
            assert tx_details.block_height == block_height


if __name__ == '__main__':
    P2PTest().run_test()
