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

from __future__ import print_function

"""Test multisig transfers
"""

from framework.daemon import Daemon
from framework.wallet import Wallet

class MultisigTest():
    def run_test(self):
        self.reset()
        self.mine('493DsrfJPqiN3Suv9RcRDoZEbQtKZX1sNcGPA3GhkKYEEmivk8kjQrTdRdVc4ZbmzWJuE157z9NNUKmF2VDfdYDR3CziGMk', 5)
        self.mine('42jSRGmmKN96V2j3B8X2DbiNThBXW1tSi1rW1uwkqbyURenq3eC3yosNm8HEMdHuWwKMFGzMUB3RCTvcTaW9kHpdRPP7p5y', 5)
        self.mine('47fF32AdrmXG84FcPY697uZdd42pMMGiH5UpiTRTt3YX2pZC7t7wkzEMStEicxbQGRfrYvAAYxH6Fe8rnD56EaNwUgxRd53', 5)
        self.mine('44SKxxLQw929wRF6BA9paQ1EWFshNnKhXM3qz6Mo3JGDE2YG3xyzVutMStEicxbQGRfrYvAAYxH6Fe8rnD56EaNwUiqhcwR', 5)
        self.mine('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 60)

        self.create_multisig_wallets(2, 2, '493DsrfJPqiN3Suv9RcRDoZEbQtKZX1sNcGPA3GhkKYEEmivk8kjQrTdRdVc4ZbmzWJuE157z9NNUKmF2VDfdYDR3CziGMk')
        self.import_multisig_info([1, 0], 5)
        txid = self.transfer([1, 0])
        self.import_multisig_info([0, 1], 6)
        self.check_transaction(txid)

        self.create_multisig_wallets(2, 3, '42jSRGmmKN96V2j3B8X2DbiNThBXW1tSi1rW1uwkqbyURenq3eC3yosNm8HEMdHuWwKMFGzMUB3RCTvcTaW9kHpdRPP7p5y')
        self.import_multisig_info([0, 2], 5)
        txid = self.transfer([0, 2])
        self.import_multisig_info([0, 1, 2], 6)
        self.check_transaction(txid)

        self.create_multisig_wallets(3, 4, '47fF32AdrmXG84FcPY697uZdd42pMMGiH5UpiTRTt3YX2pZC7t7wkzEMStEicxbQGRfrYvAAYxH6Fe8rnD56EaNwUgxRd53')
        self.import_multisig_info([0, 2, 3], 5)
        txid = self.transfer([0, 2, 3])
        self.import_multisig_info([0, 1, 2, 3], 6)
        self.check_transaction(txid)

        self.create_multisig_wallets(2, 4, '44SKxxLQw929wRF6BA9paQ1EWFshNnKhXM3qz6Mo3JGDE2YG3xyzVutMStEicxbQGRfrYvAAYxH6Fe8rnD56EaNwUiqhcwR')
        self.import_multisig_info([1, 2], 5)
        txid = self.transfer([1, 2])
        self.import_multisig_info([0, 1, 2, 3], 6)
        self.check_transaction(txid)

    def reset(self):
        print('Resetting blockchain')
        daemon = Daemon()
        daemon.pop_blocks(1000)
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
        res = self.wallet[i].prepare_multisig()
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
        assert res.ready == (M_threshold == N_total)
        assert res.threshold == M_threshold
        assert res.total == N_total

      while True:
        n_empty = 0
        for i in range(len(next_stage)):
          if len(next_stage[i]) == 0:
            n_empty += 1
        assert n_empty == 0 or n_empty == len(next_stage)
        if n_empty == len(next_stage):
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
          assert desc.ring_size == 11
          assert desc.unlock_time == 0
          assert desc.payment_id == '0000000000000000'
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
