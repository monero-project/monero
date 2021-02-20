#!/usr/bin/env python3

# Copyright (c) 2019-2020 The Monero Project
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

"""Test misc proofs (tx key, send, receive, reserve)
"""

from framework.daemon import Daemon
from framework.wallet import Wallet

TRIPTYCH_RING_SIZE = 128

class ProofsTest():
    def run_test(self):
        self.reset()
        self.create_wallets()
        self.mine('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 80 + TRIPTYCH_RING_SIZE * 2)
        txid, tx_key, amount, fee, height = self.transfer()
        self.check_tx_key(txid, tx_key, amount)
        self.check_tx_proof(txid, amount)
        self.check_spend_proof(txid)
        self.check_reserve_proof()
        self.check_historical_reserve_proof(height, amount + fee)

    def reset(self):
        print('Resetting blockchain')
        daemon = Daemon()
        res = daemon.get_height()
        daemon.pop_blocks(res.height - 1)
        daemon.flush_txpool()

    def mine(self, address, blocks):
        print("Mining some blocks")
        daemon = Daemon()
        self.miner_balances = []
        for i in range(blocks):
            daemon.generateblocks(address, 1)
            self.wallet[0].refresh()
            res = self.wallet[0].get_balance()
            self.miner_balances.append(res.balance)

    def transfer(self):
        print('Creating transaction')
        self.wallet[0].refresh()
        dst = {'address': '44Kbx4sJ7JDRDV5aAhLJzQCjDz2ViLRduE3ijDZu3osWKBjMGkV1XPk4pfDUMqt1Aiezvephdqm6YD19GKFD9ZcXVUTp6BW', 'amount':123456789000}
        res = self.wallet[0].transfer([dst], get_tx_key = True)
        assert len(res.tx_hash) == 64
        tx_hash = res.tx_hash
        assert len(res.tx_key) == 64
        tx_key = res.tx_key
        assert res.amount == 123456789000
        amount = res.amount
        assert res.fee > 0
        fee = res.fee
        daemon = Daemon()
        res = daemon.get_info()
        height = res.height
        daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 1)
        return (tx_hash, tx_key, amount, fee, height)

    def create_wallets(self):
      print('Creating wallets')
      seeds = [
        'velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted',
        'peeled mixture ionic radar utopia puddle buying illness nuns gadget river spout cavernous bounced paradise drunk looking cottage jump tequila melting went winter adjust spout',
      ]
      self.wallet = [None, None]
      for i in range(2):
        self.wallet[i] = Wallet(idx = i)
        try: self.wallet[i].close_wallet()
        except: pass
        res = self.wallet[i].restore_deterministic_wallet(seed = seeds[i])

    def check_tx_key(self, txid, tx_key, amount):
        daemon = Daemon()

        print('Checking tx key')

        self.wallet[0].refresh()
        self.wallet[1].refresh()

        sending_address = '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
        receiving_address = '44Kbx4sJ7JDRDV5aAhLJzQCjDz2ViLRduE3ijDZu3osWKBjMGkV1XPk4pfDUMqt1Aiezvephdqm6YD19GKFD9ZcXVUTp6BW'
        res = self.wallet[0].get_tx_key(txid)
        assert res.tx_key == tx_key
        res = self.wallet[0].check_tx_key(txid = txid, tx_key = tx_key, address = receiving_address)
        assert res.received == amount
        assert not res.in_pool
        assert res.confirmations == 1
        res = self.wallet[1].check_tx_key(txid = txid, tx_key = tx_key, address = receiving_address)
        assert res.received == amount
        assert not res.in_pool
        assert res.confirmations == 1

        self.wallet[1].check_tx_key(txid = txid, tx_key = tx_key, address = sending_address)
        assert res.received >= 0 # might be change
        assert not res.in_pool
        assert res.confirmations == 1

        ok = False
        try: self.wallet[1].check_tx_key(txid = '0' * 64, tx_key = tx_key, address = receiving_address)
        except: ok = True
        assert ok

        res = self.wallet[1].check_tx_key(txid = txid, tx_key = '0' * 64, address = receiving_address)
        assert res.received == 0
        assert not res.in_pool
        assert res.confirmations == 1

    def check_tx_proof(self, txid, amount):
        daemon = Daemon()

        print('Checking tx proof')

        self.wallet[0].refresh()
        self.wallet[1].refresh()

        sending_address = '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
        receiving_address = '44Kbx4sJ7JDRDV5aAhLJzQCjDz2ViLRduE3ijDZu3osWKBjMGkV1XPk4pfDUMqt1Aiezvephdqm6YD19GKFD9ZcXVUTp6BW'
        res = self.wallet[0].get_tx_proof(txid, sending_address, 'foo');
        assert res.signature.startswith('InProofV2');
        signature0i = res.signature
        res = self.wallet[0].get_tx_proof(txid, receiving_address, 'bar');
        assert res.signature.startswith('OutProofV2');
        signature0o = res.signature
        res = self.wallet[1].get_tx_proof(txid, receiving_address, 'baz');
        assert res.signature.startswith('InProofV2');
        signature1 = res.signature

        res = self.wallet[0].check_tx_proof(txid, sending_address, 'foo', signature0i);
        assert res.good
        assert res.received > 0 # likely change
        assert not res.in_pool
        assert res.confirmations == 1

        ok = False
        try: res = self.wallet[0].check_tx_proof('0' * 64, sending_address, 'foo', signature0i);
        except: ok = True
        assert ok or not res.good

        ok = False
        try: res = self.wallet[0].check_tx_proof(txid, receiving_address, 'foo', signature0i);
        except: ok = True
        assert ok or not res.good

        ok = False
        try: res = self.wallet[0].check_tx_proof(txid, sending_address, '', signature0i);
        except: ok = True
        assert ok or not res.good

        ok = False
        try: res = self.wallet[0].check_tx_proof(txid, sending_address, 'foo', signature1);
        except: ok = True
        assert ok or not res.good


        res = self.wallet[0].check_tx_proof(txid, receiving_address, 'bar', signature0o);
        assert res.good
        assert res.received == amount
        assert not res.in_pool
        assert res.confirmations == 1

        ok = False
        try: res = self.wallet[0].check_tx_proof('0' * 64, receiving_address, 'bar', signature0o);
        except: ok = True
        assert ok or not res.good

        ok = False
        try: res = self.wallet[0].check_tx_proof(txid, sending_address, 'bar', signature0o);
        except: ok = True
        assert ok or not res.good

        ok = False
        try: res = self.wallet[0].check_tx_proof(txid, receiving_address, '', signature0o);
        except: ok = True
        assert ok or not res.good

        ok = False
        try: res = self.wallet[0].check_tx_proof(txid, receiving_address, 'bar', signature0i);
        except: ok = True
        assert ok or not res.good


        res = self.wallet[1].check_tx_proof(txid, receiving_address, 'baz', signature1);
        assert res.good
        assert res.received == amount
        assert not res.in_pool
        assert res.confirmations == 1

        ok = False
        try: res = self.wallet[1].check_tx_proof('0' * 64, receiving_address, 'baz', signature1);
        except: ok = True
        assert ok or not res.good

        ok = False
        try: res = self.wallet[1].check_tx_proof(txid, sending_address, 'baz', signature1);
        except: ok = True
        assert ok or not res.good

        ok = False
        try: res = self.wallet[1].check_tx_proof(txid, receiving_address, '', signature1);
        except: ok = True
        assert ok or not res.good

        ok = False
        try: res = self.wallet[1].check_tx_proof(txid, receiving_address, 'baz', signature0o);
        except: ok = True
        assert ok or not res.good

        
        # Test bad cross-version verification
        ok = False
        try: res = self.wallet[0].check_tx_proof(txid, sending_address, 'foo', signature0i.replace('ProofV2','ProofV1'));
        except: ok = True
        assert ok or not res.good

        ok = False
        try: res = self.wallet[0].check_tx_proof(txid, receiving_address, 'bar', signature0o.replace('ProofV2','ProofV1'));
        except: ok = True
        assert ok or not res.good

        ok = False
        try: res = self.wallet[1].check_tx_proof(txid, receiving_address, 'baz', signature1.replace('ProofV2','ProofV1'));
        except: ok = True
        assert ok or not res.good

    def check_spend_proof(self, txid):
        daemon = Daemon()

        print('Checking spend proof')

        self.wallet[0].refresh()
        self.wallet[1].refresh()

        res = self.wallet[0].get_spend_proof(txid, message = 'foo')
        assert len(res.signature) > 0
        signature = res.signature
        res = self.wallet[1].check_spend_proof(txid, message = 'foo', signature = signature)
        assert res.good

        res = self.wallet[0].get_spend_proof(txid, message = 'foobar')
        assert len(res.signature) > 0
        signature2 = res.signature
        res = self.wallet[1].check_spend_proof(txid, message = 'foobar', signature = signature2)
        assert res.good

        ok = False
        try: res = self.wallet[1].check_spend_proof('0' * 64, message = 'foo', signature = signature)
        except: ok = True
        assert ok or not res.good

        ok = False
        try: res = self.wallet[1].check_spend_proof(txid, message = 'bar', signature = signature)
        except: ok = True
        assert ok or not res.good

        ok = False
        try: res = self.wallet[1].check_spend_proof(txid, message = 'foo', signature = signature2)
        except: ok = True
        assert ok or not res.good

    def check_reserve_proof(self):
        daemon = Daemon()

        print('Checking reserve proof')

        address0 = '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
        address1 = '44Kbx4sJ7JDRDV5aAhLJzQCjDz2ViLRduE3ijDZu3osWKBjMGkV1XPk4pfDUMqt1Aiezvephdqm6YD19GKFD9ZcXVUTp6BW'

        self.wallet[0].refresh()
        res = self.wallet[0].get_balance()
        balance0 = res.balance
        self.wallet[1].refresh()
        res = self.wallet[1].get_balance()
        balance1 = res.balance

        res = self.wallet[0].get_reserve_proof(all_ = True, message = 'foo')
        assert res.signature.startswith('ReserveProofV3')
        signature = res.signature
        for i in range(2):
          self.wallet[i].get_bulk_payments()
          res = self.wallet[i].check_reserve_proof(address = address0, message = 'foo', signature = signature)
          assert res.good
          assert res.total == balance0

          ok = False
          try: res = self.wallet[i].check_reserve_proof(address = address0, message = 'bar', signature = signature)
          except: ok = True
          assert ok or not res.good

          ok = False
          try: res = self.wallet[i].check_reserve_proof(address = address1, message = 'foo', signature = signature)
          except: ok = True
          assert ok or not res.good

          # Test bad cross-version verification
          ok = False
          try: res = self.wallet[i].check_reserve_proof(address = address0, message = 'foo', signature = signature.replace('ProofV3','ProofV1'))
          except: ok = True
          assert ok or not res.good

        amount = int(balance0 / 10)
        res = self.wallet[0].get_reserve_proof(all_ = False, amount = amount, message = 'foo')
        assert res.signature.startswith('ReserveProofV3')
        signature = res.signature
        for i in range(2):
          res = self.wallet[i].check_reserve_proof(address = address0, message = 'foo', signature = signature)
          assert res.good
          assert res.total >= amount and res.total <= balance0

          ok = False
          try: res = self.wallet[i].check_reserve_proof(address = address0, message = 'bar', signature = signature)
          except: ok = True
          assert ok or not res.good

          ok = False
          try: res = self.wallet[i].check_reserve_proof(address = address1, message = 'foo', signature = signature)
          except: ok = True
          assert ok or not res.good

          # Test bad cross-version verification
          ok = False
          try: res = self.wallet[i].check_reserve_proof(address = address0, message = 'foo', signature = signature.replace('ProofV3','ProofV1'))
          except: ok = True
          assert ok or not res.good

        ok = False
        try: self.wallet[0].get_reserve_proof(all_ = False, amount = balance0 + 1, message = 'foo')
        except: ok = True
        assert ok

    def check_historical_reserve_proof(self, height, paid):
        daemon = Daemon()

        print('Checking historical reserve proof')

        address0 = '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'

        self.wallet[0].refresh()

        res = daemon.get_info()
        bc_height = res.height
        assert bc_height > height
        daemon.generateblocks(address0, 1)
        bc_height += 1
        self.wallet[0].refresh()

        res = daemon.get_block_headers_range(0, bc_height - 1)
        headers = res.headers
        assert len(headers) == bc_height
        expected_balance = 0
        for h in range(1, bc_height):
            expected_balance += headers[h].reward
            if h == height:
                expected_balance -= paid
            if h in [1, 2, 3, bc_height // 4, bc_height // 2, bc_height * 3 // 4, bc_height - 3, bc_height - 2, bc_height - 1]:
                res = self.wallet[0].get_reserve_proof(all_ = True, blockchain_height = h + 1, message = 'foo')
                signature = res.signature
                res = self.wallet[1].check_reserve_proof(address = address0, blockchain_height = h + 1, message = 'foo', signature = signature)
                assert res.total - res.spent == expected_balance
        res = self.wallet[1].check_reserve_proof(address = address0, blockchain_height = 1, message = 'foo', signature = signature)
        assert res.total - res.spent == 0


class Guard:
    def __enter__(self):
        for i in range(4):
            Wallet(idx = i).auto_refresh(False)
    def __exit__(self, exc_type, exc_value, traceback):
        for i in range(4):
            Wallet(idx = i).auto_refresh(True)

if __name__ == '__main__':
    with Guard() as guard:
        ProofsTest().run_test()
