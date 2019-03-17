#!/usr/bin/env python

from __future__ import print_function
import sys
import time
import subprocess
from signal import SIGTERM
import socket
import string

USAGE = 'usage: functional_tests_rpc.py <python> <srcdir> <builddir> [<tests-to-run> | all]'
DEFAULT_TESTS = ['daemon_info', 'blockchain', 'wallet_address', 'integrated_address', 'mining', 'transfer']
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

monerod = [builddir + "/bin/monerod", "--regtest", "--fixed-difficulty", "1", "--offline", "--no-igd", "--non-interactive", "--disable-dns-checkpoints", "--check-updates", "disabled", "--rpc-ssl", "disabled", "--log-level", "1"]
wallet = [builddir + "/bin/monero-wallet-rpc", "--wallet-dir", builddir + "/functional-tests-directory", "--rpc-bind-port", "18083", "--disable-rpc-login", "--rpc-ssl", "disabled", "--daemon-ssl", "disabled", "--log-level", "1"]

monerod_output = open(builddir + '/tests/functional_tests/monerod.log', 'a+')
wallet_output = open(builddir + '/tests/functional_tests/wallet.log', 'a+')

print('Starting servers...')
monerod_process = None
wallet_process = None
try:
  #print 'Running: ' + str(monerod)
  monerod_process = subprocess.Popen(monerod, stdout = monerod_output)
  #print 'Running: ' + str(wallet)
  wallet_process = subprocess.Popen(wallet, stdout = wallet_output)
except Exception, e:
  print('Error: ' + str(e))
  sys.exit(1)

def kill():
  try: wallet_process.send_signal(SIGTERM)
  except: pass
  try: monerod_process.send_signal(SIGTERM)
  except: pass

# wait for error/startup
for i in range(10):
  time.sleep(1)
  all_open = True
  for port in [18081, 18083]:
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

PASS = []
FAIL = []
for test in tests:
  try:
    print('[TEST STARTED] ' + test)
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
  wallet_process.wait()
  monerod_process.wait()
else:
  for i in range(10):
    wallet_process.poll()
    monerod_process.poll()
    if wallet_process.returncode and monerod_process.returncode:
      print('Both done: ' + str(wallet_process.returncode) + ' and ' + str(monerod_process.returncode))
      break
    time.sleep(1)
  if not wallet_process.returncode:
    print('Failed to stop monero-wallet-rpc')
  if not monerod_process.returncode:
    print('Failed to stop monerod')

if len(FAIL) == 0:
  print('Done, ' + str(len(PASS)) + '/' + str(len(tests)) + ' tests passed')
else:
  print('Done, ' + str(len(FAIL)) + '/' + str(len(tests)) + ' tests failed: ' + string.join(FAIL, ', '))
