// Copyright (c) 2014, The Monero Project
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

#include "include_base_utils.h"
#include "common/util.h"
#include "warnings.h"
#include "crypto/crypto.h"
#include "cryptonote_config.h"
#include "cryptonote_core/cryptonote_format_utils.h"
#include "misc_language.h"
#include "cryptonote_core/blockchain_storage.h"
#include "cryptonote_core/blockchain_db.h"
#include "cryptonote_core/blockchain.h"
#include "cryptonote_core/BlockchainDB_impl/db_lmdb.h"
#include "cryptonote_core/tx_pool.h"
#include <iostream>

using namespace cryptonote;

struct fake_core
{
  tx_memory_pool m_pool;
  Blockchain dummy;

  blockchain_storage m_storage;


  fake_core(const boost::filesystem::path &path) : m_pool(dummy), dummy(m_pool), m_storage(&m_pool)
  {
    m_pool.init(path.string());
    m_storage.init(path.string(), false);
  }
};

int main(int argc, char* argv[])
{
  std::string dir = tools::get_default_data_dir();
  boost::filesystem::path default_data_path {dir};
  if (argc >= 2 && !strcmp(argv[1], "--testnet")) {
    default_data_path /= "testnet";
  }

  fake_core c(default_data_path);

  BlockchainDB *blockchain;

  blockchain = new BlockchainLMDB();

  blockchain->open(default_data_path.string());

  for (uint64_t height, i = 0; i < (height = c.m_storage.get_current_blockchain_height()); ++i)
  {
    if (i % 10 == 0)
    {
      std::cout << "\r                   \r" << "block " << i << "/" << height
         << " (" << (i+1)*100/height<< "%)" << std::flush;
    }
    block b = c.m_storage.get_block(i);
    size_t bsize = c.m_storage.get_block_size(i);
    difficulty_type bdiff = c.m_storage.get_block_cumulative_difficulty(i);
    uint64_t bcoins = c.m_storage.get_block_coins_generated(i);
    std::vector<transaction> txs;
    std::vector<crypto::hash> missed;

    c.m_storage.get_transactions(b.tx_hashes, txs, missed);
    if (missed.size())
    {
      std::cout << std::endl;
      std::cerr << "Missed transaction(s) for block at height " << i << ", exiting" << std::endl;
      delete blockchain;
      return 1;
    }

    try
    {
      blockchain->add_block(b, bsize, bdiff, bcoins, txs);
    }
    catch (const std::exception& e)
    {
      std::cout << std::endl;
      std::cerr << "Error adding block to new blockchain: " << e.what() << std::endl;
      delete blockchain;
      return 2;
    }
  }

  std::cout << std::endl;
  delete blockchain;
  return 0;
}
