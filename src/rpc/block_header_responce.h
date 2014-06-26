#pragma once

#include "serialization/keyvalue_serialization.h"

namespace cryptonote {
  struct block_header_responce
  {
      uint8_t major_version;
      uint8_t minor_version;
      uint64_t timestamp;
      std::string prev_hash;
      uint32_t nonce;
      bool orphan_status;
      uint64_t height;
      uint64_t depth;
      std::string hash;
      uint64_t difficulty;
      uint64_t reward;
      uint64_t tx_count;
      uint64_t cumulative_difficulty;

      block_header_responce() = default;

      block_header_responce(
          uint8_t major_version
        , uint8_t minor_version
        , uint64_t timestamp
        , std::string prev_hash
        , uint32_t nonce
        , bool orphan_status
        , uint64_t height
        , uint64_t depth
        , std::string hash
        , uint64_t difficulty
        , uint64_t reward
        , uint64_t tx_count
        , uint64_t cumulative_difficulty
        )
        : major_version(major_version)
        , minor_version(minor_version)
        , timestamp(timestamp)
        , prev_hash(prev_hash)
        , nonce(nonce)
        , orphan_status(orphan_status)
        , height(height)
        , depth(depth)
        , hash(hash)
        , difficulty(difficulty)
        , reward(reward)
        , tx_count(tx_count)
        , cumulative_difficulty(cumulative_difficulty)
      {}

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(major_version)
        KV_SERIALIZE(minor_version)
        KV_SERIALIZE(timestamp)
        KV_SERIALIZE(prev_hash)
        KV_SERIALIZE(nonce)
        KV_SERIALIZE(orphan_status)
        KV_SERIALIZE(height)
        KV_SERIALIZE(depth)
        KV_SERIALIZE(hash)
        KV_SERIALIZE(difficulty)
        KV_SERIALIZE(reward)
        KV_SERIALIZE(tx_count)
        KV_SERIALIZE(cumulative_difficulty)
      END_KV_SERIALIZE_MAP()
  };
}
