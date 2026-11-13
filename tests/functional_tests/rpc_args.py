#!/usr/bin/env python3

# Copyright (c) 2026, The Monero Project
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

"""Test RPC server command line options
"""

import requests
from framework import epee_binary
from framework.wallet import Wallet

import hashlib
import os
import socket
import ssl
import subprocess
import tempfile
import time

BIN_DIR = os.path.join(os.environ['FUNCTIONAL_TESTS_DIRECTORY'], '..', '..', 'bin')

def monerod_cmd(idx, data_dir, extra = []):
    return [os.path.join(BIN_DIR, 'monerod'), '--regtest', '--offline', '--non-interactive',
            '--rpc-bind-port', str(18170 + idx), '--p2p-bind-port', str(18270 + idx),
            '--zmq-rpc-bind-port', str(18370 + idx), '--rpc-ssl', 'disabled',
            '--data-dir', os.path.join(data_dir, str(idx))] + extra

def wallet_cmd(data_dir, extra = []):
    return [os.path.join(BIN_DIR, 'monero-wallet-rpc'), '--rpc-bind-port', '18099',
            '--rpc-ssl', 'disabled', '--disable-rpc-login', '--wallet-dir', data_dir,
            '--log-file', os.path.join(data_dir, 'wallet.log')] + extra

class Servers():
    def __init__(self, data_dir):
        self.data_dir = data_dir
        self.processes = []

    def __enter__(self):
        return self

    def __exit__(self, *args):
        for p in self.processes:
            if p.poll() is None:
                p.terminate()
        for p in self.processes:
            try:
                p.wait(timeout=10)
            except subprocess.TimeoutExpired:
                p.kill()
                p.wait()

    def start(self, cmd, port):
        env = os.environ.copy()
        env.pop('RPC_LOGIN', None)
        log_path = os.path.join(os.environ['FUNCTIONAL_TESTS_DIRECTORY'], 'rpc_args_{}.log'.format(port))
        with open(log_path, 'a') as log:
            p = subprocess.Popen(cmd, stdout=log, stderr=log, env=env)
        self.processes.append(p)
        deadline = time.monotonic() + 30
        while True:
            try:
                with socket.create_connection(('127.0.0.1', port), timeout=1):
                    return p
            except OSError:
                assert p.poll() is None
                assert time.monotonic() < deadline
                time.sleep(.1)

class RpcArgsTest():
    def run_test(self):
        self.test_access_control_origins()
        self.test_confirm_external_bind()
        self.test_rpc_ban()
        self.test_untrusted_daemon()
        self.test_daemon_ssl_fingerprints()

    def test_access_control_origins(self):
        print('Testing --rpc-access-control-origins')
        with tempfile.TemporaryDirectory() as data_dir, Servers(data_dir) as servers:
            # unsorted with whitespace, to check the list is trimmed and sorted
            servers.start(monerod_cmd(0, data_dir, ['--rpc-access-control-origins', 'http://b.example, http://a.example']), 18170)
            servers.start(monerod_cmd(1, data_dir), 18171)
            servers.start(wallet_cmd(data_dir, ['--offline', '--rpc-access-control-origins', '*']), 18099)

            def request(port, origin = None, method = 'POST'):
                headers = {'Origin': origin} if origin else {}
                payload = {'jsonrpc': '2.0', 'id': '0', 'method': 'get_version'}
                return requests.request(method, 'http://127.0.0.1:{}/json_rpc'.format(port),
                                        json=payload, headers=headers, timeout=5)

            for origin in ('http://a.example', 'http://b.example'):
                res = request(18170, origin)
                assert res.status_code == 200
                assert res.headers.get('Access-Control-Allow-Origin') == origin
                assert 'POST' in res.headers.get('Access-Control-Allow-Methods', '')
                assert 'Access-Control-Allow-Headers' not in res.headers

                res = request(18170, origin, 'OPTIONS')
                assert res.headers.get('Access-Control-Allow-Origin') == origin
                assert 'Authorization' in res.headers.get('Access-Control-Allow-Headers', '')

            for port, origin in ((18170, 'http://c.example'), (18170, 'http://a.example.evil'),
                                 (18170, None), (18171, 'http://a.example'), (18099, None)):
                res = request(port, origin)
                assert res.status_code == 200
                assert not any(h.lower().startswith('access-control-') for h in res.headers)

            res = request(18099, 'http://anything.example')
            assert res.status_code == 200
            assert res.headers.get('Access-Control-Allow-Origin') == 'http://anything.example'

    def test_confirm_external_bind(self):
        print('Testing --confirm-external-bind')
        error = ' permits inbound unencrypted external connections'
        with tempfile.TemporaryDirectory() as data_dir:
            for cmd, arg in ((monerod_cmd(2, data_dir, ['--rpc-bind-ip', '0.0.0.0']), '--rpc-bind-ip'),
                             (monerod_cmd(2, data_dir, ['--rpc-use-ipv6', '--rpc-bind-ipv6-address', '::']), '--rpc-bind-ipv6-address'),
                             (wallet_cmd(data_dir, ['--offline', '--rpc-bind-ip', '0.0.0.0']), '--rpc-bind-ip')):
                result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                        universal_newlines=True, timeout=30)
                assert result.returncode != 0
                assert arg + error in result.stdout

            with Servers(data_dir) as servers:
                servers.start(monerod_cmd(2, data_dir, ['--rpc-bind-ip', '0.0.0.0', '--confirm-external-bind']), 18172)
                servers.start(wallet_cmd(data_dir, ['--offline', '--rpc-bind-ip', '0.0.0.0', '--confirm-external-bind']), 18099)

    def test_rpc_ban(self):
        print('Testing RPC bans and --disable-rpc-ban')
        # no genesis in block_ids, so the lookup fails and counts against the host
        bad_request = epee_binary.Serializer().serialize({'block_ids': bytes(32), 'start_height': 0})

        def get_hashes(port):
            res = requests.post('http://127.0.0.1:{}/gethashes.bin'.format(port), data=bad_request,
                                headers={'content-type': 'application/octet-stream', 'Connection': 'close'},
                                stream=True, timeout=5)
            assert res.status_code == 200
            return epee_binary.Deserializer(res.raw).deserialize()['status']

        def get_version(port):
            payload = {'jsonrpc': '2.0', 'id': '0', 'method': 'get_version'}
            return requests.post('http://127.0.0.1:{}/json_rpc'.format(port), json=payload,
                                 headers={'Connection': 'close'}, timeout=5)

        with tempfile.TemporaryDirectory() as data_dir, Servers(data_dir) as servers:
            servers.start(monerod_cmd(3, data_dir), 18173)
            servers.start(monerod_cmd(4, data_dir, ['--disable-rpc-ban']), 18174)

            for port in (18173, 18174):
                assert get_version(port).status_code == 200
                for i in range(4):
                    assert get_hashes(port) == b'Failed'

            assert get_version(18174).status_code == 200
            banned = False
            try:
                get_version(18173)
            except requests.exceptions.ConnectionError:
                banned = True
            assert banned

    def test_untrusted_daemon(self):
        print('Testing wallet RPC commands that require a trusted daemon')
        seed = 'velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted'
        with tempfile.TemporaryDirectory() as data_dir, Servers(data_dir) as servers:
            servers.start(monerod_cmd(5, data_dir), 18175)
            servers.start(wallet_cmd(data_dir, ['--daemon-port', '18175', '--untrusted-daemon']), 18099)
            wallet = Wallet(port = 18099)
            wallet.restore_deterministic_wallet(seed = seed)

            for call in (wallet.rescan_spent, lambda: wallet.start_mining(1),
                         lambda: wallet.import_key_images([])):
                refused = False
                try:
                    call()
                except AssertionError as e:
                    assert 'This command requires a trusted daemon' in str(e)
                    refused = True
                assert refused

            wallet.set_daemon('127.0.0.1:18175', trusted = True)
            wallet.rescan_spent()
            wallet.close_wallet()

    def test_daemon_ssl_fingerprints(self):
        print('Testing wallet SSL verification of the daemon')
        seed = 'velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted'
        with tempfile.TemporaryDirectory() as data_dir, Servers(data_dir) as servers:
            cmd = monerod_cmd(6, data_dir)
            cmd[cmd.index('--rpc-ssl') + 1] = 'enabled'
            servers.start(cmd, 18176)
            # daemon generates a self-signed certificate
            pem = ssl.get_server_certificate(('127.0.0.1', 18176))
            fingerprint = hashlib.sha256(ssl.PEM_cert_to_DER_cert(pem)).hexdigest()
            wrong = '00' * 32

            servers.start(wallet_cmd(data_dir, ['--daemon-port', '18176', '--daemon-ssl', 'enabled',
                                                '--daemon-ssl-allowed-fingerprints', fingerprint]), 18099)
            wallet = Wallet(port = 18099)
            wallet.restore_deterministic_wallet(seed = seed)

            def connects():
                try:
                    wallet.refresh()
                    return True
                except AssertionError:
                    return False

            assert connects()
            address = '127.0.0.1:18176'
            wallet.set_daemon(address, ssl_support = 'enabled', ssl_allowed_fingerprints = [wrong])
            assert not connects()
            refused = False
            try:
                wallet.set_daemon(address, ssl_support = 'enabled')
            except AssertionError as e:
                assert 'no strong verification was configured' in str(e)
                refused = True
            assert refused
            wallet.set_daemon(address, ssl_support = 'enabled', ssl_allowed_fingerprints = [wrong, fingerprint])
            assert connects()
            wallet.set_daemon(address, ssl_support = 'enabled', ssl_allow_any_cert = True)
            assert connects()
            wallet.close_wallet()

if __name__ == '__main__':
    RpcArgsTest().run_test()
