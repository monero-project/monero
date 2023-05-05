#!/usr/bin/env python

from __future__ import print_function
import sys
import time
import subprocess
from signal import SIGTERM
import socket
import string
import os

USAGE = 'usage: functional_tests_rpc.py <python> <srcdir> <builddir> [<tests-to-run> | all]'
DEFAULT_TESTS = ['address_book', 'bans', 'blockchain', 'cold_signing', 'daemon_info', 'get_output_distribution', 'integrated_address', 'mining', 'multisig', 'p2p', 'proofs', 'sign_message', 'transfer', 'txpool', 'uri', 'validate_address', 'wallet']
try:
  python = sys.argv[1]
  srcdir = sys.argv[2]
  builddir = sys.argv[3]
except:
  print(USAGE)
  sys.exit(1)

try:
  sys.argv[4]
except:
  print(USAGE)
  print('Available tests: ' + string.join(DEFAULT_TESTS, ', '))
  print('Or run all with "all"')
  sys.exit(0)

try:
  tests = sys.argv[4:]
  if tests == ['all']:
    tests = DEFAULT_TESTS
except:
  tests = DEFAULT_TESTS

# a main offline monerod, does most of the tests
# a restricted RPC monerod setup with RPC payment
# two local online monerods connected to each other
N_MONERODS = 4

# 4 wallets connected to the main offline monerod
# 1 wallet connected to the first local online monerod
# 1 offline wallet
N_WALLETS = 6

WALLET_DIRECTORY = builddir + "/functional-tests-directory"
FUNCTIONAL_TESTS_DIRECTORY = builddir + "/tests/functional_tests"
DIFFICULTY = 10

monerod_base = [builddir + "/bin/monerod", "--regtest", "--fixed-difficulty", str(DIFFICULTY), "--no-igd", "--p2p-bind-port", "monerod_p2p_port", "--rpc-bind-port", "monerod_rpc_port", "--zmq-rpc-bind-port", "monerod_zmq_port", "--zmq-pub", "monerod_zmq_pub", "--non-interactive", "--disable-dns-checkpoints", "--check-updates", "disabled", "--rpc-ssl", "disabled", "--data-dir", "monerod_data_dir", "--log-level", "1"]
monerod_extra = [
  ["--offline"],
  ["--offline"],
  ["--add-exclusive-node", "127.0.0.1:18283"],
  ["--add-exclusive-node", "127.0.0.1:18282"],
]
wallet_base = [builddir + "/bin/monero-wallet-rpc", "--wallet-dir", WALLET_DIRECTORY, "--rpc-bind-port", "wallet_port", "--disable-rpc-login", "--rpc-ssl", "disabled", "--daemon-ssl", "disabled", "--log-level", "1", "--allow-mismatched-daemon-version"]
wallet_extra = [
  ["--daemon-port", "18180"],
  ["--daemon-port", "18180"],
  ["--daemon-port", "18180"],
  ["--daemon-port", "18180"],
  ["--daemon-port", "18182"],
  ["--offline"],
]

command_lines = []
processes = []
outputs = []
ports = []

for i in range(N_MONERODS):
  command_lines.append([str(18180+i) if x == "monerod_rpc_port" else str(18280+i) if x == "monerod_p2p_port" else str(18380+i) if x == "monerod_zmq_port" else "tcp://127.0.0.1:" + str(18480+i) if x == "monerod_zmq_pub" else builddir + "/functional-tests-directory/monerod" + str(i) if x == "monerod_data_dir" else x for x in monerod_base])
  if i < len(monerod_extra):
    command_lines[-1] += monerod_extra[i]
  outputs.append(open(FUNCTIONAL_TESTS_DIRECTORY + '/monerod' + str(i) + '.log', 'a+'))
  ports.append(18180+i)

for i in range(N_WALLETS):
  command_lines.append([str(18090+i) if x == "wallet_port" else x for x in wallet_base])
  if i < len(wallet_extra):
    command_lines[-1] += wallet_extra[i]
  outputs.append(open(FUNCTIONAL_TESTS_DIRECTORY + '/wallet' + str(i) + '.log', 'a+'))
  ports.append(18090+i)

print('Starting servers...')
try:
  PYTHONPATH = os.environ['PYTHONPATH'] if 'PYTHONPATH' in os.environ else ''
  if len(PYTHONPATH) > 0:
    PYTHONPATH += ':'
  PYTHONPATH += srcdir + '/../../utils/python-rpc'
  os.environ['PYTHONPATH'] = PYTHONPATH
  os.environ['WALLET_DIRECTORY'] = WALLET_DIRECTORY
  os.environ['FUNCTIONAL_TESTS_DIRECTORY'] = FUNCTIONAL_TESTS_DIRECTORY
  os.environ['SOURCE_DIRECTORY'] = srcdir
  os.environ['PYTHONIOENCODING'] = 'utf-8'
  os.environ['DIFFICULTY'] = str(DIFFICULTY)
  os.environ['MAKE_TEST_SIGNATURE'] = FUNCTIONAL_TESTS_DIRECTORY + '/make_test_signature'
  os.environ['SEEDHASH_EPOCH_BLOCKS'] = "8"
  os.environ['SEEDHASH_EPOCH_LAG'] = "4"
  if not 'MINING_SILENT' in os.environ:
    os.environ['MINING_SILENT'] = "1"

  for i in range(len(command_lines)):
    #print('Running: ' + str(command_lines[i]))
    processes.append(subprocess.Popen(command_lines[i], stdout = outputs[i]))
except Exception as e:
  print('Error: ' + str(e))
  sys.exit(1)

def kill():
  for i in range(len(processes)):
    try: processes[i].send_signal(SIGTERM)
    except: pass

# wait for error/startup
for i in range(10):
  time.sleep(1)
  all_open = True
  for port in ports:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(1)
    if s.connect_ex(('127.0.0.1', port)) != 0:
      all_open = False
      break
    s.close()
  if all_open:
    break

if not all_open:
  print('Failed to start wallet or daemon')
  kill()
  sys.exit(1)

# online daemons need some time to connect to peers to be ready
time.sleep(2)

PASS = []
FAIL = []
for test in tests:
  try:
    print('[TEST STARTED] ' + test)
    sys.stdout.flush()
    cmd = [python, srcdir + '/' + test + ".py"]
    subprocess.check_call(cmd)
    PASS.append(test)
    print('[TEST PASSED] ' + test)
  except:
    FAIL.append(test)
    print('[TEST FAILED] ' + test)
    pass

print('Stopping servers...')
kill()

# wait for exit, the poll method does not work (https://bugs.python.org/issue2475) so we wait, possibly forever if the process hangs
if True:
  for p in processes:
    p.wait()
else:
  for i in range(10):
    n_returncode = 0
    for p in processes:
      p.poll()
      if p.returncode:
        n_returncode += 1
    if n_returncode == len(processes):
      print('All done: ' + string.join([x.returncode for x in processes], ', '))
      break
    time.sleep(1)
  for p in processes:
    if not p.returncode:
      print('Failed to stop process')

if len(FAIL) == 0:
  print('Done, ' + str(len(PASS)) + '/' + str(len(tests)) + ' tests passed')
else:
  print('Done, ' + str(len(FAIL)) + '/' + str(len(tests)) + ' tests failed: ' + ', '.join(FAIL))

sys.exit(0 if len(FAIL) == 0 else 1)
