#!/usr/bin/env python
import os
import subprocess
import sys
import argparse


parser = argparse.ArgumentParser()
parser.add_argument("-d", "--debug-msg", default=False, action="store_const", const=True, help="Build debug messages")
args = parser.parse_args()

CWD = os.path.dirname(os.path.realpath(__file__))
ROOT_DIR = os.path.abspath(os.path.join(CWD, "..", "..", "..", ".."))
TREZOR_COMMON = os.path.join(ROOT_DIR, "external", "trezor-common")
TREZOR_MESSAGES = os.path.join(CWD, "..", "messages")

# check for existence of the submodule directory
common_defs = os.path.join(TREZOR_COMMON, "defs")
if not os.path.exists(common_defs):
    raise ValueError(
        "trezor-common submodule seems to be missing.\n"
        + 'Use "git submodule update --init --recursive" to retrieve it.'
    )

# regenerate messages
try:
    selected = [
        "messages.proto",
        "messages-common.proto",
        "messages-management.proto",
        "messages-monero.proto",
    ]

    if args.debug_msg:
        selected += ["messages-debug.proto"]

    proto_srcs = [os.path.join(TREZOR_COMMON, "protob", x) for x in selected]
    exec_args = [
        sys.executable,
        os.path.join(CWD, "pb2cpp.py"),
        "-o",
        TREZOR_MESSAGES,
    ] + proto_srcs

    subprocess.check_call(exec_args)

except Exception as e:
    raise
