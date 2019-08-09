// Copyright (c) 2014-2019, The Monero Project
// Copyright (c)      2018, The Loki Project
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
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "checkpoints.h"

#include "common/dns_utils.h"
#include "string_tools.h"
#include "storages/portable_storage_template_helper.h" // epee json include
#include "serialization/keyvalue_serialization.h"
#include "cryptonote_core/service_node_rules.h"
#include <vector>
#include "syncobj.h"
#include "blockchain_db/blockchain_db.h"
#include "cryptonote_basic/cryptonote_format_utils.h"

using namespace epee;

#include "common/loki_integration_test_hooks.h"
#include "common/loki.h"

#undef LOKI_DEFAULT_LOG_CATEGORY
#define LOKI_DEFAULT_LOG_CATEGORY "checkpoints"

namespace cryptonote
{
  bool checkpoint_t::check(crypto::hash const &hash) const
  {
    bool result = block_hash == hash;
    if (result) MINFO   ("CHECKPOINT PASSED FOR HEIGHT " << height << " " << block_hash);
    else        MWARNING("CHECKPOINT FAILED FOR HEIGHT " << height << ". EXPECTED HASH " << block_hash << "GIVEN HASH: " << hash);
    return result;
  }

  height_to_hash const HARDCODED_MAINNET_CHECKPOINTS[] =
  {
    {0,      "08ff156d993012b0bdf2816c4bee47c9bbc7930593b70ee02574edddf15ee933"},
    {1,      "647997953a5ea9b5ab329c2291d4cbb08eed587c287e451eeeb2c79bab9b940f"},
    {10,     "4a7cd8b9bff380d48d6f3533a5e0509f8589cc77d18218b3f7218846e77738fc"},
    {100,    "01b8d33a50713ff837f8ad7146021b8e3060e0316b5e4afc407e46cdb50b6760"},
    {1000,   "5e3b0a1f931885bc0ab1d6ecdc625816576feae29e2f9ac94c5ccdbedb1465ac"},
    {86535,  "52b7c5a60b97bf1efbf0d63a0aa1a313e8f0abe4627eb354b0c5a73cb1f4391e"},
    {97407,  "504af73abbaba85a14ddc16634658bf4dcc241dc288b1eaad09e216836b71023"},
    {98552,  "2058d5c675bd91284f4996435593499c9ab84a5a0f569f57a86cde2e815e57da"},
    {144650, "a1ab207afc790675070ecd7aac874eb0691eb6349ea37c44f8f58697a5d6cbc4"},
    {266284, "c42801a37a41e3e9f934a266063483646072a94bfc7269ace178e93c91414b1f"},
    {301187, "e23e4cf3a2fe3e9f0ffced5cc76426e5bdffd3aad822268f4ad63d82cb958559"},
  };

  crypto::hash get_newest_hardcoded_checkpoint(cryptonote::network_type nettype, uint64_t *height)
  {
    crypto::hash result = crypto::null_hash;
    *height = 0;
    if (nettype != MAINNET)
      return result;

    uint64_t last_index         = loki::array_count(HARDCODED_MAINNET_CHECKPOINTS) - 1;
    height_to_hash const &entry = HARDCODED_MAINNET_CHECKPOINTS[last_index];

    if (epee::string_tools::hex_to_pod(entry.hash, result))
      *height = entry.height;

    return result;
  }

  bool load_checkpoints_from_json(const std::string &json_hashfile_fullpath, std::vector<height_to_hash> &checkpoint_hashes)
  {
    boost::system::error_code errcode;
    if (! (boost::filesystem::exists(json_hashfile_fullpath, errcode)))
    {
      LOG_PRINT_L1("Blockchain checkpoints file not found");
      return true;
    }

    height_to_hash_json hashes;
    if (!epee::serialization::load_t_from_json_file(hashes, json_hashfile_fullpath))
    {
      MERROR("Error loading checkpoints from " << json_hashfile_fullpath);
      return false;
    }

    checkpoint_hashes = std::move(hashes.hashlines);
    return true;
  }

  static bool get_checkpoint_from_db_safe(BlockchainDB *db, uint64_t height, checkpoint_t &checkpoint)
  {
    try
    {
      auto guard = db_rtxn_guard(db);
      return db->get_block_checkpoint(height, checkpoint);
    }
    catch (const std::exception &e)
    {
      MERROR("Get block checkpoint from DB failed at height: " << height << ", what = " << e.what());
      return false;
    }
  }
  //---------------------------------------------------------------------------
  bool checkpoints::add_checkpoint(uint64_t height, const std::string& hash_str)
  {
    crypto::hash h = crypto::null_hash;
    bool r         = epee::string_tools::hex_to_pod(hash_str, h);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse checkpoint hash string into binary representation!");

    checkpoint_t checkpoint = {};
    if (get_checkpoint_from_db_safe(m_db, height, checkpoint))
    {
      crypto::hash const &curr_hash = checkpoint.block_hash;
      CHECK_AND_ASSERT_MES(h == curr_hash, false, "Checkpoint at given height already exists, and hash for new checkpoint was different!");
    }
    else
    {
      checkpoint.type       = checkpoint_type::hardcoded;
      checkpoint.height     = height;
      checkpoint.block_hash = h;
      r                     = update_checkpoint(checkpoint);
    }

    return r;
  }
  bool checkpoints::update_checkpoint(checkpoint_t const &checkpoint)
  {
    // NOTE(loki): Assumes checkpoint is valid
    bool result        = true;
    bool batch_started = false;
    try
    {
      batch_started = m_db->batch_start();
      m_db->update_block_checkpoint(checkpoint);
    }
    catch (const std::exception& e)
    {
      MERROR("Failed to add checkpoint with hash: " << checkpoint.block_hash << " at height: " << checkpoint.height << ", what = " << e.what());
      result = false;
    }

    if (batch_started)
      m_db->batch_stop();
    return result;
  }
  //---------------------------------------------------------------------------
  void checkpoints::block_added(const cryptonote::block& block, const std::vector<cryptonote::transaction>& txs)
  {
    uint64_t const height = get_block_height(block);
    if (height < service_nodes::CHECKPOINT_STORE_PERSISTENTLY_INTERVAL ||
        block.major_version < network_version_12_checkpointing)
      return;

    if (m_nettype == MAINNET && height == HF_VERSION_12_CHECKPOINTING_SOFT_FORK_HEIGHT)
    {
      uint64_t start_height = 0;
      get_newest_hardcoded_checkpoint(m_nettype, &start_height);
      start_height += 1; // Don't start deleting from the hardcoded height

      if ((start_height % service_nodes::CHECKPOINT_INTERVAL) > 0)
        start_height += (service_nodes::CHECKPOINT_INTERVAL - (start_height % service_nodes::CHECKPOINT_INTERVAL));

      for (uint64_t delete_height = start_height;
           delete_height <= height;
           delete_height += service_nodes::CHECKPOINT_INTERVAL)
      {
        try
        {
          m_db->remove_block_checkpoint(delete_height);
        }
        catch (const std::exception &e)
        {
          MERROR(
              "Deleting historical checkpoints on mainnet soft-fork to checkpointing failed non-trivially at height: "
              << delete_height << ", what = " << e.what());
        }
      }
    }

    uint64_t end_cull_height = 0;
    {
      checkpoint_t immutable_checkpoint;
      if (m_db->get_immutable_checkpoint(&immutable_checkpoint))
        end_cull_height = immutable_checkpoint.height;
    }
    uint64_t start_cull_height = (end_cull_height < service_nodes::CHECKPOINT_STORE_PERSISTENTLY_INTERVAL)
                                     ? 0
                                     : end_cull_height - service_nodes::CHECKPOINT_STORE_PERSISTENTLY_INTERVAL;

    if ((start_cull_height % service_nodes::CHECKPOINT_INTERVAL) > 0)
      start_cull_height += (service_nodes::CHECKPOINT_INTERVAL - (start_cull_height % service_nodes::CHECKPOINT_INTERVAL));

    m_last_cull_height = std::max(m_last_cull_height, start_cull_height);
    auto guard         = db_wtxn_guard(m_db);
    for (; m_last_cull_height < end_cull_height; m_last_cull_height += service_nodes::CHECKPOINT_INTERVAL)
    {
      if (m_last_cull_height % service_nodes::CHECKPOINT_STORE_PERSISTENTLY_INTERVAL == 0)
        continue;

      try
      {
        m_db->remove_block_checkpoint(m_last_cull_height);
      }
      catch (const std::exception &e)
      {
        MERROR("Pruning block checkpoint on block added failed non-trivially at height: " << m_last_cull_height << ", what = " << e.what());
      }
    }
  }
  //---------------------------------------------------------------------------
  void checkpoints::blockchain_detached(uint64_t height)
  {
    m_last_cull_height = std::min(m_last_cull_height, height);

    checkpoint_t top_checkpoint;
    auto guard = db_wtxn_guard(m_db);
    if (m_db->get_top_checkpoint(top_checkpoint))
    {
      uint64_t start_height = top_checkpoint.height;
      for (size_t delete_height = start_height;
           delete_height >= height;
           delete_height -= service_nodes::CHECKPOINT_INTERVAL)
      {
        try
        {
          m_db->remove_block_checkpoint(delete_height);
        }
        catch (const std::exception &e)
        {
          MERROR("Remove block checkpoint on detach failed non-trivially at height: " << delete_height << ", what = " << e.what());
        }
      }
    }
  }
  //---------------------------------------------------------------------------
  bool checkpoints::is_in_checkpoint_zone(uint64_t height) const
  {
    uint64_t top_checkpoint_height = 0;
    checkpoint_t top_checkpoint;
    if (m_db->get_top_checkpoint(top_checkpoint))
      top_checkpoint_height = top_checkpoint.height;

    return height <= top_checkpoint_height;
  }
  //---------------------------------------------------------------------------
  bool checkpoints::check_block(uint64_t height, const crypto::hash& h, bool* is_a_checkpoint, bool *rejected_by_service_node) const
  {
    checkpoint_t checkpoint;
    bool found = get_checkpoint_from_db_safe(m_db, height, checkpoint);
    if (is_a_checkpoint) *is_a_checkpoint = found;

    if(!found)
      return true;

    bool result = checkpoint.check(h);
    if (rejected_by_service_node)
      *rejected_by_service_node = checkpoint.type == checkpoint_type::service_node && result;

    return result;
  }
  //---------------------------------------------------------------------------
  bool checkpoints::is_alternative_block_allowed(uint64_t blockchain_height, uint64_t block_height, bool *rejected_by_service_node)
  {
    if (rejected_by_service_node)
      *rejected_by_service_node = false;

    if (0 == block_height)
      return false;

    checkpoint_t immutable_checkpoint;
    uint64_t immutable_height = 0;
    if (m_db->get_immutable_checkpoint(&immutable_checkpoint))
    {
      immutable_height = immutable_checkpoint.height;
      if (rejected_by_service_node)
        *rejected_by_service_node = (immutable_checkpoint.type == checkpoint_type::service_node);
    }

    m_oldest_allowable_alternative_block = std::max(immutable_height, m_oldest_allowable_alternative_block);
    bool result                          = block_height > m_oldest_allowable_alternative_block;
    return result;
  }
  //---------------------------------------------------------------------------
  uint64_t checkpoints::get_max_height() const
  {
    uint64_t result = 0;
    checkpoint_t top_checkpoint;
    if (m_db->get_top_checkpoint(top_checkpoint))
      result = top_checkpoint.height;

    return result;
  }
  //---------------------------------------------------------------------------
  bool checkpoints::init(network_type nettype, struct BlockchainDB *db)
  {
    *this     = {};
    m_db      = db;
    m_nettype = nettype;

#if !defined(LOKI_ENABLE_INTEGRATION_TEST_HOOKS)
    if (nettype == MAINNET)
    {
      for (size_t i = 0; i < loki::array_count(HARDCODED_MAINNET_CHECKPOINTS); ++i)
      {
        height_to_hash const &checkpoint = HARDCODED_MAINNET_CHECKPOINTS[i];
        ADD_CHECKPOINT(checkpoint.height, checkpoint.hash);
      }
    }
#endif

    return true;
  }

}

