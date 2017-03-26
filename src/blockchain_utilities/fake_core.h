// Copyright (c) 2014-2017, The Monero Project
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

#include <boost/filesystem.hpp>
#include "cryptonote_core/blockchain.h" // BlockchainDB
#include "cryptonote_core/tx_pool.h"
#include "blockchain_db/blockchain_db.h"
#include "blockchain_db/lmdb/db_lmdb.h"
#if defined(BERKELEY_DB)
#include "blockchain_db/berkeleydb/db_bdb.h"
#endif

using namespace cryptonote;

namespace
{
  // NOTE: These values should match blockchain.cpp
  // TODO: Refactor
  const uint64_t mainnet_hard_fork_version_1_till = 1009826;
  const uint64_t testnet_hard_fork_version_1_till = 624633;
}


struct fake_core_db
{
  Blockchain m_storage;
  HardFork* m_hardfork = nullptr;
  tx_memory_pool m_pool;
  bool support_batch;
  bool support_add_block;

  // for multi_db_runtime:
  fake_core_db(const boost::filesystem::path &path, const bool use_testnet=false, const bool do_batch=true, const std::string& db_type="lmdb", const int db_flags=0) : m_pool(m_storage), m_storage(m_pool)
  {
    m_pool.init(path.string());

    BlockchainDB* db = nullptr;
    if (db_type == "lmdb")
      db = new BlockchainLMDB();
#if defined(BERKELEY_DB)
    else if (db_type == "berkeley")
      db = new BlockchainBDB();
#endif
    else
    {
      LOG_ERROR("Attempted to use non-existent database type: " << db_type);
      throw std::runtime_error("Attempting to use non-existent database type");
    }

    boost::filesystem::path folder(path);

    folder /= db->get_db_name();

    LOG_PRINT_L0("Loading blockchain from folder " << folder.string() << " ...");

    const std::string filename = folder.string();
    try
    {
      db->open(filename, db_flags);
    }
    catch (const std::exception& e)
    {
      LOG_PRINT_L0("Error opening database: " << e.what());
      throw;
    }

    db->check_hard_fork_info();

    uint64_t hard_fork_version_1_till = use_testnet ? testnet_hard_fork_version_1_till : mainnet_hard_fork_version_1_till;
    m_hardfork = new HardFork(*db, 1, hard_fork_version_1_till);

    m_storage.init(db, m_hardfork, use_testnet);

    if (do_batch)
      m_storage.get_db().set_batch_transactions(do_batch);
    support_batch = true;
    support_add_block = true;
  }
  ~fake_core_db()
  {
    m_storage.get_db().check_hard_fork_info();
    m_storage.deinit();
  }

  uint64_t add_block(const block& blk
                            , const size_t& block_size
                            , const difficulty_type& cumulative_difficulty
                            , const uint64_t& coins_generated
                            , const std::vector<transaction>& txs
                            )
  {
    return m_storage.get_db().add_block(blk, block_size, cumulative_difficulty, coins_generated, txs);
  }

  bool batch_start(uint64_t batch_num_blocks = 0)
  {
    return m_storage.get_db().batch_start(batch_num_blocks);
  }

  void batch_stop()
  {
    m_storage.get_db().batch_stop();
  }

};

