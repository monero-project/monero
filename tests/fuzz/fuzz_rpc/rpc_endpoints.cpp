#include "rpc_endpoints.h"
#include "initialisation.h"
#include "rpc/rpc_payment_signature.h"
#include <cstring>
#include <map>

// Initialising common objects for calling RPC endpoint functions
cryptonote::core_rpc_server::connection_context ctx;
epee::json_rpc::error error_resp;

// Helper function to disable bootstrap daemon
void disable_bootstrap_daemon(cryptonote::core_rpc_server& rpc) {
  cryptonote::COMMAND_RPC_SET_BOOTSTRAP_DAEMON::request req;
  cryptonote::COMMAND_RPC_SET_BOOTSTRAP_DAEMON::response res;

  req.address = "";
  req.username = "";
  req.password = "";
  req.proxy = "";

  rpc.on_set_bootstrap_daemon(req, res, &ctx);
}

// Retrieve fuzz targets base on SAFE settings
std::map<int, std::function<void(cryptonote::core_rpc_server& rpc, FuzzedDataProvider&)>> get_fuzz_targets(bool safe) {
    std::map<int, std::function<void(cryptonote::core_rpc_server& rpc, FuzzedDataProvider&)>> results;

    if (safe) {
        // Only return safe and stable fuzz targets after re-indexing
        for (const auto& kv : safe_fuzz_targets) {
            results[kv.first - 14] = kv.second;
        }
    } else {
        // Return the full list of fuzz targets
        results.insert(priority_fuzz_targets.begin(), priority_fuzz_targets.end());
        results.insert(safe_fuzz_targets.begin(), safe_fuzz_targets.end());
        results.insert(risky_fuzz_targets.begin(), risky_fuzz_targets.end());
    }

    return results;
}

// Fuzzing functions for different RPC endpoint functions
void fuzz_get_height(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_HEIGHT::request req;
  cryptonote::COMMAND_RPC_GET_HEIGHT::response res;

  rpc.on_get_height(req, res, &ctx);
}

void fuzz_get_blocks(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request req;
  cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response res;
  req.client = "fuzz";

  req.requested_info = provider.ConsumeIntegralInRange<uint8_t>(0, 2);

  size_t count = provider.ConsumeIntegralInRange<size_t>(0, 16);
  for (size_t i = 0; i < count; ++i) {
    std::string hash_data = provider.ConsumeBytesAsString(sizeof(crypto::hash));
    if (hash_data.size() != sizeof(crypto::hash)) break;
    crypto::hash h;
    std::memcpy(&h, hash_data.data(), sizeof(h));
    req.block_ids.push_back(h);
  }

  req.start_height = provider.ConsumeIntegral<uint64_t>();
  req.pool_info_since = provider.ConsumeIntegral<uint64_t>();
  req.max_block_count = provider.ConsumeIntegral<uint64_t>();

  req.prune = provider.ConsumeBool();
  req.no_miner_tx = provider.ConsumeBool();

  rpc.on_get_blocks(req, res, &ctx);
}

void fuzz_get_blocks_by_height(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_BLOCKS_BY_HEIGHT::request req;
  cryptonote::COMMAND_RPC_GET_BLOCKS_BY_HEIGHT::response res;
  req.client = "fuzz";

  size_t count = provider.ConsumeIntegralInRange<size_t>(0, 16);
  for (size_t i = 0; i < count; ++i) {
    uint64_t height = provider.ConsumeIntegral<uint64_t>();
    req.heights.push_back(height);
  }

  rpc.on_get_blocks_by_height(req, res, &ctx);
}

void fuzz_get_hashes(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_HASHES_FAST::request req;
  cryptonote::COMMAND_RPC_GET_HASHES_FAST::response res;
  req.client = "fuzz";

  size_t count = provider.ConsumeIntegralInRange<size_t>(0, 16);
  for (size_t i = 0; i < count; ++i) {
    std::string hash_data = provider.ConsumeBytesAsString(sizeof(crypto::hash));
    if (hash_data.size() != sizeof(crypto::hash)) break;
    crypto::hash h;
    std::memcpy(&h, hash_data.data(), sizeof(h));
    req.block_ids.push_back(h);
  }

  req.start_height = provider.ConsumeIntegral<uint64_t>();

  rpc.on_get_hashes(req, res, &ctx);
}

void fuzz_get_indexes(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request req;
  cryptonote::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response res;
  req.client = "fuzz";

  std::string txid_data = provider.ConsumeBytesAsString(sizeof(crypto::hash));
  if (txid_data.size() == sizeof(crypto::hash)) {
    std::memcpy(&req.txid, txid_data.data(), sizeof(req.txid));
  }

  rpc.on_get_indexes(req, res, &ctx);
}

void fuzz_get_outs_bin(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_OUTPUTS_BIN::request req;
  cryptonote::COMMAND_RPC_GET_OUTPUTS_BIN::response res;
  req.client = "fuzz";

  size_t count = provider.ConsumeIntegralInRange<size_t>(0, 16);
  for (size_t i = 0; i < count; ++i) {
    cryptonote::get_outputs_out out;
    out.amount = provider.ConsumeIntegral<uint64_t>();
    out.index = provider.ConsumeIntegral<uint64_t>();
    req.outputs.push_back(out);
  }

  req.get_txid = provider.ConsumeBool();

  rpc.on_get_outs_bin(req, res, &ctx);
}

void fuzz_get_transactions(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_TRANSACTIONS::request req;
  cryptonote::COMMAND_RPC_GET_TRANSACTIONS::response res;
  req.client = "fuzz";

  // Cached valid transaction hashes
  size_t real_count = provider.ConsumeIntegralInRange<size_t>(0, cached_tx_hashes.size());
  for (size_t i = 0; i < real_count; ++i) {
    const crypto::hash& h = cached_tx_hashes[i];
    req.txs_hashes.emplace_back(epee::string_tools::pod_to_hex(h));
  }

  // Random transaction hashes (that still need 64 bytes hex
  size_t junk_count = provider.ConsumeIntegralInRange<size_t>(0, 3);
  for (size_t i = 0; i < junk_count; ++i) {
    std::string junk;
    for (size_t j = 0; j < 64; ++j) {
      junk += "0123456789abcdef"[provider.ConsumeIntegralInRange<int>(0, 15)];
    }
    req.txs_hashes.emplace_back(std::move(junk));
  }

  req.decode_as_json = provider.ConsumeBool();
  req.prune = provider.ConsumeBool();
  req.split = provider.ConsumeBool();

  rpc.on_get_transactions(req, res, &ctx);
}

void fuzz_get_alt_blocks_hashes(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_ALT_BLOCKS_HASHES::request req;
  cryptonote::COMMAND_RPC_GET_ALT_BLOCKS_HASHES::response res;
  req.client = "fuzz";

  rpc.on_get_alt_blocks_hashes(req, res, &ctx);
}

void fuzz_is_key_image_spent(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_IS_KEY_IMAGE_SPENT::request req;
  cryptonote::COMMAND_RPC_IS_KEY_IMAGE_SPENT::response res;
  req.client = "fuzz";

  size_t count = provider.ConsumeIntegralInRange<size_t>(0, 16);
  for (size_t i = 0; i < count; ++i) {
    req.key_images.push_back(provider.ConsumeRandomLengthString(64));
  }

  rpc.on_is_key_image_spent(req, res, &ctx);
}

void fuzz_send_raw_tx(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_SEND_RAW_TX::request req;
  cryptonote::COMMAND_RPC_SEND_RAW_TX::response res;
  req.client = "fuzz";

  const std::string raw_tx_blob = provider.ConsumeRandomLengthString(512);
  req.tx_as_hex = epee::string_tools::buff_to_hex_nodelimer(raw_tx_blob);
  req.do_not_relay = provider.ConsumeBool();
  req.do_sanity_checks = provider.ConsumeBool();

  rpc.on_send_raw_tx(req, res, &ctx);
}

void fuzz_start_mining(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_START_MINING::request req;
  cryptonote::COMMAND_RPC_START_MINING::response res;

  req.miner_address = provider.ConsumeRandomLengthString(128);
  req.threads_count = provider.ConsumeIntegralInRange<uint64_t>(0, 256);
  req.do_background_mining = provider.ConsumeBool();
  req.ignore_battery = provider.ConsumeBool();

  rpc.on_start_mining(req, res, &ctx);
}

void fuzz_stop_mining(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_STOP_MINING::request req;
  cryptonote::COMMAND_RPC_STOP_MINING::response res;

  rpc.on_stop_mining(req, res, &ctx);
}

void fuzz_mining_status(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_MINING_STATUS::request req;
  cryptonote::COMMAND_RPC_MINING_STATUS::response res;

  rpc.on_mining_status(req, res, &ctx);
}

void fuzz_save_bc(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_SAVE_BC::request req;
  cryptonote::COMMAND_RPC_SAVE_BC::response res;

  rpc.on_save_bc(req, res, &ctx);
}

void fuzz_get_peer_list(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_PEER_LIST::request req;
  cryptonote::COMMAND_RPC_GET_PEER_LIST::response res;

  req.public_only = provider.ConsumeBool();
  req.include_blocked = provider.ConsumeBool();

  rpc.on_get_peer_list(req, res, &ctx);
}

void fuzz_get_public_nodes(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_PUBLIC_NODES::request req;
  cryptonote::COMMAND_RPC_GET_PUBLIC_NODES::response res;

  req.gray = provider.ConsumeBool();
  req.white = provider.ConsumeBool();
  req.include_blocked = provider.ConsumeBool();

  rpc.on_get_public_nodes(req, res, &ctx);
}

void fuzz_set_log_hash_rate(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_SET_LOG_HASH_RATE::request req;
  cryptonote::COMMAND_RPC_SET_LOG_HASH_RATE::response res;

  req.visible = provider.ConsumeBool();

  rpc.on_set_log_hash_rate(req, res, &ctx);
}

void fuzz_set_log_level(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_SET_LOG_LEVEL::request req;
  cryptonote::COMMAND_RPC_SET_LOG_LEVEL::response res;

  req.level = provider.ConsumeIntegral<int8_t>();

  rpc.on_set_log_level(req, res, &ctx);
}

void fuzz_set_log_categories(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_SET_LOG_CATEGORIES::request req;
  cryptonote::COMMAND_RPC_SET_LOG_CATEGORIES::response res;

  req.categories = provider.ConsumeRandomLengthString(32);

  rpc.on_set_log_categories(req, res, &ctx);
}

void fuzz_get_transaction_pool(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::request req;
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::response res;
  req.client = "fuzz";

  rpc.on_get_transaction_pool(req, res, &ctx);
}

void fuzz_get_transaction_pool_hashes_bin(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL_HASHES_BIN::request req;
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL_HASHES_BIN::response res;
  req.client = "fuzz";

  rpc.on_get_transaction_pool_hashes_bin(req, res, &ctx);
}

void fuzz_get_transaction_pool_hashes(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL_HASHES::request req;
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL_HASHES::response res;
  req.client = "fuzz";

  rpc.on_get_transaction_pool_hashes(req, res, &ctx);
}

void fuzz_get_transaction_pool_stats(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL_STATS::request req;
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL_STATS::response res;
  req.client = "fuzz";

  rpc.on_get_transaction_pool_stats(req, res, &ctx);
}

void fuzz_set_bootstrap_daemon(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_SET_BOOTSTRAP_DAEMON::request req;
  cryptonote::COMMAND_RPC_SET_BOOTSTRAP_DAEMON::response res;

  req.address = provider.ConsumeRandomLengthString(64);
  req.username = provider.ConsumeRandomLengthString(32);
  req.password = provider.ConsumeRandomLengthString(32);
  req.proxy = provider.ConsumeRandomLengthString(32);

  rpc.on_set_bootstrap_daemon(req, res, &ctx);

  // Immediate reset bootstrap daemon to avoid affecting other fuzzing with external calls
  disable_bootstrap_daemon(rpc);
}

void fuzz_stop_daemon(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_STOP_DAEMON::request req;
  cryptonote::COMMAND_RPC_STOP_DAEMON::response res;

  rpc.on_stop_daemon(req, res, &ctx);
}

void fuzz_get_info(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_INFO::request req;
  cryptonote::COMMAND_RPC_GET_INFO::response res;
  req.client = "fuzz";

  rpc.on_get_info(req, res, &ctx);
}

void fuzz_get_net_stats(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_NET_STATS::request req;
  cryptonote::COMMAND_RPC_GET_NET_STATS::response res;

  rpc.on_get_net_stats(req, res, &ctx);
}

void fuzz_get_limit(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_LIMIT::request req;
  cryptonote::COMMAND_RPC_GET_LIMIT::response res;

  rpc.on_get_limit(req, res, &ctx);
}

void fuzz_set_limit(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_SET_LIMIT::request req;
  cryptonote::COMMAND_RPC_SET_LIMIT::response res;

  req.limit_down = provider.ConsumeIntegral<int64_t>();
  req.limit_up = provider.ConsumeIntegral<int64_t>();

  rpc.on_set_limit(req, res, &ctx);
}

void fuzz_out_peers(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_OUT_PEERS::request req;
  cryptonote::COMMAND_RPC_OUT_PEERS::response res;

  req.set = provider.ConsumeBool();
  req.out_peers = provider.ConsumeIntegral<uint32_t>();

  rpc.on_out_peers(req, res, &ctx);
}

void fuzz_in_peers(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_IN_PEERS::request req;
  cryptonote::COMMAND_RPC_IN_PEERS::response res;

  req.set = provider.ConsumeBool();
  req.in_peers = provider.ConsumeIntegral<uint32_t>();

  rpc.on_in_peers(req, res, &ctx);
}

void fuzz_get_outs(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_OUTPUTS::request req;
  cryptonote::COMMAND_RPC_GET_OUTPUTS::response res;
  req.client = "fuzz";

  size_t count = provider.ConsumeIntegralInRange<size_t>(0, 5);
  for (size_t i = 0; i < count; ++i) {
    cryptonote::get_outputs_out out;
    out.amount = provider.ConsumeIntegral<uint64_t>();
    out.index = provider.ConsumeIntegral<uint64_t>();
    req.outputs.push_back(out);
  }

  req.get_txid = provider.ConsumeBool();

  rpc.on_get_outs(req, res, &ctx);
}

void fuzz_update(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_UPDATE::request req;
  cryptonote::COMMAND_RPC_UPDATE::response res;

  req.command = provider.ConsumeRandomLengthString(16);
  req.path = provider.ConsumeRandomLengthString(32);

  rpc.on_update(req, res, &ctx);
}

void fuzz_get_output_distribution_bin(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::request req;
  cryptonote::COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::response res;
  req.client = "fuzz";
  req.amounts = {0};

  req.from_height = provider.ConsumeIntegral<uint64_t>();
  req.to_height = req.from_height + provider.ConsumeIntegralInRange<uint64_t>(0, 1000);
  req.cumulative = provider.ConsumeBool();
  req.binary = provider.ConsumeBool();
  req.compress = provider.ConsumeBool();

  rpc.on_get_output_distribution_bin(req, res, &ctx);
}

void fuzz_pop_blocks(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_POP_BLOCKS::request req;
  cryptonote::COMMAND_RPC_POP_BLOCKS::response res;

  req.nblocks = provider.ConsumeIntegral<uint64_t>();
  rpc.on_pop_blocks(req, res, &ctx);
}

void fuzz_getblockcount(cryptonote::core_rpc_server& rpc, FuzzedDataProvider&) {
  cryptonote::COMMAND_RPC_GETBLOCKCOUNT::request req;
  cryptonote::COMMAND_RPC_GETBLOCKCOUNT::response res;

  rpc.on_getblockcount(req, res, &ctx);
}


// Fuzzing functions for RPC JSONAPI endpoint functions
void fuzz_getblockhash(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GETBLOCKHASH::request req;
  cryptonote::COMMAND_RPC_GETBLOCKHASH::response res;

  size_t count = provider.ConsumeIntegralInRange<size_t>(0, 10);
  for (size_t i = 0; i < count; ++i) {
    req.push_back(provider.ConsumeIntegral<uint64_t>());
  }

  rpc.on_getblockhash(req, res, error_resp, &ctx);
}

void fuzz_getblocktemplate(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GETBLOCKTEMPLATE::request req;
  cryptonote::COMMAND_RPC_GETBLOCKTEMPLATE::response res;

  if (provider.ConsumeBool()) {
    req.wallet_address = "9uVsvEryzpN8WH2t1WWhFFCG5tS8cBNdmJYNRuckLENFimfauV5pZKeS1P2CbxGkSDTUPHXWwiYE5ZGSXDAGbaZgDxobqDN";
  } else {
    req.wallet_address = provider.ConsumeRandomLengthString(128);
  }

  req.reserve_size = provider.ConsumeIntegral<uint64_t>();
  req.prev_block = provider.ConsumeRandomLengthString(64);
  req.extra_nonce = provider.ConsumeRandomLengthString(32);

  rpc.on_getblocktemplate(req, res, error_resp, &ctx);
}

void fuzz_getminerdata(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GETMINERDATA::request req;
  cryptonote::COMMAND_RPC_GETMINERDATA::response res;

  rpc.on_getminerdata(req, res, error_resp, &ctx);
}

void fuzz_calcpow(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_CALCPOW::request req;
  cryptonote::COMMAND_RPC_CALCPOW::response res;

  req.major_version = provider.ConsumeIntegral<uint8_t>();
  req.height = provider.ConsumeIntegral<uint64_t>();
  req.block_blob = provider.ConsumeRandomLengthString(128);
  req.seed_hash = provider.ConsumeRandomLengthString(64);

  rpc.on_calcpow(req, res, error_resp, &ctx);
}

void fuzz_add_aux_pow(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_ADD_AUX_POW::request req;
  cryptonote::COMMAND_RPC_ADD_AUX_POW::response res;

  req.blocktemplate_blob = provider.ConsumeRandomLengthString(128);

  size_t count = provider.ConsumeIntegralInRange<size_t>(0, 4);
  for (size_t i = 0; i < count; ++i) {
    cryptonote::COMMAND_RPC_ADD_AUX_POW::aux_pow_t aux;
    aux.id = provider.ConsumeRandomLengthString(32);
    aux.hash = provider.ConsumeRandomLengthString(64);
    req.aux_pow.push_back(aux);
  }

  rpc.on_add_aux_pow(req, res, error_resp, &ctx);
}

void fuzz_submitblock(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_SUBMITBLOCK::request req;
  cryptonote::COMMAND_RPC_SUBMITBLOCK::response res;

  std::string hash_data = provider.ConsumeBytesAsString(sizeof(crypto::hash));
  if (hash_data.size() != sizeof(crypto::hash)) return;
  crypto::hash h;
  std::memcpy(&h, hash_data.data(), sizeof(h));
  std::string hex_str = epee::string_tools::pod_to_hex(h);
  req.push_back(hex_str);

  rpc.on_submitblock(req, res, error_resp, &ctx);
}

void fuzz_generateblocks(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GENERATEBLOCKS::request req;
  cryptonote::COMMAND_RPC_GENERATEBLOCKS::response res;

  req.amount_of_blocks = provider.ConsumeIntegral<uint64_t>();
  req.wallet_address = provider.ConsumeRandomLengthString(128);
  req.prev_block = provider.ConsumeRandomLengthString(128);
  req.starting_nonce = provider.ConsumeIntegral<uint32_t>();

  rpc.on_generateblocks(req, res, error_resp, &ctx);
}

void fuzz_get_last_block_header(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_LAST_BLOCK_HEADER::request req;
  cryptonote::COMMAND_RPC_GET_LAST_BLOCK_HEADER::response res;
  req.client = "fuzz";

  req.fill_pow_hash = provider.ConsumeBool();

  rpc.on_get_last_block_header(req, res, error_resp, &ctx);
}

void fuzz_get_block_header_by_hash(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::request req;
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::response res;
  req.client = "fuzz";

  req.hash = provider.ConsumeRandomLengthString(64);

  size_t count = provider.ConsumeIntegralInRange<size_t>(0, 3);
  for (size_t i = 0; i < count; ++i) {
    req.hashes.push_back(provider.ConsumeRandomLengthString(64));
  }

  req.fill_pow_hash = provider.ConsumeBool();

  rpc.on_get_block_header_by_hash(req, res, error_resp, &ctx);
}

void fuzz_get_block_header_by_height(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::request req;
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::response res;
  req.client = "fuzz";

  req.height = provider.ConsumeIntegral<uint64_t>();
  req.fill_pow_hash = provider.ConsumeBool();

  rpc.on_get_block_header_by_height(req, res, error_resp, &ctx);
}

void fuzz_get_block_headers_range(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADERS_RANGE::request req;
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADERS_RANGE::response res;
  req.client = "fuzz";

  req.start_height = provider.ConsumeIntegral<uint64_t>();
  req.end_height = provider.ConsumeIntegral<uint64_t>();
  req.fill_pow_hash = provider.ConsumeBool();

  rpc.on_get_block_headers_range(req, res, error_resp, &ctx);
}

void fuzz_get_block(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_BLOCK::request req;
  cryptonote::COMMAND_RPC_GET_BLOCK::response res;
  req.client = "fuzz";

  req.hash = provider.ConsumeRandomLengthString(64);
  req.height = provider.ConsumeIntegral<uint64_t>();
  req.fill_pow_hash = provider.ConsumeBool();

  rpc.on_get_block(req, res, error_resp, &ctx);
}

void fuzz_get_connections(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_CONNECTIONS::request req;
  cryptonote::COMMAND_RPC_GET_CONNECTIONS::response res;

  rpc.on_get_connections(req, res, error_resp, &ctx);
}

void fuzz_get_info_json(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_INFO::request req;
  cryptonote::COMMAND_RPC_GET_INFO::response res;
  req.client = "fuzz";

  rpc.on_get_info_json(req, res, error_resp, &ctx);
}

void fuzz_hard_fork_info(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_HARD_FORK_INFO::request req;
  cryptonote::COMMAND_RPC_HARD_FORK_INFO::response res;

  req.version = provider.ConsumeIntegral<uint8_t>();

  rpc.on_hard_fork_info(req, res, error_resp, &ctx);
}

void fuzz_set_bans(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_SETBANS::request req;
  cryptonote::COMMAND_RPC_SETBANS::response res;

  int count = provider.ConsumeIntegralInRange<int>(0, 4);
  for (int i = 0; i < count; ++i) {
    cryptonote::COMMAND_RPC_SETBANS::ban entry;
    entry.host = provider.ConsumeRandomLengthString(32);
    entry.ip = provider.ConsumeIntegral<uint32_t>();
    entry.ban = provider.ConsumeBool();
    entry.seconds = provider.ConsumeIntegral<uint32_t>();
    req.bans.push_back(entry);
  }

  rpc.on_set_bans(req, res, error_resp, &ctx);
}

void fuzz_get_bans(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GETBANS::request req;
  cryptonote::COMMAND_RPC_GETBANS::response res;

  rpc.on_get_bans(req, res, error_resp, &ctx);
}

void fuzz_banned(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_BANNED::request req;
  cryptonote::COMMAND_RPC_BANNED::response res;

  req.address = provider.ConsumeRandomLengthString(32);

  rpc.on_banned(req, res, error_resp, &ctx);
}

void fuzz_flush_txpool(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_FLUSH_TRANSACTION_POOL::request req;
  cryptonote::COMMAND_RPC_FLUSH_TRANSACTION_POOL::response res;

  int count = provider.ConsumeIntegralInRange<int>(0, 8);
  for (int i = 0; i < count; ++i) {
    req.txids.push_back(provider.ConsumeRandomLengthString(64));
  }

  rpc.on_flush_txpool(req, res, error_resp, &ctx);
}

void fuzz_get_output_histogram(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_OUTPUT_HISTOGRAM::request req;
  cryptonote::COMMAND_RPC_GET_OUTPUT_HISTOGRAM::response res;
  req.client = "fuzz";

  int amount_count = provider.ConsumeIntegralInRange<int>(0, 5);
  for (int i = 0; i < amount_count; ++i) {
    req.amounts.push_back(provider.ConsumeIntegral<uint64_t>());
  }
  req.min_count = provider.ConsumeIntegral<uint64_t>();
  req.max_count = provider.ConsumeIntegral<uint64_t>();
  req.unlocked = provider.ConsumeBool();
  req.recent_cutoff = provider.ConsumeIntegral<uint64_t>();

  rpc.on_get_output_histogram(req, res, error_resp, &ctx);
}

void fuzz_get_version(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_VERSION::request req;
  cryptonote::COMMAND_RPC_GET_VERSION::response res;

  rpc.on_get_version(req, res, error_resp, &ctx);
}

void fuzz_get_coinbase_tx_sum(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_COINBASE_TX_SUM::request req;
  cryptonote::COMMAND_RPC_GET_COINBASE_TX_SUM::response res;
  req.client = "fuzz";

  req.height = provider.ConsumeIntegral<uint64_t>();
  req.count = provider.ConsumeIntegral<uint64_t>();

  rpc.on_get_coinbase_tx_sum(req, res, error_resp, &ctx);
}

void fuzz_get_base_fee_estimate(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_BASE_FEE_ESTIMATE::request req;
  cryptonote::COMMAND_RPC_GET_BASE_FEE_ESTIMATE::response res;
  req.client = "fuzz";

  req.grace_blocks = provider.ConsumeIntegral<uint64_t>();

  rpc.on_get_base_fee_estimate(req, res, error_resp, &ctx);
}

void fuzz_get_alternate_chains(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_ALTERNATE_CHAINS::request req;
  cryptonote::COMMAND_RPC_GET_ALTERNATE_CHAINS::response res;

  rpc.on_get_alternate_chains(req, res, error_resp, &ctx);
}

void fuzz_relay_tx(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_RELAY_TX::request req;
  cryptonote::COMMAND_RPC_RELAY_TX::response res;
  req.client = "fuzz";

  size_t count = provider.ConsumeIntegralInRange<size_t>(0, 4);
  for (size_t i = 0; i < count; ++i) {
    std::string hash_data = provider.ConsumeBytesAsString(sizeof(crypto::hash));
    if (hash_data.size() != sizeof(crypto::hash)) break;
    crypto::hash h;
    std::memcpy(&h, hash_data.data(), sizeof(h));
    std::string hex_str = epee::string_tools::pod_to_hex(h);
    req.txids.push_back(hex_str);
  }
  epee::json_rpc::error error_resp;
  rpc.on_relay_tx(req, res, error_resp, &ctx);
}

void fuzz_sync_info(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_SYNC_INFO::request req;
  cryptonote::COMMAND_RPC_SYNC_INFO::response res;
  req.client = "fuzz";

  rpc.on_sync_info(req, res, error_resp, &ctx);
}

void fuzz_get_txpool_backlog(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL_BACKLOG::request req;
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL_BACKLOG::response res;
  req.client = "fuzz";

  rpc.on_get_txpool_backlog(req, res, error_resp, &ctx);
}

void fuzz_get_output_distribution(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::request req;
  cryptonote::COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::response res;
  req.client = "fuzz";
  req.amounts = {0};

  req.from_height = provider.ConsumeIntegral<uint64_t>();
  req.to_height = req.from_height + provider.ConsumeIntegralInRange<uint64_t>(0, 1000);
  req.cumulative = provider.ConsumeBool();
  req.binary = provider.ConsumeBool();
  req.compress = provider.ConsumeBool();

  rpc.on_get_output_distribution(req, res, error_resp, &ctx);
}

void fuzz_prune_blockchain(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_PRUNE_BLOCKCHAIN::request req;
  cryptonote::COMMAND_RPC_PRUNE_BLOCKCHAIN::response res;

  req.check = provider.ConsumeBool();

  rpc.on_prune_blockchain(req, res, error_resp, &ctx);
}

void fuzz_flush_cache(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_FLUSH_CACHE::request req;
  cryptonote::COMMAND_RPC_FLUSH_CACHE::response res;

  req.bad_blocks = provider.ConsumeBool();

  rpc.on_flush_cache(req, res, error_resp, &ctx);
}

void fuzz_get_txids_loose(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_GET_TXIDS_LOOSE::request req;
  cryptonote::COMMAND_RPC_GET_TXIDS_LOOSE::response res;

  req.txid_template = provider.ConsumeRandomLengthString(64);
  req.num_matching_bits = provider.ConsumeIntegral<uint32_t>();

  rpc.on_get_txids_loose(req, res, error_resp, &ctx);
}

void fuzz_rpc_access_info(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_ACCESS_INFO::request req;
  cryptonote::COMMAND_RPC_ACCESS_INFO::response res;
  req.client = "fuzz";

  rpc.on_rpc_access_info(req, res, error_resp, &ctx);
}

void fuzz_rpc_access_submit_nonce(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_ACCESS_SUBMIT_NONCE::request req;
  cryptonote::COMMAND_RPC_ACCESS_SUBMIT_NONCE::response res;
  req.client = "fuzz";

  req.nonce = provider.ConsumeIntegral<uint32_t>();
  req.cookie = provider.ConsumeIntegral<uint32_t>();

  rpc.on_rpc_access_submit_nonce(req, res, error_resp, &ctx);
}

void fuzz_rpc_access_pay(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_ACCESS_PAY::request req;
  cryptonote::COMMAND_RPC_ACCESS_PAY::response res;
  req.client = "fuzz";

  req.paying_for = provider.ConsumeRandomLengthString(32);
  req.payment = provider.ConsumeIntegral<uint64_t>();

  rpc.on_rpc_access_pay(req, res, error_resp, &ctx);
}

void fuzz_rpc_access_tracking(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_ACCESS_TRACKING::request req;
  cryptonote::COMMAND_RPC_ACCESS_TRACKING::response res;

  req.clear = provider.ConsumeBool();

  rpc.on_rpc_access_tracking(req, res, error_resp, &ctx);
}

void fuzz_rpc_access_data(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_ACCESS_DATA::request req;
  cryptonote::COMMAND_RPC_ACCESS_DATA::response res;

  rpc.on_rpc_access_data(req, res, error_resp, &ctx);
}

void fuzz_rpc_access_account(cryptonote::core_rpc_server& rpc, FuzzedDataProvider& provider) {
  cryptonote::COMMAND_RPC_ACCESS_ACCOUNT::request req;
  cryptonote::COMMAND_RPC_ACCESS_ACCOUNT::response res;
  req.client = "fuzz";

  req.delta_balance = provider.ConsumeIntegral<int64_t>();

  rpc.on_rpc_access_account(req, res, error_resp, &ctx);
}

// Maps storing all fuzzing functions
std::map<int, std::function<void(cryptonote::core_rpc_server&, FuzzedDataProvider&)>> priority_fuzz_targets = {
  {0, fuzz_get_blocks},
  {1, fuzz_get_blocks_by_height},
  {2, fuzz_get_hashes},
  {3, fuzz_get_outs_bin},
  {4, fuzz_get_transactions},
  {5, fuzz_is_key_image_spent},
  {6, fuzz_send_raw_tx},
  {7, fuzz_get_output_distribution_bin},
  {8, fuzz_pop_blocks},
  {9, fuzz_getblocktemplate},
  {10, fuzz_submitblock},
  {11, fuzz_generateblocks},
  {12, fuzz_relay_tx},
  {13, fuzz_get_output_distribution},
};

std::map<int, std::function<void(cryptonote::core_rpc_server&, FuzzedDataProvider&)>> safe_fuzz_targets = {
  {14, fuzz_get_height},
  {15, fuzz_get_indexes},
  {16, fuzz_get_alt_blocks_hashes},
  {17, fuzz_get_peer_list},
  {18, fuzz_get_public_nodes},
  {19, fuzz_set_log_hash_rate},
  {20, fuzz_set_log_level},
  {21, fuzz_set_log_categories},
  {22, fuzz_get_transaction_pool},
  {23, fuzz_get_transaction_pool_hashes_bin},
  {24, fuzz_get_transaction_pool_hashes},
  {25, fuzz_get_transaction_pool_stats},
  {26, fuzz_get_info},
  {27, fuzz_get_net_stats},
  {28, fuzz_get_limit},
  {29, fuzz_set_limit},
  {30, fuzz_out_peers},
  {31, fuzz_in_peers},
  {32, fuzz_get_outs},
  {33, fuzz_getblockcount},
  {34, fuzz_getblockhash},
  {35, fuzz_getminerdata},
  {36, fuzz_calcpow},
  {37, fuzz_get_last_block_header},
  {38, fuzz_get_block_header_by_hash},
  {39, fuzz_get_block_header_by_height},
  {40, fuzz_get_block_headers_range},
  {41, fuzz_get_block},
  {42, fuzz_get_connections},
  {43, fuzz_get_info_json},
  {44, fuzz_hard_fork_info},
  {45, fuzz_set_bans},
  {46, fuzz_get_bans},
  {47, fuzz_banned},
  {48, fuzz_get_output_histogram},
  {49, fuzz_get_version},
  {50, fuzz_get_coinbase_tx_sum},
  {51, fuzz_get_base_fee_estimate},
  {52, fuzz_get_alternate_chains},
  {53, fuzz_sync_info},
  {54, fuzz_get_txpool_backlog},
};

std::map<int, std::function<void(cryptonote::core_rpc_server&, FuzzedDataProvider&)>> risky_fuzz_targets = {
  {55, fuzz_start_mining},
  {56, fuzz_stop_mining},
  {57, fuzz_mining_status},
  {58, fuzz_save_bc},
  {59, fuzz_set_bootstrap_daemon},
  {60, fuzz_stop_daemon},
  {61, fuzz_update},
  {62, fuzz_add_aux_pow},
  {63, fuzz_flush_txpool},
  {64, fuzz_flush_cache},
  {65, fuzz_get_txids_loose},
  {66, fuzz_rpc_access_info},
  {67, fuzz_rpc_access_submit_nonce},
  {68, fuzz_rpc_access_pay},
  {69, fuzz_rpc_access_tracking},
  {70, fuzz_rpc_access_data},
  {71, fuzz_rpc_access_account},
//  {72, fuzz_prune_blockchain},
};
