// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 

namespace boost
{
  namespace serialization
  {


    template<class archive_t>
    void serialize(archive_t & ar, cryptonote::blockchain_storage::transaction_chain_entry& te, const unsigned int version)
    {
      ar & te.tx;
      ar & te.m_keeper_block_height;
      ar & te.m_blob_size;
      ar & te.m_global_output_indexes;
    }

    template<class archive_t>
    void serialize(archive_t & ar, cryptonote::blockchain_storage::block_extended_info& ei, const unsigned int version)
    {
      ar & ei.bl;
      ar & ei.height;
      ar & ei.cumulative_difficulty;
      ar & ei.block_cumulative_size;
      ar & ei.already_generated_coins;
    }

  }
}
