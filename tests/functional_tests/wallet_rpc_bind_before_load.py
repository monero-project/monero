#!/usr/bin/env python3

import pathlib
import socket
import subprocess
import sys
import tempfile


def main():
    with tempfile.TemporaryDirectory() as directory, socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        listener.listen()

        command = [
            sys.argv[1],
            "--wallet-file", str(pathlib.Path(directory) / "missing-wallet"),
            "--password", "test",
            "--rpc-bind-ip", "127.0.0.1",
            "--rpc-bind-port", str(listener.getsockname()[1]),
            "--disable-rpc-login",
            "--offline",
            "--no-initial-sync",
            "--rpc-ssl", "disabled",
            "--daemon-ssl", "disabled",
            "--log-file", str(pathlib.Path(directory) / "wallet-rpc.log"),
        ]
        result = subprocess.run(command, cwd=directory, capture_output=True, text=True, timeout=15)
        output = result.stdout + result.stderr

        if result.returncode == 0 or "Failed to bind IPv4" not in output or "Loading wallet" in output:
            raise AssertionError(output)


if __name__ == "__main__":
    main()
