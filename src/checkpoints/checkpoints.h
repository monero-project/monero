// Copyright (c) 2014-2019, The Monero Project
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

#pragma once
#include <map>
#include <vector>

#include "misc_log_ex.h"
#include "crypto/hash.h"
#include "cryptonote_config.h"
#include "cryptonote_core/service_node_voting.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "string_tools.h"

#define ADD_CHECKPOINT(h, hash)  CHECK_AND_ASSERT(add_checkpoint(h,  hash), false);
#define JSON_HASH_FILE_NAME "checkpoints.json"

namespace cryptonote
{
  struct Blockchain;
  enum struct checkpoint_type
  {
    hardcoded,
    service_node,
    count,
  };

  struct checkpoint_t
  {
    uint8_t                                        version = 0;
    checkpoint_type                                type;
    uint64_t                                       height;
    crypto::hash                                   block_hash;
    std::vector<service_nodes::voter_to_signature> signatures; // Only service node checkpoints use signatures
    uint64_t                                       prev_height;

    bool               check         (crypto::hash const &block_hash) const;
    static char const *type_to_string(checkpoint_type type)
    {
      switch(type)
      {
        case checkpoint_type::hardcoded:    return "Hardcoded";
        case checkpoint_type::service_node: return "ServiceNode";
        default: assert(false);             return "XXUnhandledVersion";
      }
    }

    BEGIN_SERIALIZE()
      FIELD(version)
      ENUM_FIELD(type, type < checkpoint_type::count);
      FIELD(height)
      FIELD(block_hash)
      FIELD(signatures)
      FIELD(prev_height)
    END_SERIALIZE()

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(version)
      KV_SERIALIZE(height)

      std::string type = checkpoint_t::type_to_string(this_ref.type);
      KV_SERIALIZE_VALUE(type);

      std::string block_hash = epee::string_tools::pod_to_hex(this_ref.block_hash);
      KV_SERIALIZE_VALUE(block_hash);

      KV_SERIALIZE(signatures)
      KV_SERIALIZE(prev_height)
    END_KV_SERIALIZE_MAP()
  };

  struct height_to_hash
  {
    uint64_t height; //!< the height of the checkpoint
    std::string hash; //!< the hash for the checkpoint
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(height)
          KV_SERIALIZE(hash)
        END_KV_SERIALIZE_MAP()
  };

  /**
   * @brief struct for loading many checkpoints from json
   */
  struct height_to_hash_json {
    std::vector<height_to_hash> hashlines; //!< the checkpoint lines from the file
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(hashlines)
        END_KV_SERIALIZE_MAP()
  };

  crypto::hash get_newest_hardcoded_checkpoint(cryptonote::network_type nettype, uint64_t *height);
  bool         load_checkpoints_from_json     (const std::string &json_hashfile_fullpath, std::vector<height_to_hash> &checkpoint_hashes);

  /**
   * @brief A container for blockchain checkpoints
   *
   * A checkpoint is a pre-defined hash for the block at a given height.
   * Some of these are compiled-in, while others can be loaded at runtime
   * either from a json file or via DNS from a checkpoint-hosting server.
   */
  class checkpoints
    : public cryptonote::BlockAddedHook,
      public cryptonote::BlockchainDetachedHook
  {
  public:
    void block_added(const cryptonote::block& block, const std::vector<cryptonote::transaction>& txs) override;
    void blockchain_detached(uint64_t height) override;

    /**
     * @brief adds a checkpoint to the container
     *
     * @param height the height of the block the checkpoint is for
     * @param hash_str the hash of the block, as a string
     *
     * @return false if parsing the hash fails, or if the height is a duplicate
     *         AND the existing checkpoint hash does not match the new one,
     *         otherwise returns true
     */
    bool add_checkpoint(uint64_t height, const std::string& hash_str);

    bool update_checkpoint(checkpoint_t const &checkpoint);

    /*
       @brief Remove checkpoints that should not be stored persistently, i.e.
       any checkpoint whose height is not divisible by
       service_nodes::CHECKPOINT_STORE_PERSISTENTLY_INTERVAL
     */
    void prune_checkpoints(uint64_t height) const;

    /**
     * @brief checks if there is a checkpoint in the future
     *
     * This function checks if the height passed is lower than the highest
     * checkpoint.
     *
     * @param height the height to check against
     *
     * @return false if no checkpoints, otherwise returns whether or not
     *         the height passed is lower than the highest checkpoint.
     */
    bool is_in_checkpoint_zone(uint64_t height) const;

    /**
     * @brief checks if the given height and hash agree with the checkpoints
     *
     * This function checks if the given height and hash exist in the
     * checkpoints container.  If so, it returns whether or not the passed
     * parameters match the stored values.
     *
     * @param height the height to be checked
     * @param h the hash to be checked
     * @param blockchain the blockchain to query ancestor blocks from the current height
     * @param is_a_checkpoint optional return-by-pointer if there is a checkpoint at the given height
     *
     * @return true if there is no checkpoint at the given height,
     *         true if the passed parameters match the stored checkpoint,
     *         false otherwise
     */
    bool check_block(uint64_t height, const crypto::hash& h, bool *is_a_checkpoint = nullptr, bool *rejected_by_service_node = nullptr) const;

    /**
     * @brief checks if alternate chain blocks should be kept for a given height and updates
     * m_oldest_allowable_alternative_block based on the available checkpoints
     *
     * this basically says if the blockchain is smaller than the first
     * checkpoint then alternate blocks are allowed.  Alternatively, if the
     * last checkpoint *before* the end of the current chain is also before
     * the block to be added, then this is fine.
     *
     * @param blockchain_height the current blockchain height
     * @param block_height the height of the block to be added as alternate
     *
     * @return true if alternate blocks are allowed given the parameters,
     *         otherwise false
     */
    bool is_alternative_block_allowed(uint64_t blockchain_height, uint64_t block_height, bool *rejected_by_service_node = nullptr);

    /**
     * @brief gets the highest checkpoint height
     *
     * @return the height of the highest checkpoint
     */
    uint64_t get_max_height() const;

    /**
     * @brief loads the default main chain checkpoints
     * @param nettype network type
     *
     * @return true unless adding a checkpoint fails
     */
    bool init(network_type nettype, class BlockchainDB *db);

  private:
    network_type m_nettype = UNDEFINED;
    uint64_t m_last_cull_height = 0;
    uint64_t m_oldest_allowable_alternative_block = 0;
    BlockchainDB *m_db;
  };

}
