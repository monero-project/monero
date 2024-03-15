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
import random

"""Test multisig transfers
"""

from framework.daemon import Daemon
from framework.wallet import Wallet

TEST_CASES = \
[
#  M  N  Primary Address
  [2, 2, '45J58b7PmKJFSiNPFFrTdtfMcFGnruP7V4CMuRpX7NsH4j3jGHKAjo3YJP2RePX6HMaSkbvTbrWUFhDNcNcHgtNmQ3gr7sG'],
  [2, 3, '44G2TQNfsiURKkvxp7gbgaJY8WynZvANnhmyMAwv6WeEbAvyAWMfKXRhh3uBXT2UAKhAsUJ7Fg5zjjF2U1iGciFk5duN94i'],
  [3, 3, '41mro238grj56GnrWkakAKTkBy2yDcXYsUZ2iXCM9pe5Ueajd2RRc6Fhh3uBXT2UAKhAsUJ7Fg5zjjF2U1iGciFk5ief4ZP'],
  [3, 4, '44vZSprQKJQRFe6t1VHgU4ESvq2dv7TjBLVGE7QscKxMdFSiyyPCEV64NnKUQssFPyWxc2meyt7j63F2S2qtCTRL6dakeff'],
  [2, 4, '47puypSwsV1gvUDratmX4y58fSwikXVehEiBhVLxJA1gRCxHyrRgTDr4NnKUQssFPyWxc2meyt7j63F2S2qtCTRL6aRPj5U'],
  [1, 2, '4A8RnBQixry4VXkqeWhmg8L7vWJVDJj4FN9PV4E7Mgad5ZZ6LKQdn8dYJP2RePX6HMaSkbvTbrWUFhDNcNcHgtNmQ4S8RSB']
]

PUB_ADDRS = [case[2] for case in TEST_CASES]

class MultisigTest():
    def run_test(self):
        self.reset()
        for pub_addr in PUB_ADDRS:
            self.mine(pub_addr, 4)
        self.mine('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 80)

        self.test_states()

        self.fund_addrs_with_normal_wallet(PUB_ADDRS)

        for M, N, pub_addr in TEST_CASES:
            assert M <= N
            shuffled_participants = list(range(N))
            random.shuffle(shuffled_participants)
            shuffled_signers = shuffled_participants[:M]

            expected_outputs = 5 # each wallet owns four mined outputs & one transferred output

            # Create multisig wallet and test transferring
            self.create_multisig_wallets(M, N, pub_addr)
            self.import_multisig_info(shuffled_signers if M != 1 else shuffled_participants, expected_outputs)
            txid = self.transfer(shuffled_signers)
            expected_outputs += 1
            self.import_multisig_info(shuffled_participants, expected_outputs)
            self.check_transaction(txid)

            # If more than 1 signer, try to freeze key image of one signer, make tx using that key
            # image on another signer, then have first signer sign multisg_txset. Should fail
            if M != 1:
                txid = self.try_transfer_frozen(shuffled_signers)
                expected_outputs += 1
                self.import_multisig_info(shuffled_participants, expected_outputs)
                self.check_transaction(txid)

            # Recreate wallet from multisig seed and test transferring
            self.remake_some_multisig_wallets_by_multsig_seed(M)
            self.import_multisig_info(shuffled_signers if M != 1 else shuffled_participants, expected_outputs)
            txid = self.transfer(shuffled_signers)
            expected_outputs += 1
            self.import_multisig_info(shuffled_participants, expected_outputs)
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

    # This method sets up N_total wallets with a threshold of M_threshold doing the following steps:
    #   * restore_deterministic_wallet(w/ hardcoded seeds)
    #   * prepare_multisig(enable_multisig_experimental = True)
    #   * make_multisig()
    #   * exchange_multisig_keys()
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

      # restore_deterministic_wallet() & prepare_multisig()
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

      # Assert that all wallets are multisig
      for i in range(N_total):
        res = self.wallet[i].is_multisig()
        assert res.multisig == False

      # make_multisig() with each other's info
      addresses = []
      next_stage = []
      for i in range(N_total):
        res = self.wallet[i].make_multisig(info, M_threshold)
        addresses.append(res.address)
        next_stage.append(res.multisig_info)

      # Assert multisig paramaters M/N for each wallet
      for i in range(N_total):
        res = self.wallet[i].is_multisig()
        assert res.multisig == True
        assert not res.ready
        assert res.threshold == M_threshold
        assert res.total == N_total

      # exchange_multisig_keys()
      num_exchange_multisig_keys_stages = 0
      while True: # while not all wallets are ready
        n_ready = 0
        for i in range(N_total):
          res = self.wallet[i].is_multisig()
          if res.ready == True:
            n_ready += 1
        assert n_ready == 0 or n_ready == N_total # No partial readiness
        if n_ready == N_total:
          break
        info = next_stage
        next_stage = []
        addresses = []
        for i in range(N_total):
          res = self.wallet[i].exchange_multisig_keys(info)
          next_stage.append(res.multisig_info)
          addresses.append(res.address)
        num_exchange_multisig_keys_stages += 1

      # We should only need N - M + 1 key exchange rounds
      assert num_exchange_multisig_keys_stages == N_total - M_threshold + 1

      # Assert that the all wallets have expected public address
      for i in range(N_total):
        assert addresses[i] == expected_address, addresses[i]
      self.wallet_address = expected_address

      # Assert multisig paramaters M/N and "ready" for each wallet
      for i in range(N_total):
        res = self.wallet[i].is_multisig()
        assert res.multisig == True
        assert res.ready == True
        assert res.threshold == M_threshold
        assert res.total == N_total

    # We want to test if multisig wallets can receive normal transfers as well and mining transfers
    def fund_addrs_with_normal_wallet(self, addrs):
      print("Funding multisig wallets with normal wallet-to-wallet transfers")

      # Generate normal deterministic wallet
      normal_seed = 'velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted'
      assert not hasattr(self, 'wallet') or not self.wallet
      self.wallet = [Wallet(idx = 0)]
      res = self.wallet[0].restore_deterministic_wallet(seed = normal_seed)
      assert res.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'

      self.wallet[0].refresh()

      # Check that we own enough spendable enotes
      res = self.wallet[0].incoming_transfers(transfer_type = 'available')
      assert 'transfers' in res
      num_outs_spendable = 0
      min_out_amount = None
      for t in res.transfers:
          if not t.spent:
            num_outs_spendable += 1
            min_out_amount = min(min_out_amount, t.amount) if min_out_amount is not None else t.amount
      assert num_outs_spendable >= 2 * len(addrs)

      # Transfer to addrs and mine to confirm tx
      dsts = [{'address': addr, 'amount': int(min_out_amount * 0.95)} for addr in addrs]
      res = self.wallet[0].transfer(dsts, get_tx_metadata = True)
      tx_hex = res.tx_metadata
      res = self.wallet[0].relay_tx(tx_hex)
      self.mine('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 10)

    def remake_some_multisig_wallets_by_multsig_seed(self, threshold):
      N = len(self.wallet)
      num_signers_to_remake = random.randint(1, N) # Do at least one
      signers_to_remake = list(range(N))
      random.shuffle(signers_to_remake)
      signers_to_remake = signers_to_remake[:num_signers_to_remake]

      for i in signers_to_remake:
        print("Remaking {}/{} multsig wallet from multisig seed: #{}".format(threshold, N, i+1))

        otherwise_unused_seed = \
          'factual wiggle awakened maul sash biscuit pause reinvest fonts sleepless knowledge tossed jewels request gusts dagger gumball onward dotted amended powder cynical strained topic request'

        # Get information about wallet, will compare against later
        old_viewkey = self.wallet[i].query_key('view_key').key
        old_spendkey = self.wallet[i].query_key('spend_key').key
        old_multisig_seed = self.wallet[i].query_key('mnemonic').key

        # Close old wallet and restore w/ random seed so we know that restoring actually did something
        self.wallet[i].close_wallet()
        self.wallet[i].restore_deterministic_wallet(seed=otherwise_unused_seed)
        mid_viewkey = self.wallet[i].query_key('view_key').key
        assert mid_viewkey != old_viewkey

        # Now restore w/ old multisig seed and check against original
        self.wallet[i].close_wallet()
        self.wallet[i].restore_deterministic_wallet(seed=old_multisig_seed, enable_multisig_experimental=True)
        new_viewkey = self.wallet[i].query_key('view_key').key
        new_spendkey = self.wallet[i].query_key('spend_key').key
        new_multisig_seed = self.wallet[i].query_key('mnemonic').key
        assert new_viewkey == old_viewkey
        assert new_spendkey == old_spendkey
        assert new_multisig_seed == old_multisig_seed

        self.wallet[i].refresh()

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
        assert len(signers) >= 1

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
          raise ValueError('sign_multisig should not have succeeded w/ frozen enotes')
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
