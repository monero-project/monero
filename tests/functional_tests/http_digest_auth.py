#!/usr/bin/env python3

# Copyright (c) 2024-2024, The Monero Project
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
import requests
from requests.auth import HTTPDigestAuth

import os
import socket
import subprocess
import tempfile
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
        self.test_restricted_no_login_requires_port()
        self.test_restricted_daemon_login()
        self.test_wallet_login_required()

        self.make_daemon_conn()
        self.create_wallet()

        self.scan_wallet()

    def test_daemon_login_required(self):
        print('Attempting to connect to daemon loginless with RPC digest authentication required...')
        bad_daemon = Daemon(idx = DAEMON_IDX)
        rejected = False
        try:
            res = bad_daemon.get_height()
        except:
            rejected = True
        assert(rejected)

    def test_restricted_daemon_login(self):
        monerod = os.path.join(os.environ['FUNCTIONAL_TESTS_DIRECTORY'],
                               '..', '..', 'bin', 'monerod')
        processes = []
        with tempfile.TemporaryDirectory() as data_dir:
            try:
                for idx in (5, 6):
                    cmd = [monerod, '--regtest', '--offline', '--non-interactive',
                           '--rpc-bind-port', str(18180 + idx),
                           '--rpc-restricted-bind-port', str(18190 + idx),
                           '--rpc-restricted-no-login', '--p2p-bind-port', str(18280 + idx),
                           '--zmq-rpc-bind-port', str(18380 + idx), '--rpc-ssl', 'disabled',
                           '--data-dir', os.path.join(data_dir, str(idx))]
                    env = os.environ.copy()
                    env.pop('RPC_LOGIN', None)
                    if idx == 5:
                        env['RPC_LOGIN'] = 'env_lover:Z1ON0101'
                    else:
                        cmd += ['--rpc-login', 'md5_lover:Z1ON0101', '--restricted-rpc']
                    log_path = os.path.join(os.environ['FUNCTIONAL_TESTS_DIRECTORY'],
                                            'monerod_rpc_auth_{}.log'.format(idx))
                    with open(log_path, 'a') as log:
                        processes.append(subprocess.Popen(cmd, stdout=log, stderr=log, env=env))

                deadline = time.monotonic() + 20
                for port in (18185, 18195, 18186, 18196):
                    while True:
                        try:
                            with socket.create_connection(('127.0.0.1', port), timeout=1):
                                break
                        except OSError:
                            assert all(p.poll() is None for p in processes)
                            assert time.monotonic() < deadline
                            time.sleep(.1)

                self.check_restricted_daemon_login()
            finally:
                for p in processes:
                    if p.poll() is None:
                        p.terminate()
                for p in processes:
                    try:
                        p.wait(timeout=10)
                    except subprocess.TimeoutExpired:
                        p.kill()
                        p.wait()

    def check_restricted_daemon_login(self):
        payload = {'jsonrpc': '2.0', 'id': '0', 'method': 'get_block_count'}

        def status(port, auth=None):
            url = 'http://127.0.0.1:{}/json_rpc'.format(port)
            return requests.post(url, json=payload, auth=auth, timeout=5).status_code

        for port in (18184, 18194, 18185, 18186):
            assert status(port) == 401

        for port in (18184, 18194, 18186):
            assert status(port, HTTPDigestAuth(DAEMON_USER, DAEMON_PASS)) == 200

        assert status(18185, HTTPDigestAuth('env_lover', DAEMON_PASS)) == 200
        assert status(18195) == 200
        assert status(18196) == 200

        payload['method'] = 'get_connections'
        main = requests.post('http://127.0.0.1:18185/json_rpc', json=payload,
                             auth=HTTPDigestAuth('env_lover', DAEMON_PASS), timeout=5)
        assert main.status_code == 200 and 'result' in main.json()
        restricted = requests.post('http://127.0.0.1:18195/json_rpc', json=payload, timeout=5)
        assert restricted.status_code != 200 or 'error' in restricted.json()

    def test_restricted_no_login_requires_port(self):
        monerod = os.path.join(os.environ['FUNCTIONAL_TESTS_DIRECTORY'],
                               '..', '..', 'bin', 'monerod')
        with tempfile.TemporaryDirectory() as data_dir:
            result = subprocess.run([monerod, '--regtest', '--offline', '--non-interactive',
                                     '--rpc-restricted-no-login', '--rpc-bind-port', '18187',
                                     '--p2p-bind-port', '18287', '--zmq-rpc-bind-port', '18387',
                                     '--rpc-ssl', 'disabled', '--data-dir', data_dir],
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                    universal_newlines=True, timeout=30)
        assert result.returncode != 0
        assert '--rpc-restricted-no-login requires --rpc-restricted-bind-port' in result.stdout

    def test_wallet_login_required(self):
        print('Attempting to connect to wallet server loginless with RPC digest authentication required...')
        bad_wallet = Wallet(idx = WALLET_IDX)
        rejected = False
        try:
            res = bad_wallet.get_balance()
        except:
            rejected = True
        assert(rejected)

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
        self.wallet_address = res.address
        self.wallet.auto_refresh(False)

    def scan_wallet(self):
        print('Telling login-required wallet server to rescan blockchain from login-required daemon...')
        self.wallet.refresh()
        h1 = self.daemon.get_height().height
        assert(self.wallet.get_height().height == h1)
        self.daemon.generateblocks(self.wallet_address, 1)
        h2 = self.daemon.get_height().height
        assert(h2 > h1)
        assert(self.wallet.get_height().height == h1)
        self.wallet.rescan_blockchain(hard = True)
        assert(self.wallet.get_height().height == h2)

if __name__ == '__main__':
    HttpDigestAuthTest().run_test()
