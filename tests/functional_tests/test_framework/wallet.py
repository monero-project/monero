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

class Wallet(object):

    def __init__(self, protocol='http', host='127.0.0.1', port=18083, path='/json_rpc'):
        self.rpc = JSONRPC('{protocol}://{host}:{port}{path}'.format(protocol=protocol, host=host, port=port, path=path))

    def make_uniform_destinations(self, address, transfer_amount, transfer_number_of_destinations=1):
        destinations = []
        for i in range(transfer_number_of_destinations):
            destinations.append({"amount":transfer_amount,"address":address})
        return destinations

    def make_destinations(self, addresses, transfer_amounts):
        destinations = []
        for i in range(len(addresses)):
            destinations.append({'amount':transfer_amounts[i],'address':addresses[i]})
        return destinations

    def transfer(self, destinations, ringsize=7, payment_id=''):
        transfer = {
            'method': 'transfer',
            'params': {
                'destinations': destinations,
                'mixin' : ringsize - 1,
                'get_tx_key' : True
            },
            'jsonrpc': '2.0', 
            'id': '0'    
        }
        if(len(payment_id) > 0):
            transfer['params'].update({'payment_id' : payment_id})
        return self.rpc.send_request(transfer)   

    def transfer_split(self, destinations, ringsize=7, payment_id=''):
        print(destinations)
        transfer = {
            "method": "transfer_split",
            "params": {
                "destinations": destinations,
                "mixin" : ringsize - 1,
                "get_tx_key" : True,
                "new_algorithm" : True
            },
            "jsonrpc": "2.0", 
            "id": "0"    
        }
        if(len(payment_id) > 0):
            transfer['params'].update({'payment_id' : payment_id})
        return self.rpc.send_request(transfer)   

    def create_wallet(self, index=''):
        create_wallet = {
            'method': 'create_wallet',
            'params': {
                'filename': 'testWallet' + index,
                'password' : '',
                'language' : 'English'
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_request(create_wallet)

    def get_balance(self):
        get_balance = {
            'method': 'get_balance',
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_request(get_balance)

    def sweep_dust(self):
        sweep_dust = {
            'method': 'sweep_dust',
            'jsonrpc': '2.0', 
            'id': '0'   
        }
        return self.rpc.send_request(sweep_dust)

    def sweep_all(self, address):
        sweep_all = {
            'method': 'sweep_all',
            'params' : {
                'address' : ''
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_request(sweep_all)
