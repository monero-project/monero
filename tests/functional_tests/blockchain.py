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

"""Test daemon blockchain RPC calls

Test the following RPCs:
    - get_info
    - generateblocks
    - misc block retrieval
    - pop_blocks
    - [TODO: many tests still need to be written]

"""

from framework.daemon import Daemon

class BlockchainTest():
    def run_test(self):
        self.reset()
        self._test_generateblocks(5)
        self._test_alt_chains()

    def reset(self):
        print('Resetting blockchain')
        daemon = Daemon()
        res = daemon.get_height()
        daemon.pop_blocks(res.height - 1)
        daemon.flush_txpool()

    def _test_generateblocks(self, blocks):
        assert blocks >= 2

        print("Test generating", blocks, 'blocks')

        daemon = Daemon()

        # check info/height before generating blocks
        res_info = daemon.get_info()
        height = res_info.height
        prev_block = res_info.top_block_hash
        res_height = daemon.get_height()
        assert res_height.height == height
        assert int(res_info.wide_cumulative_difficulty, 16) == (res_info.cumulative_difficulty_top64 << 64) + res_info.cumulative_difficulty
        cumulative_difficulty = int(res_info.wide_cumulative_difficulty, 16)

        # we should not see a block at height
        ok = False
        try: daemon.getblock(height = height)
        except: ok = True
        assert ok

        res = daemon.get_fee_estimate()
        assert res.fee == 234562
        assert res.quantization_mask == 10000
        res = daemon.get_fee_estimate(10)
        assert res.fee <= 234562

        # generate blocks
        res_generateblocks = daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', blocks)

        # check info/height after generateblocks blocks
        assert res_generateblocks.height == height + blocks - 1
        res_info = daemon.get_info()
        assert res_info.height == height + blocks
        assert res_info.top_block_hash != prev_block
        res_height = daemon.get_height()
        assert res_height.height == height + blocks

        # get the blocks, check they have the right height
        res_getblock = []
        for n in range(blocks):
            res_getblock.append(daemon.getblock(height = height + n))
            block_header = res_getblock[n].block_header
            assert abs(block_header.timestamp - time.time()) < 60 # within 60 seconds
            assert block_header.height == height + n
            assert block_header.orphan_status == False
            assert block_header.depth == blocks - n - 1
            assert block_header.prev_hash == prev_block, prev_block
            assert int(block_header.wide_difficulty, 16) == (block_header.difficulty_top64 << 64) + block_header.difficulty
            assert int(block_header.wide_cumulative_difficulty, 16) == (block_header.cumulative_difficulty_top64 << 64) + block_header.cumulative_difficulty
            assert block_header.reward >= 600000000000 # tail emission
            cumulative_difficulty += int(block_header.wide_difficulty, 16)
            assert cumulative_difficulty == int(block_header.wide_cumulative_difficulty, 16)
            assert block_header.block_size > 0
            assert block_header.block_weight >= block_header.block_size
            assert block_header.long_term_weight > 0
            prev_block = block_header.hash

        # we should not see a block after that
        ok = False
        try: daemon.getblock(height = height + blocks)
        except: ok = True
        assert ok

        # getlastblockheader and by height/hash should return the same block
        res_getlastblockheader = daemon.getlastblockheader()
        assert res_getlastblockheader.block_header == block_header
        res_getblockheaderbyhash = daemon.getblockheaderbyhash(prev_block)
        assert res_getblockheaderbyhash.block_header == block_header
        res_getblockheaderbyheight = daemon.getblockheaderbyheight(height + blocks - 1)
        assert res_getblockheaderbyheight.block_header == block_header

        # getting a block template after that should have the right height, etc
        res_getblocktemplate = daemon.getblocktemplate('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm')
        assert res_getblocktemplate.height == height + blocks
        assert res_getblocktemplate.reserved_offset > 0
        assert res_getblocktemplate.prev_hash == res_info.top_block_hash
        assert res_getblocktemplate.expected_reward >= 600000000000
        assert len(res_getblocktemplate.blocktemplate_blob) > 0
        assert len(res_getblocktemplate.blockhashing_blob) > 0
        assert int(res_getblocktemplate.wide_difficulty, 16) == (res_getblocktemplate.difficulty_top64 << 64) + res_getblocktemplate.difficulty

        # diff etc should be the same
        assert res_getblocktemplate.prev_hash == res_info.top_block_hash

        res_getlastblockheader = daemon.getlastblockheader()

        # pop a block
        res_popblocks = daemon.pop_blocks(1)
        assert res_popblocks.height == height + blocks - 1

        res_info = daemon.get_info()
        assert res_info.height == height + blocks - 1

        # getlastblockheader and by height/hash should return the previous block
        block_header = res_getblock[blocks - 2].block_header
        block_header.depth = 0   # this will be different, ignore it
        res_getlastblockheader = daemon.getlastblockheader()
        assert res_getlastblockheader.block_header == block_header
        res_getblockheaderbyhash = daemon.getblockheaderbyhash(block_header.hash)
        assert res_getblockheaderbyhash.block_header == block_header
        res_getblockheaderbyheight = daemon.getblockheaderbyheight(height + blocks - 2)
        assert res_getblockheaderbyheight.block_header == block_header

        # we should not see the popped block anymore
        ok = False
        try: daemon.getblock(height = height + blocks - 1)
        except: ok = True
        assert ok

        # get transactions
        res = daemon.get_info()
        assert res.height == height + blocks - 1
        nblocks = height + blocks - 1
        res = daemon.getblockheadersrange(0, nblocks - 1)
        assert len(res.headers) == nblocks
        assert res.headers[-1] == block_header
        txids = [x.miner_tx_hash for x in res.headers]
        res = daemon.get_transactions(txs_hashes = txids)
        assert len(res.txs) == nblocks
        assert not 'missed_txs' in res or len(res.missed_txs) == 0
        running_output_index = 0
        for i in range(len(txids)):
            tx = res.txs[i]
            assert tx.tx_hash == txids[i]
            assert not tx.double_spend_seen
            assert not tx.in_pool
            assert tx.block_height == i
            if i > 0:
                for idx in tx.output_indices:
                    assert idx == running_output_index
                    running_output_index += 1
                res_out = daemon.get_outs([{'amount': 0, 'index': idx} for idx in tx.output_indices], get_txid = True)
                assert len(res_out.outs) == len(tx.output_indices)
                for out in res_out.outs:
                    assert len(out.key) == 64
                    assert len(out.mask) == 64
                    assert not out.unlocked
                    assert out.height == i
                    assert out.txid == txids[i]

        for i in range(height + nblocks - 1):
            res_sum = daemon.get_coinbase_tx_sum(i, 1)
            res_header = daemon.getblockheaderbyheight(i)
            assert res_sum.emission_amount == res_header.block_header.reward
            assert res_sum.emission_amount_top64 == 0
            assert res_sum.emission_amount == int(res_sum.wide_emission_amount, 16)
            assert res_sum.fee_amount == int(res_sum.wide_fee_amount, 16)

        res = daemon.get_coinbase_tx_sum(0, 1)
        assert res.emission_amount == 17592186044415
        assert res.emission_amount_top64 == 0
        assert res.fee_amount == 0
        assert res.fee_amount_top64 == 0
        sum_blocks = height + nblocks - 1
        res = daemon.get_coinbase_tx_sum(0, sum_blocks)
        extrapolated = 17592186044415 + 17592186044415 * 2 * (sum_blocks - 1)
        assert res.emission_amount < extrapolated and res.emission_amount > extrapolated - 1e12
        assert res.fee_amount == 0
        sum_blocks_emission = res.emission_amount
        res = daemon.get_coinbase_tx_sum(1, sum_blocks)
        assert res.emission_amount == sum_blocks_emission - 17592186044415
        assert res.fee_amount == 0

        res = daemon.get_output_distribution([0, 1, 17592186044415], 0, 0)
        assert len(res.distributions) == 3
        for a in range(3):
            assert res.distributions[a].amount == [0, 1, 17592186044415][a]
            assert res.distributions[a].start_height == 0
            assert res.distributions[a].base == 0
            assert len(res.distributions[a].distribution) == height + nblocks - 1
            assert res.distributions[a].binary == False
            for i in range(height + nblocks - 1):
                assert res.distributions[a].distribution[i] == (1 if i > 0 and a == 0 else 1 if a == 2 and i == 0 else 0)

        res = daemon.get_output_histogram([], min_count = 0, max_count = 0)
        assert len(res.histogram) == 2
        for i in range(2):
            assert res.histogram[i].amount in [0, 17592186044415]
            assert res.histogram[i].total_instances in [height + nblocks - 2, 1]
            assert res.histogram[i].unlocked_instances == 0
            assert res.histogram[i].recent_instances == 0

        res = daemon.get_fee_estimate()
        assert res.fee == 234560
        assert res.quantization_mask == 10000
        res = daemon.get_fee_estimate(10)
        assert res.fee <= 234560

    def _test_alt_chains(self):
        print('Testing alt chains')
        daemon = Daemon()
        res = daemon.get_alt_blocks_hashes()
        starting_alt_blocks = res.blks_hashes if 'blks_hashes' in res else []
        res = daemon.get_info()
        root_block_hash = res.top_block_hash
        height = res.height
        prev_hash = res.top_block_hash
        res_template = daemon.getblocktemplate('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm')
        nonce = 0

        # 5 siblings
        alt_blocks = [None] * 5
        for i in range(len(alt_blocks)):
            res = daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 1, prev_block = prev_hash, starting_nonce = nonce)
            assert res.height == height
            assert len(res.blocks) == 1
            txid = res.blocks[0]
            res = daemon.getblockheaderbyhash(txid)
            nonce = res.block_header.nonce
            print('mined ' + ('alt' if res.block_header.orphan_status else 'tip') + ' block ' + str(height) + ', nonce ' + str(nonce))
            assert res.block_header.prev_hash == prev_hash
            assert res.block_header.orphan_status == (i > 0)
            alt_blocks[i] = txid
            nonce += 1

        print('mining 3 on 1')
        # three more on [1]
        chain1 = []
        res = daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 3, prev_block = alt_blocks[1], starting_nonce = nonce)
        assert res.height == height + 3
        assert len(res.blocks) == 3
        blk_hash = res.blocks[2]
        res = daemon.getblockheaderbyhash(blk_hash)
        nonce = res.block_header.nonce
        assert not res.block_header.orphan_status
        nonce += 1
        chain1.append(blk_hash)
        chain1.append(res.block_header.prev_hash)

        print('Checking alt blocks match')
        res = daemon.get_alt_blocks_hashes()
        assert len(res.blks_hashes) == len(starting_alt_blocks) + 4
        for txid in alt_blocks:
            assert txid in res.blks_hashes or txid == alt_blocks[1]

        print('mining 4 on 3')
        # 4 more on [3], the chain will reorg when we mine the 4th
        top_block_hash = blk_hash
        prev_block = alt_blocks[3]
        for i in range(4):
            res = daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 1, prev_block = prev_block)
            assert res.height == height + 1 + i
            assert len(res.blocks) == 1
            prev_block = res.blocks[-1]
            res = daemon.getblockheaderbyhash(res.blocks[-1])
            assert res.block_header.orphan_status == (i < 3)

            res = daemon.get_info()
            assert res.height == ((height + 4) if i < 3 else height + 5)
            assert res.top_block_hash == (top_block_hash if i < 3 else prev_block)

        res = daemon.get_info()
        assert res.height == height + 5
        assert res.top_block_hash == prev_block

        print('Checking alt blocks match')
        res = daemon.get_alt_blocks_hashes()
        blks_hashes = res.blks_hashes
        assert len(blks_hashes) == len(starting_alt_blocks) + 7
        for txid in alt_blocks:
            assert txid in blks_hashes or txid == alt_blocks[3]
        for txid in chain1:
            assert txid in blks_hashes

        res = daemon.get_alternate_chains()
        assert len(res.chains) == 4
        tips = [chain.block_hash for chain in res.chains]
        for txid in tips:
            assert txid in blks_hashes
        for chain in res.chains:
            assert chain.length in [1, 4]
            assert chain.length == len(chain.block_hashes)
            assert chain.height == height + chain.length - 1 # all happen start at the same height
            assert chain.main_chain_parent_block == root_block_hash
        for txid in [alt_blocks[0], alt_blocks[2], alt_blocks[4]]:
          assert len([chain for chain in res.chains if chain.block_hash == txid]) == 1

        print('Saving blockchain explicitely')
        daemon.save_bc()


if __name__ == '__main__':
    BlockchainTest().run_test()
