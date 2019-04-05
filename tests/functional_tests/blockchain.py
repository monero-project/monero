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
        self._test_generateblocks(5)

    def _test_generateblocks(self, blocks):
        assert blocks >= 2

        print "Test generating", blocks, 'blocks'

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
        try: daemon.getblock(height)
        except: ok = True
        assert ok

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
            res_getblock.append(daemon.getblock(height + n))
            block_header = res_getblock[n].block_header
            assert abs(block_header.timestamp - time.time()) < 10 # within 10 seconds
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
        try: daemon.getblock(height + blocks)
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
        try: daemon.getblock(height + blocks - 1)
        except: ok = True
        assert ok


if __name__ == '__main__':
    BlockchainTest().run_test()
