#pragma once

#include "serialization/keyvalue_serialization.h"

namespace cryptonote {
  struct tx_info
  {
    std::string id_hash;
    std::string tx_json; // TODO - expose this data directly
    uint64_t blob_size;
    uint64_t fee;
    std::string max_used_block_id_hash;
    uint64_t max_used_block_height;
    bool kept_by_block;
    uint64_t last_failed_height;
    std::string last_failed_id_hash;
    uint64_t receive_time;

    tx_info() = default;

    tx_info(
          std::string id_hash
        , std::string tx_json
        , uint64_t blob_size
        , uint64_t fee
        , std::string max_used_block_id_hash
        , uint64_t max_used_block_height
        , bool kept_by_block
        , uint64_t last_failed_height
        , std::string last_failed_id_hash
        , uint64_t receive_time
        )
      : id_hash(std::move(id_hash))
      , tx_json(tx_json)
      , blob_size(blob_size)
      , fee(fee)
      , max_used_block_id_hash(std::move(max_used_block_id_hash))
      , kept_by_block(kept_by_block)
      , last_failed_height(last_failed_height)
      , last_failed_id_hash(std::move(last_failed_id_hash))
      , receive_time(receive_time)
    {}

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(id_hash)
      KV_SERIALIZE(tx_json)
      KV_SERIALIZE(blob_size)
      KV_SERIALIZE(fee)
      KV_SERIALIZE(max_used_block_id_hash)
      KV_SERIALIZE(max_used_block_height)
      KV_SERIALIZE(kept_by_block)
      KV_SERIALIZE(last_failed_height)
      KV_SERIALIZE(last_failed_id_hash)
      KV_SERIALIZE(receive_time)
    END_KV_SERIALIZE_MAP()
  };
}
