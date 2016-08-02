#pragma once

#include "crypto/hash.h"
#include "cryptonote_core/cryptonote_basic.h"
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

  struct outputs_for_amount
  {
    uint64_t amount;
    std::vector<output_key_and_amount_index> outputs;
  };

}  // namespace rpc

}  // namespace cryptonote
