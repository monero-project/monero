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
import json
import pprint
from deepdiff import DeepDiff
pp = pprint.PrettyPrinter(indent=2)

"""Test simple transfers
"""

from framework.daemon import Daemon
from framework.wallet import Wallet

seeds = [
    'velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted',
    'peeled mixture ionic radar utopia puddle buying illness nuns gadget river spout cavernous bounced paradise drunk looking cottage jump tequila melting went winter adjust spout',
    'dilute gutter certain antics pamphlet macro enjoy left slid guarded bogeys upload nineteen bomb jubilee enhanced irritate turnip eggs swung jukebox loudly reduce sedan slid',
]

class TransferTest():
    def run_test(self):
        self.reset()
        self.create()
        self.mine()
        self.transfer()
        self.check_get_bulk_payments()
        self.check_get_payments()
        self.check_double_spend_detection()
        self.sweep_dust()
        self.sweep_single()
        self.check_destinations()
        self.check_tx_notes()
        self.check_rescan()
        self.check_is_key_image_spent()
        self.check_multiple_submissions()
        self.check_scan_tx()

    def reset(self):
        print('Resetting blockchain')
        daemon = Daemon()
        res = daemon.get_height()
        daemon.pop_blocks(res.height - 1)
        daemon.flush_txpool()

    def create(self):
        print('Creating wallets')
        self.wallet = [None] * len(seeds)
        for i in range(len(seeds)):
            self.wallet[i] = Wallet(idx = i)
            # close the wallet if any, will throw if none is loaded
            try: self.wallet[i].close_wallet()
            except: pass
            res = self.wallet[i].restore_deterministic_wallet(seed = seeds[i])

    def mine(self):
        print("Mining some blocks")
        daemon = Daemon()

        res = daemon.get_info()
        height = res.height

        daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 100)
        for i in range(len(self.wallet)):
            self.wallet[i].refresh()
            res = self.wallet[i].get_height()
            assert res.height == height + 100

    def transfer(self):
        daemon = Daemon()

        print("Creating transfer to self")

        dst = {'address': '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 'amount': 1000000000000}
        payment_id = '1234500000012345abcde00000abcdeff1234500000012345abcde00000abcde'

        start_balances = [0] * len(self.wallet)
        running_balances = [0] * len(self.wallet)
        for i in range(len(self.wallet)):
          res = self.wallet[i].get_balance()
          start_balances[i] = res.balance
          running_balances[i] = res.balance
          assert res.unlocked_balance <= res.balance
          if i == 0:
            assert res.blocks_to_unlock == 59 # we've been mining to it
          else:
            assert res.blocks_to_unlock == 0

        print ('Checking short payment IDs cannot be used when not in an integrated address')
        ok = False
        try: self.wallet[0].transfer([dst], ring_size = 16, payment_id = '1234567812345678', get_tx_key = False)
        except: ok = True
        assert ok

        print ('Checking long payment IDs are rejected')
        ok = False
        try: self.wallet[0].transfer([dst], ring_size = 16, payment_id = payment_id, get_tx_key = False, get_tx_hex = True)
        except: ok = True
        assert ok

        print ('Checking empty destination is rejected')
        ok = False
        try: self.wallet[0].transfer([], ring_size = 16, get_tx_key = False)
        except: ok = True
        assert ok

        res = self.wallet[0].transfer([dst], ring_size = 16, get_tx_key = False, get_tx_hex = True)
        assert len(res.tx_hash) == 32*2
        txid = res.tx_hash
        assert len(res.tx_key) == 0
        assert res.amount > 0
        amount = res.amount
        assert res.fee > 0
        fee = res.fee
        assert len(res.tx_blob) > 0
        tx_weight = res.weight
        assert len(res.tx_metadata) == 0
        assert len(res.multisig_txset) == 0
        assert len(res.unsigned_txset) == 0
        unsigned_txset = res.unsigned_txset

        res = daemon.get_fee_estimate(10)
        assert res.fee > 0
        assert res.quantization_mask > 0
        expected_fee = (res.fee * 1 * tx_weight + res.quantization_mask - 1) // res.quantization_mask * res.quantization_mask
        assert abs(1 - fee / expected_fee) < 0.01

        self.wallet[0].refresh()

        res = daemon.get_info()
        height = res.height

        res = self.wallet[0].get_transfers()
        assert len(res['in']) == height - 1 # coinbases
        assert not 'out' in res or len(res.out) == 0 # not mined yet
        assert len(res.pending) == 1
        assert not 'pool' in res or len(res.pool) == 0
        assert not 'failed' in res or len(res.failed) == 0
        for e in res['in']:
          assert e.type == 'block'
        e = res.pending[0]
        assert e.txid == txid
        assert e.payment_id in ['', '0000000000000000']
        assert e.type == 'pending'
        assert e.unlock_time == 0
        assert e.subaddr_index.major == 0
        assert e.subaddr_indices == [{'major': 0, 'minor': 0}]
        assert e.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
        assert e.double_spend_seen == False
        assert not 'confirmations' in e or e.confirmations == 0

        running_balances[0] -= fee

        res = self.wallet[0].get_balance()
        assert res.balance == running_balances[0]
        assert res.unlocked_balance <= res.balance
        assert res.blocks_to_unlock == 59

        daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 1)
        res = daemon.getlastblockheader()
        running_balances[0] += res.block_header.reward
        self.wallet[0].refresh()

        res = self.wallet[0].get_transfers()
        assert len(res['in']) == height # coinbases
        assert len(res.out) == 1 # not mined yet
        assert not 'pending' in res or len(res.pending) == 0
        assert not 'pool' in res or len(res.pool) == 0
        assert not 'failed' in res or len(res.failed) == 0
        for e in res['in']:
          assert e.type == 'block'
        e = res.out[0]
        assert e.txid == txid
        assert e.payment_id in ['', '0000000000000000']
        assert e.type == 'out'
        assert e.unlock_time == 0
        assert e.subaddr_index.major == 0
        assert e.subaddr_indices == [{'major': 0, 'minor': 0}]
        assert e.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
        assert e.double_spend_seen == False
        assert e.confirmations == 1

        res = self.wallet[0].get_height()
        wallet_height = res.height
        res = self.wallet[0].get_transfer_by_txid(txid)
        assert len(res.transfers) == 1
        assert res.transfers[0] == res.transfer
        t = res.transfer
        assert t.txid == txid
        assert t.payment_id in ['', '0000000000000000']
        assert t.height == wallet_height - 1
        assert t.timestamp > 0
        assert t.amount == 0 # to self, so it's just "pay a fee" really
        assert t.fee == fee
        assert t.note == ''
        assert len(t.destinations) == 1
        assert t.destinations[0] == {'address': '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 'amount': 1000000000000}
        assert t.type == 'out'
        assert t.unlock_time == 0
        assert t.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
        assert t.double_spend_seen == False
        assert t.confirmations == 1

        res = self.wallet[0].get_balance()
        assert res.balance == running_balances[0]
        assert res.unlocked_balance <= res.balance
        assert res.blocks_to_unlock == 59

        print("Creating transfer to another, manual relay")

        dst = {'address': '44Kbx4sJ7JDRDV5aAhLJzQCjDz2ViLRduE3ijDZu3osWKBjMGkV1XPk4pfDUMqt1Aiezvephdqm6YD19GKFD9ZcXVUTp6BW', 'amount': 1000000000000}
        res = self.wallet[0].transfer([dst], ring_size = 16, get_tx_key = True, do_not_relay = True, get_tx_hex = True)
        assert len(res.tx_hash) == 32*2
        txid = res.tx_hash
        assert len(res.tx_key) == 32*2
        assert res.amount == 1000000000000
        amount = res.amount
        assert res.fee > 0
        fee = res.fee
        assert len(res.tx_blob) > 0
        assert len(res.tx_metadata) == 0
        assert len(res.multisig_txset) == 0
        assert len(res.unsigned_txset) == 0
        tx_blob = res.tx_blob

        res = daemon.send_raw_transaction(tx_blob)
        assert res.not_relayed == False
        assert res.low_mixin == False
        assert res.double_spend == False
        assert res.invalid_input == False
        assert res.invalid_output == False
        assert res.too_big == False
        assert res.overspend == False
        assert res.fee_too_low == False

        self.wallet[0].refresh()

        res = self.wallet[0].get_balance()
        assert res.balance == running_balances[0]
        assert res.unlocked_balance <= res.balance
        assert res.blocks_to_unlock == 59

        self.wallet[1].refresh()

        res = self.wallet[1].get_transfers()
        assert not 'in' in res or len(res['in']) == 0
        assert not 'out' in res or len(res.out) == 0
        assert not 'pending' in res or len(res.pending) == 0
        assert len(res.pool) == 1
        assert not 'failed' in res or len(res.failed) == 0
        e = res.pool[0]
        assert e.txid == txid
        assert e.payment_id in ["", "0000000000000000"] # long payment IDs are now ignored
        assert e.type == 'pool'
        assert e.unlock_time == 0
        assert e.subaddr_index.major == 0
        assert e.subaddr_indices == [{'major': 0, 'minor': 0}]
        assert e.address == '44Kbx4sJ7JDRDV5aAhLJzQCjDz2ViLRduE3ijDZu3osWKBjMGkV1XPk4pfDUMqt1Aiezvephdqm6YD19GKFD9ZcXVUTp6BW'
        assert e.double_spend_seen == False
        assert not 'confirmations' in e or e.confirmations == 0
        assert e.amount == amount
        assert e.fee == fee

        daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 1)
        res = daemon.getlastblockheader()
        running_balances[0] -= 1000000000000 + fee
        running_balances[0] += res.block_header.reward
        self.wallet[1].refresh()
        running_balances[1] += 1000000000000

        res = self.wallet[1].get_transfers()
        assert len(res['in']) == 1
        assert not 'out' in res or len(res.out) == 0
        assert not 'pending' in res or len(res.pending) == 0
        assert not 'pool' in res or len(res.pool) == 0
        assert not 'failed' in res or len(res.failed) == 0
        e = res['in'][0]
        assert e.txid == txid
        assert e.payment_id in ["", "0000000000000000"] # long payment IDs are now ignored
        assert e.type == 'in'
        assert e.unlock_time == 0
        assert e.subaddr_index.major == 0
        assert e.subaddr_indices == [{'major': 0, 'minor': 0}]
        assert e.address == '44Kbx4sJ7JDRDV5aAhLJzQCjDz2ViLRduE3ijDZu3osWKBjMGkV1XPk4pfDUMqt1Aiezvephdqm6YD19GKFD9ZcXVUTp6BW'
        assert e.double_spend_seen == False
        assert e.confirmations == 1
        assert e.amount == amount
        assert e.fee == fee

        res = self.wallet[1].get_balance()
        assert res.balance == running_balances[1]
        assert res.unlocked_balance <= res.balance
        assert res.blocks_to_unlock == 9

        print('Creating multi out transfer')

        self.wallet[0].refresh()

        dst0 = {'address': '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 'amount': 1000000000000}
        dst1 = {'address': '44Kbx4sJ7JDRDV5aAhLJzQCjDz2ViLRduE3ijDZu3osWKBjMGkV1XPk4pfDUMqt1Aiezvephdqm6YD19GKFD9ZcXVUTp6BW', 'amount': 1100000000000}
        dst2 = {'address': '46r4nYSevkfBUMhuykdK3gQ98XDqDTYW1hNLaXNvjpsJaSbNtdXh1sKMsdVgqkaihChAzEy29zEDPMR3NHQvGoZCLGwTerK', 'amount': 1200000000000}
        res = self.wallet[0].transfer([dst0, dst1, dst2], ring_size = 16, get_tx_key = True)
        assert len(res.tx_hash) == 32*2
        txid = res.tx_hash
        assert len(res.tx_key) == 32*2
        assert res.amount == 1000000000000 + 1100000000000 + 1200000000000
        amount = res.amount
        assert res.fee > 0
        fee = res.fee
        assert len(res.tx_blob) == 0
        assert len(res.tx_metadata) == 0
        assert len(res.multisig_txset) == 0
        assert len(res.unsigned_txset) == 0
        unsigned_txset = res.unsigned_txset

        running_balances[0] -= 1100000000000 + 1200000000000 + fee

        res = self.wallet[0].get_balance()
        assert res.balance == running_balances[0]
        assert res.unlocked_balance <= res.balance
        assert res.blocks_to_unlock == 59

        daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 1)
        res = daemon.getlastblockheader()
        running_balances[0] += res.block_header.reward
        running_balances[1] += 1100000000000
        running_balances[2] += 1200000000000
        self.wallet[0].refresh()

        res = self.wallet[0].get_transfers()
        assert len(res['in']) == height + 2
        assert len(res.out) == 3
        assert not 'pending' in res or len(res.pending) == 0
        assert not 'pool' in res or len(res.pool) == 1
        assert not 'failed' in res or len(res.failed) == 0
        e = [o for o in res.out if o.txid == txid]
        assert len(e) == 1
        e = e[0]
        assert e.txid == txid
        assert e.payment_id in ["", "0000000000000000"] # long payment IDs are now ignored
        assert e.type == 'out'
        assert e.unlock_time == 0
        assert e.subaddr_index.major == 0
        assert e.subaddr_indices == [{'major': 0, 'minor': 0}]
        assert e.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
        assert e.double_spend_seen == False
        assert e.confirmations == 1

        assert e.amount == amount
        assert e.fee == fee

        res = self.wallet[0].get_balance()
        assert res.balance == running_balances[0]
        assert res.unlocked_balance <= res.balance
        assert res.blocks_to_unlock == 59

        self.wallet[1].refresh()
        res = self.wallet[1].get_transfers()
        assert len(res['in']) == 2
        assert not 'out' in res or len(res.out) == 0
        assert not 'pending' in res or len(res.pending) == 0
        assert not 'pool' in res or len(res.pool) == 0
        assert not 'failed' in res or len(res.failed) == 0
        e = [o for o in res['in'] if o.txid == txid]
        assert len(e) == 1
        e = e[0]
        assert e.txid == txid
        assert e.payment_id in ["", "0000000000000000"] # long payment IDs are now ignored
        assert e.type == 'in'
        assert e.unlock_time == 0
        assert e.subaddr_index.major == 0
        assert e.subaddr_indices == [{'major': 0, 'minor': 0}]
        assert e.address == '44Kbx4sJ7JDRDV5aAhLJzQCjDz2ViLRduE3ijDZu3osWKBjMGkV1XPk4pfDUMqt1Aiezvephdqm6YD19GKFD9ZcXVUTp6BW'
        assert e.double_spend_seen == False
        assert e.confirmations == 1
        assert e.amount == 1100000000000
        assert e.fee == fee

        res = self.wallet[1].get_balance()
        assert res.balance == running_balances[1]
        assert res.unlocked_balance <= res.balance
        assert res.blocks_to_unlock == 9

        self.wallet[2].refresh()
        res = self.wallet[2].get_transfers()
        assert len(res['in']) == 1
        assert not 'out' in res or len(res.out) == 0
        assert not 'pending' in res or len(res.pending) == 0
        assert not 'pool' in res or len(res.pool) == 0
        assert not 'failed' in res or len(res.failed) == 0
        e = [o for o in res['in'] if o.txid == txid]
        assert len(e) == 1
        e = e[0]
        assert e.txid == txid
        assert e.payment_id in ["", "0000000000000000"] # long payment IDs are now ignored
        assert e.type == 'in'
        assert e.unlock_time == 0
        assert e.subaddr_index.major == 0
        assert e.subaddr_indices == [{'major': 0, 'minor': 0}]
        assert e.address == '46r4nYSevkfBUMhuykdK3gQ98XDqDTYW1hNLaXNvjpsJaSbNtdXh1sKMsdVgqkaihChAzEy29zEDPMR3NHQvGoZCLGwTerK'
        assert e.double_spend_seen == False
        assert e.confirmations == 1
        assert e.amount == 1200000000000
        assert e.fee == fee

        res = self.wallet[2].get_balance()
        assert res.balance == running_balances[2]
        assert res.unlocked_balance <= res.balance
        assert res.blocks_to_unlock == 9

        print('Sending to integrated address')
        self.wallet[0].refresh()
        res = self.wallet[0].get_balance()
        i_pid = '1111111122222222'
        res = self.wallet[0].make_integrated_address(standard_address = '44Kbx4sJ7JDRDV5aAhLJzQCjDz2ViLRduE3ijDZu3osWKBjMGkV1XPk4pfDUMqt1Aiezvephdqm6YD19GKFD9ZcXVUTp6BW', payment_id = i_pid)
        i_address = res.integrated_address
        res = self.wallet[0].transfer([{'address': i_address, 'amount': 200000000}])
        assert len(res.tx_hash) == 32*2
        i_txid = res.tx_hash
        assert len(res.tx_key) == 32*2
        assert res.amount == 200000000
        i_amount = res.amount
        assert res.fee > 0
        fee = res.fee
        assert len(res.tx_blob) == 0
        assert len(res.tx_metadata) == 0
        assert len(res.multisig_txset) == 0
        assert len(res.unsigned_txset) == 0

        running_balances[0] -= 200000000 + fee

        res = self.wallet[0].get_balance()
        assert res.balance == running_balances[0]
        assert res.unlocked_balance <= res.balance
        assert res.blocks_to_unlock == 59

        daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 1)
        res = daemon.getlastblockheader()
        running_balances[0] += res.block_header.reward
        running_balances[1] += 200000000

        self.wallet[0].refresh()
        res = self.wallet[0].get_balance()
        assert res.balance == running_balances[0]
        assert res.unlocked_balance <= res.balance
        assert res.blocks_to_unlock == 59

        self.wallet[1].refresh()
        res = self.wallet[1].get_balance()
        assert res.balance == running_balances[1]
        assert res.unlocked_balance <= res.balance
        assert res.blocks_to_unlock == 9

        self.wallet[2].refresh()
        res = self.wallet[2].get_balance()
        assert res.balance == running_balances[2]
        assert res.unlocked_balance <= res.balance
        assert res.blocks_to_unlock == 8

        daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 1)
        res = daemon.getlastblockheader()
        running_balances[0] += res.block_header.reward

        self.wallet[0].refresh()
        res = self.wallet[0].get_balance()
        assert res.balance == running_balances[0]
        assert res.unlocked_balance <= res.balance
        assert res.blocks_to_unlock == 59

        self.wallet[1].refresh()
        res = self.wallet[1].get_balance()
        assert res.balance == running_balances[1]
        assert res.unlocked_balance <= res.balance
        assert res.blocks_to_unlock == 8

        self.wallet[2].refresh()
        res = self.wallet[2].get_balance()
        assert res.balance == running_balances[2]
        assert res.unlocked_balance <= res.balance
        assert res.blocks_to_unlock == 7


    def check_get_bulk_payments(self):
        print('Checking get_bulk_payments')

        daemon = Daemon()
        res = daemon.get_info()
        height = res.height

        self.wallet[0].refresh()
        res = self.wallet[0].get_bulk_payments()
        assert len(res.payments) >= 83 # at least 83 coinbases
        res = self.wallet[0].get_bulk_payments(payment_ids = ['1234500000012345abcde00000abcdeff1234500000012345abcde00000abcde'])
        assert 'payments' not in res or len(res.payments) == 0
        res = self.wallet[0].get_bulk_payments(min_block_height = height)
        assert 'payments' not in res or len(res.payments) == 0
        res = self.wallet[0].get_bulk_payments(min_block_height = height - 40)
        assert len(res.payments) >= 39 # coinbases

        self.wallet[1].refresh()
        res = self.wallet[1].get_bulk_payments()
        assert len(res.payments) >= 3 # two txes to standard address were sent, plus one to integrated address
        res = self.wallet[1].get_bulk_payments(payment_ids = ['1234500000012345abcde00000abcdeff1234500000012345abcde00000abcde'])
        assert not 'payments' in res or len(res.payments) == 0 # long payment IDs are now ignored on receipt
        res = self.wallet[1].get_bulk_payments(payment_ids = ['ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff'])
        assert 'payments' not in res or len(res.payments) == 0 # none with that payment id
        res = self.wallet[1].get_bulk_payments(payment_ids = ['1111111122222222' + '0'*48])
        assert len(res.payments) >= 1 # one tx to integrated address

        self.wallet[2].refresh()
        res = self.wallet[2].get_bulk_payments()
        assert len(res.payments) >= 1 # one tx was sent
        res = self.wallet[2].get_bulk_payments(payment_ids = ['1'*64, '1234500000012345abcde00000abcdeff1234500000012345abcde00000abcde', '2'*64])
        assert not 'payments' in res or len(res.payments) == 0 # long payment IDs are now ignored

        res = self.wallet[1].get_bulk_payments(["1111111122222222"])
        assert len(res.payments) >= 1 # we have one of these

    def check_get_payments(self):
        print('Checking get_payments')

        daemon = Daemon()
        res = daemon.get_info()
        height = res.height

        self.wallet[0].refresh()
        self.wallet[1].refresh()

        res = self.wallet[0].get_payments('1234500000012345abcde00000abcdeff1234500000012345abcde00000abcde')
        assert 'payments' not in res or len(res.payments) == 0

        res = self.wallet[1].get_payments('1234500000012345abcde00000abcdeff1234500000012345abcde00000abcde')
        assert 'payments' not in res or len(res.payments) == 0

        res = self.wallet[1].get_payments('ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff')
        assert 'payments' not in res or len(res.payments) == 0

        res = self.wallet[1].get_payments(payment_id = '1111111122222222' + '0'*48)
        assert len(res.payments) >= 1 # one tx to integrated address

    def check_double_spend_detection(self):
        print('Checking double spend detection')
        txes = [[None, None], [None, None]]
        for i in range(2):
            self.wallet[0].restore_deterministic_wallet(seed = 'velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted')
            self.wallet[0].refresh()
            res = self.wallet[0].get_balance()
            unlocked_balance = res.unlocked_balance
            res = self.wallet[0].sweep_all(address = '44Kbx4sJ7JDRDV5aAhLJzQCjDz2ViLRduE3ijDZu3osWKBjMGkV1XPk4pfDUMqt1Aiezvephdqm6YD19GKFD9ZcXVUTp6BW', do_not_relay = True, get_tx_hex = True)
            assert len(res.tx_hash_list) == 1
            assert len(res.tx_hash_list[0]) == 32*2
            txes[i][0] = res.tx_hash_list[0]
            assert len(res.fee_list) == 1
            assert res.fee_list[0] > 0
            assert len(res.amount_list) == 1
            assert res.amount_list[0] == unlocked_balance - res.fee_list[0]
            assert len(res.tx_blob_list) > 0
            assert len(res.tx_blob_list[0]) > 0
            assert not 'tx_metadata_list' in res or len(res.tx_metadata_list) == 0
            assert not 'multisig_txset' in res or len(res.multisig_txset) == 0
            assert not 'unsigned_txset' in res or len(res.unsigned_txset) == 0
            assert len(res.tx_blob_list) == 1
            txes[i][1] = res.tx_blob_list[0]

        daemon = Daemon()
        res = daemon.send_raw_transaction(txes[0][1])
        assert res.not_relayed == False
        assert res.low_mixin == False
        assert res.double_spend == False
        assert res.invalid_input == False
        assert res.invalid_output == False
        assert res.too_big == False
        assert res.overspend == False
        assert res.fee_too_low == False

        res = daemon.get_transactions([txes[0][0]])
        assert len(res.txs) >= 1
        tx = [tx for tx in res.txs if tx.tx_hash == txes[0][0]][0]
        assert tx.in_pool
        assert not tx.double_spend_seen

        res = daemon.send_raw_transaction(txes[1][1])
        assert res.not_relayed == False
        assert res.low_mixin == False
        assert res.double_spend == True
        assert res.invalid_input == False
        assert res.invalid_output == False
        assert res.too_big == False
        assert res.overspend == False
        assert res.fee_too_low == False
        assert res.too_few_outputs == False

        res = daemon.get_transactions([txes[0][0]])
        assert len(res.txs) >= 1
        tx = [tx for tx in res.txs if tx.tx_hash == txes[0][0]][0]
        assert tx.in_pool
        assert tx.double_spend_seen

    def sweep_dust(self):
        print("Sweeping dust")
        daemon = Daemon()
        self.wallet[0].refresh()
        res = self.wallet[0].sweep_dust()
        assert not 'tx_hash_list' in res or len(res.tx_hash_list) == 0 # there's just one, but it cannot meet the fee

    def sweep_single(self):
        daemon = Daemon()

        print("Sending single output")

        daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 1)
        self.wallet[0].refresh()
        res = self.wallet[0].incoming_transfers(transfer_type = 'available')
        for t in res.transfers:
            assert not t.spent
        assert len(res.transfers) > 8 # we mined a lot
        index = 8
        assert not res.transfers[index].spent
        assert res.transfers[index].amount > 0
        ki = res.transfers[index].key_image
        amount = res.transfers[index].amount
        daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 10) # ensure unlocked
        self.wallet[0].refresh()
        res = self.wallet[0].get_balance()
        balance = res.balance
        res = daemon.is_key_image_spent([ki])
        assert len(res.spent_status) == 1
        assert res.spent_status[0] == 0
        res = self.wallet[0].sweep_single('44Kbx4sJ7JDRDV5aAhLJzQCjDz2ViLRduE3ijDZu3osWKBjMGkV1XPk4pfDUMqt1Aiezvephdqm6YD19GKFD9ZcXVUTp6BW', key_image = ki)
        assert len(res.tx_hash) == 64
        tx_hash = res.tx_hash
        res = daemon.is_key_image_spent([ki])
        assert len(res.spent_status) == 1
        assert res.spent_status[0] == 2
        daemon.generateblocks('44Kbx4sJ7JDRDV5aAhLJzQCjDz2ViLRduE3ijDZu3osWKBjMGkV1XPk4pfDUMqt1Aiezvephdqm6YD19GKFD9ZcXVUTp6BW', 1)
        res = daemon.is_key_image_spent([ki])
        assert len(res.spent_status) == 1
        assert res.spent_status[0] == 1
        self.wallet[0].refresh()
        res = self.wallet[0].get_balance()
        new_balance = res.balance
        res = daemon.get_transactions([tx_hash], decode_as_json = True)
        assert len(res.txs) == 1
        tx = res.txs[0]
        assert tx.tx_hash == tx_hash
        assert not tx.in_pool
        assert len(tx.as_json) > 0
        try:
            j = json.loads(tx.as_json)
        except:
            j = None
        assert j
        assert new_balance == balance - amount
        assert len(j['vin']) == 1
        assert j['vin'][0]['key']['k_image'] == ki
        self.wallet[0].refresh()
        res = self.wallet[0].incoming_transfers(transfer_type = 'available')
        assert len([t for t in res.transfers if t.key_image == ki]) == 0
        res = self.wallet[0].incoming_transfers(transfer_type = 'unavailable')
        assert len([t for t in res.transfers if t.key_image == ki]) == 1

    def check_destinations(self):
        daemon = Daemon()

        print("Checking transaction destinations")

        dst = {'address': '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 'amount': 1000000000000}
        res = self.wallet[0].transfer([dst])
        assert len(res.tx_hash) == 64
        tx_hash = res.tx_hash
        for i in range(2):
            res = self.wallet[0].get_transfers(pending = True, out = True)
            l = [x for x in (res.pending if i == 0 else res.out) if x.txid == tx_hash]
            assert len(l) == 1
            e = l[0]
            assert len(e.destinations) == 1
            assert e.destinations[0].amount == 1000000000000
            assert e.destinations[0].address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'

            if i == 0:
                daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 1)
                self.wallet[0].refresh()

        dst = {'address': '8AsN91rznfkBGTY8psSNkJBg9SZgxxGGRUhGwRptBhgr5XSQ1XzmA9m8QAnoxydecSh5aLJXdrgXwTDMMZ1AuXsN1EX5Mtm', 'amount': 1000000000000}
        res = self.wallet[0].transfer([dst])
        assert len(res.tx_hash) == 64
        tx_hash = res.tx_hash
        for i in range(2):
            res = self.wallet[0].get_transfers(pending = True, out = True)
            l = [x for x in (res.pending if i == 0 else res.out) if x.txid == tx_hash]
            assert len(l) == 1
            e = l[0]
            assert len(e.destinations) == 1
            assert e.destinations[0].amount == 1000000000000
            assert e.destinations[0].address == '8AsN91rznfkBGTY8psSNkJBg9SZgxxGGRUhGwRptBhgr5XSQ1XzmA9m8QAnoxydecSh5aLJXdrgXwTDMMZ1AuXsN1EX5Mtm'

            if i == 0:
                daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 1)
                self.wallet[0].refresh()

        dst = {'address': '4BxSHvcgTwu25WooY4BVmgdcKwZu5EksVZSZkDd6ooxSVVqQ4ubxXkhLF6hEqtw96i9cf3cVfLw8UWe95bdDKfRQeYtPwLm1Jiw7AKt2LY', 'amount': 1000000000000}
        res = self.wallet[0].transfer([dst])
        assert len(res.tx_hash) == 64
        tx_hash = res.tx_hash
        for i in range(2):
            res = self.wallet[0].get_transfers(pending = True, out = True)
            l = [x for x in (res.pending if i == 0 else res.out) if x.txid == tx_hash]
            assert len(l) == 1
            e = l[0]
            assert len(e.destinations) == 1
            assert e.destinations[0].amount == 1000000000000
            assert e.destinations[0].address == '4BxSHvcgTwu25WooY4BVmgdcKwZu5EksVZSZkDd6ooxSVVqQ4ubxXkhLF6hEqtw96i9cf3cVfLw8UWe95bdDKfRQeYtPwLm1Jiw7AKt2LY'

            if i == 0:
                daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 1)
                self.wallet[0].refresh()

    def check_tx_notes(self):
        daemon = Daemon()

        print('Testing tx notes')
        res = self.wallet[0].get_transfers()
        assert len(res['in']) > 0
        in_txid = res['in'][0].txid
        assert len(res['out']) > 0
        out_txid = res['out'][0].txid
        res = self.wallet[0].get_tx_notes([in_txid, out_txid])
        assert res.notes == ['', '']
        res = self.wallet[0].set_tx_notes([in_txid, out_txid], ['in txid', 'out txid'])
        res = self.wallet[0].get_tx_notes([in_txid, out_txid])
        assert res.notes == ['in txid', 'out txid']
        res = self.wallet[0].get_tx_notes([out_txid, in_txid])
        assert res.notes == ['out txid', 'in txid']

    def check_rescan(self):
        daemon = Daemon()

        print('Testing rescan_spent')
        res = self.wallet[0].incoming_transfers(transfer_type = 'all')
        transfers = res.transfers
        res = self.wallet[0].rescan_spent()
        res = self.wallet[0].incoming_transfers(transfer_type = 'all')
        assert transfers == res.transfers

        for hard in [False, True]:
            print('Testing %s rescan_blockchain' % ('hard' if hard else 'soft'))
            res = self.wallet[0].incoming_transfers(transfer_type = 'all')
            transfers = res.transfers
            res = self.wallet[0].get_transfers()
            t_in = res['in']
            t_out = res.out
            res = self.wallet[0].rescan_blockchain(hard = hard)
            res = self.wallet[0].incoming_transfers(transfer_type = 'all')
            assert transfers == res.transfers
            res = self.wallet[0].get_transfers()
            assert t_in == res['in']
            # some information can not be recovered for out txes
            unrecoverable_fields = ['payment_id', 'destinations', 'note']
            old_t_out = []
            for x in t_out:
                e = {}
                for k in x.keys():
                    if not k in unrecoverable_fields:
                        e[k] = x[k]
                old_t_out.append(e)
            new_t_out = []
            for x in res.out:
                e = {}
                for k in x.keys():
                    if not k in unrecoverable_fields:
                        e[k] = x[k]
                new_t_out.append(e)
            assert sorted(old_t_out, key = lambda k: k['txid']) == sorted(new_t_out, key = lambda k: k['txid'])

    def check_is_key_image_spent(self):
        daemon = Daemon()

        print('Testing is_key_image_spent')
        res = self.wallet[0].incoming_transfers(transfer_type = 'all')
        transfers = res.transfers
        ki = [x.key_image for x in transfers]
        expected = [1 if x.spent else 0 for x in transfers]
        res = daemon.is_key_image_spent(ki)
        assert res.spent_status == expected

        res = self.wallet[0].incoming_transfers(transfer_type = 'available')
        transfers = res.transfers
        ki = [x.key_image for x in transfers]
        expected = [0 for x in transfers]
        res = daemon.is_key_image_spent(ki)
        assert res.spent_status == expected

        res = self.wallet[0].incoming_transfers(transfer_type = 'unavailable')
        transfers = res.transfers
        ki = [x.key_image for x in transfers]
        expected = [1 for x in transfers]
        res = daemon.is_key_image_spent(ki)
        assert res.spent_status == expected

        ki = [ki[-1]] * 5
        expected = [1] * len(ki)
        res = daemon.is_key_image_spent(ki)
        assert res.spent_status == expected

        ki = ['2'*64, '1'*64]
        expected = [0, 0]
        res = daemon.is_key_image_spent(ki)
        assert res.spent_status == expected

    def check_multiple_submissions(self):
        daemon = Daemon()

        print('Testing multiple submissions')

        dst = {'address': '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 'amount': 1000000000000}

        self.wallet[0].refresh()
        res = self.wallet[0].get_balance()
        balance = res.balance

        res = self.wallet[0].transfer([dst], ring_size = 16, get_tx_key = False, get_tx_hex = False, get_tx_metadata = True)
        tx_hex = res.tx_metadata
        tx_fee = res.fee
        res = self.wallet[0].relay_tx(tx_hex)

        # submit again before mined
        res = self.wallet[0].relay_tx(tx_hex)
        daemon.generateblocks('44Kbx4sJ7JDRDV5aAhLJzQCjDz2ViLRduE3ijDZu3osWKBjMGkV1XPk4pfDUMqt1Aiezvephdqm6YD19GKFD9ZcXVUTp6BW', 1)

        self.wallet[0].refresh()
        res = self.wallet[0].get_balance()
        assert res.balance == balance - tx_fee

        balance = res.balance

        # submit again after mined
        res = self.wallet[0].relay_tx(tx_hex)
        daemon.generateblocks('44Kbx4sJ7JDRDV5aAhLJzQCjDz2ViLRduE3ijDZu3osWKBjMGkV1XPk4pfDUMqt1Aiezvephdqm6YD19GKFD9ZcXVUTp6BW', 1)

        self.wallet[0].refresh()
        res = self.wallet[0].get_balance()
        assert res.balance == balance

    def check_scan_tx(self):
        daemon = Daemon()

        print('Testing scan_tx')

        def diff_transfers(actual_transfers, expected_transfers):
            diff = DeepDiff(actual_transfers, expected_transfers)
            if diff != {}:
                pp.pprint(diff)
            assert diff == {}

        # set up sender_wallet
        sender_wallet = self.wallet[0]
        try: sender_wallet.close_wallet()
        except: pass
        sender_wallet.restore_deterministic_wallet(seed = seeds[0])
        sender_wallet.auto_refresh(enable = False)
        sender_wallet.refresh()
        res = sender_wallet.get_transfers()
        out_len = 0 if 'out' not in res else len(res.out)
        sender_starting_balance = sender_wallet.get_balance().balance
        amount = 1000000000000
        assert sender_starting_balance > amount

        # set up receiver_wallet
        receiver_wallet = self.wallet[1]
        try: receiver_wallet.close_wallet()
        except: pass
        receiver_wallet.restore_deterministic_wallet(seed = seeds[1])
        receiver_wallet.auto_refresh(enable = False)
        receiver_wallet.refresh()
        res = receiver_wallet.get_transfers()
        in_len = 0 if 'in' not in res else len(res['in'])
        receiver_starting_balance = receiver_wallet.get_balance().balance

        # transfer from sender_wallet to receiver_wallet
        dst = {'address': '44Kbx4sJ7JDRDV5aAhLJzQCjDz2ViLRduE3ijDZu3osWKBjMGkV1XPk4pfDUMqt1Aiezvephdqm6YD19GKFD9ZcXVUTp6BW', 'amount': amount}
        res = sender_wallet.transfer([dst])
        assert len(res.tx_hash) == 32*2
        txid = res.tx_hash
        assert res.amount == amount
        assert res.fee > 0
        fee = res.fee

        expected_sender_balance = sender_starting_balance - (amount + fee)
        expected_receiver_balance = receiver_starting_balance + amount

        test = 'Checking scan_tx on outgoing pool tx'
        for attempt in range(2): # test re-scanning
            print(test + ' (' + ('first attempt' if attempt == 0 else 're-scanning tx') + ')')
            sender_wallet.scan_tx([txid])
            res = sender_wallet.get_transfers()
            assert 'pool' not in res or len(res.pool) == 0
            if out_len == 0:
                assert 'out' not in res
            else:
                assert len(res.out) == out_len
            assert len(res.pending) == 1
            tx = [x for x in res.pending if x.txid == txid]
            assert len(tx) == 1
            tx = tx[0]
            assert tx.amount == amount
            assert tx.fee == fee
            assert len(tx.destinations) == 1
            assert tx.destinations[0].amount == amount
            assert tx.destinations[0].address == dst['address']
            assert sender_wallet.get_balance().balance == expected_sender_balance

        test = 'Checking scan_tx on incoming pool tx'
        for attempt in range(2): # test re-scanning
            print(test + ' (' + ('first attempt' if attempt == 0 else 're-scanning tx') + ')')
            receiver_wallet.scan_tx([txid])
            res = receiver_wallet.get_transfers()
            assert 'pending' not in res or len(res.pending) == 0
            if in_len == 0:
                assert 'in' not in res
            else:
                assert len(res['in']) == in_len
            assert 'pool' in res and len(res.pool) == 1
            tx = [x for x in res.pool if x.txid == txid]
            assert len(tx) == 1
            tx = tx[0]
            assert tx.amount == amount
            assert tx.fee == fee
            assert receiver_wallet.get_balance().balance == expected_receiver_balance

        # mine the tx
        height = daemon.generateblocks(dst['address'], 1).height
        block_header = daemon.getblockheaderbyheight(height = height).block_header
        miner_txid = block_header.miner_tx_hash
        expected_receiver_balance += block_header.reward

        print('Checking scan_tx on outgoing tx before refresh')
        sender_wallet.scan_tx([txid])
        res = sender_wallet.get_transfers()
        assert 'pending' not in res or len(res.pending) == 0
        assert 'pool' not in res or len (res.pool) == 0
        assert len(res.out) == out_len + 1
        tx = [x for x in res.out if x.txid == txid]
        assert len(tx) == 1
        tx = tx[0]
        assert tx.amount == amount
        assert tx.fee == fee
        assert len(tx.destinations) == 1
        assert tx.destinations[0].amount == amount
        assert tx.destinations[0].address == dst['address']
        assert sender_wallet.get_balance().balance == expected_sender_balance

        print('Checking scan_tx on outgoing tx after refresh')
        sender_wallet.refresh()
        sender_wallet.scan_tx([txid])
        diff_transfers(sender_wallet.get_transfers(), res)
        assert sender_wallet.get_balance().balance == expected_sender_balance

        print("Checking scan_tx on outgoing wallet's earliest tx")
        earliest_height = height
        earliest_txid = txid
        for x in res['in']:
            if x.height < earliest_height:
                earliest_height = x.height
                earliest_txid = x.txid
        sender_wallet.scan_tx([earliest_txid])
        diff_transfers(sender_wallet.get_transfers(), res)
        assert sender_wallet.get_balance().balance == expected_sender_balance

        test = 'Checking scan_tx on outgoing wallet restored at current height'
        for i, out_tx in enumerate(res.out):
            if 'destinations' in out_tx:
                del res.out[i]['destinations'] # destinations are not expected after wallet restore
        out_txids = [x.txid for x in res.out]
        in_txids = [x.txid for x in res['in']]
        all_txs = out_txids + in_txids
        for test_type in ["all txs", "incoming first", "duplicates within", "duplicates across"]:
            print(test + ' (' + test_type + ')')
            sender_wallet.close_wallet()
            sender_wallet.restore_deterministic_wallet(seed = seeds[0], restore_height = height)
            assert sender_wallet.get_transfers() == {}
            if test_type == "all txs":
                sender_wallet.scan_tx(all_txs)
            elif test_type == "incoming first":
                sender_wallet.scan_tx(in_txids)
                sender_wallet.scan_tx(out_txids)
            # TODO: test_type == "outgoing first"
            elif test_type == "duplicates within":
                sender_wallet.scan_tx(all_txs + all_txs)
            elif test_type == "duplicates across":
                sender_wallet.scan_tx(all_txs)
                sender_wallet.scan_tx(all_txs)
            else:
                assert True == False
            diff_transfers(sender_wallet.get_transfers(), res)
            assert sender_wallet.get_balance().balance == expected_sender_balance

        print('Sanity check against outgoing wallet restored at height 0')
        sender_wallet.close_wallet()
        sender_wallet.restore_deterministic_wallet(seed = seeds[0], restore_height = 0)
        sender_wallet.refresh()
        diff_transfers(sender_wallet.get_transfers(), res)
        assert sender_wallet.get_balance().balance == expected_sender_balance

        print('Checking scan_tx on incoming txs before refresh')
        receiver_wallet.scan_tx([txid, miner_txid])
        res = receiver_wallet.get_transfers()
        assert 'pending' not in res or len(res.pending) == 0
        assert 'pool' not in res or len (res.pool) == 0
        assert len(res['in']) == in_len + 2
        tx = [x for x in res['in'] if x.txid == txid]
        assert len(tx) == 1
        tx = tx[0]
        assert tx.amount == amount
        assert tx.fee == fee
        assert receiver_wallet.get_balance().balance == expected_receiver_balance

        print('Checking scan_tx on incoming txs after refresh')
        receiver_wallet.refresh()
        receiver_wallet.scan_tx([txid, miner_txid])
        diff_transfers(receiver_wallet.get_transfers(), res)
        assert receiver_wallet.get_balance().balance == expected_receiver_balance

        print("Checking scan_tx on incoming wallet's earliest tx")
        earliest_height = height
        earliest_txid = txid
        for x in res['in']:
            if x.height < earliest_height:
                earliest_height = x.height
                earliest_txid = x.txid
        receiver_wallet.scan_tx([earliest_txid])
        diff_transfers(receiver_wallet.get_transfers(), res)
        assert receiver_wallet.get_balance().balance == expected_receiver_balance

        print('Checking scan_tx on incoming wallet restored at current height')
        txids = [x.txid for x in res['in']]
        if 'out' in res:
            txids = txids + [x.txid for x in res.out]
        receiver_wallet.close_wallet()
        receiver_wallet.restore_deterministic_wallet(seed = seeds[1], restore_height = height)
        assert receiver_wallet.get_transfers() == {}
        receiver_wallet.scan_tx(txids)
        if 'out' in res:
            for i, out_tx in enumerate(res.out):
                if 'destinations' in out_tx:
                    del res.out[i]['destinations'] # destinations are not expected after wallet restore
        diff_transfers(receiver_wallet.get_transfers(), res)
        assert receiver_wallet.get_balance().balance == expected_receiver_balance

        print('Sanity check against incoming wallet restored at height 0')
        receiver_wallet.close_wallet()
        receiver_wallet.restore_deterministic_wallet(seed = seeds[1], restore_height = 0)
        receiver_wallet.refresh()
        diff_transfers(receiver_wallet.get_transfers(), res)
        assert receiver_wallet.get_balance().balance == expected_receiver_balance

if __name__ == '__main__':
    TransferTest().run_test()
