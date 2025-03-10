#!/usr/bin/env python3

# Copyright (c) 2018-2022, The Monero Project

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
        self.test_p2p_block_propagation_shared(txid)
        txid = self.test_p2p_tx_propagation()
        self.test_p2p_block_propagation_new(txid)

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

    def test_p2p_tx_propagation(self):
        print('Testing P2P tx propagation')
        daemons = (Daemon(idx=2), Daemon(idx=3))

        for daemon in daemons:
            res = daemon.get_transaction_pool_hashes()
            assert len(res.get('tx_hashes', [])) == 0

        self.wallet.refresh()
        res = self.wallet.get_balance()

        dst = {'address': '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 'amount': 1000000000000}
        res = self.wallet.transfer([dst])
        assert len(res.tx_hash) == 32*2
        txid = res.tx_hash

        # Due to Dandelion++, the network propagates transactions with a
        # random delay, so poll for the transaction with a timeout. The delay
        # should almost never exceed a maximum of 13s (~1/billion samples).
        # Set the timeout slightly higher than the maximum delay to account
        # for transmission and processing time.
        timeout = 13.5
        pending_daemons = set(daemons)
        expected_hashes = [txid]
        wait_cutoff = time.monotonic() + timeout
        while True:
            done = []
            for daemon in pending_daemons:
                res = daemon.get_transaction_pool_hashes()
                hashes = res.get('tx_hashes')
                if hashes:
                    assert hashes == expected_hashes
                    done.append(daemon)
            pending_daemons.difference_update(done)
            if len(pending_daemons) == 0:
                break
            max_delay = wait_cutoff - time.monotonic()
            if max_delay <= 0:
                break
            time.sleep(min(.2, max_delay))
        npending = len(pending_daemons)
        assert npending == 0, '%d daemons pending' % npending

        return txid

    def test_p2p_block_propagation_shared(self, mempool_txid):
        print('Testing P2P block propagation with shared TX')
        daemon2 = Daemon(idx = 2)
        daemon3 = Daemon(idx = 3)

        # check precondition: txid in daemon2's and daemon3's mempool
        res = daemon2.get_transaction_pool_hashes()
        assert mempool_txid in res.get('tx_hashes', [])

        res = daemon3.get_transaction_pool_hashes()
        assert mempool_txid in res.get('tx_hashes', [])

        # mine block on daemon2
        res = daemon2.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 1)
        block_height = res.height

        # wait until both are synced, or 5 seconds, whichever is first.
        # the timeout time should be very, very low here since block propagation, unlike tx propagation, is designed
        # to be as fast as possible.
        # and since both daemons already have the tx in their mempools, a notification of a new fluffy block, which is
        # pushed out immediately upon being mined, should result in an instant addition of that block to the chain,
        # without any round trips.
        # this test should only fail if you're running it on a potato where PoW verification + check_tx_inputs() takes
        # more than 5 second.
        deadline = time.monotonic() + 5
        result = None
        while result is None:
            res2 = daemon2.get_info()
            res3 = daemon3.get_info()
            if res2.top_block_hash == res3.top_block_hash:
                result = True
            elif time.monotonic() > deadline:
                result = False
            else:
                time.sleep(.25)
        assert result, "Shared tx block propagation timed out"

        # check the tx is moved to both daemons's blockchains at top block
        for daemon in [daemon2, daemon3]:
            res = daemon.get_transaction_pool_hashes()
            assert not 'tx_hashes' in res or len(res.tx_hashes) == 0

            res = daemon.get_transactions([mempool_txid])
            assert len(res.get('txs', [])) == 1
            tx_details = res.txs[0]
            assert ('in_pool' not in tx_details) or (not tx_details.in_pool)
            assert tx_details.block_height == block_height

    def test_p2p_block_propagation_new(self, mempool_txid):
        # there's a big problem with this testcase in that there's not yet a way to prevent daemon's from syncing
        # mempools only, but still allow block propagation. so there's a good chance that the transaction will be synced
        # between daemons between when daemon2's mempool is flushed and when daemon3 mines a new block. in this
        # scenario, this testcase basically just degenerates into test_p2p_block_propagation_shared(). however, if this
        # one ever fails but test_p2p_block_propagation_shared() passes, then we might have actually caught a problem
        # with block propagation when one of the daemons is missing a tx(s)

        print('Testing P2P block propagation with (possibly) new TX')
        daemon2 = Daemon(idx = 2)
        daemon3 = Daemon(idx = 3)

        # check precondition: txid in daemon3's mempool
        res = daemon3.get_transaction_pool_hashes()
        assert mempool_txid in res.get('tx_hashes', [])

        # flush daemon2 mempool
        daemon2.flush_txpool()

        # mine block on daemon3
        res = daemon3.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 1)
        block_height = res.height

        # wait until both are synced, or 5 seconds, whichever is first.
        # the timeout time should be very, very low here since block propagation, unlike tx propagation, is designed
        # to be as fast as possible. however, it might have to be raised if the daemon actually does make a round trip
        # to request a missing tx in a fluffy block
        deadline = time.monotonic() + 5
        result = None
        while result is None:
            res2 = daemon2.get_info()
            res3 = daemon3.get_info()
            if res2.top_block_hash == res3.top_block_hash:
                result = True
            elif time.monotonic() > deadline:
                result = False
            else:
                time.sleep(.25)
        assert result, "New tx block propagation timed out"

        # check the tx is moved to both daemons's blockchains at top block
        for daemon in [daemon2, daemon3]:
            res = daemon.get_transaction_pool_hashes()
            assert not 'tx_hashes' in res or len(res.tx_hashes) == 0

            res = daemon.get_transactions([mempool_txid])
            assert len(res.get('txs', [])) == 1
            tx_details = res.txs[0]
            assert ('in_pool' not in tx_details) or (not tx_details.in_pool)
            assert tx_details.block_height == block_height


if __name__ == '__main__':
    P2PTest().run_test()
