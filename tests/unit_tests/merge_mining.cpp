// Copyright (c) 2026, The Monero Project
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

#include "gtest/gtest.h"

#include <algorithm>
#include <limits>
#include <boost/uuid/uuid_io.hpp>

#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_basic/tx_extra.h"
#include "cryptonote_core/cryptonote_tx_utils.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.inl"
#include "p2p/net_node.inl"
#include "rpc/core_rpc_server.h"
#include "rpc/core_rpc_server_error_codes.h"
#include "string_tools.h"

namespace
{
  constexpr uint8_t hf_version = 16;
  constexpr size_t median_weight = CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V5;
  constexpr size_t txs_weight = median_weight + median_weight / 10;
  constexpr uint64_t fee = 10000000000;

  class merge_mining : public testing::Test
  {
  protected:
    merge_mining()
      : m_core(nullptr), m_protocol(m_core, nullptr), m_p2p(m_protocol), m_rpc(m_core, m_p2p)
    {
      m_miner.generate();
    }

    bool make_template(cryptonote::block& block, const std::string& nonce, bool reserve = true)
    {
      block = {};
      block.major_version = hf_version;
      block.minor_version = hf_version;
      block.timestamp = 1700000000;
      block.nonce = 123456;
      block.tx_hashes.push_back(crypto::null_hash);
      size_t weight = txs_weight;
      for (size_t attempt = 0; attempt < 10; ++attempt)
      {
        if (!cryptonote::construct_miner_tx(100, median_weight, MONEY_SUPPLY, weight, fee,
            m_miner.get_keys().m_account_address, block.miner_tx, nonce, 1, hf_version, reserve))
          return false;
        const size_t actual_weight = txs_weight + cryptonote::get_transaction_weight(block.miner_tx);
        if (actual_weight == weight)
          return true;
        weight = actual_weight;
      }
      return false;
    }

    cryptonote::COMMAND_RPC_ADD_AUX_POW::request make_request(const cryptonote::block& block, size_t count)
    {
      cryptonote::COMMAND_RPC_ADD_AUX_POW::request request;
      request.blocktemplate_blob = epee::string_tools::buff_to_hex_nodelimer(
        cryptonote::t_serializable_object_to_blob(block));
      for (size_t i = 0; i < count; ++i)
      {
        crypto::hash id = crypto::null_hash;
        crypto::hash hash = crypto::null_hash;
        id.data[0] = i;
        hash.data[0] = i + 1;
        request.aux_pow.push_back({epee::string_tools::pod_to_hex(id), epee::string_tools::pod_to_hex(hash)});
      }
      return request;
    }

    void check_replacement(const cryptonote::block& before,
        const cryptonote::COMMAND_RPC_ADD_AUX_POW::response& response, cryptonote::block& after)
    {
      cryptonote::blobdata blob;
      ASSERT_TRUE(epee::string_tools::parse_hexstr_to_binbuff(response.blocktemplate_blob, blob));
      ASSERT_TRUE(cryptonote::parse_and_validate_block_from_blob(blob, after));
      EXPECT_EQ(before.miner_tx.extra.size(), after.miner_tx.extra.size());
      EXPECT_EQ(cryptonote::get_transaction_weight(before.miner_tx),
        cryptonote::get_transaction_weight(after.miner_tx));
      EXPECT_EQ(epee::string_tools::buff_to_hex_nodelimer(cryptonote::get_block_hashing_blob(after)),
        response.blockhashing_blob);

      cryptonote::block restored = after;
      restored.miner_tx.extra = before.miner_tx.extra;
      EXPECT_EQ(cryptonote::t_serializable_object_to_blob(before),
        cryptonote::t_serializable_object_to_blob(restored));
      std::vector<uint8_t> before_extra = before.miner_tx.extra;
      std::vector<uint8_t> after_extra = after.miner_tx.extra;
      for (auto* extra : {&before_extra, &after_extra})
      {
        ASSERT_TRUE(cryptonote::remove_field_from_tx_extra(*extra, typeid(cryptonote::tx_extra_merge_mining_tag)));
        ASSERT_TRUE(cryptonote::remove_field_from_tx_extra(*extra, typeid(cryptonote::tx_extra_padding)));
      }
      EXPECT_EQ(before_extra, after_extra);
      ASSERT_LE(before_extra.size(), after.miner_tx.extra.size());
      EXPECT_TRUE(std::equal(before_extra.begin(), before_extra.end(), after.miner_tx.extra.begin()));

      std::vector<cryptonote::tx_extra_field> fields;
      ASSERT_TRUE(cryptonote::parse_tx_extra(after.miner_tx.extra, fields));
      cryptonote::tx_extra_merge_mining_tag tag;
      ASSERT_TRUE(cryptonote::find_tx_extra_field_by_type(fields, tag));
      EXPECT_EQ(response.merkle_tree_depth, tag.depth);
      EXPECT_EQ(response.merkle_root, epee::string_tools::pod_to_hex(tag.merkle_root));
      EXPECT_EQ(1, std::count_if(fields.begin(), fields.end(), [](const cryptonote::tx_extra_field& field) {
        return field.type() == typeid(cryptonote::tx_extra_merge_mining_tag);
      }));
    }

    cryptonote::core m_core;
    cryptonote::t_cryptonote_protocol_handler<cryptonote::core> m_protocol;
    nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core>> m_p2p;
    cryptonote::core_rpc_server m_rpc;
    cryptonote::account_base m_miner;
  };
}

TEST_F(merge_mining, reservation_is_opt_in)
{
  cryptonote::block block;
  for (const bool reserve : {false, true})
  {
    ASSERT_TRUE(make_template(block, "mining nonce", reserve));
    std::vector<cryptonote::tx_extra_field> fields;
    ASSERT_TRUE(cryptonote::parse_tx_extra(block.miner_tx.extra, fields));
    cryptonote::tx_extra_padding padding;
    ASSERT_EQ(reserve, cryptonote::find_tx_extra_field_by_type(fields, padding));
    cryptonote::tx_extra_merge_mining_tag tag;
    EXPECT_FALSE(cryptonote::find_tx_extra_field_by_type(fields, tag));
    if (reserve)
    {
      std::vector<uint8_t> largest_tag;
      ASSERT_TRUE(cryptonote::add_mm_merkle_root_to_tx_extra(largest_tag, crypto::null_hash,
        std::numeric_limits<uint64_t>::max()));
      EXPECT_GE(padding.size, largest_tag.size());
    }
  }
}

TEST_F(merge_mining, penalized_reward_and_reserved_nonce_survive_replacement)
{
  // Straddle the extra-length varint boundary.
  for (const size_t nonce_size : {0, 1, 47, 48, 49, 50, 127, 255})
  {
    SCOPED_TRACE(nonce_size);
    const std::string nonce(nonce_size, '\x5a');
    cryptonote::block block;
    ASSERT_TRUE(make_template(block, nonce));
    uint64_t full_reward, reward;
    ASSERT_TRUE(cryptonote::get_block_reward(median_weight, median_weight, MONEY_SUPPLY, full_reward, hf_version));
    ASSERT_TRUE(cryptonote::get_block_reward(median_weight,
      txs_weight + cryptonote::get_transaction_weight(block.miner_tx), MONEY_SUPPLY, reward, hf_version));
    ASSERT_LT(reward, full_reward);
    ASSERT_EQ(reward + fee, cryptonote::get_outs_money_amount(block.miner_tx));

    // These IDs produce one-, two-, and three-byte depth encodings, then shrink again.
    for (const size_t count : {1, 4, 10, 1})
    {
      SCOPED_TRACE(count);
      cryptonote::COMMAND_RPC_ADD_AUX_POW::response response;
      epee::json_rpc::error error;
      ASSERT_TRUE(m_rpc.on_add_aux_pow(make_request(block, count), response, error)) << error.message;
      EXPECT_EQ(CORE_RPC_STATUS_OK, response.status);
      if (count > 1)
        EXPECT_GE(response.merkle_tree_depth, 128);
      if (count == 10)
        EXPECT_GE(response.merkle_tree_depth, 16384);
      cryptonote::block updated;
      check_replacement(block, response, updated);
      ASSERT_FALSE(HasFatalFailure());
      uint64_t permitted_reward;
      ASSERT_TRUE(cryptonote::get_block_reward(median_weight,
        txs_weight + cryptonote::get_transaction_weight(updated.miner_tx),
        MONEY_SUPPLY, permitted_reward, hf_version));
      EXPECT_EQ(permitted_reward + fee, cryptonote::get_outs_money_amount(updated.miner_tx));
      block = std::move(updated);
    }
  }
}

TEST_F(merge_mining, rejects_unreserved_growth)
{
  cryptonote::block block;
  ASSERT_TRUE(make_template(block, "reserve nonce", false));
  cryptonote::COMMAND_RPC_ADD_AUX_POW::response response;
  epee::json_rpc::error error;
  EXPECT_FALSE(m_rpc.on_add_aux_pow(make_request(block, 1), response, error));
  EXPECT_EQ(CORE_RPC_ERROR_CODE_WRONG_PARAM, error.code);
  EXPECT_TRUE(response.blocktemplate_blob.empty());
}

TEST_F(merge_mining, existing_tag_requires_space_only_when_it_grows)
{
  cryptonote::block block;
  ASSERT_TRUE(make_template(block, "reserve nonce", false));
  ASSERT_TRUE(cryptonote::add_mm_merkle_root_to_tx_extra(block.miner_tx.extra, crypto::null_hash, 0));
  block.miner_tx.invalidate_hashes();

  cryptonote::COMMAND_RPC_ADD_AUX_POW::response response;
  epee::json_rpc::error error;
  ASSERT_TRUE(m_rpc.on_add_aux_pow(make_request(block, 1), response, error)) << error.message;
  cryptonote::block updated;
  check_replacement(block, response, updated);
  ASSERT_FALSE(HasFatalFailure());

  response = {};
  error = {};
  EXPECT_FALSE(m_rpc.on_add_aux_pow(make_request(updated, 4), response, error));
  EXPECT_EQ(CORE_RPC_ERROR_CODE_WRONG_PARAM, error.code);
  EXPECT_TRUE(response.blocktemplate_blob.empty());
}

TEST_F(merge_mining, reservation_supports_the_full_depth_range)
{
  cryptonote::block block;
  ASSERT_TRUE(make_template(block, "reserve nonce"));
  const size_t weight = cryptonote::get_transaction_weight(block.miner_tx);
  const size_t extra_size = block.miner_tx.extra.size();
  crypto::hash root = crypto::null_hash;
  root.data[0] = 1;

  for (const uint64_t depth : {std::numeric_limits<uint64_t>::max(), uint64_t(0),
      std::numeric_limits<uint64_t>::max()})
  {
    ASSERT_TRUE(cryptonote::update_mm_merkle_root_in_tx_extra(block.miner_tx.extra, root, depth));
    block.miner_tx.invalidate_hashes();
    EXPECT_EQ(extra_size, block.miner_tx.extra.size());
    EXPECT_EQ(weight, cryptonote::get_transaction_weight(block.miner_tx));
    std::vector<cryptonote::tx_extra_field> fields;
    ASSERT_TRUE(cryptonote::parse_tx_extra(block.miner_tx.extra, fields));
    cryptonote::tx_extra_merge_mining_tag tag;
    ASSERT_TRUE(cryptonote::find_tx_extra_field_by_type(fields, tag));
    EXPECT_EQ(depth, tag.depth);
    EXPECT_EQ(root, tag.merkle_root);
  }
}

TEST_F(merge_mining, invalid_extra_is_unchanged)
{
  cryptonote::block block;
  ASSERT_TRUE(make_template(block, "reserve nonce"));
  block.miner_tx.extra.back() = TX_EXTRA_TAG_PUBKEY;
  const auto original = block.miner_tx.extra;
  EXPECT_FALSE(cryptonote::update_mm_merkle_root_in_tx_extra(block.miner_tx.extra, crypto::null_hash, 0));
  EXPECT_EQ(original, block.miner_tx.extra);
}

TEST_F(merge_mining, rejects_replacement_requiring_excess_padding)
{
  std::vector<uint8_t> extra;
  ASSERT_TRUE(cryptonote::add_mm_merkle_root_to_tx_extra(extra, crypto::null_hash,
    std::numeric_limits<uint64_t>::max()));
  extra.insert(extra.end(), TX_EXTRA_PADDING_MAX_COUNT, 0);
  std::vector<cryptonote::tx_extra_field> fields;
  ASSERT_TRUE(cryptonote::parse_tx_extra(extra, fields));
  const auto original = extra;
  // A shorter depth would require more than 255 bytes of padding.
  EXPECT_FALSE(cryptonote::update_mm_merkle_root_in_tx_extra(extra, crypto::null_hash, 0));
  EXPECT_EQ(original, extra);
}
