#!/usr/bin/env python3

# Copyright (c) 2014-2024, The Monero Project
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

import json
import os
import shutil
import socket
import subprocess
import sys
import threading
import time
import urllib.error
import urllib.request
from signal import SIGTERM


USAGE = "usage: libwallet_api_tests.py <builddir> <libwallet_api_tests_exe>"
MINED_BLOCKS = 90


def reserve_ports(count):
    sockets = []
    ports = []
    try:
        for _ in range(count):
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.bind(("127.0.0.1", 0))
            sockets.append(s)
            ports.append(s.getsockname()[1])
    finally:
        for s in sockets:
            s.close()
    return ports


def wait_for_port(port, timeout=20):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(0.2)
            if s.connect_ex(("127.0.0.1", port)) == 0:
                return
        time.sleep(0.1)
    raise RuntimeError("timed out waiting for port {}".format(port))


def rpc_json(rpc_port, path, payload, timeout=30):
    data = json.dumps(payload).encode("utf-8")
    request = urllib.request.Request(
        "http://127.0.0.1:{}{}".format(rpc_port, path),
        data=data,
        headers={"Content-Type": "application/json"},
    )
    with urllib.request.urlopen(request, timeout=timeout) as response:
        return json.loads(response.read().decode("utf-8"))


def get_height(rpc_port):
    response = rpc_json(rpc_port, "/get_height", {})
    return int(response["height"])


def wait_for_height(rpc_port, height, timeout=180):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            current_height = get_height(rpc_port)
            if current_height >= height:
                return current_height
        except (urllib.error.URLError, TimeoutError, KeyError):
            pass
        time.sleep(0.5)
    raise RuntimeError("timed out waiting for regtest height {}".format(height))


def generate_blocks(rpc_port, address, blocks):
    response = rpc_json(
        rpc_port,
        "/json_rpc",
        {
            "jsonrpc": "2.0",
            "id": "0",
            "method": "generateblocks",
            "params": {
                "wallet_address": address,
                "amount_of_blocks": blocks,
            },
        },
    )
    if "error" in response:
        raise RuntimeError("generateblocks failed: {}".format(response["error"]))
    return response


def pulse_blocks(stop_event, rpc_port, address):
    while not stop_event.wait(1.0):
        try:
            generate_blocks(rpc_port, address, 1)
        except Exception as e:
            print("Failed to generate regtest block: {}".format(e))
            sys.stdout.flush()
            return


def stop_process(process):
    if process.poll() is not None:
        return
    try:
        process.send_signal(SIGTERM)
        process.wait(timeout=20)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait()


def main():
    if len(sys.argv) != 3:
        print(USAGE)
        return 1

    builddir = os.path.abspath(sys.argv[1])
    test_exe = os.path.abspath(sys.argv[2])
    bin_dir = os.path.join(builddir, "bin")
    monerod = os.path.join(bin_dir, "monerod")

    for executable in (monerod, test_exe):
        if not os.path.exists(executable):
            print("missing executable: {}".format(executable))
            return 1

    run_dir = os.path.join(builddir, "libwallet-api-tests-directory")
    wallets_dir = os.path.join(run_dir, "wallets")
    ringdb_dir = os.path.join(run_dir, "ringdb")
    data_dir = os.path.join(run_dir, "regtest")
    log_dir = os.path.join(builddir, "tests", "libwallet_api_tests")

    shutil.rmtree(run_dir, ignore_errors=True)
    os.makedirs(wallets_dir)
    os.makedirs(ringdb_dir)
    os.makedirs(data_dir)
    os.makedirs(log_dir, exist_ok=True)

    rpc_port, p2p_port, zmq_rpc_port, zmq_pub_port = reserve_ports(4)
    daemon_log_path = os.path.join(log_dir, "monerod-regtest.log")
    daemon_log = open(daemon_log_path, "ab")
    daemon = None
    block_pulse_stop = threading.Event()
    block_pulse = None

    try:
        daemon = subprocess.Popen(
            [
                monerod,
                "--regtest",
                "--fixed-difficulty",
                "1",
                "--p2p-bind-ip",
                "127.0.0.1",
                "--p2p-bind-port",
                str(p2p_port),
                "--rpc-bind-ip",
                "127.0.0.1",
                "--rpc-bind-port",
                str(rpc_port),
                "--zmq-rpc-bind-ip",
                "127.0.0.1",
                "--zmq-rpc-bind-port",
                str(zmq_rpc_port),
                "--zmq-pub",
                "tcp://127.0.0.1:{}".format(zmq_pub_port),
                "--non-interactive",
                "--offline",
                "--disable-dns-checkpoints",
                "--check-updates",
                "disabled",
                "--rpc-ssl",
                "disabled",
                "--data-dir",
                data_dir,
                "--log-level",
                "1",
            ],
            stdout=daemon_log,
            stderr=subprocess.STDOUT,
        )
        wait_for_port(rpc_port)

        env = os.environ.copy()
        env["WALLETS_ROOT_DIR"] = wallets_dir
        env["DAEMON_ADDRESS"] = "127.0.0.1:{}".format(rpc_port)
        env["LIBWALLET_API_TESTS_RINGDB_DIR"] = ringdb_dir

        generated = subprocess.check_output(
            [test_exe, "--generate-test-wallets"],
            cwd=run_dir,
            env=env,
            text=True,
        )
        generated_values = dict(
            line.split("=", 1)
            for line in generated.splitlines()
            if line.startswith(("miner_address=", "pulse_miner_address="))
        )
        miner_address = generated_values["miner_address"].strip()
        pulse_miner_address = generated_values["pulse_miner_address"].strip()
        response = generate_blocks(rpc_port, miner_address, MINED_BLOCKS)
        print("Generated regtest blocks to {}: {}".format(miner_address, response))
        print("Current regtest height: {}".format(get_height(rpc_port)))
        sys.stdout.flush()
        wait_for_height(rpc_port, MINED_BLOCKS)

        env.setdefault("GTEST_COLOR", "yes")
        gtest_filter = env.get("LIBWALLET_API_TESTS_GTEST_FILTER", "*")

        block_pulse = threading.Thread(target=pulse_blocks, args=(block_pulse_stop, rpc_port, pulse_miner_address))
        block_pulse.start()
        return subprocess.call(
            [test_exe, "--gtest_filter={}".format(gtest_filter)],
            cwd=run_dir,
            env=env,
        )
    finally:
        block_pulse_stop.set()
        if block_pulse is not None:
            block_pulse.join(timeout=10)
        if daemon is not None:
            stop_process(daemon)
        daemon_log.close()


if __name__ == "__main__":
    sys.exit(main())
