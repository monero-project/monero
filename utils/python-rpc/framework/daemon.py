# Copyright (c) 2018-2022, The Monero Project

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

    def __init__(self, protocol='http', host='127.0.0.1', port=0, idx=0, restricted_rpc = False):
        base = 18480 if restricted_rpc else 18180
        self.host = host
        self.port = port
        self.rpc = JSONRPC('{protocol}://{host}:{port}'.format(protocol=protocol, host=host, port=port if port else base+idx))

    def getblocktemplate(self, address, prev_block = "", client = ""):
        getblocktemplate = {
            'method': 'getblocktemplate',
            'params': {
                'client': client,
                'wallet_address': address,
                'reserve_size' : 1,
                'prev_block' : prev_block,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(getblocktemplate)
    get_block_template = getblocktemplate

    def get_miner_data(self):
        get_miner_data = {
            'method': 'get_miner_data',
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_miner_data)

    def calc_pow(self, major_version, height, block_blob, seed_hash = ''):
        calc_pow = {
            'method': 'calc_pow',
            'params': {
                'major_version': major_version,
                'height': height,
                'block_blob' : block_blob,
                'seed_hash' : seed_hash,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(calc_pow)

    def add_aux_pow(self, blocktemplate_blob, aux_pow, client = ""):
        add_aux_pow = {
            'method': 'add_aux_pow',
            'params': {
                'blocktemplate_blob': blocktemplate_blob,
                'aux_pow' : aux_pow,
                'client' : client,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(add_aux_pow)

    def send_raw_transaction(self, tx_as_hex, do_not_relay = False, do_sanity_checks = True, client = ""):
        send_raw_transaction = {
            'client': client,
            'tx_as_hex': tx_as_hex,
            'do_not_relay': do_not_relay,
            'do_sanity_checks': do_sanity_checks,
        }
        return self.rpc.send_request("/send_raw_transaction", send_raw_transaction)
    sendrawtransaction = send_raw_transaction

    def submitblock(self, block):
        submitblock = {
            'method': 'submitblock',
            'params': [ block ],    
            'jsonrpc': '2.0', 
            'id': '0'
        }    
        return self.rpc.send_json_rpc_request(submitblock)
    submit_block = submitblock

    def getblock(self, hash = '', height = 0, fill_pow_hash = False, client = ""):
        getblock = {
            'method': 'getblock',
            'params': {
                'client': client,
                'hash': hash,
                'height': height,
                'fill_pow_hash': fill_pow_hash,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(getblock)
    get_block = getblock

    def getlastblockheader(self, client = ""):
        getlastblockheader = {
            'method': 'getlastblockheader',
            'params': {
                'client': client,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(getlastblockheader)
    get_last_block_header = getlastblockheader

    def getblockheaderbyhash(self, hash = "", hashes = [], client = ""):
        getblockheaderbyhash = {
            'method': 'getblockheaderbyhash',
            'params': {
                'client': client,
                'hash': hash,
                'hashes': hashes,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(getblockheaderbyhash)
    get_block_header_by_hash = getblockheaderbyhash

    def getblockheaderbyheight(self, height, client = ""):
        getblockheaderbyheight = {
            'method': 'getblockheaderbyheight',
            'params': {
                'client': client,
                'height': height,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(getblockheaderbyheight)
    get_block_header_by_height = getblockheaderbyheight

    def getblockheadersrange(self, start_height, end_height, fill_pow_hash = False, client = ""):
        getblockheadersrange = {
            'method': 'getblockheadersrange',
            'params': {
                'client': client,
                'start_height': start_height,
                'end_height': end_height,
                'fill_pow_hash': fill_pow_hash,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(getblockheadersrange)
    get_block_headers_range = getblockheadersrange

    def get_connections(self, client = ""):
        get_connections = {
            'client': client,
            'method': 'get_connections',
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_connections)

    def get_info(self, client = ""):
        get_info = {
            'method': 'get_info',
            'params': {
                'client': client,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }    
        return self.rpc.send_json_rpc_request(get_info)
    getinfo = get_info

    def hard_fork_info(self, client = ""):
        hard_fork_info = {
            'method': 'hard_fork_info',
            'params': {
                'client': client,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }    
        return self.rpc.send_json_rpc_request(hard_fork_info)

    def generateblocks(self, address, blocks=1, prev_block = "", starting_nonce = 0):
        generateblocks = {
            'method': 'generateblocks',
            'params': {
                'amount_of_blocks' : blocks,
                'reserve_size' : 20,
                'wallet_address': address,
                'prev_block': prev_block,
                'starting_nonce': starting_nonce,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(generateblocks)

    def get_height(self, client = ""):
        get_height = {
                'method': 'get_height',
                'jsonrpc': '2.0',
                'id': '0'
        }
        return self.rpc.send_request("/get_height", get_height)
    getheight = get_height

    def pop_blocks(self, nblocks = 1):
        pop_blocks = {
            'nblocks' : nblocks,
        }
        return self.rpc.send_request("/pop_blocks", pop_blocks)

    def start_mining(self, miner_address, threads_count = 0, do_background_mining = False, ignore_battery = False):
        start_mining = {
            'miner_address' : miner_address,
            'threads_count' : threads_count,
            'do_background_mining' : do_background_mining,
            'ignore_battery' : ignore_battery,
        }
        return self.rpc.send_request('/start_mining', start_mining)

    def stop_mining(self):
        stop_mining = {
        }
        return self.rpc.send_request('/stop_mining', stop_mining)

    def mining_status(self):
        mining_status = {
        }
        return self.rpc.send_request('/mining_status', mining_status)

    def get_transaction_pool(self, client = ""):
        get_transaction_pool = {
            'client': client,
        }
        return self.rpc.send_request('/get_transaction_pool', get_transaction_pool)

    def get_transaction_pool_hashes(self, client = ""):
        get_transaction_pool_hashes = {
            'client': client,
        }
        return self.rpc.send_request('/get_transaction_pool_hashes', get_transaction_pool_hashes)

    def get_transaction_pool_stats(self, client = ""):
        get_transaction_pool_stats = {
            'client': client,
        }
        return self.rpc.send_request('/get_transaction_pool_stats', get_transaction_pool_stats)

    def flush_txpool(self, txids = []):
        flush_txpool = {
            'method': 'flush_txpool',
            'params': {
                'txids': txids
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(flush_txpool)

    def get_version(self):
        get_version = {
            'method': 'get_version',
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_version)

    def get_bans(self):
        get_bans = {
            'method': 'get_bans',
            'params': {
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_bans)

    def set_bans(self, bans = []):
        set_bans = {
            'method': 'set_bans',
            'params': {
                'bans': bans
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(set_bans)

    def banned(self, address = ''):
        banned = {
            'method': 'banned',
            'params': {
                'address': address
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(banned)

    def set_bootstrap_daemon(self, address, username = '', password = ''):
        set_bootstrap_daemon = {
            'address': address,
            'username': username,
            'password': password,
        }
        return self.rpc.send_request('/set_bootstrap_daemon', set_bootstrap_daemon)

    def get_public_nodes(self, gray = False, white = True):
        get_public_nodes = {
            'gray': gray,
            'white': white,
        }
        return self.rpc.send_request('/get_public_nodes', get_public_nodes)

    def get_transactions(self, txs_hashes = [], decode_as_json = False, prune = False, split = False, client = ""):
        get_transactions = {
            'client': client,
            'txs_hashes': txs_hashes,
            'decode_as_json': decode_as_json,
            'prune': prune,
            'split': split,
        }
        return self.rpc.send_request('/get_transactions', get_transactions)
    gettransactions = get_transactions

    def get_outs(self, outputs = [], get_txid = False, client = ""):
        get_outs = {
            'client': client,
            'outputs': outputs,
            'get_txid': get_txid,
        }
        return self.rpc.send_request('/get_outs', get_outs)

    def get_coinbase_tx_sum(self, height, count, client = ""):
        get_coinbase_tx_sum = {
            'method': 'get_coinbase_tx_sum',
            'params': {
                'client': client,
                'height': height,
                'count': count,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_coinbase_tx_sum)

    def get_output_distribution(self, amounts = [], from_height = 0, to_height = 0, cumulative = False, binary = False, compress = False, client = ""):
        get_output_distribution = {
            'method': 'get_output_distribution',
            'params': {
                'client': client,
                'amounts': amounts,
                'from_height': from_height,
                'to_height': to_height,
                'cumulative': cumulative,
                'binary': binary,
                'compress': compress,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_output_distribution)

    def get_output_histogram(self, amounts = [], min_count = 0, max_count = 0, unlocked = False, recent_cutoff = 0, client = ""):
        get_output_histogram = {
            'method': 'get_output_histogram',
            'params': {
                'client': client,
                'amounts': amounts,
                'min_count': min_count,
                'max_count': max_count,
                'unlocked': unlocked,
                'recent_cutoff': recent_cutoff,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_output_histogram)

    def set_log_level(self, level):
        set_log_level = {
            'level': level,
        }
        return self.rpc.send_request('/set_log_level', set_log_level)

    def set_log_categories(self, categories = ''):
        set_log_categories = {
            'categories': categories,
        }
        return self.rpc.send_request('/set_log_categories', set_log_categories)

    def get_alt_blocks_hashes(self, client = ""):
        get_alt_blocks_hashes = {
            'client': client,
        }
        return self.rpc.send_request('/get_alt_blocks_hashes', get_alt_blocks_hashes)

    def get_alternate_chains(self, client = ""):
        get_alternate_chains = {
            'method': 'get_alternate_chains',
            'params': {
                'client': client,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_alternate_chains)

    def get_fee_estimate(self, grace_blocks = 0):
        get_fee_estimate = {
            'method': 'get_fee_estimate',
            'params': {
                'grace_blocks': grace_blocks,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_fee_estimate)

    def is_key_image_spent(self, key_images = [], client = ""):
        is_key_image_spent = {
            'key_images': key_images,
            'client': client,
        }
        return self.rpc.send_request('/is_key_image_spent', is_key_image_spent)

    def save_bc(self):
        save_bc = {
        }
        return self.rpc.send_request('/save_bc', save_bc)

    def get_peer_list(self):
        get_peer_list = {
        }
        return self.rpc.send_request('/get_peer_list', get_peer_list)

    def set_log_hash_rate(self, visible):
        set_log_hash_rate = {
            'visible': visible,
        }
        return self.rpc.send_request('/set_log_hash_rate', set_log_hash_rate)

    def stop_daemon(self):
        stop_daemon = {
        }
        return self.rpc.send_request('/stop_daemon', stop_daemon)

    def get_net_stats(self):
        get_net_stats = {
        }
        return self.rpc.send_request('/get_net_stats', get_net_stats)

    def get_limit(self):
        get_limit = {
        }
        return self.rpc.send_request('/get_limit', get_limit)

    def set_limit(self, limit_down, limit_up):
        set_limit = {
            'limit_down': limit_down,
            'limit_up': limit_up,
        }
        return self.rpc.send_request('/set_limit', set_limit)

    def out_peers(self, out_peers):
        out_peers = {
            'out_peers': out_peers,
        }
        return self.rpc.send_request('/out_peers', out_peers)

    def in_peers(self, in_peers):
        in_peers = {
            'in_peers': in_peers,
        }
        return self.rpc.send_request('/in_peers', in_peers)

    def update(self, command, path = None):
        update = {
            'command': command,
            'path': path,
        }
        return self.rpc.send_request('/update', update)

    def get_block_count(self):
        get_block_count = {
            'method': 'get_block_count',
            'params': {
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_block_count)
    getblockcount = get_block_count

    def get_block_hash(self, height):
        get_block_hash = {
            'method': 'get_block_hash',
            'params': [height],
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_block_hash)
    on_get_block_hash = get_block_hash
    on_getblockhash = get_block_hash

    def relay_tx(self, txids = [], client = ""):
        relay_tx = {
            'method': 'relay_tx',
            'params': {
                'txids': txids,
                'client': client,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(relay_tx)

    def sync_info(self, client = ""):
        sync_info = {
            'method': 'sync_info',
            'params': {
                'client': client,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(sync_info)

    def get_txpool_backlog(self, client = ""):
        get_txpool_backlog = {
            'method': 'get_txpool_backlog',
            'params': {
                'client': client,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_txpool_backlog)

    def prune_blockchain(self, check = False):
        prune_blockchain = {
            'method': 'prune_blockchain',
            'params': {
                'check': check,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(prune_blockchain)

    def flush_cache(self, bad_txs = False):
        flush_cache = {
            'method': 'flush_cache',
            'params': {
                'bad_txs': bad_txs,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(flush_cache)

    def sync_txpool(self):
        sync_txpool = {
            'method': 'sync_txpool',
            'params': {
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(sync_txpool)

    def rpc_access_info(self, client):
        rpc_access_info = {
            'method': 'rpc_access_info',
            'params': {
                'client': client,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(rpc_access_info)

    def rpc_access_submit_nonce(self, client, nonce, cookie):
        rpc_access_submit_nonce = {
            'method': 'rpc_access_submit_nonce',
            'params': {
                'client': client,
                'nonce': nonce,
                'cookie': cookie,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(rpc_access_submit_nonce)

    def rpc_access_pay(self, client, paying_for, payment):
        rpc_access_pay = {
            'method': 'rpc_access_pay',
            'params': {
                'client': client,
                'paying_for': paying_for,
                'payment': payment,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(rpc_access_pay)

    def rpc_access_tracking(self, clear = False):
        rpc_access_tracking = {
            'method': 'rpc_access_tracking',
            'params': {
                'clear': clear,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(rpc_access_tracking)

    def rpc_access_data(self):
        rpc_access_data = {
            'method': 'rpc_access_data',
            'params': {
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(rpc_access_data)

    def rpc_access_account(self, client, delta_balance = 0):
        rpc_access_account = {
            'method': 'rpc_access_account',
            'params': {
                'client': client,
                'delta_balance': delta_balance,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(rpc_access_account)
