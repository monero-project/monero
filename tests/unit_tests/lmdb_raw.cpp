// Copyright (c) 2014-2018, The Monero Project
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
#include <boost/filesystem.hpp>
#include "misc_language.h"
#include <gtest/gtest.h>
#include <lmdb.h>
#include <thread>

TEST(lmdb_raw, writemap_slow_reader_fast_writer)
{
  using workers_t = std::vector<std::thread>;
  using tag_t = uint64_t;
  using ec_t = boost::system::error_code;
  using path_t = boost::filesystem::path;

  MDB_env *env;
  auto error = mdb_env_create(&env);
  ASSERT_TRUE(not error);
  error = mdb_env_set_maxdbs(env, 1);
  ASSERT_TRUE(not error);
  auto create_dir = []{
    ec_t ec;
    path_t path = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path("lmdb-%%%%%%%%%%%%%%%%", ec);
    if (ec)
      return path_t{};
    auto success = boost::filesystem::create_directory(path, ec);
    if (not ec && success)
      return path;
    return path_t{};
  };
  auto dir = create_dir();
  ASSERT_TRUE(not dir.empty());
  error = mdb_env_open(env, dir.c_str(), MDB_NOSYNC | MDB_WRITEMAP | MDB_MAPASYNC, 0644);
  ASSERT_TRUE(not error);
  error = mdb_env_set_mapsize(env, mdb_size_t(1ULL << 30));
  ASSERT_TRUE(not error);
  MDB_dbi db;
  {
    MDB_txn *txn;
    error = mdb_txn_begin(env, nullptr, 0, &txn);
    ASSERT_TRUE(not error);
    error = mdb_dbi_open(txn, "QUEUE", MDB_INTEGERKEY | MDB_CREATE, &db);
    ASSERT_TRUE(not error);
    error = mdb_txn_commit(txn);
    ASSERT_TRUE(not error);
  }
  workers_t workers;
  auto running = true;
  workers.emplace_back([&env, &db, &running]{
    auto on_scope_exit = epee::misc_utils::create_scope_leave_handler([&running]{
      running = false;
    });
    constexpr auto MAX_SIZE = 5500;
    constexpr auto TAGS_PER_TXN = 100;
    constexpr auto MAX_TAG = 2000000;
    tag_t tag = 0;
    auto size = 0;
    while (tag + TAGS_PER_TXN < MAX_TAG) {
      MDB_txn *txn;
      auto error = mdb_txn_begin(env, nullptr, 0, &txn);
      ASSERT_TRUE(not error);
      MDB_cursor *cursor;
      error = mdb_cursor_open(txn, db, &cursor);
      ASSERT_TRUE(not error);
      for (auto i = 0; i < TAGS_PER_TXN; ++i) {
        MDB_val key_ptr{sizeof(tag), (void *)&tag};
        error = mdb_cursor_put(cursor, &key_ptr, &key_ptr, 0);
        ASSERT_TRUE(not error);
        ++tag;
        ++size;
      }
      while (size > MAX_SIZE) {
        error = mdb_cursor_get(cursor, nullptr, nullptr, MDB_FIRST);
        ASSERT_TRUE(not error);
        error = mdb_cursor_del(cursor, 0);
        ASSERT_TRUE(not error);
        --size;
      }
      error = mdb_txn_commit(txn);
      ASSERT_TRUE(not error);
    }
  });
  workers.emplace_back([&env, &db, &running]{
    constexpr auto READ_DELAY = 10000;
    while (running) {
      MDB_txn *txn;
      auto error = mdb_txn_begin(env, nullptr, MDB_RDONLY , &txn);
      ASSERT_TRUE(not error);
      std::this_thread::sleep_for(std::chrono::nanoseconds(READ_DELAY));
      mdb_txn_abort(txn);
    }
  });
  for (auto &w: workers)
    w.join();
  mdb_env_close(env);
  auto remove_tree = [](const path_t &path){
    ec_t ec;
    boost::filesystem::remove_all(path, ec);
  };
  remove_tree(dir);
}
