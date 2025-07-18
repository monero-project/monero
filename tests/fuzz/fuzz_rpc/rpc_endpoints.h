#pragma once
#include "common/perf_timer.h"
#include "rpc/core_rpc_server.h"
#include "fuzzer/FuzzedDataProvider.h"
#include <functional>
#include <vector>

namespace tools {
  extern __thread std::vector<LoggingPerformanceTimer*> *performance_timers;
}

void disable_bootstrap_daemon(cryptonote::core_rpc_server& rpc);
std::map<int, std::function<void(cryptonote::core_rpc_server& rpc, FuzzedDataProvider&)>> get_fuzz_targets(bool safe);

void fuzz_get_height(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_blocks(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_blocks_by_height(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_hashes(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_indexes(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_outs_bin(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_transactions(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_alt_blocks_hashes(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_is_key_image_spent(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_send_raw_tx(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_start_mining(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_stop_mining(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_mining_status(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_save_bc(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_peer_list(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_public_nodes(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_set_log_hash_rate(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_set_log_level(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_set_log_categories(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_transaction_pool(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_transaction_pool_hashes_bin(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_transaction_pool_hashes(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_transaction_pool_stats(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_set_bootstrap_daemon(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_stop_daemon(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_info(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_net_stats(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_limit(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_set_limit(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_out_peers(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_in_peers(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_outs(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_update(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_output_distribution_bin(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_pop_blocks(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_getblockcount(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_getblockhash(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_getblocktemplate(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_getminerdata(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_calcpow(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_add_aux_pow(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_submitblock(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_generateblocks(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_last_block_header(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_block_header_by_hash(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_block_header_by_height(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_block_headers_range(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_block(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_connections(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_info_json(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_hard_fork_info(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_set_bans(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_bans(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_banned(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_flush_txpool(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_output_histogram(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_version(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_coinbase_tx_sum(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_base_fee_estimate(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_alternate_chains(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_relay_tx(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_sync_info(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_txpool_backlog(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_output_distribution(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_prune_blockchain(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_flush_cache(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_txids_loose(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_rpc_access_info(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_rpc_access_submit_nonce(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_rpc_access_pay(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_rpc_access_tracking(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_rpc_access_data(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_rpc_access_account(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);
void fuzz_get_height(cryptonote::core_rpc_server&, FuzzedDataProvider& provider);

extern std::map<int, std::function<void(cryptonote::core_rpc_server&, FuzzedDataProvider&)>> priority_fuzz_targets;
extern std::map<int, std::function<void(cryptonote::core_rpc_server&, FuzzedDataProvider&)>> safe_fuzz_targets;
extern std::map<int, std::function<void(cryptonote::core_rpc_server&, FuzzedDataProvider&)>> risky_fuzz_targets;
