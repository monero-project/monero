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

"""Test txpool
"""

from framework.daemon import Daemon
from framework.wallet import Wallet

class TransferTest():
    def run_test(self):
        self.reset()
        self.create()
        self.mine()
        self.check_txpool()

    def reset(self):
        print('Resetting blockchain')
        daemon = Daemon()
        daemon.pop_blocks(1000)
        daemon.flush_txpool()

    def create(self):
        print('Creating wallet')
        wallet = Wallet()
        # close the wallet if any, will throw if none is loaded
        try: wallet.close_wallet()
        except: pass
        seed = 'velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted'
        res = wallet.restore_deterministic_wallet(seed = seed)

    def mine(self):
        print("Mining some blocks")
        daemon = Daemon()
        wallet = Wallet()

        daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 80)
        wallet.refresh()

    def create_txes(self, address, ntxes):
        print('Creating ' + str(ntxes) + ' transactions')

        daemon = Daemon()
        wallet = Wallet()

        dst = {'address': address, 'amount': 1000000000000}

        txes = {}
        for i in range(ntxes):
          res = wallet.transfer([dst], get_tx_hex = True)
          txes[res.tx_hash] = res

        return txes

    def check_txpool(self):
        daemon = Daemon()
        wallet = Wallet()

        res = daemon.get_info()
        height = res.height
        txpool_size = res.tx_pool_size

        txes = self.create_txes('46r4nYSevkfBUMhuykdK3gQ98XDqDTYW1hNLaXNvjpsJaSbNtdXh1sKMsdVgqkaihChAzEy29zEDPMR3NHQvGoZCLGwTerK', 5)

        res = daemon.get_info()
        assert res.tx_pool_size == txpool_size + 5
        txpool_size = res.tx_pool_size

        res = daemon.get_transaction_pool()
        assert len(res.transactions) == txpool_size
        for txid in txes.keys():
            x = [x for x in res.transactions if x.id_hash == txid]
            assert len(x) == 1
            x = x[0]
            assert x.kept_by_block == False
            assert x.last_failed_id_hash == '0'*64
            assert x.double_spend_seen == False
            assert x.weight >= x.blob_size

            assert x.blob_size * 2 == len(txes[txid].tx_blob)
            assert x.fee == txes[txid].fee
            assert x.tx_blob == txes[txid].tx_blob

        res = daemon.get_transaction_pool_hashes()
        assert sorted(res.tx_hashes) == sorted(txes.keys())

        print('Flushing 2 transactions')
        txes_keys = list(txes.keys())
        daemon.flush_txpool([txes_keys[1], txes_keys[3]])
        res = daemon.get_transaction_pool()
        assert len(res.transactions) == txpool_size - 2
        assert len([x for x in res.transactions if x.id_hash == txes_keys[1]]) == 0
        assert len([x for x in res.transactions if x.id_hash == txes_keys[3]]) == 0

        new_keys = list(txes.keys())
        new_keys.remove(txes_keys[1])
        new_keys.remove(txes_keys[3])
        res = daemon.get_transaction_pool_hashes()
        assert sorted(res.tx_hashes) == sorted(new_keys)

        print('Flushing unknown transactions')
        unknown_txids = ['1'*64, '2'*64, '3'*64]
        daemon.flush_txpool(unknown_txids)
        res = daemon.get_transaction_pool()
        assert len(res.transactions) == txpool_size - 2

        print('Mining transactions')
        daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 1)
        res = daemon.get_transaction_pool()
        assert not 'transactions' in res or len(res.transactions) == txpool_size - 5
        res = daemon.get_transaction_pool_hashes()
        assert not 'tx_hashes' in res or len(res.tx_hashes) == 0

        print('Popping block')
        daemon.pop_blocks(1)
        res = daemon.get_transaction_pool_hashes()
        assert sorted(res.tx_hashes) == sorted(new_keys)
        res = daemon.get_transaction_pool()
        assert len(res.transactions) == txpool_size - 2
        for txid in new_keys:
            x = [x for x in res.transactions if x.id_hash == txid]
            assert len(x) == 1
            x = x[0]
            assert x.kept_by_block == True
            assert x.last_failed_id_hash == '0'*64
            assert x.double_spend_seen == False
            assert x.weight >= x.blob_size

            assert x.blob_size * 2 == len(txes[txid].tx_blob)
            assert x.fee == txes[txid].fee
            assert x.tx_blob == txes[txid].tx_blob


if __name__ == '__main__':
    TransferTest().run_test()
