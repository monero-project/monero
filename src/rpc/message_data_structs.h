#pragma once

#include "crypto/hash.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "ringct/rctSigs.h"

#include <unordered_map>
#include <vector>

namespace cryptonote
{

namespace rpc
{

  struct block_with_transactions
  {
    cryptonote::block block;
    std::unordered_map<crypto::hash, cryptonote::transaction> transactions;
  };

  typedef std::vector<uint64_t> tx_output_indices;

  typedef std::vector<tx_output_indices> block_output_indices;

  struct transaction_info
  {
    cryptonote::transaction transaction;
    bool in_pool;
    uint64_t height;
  };

  struct output_key_and_amount_index
  {
    uint64_t amount_index;
    crypto::public_key key;
  };

  typedef std::vector<output_key_and_amount_index> outputs_for_amount;

  struct amount_with_random_outputs
  {
    uint64_t amount;
    outputs_for_amount outputs;
  };

  struct peer
  {
    uint64_t id;
    uint32_t ip;
    uint16_t port;
    uint64_t last_seen;
  };

  // hate to duplicate tx_memory_pool::tx_details here, but
  // including tx_pool.h seems unnecessarily heavy
  struct tx_in_pool
  {
    cryptonote::transaction tx;
    uint64_t blob_size;
    uint64_t fee;
    crypto::hash max_used_block_hash;
    uint64_t max_used_block_height;
    bool kept_by_block;
    crypto::hash last_failed_block_hash;
    uint64_t last_failed_block_height;
    uint64_t receive_time;

    // parameters present in tx_memory_pool::tx_details but
    // not in the old RPC.  May as well include.
    uint64_t last_relayed_time;
    bool relayed;
  };

  struct output_amount_count
  {
    uint64_t amount;
    uint64_t total_count;
    uint64_t unlocked_count;
    uint64_t recent_count;
  };

  struct output_amount_and_index
  {
    uint64_t amount;
    uint64_t index;
  };

  struct output_key_mask_unlocked
  {
    crypto::public_key key;
    rct::key mask;
    bool unlocked;
  };

  struct hard_fork_info
  {
    uint8_t version;
    bool enabled;
    uint32_t window;
    uint32_t votes;
    uint32_t threshold;
    uint8_t voting;
    uint32_t state;
    uint64_t earliest_height;
  };

  //required by JSON-RPC 2.0 spec
  struct error
  {
    // not really using code, maybe later.
    error() : use(false), code(1) { }

    bool use;  // do not serialize

    int32_t code;

    // not required by spec, but int error codes aren't perfect
    std::string error_str;

    std::string message;

    //TODO: data member?  not required, may want later.
  };

  struct BlockHeaderResponse
  {
    uint64_t major_version;
    uint64_t minor_version;
    uint64_t timestamp;
    crypto::hash  prev_id;
    uint32_t nonce;
    uint64_t height;
    uint64_t depth;
    crypto::hash hash;
    uint64_t difficulty;
    uint64_t reward;
  };

}  // namespace rpc

}  // namespace cryptonote
