# Copyright (c) 2018-2024, The Monero Project

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

    def __init__(self, protocol='http', host='127.0.0.1', port=0, idx=0):
        self.host = host
        self.port = port
        self.rpc = JSONRPC('{protocol}://{host}:{port}'.format(protocol=protocol, host=host, port=port if port else 18090+idx))

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

    def get_transfer_by_txid(self, txid, account_index = 0):
        get_transfer_by_txid = {
            'method': 'get_transfer_by_txid',
            'params': {
                'txid': txid,
                'account_index': account_index,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_transfer_by_txid)

    def get_payments(self, payment_id):
        get_payments = {
            'method': 'get_payments',
            'params': {
                'payment_id': payment_id,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_payments)

    def get_bulk_payments(self, payment_ids = [], min_block_height = 0):
        get_bulk_payments = {
            'method': 'get_bulk_payments',
            'params': {
                'payment_ids': payment_ids,
                'min_block_height': min_block_height,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_bulk_payments)

    def describe_transfer(self, unsigned_txset = '', multisig_txset = ''):
        describe_transfer = {
            'method': 'describe_transfer',
            'params': {
                'unsigned_txset': unsigned_txset,
                'multisig_txset': multisig_txset,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(describe_transfer)

    def create_wallet(self, filename='', password = '', language = 'English'):
        create_wallet = {
            'method': 'create_wallet',
            'params': {
                'filename': filename,
                'password': password,
                'language': language
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(create_wallet)

    def get_balance(self, account_index = 0, address_indices = [], all_accounts = False, strict = False):
        get_balance = {
            'method': 'get_balance',
            'params': {
                'account_index': account_index,
                'address_indices': address_indices,
                'all_accounts': all_accounts,
                'strict': strict,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_balance)
    getbalance = get_balance

    def sweep_dust(self, get_tx_keys = True, do_not_relay = False, get_tx_hex = False, get_tx_metadata = False):
        sweep_dust = {
            'method': 'sweep_dust',
            'params': {
                'get_tx_keys': get_tx_keys,
                'do_not_relay': do_not_relay,
                'get_tx_hex': get_tx_hex,
                'get_tx_metadata': get_tx_metadata,
            },
            'jsonrpc': '2.0', 
            'id': '0'   
        }
        return self.rpc.send_json_rpc_request(sweep_dust)
    sweep_unmixable = sweep_dust

    def sweep_all(self, address = '', account_index = 0, subaddr_indices = [], priority = 0, ring_size = 0, outputs = 1, unlock_time = 0, payment_id = '', get_tx_keys = False, below_amount = 0, do_not_relay = False, get_tx_hex = False, get_tx_metadata = False):
        sweep_all = {
            'method': 'sweep_all',
            'params' : {
                'address' : address,
                'account_index' : account_index,
                'subaddr_indices' : subaddr_indices,
                'priority' : priority,
                'ring_size' : ring_size,
                'outputs' : outputs,
                'unlock_time' : unlock_time,
                'payment_id' : payment_id,
                'get_tx_keys' : get_tx_keys,
                'below_amount' : below_amount,
                'do_not_relay' : do_not_relay,
                'get_tx_hex' : get_tx_hex,
                'get_tx_metadata' : get_tx_metadata,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(sweep_all)

    def sweep_single(self, address = '', priority = 0, ring_size = 0, outputs = 1, unlock_time = 0, payment_id = '', get_tx_keys = False, key_image = "", do_not_relay = False, get_tx_hex = False, get_tx_metadata = False):
        sweep_single = {
            'method': 'sweep_single',
            'params' : {
                'address' : address,
                'priority' : priority,
                'ring_size' : ring_size,
                'outputs' : outputs,
                'unlock_time' : unlock_time,
                'payment_id' : payment_id,
                'get_tx_keys' : get_tx_keys,
                'key_image' : key_image,
                'do_not_relay' : do_not_relay,
                'get_tx_hex' : get_tx_hex,
                'get_tx_metadata' : get_tx_metadata,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(sweep_single)

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
    getaddress = get_address

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

    def create_address(self, account_index = 0, label = "", count = 1):
        create_address = {
            'method': 'create_address',
            'params' : {
                'account_index': account_index,
                'label': label,
                'count': count
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

    def restore_deterministic_wallet(self, seed = '', seed_offset = '', filename = '', restore_height = 0, password = '', language = '', autosave_current = True, enable_multisig_experimental = False):
        restore_deterministic_wallet = {
            'method': 'restore_deterministic_wallet',
            'params' : {
                'restore_height': restore_height,
                'filename': filename,
                'seed': seed,
                'seed_offset': seed_offset,
                'password': password,
                'language': language,
                'autosave_current': autosave_current,
                'enable_multisig_experimental': enable_multisig_experimental
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(restore_deterministic_wallet)

    def generate_from_keys(self, restore_height = 0, filename = "", password = "", address = "", spendkey = "", viewkey = "", autosave_current = True):
        generate_from_keys = {
            'method': 'generate_from_keys',
            'params' : {
                'restore_height': restore_height,
                'filename': filename,
                'address': address,
                'spendkey': spendkey,
                'viewkey': viewkey,
                'password': password,
                'autosave_current': autosave_current,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(generate_from_keys)

    def open_wallet(self, filename, password='', autosave_current = True):
        open_wallet = {
            'method': 'open_wallet',
            'params' : {
                'filename': filename,
                'password': password,
                'autosave_current': autosave_current,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(open_wallet)

    def close_wallet(self, autosave_current = True):
        close_wallet = {
            'method': 'close_wallet',
            'params' : {
                'autosave_current': autosave_current
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(close_wallet)

    def change_wallet_password(self, old_password, new_password):
        change_wallet_password = {
            'method': 'change_wallet_password',
            'params' : {
                'old_password': old_password,
                'new_password': new_password,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(change_wallet_password)

    def store(self):
        store = {
            'method': 'store',
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(store)

    def stop_wallet(self):
        stop_wallet = {
            'method': 'stop_wallet',
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(stop_wallet)

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

    def make_integrated_address(self, standard_address = '', payment_id = ''):
        make_integrated_address = {
            'method': 'make_integrated_address',
            'params' : {
                'standard_address': standard_address,
                'payment_id': payment_id,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(make_integrated_address)

    def split_integrated_address(self, integrated_address):
        split_integrated_address = {
            'method': 'split_integrated_address',
            'params' : {
                'integrated_address': integrated_address,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(split_integrated_address)

    def auto_refresh(self, enable, period = 0):
        auto_refresh = {
            'method': 'auto_refresh',
            'params' : {
                'enable': enable,
                'period': period
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(auto_refresh)

    def set_daemon(self, address, trusted = False, ssl_support = "autodetect", ssl_private_key_path = "", ssl_certificate_path = "", ssl_allowed_certificates = [], ssl_allowed_fingerprints = [], ssl_allow_any_cert = False):
        set_daemon = {
            'method': 'set_daemon',
            'params' : {
                'address': address,
                'trusted': trusted,
                'ssl_support': ssl_support,
                'ssl_private_key_path': ssl_private_key_path,
                'ssl_certificate_path': ssl_certificate_path,
                'ssl_allowed_certificates': ssl_allowed_certificates,
                'ssl_allowed_fingerprints': ssl_allowed_fingerprints,
                'ssl_allow_any_cert': ssl_allow_any_cert,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(set_daemon)

    def is_multisig(self):
        is_multisig = {
            'method': 'is_multisig',
            'params' : {
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(is_multisig)

    def prepare_multisig(self, enable_multisig_experimental = False):
        prepare_multisig = {
            'method': 'prepare_multisig',
            'params' : {
                'enable_multisig_experimental': enable_multisig_experimental,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(prepare_multisig)

    def make_multisig(self, multisig_info, threshold, password = ''):
        make_multisig = {
            'method': 'make_multisig',
            'params' : {
                'multisig_info': multisig_info,
                'threshold': threshold,
                'password': password,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(make_multisig)

    def finalize_multisig(self):
        finalize_multisig = {
            'method': 'finalize_multisig',
            'params' : {
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(finalize_multisig)

    def exchange_multisig_keys(self, multisig_info, password = '', force_update_use_with_caution = False):
        exchange_multisig_keys = {
            'method': 'exchange_multisig_keys',
            'params' : {
                'multisig_info': multisig_info,
                'password': password,
                'force_update_use_with_caution': force_update_use_with_caution,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(exchange_multisig_keys)

    def export_multisig_info(self):
        export_multisig_info = {
            'method': 'export_multisig_info',
            'params' : {
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(export_multisig_info)

    def import_multisig_info(self, info = []):
        import_multisig_info = {
            'method': 'import_multisig_info',
            'params' : {
                'info': info
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(import_multisig_info)

    def sign_multisig(self, tx_data_hex):
        sign_multisig = {
            'method': 'sign_multisig',
            'params' : {
                'tx_data_hex': tx_data_hex
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(sign_multisig)

    def submit_multisig(self, tx_data_hex):
        submit_multisig = {
            'method': 'submit_multisig',
            'params' : {
                'tx_data_hex': tx_data_hex
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(submit_multisig)

    def sign_transfer(self, unsigned_txset, export_raw = False, get_tx_keys = False):
        sign_transfer = {
            'method': 'sign_transfer',
            'params' : {
                'unsigned_txset': unsigned_txset,
                'export_raw': export_raw,
                'get_tx_keys': get_tx_keys,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(sign_transfer)

    def submit_transfer(self, tx_data_hex):
        submit_transfer = {
            'method': 'submit_transfer',
            'params' : {
                'tx_data_hex': tx_data_hex,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(submit_transfer)

    def get_tx_key(self, txid):
        get_tx_key = {
            'method': 'get_tx_key',
            'params' : {
                'txid': txid,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_tx_key)

    def check_tx_key(self, txid = '', tx_key = '', address = ''):
        check_tx_key = {
            'method': 'check_tx_key',
            'params' : {
                'txid': txid,
                'tx_key': tx_key,
                'address': address,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(check_tx_key)

    def get_tx_proof(self, txid = '', address = '', message = ''):
        get_tx_proof = {
            'method': 'get_tx_proof',
            'params' : {
                'txid': txid,
                'address': address,
                'message': message,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_tx_proof)

    def check_tx_proof(self, txid = '', address = '', message = '', signature = ''):
        check_tx_proof = {
            'method': 'check_tx_proof',
            'params' : {
                'txid': txid,
                'address': address,
                'message': message,
                'signature': signature,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(check_tx_proof)

    def get_spend_proof(self, txid = '', message = ''):
        get_spend_proof = {
            'method': 'get_spend_proof',
            'params' : {
                'txid': txid,
                'message': message,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_spend_proof)

    def check_spend_proof(self, txid = '', message = '', signature = ''):
        check_spend_proof = {
            'method': 'check_spend_proof',
            'params' : {
                'txid': txid,
                'message': message,
                'signature': signature,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(check_spend_proof)

    def get_reserve_proof(self, all_ = True, account_index = 0, amount = 0, message = ''):
        get_reserve_proof = {
            'method': 'get_reserve_proof',
            'params' : {
                'all': all_,
                'account_index': account_index,
                'amount': amount,
                'message': message,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_reserve_proof)

    def check_reserve_proof(self, address = '', message = '', signature = ''):
        check_reserve_proof = {
            'method': 'check_reserve_proof',
            'params' : {
                'address': address,
                'message': message,
                'signature': signature,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(check_reserve_proof)

    def sign(self, data, account_index = 0, address_index = 0, signature_type = ""):
        sign = {
            'method': 'sign',
            'params' : {
                'data': data,
                'account_index': account_index,
                'address_index': address_index,
                'signature_type': signature_type,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(sign)

    def verify(self, data, address, signature):
        verify = {
            'method': 'verify',
            'params' : {
                'data': data,
                'address': address,
                'signature': signature,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(verify)

    def get_height(self):
        get_height = {
            'method': 'get_height',
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_height)
    getheight = get_height

    def relay_tx(self, hex_):
        relay_tx = {
            'method': 'relay_tx',
            'params': {
                'hex': hex_,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(relay_tx)

    def get_languages(self):
        get_languages = {
            'method': 'get_languages',
            'params': {
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_languages)

    def export_outputs(self, all = False, start = 0, count = 0xffffffff):
        export_outputs = {
            'method': 'export_outputs',
            'params': {
                'all': all,
                'start': start,
                'count': count,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(export_outputs)

    def import_outputs(self, outputs_data_hex):
        import_outputs = {
            'method': 'import_outputs',
            'params': {
                'outputs_data_hex': outputs_data_hex
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(import_outputs)

    def export_key_images(self, all_ = False):
        export_key_images = {
            'method': 'export_key_images',
            'params': {
                'all': all_
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(export_key_images)

    def import_key_images(self, signed_key_images, offset = 0):
        import_key_images = {
            'method': 'import_key_images',
            'params': {
                'offset': offset,
                'signed_key_images': signed_key_images,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(import_key_images)

    def set_log_level(self, level):
        set_log_level = {
            'method': 'set_log_level',
            'params': {
                'level': level,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(set_log_level)

    def set_log_categories(self, categories):
        set_log_categories = {
            'method': 'set_log_categories',
            'params': {
                'categories': categories,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(set_log_categories)

    def validate_address(self, address, any_net_type = False, allow_openalias = False):
        validate_address = {
            'method': 'validate_address',
            'params': {
                'address': address,
                'any_net_type': any_net_type,
                'allow_openalias': allow_openalias,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(validate_address)

    def get_accounts(self, tag, strict_balances = False):
        get_accounts = {
            'method': 'get_accounts',
            'params': {
                'tag': tag,
                'strict_balances': strict_balances,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_accounts)

    def get_account_tags(self):
        get_account_tags = {
            'method': 'get_account_tags',
            'params': {
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_account_tags)

    def tag_accounts(self, tag, accounts = []):
        tag_accounts = {
            'method': 'tag_accounts',
            'params': {
                'tag': tag,
                'accounts': accounts,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(tag_accounts)

    def untag_accounts(self, accounts = []):
        untag_accounts = {
            'method': 'untag_accounts',
            'params': {
                'accounts': accounts,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(untag_accounts)

    def set_account_tag_description(self, tag, description):
        set_account_tag_description = {
            'method': 'set_account_tag_description',
            'params': {
                'tag': tag,
                'description': description,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(set_account_tag_description)

    def rescan_blockchain(self, hard = False):
        rescan_blockchain = {
            'method': 'rescan_blockchain',
            'jsonrpc': '2.0',
            'params': {
                'hard': hard,
            },
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(rescan_blockchain)

    def rescan_spent(self):
        rescan_spent = {
            'method': 'rescan_spent',
            'jsonrpc': '2.0',
            'params': {
            },
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(rescan_spent)

    def set_tx_notes(self, txids = [], notes = []):
        set_tx_notes = {
            'method': 'set_tx_notes',
            'jsonrpc': '2.0',
            'params': {
                'txids': txids,
                'notes': notes,
            },
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(set_tx_notes)

    def get_tx_notes(self, txids = []):
        get_tx_notes = {
            'method': 'get_tx_notes',
            'jsonrpc': '2.0',
            'params': {
                'txids': txids,
            },
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_tx_notes)

    def set_attribute(self, key, value):
        set_attribute = {
            'method': 'set_attribute',
            'jsonrpc': '2.0',
            'params': {
                'key': key,
                'value': value,
            },
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(set_attribute)

    def get_attribute(self, key):
        get_attribute = {
            'method': 'get_attribute',
            'jsonrpc': '2.0',
            'params': {
                'key': key,
            },
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_attribute)

    def make_uri(self, address = '', payment_id = '', amount = 0, tx_description = '', recipient_name = ''):
        make_uri = {
            'method': 'make_uri',
            'jsonrpc': '2.0',
            'params': {
                'address': address,
                'payment_id': payment_id,
                'amount': amount,
                'tx_description': tx_description,
                'recipient_name': recipient_name,
            },
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(make_uri)

    def parse_uri(self, uri):
        parse_uri = {
            'method': 'parse_uri',
            'jsonrpc': '2.0',
            'params': {
                'uri': uri,
            },
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(parse_uri)

    def add_address_book(self, address = '', payment_id = '', description = ''):
        add_address_book = {
            'method': 'add_address_book',
            'jsonrpc': '2.0',
            'params': {
                'address': address,
                'payment_id': payment_id,
                'description': description,
            },
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(add_address_book)

    def edit_address_book(self, index, address = None, payment_id = None, description = None):
        edit_address_book = {
            'method': 'edit_address_book',
            'jsonrpc': '2.0',
            'params': {
                'index': index,
                'set_address': address != None,
                'address': address or '',
                'set_payment_id': payment_id != None,
                'payment_id': payment_id or '',
                'set_description': description != None,
                'description': description or '',
            },
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(edit_address_book)

    def get_address_book(self, entries = []):
        get_address_book = {
            'method': 'get_address_book',
            'jsonrpc': '2.0',
            'params': {
                'entries': entries,
            },
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_address_book)

    def delete_address_book(self, index):
        delete_address_book = {
            'method': 'delete_address_book',
            'jsonrpc': '2.0',
            'params': {
                'index': index,
            },
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(delete_address_book)

    def start_mining(self, threads_count, do_background_mining = False, ignore_battery = False):
        start_mining = {
            'method': 'start_mining',
            'jsonrpc': '2.0',
            'params': {
                'threads_count': threads_count,
                'do_background_mining': do_background_mining,
                'ignore_battery': ignore_battery,
            },
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(start_mining)

    def stop_mining(self):
        stop_mining = {
            'method': 'stop_mining',
            'jsonrpc': '2.0',
            'params': {
            },
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(stop_mining)

    def estimate_tx_size_and_weight(self, n_inputs, n_outputs, ring_size = 0, rct = True):
        estimate_tx_size_and_weight = {
            'method': 'estimate_tx_size_and_weight',
            'jsonrpc': '2.0',
            'params': {
                'n_inputs': n_inputs,
                'n_outputs': n_outputs,
                'ring_size': ring_size,
                'rct': rct,
            },
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(estimate_tx_size_and_weight)

    def get_version(self):
        get_version = {
            'method': 'get_version',
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_version)

    def scan_tx(self, txids):
        scan_tx = {
            'method': 'scan_tx',
            'jsonrpc': '2.0',
            'params' : {
                'txids': txids,
            },
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(scan_tx)

    def freeze(self, key_image):
        freeze = {
            'method': 'freeze',
            'jsonrpc': '2.0',
            'params' : {
                'key_image': key_image,
            },
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(freeze)

    def thaw(self, key_image):
        thaw = {
            'method': 'thaw',
            'jsonrpc': '2.0',
            'params' : {
                'key_image': key_image,
            },
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(thaw)

    def frozen(self, key_image):
        frozen = {
            'method': 'frozen',
            'jsonrpc': '2.0',
            'params' : {
                'key_image': key_image,
            },
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(frozen)
