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

    def __init__(self, protocol='http', host='127.0.0.1', port=18083):
        self.rpc = JSONRPC('{protocol}://{host}:{port}'.format(protocol=protocol, host=host, port=port))

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

    def transfer(self, destinations, account_index = 0, subaddr_indices = [], priority = 0, ring_size = 0, unlock_time = 0, payment_id = '', get_tx_key = True, do_not_relay = False, get_tx_hex = False, get_tx_metadata = False):
        transfer = {
            'method': 'transfer',
            'params': {
                'destinations': destinations,
                'account_index': account_index,
                'subaddr_indices': subaddr_indices,
                'priority': priority,
                'ring_size' : ring_size,
                'unlock_time' : unlock_time,
                'payment_id' : payment_id,
                'get_tx_key' : get_tx_key,
                'do_not_relay' : do_not_relay,
                'get_tx_hex' : get_tx_hex,
                'get_tx_metadata' : get_tx_metadata,
            },
            'jsonrpc': '2.0', 
            'id': '0'    
        }
        return self.rpc.send_json_rpc_request(transfer)   

    def transfer_split(self, destinations, account_index = 0, subaddr_indices = [], priority = 0, ring_size = 0, unlock_time = 0, payment_id = '', get_tx_key = True, do_not_relay = False, get_tx_hex = False, get_tx_metadata = False):
        transfer = {
            "method": "transfer_split",
            "params": {
                'destinations': destinations,
                'account_index': account_index,
                'subaddr_indices': subaddr_indices,
                'priority': priority,
                'ring_size' : ring_size,
                'unlock_time' : unlock_time,
                'payment_id' : payment_id,
                'get_tx_key' : get_tx_key,
                'do_not_relay' : do_not_relay,
                'get_tx_hex' : get_tx_hex,
                'get_tx_metadata' : get_tx_metadata,
            },
            "jsonrpc": "2.0", 
            "id": "0"    
        }
        return self.rpc.send_json_rpc_request(transfer)   

    def describe_transfer(self, unsigned_txset):
        describe_transfer = {
            'method': 'describe_transfer',
            'params': {
                'unsigned_txset': unsigned_txset,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(describe_transfer)

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
        return self.rpc.send_json_rpc_request(create_wallet)

    def get_balance(self):
        get_balance = {
            'method': 'get_balance',
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_balance)

    def sweep_dust(self):
        sweep_dust = {
            'method': 'sweep_dust',
            'jsonrpc': '2.0', 
            'id': '0'   
        }
        return self.rpc.send_json_rpc_request(sweep_dust)

    def sweep_all(self, address):
        sweep_all = {
            'method': 'sweep_all',
            'params' : {
                'address' : ''
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(sweep_all)

    def get_address(self, account_index = 0, subaddresses = []):
        get_address = {
            'method': 'get_address',
            'params' : {
                'account_index' : account_index,
                'address_index': subaddresses
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_address)

    def create_account(self, label = ""):
        create_account = {
            'method': 'create_account',
            'params' : {
                'label': label
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(create_account)

    def create_address(self, account_index = 0, label = ""):
        create_address = {
            'method': 'create_address',
            'params' : {
                'account_index': account_index,
                'label': label
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(create_address)

    def label_address(self, subaddress_index, label):
        label_address = {
            'method': 'label_address',
            'params' : {
                'index': { 'major': subaddress_index[0], 'minor': subaddress_index[1]},
                'label': label
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(label_address)

    def label_account(self, account_index, label):
        label_account = {
            'method': 'label_account',
            'params' : {
                'account_index': account_index,
                'label': label
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(label_account)

    def get_address_index(self, address):
        get_address_index = {
            'method': 'get_address_index',
            'params' : {
                'address': address
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_address_index)

    def query_key(self, key_type):
        query_key = {
            'method': 'query_key',
            'params' : {
                'key_type': key_type
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(query_key)

    def restore_deterministic_wallet(self, seed = '', seed_offset = '', filename = '', restore_height = 0, password = '', language = ''):
        restore_deterministic_wallet = {
            'method': 'restore_deterministic_wallet',
            'params' : {
                'restore_height': restore_height,
                'filename': filename,
                'seed': seed,
                'seed_offset': seed_offset,
                'password': password,
                'language': language
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(restore_deterministic_wallet)

    def close_wallet(self):
        close_wallet = {
            'method': 'close_wallet',
            'params' : {
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(close_wallet)

    def refresh(self):
        refresh = {
            'method': 'refresh',
            'params' : {
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(refresh)

    def incoming_transfers(self, transfer_type='all', account_index = 0, subaddr_indices = []):
        incoming_transfers = {
            'method': 'incoming_transfers',
            'params' : {
                'transfer_type': transfer_type,
                'account_index': account_index,
                'subaddr_indices': subaddr_indices,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(incoming_transfers)

    def get_transfers(self, in_ = True, out = True, pending = True, failed = True, pool = True, min_height = None, max_height = None, account_index = 0, subaddr_indices = [], all_accounts = False):
        get_transfers = {
            'method': 'get_transfers',
            'params' : {
                'in': in_,
                'out': out,
                'pending': pending,
                'failed': failed,
                'pool': pool,
                'min_height': min_height,
                'max_height': max_height,
                'filter_by_height': min_height or max_height,
                'account_index': account_index,
                'subaddr_indices': subaddr_indices,
                'all_accounts': all_accounts,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_transfers)
