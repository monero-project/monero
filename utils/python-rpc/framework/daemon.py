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

    def __init__(self, protocol='http', host='127.0.0.1', port=0, idx=0):
        self.host = host
        self.port = port
        self.rpc = JSONRPC('{protocol}://{host}:{port}'.format(protocol=protocol, host=host, port=port if port else 18180+idx))

    def getblocktemplate(self, address, prev_block = ""):
        getblocktemplate = {
            'method': 'getblocktemplate',
            'params': {
                'wallet_address': address,
                'reserve_size' : 1,
                'prev_block' : prev_block,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(getblocktemplate)

    def send_raw_transaction(self, tx_as_hex, do_not_relay = False, do_sanity_checks = True):
        send_raw_transaction = {
            'tx_as_hex': tx_as_hex,
            'do_not_relay': do_not_relay,
            'do_sanity_checks': do_sanity_checks,
        }
        return self.rpc.send_request("/send_raw_transaction", send_raw_transaction)

    def submitblock(self, block):
        submitblock = {
            'method': 'submitblock',
            'params': [ block ],    
            'jsonrpc': '2.0', 
            'id': '0'
        }    
        return self.rpc.send_json_rpc_request(submitblock)

    def getblock(self, height=0):
        getblock = {
            'method': 'getblock',
            'params': {
                'height': height
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(getblock)

    def getlastblockheader(self):
        getlastblockheader = {
            'method': 'getlastblockheader',
            'params': {
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(getlastblockheader)

    def getblockheaderbyhash(self, hash):
        getblockheaderbyhash = {
            'method': 'getblockheaderbyhash',
            'params': {
                'hash': hash,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(getblockheaderbyhash)

    def getblockheaderbyheight(self, height):
        getblockheaderbyheight = {
            'method': 'getblockheaderbyheight',
            'params': {
                'height': height,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(getblockheaderbyheight)

    def getblockheadersrange(self, start_height, end_height, fill_pow_hash = False):
        getblockheadersrange = {
            'method': 'getblockheadersrange',
            'params': {
                'start_height': start_height,
                'end_height': end_height,
                'fill_pow_hash': fill_pow_hash,
            },
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(getblockheadersrange)

    def get_connections(self):
        get_connections = {
            'method': 'get_connections',
            'jsonrpc': '2.0', 
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_connections)

    def get_info(self):
        get_info = {
                'method': 'get_info',
                'jsonrpc': '2.0', 
                'id': '0'
        }    
        return self.rpc.send_json_rpc_request(get_info)

    def hard_fork_info(self):
        hard_fork_info = {
                'method': 'hard_fork_info',
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

    def get_height(self):
        get_height = {
                'method': 'get_height',
                'jsonrpc': '2.0',
                'id': '0'
        }
        return self.rpc.send_request("/get_height", get_height)

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

    def get_transaction_pool(self):
        get_transaction_pool = {
        }
        return self.rpc.send_request('/get_transaction_pool', get_transaction_pool)

    def get_transaction_pool_hashes(self):
        get_transaction_pool_hashes = {
        }
        return self.rpc.send_request('/get_transaction_pool_hashes', get_transaction_pool_hashes)

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

    def get_transactions(self, txs_hashes = [], decode_as_json = False, prune = False, split = False):
        get_transactions = {
            'txs_hashes': txs_hashes,
            'decode_as_json': decode_as_json,
            'prune': prune,
            'split': split,
        }
        return self.rpc.send_request('/get_transactions', get_transactions)

    def get_outs(self, outputs = [], get_txid = False):
        get_outs = {
            'outputs': outputs,
            'get_txid': get_txid,
        }
        return self.rpc.send_request('/get_outs', get_outs)

    def get_coinbase_tx_sum(self, height, count):
        get_coinbase_tx_sum = {
            'method': 'get_coinbase_tx_sum',
            'params': {
                'height': height,
                'count': count,
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_coinbase_tx_sum)

    def get_output_distribution(self, amounts = [], from_height = 0, to_height = 0, cumulative = False, binary = False, compress = False):
        get_output_distribution = {
            'method': 'get_output_distribution',
            'params': {
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

    def get_output_histogram(self, amounts = [], min_count = 0, max_count = 0, unlocked = False, recent_cutoff = 0):
        get_output_histogram = {
            'method': 'get_output_histogram',
            'params': {
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

    def get_alt_blocks_hashes(self):
        get_alt_blocks_hashes = {
        }
        return self.rpc.send_request('/get_alt_blocks_hashes', get_alt_blocks_hashes)

    def get_alternate_chains(self):
        get_alternate_chains = {
            'method': 'get_alternate_chains',
            'params': {
            },
            'jsonrpc': '2.0',
            'id': '0'
        }
        return self.rpc.send_json_rpc_request(get_alternate_chains)
