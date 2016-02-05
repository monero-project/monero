// Copyright (c) 2014-2016, The Monero Project
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
#include "cryptonote_core/blockchain_storage.h" // in-memory DB
#include "blockchain_db/blockchain_db.h"
#include "blockchain_db/lmdb/db_lmdb.h"

using namespace cryptonote;


#if !defined(BLOCKCHAIN_DB) || BLOCKCHAIN_DB == DB_LMDB

struct fake_core_lmdb
{
  Blockchain m_storage;
  tx_memory_pool m_pool;
  bool support_batch;
  bool support_add_block;

  // for multi_db_runtime:
#if !defined(BLOCKCHAIN_DB)
  fake_core_lmdb(const boost::filesystem::path &path, const bool use_testnet=false, const bool do_batch=true, const int mdb_flags=0) : m_pool(&m_storage), m_storage(m_pool)
  // for multi_db_compile:
#else
  fake_core_lmdb(const boost::filesystem::path &path, const bool use_testnet=false, const bool do_batch=true, const int mdb_flags=0) : m_pool(m_storage), m_storage(m_pool)
#endif
  {
    m_pool.init(path.string());

    BlockchainDB* db = new BlockchainLMDB();

    boost::filesystem::path folder(path);

    folder /= db->get_db_name();

    LOG_PRINT_L0("Loading blockchain from folder " << folder.string() << " ...");

    const std::string filename = folder.string();
    try
    {
      db->open(filename, mdb_flags);
    }
    catch (const std::exception& e)
    {
      LOG_PRINT_L0("Error opening database: " << e.what());
      throw;
    }

    db->check_hard_fork_info();

    m_storage.init(db, use_testnet);

    if (do_batch)
      m_storage.get_db().set_batch_transactions(do_batch);
    support_batch = true;
    support_add_block = true;
  }
  ~fake_core_lmdb()
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

  void batch_start(uint64_t batch_num_blocks = 0)
  {
    m_storage.get_db().batch_start(batch_num_blocks);
  }

  void batch_stop()
  {
    m_storage.get_db().batch_stop();
  }

};
#endif

#if !defined(BLOCKCHAIN_DB) || BLOCKCHAIN_DB == DB_MEMORY

struct fake_core_memory
{
  blockchain_storage m_storage;
  tx_memory_pool m_pool;
  bool support_batch;
  bool support_add_block;

  // for multi_db_runtime:
#if !defined(BLOCKCHAIN_DB)
  fake_core_memory(const boost::filesystem::path &path, const bool use_testnet=false) : m_pool(&m_storage), m_storage(m_pool)
#else
  // for multi_db_compile:
  fake_core_memory(const boost::filesystem::path &path, const bool use_testnet=false) : m_pool(m_storage), m_storage(&m_pool)
#endif
  {
    m_pool.init(path.string());
    m_storage.init(path.string(), use_testnet);
    support_batch = false;
    support_add_block = false;
  }
  ~fake_core_memory()
  {
    LOG_PRINT_L3("fake_core_memory() destructor called - want to see it ripple down");
    m_storage.deinit();
  }

  uint64_t add_block(const block& blk
                            , const size_t& block_size
                            , const difficulty_type& cumulative_difficulty
                            , const uint64_t& coins_generated
                            , const std::vector<transaction>& txs
                            )
  {
    // TODO:
    // would need to refactor handle_block_to_main_chain() to have a direct add_block() method like Blockchain class
    throw std::runtime_error("direct add_block() method not implemented for in-memory db");
    return 2;
  }

  void batch_start(uint64_t batch_num_blocks = 0)
  {
    LOG_PRINT_L0("WARNING: [batch_start] opt_batch set, but this database doesn't support/need transactions - ignoring");
  }

  void batch_stop()
  {
    LOG_PRINT_L0("WARNING: [batch_stop] opt_batch set, but this database doesn't support/need transactions - ignoring");
  }

};

#endif
