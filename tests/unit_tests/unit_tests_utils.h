// Copyright (c) 2014-2024, The Monero Project
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

#include "gtest/gtest.h"

#include "blockchain_db/blockchain_db.h"
#include "blockchain_db/lmdb/db_lmdb.h"
#include "fcmp_pp/curve_trees.h"
#include "misc_log_ex.h"

#include <atomic>
#include <boost/filesystem.hpp>

namespace unit_test
{
  extern boost::filesystem::path data_dir;

  class call_counter
  {
  public:
    call_counter()
      : m_counter(0)
    {
    }

    void inc() volatile
    {
      // memory_order_relaxed is enough for call counter
      m_counter.fetch_add(1, std::memory_order_relaxed);
    }

    size_t get() volatile const
    {
      return m_counter.load(std::memory_order_relaxed);
    }

    void reset() volatile
    {
      m_counter.store(0, std::memory_order_relaxed);
    }

  private:
    std::atomic<size_t> m_counter;
  };

  class BlockchainLMDBTest
  {
  public:
    BlockchainLMDBTest(bool is_copy = false) :
      m_temp_db_dir(boost::filesystem::temp_directory_path().string() + "/monero-lmdb-tests/"),
      m_is_copy{is_copy}
    {}

    ~BlockchainLMDBTest()
    {
      delete m_db;
      if (m_temp_db_dir.find("/monero-lmdb-tests/") == std::string::npos)
      {
        LOG_ERROR("unexpected temp db dir");
        return;
      }
      if (!m_is_copy)
        boost::filesystem::remove_all(m_temp_db_dir);
    }

    void init_new_db(std::shared_ptr<fcmp_pp::curve_trees::CurveTreesV1> curve_trees)
    {
      CHECK_AND_ASSERT_THROW_MES(this->m_db == nullptr, "expected nullptr m_db");
      this->m_db = new cryptonote::BlockchainLMDB(true/*batch_transactions*/, curve_trees);

      const auto temp_db_path = boost::filesystem::unique_path();
      const std::string dir_path = m_temp_db_dir + temp_db_path.string();

      MDEBUG("Creating test db at path " << dir_path);
      ASSERT_NO_THROW(this->m_db->open(dir_path));
      m_cur_dir_path = dir_path;
    }

    void init_hardfork(cryptonote::HardFork *hardfork)
    {
      hardfork->init();
      this->m_db->set_hard_fork(hardfork);
    }

    BlockchainLMDBTest *copy_db(std::shared_ptr<fcmp_pp::curve_trees::CurveTreesV1> curve_trees)
    {
      CHECK_AND_ASSERT_THROW_MES(this->m_db != nullptr, "expected non-null m_db");
      CHECK_AND_ASSERT_THROW_MES(this->m_cur_dir_path != "", "expected cur dir path set");

      const boost::filesystem::path lmdb_data_path = boost::filesystem::path(m_cur_dir_path + "/data.mdb");
      CHECK_AND_ASSERT_THROW_MES(boost::filesystem::exists(lmdb_data_path), "did not find lmdb data file");

      // Close db, copy db file, open copy, then reopen the db
      this->m_db->close();
      const auto temp_db_path = boost::filesystem::unique_path();
      const std::string dest_path = m_temp_db_dir + temp_db_path.string();
      CHECK_AND_ASSERT_THROW_MES(boost::filesystem::create_directories(dest_path),
        "failed to create new db dirs");
      boost::filesystem::copy_file(lmdb_data_path, dest_path + "/data.mdb");

      // Open db copy
      BlockchainLMDBTest *copy_db = new BlockchainLMDBTest(true/*is_copy*/);
      copy_db->m_db = new cryptonote::BlockchainLMDB(true/*batch_transactions*/, curve_trees);
      copy_db->m_db->open(dest_path);
      copy_db->m_cur_dir_path = dest_path;

      // Reopen original db so it's ready for use
      this->m_db->open(m_cur_dir_path);

      return copy_db;
    }

    cryptonote::BlockchainDB* m_db{nullptr};
    const std::string m_temp_db_dir;
    std::string m_cur_dir_path{""};
    const bool m_is_copy{false};
  };
}

#define INIT_BLOCKCHAIN_LMDB_TEST_DB(test_db, curve_trees) \
  if (curve_trees != nullptr) \
    test_db.init_new_db(curve_trees); \
  auto hardfork = cryptonote::HardFork(*test_db.m_db, 1, 0); \
  test_db.init_hardfork(&hardfork); \
  auto scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&](){ \
    ASSERT_NO_THROW(test_db.m_db->close()); \
    delete test_db.m_db; \
    test_db.m_db = nullptr; \
  })

# define ASSERT_EQ_MAP(val, map, key) \
  do { \
    auto found = map.find(key); \
    ASSERT_TRUE(found != map.end()); \
    ASSERT_EQ(val, found->second); \
  } while (false)
