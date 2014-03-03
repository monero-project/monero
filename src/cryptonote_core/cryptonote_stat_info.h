// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "serialization/keyvalue_serialization.h"


namespace cryptonote
{
  struct core_stat_info
  {
    uint64_t tx_pool_size;
    uint64_t blockchain_height;
    uint64_t mining_speed;
    uint64_t alternative_blocks;
    std::string top_block_id_str;
    
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(tx_pool_size)
      KV_SERIALIZE(blockchain_height)
      KV_SERIALIZE(mining_speed)
      KV_SERIALIZE(alternative_blocks)
      KV_SERIALIZE(top_block_id_str)
    END_KV_SERIALIZE_MAP()
  };
}
