# Copyright (c) 2018 The Monero Project
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

"""Daemon class to make rpc calls and store state."""

from .rpc import JSONRPC 

class Daemon(object):

    def __init__(self, protocol='http', host='127.0.0.1', port=18081, path='/json_rpc'):
        self.rpc = JSONRPC('{protocol}://{host}:{port}{path}'.format(protocol=protocol, host=host, port=port, path=path))

    def getblocktemplate(self, address):
        getblocktemplate = {
            'method': 'getblocktemplate',
            'params': {
                'wallet_address': address,
                'reserve_size' : 1
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_request(getblocktemplate)

    def submitblock(self, block):
        submitblock = {
            'method': 'submitblock',
            'params': [ block ],    
            'jsonrpc': '2.0', 
            'id': '0'
        }    
        return self.rpc.send_request(submitblock)

    def getblock(self, height=0):
        getblock = {
            'method': 'getblock',
            'params': {
                'height': height
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_request(getblock)

    def get_connections(self):
        get_connections = {
            'method': 'get_connections',
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_request(get_connections)

    def get_info(self):
        get_info = {
                'method': 'get_info',
                'jsonrpc': '2.0', 
                'id': '0'
        }    
        return self.rpc.send_request(get_info)    

    def hard_fork_info(self):
        hard_fork_info = {
                'method': 'hard_fork_info',
                'jsonrpc': '2.0', 
                'id': '0'
        }    
        return self.rpc.send_request(hard_fork_info)    

    def generateblocks(self, address, blocks=1):
        generateblocks = {
            'method': 'generateblocks',
            'params': {
                'amount_of_blocks' : blocks,
                'reserve_size' : 20,
                'wallet_address': address
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_request(generateblocks)
