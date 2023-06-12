#!/usr/bin/env python3

# Copyright (c) 2019-2022, The Monero Project
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

"""Test multisig transfers
"""

from framework.daemon import Daemon
from framework.wallet import Wallet

class MultisigTest():
    def run_test(self):
        self.reset()
        self.mine('45J58b7PmKJFSiNPFFrTdtfMcFGnruP7V4CMuRpX7NsH4j3jGHKAjo3YJP2RePX6HMaSkbvTbrWUFhDNcNcHgtNmQ3gr7sG', 5)
        self.mine('44G2TQNfsiURKkvxp7gbgaJY8WynZvANnhmyMAwv6WeEbAvyAWMfKXRhh3uBXT2UAKhAsUJ7Fg5zjjF2U1iGciFk5duN94i', 5)
        self.mine('41mro238grj56GnrWkakAKTkBy2yDcXYsUZ2iXCM9pe5Ueajd2RRc6Fhh3uBXT2UAKhAsUJ7Fg5zjjF2U1iGciFk5ief4ZP', 5)
        self.mine('44vZSprQKJQRFe6t1VHgU4ESvq2dv7TjBLVGE7QscKxMdFSiyyPCEV64NnKUQssFPyWxc2meyt7j63F2S2qtCTRL6dakeff', 5)
        self.mine('47puypSwsV1gvUDratmX4y58fSwikXVehEiBhVLxJA1gRCxHyrRgTDr4NnKUQssFPyWxc2meyt7j63F2S2qtCTRL6aRPj5U', 5)
        self.mine('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 80)

        self.test_states()

        self.create_multisig_wallets(2, 2, '45J58b7PmKJFSiNPFFrTdtfMcFGnruP7V4CMuRpX7NsH4j3jGHKAjo3YJP2RePX6HMaSkbvTbrWUFhDNcNcHgtNmQ3gr7sG')
        self.import_multisig_info([1, 0], 5)
        txid = self.transfer([1, 0])
        self.import_multisig_info([0, 1], 6)
        self.check_transaction(txid)

        self.create_multisig_wallets(2, 3, '44G2TQNfsiURKkvxp7gbgaJY8WynZvANnhmyMAwv6WeEbAvyAWMfKXRhh3uBXT2UAKhAsUJ7Fg5zjjF2U1iGciFk5duN94i')
        self.import_multisig_info([0, 2], 5)
        txid = self.transfer([0, 2])
        self.import_multisig_info([0, 1, 2], 6)
        self.check_transaction(txid)

        self.create_multisig_wallets(3, 3, '41mro238grj56GnrWkakAKTkBy2yDcXYsUZ2iXCM9pe5Ueajd2RRc6Fhh3uBXT2UAKhAsUJ7Fg5zjjF2U1iGciFk5ief4ZP')
        self.import_multisig_info([2, 0, 1], 5)
        txid = self.transfer([2, 1, 0])
        self.import_multisig_info([0, 2, 1], 6)
        self.check_transaction(txid)

        self.create_multisig_wallets(3, 4, '44vZSprQKJQRFe6t1VHgU4ESvq2dv7TjBLVGE7QscKxMdFSiyyPCEV64NnKUQssFPyWxc2meyt7j63F2S2qtCTRL6dakeff')
        self.import_multisig_info([0, 2, 3], 5)
        txid = self.transfer([0, 2, 3])
        self.import_multisig_info([0, 1, 2, 3], 6)
        self.check_transaction(txid)

        self.create_multisig_wallets(2, 4, '47puypSwsV1gvUDratmX4y58fSwikXVehEiBhVLxJA1gRCxHyrRgTDr4NnKUQssFPyWxc2meyt7j63F2S2qtCTRL6aRPj5U')
        self.import_multisig_info([1, 2], 5)
        txid = self.transfer([1, 2])
        self.import_multisig_info([0, 1, 2, 3], 6)
        self.check_transaction(txid)
        txid = self.try_transfer_frozen([2, 3])
        self.import_multisig_info([0, 1, 2, 3], 7)
        self.check_transaction(txid)

    def reset(self):
        print('Resetting blockchain')
        daemon = Daemon()
        res = daemon.get_height()
        daemon.pop_blocks(res.height - 1)
        daemon.flush_txpool()

    def mine(self, address, blocks):
        print("Mining some blocks")
        daemon = Daemon()
        daemon.generateblocks(address, blocks)

    def create_multisig_wallets(self, M_threshold, N_total, expected_address):
      print('Creating ' + str(M_threshold) + '/' + str(N_total) + ' multisig wallet')
      seeds = [
        'velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted',
        'peeled mixture ionic radar utopia puddle buying illness nuns gadget river spout cavernous bounced paradise drunk looking cottage jump tequila melting went winter adjust spout',
        'dilute gutter certain antics pamphlet macro enjoy left slid guarded bogeys upload nineteen bomb jubilee enhanced irritate turnip eggs swung jukebox loudly reduce sedan slid',
        'waking gown buffet negative reorder speedy baffles hotel pliers dewdrop actress diplomat lymph emit ajar mailed kennel cynical jaunt justice weavers height teardrop toyed lymph',
      ]
      assert M_threshold <= N_total
      assert N_total <= len(seeds)
      self.wallet = [None] * N_total
      info = []
      for i in range(N_total):
        self.wallet[i] = Wallet(idx = i)
        try: self.wallet[i].close_wallet()
        except: pass
        res = self.wallet[i].restore_deterministic_wallet(seed = seeds[i])
        res = self.wallet[i].prepare_multisig(enable_multisig_experimental = True)
        assert len(res.multisig_info) > 0
        info.append(res.multisig_info)

      for i in range(N_total):
        res = self.wallet[i].is_multisig()
        assert res.multisig == False

      addresses = []
      next_stage = []
      for i in range(N_total):
        res = self.wallet[i].make_multisig(info, M_threshold)
        addresses.append(res.address)
        next_stage.append(res.multisig_info)

      for i in range(N_total):
        res = self.wallet[i].is_multisig()
        assert res.multisig == True
        assert not res.ready
        assert res.threshold == M_threshold
        assert res.total == N_total

      while True:
        n_ready = 0
        for i in range(N_total):
          res = self.wallet[i].is_multisig()
          if res.ready == True:
            n_ready += 1
        assert n_ready == 0 or n_ready == N_total
        if n_ready == N_total:
          break
        info = next_stage
        next_stage = []
        addresses = []
        for i in range(N_total):
          res = self.wallet[i].exchange_multisig_keys(info)
          next_stage.append(res.multisig_info)
          addresses.append(res.address)
      for i in range(N_total):
        assert addresses[i] == expected_address
      self.wallet_address = expected_address

      for i in range(N_total):
        res = self.wallet[i].is_multisig()
        assert res.multisig == True
        assert res.ready == True
        assert res.threshold == M_threshold
        assert res.total == N_total

    def test_states(self):
        print('Testing multisig states')
        seeds = [
            'velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted',
            'peeled mixture ionic radar utopia puddle buying illness nuns gadget river spout cavernous bounced paradise drunk looking cottage jump tequila melting went winter adjust spout',
            'dilute gutter certain antics pamphlet macro enjoy left slid guarded bogeys upload nineteen bomb jubilee enhanced irritate turnip eggs swung jukebox loudly reduce sedan slid',
        ]
        info2of2 = []
        wallet2of2 = [None, None]
        for i in range(2):
            wallet2of2[i] = Wallet(idx = i)
            try: wallet2of2[i].close_wallet()
            except: pass
            res = wallet2of2[i].restore_deterministic_wallet(seed = seeds[i])
            res = wallet2of2[i].is_multisig()
            assert not res.multisig
            res = wallet2of2[i].prepare_multisig(enable_multisig_experimental = True)
            assert len(res.multisig_info) > 0
            info2of2.append(res.multisig_info)

        kex_info = []
        res = wallet2of2[0].make_multisig(info2of2, 2)
        kex_info.append(res.multisig_info)
        res = wallet2of2[1].make_multisig(info2of2, 2)
        kex_info.append(res.multisig_info)
        res = wallet2of2[0].exchange_multisig_keys(kex_info)
        res = wallet2of2[0].is_multisig()
        assert res.multisig
        assert res.ready

        ok = False
        try: res = wallet2of2[0].prepare_multisig(enable_multisig_experimental = True)
        except: ok = True
        assert ok

        ok = False
        try: res = wallet2of2[0].make_multisig(info2of2, 2)
        except: ok = True
        assert ok

        info2of3 = []
        wallet2of3 = [None, None, None]
        for i in range(3):
            wallet2of3[i] = Wallet(idx = i)
            try: wallet2of3[i].close_wallet()
            except: pass
            res = wallet2of3[i].restore_deterministic_wallet(seed = seeds[i])
            res = wallet2of3[i].is_multisig()
            assert not res.multisig
            res = wallet2of3[i].prepare_multisig(enable_multisig_experimental = True)
            assert len(res.multisig_info) > 0
            info2of3.append(res.multisig_info)

        for i in range(3):
            ok = False
            try: res = wallet2of3[i].exchange_multisig_keys(info)
            except: ok = True
            assert ok
            res = wallet2of3[i].is_multisig()
            assert not res.multisig

        res = wallet2of3[1].make_multisig(info2of3, 2)
        res = wallet2of3[1].is_multisig()
        assert res.multisig
        assert not res.ready

        ok = False
        try: res = wallet2of3[1].prepare_multisig(enable_multisig_experimental = True)
        except: ok = True
        assert ok

        ok = False
        try: res = wallet2of3[1].make_multisig(info2of3[0:2], 2)
        except: ok = True
        assert ok

    def import_multisig_info(self, signers, expected_outputs):
        assert len(signers) >= 2

        print('Importing multisig info from ' + str(signers))

        info = []
        for i in signers:
          self.wallet[i].refresh()
          res = self.wallet[i].export_multisig_info()
          assert len(res.info) > 0
          info.append(res.info)
        for i in signers:
          res = self.wallet[i].import_multisig_info(info)
          assert res.n_outputs == expected_outputs

    def transfer(self, signers):
        assert len(signers) >= 2

        daemon = Daemon()

        print("Creating multisig transaction from wallet " + str(signers[0]))

        dst = {'address': '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 'amount': 1000000000000}
        res = self.wallet[signers[0]].transfer([dst])
        assert len(res.tx_hash) == 0 # not known yet
        txid = res.tx_hash
        assert len(res.tx_key) == 32*2
        assert res.amount > 0
        amount = res.amount
        assert res.fee > 0
        fee = res.fee
        assert len(res.tx_blob) == 0
        assert len(res.tx_metadata) == 0
        assert len(res.multisig_txset) > 0
        assert len(res.unsigned_txset) == 0
        multisig_txset = res.multisig_txset

        daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 1)
        for i in range(len(self.wallet)):
          self.wallet[i].refresh()

        for i in range(len(signers[1:])):
          print('Signing multisig transaction with wallet ' + str(signers[i+1]))
          res = self.wallet[signers[i+1]].describe_transfer(multisig_txset = multisig_txset)
          assert len(res.desc) == 1
          desc = res.desc[0]
          assert desc.amount_in >= amount + fee
          assert desc.amount_out == desc.amount_in - fee
          assert desc.ring_size == 16
          assert desc.unlock_time == 0
          assert not 'payment_id' in desc or desc.payment_id in ['', '0000000000000000']
          assert desc.change_amount == desc.amount_in - 1000000000000 - fee
          assert desc.change_address == self.wallet_address
          assert desc.fee == fee
          assert len(desc.recipients) == 1
          rec = desc.recipients[0]
          assert rec.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
          assert rec.amount == 1000000000000

          res = self.wallet[signers[i+1]].sign_multisig(multisig_txset)
          multisig_txset = res.tx_data_hex
          assert len(res.tx_hash_list if 'tx_hash_list' in res else []) == (i == len(signers[1:]) - 1)

          if i < len(signers[1:]) - 1:
            print('Submitting multisig transaction prematurely with wallet ' + str(signers[-1]))
            ok = False
            try: self.wallet[signers[-1]].submit_multisig(multisig_txset)
            except: ok = True
            assert ok

        print('Submitting multisig transaction with wallet ' + str(signers[-1]))
        res = self.wallet[signers[-1]].submit_multisig(multisig_txset)
        assert len(res.tx_hash_list) == 1
        txid = res.tx_hash_list[0]

        for i in range(len(self.wallet)):
          self.wallet[i].refresh()
          res = self.wallet[i].get_transfers()
          assert len([x for x in (res['pending'] if 'pending' in res else []) if x.txid == txid]) == (1 if i == signers[-1] else 0)
          assert len([x for x in (res['out'] if 'out' in res else []) if x.txid == txid]) == 0

        daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 1)
        return txid

    def try_transfer_frozen(self, signers):
        assert len(signers) >= 2

        daemon = Daemon()

        print("Creating multisig transaction from wallet " + str(signers[0]))

        dst = {'address': '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 'amount': 1000000000000}
        res = self.wallet[signers[0]].transfer([dst])
        assert len(res.tx_hash) == 0 # not known yet
        txid = res.tx_hash
        assert len(res.tx_key) == 32*2
        assert res.amount > 0
        amount = res.amount
        assert res.fee > 0
        fee = res.fee
        assert len(res.tx_blob) == 0
        assert len(res.tx_metadata) == 0
        assert len(res.multisig_txset) > 0
        assert len(res.unsigned_txset) == 0
        spent_key_images = res.spent_key_images.key_images
        multisig_txset = res.multisig_txset

        daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 1)
        for i in range(len(self.wallet)):
          self.wallet[i].refresh()

        for i in range(len(signers[1:])):
          # Check that each signer wallet has key image and it is not frozen
          for ki in spent_key_images:
            frozen = self.wallet[signers[i+1]].frozen(ki).frozen
            assert not frozen

        # Freeze key image involved with initiated transfer
        assert len(spent_key_images)
        ki0 = spent_key_images[0]
        print("Freezing involved key image:", ki0)
        self.wallet[signers[1]].freeze(ki0)
        frozen = self.wallet[signers[1]].frozen(ki).frozen
        assert frozen

        # Try signing multisig (this operation should fail b/c of the frozen key image)
        print("Attemping to sign with frozen key image. This should fail")
        try:
          res = self.wallet[signers[1]].sign_multisig(multisig_txset)
          raise ValueError('sign_multisig should not have succeeded w/ fronzen enotes')
        except AssertionError:
          pass

        # Thaw key image and continue transfer as normal
        print("Thawing key image and continuing transfer as normal")
        self.wallet[signers[1]].thaw(ki0)
        frozen = self.wallet[signers[1]].frozen(ki).frozen
        assert not frozen

        for i in range(len(signers[1:])):
          print('Signing multisig transaction with wallet ' + str(signers[i+1]))
          res = self.wallet[signers[i+1]].describe_transfer(multisig_txset = multisig_txset)
          assert len(res.desc) == 1
          desc = res.desc[0]
          assert desc.amount_in >= amount + fee
          assert desc.amount_out == desc.amount_in - fee
          assert desc.ring_size == 16
          assert desc.unlock_time == 0
          assert not 'payment_id' in desc or desc.payment_id in ['', '0000000000000000']
          assert desc.change_amount == desc.amount_in - 1000000000000 - fee
          assert desc.change_address == self.wallet_address
          assert desc.fee == fee
          assert len(desc.recipients) == 1
          rec = desc.recipients[0]
          assert rec.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
          assert rec.amount == 1000000000000

          res = self.wallet[signers[i+1]].sign_multisig(multisig_txset)
          multisig_txset = res.tx_data_hex
          assert len(res.tx_hash_list if 'tx_hash_list' in res else []) == (i == len(signers[1:]) - 1)

          if i < len(signers[1:]) - 1:
            print('Submitting multisig transaction prematurely with wallet ' + str(signers[-1]))
            ok = False
            try: self.wallet[signers[-1]].submit_multisig(multisig_txset)
            except: ok = True
            assert ok

        print('Submitting multisig transaction with wallet ' + str(signers[-1]))
        res = self.wallet[signers[-1]].submit_multisig(multisig_txset)
        assert len(res.tx_hash_list) == 1
        txid = res.tx_hash_list[0]

        for i in range(len(self.wallet)):
          self.wallet[i].refresh()
          res = self.wallet[i].get_transfers()
          assert len([x for x in (res['pending'] if 'pending' in res else []) if x.txid == txid]) == (1 if i == signers[-1] else 0)
          assert len([x for x in (res['out'] if 'out' in res else []) if x.txid == txid]) == 0

        daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 1)
        return txid

    def check_transaction(self, txid):
        for i in range(len(self.wallet)):
          self.wallet[i].refresh()
          res = self.wallet[i].get_transfers()
          assert len([x for x in (res['pending'] if 'pending' in res else []) if x.txid == txid]) == 0
          assert len([x for x in (res['out'] if 'out' in res else []) if x.txid == txid]) == 1


class Guard:
    def __enter__(self):
        for i in range(4):
            Wallet(idx = i).auto_refresh(False)
    def __exit__(self, exc_type, exc_value, traceback):
        for i in range(4):
            Wallet(idx = i).auto_refresh(True)

if __name__ == '__main__':
    with Guard() as guard:
        MultisigTest().run_test()
