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

# Check that set_daemon does not silently remove an existing daemon proxy.

import json
import socket
import socketserver
import subprocess
import sys
import tempfile
import threading
import time
import urllib.request
from pathlib import Path


class ConnectionRecorder(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True

    def __init__(self):
        super().__init__(("127.0.0.1", 0), self.Handler)
        self.connections = 0
        threading.Thread(target=self.serve_forever, daemon=True).start()

    class Handler(socketserver.BaseRequestHandler):
        def handle(self):
            self.server.connections += 1
            self.request.settimeout(1)
            try:
                self.request.recv(4096)
            except (OSError, TimeoutError):
                pass


def rpc(port, method, params=None):
    body = json.dumps({"jsonrpc": "2.0", "id": "0", "method": method,
                       "params": params or {}}).encode()
    request = urllib.request.Request(f"http://127.0.0.1:{port}/json_rpc", body,
                                     {"Content-Type": "application/json"})
    with urllib.request.urlopen(request, timeout=20) as response:
        return json.load(response)


def expect_ok(port, method, params=None):
    response = rpc(port, method, params)
    assert "error" not in response, (method, response)
    return response["result"]


def refresh_through(port, expected, others):
    before = expected.connections
    unchanged = [server.connections for server in others]
    rpc(port, "refresh")  # The dummy endpoint cannot serve blockchain data.
    assert expected.connections > before, "no request reached the expected endpoint"
    assert [server.connections for server in others] == unchanged, "request bypassed the expected endpoint"


def run(binary):
    with tempfile.TemporaryDirectory(prefix="monero-rpc-proxy-") as directory:
        first_proxy, second_proxy, direct = (ConnectionRecorder() for _ in range(3))
        servers = (first_proxy, second_proxy, direct)
        with socket.socket() as port_socket:
            port_socket.bind(("127.0.0.1", 0))
            rpc_port = port_socket.getsockname()[1]
        logfile = open(Path(directory) / "wallet-rpc.log", "w+")
        process = subprocess.Popen([
            binary, "--wallet-dir", directory, "--rpc-bind-ip", "127.0.0.1",
            "--rpc-bind-port", str(rpc_port), "--disable-rpc-login", "--rpc-ssl", "disabled",
            "--daemon-address", "http://example.onion:18081", "--daemon-ssl", "enabled",
            "--proxy", f"127.0.0.1:{first_proxy.server_address[1]}",
            "--log-level", "0",
        ], stdout=logfile, stderr=subprocess.STDOUT)
        try:
            for _ in range(100):
                if process.poll() is not None:
                    raise RuntimeError("wallet RPC exited during startup")
                try:
                    expect_ok(rpc_port, "get_version")
                    break
                except (OSError, TimeoutError):
                    time.sleep(.1)
            else:
                raise RuntimeError("wallet RPC did not start")

            expect_ok(rpc_port, "auto_refresh", {"enable": False})
            address = f"http://127.0.0.1:{direct.server_address[1]}"
            for invalid_address in ("https://example.org/path.onion:443",
                                    "https://example.org\0.onion:443"):
                assert "error" in rpc(rpc_port, "set_daemon", {"address": invalid_address})
            expect_ok(rpc_port, "set_daemon", {"address": "https://example.onion"})
            # An omitted SSL configuration retains the CLI TLS requirement.
            assert "error" in rpc(rpc_port, "set_daemon", {"address": address, "proxy": ""})
            params = {"address": address, "ssl_support": "disabled",
                      "ssl_allow_any_cert": True}
            expect_ok(rpc_port, "set_daemon", params)  # Proxy omitted.
            expect_ok(rpc_port, "set_daemon", params)  # Pending settings retained.
            expect_ok(rpc_port, "create_wallet", {"filename": "test", "password": "", "language": "English"})
            refresh_through(rpc_port, first_proxy, (second_proxy, direct))

            expect_ok(rpc_port, "close_wallet")
            expect_ok(rpc_port, "open_wallet", {"filename": "test", "password": ""})
            expect_ok(rpc_port, "set_daemon", params)  # Loaded wallet settings retained.
            refresh_through(rpc_port, first_proxy, (second_proxy, direct))

            # A wrong JSON type must not clear the existing proxy.
            assert "error" in rpc(rpc_port, "set_daemon", {**params, "proxy": 1})
            assert "error" in rpc(rpc_port, "set_daemon", {**params,
                                                            "ssl_allowed_fingerprints": [1]})
            refresh_through(rpc_port, first_proxy, (second_proxy, direct))

            expect_ok(rpc_port, "set_daemon", {**params, "proxy": ""})
            refresh_through(rpc_port, direct, (first_proxy, second_proxy))

            expect_ok(rpc_port, "set_daemon", {**params,
                                                "proxy": f"127.0.0.1:{second_proxy.server_address[1]}"})
            refresh_through(rpc_port, second_proxy, (first_proxy, direct))

            expect_ok(rpc_port, "set_daemon", {"address": "http://example.onion:18081",
                                                "proxy": "", "ssl_support": "enabled"})
            assert "error" in rpc(rpc_port, "set_daemon", {"address": address, "proxy": ""})
        finally:
            process.terminate()
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
            logfile.close()
            for server in servers:
                server.shutdown()
                server.server_close()


if __name__ == "__main__":
    run(sys.argv[1])
