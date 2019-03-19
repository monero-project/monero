#!/usr/bin/env python3

# Copyright (c) 2019 The Monero Project
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

"""Test simple transfers
"""

from test_framework.daemon import Daemon
from test_framework.wallet import Wallet

class TransferTest():
    def run_test(self):
        self.create(0)
        self.mine()
        self.transfer()
        self.check_get_bulk_payments()

    def create(self, idx):
        print 'Creating wallet'
        wallet = Wallet()
        # close the wallet if any, will throw if none is loaded
        try: wallet.close_wallet()
        except: pass
        seeds = [
          'velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted',
          'peeled mixture ionic radar utopia puddle buying illness nuns gadget river spout cavernous bounced paradise drunk looking cottage jump tequila melting went winter adjust spout',
          'dilute gutter certain antics pamphlet macro enjoy left slid guarded bogeys upload nineteen bomb jubilee enhanced irritate turnip eggs swung jukebox loudly reduce sedan slid',
        ]
        res = wallet.restore_deterministic_wallet(seed = seeds[idx])

    def mine(self):
        print("Mining some blocks")
        daemon = Daemon()
        wallet = Wallet()

        daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 80)
        wallet.refresh()

    def transfer(self):
        daemon = Daemon()
        wallet = Wallet()

        print("Creating transfer to self")

        dst = {'address': '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 'amount': 1000000000000}

        payment_id = '1234500000012345abcde00000abcdeff1234500000012345abcde00000abcde'

        res = wallet.transfer([dst], ring_size = 11, payment_id = payment_id, get_tx_key = False)
        assert len(res.tx_hash) == 32*2
        txid = res.tx_hash
        assert len(res.tx_key) == 0
        assert res.amount > 0
        amount = res.amount
        assert res.fee > 0
        fee = res.fee
        assert len(res.tx_blob) == 0
        assert len(res.tx_metadata) == 0
        assert len(res.multisig_txset) == 0
        assert len(res.unsigned_txset) == 0
        unsigned_txset = res.unsigned_txset

        wallet.refresh()

        res = daemon.get_info()
        height = res.height

        res = wallet.get_transfers()
        assert len(res['in']) == height - 1 # coinbases
        assert not 'out' in res or len(res.out) == 0 # not mined yet
        assert len(res.pending) == 1
        assert not 'pool' in res or len(res.pool) == 0
        assert not 'failed' in res or len(res.failed) == 0
        for e in res['in']:
          assert e.type == 'block'
        e = res.pending[0]
        assert e.txid == txid
        assert e.payment_id == payment_id
        assert e.type == 'pending'
        assert e.unlock_time == 0
        assert e.subaddr_index.major == 0
        assert e.subaddr_indices == [{'major': 0, 'minor': 0}]
        assert e.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
        assert e.double_spend_seen == False
        assert e.confirmations == 0

        daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 1)
        wallet.refresh()

        res = wallet.get_transfers()
        assert len(res['in']) == height # coinbases
        assert len(res.out) == 1 # not mined yet
        assert not 'pending' in res or len(res.pending) == 0
        assert not 'pool' in res or len(res.pool) == 0
        assert not 'failed' in res or len(res.failed) == 0
        for e in res['in']:
          assert e.type == 'block'
        e = res.out[0]
        assert e.txid == txid
        assert e.payment_id == payment_id
        assert e.type == 'out'
        assert e.unlock_time == 0
        assert e.subaddr_index.major == 0
        assert e.subaddr_indices == [{'major': 0, 'minor': 0}]
        assert e.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
        assert e.double_spend_seen == False
        assert e.confirmations == 1

        print("Creating transfer to another")

        dst = {'address': '44Kbx4sJ7JDRDV5aAhLJzQCjDz2ViLRduE3ijDZu3osWKBjMGkV1XPk4pfDUMqt1Aiezvephdqm6YD19GKFD9ZcXVUTp6BW', 'amount': 1000000000000}
        res = wallet.transfer([dst], ring_size = 11, payment_id = payment_id, get_tx_key = True)
        assert len(res.tx_hash) == 32*2
        txid = res.tx_hash
        assert len(res.tx_key) == 32*2
        assert res.amount == 1000000000000
        amount = res.amount
        assert res.fee > 0
        fee = res.fee
        assert len(res.tx_blob) == 0
        assert len(res.tx_metadata) == 0
        assert len(res.multisig_txset) == 0
        assert len(res.unsigned_txset) == 0
        unsigned_txset = res.unsigned_txset

        self.create(1)
        wallet.refresh()

        res = wallet.get_transfers()
        assert not 'in' in res or len(res['in']) == 0
        assert not 'out' in res or len(res.out) == 0
        assert not 'pending' in res or len(res.pending) == 0
        assert len(res.pool) == 1
        assert not 'failed' in res or len(res.failed) == 0
        e = res.pool[0]
        assert e.txid == txid
        assert e.payment_id == payment_id
        assert e.type == 'pool'
        assert e.unlock_time == 0
        assert e.subaddr_index.major == 0
        assert e.subaddr_indices == [{'major': 0, 'minor': 0}]
        assert e.address == '44Kbx4sJ7JDRDV5aAhLJzQCjDz2ViLRduE3ijDZu3osWKBjMGkV1XPk4pfDUMqt1Aiezvephdqm6YD19GKFD9ZcXVUTp6BW'
        assert e.double_spend_seen == False
        assert e.confirmations == 0
        assert e.amount == amount
        assert e.fee == fee

        daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 1)
        wallet.refresh()

        res = wallet.get_transfers()
        assert len(res['in']) == 1
        assert not 'out' in res or len(res.out) == 0
        assert not 'pending' in res or len(res.pending) == 0
        assert not 'pool' in res or len(res.pool) == 0
        assert not 'failed' in res or len(res.failed) == 0
        e = res['in'][0]
        assert e.txid == txid
        assert e.payment_id == payment_id
        assert e.type == 'in'
        assert e.unlock_time == 0
        assert e.subaddr_index.major == 0
        assert e.subaddr_indices == [{'major': 0, 'minor': 0}]
        assert e.address == '44Kbx4sJ7JDRDV5aAhLJzQCjDz2ViLRduE3ijDZu3osWKBjMGkV1XPk4pfDUMqt1Aiezvephdqm6YD19GKFD9ZcXVUTp6BW'
        assert e.double_spend_seen == False
        assert e.confirmations == 1
        assert e.amount == amount
        assert e.fee == fee

        print 'Creating multi out transfer'

        self.create(0)
        wallet.refresh()

        dst0 = {'address': '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 'amount': 1000000000000}
        dst1 = {'address': '44Kbx4sJ7JDRDV5aAhLJzQCjDz2ViLRduE3ijDZu3osWKBjMGkV1XPk4pfDUMqt1Aiezvephdqm6YD19GKFD9ZcXVUTp6BW', 'amount': 1100000000000}
        dst2 = {'address': '46r4nYSevkfBUMhuykdK3gQ98XDqDTYW1hNLaXNvjpsJaSbNtdXh1sKMsdVgqkaihChAzEy29zEDPMR3NHQvGoZCLGwTerK', 'amount': 1200000000000}
        res = wallet.transfer([dst0, dst1, dst2], ring_size = 11, payment_id = payment_id, get_tx_key = True)
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

        daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 1)
        wallet.refresh()

        res = wallet.get_transfers()
        assert len(res['in']) == height + 2
        assert len(res.out) == 3
        assert not 'pending' in res or len(res.pending) == 0
        assert not 'pool' in res or len(res.pool) == 1
        assert not 'failed' in res or len(res.failed) == 0
        e = [o for o in res.out if o.txid == txid]
        assert len(e) == 1
        e = e[0]
        assert e.txid == txid
        assert e.payment_id == payment_id
        assert e.type == 'out'
        assert e.unlock_time == 0
        assert e.subaddr_index.major == 0
        assert e.subaddr_indices == [{'major': 0, 'minor': 0}]
        assert e.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
        assert e.double_spend_seen == False
        assert e.confirmations == 1

        assert e.amount == amount
        assert e.fee == fee

        self.create(1)
        wallet.refresh()
        res = wallet.get_transfers()
        assert len(res['in']) == 2
        assert not 'out' in res or len(res.out) == 0
        assert not 'pending' in res or len(res.pending) == 0
        assert not 'pool' in res or len(res.pool) == 0
        assert not 'failed' in res or len(res.failed) == 0
        e = [o for o in res['in'] if o.txid == txid]
        assert len(e) == 1
        e = e[0]
        assert e.txid == txid
        assert e.payment_id == payment_id
        assert e.type == 'in'
        assert e.unlock_time == 0
        assert e.subaddr_index.major == 0
        assert e.subaddr_indices == [{'major': 0, 'minor': 0}]
        assert e.address == '44Kbx4sJ7JDRDV5aAhLJzQCjDz2ViLRduE3ijDZu3osWKBjMGkV1XPk4pfDUMqt1Aiezvephdqm6YD19GKFD9ZcXVUTp6BW'
        assert e.double_spend_seen == False
        assert e.confirmations == 1
        assert e.amount == 1100000000000
        assert e.fee == fee

        self.create(2)
        wallet.refresh()
        res = wallet.get_transfers()
        assert len(res['in']) == 1
        assert not 'out' in res or len(res.out) == 0
        assert not 'pending' in res or len(res.pending) == 0
        assert not 'pool' in res or len(res.pool) == 0
        assert not 'failed' in res or len(res.failed) == 0
        e = [o for o in res['in'] if o.txid == txid]
        assert len(e) == 1
        e = e[0]
        assert e.txid == txid
        assert e.payment_id == payment_id
        assert e.type == 'in'
        assert e.unlock_time == 0
        assert e.subaddr_index.major == 0
        assert e.subaddr_indices == [{'major': 0, 'minor': 0}]
        assert e.address == '46r4nYSevkfBUMhuykdK3gQ98XDqDTYW1hNLaXNvjpsJaSbNtdXh1sKMsdVgqkaihChAzEy29zEDPMR3NHQvGoZCLGwTerK'
        assert e.double_spend_seen == False
        assert e.confirmations == 1
        assert e.amount == 1200000000000
        assert e.fee == fee

    def check_get_bulk_payments(self):
        print('Checking get_bulk_payments')

        daemon = Daemon()
        res = daemon.get_info()
        height = res.height

        wallet = Wallet()

        self.create(0)
        wallet.refresh()
        res = wallet.get_bulk_payments()
        assert len(res.payments) >= 83 # at least 83 coinbases
        res = wallet.get_bulk_payments(payment_ids = ['1234500000012345abcde00000abcdeff1234500000012345abcde00000abcde'])
        assert 'payments' not in res or len(res.payments) == 0
        res = wallet.get_bulk_payments(min_block_height = height)
        assert 'payments' not in res or len(res.payments) == 0
        res = wallet.get_bulk_payments(min_block_height = height - 40)
        assert len(res.payments) >= 39 # coinbases

        self.create(1)
        wallet.refresh()
        res = wallet.get_bulk_payments()
        assert len(res.payments) >= 2 # two txes were sent
        res = wallet.get_bulk_payments(payment_ids = ['1234500000012345abcde00000abcdeff1234500000012345abcde00000abcde'])
        assert len(res.payments) >= 2 # two txes were sent with that payment id
        res = wallet.get_bulk_payments(payment_ids = ['ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff'])
        assert 'payments' not in res or len(res.payments) == 0 # none with that payment id

        self.create(2)
        wallet.refresh()
        res = wallet.get_bulk_payments()
        assert len(res.payments) >= 1 # one tx was sent
        res = wallet.get_bulk_payments(payment_ids = ['1'*64, '1234500000012345abcde00000abcdeff1234500000012345abcde00000abcde', '2'*64])
        assert len(res.payments) >= 1 # one tx was sent

if __name__ == '__main__':
    TransferTest().run_test()
