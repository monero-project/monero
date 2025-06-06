#!/usr/bin/env python3

# Copyright (c) 2023-2024, The Monero Project
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

import math
import random

"""
Test the k-anonymity daemon RPC features:
* txid fetching by prefix
"""

from framework.daemon import Daemon
from framework.wallet import Wallet

seeds = [
    'velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted',
    'peeled mixture ionic radar utopia puddle buying illness nuns gadget river spout cavernous bounced paradise drunk looking cottage jump tequila melting went winter adjust spout',
    'tadpoles shrugged ritual exquisite deepest rest people musical farming otherwise shelter fabrics altitude seventh request tidy ivory diet vapidly syllabus logic espionage oozed opened people',
    'ocio charla pomelo humilde maduro geranio bruto moño admitir mil difícil diva lucir cuatro odisea riego bebida mueble cáncer puchero carbón poeta flor fruta fruta'
]

pub_addrs = [
    '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm',
    '44Kbx4sJ7JDRDV5aAhLJzQCjDz2ViLRduE3ijDZu3osWKBjMGkV1XPk4pfDUMqt1Aiezvephdqm6YD19GKFD9ZcXVUTp6BW',
    '45uQD4jzWwPazqr9QJx8CmFPN7a9RaEE8T4kULg6r8GzfcrcgKXshfYf8cezLWwmENHC9pDN2fGAUFmmdFxjeZSs3n671rz',
    '48hKTTTMfuiW2gDkmsibERHCjTCpqyCCh57WcU4KBeqDSAw7dG7Ad1h7v8iJF4q59aDqBATg315MuZqVmkF89E3cLPrBWsi'
]

CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW = 60
CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE = 10
RESTRICTED_SPENT_KEY_IMAGES_COUNT = 5000

def make_hash32_loose_template(txid, nbits):
    txid_bytes = list(bytes.fromhex(txid))
    for i in reversed(range(32)):
        mask_nbits = min(8, nbits)
        mask = 256 - (1 << (8 - mask_nbits))
        nbits -= mask_nbits
        txid_bytes[i] &= mask
    return bytes(txid_bytes).hex()

def txid_list_is_sorted_in_template_order(txids):
    reversed_txid_bytes = [bytes(reversed(bytes.fromhex(txid))) for txid in txids]
    return sorted(reversed_txid_bytes) == reversed_txid_bytes

def txid_matches_template(txid, template, nbits):
    txid_bytes = bytes.fromhex(txid)
    template_bytes = bytes.fromhex(template)
    for i in reversed(range(32)):
        mask_nbits = min(8, nbits)
        mask = 256 - (1 << (8 - mask_nbits))
        nbits -= mask_nbits
        if 0 != ((txid_bytes[i] ^ template_bytes[i]) & mask):
            return False
    return True

class KAnonymityTest:
    def run_test(self):
        self.reset()
        self.create_wallets()

        # If each of the N wallets is making N-1 transfers the first round, each N wallets needs
        # N-1 unlocked coinbase outputs
        N = len(seeds)
        self.mine_and_refresh(2 * N * (N - 1))
        self.mine_and_refresh(CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW)

        # Generate a bunch of transactions
        NUM_ROUNDS = 10
        intermediate_mining_period = int(math.ceil(CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE / N)) * N
        for i in range(NUM_ROUNDS):
            self.transfer_around()
            self.mine_and_refresh(intermediate_mining_period)
        print("Wallets created {} transactions in {} rounds".format(len(self.wallet_txids), NUM_ROUNDS))

        self.test_all_chain_txids() # Also gathers miner_txids

        self.test_get_txids_loose_chain_suite()

        self.test_get_txids_loose_pool_suite()

        self.test_bad_txid_templates()

    def reset(self):
        print('Resetting blockchain')
        daemon = Daemon()
        res = daemon.get_height()
        daemon.pop_blocks(res.height - 1)
        daemon.flush_txpool()
        self.wallet_txids = set()
        self.total_blocks_mined = 0
        self.miner_txids = set()
        self.pool_txids = set()

    def create_wallets(self):
        print('Creating wallets')
        assert len(seeds) == len(pub_addrs)
        self.wallet = [None] * len(seeds)
        for i in range(len(seeds)):
            self.wallet[i] = Wallet(idx = i)
            # close the wallet if any, will throw if none is loaded
            try: self.wallet[i].close_wallet()
            except: pass
            res = self.wallet[i].restore_deterministic_wallet(seed = seeds[i])

    def mine_and_refresh(self, num_blocks):
        print("Mining {} blocks".format(num_blocks))
        daemon = Daemon()

        res = daemon.get_info()
        old_height = res.height

        assert num_blocks % len(self.wallet) == 0
        assert len(self.wallet) == len(pub_addrs)

        for i in range(len(self.wallet)):
            daemon.generateblocks(pub_addrs[i], num_blocks // len(self.wallet))

        res = daemon.get_info()
        new_height = res.height
        assert new_height == old_height + num_blocks, "height {} -> {}".format(old_height, new_height)

        for i in range(len(self.wallet)):
            self.wallet[i].refresh()
            res = self.wallet[i].get_height()
            assert res.height == new_height, "{} vs {}".format(res.height, new_height)

        self.wallet_txids.update(self.pool_txids)
        self.pool_txids.clear()
        self.total_blocks_mined += num_blocks

    def transfer_around(self):
        N = len(self.wallet)
        assert N == len(pub_addrs)

        print("Creating transfers b/t wallets")

        max_n_inputs = 8

        num_successful_transfers = 0
        fee_margin = 0.05 # 5%
        for sender in range(N):
            receivers = list((r for r in range(N) if r != sender))
            random.shuffle(receivers)
            assert len(receivers) == N - 1
            for j, receiver in enumerate(receivers):
                unlocked_balance = self.wallet[sender].get_balance().unlocked_balance
                if 0 == unlocked_balance:
                    assert j != 0 # we want all wallets to start out with at least some funds
                    break
                imperfect_starting_balance = unlocked_balance * (N - 1) / (N - 1 - j) * (1 - fee_margin)
                transfer_amount = int(imperfect_starting_balance / (N - 1))

                # Limit transfer amount to sum of max_n_inputs-1 random unlocked input candidates,
                # so that FCMP++ txs can always pay the target sum
                incoming_transfers = self.wallet[sender].get_transfers(out=False, pending=False, pool=False)['in']
                unlocked_incoming_transfers = list(filter(lambda e: not e.locked, incoming_transfers))
                random.shuffle(unlocked_incoming_transfers)
                random_unlocked_subset_sum = sum(e.amount for e in unlocked_incoming_transfers[:(max_n_inputs-1)])
                transfer_amount = min(transfer_amount, random_unlocked_subset_sum)

                assert transfer_amount < unlocked_balance
                dst = {'address': pub_addrs[receiver], 'amount': transfer_amount}
                res = self.wallet[sender].transfer([dst], get_tx_metadata = True)
                tx_hex = res.tx_metadata
                self.pool_txids.add(res.tx_hash)
                res = self.wallet[sender].relay_tx(tx_hex)
                self.wallet[sender].refresh()
                num_successful_transfers += 1

        print("Transferred {} times".format(num_successful_transfers))

    def test_all_chain_txids(self):
        daemon = Daemon()

        print("Grabbing all txids from the daemon and testing against known txids")

        # If assert stmt below fails, this test case needs to be rewritten to chunk the requests;
        # there are simply too many txids on-chain to gather at once
        expected_total_num_txids = len(self.wallet_txids) + self.total_blocks_mined + 1 # +1 for genesis coinbase tx
        assert expected_total_num_txids <= RESTRICTED_SPENT_KEY_IMAGES_COUNT

        res = daemon.get_txids_loose('0' * 64, 0)
        all_txids = res.txids
        assert 'c88ce9783b4f11190d7b9c17a69c1c52200f9faaee8e98dd07e6811175177139' in all_txids # genesis coinbase tx
        assert len(all_txids) == expected_total_num_txids, "{} {}".format(len(all_txids), expected_total_num_txids)

        assert txid_list_is_sorted_in_template_order(all_txids)

        for txid in self.wallet_txids:
            assert txid in all_txids

        self.miner_txids = set(all_txids) - self.wallet_txids

    def test_get_txids_loose_success(self, txid, num_matching_bits):
        daemon = Daemon()

        txid_template = make_hash32_loose_template(txid, num_matching_bits)

        res = daemon.get_txids_loose(txid_template, num_matching_bits)
        assert 'txids' in res
        txids = res.txids

        first_pool_index = 0
        while first_pool_index < len(txids):
            if txids[first_pool_index] in self.pool_txids:
                break
            else:
                first_pool_index += 1

        chain_txids = txids[:first_pool_index]
        pool_txids = txids[first_pool_index:]

        assert txid_list_is_sorted_in_template_order(chain_txids)
        assert txid_list_is_sorted_in_template_order(pool_txids)

        # Assert we know where txids came from
        for txid in chain_txids:
            assert (txid in self.wallet_txids) or (txid in self.miner_txids)
        for txid in pool_txids:
            assert txid in self.pool_txids

        # Assert that all known txids were matched as they should've been
        for txid in self.wallet_txids:
            assert txid_matches_template(txid, txid_template, num_matching_bits) == (txid in chain_txids)
        for txid in self.miner_txids:
            assert txid_matches_template(txid, txid_template, num_matching_bits) == (txid in chain_txids)
        for txid in self.pool_txids:
            assert txid_matches_template(txid, txid_template, num_matching_bits) == (txid in pool_txids)

    def test_get_txids_loose_chain_suite(self):
        daemon = Daemon()

        print("Testing grabbing on-chain txids loosely with all different bit sizes")

        # Assert pool empty
        assert len(self.pool_txids) == 0
        res = daemon.get_transaction_pool_hashes()
        assert not 'tx_hashes' in res or len(res.tx_hashes) == 0

        assert len(self.wallet_txids)

        current_chain_txids = list(self.wallet_txids.union(self.miner_txids))
        for nbits in range(0, 256):
            random_txid = random.choice(current_chain_txids)
            self.test_get_txids_loose_success(random_txid, nbits)

    def test_get_txids_loose_pool_suite(self):
        daemon = Daemon()

        print("Testing grabbing pool txids loosely with all different bit sizes")

        # Create transactions to pool
        self.transfer_around()

        # Assert pool not empty
        assert len(self.pool_txids) != 0
        res = daemon.get_transaction_pool_hashes()
        assert 'tx_hashes' in res and set(res.tx_hashes) == self.pool_txids

        current_pool_txids = list(self.pool_txids)
        for nbits in range(0, 256):
            random_txid = random.choice(current_pool_txids)
            self.test_get_txids_loose_success(random_txid, nbits)

    def test_bad_txid_templates(self):
        daemon = Daemon()

        print("Making sure the daemon catches bad txid templates")

        test_cases = [
            ['q', 256],
            ['a', 128],
            ['69' * 32, 257],
            ['0abcdef1234567890abcdef1234567890abcdef1234567890abcdef123456789', 0],
            ['0abcdef1234567890abcdef1234567890abcdef1234567890abcdef123456789', 1],
            ['0abcdef1234567890abcdef1234567890abcdef1234567890abcdef123456789', 2],
            ['0abcdef1234567890abcdef1234567890abcdef1234567890abcdef123456789', 4],
            ['0abcdef1234567890abcdef1234567890abcdef1234567890abcdef123456789', 8],
            ['0abcdef1234567890abcdef1234567890abcdef1234567890abcdef123456789', 16],
            ['0abcdef1234567890abcdef1234567890abcdef1234567890abcdef123456789', 32],
            ['0abcdef1234567890abcdef1234567890abcdef1234567890abcdef123456789', 64],
            ['0abcdef1234567890abcdef1234567890abcdef1234567890abcdef123456789', 128],
            ['0abcdef1234567890abcdef1234567890abcdef1234567890abcdef123456789', 193],
            ['0000000000000000000000000000000000000000000000000000000000000080', 0],
            ['0000000000000000000000000000000000000000000000000000000000000007', 5],
            ['00000000000000000000000000000000000000000000000000000000000000f7', 5],
        ]

        for txid_template, num_matching_bits in test_cases:
            ok = False
            try: res = daemon.get_txids_loose(txid_template, num_matching_bits)
            except: ok = True
            assert ok, 'bad template didnt error: {} {}'.format(txid_template, num_matching_bits)

if __name__ == '__main__':
    KAnonymityTest().run_test()
