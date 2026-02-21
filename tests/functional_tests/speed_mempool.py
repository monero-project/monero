#!/usr/bin/env python3

# Copyright (c) 2018-2024, The Monero Project

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

"""Mempool throughput benchmark

Measures:
    - TX creation time (wallet.transfer)
    - Mempool add throughput (send_raw_transaction)
    - Mempool query performance under load (get_transaction_pool, pool_stats, pool_hashes)
    - Block template fill time (generateblocks with loaded mempool)

"""

import time
import statistics

from framework.daemon import Daemon
from framework.wallet import Wallet

SEED = 'velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted'
MINER_ADDR = '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
DEST_ADDR = '888tNkZrPN6JsEgekjMnABU4TBzc2Dt29EPAvkRxbANsAnjyPbb3iQ1YBRk1UXcdRsiKc9dhwMVgN5S9cQUiyoogDavup3H'
AMOUNT = 1000000000000
TX_COUNT = 20
MINE_BLOCKS = 130
QUERY_REPS = 5


def fmt_stats(times):
    """Format a list of timing measurements as a stats string."""
    if not times:
        return 'N/A'
    mn = min(times)
    mx = max(times)
    avg = statistics.mean(times)
    med = statistics.median(times)
    return 'min={:.4f}s  max={:.4f}s  avg={:.4f}s  med={:.4f}s'.format(mn, mx, avg, med)


class MempoolBenchmark():
    def run_test(self):
        self.results = {}
        self.reset()
        self.create()
        self.mine()
        self.bench_tx_creation()
        self.reset_wallet()
        self.bench_mempool_add_raw()
        self.bench_mempool_queries_under_load()
        self.bench_block_template_fill()
        self.print_summary()

    def reset(self):
        print('Resetting blockchain')
        daemon = Daemon()
        res = daemon.get_height()
        daemon.pop_blocks(res.height - 1)
        daemon.flush_txpool()

    def create(self):
        print('Creating wallet')
        wallet = Wallet()
        try: wallet.close_wallet()
        except: pass
        wallet.restore_deterministic_wallet(seed = SEED)

    def reset_wallet(self):
        print('Resetting wallet state')
        daemon = Daemon()
        wallet = Wallet()
        daemon.flush_txpool()
        wallet.close_wallet()
        wallet.restore_deterministic_wallet(seed = SEED)
        wallet.refresh()

    def mine(self):
        print('Mining {} blocks for funds'.format(MINE_BLOCKS))
        daemon = Daemon()
        wallet = Wallet()

        start = time.time()
        daemon.generateblocks(MINER_ADDR, MINE_BLOCKS)
        elapsed = time.time() - start
        print('  mining took {:.2f}s'.format(elapsed))

        wallet.refresh()
        res = wallet.get_balance()
        print('  unlocked balance: {}'.format(res.unlocked_balance))
        assert res.unlocked_balance > 0, 'No unlocked balance after mining'

    def bench_tx_creation(self):
        print('\n=== Benchmark: TX creation (wallet.transfer) ===')
        wallet = Wallet()
        dst = {'address': DEST_ADDR, 'amount': AMOUNT}

        creation_times = []
        for i in range(TX_COUNT):
            start = time.time()
            res = wallet.transfer([dst], do_not_relay = True, get_tx_hex = True)
            elapsed = time.time() - start
            creation_times.append(elapsed)
            assert res.tx_hash
            assert res.tx_blob
            print('  tx {:2d}/{}: {:.4f}s  (fee={})'.format(i + 1, TX_COUNT, elapsed, res.fee))

        self.results['tx_creation'] = creation_times
        print('  Stats: ' + fmt_stats(creation_times))

    def bench_mempool_add_raw(self):
        print('\n=== Benchmark: Mempool add (send_raw_transaction) ===')
        daemon = Daemon()
        wallet = Wallet()
        dst = {'address': DEST_ADDR, 'amount': AMOUNT}

        # Relay txs first so the wallet assigns unique key images,
        # then flush and re-submit to isolate mempool insertion time.
        print('  Pre-creating {} transactions...'.format(TX_COUNT))
        raw_txs = []
        for i in range(TX_COUNT):
            res = wallet.transfer([dst], get_tx_hex = True)
            raw_txs.append(res.tx_blob)

        res = daemon.get_info()
        assert res.tx_pool_size == TX_COUNT

        print('  Flushing and re-submitting to mempool...')
        daemon.flush_txpool()

        add_times = []
        for i, tx_hex in enumerate(raw_txs):
            start = time.time()
            res = daemon.send_raw_transaction(tx_hex)
            elapsed = time.time() - start
            add_times.append(elapsed)
            assert res.status == 'OK', 'send_raw_transaction failed: {}'.format(res.status)
            print('  tx {:2d}/{}: {:.4f}s'.format(i + 1, TX_COUNT, elapsed))

        total = sum(add_times)
        throughput = len(add_times) / total if total > 0 else 0
        self.results['mempool_add'] = add_times
        self.results['mempool_add_throughput'] = throughput
        print('  Stats: ' + fmt_stats(add_times))
        print('  Throughput: {:.2f} txs/sec'.format(throughput))

        res = daemon.get_info()
        assert res.tx_pool_size == TX_COUNT, 'Expected {} txs in pool, got {}'.format(TX_COUNT, res.tx_pool_size)

    def bench_mempool_queries_under_load(self):
        print('\n=== Benchmark: Mempool queries under load ===')
        daemon = Daemon()
        wallet = Wallet()
        dst = {'address': DEST_ADDR, 'amount': AMOUNT}

        daemon.flush_txpool()
        wallet.refresh()

        for pool_size in [0, 10, 20]:
            current = daemon.get_info().tx_pool_size
            needed = pool_size - current
            if needed > 0:
                print('  Adding {} txs to reach pool size {}...'.format(needed, pool_size))
                for _ in range(needed):
                    wallet.transfer([dst])

            actual_size = daemon.get_info().tx_pool_size
            print('\n  Pool size: {} txs'.format(actual_size))

            pool_times = []
            for _ in range(QUERY_REPS):
                start = time.time()
                daemon.get_transaction_pool()
                pool_times.append(time.time() - start)

            hashes_times = []
            for _ in range(QUERY_REPS):
                start = time.time()
                daemon.get_transaction_pool_hashes()
                hashes_times.append(time.time() - start)

            stats_times = []
            for _ in range(QUERY_REPS):
                start = time.time()
                daemon.get_transaction_pool_stats()
                stats_times.append(time.time() - start)

            label = 'queries_pool_{}'.format(actual_size)
            self.results[label + '_get_pool'] = pool_times
            self.results[label + '_get_hashes'] = hashes_times
            self.results[label + '_get_stats'] = stats_times
            print('    get_transaction_pool:       ' + fmt_stats(pool_times))
            print('    get_transaction_pool_hashes: ' + fmt_stats(hashes_times))
            print('    get_transaction_pool_stats:  ' + fmt_stats(stats_times))

    def bench_block_template_fill(self):
        print('\n=== Benchmark: Block template fill (mine with loaded mempool) ===')
        daemon = Daemon()

        pool_size = daemon.get_info().tx_pool_size
        print('  Mempool has {} txs'.format(pool_size))

        start = time.time()
        daemon.generateblocks(MINER_ADDR, 1)
        elapsed = time.time() - start

        new_pool_size = daemon.get_info().tx_pool_size
        txs_mined = pool_size - new_pool_size
        self.results['block_fill_time'] = elapsed
        self.results['block_fill_txs'] = txs_mined
        print('  generateblocks(1) took {:.4f}s'.format(elapsed))
        print('  Transactions mined: {}'.format(txs_mined))
        if txs_mined > 0:
            print('  Avg verification per tx: {:.4f}s'.format(elapsed / txs_mined))

    def print_summary(self):
        print('\n' + '=' * 60)
        print('MEMPOOL BENCHMARK SUMMARY')
        print('=' * 60)

        if 'tx_creation' in self.results:
            times = self.results['tx_creation']
            print('\nTX Creation ({} txs):'.format(len(times)))
            print('  ' + fmt_stats(times))
            print('  Total: {:.4f}s'.format(sum(times)))

        if 'mempool_add' in self.results:
            times = self.results['mempool_add']
            print('\nMempool Add ({} txs):'.format(len(times)))
            print('  ' + fmt_stats(times))
            print('  Total: {:.4f}s'.format(sum(times)))
            print('  Throughput: {:.2f} txs/sec'.format(self.results.get('mempool_add_throughput', 0)))

        printed_pools = set()
        for key, val in sorted(self.results.items()):
            if key.startswith('queries_pool_') and isinstance(val, list):
                parts = key.split('_', 3)
                pool_n = parts[2]
                query = parts[3]
                if pool_n not in printed_pools:
                    print('\nMempool Queries (pool={}):'.format(pool_n))
                    printed_pools.add(pool_n)
                print('  {}: {}'.format(query, fmt_stats(val)))

        if 'block_fill_time' in self.results:
            print('\nBlock Template Fill:')
            print('  Time: {:.4f}s'.format(self.results['block_fill_time']))
            txs = self.results.get('block_fill_txs', 0)
            print('  TXs mined: {}'.format(txs))
            if txs > 0:
                print('  Avg per tx: {:.4f}s'.format(self.results['block_fill_time'] / txs))

        print('\n' + '=' * 60)


if __name__ == '__main__':
    MempoolBenchmark().run_test()
