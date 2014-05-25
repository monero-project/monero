// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <map>
#include "cryptonote_basic_impl.h"


namespace cryptonote
{
  class checkpoints
  {
  public:
    checkpoints();
    bool add_checkpoint(uint64_t height, const std::string& hash_str);
    bool is_in_checkpoint_zone(uint64_t height) const;
    bool check_block(uint64_t height, const crypto::hash& h) const;
    bool check_block(uint64_t height, const crypto::hash& h, bool& is_a_checkpoint) const;
    bool is_alternative_block_allowed(uint64_t blockchain_height, uint64_t block_height) const;

  private:
    std::map<uint64_t, crypto::hash> m_points;
  };
}
