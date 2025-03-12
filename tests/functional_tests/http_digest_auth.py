#!/usr/bin/env python3

# Copyright (c) 2024-2025, The Monero Project
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

from framework.daemon import Daemon
from framework.wallet import Wallet

import time


DAEMON_IDX = 4
DAEMON_USER = "md5_lover"
DAEMON_PASS = "Z1ON0101"
WALLET_IDX = 6
WALLET_USER = "kyle"
WALLET_PASS = "reveille"
WALLET_SEED = "velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits \
               benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted"

class HttpDigestAuthTest():
    def run_test(self):
        self.test_daemon_login_required()
        self.test_wallet_login_required()

        self.make_daemon_conn()
        self.create_wallet()

        self.mine_through_wallet()

    def test_daemon_login_required(self):
        print('Attempting to connect to daemon loginless with RPC digest authentication required...')
        bad_daemon = Daemon(idx = DAEMON_IDX)
        try:
            res = bad_daemon.get_height()
            assert(False)
        except:
            pass
    
    def test_wallet_login_required(self):
        print('Attempting to connect to wallet server loginless with RPC digest authentication required...')
        bad_wallet = Wallet(idx = WALLET_IDX)
        try:
            res = bad_wallet.get_balance()
            assert(False)
        except:
            pass

    def make_daemon_conn(self):
        print('Connecting to daemon with RPC digest authentication required...')
        self.daemon = Daemon(idx = DAEMON_IDX, username = DAEMON_USER, password = DAEMON_PASS)
        res = self.daemon.get_height()
        self.daemon.pop_blocks(res.height - 1)
        self.daemon.flush_txpool()

    def create_wallet(self):
        print('Connecting to wallet server with RPC digest authentication required...')
        self.wallet = Wallet(idx = WALLET_IDX, username = WALLET_USER, password = WALLET_PASS)
        # close the wallet if any, will throw if none is loaded
        try: self.wallet.close_wallet()
        except: pass
        res = self.wallet.restore_deterministic_wallet(seed = WALLET_SEED)

    def mine_through_wallet(self):
        print('Telling login-required daemon to start mining through login-required wallet server...')
        start_height = self.daemon.get_height().height
        self.wallet.start_mining(2)

        print("Waiting a few seconds for mining to occur...")
        for tries in range(20):
            time.sleep(1)

            stop_height = self.daemon.get_height().height
            if stop_height > start_height:
                break

        print('Telling login-required daemon to stop mining through login-required wallet server...')
        self.wallet.stop_mining()

        num_blocks_mined = stop_height - start_height
        assert num_blocks_mined > 0
        print('Mined {} blocks!'.format(num_blocks_mined))

if __name__ == '__main__':
    HttpDigestAuthTest().run_test()
