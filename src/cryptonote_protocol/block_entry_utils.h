// Copyright (c) 2014-2026, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <unordered_set>
#include <utility>
#include <vector>

#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_core/tx_verification_utils.h"
#include "cryptonote_protocol_defs.h"
#include "misc_log_ex.h"

namespace cryptonote
{
  template <class CryptoHashContainer>
  inline bool make_pool_supplement_from_block_entry(
    const std::vector<cryptonote::tx_blob_entry>& tx_entries,
    const CryptoHashContainer& blk_tx_hashes,
    const bool allow_pruned,
    cryptonote::pool_supplement& pool_supplement)
  {
    if (tx_entries.size() > blk_tx_hashes.size())
    {
      MCERROR("verify", "Failed to make pool supplement: Too many transaction blobs!");
      return false;
    }

    for (const cryptonote::tx_blob_entry& tx_entry: tx_entries)
    {
      const bool is_pruned = tx_entry.prunable_hash != crypto::null_hash;
      if (is_pruned && !allow_pruned)
      {
        MCERROR("verify", "Pruned transaction not allowed here");
        return false;
      }

      cryptonote::transaction tx;
      crypto::hash tx_hash;
      bool parse_success = false;
      const bool max_size_check = true;
      if (is_pruned)
      {
        if ((parse_success = cryptonote::parse_and_validate_tx_base_from_blob(tx_entry.blob, tx, max_size_check)))
          parse_success = cryptonote::get_pruned_transaction_hash(tx, tx_entry.prunable_hash, tx_hash);
      }
      else
      {
        parse_success = cryptonote::parse_and_validate_tx_from_blob(tx_entry.blob, tx, tx_hash, max_size_check);
      }

      if (!parse_success)
      {
        MCERROR("verify", "failed to parse and/or validate transaction: "
          << epee::string_tools::buff_to_hex_nodelimer(tx_entry.blob)
        );
        return false;
      }
      else if (!blk_tx_hashes.count(tx_hash))
      {
        MCERROR("verify", "transaction " << tx_hash << " not in block");
        return false;
      }

      if (!pool_supplement.add_tx(tx_hash, std::move(tx), tx_entry.blob))
      {
        MCERROR("verify", "Duplicate transaction " << tx_hash << " in block entry");
        return false;
      }
    }

    return true;
  }

  inline bool make_full_pool_supplement_from_block_entry(
    const cryptonote::block_complete_entry& blk_entry,
    cryptonote::pool_supplement& pool_supplement)
  {
    cryptonote::block blk;
    if (!cryptonote::parse_and_validate_block_from_blob(blk_entry.block, blk))
    {
      MCERROR("verify", "sent bad block: failed to parse and/or validate block: "
        << epee::string_tools::buff_to_hex_nodelimer(blk_entry.block)
      );
      return false;
    }

    const std::unordered_set<crypto::hash> blk_tx_hashes(blk.tx_hashes.cbegin(), blk.tx_hashes.cend());

    if (blk_tx_hashes.size() != blk_entry.txs.size())
    {
      MCERROR("verify", "sent bad block entry: number of hashes is not equal number of tx blobs: "
        << epee::string_tools::buff_to_hex_nodelimer(blk_entry.block)
      );
      return false;
    }
    else if (blk_tx_hashes.size() != blk.tx_hashes.size())
    {
      MCERROR("verify", "sent bad block entry: there are duplicate tx hashes in parsed block: "
        << epee::string_tools::buff_to_hex_nodelimer(blk_entry.block));
      return false;
    }

    // We set `allow_pruned` equal to whether this block entry is pruned since the pruned flag
    // should be checked anyways by the time we deserialize transactions
    return make_pool_supplement_from_block_entry(blk_entry.txs, blk_tx_hashes, blk_entry.pruned, pool_supplement);
  }
} // namespace cryptonote
