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

"""Test cold tx signing
"""

from framework.daemon import Daemon
from framework.wallet import Wallet
import random

SEED = 'velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted'
STANDARD_ADDRESS = '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
SUBADDRESS = '84QRUYawRNrU3NN1VpFRndSukeyEb3Xpv8qZjjsoJZnTYpDYceuUTpog13D7qPxpviS7J29bSgSkR11hFFoXWk2yNdsR9WF'

class ColdSigningTest():
    def run_test(self):
        print('TODO: update cold signing for FCMP++/Carrot')
        return
        self.reset()
        self.create(0)
        self.mine()
        for piecemeal_output_export in [False, True]:
            self.transfer(piecemeal_output_export)
        for piecemeal_output_export in [False, True]:
            self.self_transfer_to_subaddress(piecemeal_output_export)
        self.transfer_after_empty_export_import()

    def reset(self):
        print('Resetting blockchain')
        daemon = Daemon()
        res = daemon.get_height()
        daemon.pop_blocks(res.height - 1)
        daemon.flush_txpool()

    def create(self, idx):
        print('Creating hot and cold wallet')

        self.hot_wallet = Wallet(idx = 0)
        # close the wallet if any, will throw if none is loaded
        try: self.hot_wallet.close_wallet()
        except: pass

        self.cold_wallet = Wallet(idx = 5)
        # close the wallet if any, will throw if none is loaded
        try: self.cold_wallet.close_wallet()
        except: pass

        res = self.cold_wallet.restore_deterministic_wallet(seed = SEED)
        spend_key = self.cold_wallet.query_key("spend_key").key
        view_key = self.cold_wallet.query_key("view_key").key
        res = self.hot_wallet.generate_from_keys(viewkey = view_key, address = STANDARD_ADDRESS)

        ok = False
        try: res = self.hot_wallet.query_key("spend_key")
        except: ok = True
        assert ok
        ok = False
        try: self.hot_wallet.query_key("mnemonic")
        except: ok = True
        assert ok
        assert self.cold_wallet.query_key("view_key").key == view_key
        assert self.cold_wallet.get_address().address == self.hot_wallet.get_address().address
        assert self.cold_wallet.get_address().address == STANDARD_ADDRESS

    def mine(self):
        print("Mining some blocks")
        daemon = Daemon()
        wallet = Wallet()

        daemon.generateblocks(STANDARD_ADDRESS, 80)
        wallet.refresh()

    def export_import(self, piecemeal_output_export):
        self.hot_wallet.refresh()

        if piecemeal_output_export:
            res = self.hot_wallet.incoming_transfers()
            num_outputs = len(res.transfers)
            done = [False] * num_outputs
            while len([x for x in done if not done[x]]) > 0:
                start = int(random.random() * num_outputs)
                if start == num_outputs:
                    num_outputs -= 1
                count = 1 + int(random.random() * 5)
                res = self.hot_wallet.export_outputs(all = True, start = start, count = count)

                # the hot wallet cannot import outputs
                ok = False
                try:
                    self.hot_wallet.import_outputs(res.outputs_data_hex)
                except:
                    ok = True
                assert ok

                try:
                    self.cold_wallet.import_outputs(res.outputs_data_hex)
                except Exception as e:
                    # this just means we selected later outputs first, without filling
                    # new outputs first
                    if 'Imported outputs omit more outputs that we know of' not in str(e):
                        raise
                for i in range(start, start + count):
                    if i < len(done):
                        done[i] = True
        else:
            res = self.hot_wallet.export_outputs()
            self.cold_wallet.import_outputs(res.outputs_data_hex)

        res = self.cold_wallet.export_key_images(True)
        self.hot_wallet.import_key_images(res.signed_key_images, offset = res.offset)

    def create_tx(self, destination_addr, piecemeal_output_export):
        daemon = Daemon()

        dst = {'address': destination_addr, 'amount': 1000000000000}

        self.export_import(piecemeal_output_export)

        res = self.hot_wallet.transfer([dst], ring_size = 16, get_tx_key = False)
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
        assert len(res.unsigned_txset) > 0
        unsigned_txset = res.unsigned_txset

        print('Signing transaction with cold wallet')
        res = self.cold_wallet.describe_transfer(unsigned_txset = unsigned_txset)
        assert len(res.desc) == 1
        desc = res.desc[0]
        assert desc.amount_in >= amount + fee
        assert desc.amount_out == desc.amount_in - fee
        assert desc.ring_size == 16
        assert desc.unlock_time == 0
        assert desc.payment_id in ['', '0000000000000000']
        assert desc.change_amount == desc.amount_in - 1000000000000 - fee
        assert desc.change_address == STANDARD_ADDRESS
        assert desc.fee == fee
        assert len(desc.recipients) == 1
        rec = desc.recipients[0]
        assert rec.address == destination_addr
        assert rec.amount == 1000000000000

        res = self.cold_wallet.sign_transfer(unsigned_txset)
        assert len(res.signed_txset) > 0
        signed_txset = res.signed_txset
        assert len(res.tx_hash_list) == 1
        txid = res.tx_hash_list[0]
        assert len(txid) == 64

        print('Submitting transaction with hot wallet')
        res = self.hot_wallet.submit_transfer(signed_txset)
        assert len(res.tx_hash_list) > 0
        assert res.tx_hash_list[0] == txid

        res = self.hot_wallet.get_transfers()
        assert len([x for x in (res['pending'] if 'pending' in res else []) if x.txid == txid]) == 1
        assert len([x for x in (res['out'] if 'out' in res else []) if x.txid == txid]) == 0

        daemon.generateblocks(STANDARD_ADDRESS, 1)
        self.hot_wallet.refresh()

        res = self.hot_wallet.get_transfers()
        assert len([x for x in (res['pending'] if 'pending' in res else []) if x.txid == txid]) == 0
        assert len([x for x in (res['out'] if 'out' in res else []) if x.txid == txid]) == 1

        res = self.hot_wallet.get_tx_key(txid)
        assert len(res.tx_key) == 0 or res.tx_key == '01' + '0' * 62 # identity is used as placeholder
        res = self.cold_wallet.get_tx_key(txid)
        assert len(res.tx_key) == 64

        self.export_import(piecemeal_output_export)

    def transfer(self, piecemeal_output_export):
        print("Creating transaction in hot wallet")
        self.create_tx(STANDARD_ADDRESS, piecemeal_output_export)

        res = self.cold_wallet.get_address()
        assert len(res['addresses']) == 1
        assert res['addresses'][0].address == STANDARD_ADDRESS
        assert res['addresses'][0].used

        res = self.hot_wallet.get_address()
        assert len(res['addresses']) == 1
        assert res['addresses'][0].address == STANDARD_ADDRESS
        assert res['addresses'][0].used

    def self_transfer_to_subaddress(self, piecemeal_output_export):
        print("Self-spending to subaddress in hot wallet")
        self.create_tx(SUBADDRESS, piecemeal_output_export)

        res = self.cold_wallet.get_address()
        assert len(res['addresses']) == 2
        assert res['addresses'][0].address == STANDARD_ADDRESS
        assert res['addresses'][0].used
        assert res['addresses'][1].address == SUBADDRESS
        assert res['addresses'][1].used

        res = self.hot_wallet.get_address()
        assert len(res['addresses']) == 2
        assert res['addresses'][0].address == STANDARD_ADDRESS
        assert res['addresses'][0].used
        assert res['addresses'][1].address == SUBADDRESS
        assert res['addresses'][1].used

    def transfer_after_empty_export_import(self):
        print("Creating transaction in hot wallet after empty export & import")
        start_len = len(self.hot_wallet.get_transfers()['in'])
        self.export_import(False)
        assert start_len == len(self.hot_wallet.get_transfers()['in'])
        self.create_tx(STANDARD_ADDRESS, False)
        assert start_len == len(self.hot_wallet.get_transfers()['in']) - 1

class Guard:
    def __enter__(self):
        for i in range(2):
            Wallet(idx = i).auto_refresh(False)
    def __exit__(self, exc_type, exc_value, traceback):
        for i in range(2):
            Wallet(idx = i).auto_refresh(True)

if __name__ == '__main__':
    with Guard() as guard:
        cs = ColdSigningTest().run_test()
