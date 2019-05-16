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

using namespace epee;

#include "common/loki_integration_test_hooks.h"
#include "common/loki.h"

#undef LOKI_DEFAULT_LOG_CATEGORY
#define LOKI_DEFAULT_LOG_CATEGORY "checkpoints"

namespace cryptonote
{
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

  static bool get_checkpoint_from_db_safe(BlockchainDB const *db, uint64_t height, checkpoint_t &checkpoint)
  {
    try
    {
      return db->get_block_checkpoint(height, checkpoint);
    }
    catch (const std::exception &e)
    {
      MERROR("Get block checkpoint from DB failed at height: " << height << ", what = " << e.what());
      return false;
    }
  }

  static bool update_checkpoint_in_db_safe(BlockchainDB *db, checkpoint_t &checkpoint)
  {
    try
    {
      db->update_block_checkpoint(checkpoint);
    }
    catch (const std::exception& e)
    {
      MERROR("Failed to add checkpoint with hash: " << checkpoint.block_hash << " at height: " << checkpoint.height << ", what = " << e.what());
      return false;
    }

    return true;
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
      checkpoint.height       = height;
      checkpoint.type         = checkpoint_type::hardcoded;
      checkpoint.block_hash   = h;
      r = update_checkpoint_in_db_safe(m_db, checkpoint);
    }

    return r;
  }
  //---------------------------------------------------------------------------
  static bool add_vote_if_unique(checkpoint_t &checkpoint, service_nodes::checkpoint_vote const &vote)
  {
    CHECK_AND_ASSERT_MES(checkpoint.block_hash == vote.block_hash,
                         false,
                         "Developer error: Add vote if unique should only be called when the vote hash and checkpoint hash match");

    // TODO(doyle): Factor this out, a similar function exists in service node deregister
    CHECK_AND_ASSERT_MES(vote.voters_quorum_index < service_nodes::QUORUM_SIZE,
                         false,
                         "Vote is indexing out of bounds");

    const auto signature_it = std::find_if(checkpoint.signatures.begin(), checkpoint.signatures.end(), [&vote](service_nodes::voter_to_signature const &check) {
      return vote.voters_quorum_index == check.quorum_index;
    });

    if (signature_it == checkpoint.signatures.end())
    {
      service_nodes::voter_to_signature new_voter_to_signature = {};
      new_voter_to_signature.quorum_index                      = vote.voters_quorum_index;
      new_voter_to_signature.signature                         = vote.signature;
      checkpoint.signatures.push_back(new_voter_to_signature);
      return true;
    }

    return false;
  }
  //---------------------------------------------------------------------------
  bool checkpoints::add_checkpoint_vote(service_nodes::checkpoint_vote const &vote)
  {
   // TODO(doyle): Always accept votes. Later on, we should only accept votes up
   // to a certain point, so that eventually once a certain number of blocks
   // have transpired, we can go through all our "seen" votes and deregister
   // anyone who has not participated in voting by that point.

#if 0
    uint64_t newest_checkpoint_height = get_max_height();
    if (vote.block_height < newest_checkpoint_height)
      return true;
#endif

    // TODO(doyle): Double work. Factor into a generic vote checker as we already have one in service node deregister
    std::array<int, service_nodes::QUORUM_SIZE> unique_vote_set = {};
    std::vector<checkpoint_t> &candidate_checkpoints            = m_staging_points[vote.block_height];
    std::vector<checkpoint_t>::iterator curr_checkpoint         = candidate_checkpoints.end();
    for (auto it = candidate_checkpoints.begin(); it != candidate_checkpoints.end(); it++)
    {
      checkpoint_t const &checkpoint = *it;
      if (checkpoint.block_hash == vote.block_hash)
        curr_checkpoint = it;

      for (service_nodes::voter_to_signature const &vote_to_sig : checkpoint.signatures)
      {
        if (vote_to_sig.quorum_index > unique_vote_set.size())
          return false;

        if (++unique_vote_set[vote_to_sig.quorum_index] > 1)
        {
          // NOTE: Voter is trying to vote twice
          return false;
        }
      }
    }

    if (curr_checkpoint == candidate_checkpoints.end())
    {
      checkpoint_t new_checkpoint = {};
      new_checkpoint.height       = vote.block_height;
      new_checkpoint.type         = checkpoint_type::service_node;
      new_checkpoint.block_hash   = vote.block_hash;
      candidate_checkpoints.push_back(new_checkpoint);
      curr_checkpoint = (candidate_checkpoints.end() - 1);
    }

    if (add_vote_if_unique(*curr_checkpoint, vote))
    {
      if (curr_checkpoint->signatures.size() > service_nodes::MIN_VOTES_TO_CHECKPOINT)
      {
        update_checkpoint_in_db_safe(m_db, *curr_checkpoint);
      }
    }

    return true;
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
  bool checkpoints::check_block(uint64_t height, const crypto::hash& h, bool* is_a_checkpoint) const
  {
    checkpoint_t checkpoint;
    bool found = get_checkpoint_from_db_safe(m_db, height, checkpoint);
    if (is_a_checkpoint) *is_a_checkpoint = found;

    if(!found)
      return true;

    bool result = checkpoint.block_hash == h;
    if (result) MINFO   ("CHECKPOINT PASSED FOR HEIGHT " << height << " " << h);
    else        MWARNING("CHECKPOINT FAILED FOR HEIGHT " << height << ". EXPECTED HASH " << checkpoint.block_hash << "FETCHED HASH: " << h);
    return result;
  }
  //---------------------------------------------------------------------------
  bool checkpoints::is_alternative_block_allowed(uint64_t blockchain_height, uint64_t block_height) const
  {
    if (0 == block_height)
      return false;

    size_t num_desired_checkpoints = 2;
    std::vector<checkpoint_t> checkpoints = m_db->get_checkpoints_range(blockchain_height, 0, num_desired_checkpoints);

    if (checkpoints.size() == 0) // No checkpoints recorded yet for blocks preceeding blockchain_height
      return true;

    uint64_t sentinel_reorg_height = 0;
    if (checkpoints[0].type == checkpoint_type::service_node) // checkpoint[0] is the first closest checkpoint that is <= my height
    {
      // NOTE: The current checkpoint is a service node checkpoint. Go back
      // 1 checkpoint, which will either be another service node checkpoint or
      // a predefined one.
      if (checkpoints.size() == 1)
      {
        return true; // NOTE: Only one service node checkpoint recorded, we can override this checkpoint.
      }
      else
      {
        // If it's a service node checkpoint, this is the 2nd newest checkpoint,
        // so we can't reorg past that height. If it's predefined, that's ok as
        // well, we can't reorg past that height so irrespective, always accept
        // the height of this next checkpoint.
        sentinel_reorg_height = checkpoints[1].height;
      }
    }
    else
    {
      sentinel_reorg_height = checkpoints[0].height;
    }

    bool result = sentinel_reorg_height < block_height;
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
    *this = {};
    m_db = db;

    db_wtxn_guard txn_guard(m_db);
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

