#!/usr/bin/env python

from __future__ import print_function
import sys
import subprocess
import socket
from framework import rpc
from framework import wallet
from framework import daemon

USAGE = 'usage: python -i console.py <port>'
try:
  port = int(sys.argv[1])
except:
  print(USAGE)
  sys.exit(1)

# check for open port
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.settimeout(1)
if s.connect_ex(('127.0.0.1', port)) != 0:
  print('No wallet or daemon RPC on port ' + str(port))
  sys.exit(1)
s.close()

# both wallet and daemon have a get_version JSON RPC
rpc = rpc.JSONRPC('{protocol}://{host}:{port}'.format(protocol='http', host='127.0.0.1', port=port))
get_version = {
    'method': 'get_version',
    'jsonrpc': '2.0', 
    'id': '0'
}
try:
  res = rpc.send_json_rpc_request(get_version)
except Exception, e:
  print('Failed to call version RPC: ' + str(e))
  sys.exit(1)

if 'version' not in res:
  print('Server is not a monero process')
  sys.exit(1)

if 'status' in res:
  rpc = daemon.Daemon(port=port)
else:
  rpc = wallet.Wallet(port=port)

print('Connected to %s RPC on port %u' % ('daemon' if 'status' in res else 'wallet', port))
print('The \'rpc\' object may now be used to use the API')
