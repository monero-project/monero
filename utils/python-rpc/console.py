#!/usr/bin/env python

from __future__ import print_function
import sys
import subprocess
import socket
try:
  import urllib.parse
  url_parser = urllib.parse.urlparse
except:
  try:
    import urlparse
    url_parser = urlparse.urlparse
  except:
    print('urllib or urlparse is needed')
    sys.exit(1)
import framework.rpc
import framework.daemon
import framework.wallet

USAGE = 'usage: python -i console.py [[[scheme]<host>:]<port> [[[scheme]<host>:]<port>...]]'
daemons = []
wallets = []
rpcs = []
for n in range(1, len(sys.argv)):
  scheme='http'
  host='127.0.0.1'
  port=None
  try:
    try:
      port = int(sys.argv[n])
    except:
      t = url_parser(sys.argv[n], allow_fragments = False)
      scheme = t.scheme or scheme
      host = t.hostname or host
      port = t.port or port
      if scheme != 'http' and scheme != 'https':
        raise Exception(USAGE)
      if port <= 0 or port > 65535:
        raise Exception(USAGE)
  except Exception as e:
    print('Error: ' + str(e))
    raise Exception(USAGE)

  # check for open port
  s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  s.settimeout(1)
  if s.connect_ex((host, port)) != 0:
    raise Exception('No wallet or daemon RPC on port ' + str(port))
  s.close()

  # both wallet and daemon have a get_version JSON RPC
  rpc = framework.rpc.JSONRPC('{protocol}://{host}:{port}'.format(protocol=scheme, host=host, port=port))
  get_version = {
      'method': 'get_version',
      'jsonrpc': '2.0', 
      'id': '0'
  }
  try:
    res = rpc.send_json_rpc_request(get_version)
  except Exception as e:
    raise Exception('Failed to call version RPC: ' + str(e))

  if 'version' not in res:
    raise Exception('Server is not a Monero process')

  if 'status' in res:
    daemons.append(framework.daemon.Daemon(port=port))
    rpcs.append(daemons[-1])
  else:
    wallets.append(framework.wallet.Wallet(port=port))
    rpcs.append(wallets[-1])

# add tab completion if we can: https://stackoverflow.com/questions/246725
try:
  import readline
except:
  pass
else:
  import rlcompleter
  readline.parse_and_bind('tab: complete')

if len(daemons) == 1:
  daemon = daemons[0]
if len(wallets) == 1:
  wallet = wallets[0]

didx = 0
widx = 0
for rpc in rpcs:
  if type(rpc) == framework.daemon.Daemon:
    var = "daemon" if len(daemons) == 1 else "daemons[" + str(didx) + "]"
    didx += 1
  else:
    var = "wallet" if len(wallets) == 1 else "wallets[" + str(widx) + "]"
    widx += 1
  print('Variable \'%s\' connected to %s RPC on %s:%u' % (var, 'daemon' if type(rpc) == framework.daemon.Daemon else 'wallet', rpc.host ,rpc.port))
