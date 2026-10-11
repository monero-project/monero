#!/usr/bin/env python3

import sys
import re

USAGE = 'usage: check_untested_methods.py <rootdir>'
try:
  rootdir = sys.argv[1]
except:
  print(USAGE)
  sys.exit(1)

sys.path.insert(0, rootdir + '/utils/python-rpc')

from framework import daemon
from framework import wallet

modules = [
  {
    'name': 'daemon',
    'object': daemon.Daemon(),
    'path': rootdir + '/src/rpc/core_rpc_server.cpp',
    'ignore': []
  },
  {
    'name': 'wallet',
    'object': wallet.Wallet(),
    'path': rootdir + '/src/wallet/wallet_rpc_server.cpp',
    'ignore': []
  }
]

error = False
for module in modules:
  found = False
  for line in open(module['path']).readlines():
    # routes registered in `make_routes()`: `r.add_json("/path", ...)` and `j.add("method", ...)`
    match = re.search(r'\b(add_json|j\.add)\("([^"]*)"', line)
    if not match:
      continue
    found = True
    kind, name = match.groups()
    if name in module['ignore'] or name.endswith('.bin'):
      continue
    if kind == 'add_json':
      if not name.startswith('/'):
        print('Error: %s does not start with /' % name)
        error = True
      name = name[1:]
    if not hasattr(module['object'], name):
      print('Error: %s API method %s does not have a matching function' % (module['name'], name))
      error = True
  if not found:
    print('Error: no %s API methods found in %s' % (module['name'], module['path']))
    error = True

sys.exit(1 if error else 0)
