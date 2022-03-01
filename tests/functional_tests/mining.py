#!/usr/bin/env python3

# Copyright (c) 2018-2022, The Monero Project

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
import time
import os
import math
import monotonic
import util_resources
import multiprocessing

"""Test daemon mining RPC calls

Test the following RPCs:
    - start_mining
    - stop_mining
    - mining_status
    
Control the behavior with these environment variables:
    MINING_NO_MEASUREMENT - set to anything to use large enough and fixed mining timeouts
    MINING_SILENT         - set to anything to disable mining logging
"""

from framework.daemon import Daemon
from framework.wallet import Wallet

class MiningTest():
    def run_test(self):
        self.reset()
        self.create()
        self.mine(True)
        self.mine(False)
        self.submitblock()
        self.reset()
        self.test_randomx()

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
        res = wallet.restore_deterministic_wallet(seed = 'velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted')

    def mine(self, via_daemon):
        print("Test mining via " + ("daemon" if via_daemon else "wallet"))

        cores_init = multiprocessing.cpu_count() # RX init uses all cores
        cores_mine = 1                           # Mining uses a parametric number of cores
        is_mining_measurent = 'MINING_NO_MEASUREMENT' not in os.environ

        if is_mining_measurent: # A dynamic calculation of the CPU power requested
            time_pi_single_cpu = self.measure_cpu_power_get_time(cores_mine)
            time_pi_all_cores = self.measure_cpu_power_get_time(cores_init)
        # This is the last measurement, since it takes very little time and can be placed timewise-closer to the mining itself.
        available_ram = self.get_available_ram() # So far no ideas how to use this var, other than printing it

        start = monotonic.monotonic()
        daemon = Daemon()
        wallet = Wallet()

        # check info/height/balance before generating blocks
        res_info = daemon.get_info()
        initial_height = res_info.height
        res_getbalance = wallet.get_balance()
        prev_balance = res_getbalance.balance

        res_status = daemon.mining_status()

        if via_daemon:
            res = daemon.start_mining('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', threads_count = 1)
        else:
            res = wallet.start_mining(threads_count = cores_mine)

        res_status = daemon.mining_status()
        assert res_status.active == True
        assert res_status.threads_count == cores_mine
        assert res_status.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
        assert res_status.is_background_mining_enabled == False
        assert res_status.block_reward >= 600000000000

        # wait till we mined a few of them
        target_height = initial_height + 5
        height = initial_height
        
        if not is_mining_measurent:
            timeout_init = 600
            timeout_mine = 300
        else:
            """
            Randomx init has high variance on CI machines due to noisy neighbors,
            taking up resources in parallel (including by our own jobs).
            
            Mining is organized in the following scheme:
                1) first loop's pass: RandomX init and mining
                2) every next pass:   only mining
            Pass 1) takes much more time than pass 2)
            Pass 1) uses all cores, pass 2) just one (currently)
            For the above reasons both passes need separate timeouts and adjustments.
            After the first pass, the timeout is being reset to a lower value.
            """

            def calc_timeout(seconds_constant, time_pi, cores):
                """
                The time it took to calculate pi under certain conditions
                is proportional to the time it will take to calculate the real job.

                The number of cores used decreases the time almost linearly.
                """
                timeout = float(seconds_constant) * time_pi / float(cores)
                return timeout

            timeout_base_init = 60 # RX init needs more time
            timeout_base_mine = 20
            timeout_init = calc_timeout(timeout_base_init, time_pi_all_cores,  cores_init)
            timeout_mine = calc_timeout(timeout_base_mine, time_pi_single_cpu, cores_mine)
        
        msg_timeout_src = "adjusted for the currently available CPU power" if is_mining_measurent else "selected to have the default value"
        msg = "Timeout for {} {}, is {:.1f} s"
        self.print_mining_info(msg.format("init,  ", msg_timeout_src, timeout_init))
        self.print_mining_info(msg.format("mining,", msg_timeout_src, timeout_mine))
        timeout = timeout_init
        rx_inited = False  # Gets initialized in the first pass of the below loop
        while height < target_height:
            seen_height = height
            for _ in range(int(math.ceil(timeout))):
                time.sleep(1)
                seconds_passed = monotonic.monotonic() - start
                height = daemon.get_info().height
                if height > seen_height:
                    break
            else:
                assert False, 'Failed to mine successor to block %d (initial block = %d) after %d s. RX initialized = %r' % (seen_height, initial_height, round(seconds_passed), rx_inited)
            if not rx_inited:
                rx_inited = True
                timeout = timeout_mine # Resetting the timeout after first mined block and RX init
                self.print_time_taken(start, "RX init + mining 1st block")
            else:
                self.print_time_taken(start, "mining iteration")

        self.print_time_taken(start, "mining total")

        if via_daemon:
            res = daemon.stop_mining()
        else:
            res = wallet.stop_mining()

        res_status = daemon.mining_status()
        assert res_status.active == False

        res_info = daemon.get_info()
        new_height = res_info.height

        wallet.refresh()
        res_getbalance = wallet.get_balance()
        balance = res_getbalance.balance
        assert balance >= prev_balance + (new_height - initial_height) * 600000000000

        if via_daemon:
            res = daemon.start_mining('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', threads_count = 1, do_background_mining = True)
        else:
            res = wallet.start_mining(threads_count = 1, do_background_mining = True)
        res_status = daemon.mining_status()
        assert res_status.active == True
        assert res_status.threads_count == 1
        assert res_status.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
        assert res_status.is_background_mining_enabled == True
        assert res_status.block_reward >= 600000000000

        # don't wait, might be a while if the machine is busy, which it probably is
        if via_daemon:
            res = daemon.stop_mining()
        else:
            res = wallet.stop_mining()
        res_status = daemon.mining_status()
        assert res_status.active == False

    def measure_cpu_power_get_time(self, cores):
        self.print_mining_info("Measuring the currently available CPU power...")
        build_dir_funcional_tests = os.environ['FUNCTIONAL_TESTS_DIRECTORY']
        time_pi = util_resources.get_time_pi_seconds(cores, build_dir_funcional_tests)
        self.print_mining_info("Time taken to calculate Pi on {} core(s) was {:.2f} s.".format(cores, time_pi))
        return time_pi

    def get_available_ram(self):
        available_ram = util_resources.available_ram_gb()
        threshold_ram = 3
        self.print_mining_info("Available RAM = " + str(round(available_ram, 1)) + " GB")
        if available_ram < threshold_ram:
            print("Warning! Available RAM =", round(available_ram, 1), 
                  "GB is less than the reasonable threshold =", threshold_ram,
                  ". The RX init might exceed the calculated timeout.")
        return available_ram

    def submitblock(self):
        print("Test submitblock")

        daemon = Daemon()
        res = daemon.get_height()
        height = res.height
        res = daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 5)
        assert len(res.blocks) == 5
        hashes = res.blocks
        blocks = []
        for block_hash in hashes:
            res = daemon.getblock(hash = block_hash)
            assert len(res.blob) > 0 and len(res.blob) % 2 == 0
            blocks.append(res.blob)
        res = daemon.get_height()
        assert res.height == height + 5
        res = daemon.pop_blocks(5)
        res = daemon.get_height()
        assert res.height == height
        for i in range(len(hashes)):
            block_hash = hashes[i]
            assert len(block_hash) == 64
            res = daemon.submitblock(blocks[i])
            res = daemon.get_height()
            assert res.height == height + i + 1
            assert res.hash == block_hash

    def is_mining_silent(self):
        return 'MINING_SILENT' in os.environ

    def print_mining_info(self, msg):
        if self.is_mining_silent():
            return
        print(msg)

    def print_time_taken(self, start, msg_context):
        if self.is_mining_silent():
            return
        seconds_passed = monotonic.monotonic() - start
        print("Time taken for", msg_context, "=", round(seconds_passed, 1), "s.")

    def test_randomx(self):
        print("Test RandomX")

        daemon = Daemon()
        wallet = Wallet()

        res = daemon.get_height()
        daemon.pop_blocks(res.height - 1)
        daemon.flush_txpool()

        epoch = int(os.environ['SEEDHASH_EPOCH_BLOCKS'])
        lag = int(os.environ['SEEDHASH_EPOCH_LAG'])
        address = '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'

        # check we can generate blocks, and that the seed hash changes when expected
        res = daemon.getblocktemplate(address)
        first_seed_hash = res.seed_hash
        daemon.generateblocks(address, 1 + lag)
        res = daemon.mining_status()
        assert res.active == False
        assert res.pow_algorithm == 'RandomX'
        res = daemon.getblocktemplate(address)
        seed_hash = res.seed_hash
        t0 = time.time()
        daemon.generateblocks(address, epoch - 3)
        t0 = time.time() - t0
        res = daemon.get_info()
        assert res.height == lag + epoch - 1
        res = daemon.getblocktemplate(address)
        assert seed_hash == res.seed_hash
        t0 = time.time()
        daemon.generateblocks(address, 1)
        t0 = time.time() - t0
        res = daemon.get_info()
        assert res.height == lag + epoch
        daemon.generateblocks(address, 1)
        res = daemon.getblocktemplate(address)
        assert seed_hash != res.seed_hash
        new_seed_hash = res.seed_hash
        t0 = time.time()
        daemon.generateblocks(address, epoch - 1)
        t0 = time.time() - t0
        res = daemon.getblocktemplate(address)
        assert new_seed_hash == res.seed_hash
        daemon.generateblocks(address, 1)
        res = daemon.getblocktemplate(address)
        assert new_seed_hash != res.seed_hash
        new_seed_hash = res.seed_hash
        t0 = time.time()
        daemon.generateblocks(address, epoch - 1)
        t0 = time.time() - t0
        res = daemon.getblocktemplate(address)
        assert new_seed_hash == res.seed_hash
        daemon.generateblocks(address, 1)
        res = daemon.getblocktemplate(address)
        assert new_seed_hash != res.seed_hash
        #print('First mining: ' + str(t0))

        # pop all these blocks, and feed them again to monerod
        print('Recreating the chain')
        res = daemon.get_info()
        height = res.height
        assert height == lag + epoch * 3 + 1
        block_hashes = [x.hash for x in daemon.getblockheadersrange(0, height - 1).headers]
        assert len(block_hashes) == height
        blocks = []
        for i in range(len(block_hashes)):
            res = daemon.getblock(height = i)
            assert res.block_header.hash == block_hashes[i]
            blocks.append(res.blob)
        daemon.pop_blocks(height)
        res = daemon.get_info()
        assert res.height == 1
        res = daemon.getblocktemplate(address)
        assert first_seed_hash == res.seed_hash
        t0 = time.time()
        for h in range(len(block_hashes)):
            res = daemon.submitblock(blocks[h])
        t0 = time.time() - t0
        res = daemon.get_info()
        assert height == res.height
        res = daemon.getblocktemplate(address)
        assert new_seed_hash != res.seed_hash
        res = daemon.pop_blocks(1)
        res = daemon.getblocktemplate(address)
        assert new_seed_hash == res.seed_hash
        #print('Submit: ' + str(t0))

        # start mining from the genesis block again
        print('Mining from genesis block again')
        res = daemon.get_height()
        top_hash = res.hash
        res = daemon.getblockheaderbyheight(0)
        genesis_block_hash = res.block_header.hash
        t0 = time.time()
        daemon.generateblocks(address, height - 2, prev_block = genesis_block_hash)
        t0 = time.time() - t0
        res = daemon.get_info()
        assert res.height == height - 1
        assert res.top_block_hash == top_hash
        #print('Second mining: ' + str(t0))

        # that one will cause a huge reorg
        print('Adding one to reorg')
        res = daemon.generateblocks(address, 1)
        assert len(res.blocks) == 1
        new_top_hash = res.blocks[0]
        res = daemon.get_info()
        assert res.height == height
        assert res.top_block_hash == new_top_hash


class Guard:
    def __enter__(self):
        pass

    def __exit__(self, exc_type, exc_value, traceback):
        daemon = Daemon()
        try: daemon.stop_mining()
        except: pass

if __name__ == '__main__':
    with Guard() as guard:
        MiningTest().run_test()
