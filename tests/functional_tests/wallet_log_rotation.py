#!/usr/bin/env python3

import os
import signal
import socket
import subprocess
import sys
import tempfile
import time


def log_contains(path, text):
    if not os.path.isfile(path):
        return False
    with open(path) as log_file:
        return text in log_file.read()


if os.name != 'posix':
    sys.exit(0)

builddir = os.path.abspath(os.path.join(os.environ['FUNCTIONAL_TESTS_DIRECTORY'], '..', '..'))
wallet_rpc = os.path.join(builddir, 'bin', 'monero-wallet-rpc')

with tempfile.TemporaryDirectory(prefix='monero-wallet-rpc-log-') as directory:
    log = os.path.join(directory, 'wallet-rpc.log')
    rotated = log + '.1'
    wallet_dir = os.path.join(directory, 'wallets')

    with socket.socket() as sock:
        sock.bind(('127.0.0.1', 0))
        port = sock.getsockname()[1]

    process = subprocess.Popen([
        wallet_rpc, '--wallet-dir', wallet_dir, '--offline', '--disable-rpc-login',
        '--rpc-bind-ip', '127.0.0.1', '--rpc-bind-port', str(port),
        '--rpc-ssl', 'disabled', '--log-file', log, '--log-level', '1',
    ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
       env={**os.environ, 'LC_ALL': 'C'})
    try:
        deadline = time.monotonic() + 15
        while time.monotonic() < deadline:
            if process.poll() is not None:
                raise AssertionError('wallet RPC exited during startup')
            try:
                with socket.create_connection(('127.0.0.1', port), timeout=0.2):
                    break
            except OSError:
                time.sleep(0.1)
        else:
            raise AssertionError('wallet RPC did not start')

        os.rename(log, rotated)
        os.mkdir(log)
        process.send_signal(signal.SIGUSR1)

        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            if process.poll() is not None:
                raise AssertionError('SIGUSR1 stopped wallet RPC')
            if log_contains(rotated, 'Failed to reopen the log file after SIGUSR1'):
                break
            time.sleep(0.1)
        else:
            raise AssertionError('wallet RPC did not report the failed reopen')

        os.rmdir(log)
        process.send_signal(signal.SIGUSR1)
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            if process.poll() is not None:
                raise AssertionError('SIGUSR1 stopped wallet RPC')
            if log_contains(log, 'Reopened log file after SIGUSR1'):
                break
            time.sleep(0.1)
        else:
            raise AssertionError('SIGUSR1 did not reopen the log file')

        process.send_signal(signal.SIGTERM)
        if process.wait(timeout=10) != 0:
            raise AssertionError('wallet RPC did not stop cleanly')

        with open(rotated) as old_log, open(log) as new_log:
            old_text, new_text = old_log.read(), new_log.read()
        if 'Starting wallet RPC server' not in old_text or 'Stopped wallet RPC server' in old_text:
            raise AssertionError('wallet RPC wrote to the rotated file after reopening')
        if 'Stopped wallet RPC server' not in new_text:
            raise AssertionError('wallet RPC did not write to the new log file')
    finally:
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
