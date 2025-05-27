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

"""Test txpool
"""

from framework.daemon import Daemon
from framework.wallet import Wallet
from framework.zmq import Zmq

class TransferTest():
    def run_test(self):
        self.reset()
        self.create()
        self.mine()
        self.check_txpool()

    def reset(self):
        print('Resetting blockchain')
        daemon = Daemon()
        res = daemon.get_height()
        daemon.pop_blocks(res.height - 1)
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

    def check_empty_pool(self):
        daemon = Daemon()

        res = daemon.get_transaction_pool_hashes()
        assert not 'tx_hashes' in res or len(res.tx_hashes) == 0
        res = daemon.get_transaction_pool_stats()
        assert res.pool_stats.bytes_total == 0
        assert res.pool_stats.bytes_min == 0
        assert res.pool_stats.bytes_max == 0
        assert res.pool_stats.bytes_med == 0
        assert res.pool_stats.fee_total == 0
        assert res.pool_stats.oldest == 0
        assert res.pool_stats.txs_total == 0
        assert res.pool_stats.num_failing == 0
        assert res.pool_stats.num_10m == 0
        assert res.pool_stats.num_not_relayed == 0
        assert res.pool_stats.histo_98pc == 0
        assert not 'histo' in res.pool_stats or len(res.pool_stats.histo) == 0
        assert res.pool_stats.num_double_spends == 0

    def check_txpool(self):
        daemon = Daemon()
        wallet = Wallet()
        zmq = Zmq()

        zmq_topic = "json-minimal-txpool_add"
        zmq.sub(zmq_topic)

        res = daemon.get_info()
        height = res.height
        txpool_size = res.tx_pool_size

        self.check_empty_pool()

        txes = self.create_txes('46r4nYSevkfBUMhuykdK3gQ98XDqDTYW1hNLaXNvjpsJaSbNtdXh1sKMsdVgqkaihChAzEy29zEDPMR3NHQvGoZCLGwTerK', 5)

        res = daemon.get_info()
        assert res.tx_pool_size == txpool_size + 5
        txpool_size = res.tx_pool_size

        res = daemon.get_transaction_pool()
        assert len(res.transactions) == txpool_size
        total_weight = 0
        total_fee = 0
        min_weight = 99999999999999
        max_weight = 0
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

            total_fee += x.fee

        print('Checking all txs received via zmq')
        for i in range(len(txes.keys())):
            zmq_event = zmq.recv(zmq_topic)
            assert len(zmq_event) == 1

            zmq_tx = zmq_event[0]

            x = [x for x in res.transactions if x.id_hash == zmq_tx["id"]]
            assert len(x) == 1

            x = x[0]
            assert x.blob_size == zmq_tx["blob_size"]
            assert x.weight == zmq_tx["weight"]
            assert x.fee == zmq_tx["fee"]

            total_weight += x.weight
            min_weight = min(min_weight, x.weight)
            max_weight = max(max_weight, x.weight)

        res = daemon.get_transaction_pool_hashes()
        assert sorted(res.tx_hashes) == sorted(txes.keys())

        res = daemon.get_transaction_pool_stats()
        assert res.pool_stats.bytes_total == total_weight
        assert res.pool_stats.bytes_min == min_weight
        assert res.pool_stats.bytes_max == max_weight
        assert res.pool_stats.bytes_med >= min_weight and res.pool_stats.bytes_med <= max_weight
        assert res.pool_stats.fee_total == total_fee
        assert res.pool_stats.txs_total == len(txes)
        assert res.pool_stats.num_failing == 0
        assert res.pool_stats.num_10m == 0
        assert res.pool_stats.num_not_relayed == 0
        assert res.pool_stats.num_double_spends == 0

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

        res = daemon.get_transaction_pool()
        assert len(res.transactions) == len(new_keys)
        total_weight = 0
        total_fee = 0
        min_weight = 99999999999999
        max_weight = 0
        for txid in new_keys:
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

            total_weight += x.weight
            total_fee += x.fee
            min_weight = min(min_weight, x.weight)
            max_weight = max(max_weight, x.weight)

        res = daemon.get_transaction_pool_stats()
        assert res.pool_stats.bytes_total == total_weight
        assert res.pool_stats.bytes_min == min_weight
        assert res.pool_stats.bytes_max == max_weight
        assert res.pool_stats.bytes_med >= min_weight and res.pool_stats.bytes_med <= max_weight
        assert res.pool_stats.fee_total == total_fee
        assert res.pool_stats.txs_total == len(new_keys)
        assert res.pool_stats.num_failing == 0
        assert res.pool_stats.num_10m == 0
        assert res.pool_stats.num_not_relayed == 0
        assert res.pool_stats.num_double_spends == 0

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

        self.check_empty_pool()

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

        print('Checking relaying txes')
        res = daemon.get_transaction_pool_hashes()
        assert len(res.tx_hashes) > 0
        txid = res.tx_hashes[0]
        daemon.relay_tx([txid])
        res = daemon.get_transactions([txid])
        assert len(res.txs) == 1
        assert res.txs[0].tx_hash == txid
        assert res.txs[0].in_pool
        assert res.txs[0].relayed

        daemon.flush_txpool()
        self.check_empty_pool()


if __name__ == '__main__':
    TransferTest().run_test()
