#!/usr/bin/env python3

# Copyright (c) 2018-2024, The Monero Project

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

"""Test is_key_image_spent RPC across all three status codes

Tests the full lifecycle of key image spent status:
    - 0 = UNSPENT (key image not seen anywhere)
    - 1 = SPENT_IN_BLOCKCHAIN (confirmed in a block)
    - 2 = SPENT_IN_POOL (pending in mempool)

The existing test in transfer.py only covers status 0 and 1.
This test adds coverage for status 2 (SPENT_IN_POOL) and
lifecycle transitions between all three states.
"""


import json

from framework.daemon import Daemon
from framework.wallet import Wallet

SEED = 'velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted'
MINER_ADDR = '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
DEST_ADDR = '888tNkZrPN6JsEgekjMnABU4TBzc2Dt29EPAvkRxbANsAnjyPbb3iQ1YBRk1UXcdRsiKc9dhwMVgN5S9cQUiyoogDavup3H'


class KeyImageSpentTest():
    def run_test(self):
        self.reset()
        self.create_wallet()
        self.mine_blocks()
        self.check_unspent()
        self.check_spent_in_pool()
        self.check_spent_in_blockchain()
        self.check_flush_returns_to_unspent()
        self.check_mixed_states()
        self.check_edge_cases()

    def reset(self):
        print('Resetting blockchain')
        daemon = Daemon()
        res = daemon.get_height()
        daemon.pop_blocks(res.height - 1)
        daemon.flush_txpool()

    def create_wallet(self):
        print('Creating wallet')
        self.wallet = Wallet()
        try: self.wallet.close_wallet()
        except: pass
        self.wallet.restore_deterministic_wallet(seed = SEED)

    def mine_blocks(self):
        print('Mining blocks')
        daemon = Daemon()
        daemon.generateblocks(MINER_ADDR, 130)
        self.wallet.refresh()
        res = self.wallet.get_balance()
        assert res.unlocked_balance > 0, 'No unlocked balance after mining'

    def _get_input_key_images(self, tx_hash):
        """Extract input key images from a transaction via decode_as_json"""
        daemon = Daemon()
        res = daemon.get_transactions([tx_hash], decode_as_json = True)
        tx_json = json.loads(res.txs[0].as_json)
        return [inp['key']['k_image'] for inp in tx_json['vin']]

    def check_unspent(self):
        print('Testing UNSPENT status (0)')
        daemon = Daemon()

        # All available (unspent) wallet key images should return status 0
        res = self.wallet.incoming_transfers(transfer_type = 'available')
        ki = [x.key_image for x in res.transfers]
        assert len(ki) > 0, 'No available transfers found'

        res = daemon.is_key_image_spent(ki)
        assert res.spent_status == [0] * len(ki), \
            'Expected all UNSPENT (0), got: %s' % str(res.spent_status)

        print('  All key images correctly report UNSPENT (0)')

    def check_spent_in_pool(self):
        print('Testing SPENT_IN_POOL status (2)')
        daemon = Daemon()

        # Send a transaction (relayed to mempool)
        dst = [{'address': DEST_ADDR, 'amount': 1000000000000}]
        res = self.wallet.transfer(dst)
        tx_hash = res.tx_hash

        # Extract key images from the pending transaction
        ki = self._get_input_key_images(tx_hash)
        assert len(ki) > 0, 'No input key images found in tx'
        self.pool_key_images = ki

        # Key images should now be SPENT_IN_POOL (2)
        res = daemon.is_key_image_spent(ki)
        assert res.spent_status == [2] * len(ki), \
            'Expected all SPENT_IN_POOL (2), got: %s' % str(res.spent_status)

        print('  %d key image(s) correctly report SPENT_IN_POOL (2)' % len(ki))

    def check_spent_in_blockchain(self):
        print('Testing SPENT_IN_POOL -> SPENT_IN_BLOCKCHAIN transition (2 -> 1)')
        daemon = Daemon()

        # Mine a block to confirm the pool transaction
        daemon.generateblocks(MINER_ADDR, 1)
        self.wallet.refresh()

        # Key images should now be SPENT_IN_BLOCKCHAIN (1)
        ki = self.pool_key_images
        res = daemon.is_key_image_spent(ki)
        assert res.spent_status == [1] * len(ki), \
            'Expected all SPENT_IN_BLOCKCHAIN (1) after mining, got: %s' % str(res.spent_status)

        # Save one confirmed key image for mixed-state test
        self.confirmed_key_images = ki

        print('  %d key image(s) correctly transitioned to SPENT_IN_BLOCKCHAIN (1)' % len(ki))

    def check_flush_returns_to_unspent(self):
        print('Testing pool flush returns key images to UNSPENT (2 -> 0)')
        daemon = Daemon()

        # Create a new transaction (need to refresh wallet first)
        self.wallet.refresh()
        dst = [{'address': DEST_ADDR, 'amount': 1000000000000}]
        res = self.wallet.transfer(dst)
        tx_hash = res.tx_hash

        # Verify it's in pool
        ki = self._get_input_key_images(tx_hash)
        res = daemon.is_key_image_spent(ki)
        assert res.spent_status == [2] * len(ki), \
            'Expected SPENT_IN_POOL (2) before flush, got: %s' % str(res.spent_status)

        # Flush the mempool
        daemon.flush_txpool()

        # Key images should revert to UNSPENT (0)
        res = daemon.is_key_image_spent(ki)
        assert res.spent_status == [0] * len(ki), \
            'Expected UNSPENT (0) after flush, got: %s' % str(res.spent_status)

        print('  %d key image(s) correctly reverted to UNSPENT (0) after flush' % len(ki))

    def check_mixed_states(self):
        print('Testing mixed states in a single query')
        daemon = Daemon()

        # Refresh wallet to pick up the flushed outputs
        self.wallet.refresh()

        # Create a new pool transaction for a fresh SPENT_IN_POOL key image
        dst = [{'address': DEST_ADDR, 'amount': 1000000000000}]
        res = self.wallet.transfer(dst)
        tx_hash = res.tx_hash
        pool_ki = self._get_input_key_images(tx_hash)

        # We now have three types of key images:
        # - confirmed_key_images: SPENT_IN_BLOCKCHAIN (1)
        # - unknown key image: UNSPENT (0)
        # - pool_ki: SPENT_IN_POOL (2)
        unknown_ki = ['ab' * 32]  # 64 hex chars, never seen

        query = self.confirmed_key_images[:1] + unknown_ki + pool_ki[:1]
        expected = [1, 0, 2]

        res = daemon.is_key_image_spent(query)
        assert res.spent_status == expected, \
            'Mixed state query: expected %s, got %s' % (str(expected), str(res.spent_status))

        # Clean up: flush the pool transaction
        daemon.flush_txpool()

        print('  Mixed states [BLOCKCHAIN=1, UNSPENT=0, POOL=2] verified in single call')

    def check_edge_cases(self):
        print('Testing edge cases')
        daemon = Daemon()

        # Empty list: RPC omits spent_status field entirely
        res = daemon.is_key_image_spent([])
        assert 'spent_status' not in res or res.spent_status == [], \
            'Expected no spent_status or empty list for empty input'
        print('  Empty key_images list handled correctly')

        # Unknown key images (valid hex, correct length)
        unknown = ['00' * 32, 'ff' * 32, 'aa' * 32]
        res = daemon.is_key_image_spent(unknown)
        assert res.spent_status == [0, 0, 0], \
            'Expected all UNSPENT for unknown key images, got: %s' % str(res.spent_status)
        print('  Unknown key images correctly return UNSPENT (0)')

        # Duplicate key images in a single query
        res = self.wallet.incoming_transfers(transfer_type = 'unavailable')
        if 'transfers' in res and len(res.transfers) > 0:
            ki = res.transfers[0].key_image
            res = daemon.is_key_image_spent([ki, ki, ki])
            assert res.spent_status == [1, 1, 1], \
                'Expected all SPENT_IN_BLOCKCHAIN for duplicates, got: %s' % str(res.spent_status)
            print('  Duplicate key images handled correctly')

        print('  Edge cases passed')


if __name__ == '__main__':
    KeyImageSpentTest().run_test()
