// Copyright (c) 2014-2018, The Monero Project
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

#include <algorithm>
#include <cstdio>
#include <boost/filesystem.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include "include_base_utils.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "tx_pool.h"
#include "blockchain.h"
#include "blockchain_db/blockchain_db.h"
#include "cryptonote_basic/cryptonote_boost_serialization.h"
#include "cryptonote_config.h"
#include "cryptonote_basic/miner.h"
#include "misc_language.h"
#include "profile_tools.h"
#include "file_io_utils.h"
#include "int-util.h"
#include "common/threadpool.h"
#include "common/boost_serialization_helper.h"
#include "warnings.h"
#include "crypto/hash.h"
#include "cryptonote_core.h"
#include "ringct/rctSigs.h"
#include "common/perf_timer.h"
#include "common/notify.h"
#include "common/varint.h"
#include "common/pruning.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "blockchain"

#define FIND_BLOCKCHAIN_SUPPLEMENT_MAX_SIZE (100*1024*1024) // 100 MB

using namespace crypto;

//#include "serialization/json_archive.h"

/* TODO:
 *  Clean up code:
 *    Possibly change how outputs are referred to/indexed in blockchain and wallets
 *
 */

using namespace cryptonote;
using epee::string_tools::pod_to_hex;
extern "C" void slow_hash_allocate_state();
extern "C" void slow_hash_free_state();

DISABLE_VS_WARNINGS(4267)

#define MERROR_VER(x) MCERROR("verify", x)

// used to overestimate the block reward when estimating a per kB to use
#define BLOCK_REWARD_OVERESTIMATE (10 * 1000000000000)

static const struct {
  uint8_t version;
  uint64_t height;
  uint8_t threshold;
  time_t time;
} mainnet_hard_forks[] = {
  // version 1 from the start of the blockchain
  { 1, 1, 0, 1341378000 },

  // version 2 starts from block 1009827, which is on or around the 20th of March, 2016. Fork time finalised on 2015-09-20. No fork voting occurs for the v2 fork.
  { 2, 1009827, 0, 1442763710 },

  // version 3 starts from block 1141317, which is on or around the 24th of September, 2016. Fork time finalised on 2016-03-21.
  { 3, 1141317, 0, 1458558528 },
  
  // version 4 starts from block 1220516, which is on or around the 5th of January, 2017. Fork time finalised on 2016-09-18.
  { 4, 1220516, 0, 1483574400 },
  
  // version 5 starts from block 1288616, which is on or around the 15th of April, 2017. Fork time finalised on 2017-03-14.
  { 5, 1288616, 0, 1489520158 },  

  // version 6 starts from block 1400000, which is on or around the 16th of September, 2017. Fork time finalised on 2017-08-18.
  { 6, 1400000, 0, 1503046577 },

  // version 7 starts from block 1546000, which is on or around the 6th of April, 2018. Fork time finalised on 2018-03-17.
  { 7, 1546000, 0, 1521303150 },

  // version 8 starts from block 1685555, which is on or around the 18th of October, 2018. Fork time finalised on 2018-09-02.
  { 8, 1685555, 0, 1535889547 },

  // version 9 starts from block 1686275, which is on or around the 19th of October, 2018. Fork time finalised on 2018-09-02.
  { 9, 1686275, 0, 1535889548 },
};
static const uint64_t mainnet_hard_fork_version_1_till = 1009826;

static const struct {
  uint8_t version;
  uint64_t height;
  uint8_t threshold;
  time_t time;
} testnet_hard_forks[] = {
  // version 1 from the start of the blockchain
  { 1, 1, 0, 1341378000 },

  // version 2 starts from block 624634, which is on or around the 23rd of November, 2015. Fork time finalised on 2015-11-20. No fork voting occurs for the v2 fork.
  { 2, 624634, 0, 1445355000 },

  // versions 3-5 were passed in rapid succession from September 18th, 2016
  { 3, 800500, 0, 1472415034 },
  { 4, 801219, 0, 1472415035 },
  { 5, 802660, 0, 1472415036 + 86400*180 }, // add 5 months on testnet to shut the update warning up since there's a large gap to v6

  { 6, 971400, 0, 1501709789 },
  { 7, 1057027, 0, 1512211236 },
  { 8, 1057058, 0, 1533211200 },
  { 9, 1057778, 0, 1533297600 },
};
static const uint64_t testnet_hard_fork_version_1_till = 624633;

static const struct {
  uint8_t version;
  uint64_t height;
  uint8_t threshold;
  time_t time;
} stagenet_hard_forks[] = {
  // version 1 from the start of the blockchain
  { 1, 1, 0, 1341378000 },

  // versions 2-7 in rapid succession from March 13th, 2018
  { 2, 32000, 0, 1521000000 },
  { 3, 33000, 0, 1521120000 },
  { 4, 34000, 0, 1521240000 },
  { 5, 35000, 0, 1521360000 },
  { 6, 36000, 0, 1521480000 },
  { 7, 37000, 0, 1521600000 },
  { 8, 176456, 0, 1537821770 },
  { 9, 177176, 0, 1537821771 },
};

//------------------------------------------------------------------
Blockchain::Blockchain(tx_memory_pool& tx_pool) :
  m_db(), m_tx_pool(tx_pool), m_hardfork(NULL), m_timestamps_and_difficulties_height(0), m_current_block_cumul_weight_limit(0), m_current_block_cumul_weight_median(0),
  m_enforce_dns_checkpoints(false), m_max_prepare_blocks_threads(4), m_db_sync_on_blocks(true), m_db_sync_threshold(1), m_db_sync_mode(db_async), m_db_default_sync(false), m_fast_sync(true), m_show_time_stats(false), m_sync_counter(0), m_bytes_to_sync(0), m_cancel(false),
  m_difficulty_for_next_block_top_hash(crypto::null_hash),
  m_difficulty_for_next_block(1),
  m_btc_valid(false)
{
  LOG_PRINT_L3("Blockchain::" << __func__);
}
//------------------------------------------------------------------
Blockchain::~Blockchain()
{
  try { deinit(); }
  catch (const std::exception &e) { /* ignore */ }
}
//------------------------------------------------------------------
bool Blockchain::have_tx(const crypto::hash &id) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  // WARNING: this function does not take m_blockchain_lock, and thus should only call read only
  // m_db functions which do not depend on one another (ie, no getheight + gethash(height-1), as
  // well as not accessing class members, even read only (ie, m_invalid_blocks). The caller must
  // lock if it is otherwise needed.
  return m_db->tx_exists(id);
}
//------------------------------------------------------------------
bool Blockchain::have_tx_keyimg_as_spent(const crypto::key_image &key_im) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  // WARNING: this function does not take m_blockchain_lock, and thus should only call read only
  // m_db functions which do not depend on one another (ie, no getheight + gethash(height-1), as
  // well as not accessing class members, even read only (ie, m_invalid_blocks). The caller must
  // lock if it is otherwise needed.
  return  m_db->has_key_image(key_im);
}
//------------------------------------------------------------------
// This function makes sure that each "input" in an input (mixins) exists
// and collects the public key for each from the transaction it was included in
// via the visitor passed to it.
template <class visitor_t>
bool Blockchain::scan_outputkeys_for_indexes(size_t tx_version, const txin_to_key& tx_in_to_key, visitor_t &vis, const crypto::hash &tx_prefix_hash, uint64_t* pmax_related_block_height) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);

  // ND: Disable locking and make method private.
  //CRITICAL_REGION_LOCAL(m_blockchain_lock);

  // verify that the input has key offsets (that it exists properly, really)
  if(!tx_in_to_key.key_offsets.size())
    return false;

  // cryptonote_format_utils uses relative offsets for indexing to the global
  // outputs list.  that is to say that absolute offset #2 is absolute offset
  // #1 plus relative offset #2.
  // TODO: Investigate if this is necessary / why this is done.
  std::vector<uint64_t> absolute_offsets = relative_output_offsets_to_absolute(tx_in_to_key.key_offsets);
  std::vector<output_data_t> outputs;

  bool found = false;
  auto it = m_scan_table.find(tx_prefix_hash);
  if (it != m_scan_table.end())
  {
    auto its = it->second.find(tx_in_to_key.k_image);
    if (its != it->second.end())
    {
      outputs = its->second;
      found = true;
    }
  }

  if (!found)
  {
    try
    {
      m_db->get_output_key(epee::span<const uint64_t>(&tx_in_to_key.amount, 1), absolute_offsets, outputs, true);
      if (absolute_offsets.size() != outputs.size())
      {
        MERROR_VER("Output does not exist! amount = " << tx_in_to_key.amount);
        return false;
      }
    }
    catch (...)
    {
      MERROR_VER("Output does not exist! amount = " << tx_in_to_key.amount);
      return false;
    }
  }
  else
  {
    // check for partial results and add the rest if needed;
    if (outputs.size() < absolute_offsets.size() && outputs.size() > 0)
    {
      MDEBUG("Additional outputs needed: " << absolute_offsets.size() - outputs.size());
      std::vector < uint64_t > add_offsets;
      std::vector<output_data_t> add_outputs;
      add_outputs.reserve(absolute_offsets.size() - outputs.size());
      for (size_t i = outputs.size(); i < absolute_offsets.size(); i++)
        add_offsets.push_back(absolute_offsets[i]);
      try
      {
        m_db->get_output_key(epee::span<const uint64_t>(&tx_in_to_key.amount, 1), add_offsets, add_outputs, true);
        if (add_offsets.size() != add_outputs.size())
        {
          MERROR_VER("Output does not exist! amount = " << tx_in_to_key.amount);
          return false;
        }
      }
      catch (...)
      {
        MERROR_VER("Output does not exist! amount = " << tx_in_to_key.amount);
        return false;
      }
      outputs.insert(outputs.end(), add_outputs.begin(), add_outputs.end());
    }
  }

  size_t count = 0;
  for (const uint64_t& i : absolute_offsets)
  {
    try
    {
      output_data_t output_index;
      try
      {
        // get tx hash and output index for output
        if (count < outputs.size())
          output_index = outputs.at(count);
        else
          output_index = m_db->get_output_key(tx_in_to_key.amount, i);

        // call to the passed boost visitor to grab the public key for the output
        if (!vis.handle_output(output_index.unlock_time, output_index.pubkey, output_index.commitment))
        {
          MERROR_VER("Failed to handle_output for output no = " << count << ", with absolute offset " << i);
          return false;
        }
      }
      catch (...)
      {
        MERROR_VER("Output does not exist! amount = " << tx_in_to_key.amount << ", absolute_offset = " << i);
        return false;
      }

      // if on last output and pmax_related_block_height not null pointer
      if(++count == absolute_offsets.size() && pmax_related_block_height)
      {
        // set *pmax_related_block_height to tx block height for this output
        auto h = output_index.height;
        if(*pmax_related_block_height < h)
        {
          *pmax_related_block_height = h;
        }
      }

    }
    catch (const OUTPUT_DNE& e)
    {
      MERROR_VER("Output does not exist: " << e.what());
      return false;
    }
    catch (const TX_DNE& e)
    {
      MERROR_VER("Transaction does not exist: " << e.what());
      return false;
    }

  }

  return true;
}
//------------------------------------------------------------------
uint64_t Blockchain::get_current_blockchain_height() const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  // WARNING: this function does not take m_blockchain_lock, and thus should only call read only
  // m_db functions which do not depend on one another (ie, no getheight + gethash(height-1), as
  // well as not accessing class members, even read only (ie, m_invalid_blocks). The caller must
  // lock if it is otherwise needed.
  return m_db->height();
}
//------------------------------------------------------------------
//FIXME: possibly move this into the constructor, to avoid accidentally
//       dereferencing a null BlockchainDB pointer
bool Blockchain::init(BlockchainDB* db, const network_type nettype, bool offline, const cryptonote::test_options *test_options, difficulty_type fixed_difficulty, const GetCheckpointsCallback& get_checkpoints/* = nullptr*/)
{
  LOG_PRINT_L3("Blockchain::" << __func__);

  CHECK_AND_ASSERT_MES(nettype != FAKECHAIN || test_options, false, "fake chain network type used without options");

  CRITICAL_REGION_LOCAL(m_tx_pool);
  CRITICAL_REGION_LOCAL1(m_blockchain_lock);

  if (db == nullptr)
  {
    LOG_ERROR("Attempted to init Blockchain with null DB");
    return false;
  }
  if (!db->is_open())
  {
    LOG_ERROR("Attempted to init Blockchain with unopened DB");
    delete db;
    return false;
  }

  m_db = db;

  m_nettype = test_options != NULL ? FAKECHAIN : nettype;
  m_offline = offline;
  m_fixed_difficulty = fixed_difficulty;
  if (m_hardfork == nullptr)
  {
    if (m_nettype ==  FAKECHAIN || m_nettype == STAGENET)
      m_hardfork = new HardFork(*db, 1, 0);
    else if (m_nettype == TESTNET)
      m_hardfork = new HardFork(*db, 1, testnet_hard_fork_version_1_till);
    else
      m_hardfork = new HardFork(*db, 1, mainnet_hard_fork_version_1_till);
  }
  if (m_nettype == FAKECHAIN)
  {
    for (size_t n = 0; test_options->hard_forks[n].first; ++n)
      m_hardfork->add_fork(test_options->hard_forks[n].first, test_options->hard_forks[n].second, 0, n + 1);
  }
  else if (m_nettype == TESTNET)
  {
    for (size_t n = 0; n < sizeof(testnet_hard_forks) / sizeof(testnet_hard_forks[0]); ++n)
      m_hardfork->add_fork(testnet_hard_forks[n].version, testnet_hard_forks[n].height, testnet_hard_forks[n].threshold, testnet_hard_forks[n].time);
  }
  else if (m_nettype == STAGENET)
  {
    for (size_t n = 0; n < sizeof(stagenet_hard_forks) / sizeof(stagenet_hard_forks[0]); ++n)
      m_hardfork->add_fork(stagenet_hard_forks[n].version, stagenet_hard_forks[n].height, stagenet_hard_forks[n].threshold, stagenet_hard_forks[n].time);
  }
  else
  {
    for (size_t n = 0; n < sizeof(mainnet_hard_forks) / sizeof(mainnet_hard_forks[0]); ++n)
      m_hardfork->add_fork(mainnet_hard_forks[n].version, mainnet_hard_forks[n].height, mainnet_hard_forks[n].threshold, mainnet_hard_forks[n].time);
  }
  m_hardfork->init();

  m_db->set_hard_fork(m_hardfork);

  // if the blockchain is new, add the genesis block
  // this feels kinda kludgy to do it this way, but can be looked at later.
  // TODO: add function to create and store genesis block,
  //       taking testnet into account
  if(!m_db->height())
  {
    MINFO("Blockchain not loaded, generating genesis block.");
    block bl;
    block_verification_context bvc = boost::value_initialized<block_verification_context>();
    generate_genesis_block(bl, get_config(m_nettype).GENESIS_TX, get_config(m_nettype).GENESIS_NONCE);
    add_new_block(bl, bvc);
    CHECK_AND_ASSERT_MES(!bvc.m_verifivation_failed, false, "Failed to add genesis block to blockchain");
  }
  // TODO: if blockchain load successful, verify blockchain against both
  //       hard-coded and runtime-loaded (and enforced) checkpoints.
  else
  {
  }

  if (m_nettype != FAKECHAIN)
  {
    // ensure we fixup anything we found and fix in the future
    m_db->fixup();
  }

  m_db->block_txn_start(true);
  // check how far behind we are
  uint64_t top_block_timestamp = m_db->get_top_block_timestamp();
  uint64_t timestamp_diff = time(NULL) - top_block_timestamp;

  // genesis block has no timestamp, could probably change it to have timestamp of 1341378000...
  if(!top_block_timestamp)
    timestamp_diff = time(NULL) - 1341378000;

  // create general purpose async service queue

  m_async_work_idle = std::unique_ptr < boost::asio::io_service::work > (new boost::asio::io_service::work(m_async_service));
  // we only need 1
  m_async_pool.create_thread(boost::bind(&boost::asio::io_service::run, &m_async_service));

#if defined(PER_BLOCK_CHECKPOINT)
  if (m_nettype != FAKECHAIN)
    load_compiled_in_block_hashes(get_checkpoints);
#endif

  MINFO("Blockchain initialized. last block: " << m_db->height() - 1 << ", " << epee::misc_utils::get_time_interval_string(timestamp_diff) << " time ago, current difficulty: " << get_difficulty_for_next_block());
  m_db->block_txn_stop();

  uint64_t num_popped_blocks = 0;
  while (!m_db->is_read_only())
  {
    const uint64_t top_height = m_db->height() - 1;
    const crypto::hash top_id = m_db->top_block_hash();
    const block top_block = m_db->get_top_block();
    const uint8_t ideal_hf_version = get_ideal_hard_fork_version(top_height);
    if (ideal_hf_version <= 1 || ideal_hf_version == top_block.major_version)
    {
      if (num_popped_blocks > 0)
        MGINFO("Initial popping done, top block: " << top_id << ", top height: " << top_height << ", block version: " << (uint64_t)top_block.major_version);
      break;
    }
    else
    {
      if (num_popped_blocks == 0)
        MGINFO("Current top block " << top_id << " at height " << top_height << " has version " << (uint64_t)top_block.major_version << " which disagrees with the ideal version " << (uint64_t)ideal_hf_version);
      if (num_popped_blocks % 100 == 0)
        MGINFO("Popping blocks... " << top_height);
      ++num_popped_blocks;
      block popped_block;
      std::vector<transaction> popped_txs;
      try
      {
        m_db->pop_block(popped_block, popped_txs);
      }
      // anything that could cause this to throw is likely catastrophic,
      // so we re-throw
      catch (const std::exception& e)
      {
        MERROR("Error popping block from blockchain: " << e.what());
        throw;
      }
      catch (...)
      {
        MERROR("Error popping block from blockchain, throwing!");
        throw;
      }
    }
  }
  if (num_popped_blocks > 0)
  {
    m_timestamps_and_difficulties_height = 0;
    m_hardfork->reorganize_from_chain_height(get_current_blockchain_height());
    m_tx_pool.on_blockchain_dec(m_db->height()-1, get_tail_id());
  }

  update_next_cumulative_weight_limit();
  return true;
}
//------------------------------------------------------------------
bool Blockchain::init(BlockchainDB* db, HardFork*& hf, const network_type nettype, bool offline)
{
  if (hf != nullptr)
    m_hardfork = hf;
  bool res = init(db, nettype, offline, NULL);
  if (hf == nullptr)
    hf = m_hardfork;
  return res;
}
//------------------------------------------------------------------
bool Blockchain::store_blockchain()
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  // lock because the rpc_thread command handler also calls this
  CRITICAL_REGION_LOCAL(m_db->m_synchronization_lock);

  TIME_MEASURE_START(save);
  // TODO: make sure sync(if this throws that it is not simply ignored higher
  // up the call stack
  try
  {
    m_db->sync();
  }
  catch (const std::exception& e)
  {
    MERROR(std::string("Error syncing blockchain db: ") + e.what() + "-- shutting down now to prevent issues!");
    throw;
  }
  catch (...)
  {
    MERROR("There was an issue storing the blockchain, shutting down now to prevent issues!");
    throw;
  }

  TIME_MEASURE_FINISH(save);
  if(m_show_time_stats)
    MINFO("Blockchain stored OK, took: " << save << " ms");
  return true;
}
//------------------------------------------------------------------
bool Blockchain::deinit()
{
  LOG_PRINT_L3("Blockchain::" << __func__);

  MTRACE("Stopping blockchain read/write activity");

 // stop async service
  m_async_work_idle.reset();
  m_async_pool.join_all();
  m_async_service.stop();

  // as this should be called if handling a SIGSEGV, need to check
  // if m_db is a NULL pointer (and thus may have caused the illegal
  // memory operation), otherwise we may cause a loop.
  try
  {
    if (m_db)
    {
      m_db->close();
      MTRACE("Local blockchain read/write activity stopped successfully");
    }
  }
  catch (const std::exception& e)
  {
    LOG_ERROR(std::string("Error closing blockchain db: ") + e.what());
  }
  catch (...)
  {
    LOG_ERROR("There was an issue closing/storing the blockchain, shutting down now to prevent issues!");
  }

  delete m_hardfork;
  m_hardfork = NULL;
  delete m_db;
  m_db = NULL;
  return true;
}
//------------------------------------------------------------------
// This function removes blocks from the top of blockchain.
// It starts a batch and calls private method pop_block_from_blockchain().
void Blockchain::pop_blocks(uint64_t nblocks)
{
  uint64_t i;
  CRITICAL_REGION_LOCAL(m_tx_pool);
  CRITICAL_REGION_LOCAL1(m_blockchain_lock);

  while (!m_db->batch_start())
  {
    m_blockchain_lock.unlock();
    m_tx_pool.unlock();
    epee::misc_utils::sleep_no_w(1000);
    m_tx_pool.lock();
    m_blockchain_lock.lock();
  }

  try
  {
    for (i=0; i < nblocks; ++i)
    {
      pop_block_from_blockchain();
    }
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("Error when popping blocks, only " << i << " blocks are popped: " << e.what());
  }

  m_db->batch_stop();
}
//------------------------------------------------------------------
// This function tells BlockchainDB to remove the top block from the
// blockchain and then returns all transactions (except the miner tx, of course)
// from it to the tx_pool
block Blockchain::pop_block_from_blockchain()
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  m_timestamps_and_difficulties_height = 0;

  block popped_block;
  std::vector<transaction> popped_txs;

  try
  {
    m_db->pop_block(popped_block, popped_txs);
  }
  // anything that could cause this to throw is likely catastrophic,
  // so we re-throw
  catch (const std::exception& e)
  {
    LOG_ERROR("Error popping block from blockchain: " << e.what());
    throw;
  }
  catch (...)
  {
    LOG_ERROR("Error popping block from blockchain, throwing!");
    throw;
  }

  // make sure the hard fork object updates its current version
  m_hardfork->on_block_popped(1);

  // return transactions from popped block to the tx_pool
  size_t pruned = 0;
  for (transaction& tx : popped_txs)
  {
    if (tx.pruned)
    {
      ++pruned;
      continue;
    }
    if (!is_coinbase(tx))
    {
      cryptonote::tx_verification_context tvc = AUTO_VAL_INIT(tvc);

      // FIXME: HardFork
      // Besides the below, popping a block should also remove the last entry
      // in hf_versions.
      uint8_t version = get_ideal_hard_fork_version(m_db->height());

      // We assume that if they were in a block, the transactions are already
      // known to the network as a whole. However, if we had mined that block,
      // that might not be always true. Unlikely though, and always relaying
      // these again might cause a spike of traffic as many nodes re-relay
      // all the transactions in a popped block when a reorg happens.
      bool r = m_tx_pool.add_tx(tx, tvc, true, true, false, version);
      if (!r)
      {
        LOG_ERROR("Error returning transaction to tx_pool");
      }
    }
  }
  if (pruned)
    MWARNING(pruned << " pruned txes could not be added back to the txpool");

  m_blocks_longhash_table.clear();
  m_scan_table.clear();
  m_blocks_txs_check.clear();
  m_check_txin_table.clear();

  update_next_cumulative_weight_limit();
  m_tx_pool.on_blockchain_dec(m_db->height()-1, get_tail_id());
  invalidate_block_template_cache();

  return popped_block;
}
//------------------------------------------------------------------
bool Blockchain::reset_and_set_genesis_block(const block& b)
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  m_timestamps_and_difficulties_height = 0;
  m_alternative_chains.clear();
  invalidate_block_template_cache();
  m_db->reset();
  m_hardfork->init();

  block_verification_context bvc = boost::value_initialized<block_verification_context>();
  add_new_block(b, bvc);
  update_next_cumulative_weight_limit();
  return bvc.m_added_to_main_chain && !bvc.m_verifivation_failed;
}
//------------------------------------------------------------------
crypto::hash Blockchain::get_tail_id(uint64_t& height) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  height = m_db->height() - 1;
  return get_tail_id();
}
//------------------------------------------------------------------
crypto::hash Blockchain::get_tail_id() const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  // WARNING: this function does not take m_blockchain_lock, and thus should only call read only
  // m_db functions which do not depend on one another (ie, no getheight + gethash(height-1), as
  // well as not accessing class members, even read only (ie, m_invalid_blocks). The caller must
  // lock if it is otherwise needed.
  return m_db->top_block_hash();
}
//------------------------------------------------------------------
/*TODO: this function was...poorly written.  As such, I'm not entirely
 *      certain on what it was supposed to be doing.  Need to look into this,
 *      but it doesn't seem terribly important just yet.
 *
 * puts into list <ids> a list of hashes representing certain blocks
 * from the blockchain in reverse chronological order
 *
 * the blocks chosen, at the time of this writing, are:
 *   the most recent 11
 *   powers of 2 less recent from there, so 13, 17, 25, etc...
 *
 */
bool Blockchain::get_short_chain_history(std::list<crypto::hash>& ids) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  uint64_t i = 0;
  uint64_t current_multiplier = 1;
  uint64_t sz = m_db->height();

  if(!sz)
    return true;

  m_db->block_txn_start(true);
  bool genesis_included = false;
  uint64_t current_back_offset = 1;
  while(current_back_offset < sz)
  {
    ids.push_back(m_db->get_block_hash_from_height(sz - current_back_offset));

    if(sz-current_back_offset == 0)
    {
      genesis_included = true;
    }
    if(i < 10)
    {
      ++current_back_offset;
    }
    else
    {
      current_multiplier *= 2;
      current_back_offset += current_multiplier;
    }
    ++i;
  }

  if (!genesis_included)
  {
    ids.push_back(m_db->get_block_hash_from_height(0));
  }
  m_db->block_txn_stop();

  return true;
}
//------------------------------------------------------------------
crypto::hash Blockchain::get_block_id_by_height(uint64_t height) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  // WARNING: this function does not take m_blockchain_lock, and thus should only call read only
  // m_db functions which do not depend on one another (ie, no getheight + gethash(height-1), as
  // well as not accessing class members, even read only (ie, m_invalid_blocks). The caller must
  // lock if it is otherwise needed.
  try
  {
    return m_db->get_block_hash_from_height(height);
  }
  catch (const BLOCK_DNE& e)
  {
  }
  catch (const std::exception& e)
  {
    MERROR(std::string("Something went wrong fetching block hash by height: ") + e.what());
    throw;
  }
  catch (...)
  {
    MERROR(std::string("Something went wrong fetching block hash by height"));
    throw;
  }
  return null_hash;
}
//------------------------------------------------------------------
bool Blockchain::get_block_by_hash(const crypto::hash &h, block &blk, bool *orphan) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  // try to find block in main chain
  try
  {
    blk = m_db->get_block(h);
    if (orphan)
      *orphan = false;
    return true;
  }
  // try to find block in alternative chain
  catch (const BLOCK_DNE& e)
  {
    blocks_ext_by_hash::const_iterator it_alt = m_alternative_chains.find(h);
    if (m_alternative_chains.end() != it_alt)
    {
      blk = it_alt->second.bl;
      if (orphan)
        *orphan = true;
      return true;
    }
  }
  catch (const std::exception& e)
  {
    MERROR(std::string("Something went wrong fetching block by hash: ") + e.what());
    throw;
  }
  catch (...)
  {
    MERROR(std::string("Something went wrong fetching block hash by hash"));
    throw;
  }

  return false;
}
//------------------------------------------------------------------
// This function aggregates the cumulative difficulties and timestamps of the
// last DIFFICULTY_BLOCKS_COUNT blocks and passes them to next_difficulty,
// returning the result of that call.  Ignores the genesis block, and can use
// less blocks than desired if there aren't enough.
difficulty_type Blockchain::get_difficulty_for_next_block()
{
  if (m_fixed_difficulty)
  {
    return m_db->height() ? m_fixed_difficulty : 1;
  }

  LOG_PRINT_L3("Blockchain::" << __func__);

  crypto::hash top_hash = get_tail_id();
  {
    CRITICAL_REGION_LOCAL(m_difficulty_lock);
    // we can call this without the blockchain lock, it might just give us
    // something a bit out of date, but that's fine since anything which
    // requires the blockchain lock will have acquired it in the first place,
    // and it will be unlocked only when called from the getinfo RPC
    if (top_hash == m_difficulty_for_next_block_top_hash)
      return m_difficulty_for_next_block;
  }

  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  std::vector<uint64_t> timestamps;
  std::vector<difficulty_type> difficulties;
  auto height = m_db->height();
  top_hash = get_tail_id(); // get it again now that we have the lock
  // ND: Speedup
  // 1. Keep a list of the last 735 (or less) blocks that is used to compute difficulty,
  //    then when the next block difficulty is queried, push the latest height data and
  //    pop the oldest one from the list. This only requires 1x read per height instead
  //    of doing 735 (DIFFICULTY_BLOCKS_COUNT).
  if (m_timestamps_and_difficulties_height != 0 && ((height - m_timestamps_and_difficulties_height) == 1) && m_timestamps.size() >= DIFFICULTY_BLOCKS_COUNT)
  {
    uint64_t index = height - 1;
    m_timestamps.push_back(m_db->get_block_timestamp(index));
    m_difficulties.push_back(m_db->get_block_cumulative_difficulty(index));

    while (m_timestamps.size() > DIFFICULTY_BLOCKS_COUNT)
      m_timestamps.erase(m_timestamps.begin());
    while (m_difficulties.size() > DIFFICULTY_BLOCKS_COUNT)
      m_difficulties.erase(m_difficulties.begin());

    m_timestamps_and_difficulties_height = height;
    timestamps = m_timestamps;
    difficulties = m_difficulties;
  }
  else
  {
    uint64_t offset = height - std::min <uint64_t> (height, static_cast<uint64_t>(DIFFICULTY_BLOCKS_COUNT));
    if (offset == 0)
      ++offset;

    timestamps.clear();
    difficulties.clear();
    if (height > offset)
    {
      timestamps.reserve(height - offset);
      difficulties.reserve(height - offset);
    }
    for (; offset < height; offset++)
    {
      timestamps.push_back(m_db->get_block_timestamp(offset));
      difficulties.push_back(m_db->get_block_cumulative_difficulty(offset));
    }

    m_timestamps_and_difficulties_height = height;
    m_timestamps = timestamps;
    m_difficulties = difficulties;
  }
  size_t target = get_difficulty_target();
  difficulty_type diff = next_difficulty(timestamps, difficulties, target);

  CRITICAL_REGION_LOCAL1(m_difficulty_lock);
  m_difficulty_for_next_block_top_hash = top_hash;
  m_difficulty_for_next_block = diff;
  return diff;
}
//------------------------------------------------------------------
std::vector<time_t> Blockchain::get_last_block_timestamps(unsigned int blocks) const
{
  uint64_t height = m_db->height();
  if (blocks > height)
    blocks = height;
  std::vector<time_t> timestamps(blocks);
  while (blocks--)
    timestamps[blocks] = m_db->get_block_timestamp(height - blocks - 1);
  return timestamps;
}
//------------------------------------------------------------------
// This function removes blocks from the blockchain until it gets to the
// position where the blockchain switch started and then re-adds the blocks
// that had been removed.
bool Blockchain::rollback_blockchain_switching(std::list<block>& original_chain, uint64_t rollback_height)
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  // fail if rollback_height passed is too high
  if (rollback_height > m_db->height())
  {
    return true;
  }

  m_timestamps_and_difficulties_height = 0;

  // remove blocks from blockchain until we get back to where we should be.
  while (m_db->height() != rollback_height)
  {
    pop_block_from_blockchain();
  }

  // make sure the hard fork object updates its current version
  m_hardfork->reorganize_from_chain_height(rollback_height);

  //return back original chain
  for (auto& bl : original_chain)
  {
    block_verification_context bvc = boost::value_initialized<block_verification_context>();
    bool r = handle_block_to_main_chain(bl, bvc);
    CHECK_AND_ASSERT_MES(r && bvc.m_added_to_main_chain, false, "PANIC! failed to add (again) block while chain switching during the rollback!");
  }

  m_hardfork->reorganize_from_chain_height(rollback_height);

  MINFO("Rollback to height " << rollback_height << " was successful.");
  if (!original_chain.empty())
  {
    MINFO("Restoration to previous blockchain successful as well.");
  }
  return true;
}
//------------------------------------------------------------------
// This function attempts to switch to an alternate chain, returning
// boolean based on success therein.
bool Blockchain::switch_to_alternative_blockchain(std::list<blocks_ext_by_hash::iterator>& alt_chain, bool discard_disconnected_chain)
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  m_timestamps_and_difficulties_height = 0;

  // if empty alt chain passed (not sure how that could happen), return false
  CHECK_AND_ASSERT_MES(alt_chain.size(), false, "switch_to_alternative_blockchain: empty chain passed");

  // verify that main chain has front of alt chain's parent block
  if (!m_db->block_exists(alt_chain.front()->second.bl.prev_id))
  {
    LOG_ERROR("Attempting to move to an alternate chain, but it doesn't appear to connect to the main chain!");
    return false;
  }

  // pop blocks from the blockchain until the top block is the parent
  // of the front block of the alt chain.
  std::list<block> disconnected_chain;
  while (m_db->top_block_hash() != alt_chain.front()->second.bl.prev_id)
  {
    block b = pop_block_from_blockchain();
    disconnected_chain.push_front(b);
  }

  auto split_height = m_db->height();

  //connecting new alternative chain
  for(auto alt_ch_iter = alt_chain.begin(); alt_ch_iter != alt_chain.end(); alt_ch_iter++)
  {
    auto ch_ent = *alt_ch_iter;
    block_verification_context bvc = boost::value_initialized<block_verification_context>();

    // add block to main chain
    bool r = handle_block_to_main_chain(ch_ent->second.bl, bvc);

    // if adding block to main chain failed, rollback to previous state and
    // return false
    if(!r || !bvc.m_added_to_main_chain)
    {
      MERROR("Failed to switch to alternative blockchain");

      // rollback_blockchain_switching should be moved to two different
      // functions: rollback and apply_chain, but for now we pretend it is
      // just the latter (because the rollback was done above).
      rollback_blockchain_switching(disconnected_chain, split_height);

      // FIXME: Why do we keep invalid blocks around?  Possibly in case we hear
      // about them again so we can immediately dismiss them, but needs some
      // looking into.
      add_block_as_invalid(ch_ent->second, get_block_hash(ch_ent->second.bl));
      MERROR("The block was inserted as invalid while connecting new alternative chain, block_id: " << get_block_hash(ch_ent->second.bl));
      m_alternative_chains.erase(*alt_ch_iter++);

      for(auto alt_ch_to_orph_iter = alt_ch_iter; alt_ch_to_orph_iter != alt_chain.end(); )
      {
        add_block_as_invalid((*alt_ch_to_orph_iter)->second, (*alt_ch_to_orph_iter)->first);
        m_alternative_chains.erase(*alt_ch_to_orph_iter++);
      }
      return false;
    }
  }

  // if we're to keep the disconnected blocks, add them as alternates
  if(!discard_disconnected_chain)
  {
    //pushing old chain as alternative chain
    for (auto& old_ch_ent : disconnected_chain)
    {
      block_verification_context bvc = boost::value_initialized<block_verification_context>();
      bool r = handle_alternative_block(old_ch_ent, get_block_hash(old_ch_ent), bvc);
      if(!r)
      {
        MERROR("Failed to push ex-main chain blocks to alternative chain ");
        // previously this would fail the blockchain switching, but I don't
        // think this is bad enough to warrant that.
      }
    }
  }

  //removing alt_chain entries from alternative chains container
  for (auto ch_ent: alt_chain)
  {
    m_alternative_chains.erase(ch_ent);
  }

  m_hardfork->reorganize_from_chain_height(split_height);

  std::shared_ptr<tools::Notify> reorg_notify = m_reorg_notify;
  if (reorg_notify)
    reorg_notify->notify("%s", std::to_string(split_height).c_str(), "%h", std::to_string(m_db->height()).c_str(),
        "%n", std::to_string(m_db->height() - split_height).c_str(), NULL);

  MGINFO_GREEN("REORGANIZE SUCCESS! on height: " << split_height << ", new blockchain size: " << m_db->height());
  return true;
}
//------------------------------------------------------------------
// This function calculates the difficulty target for the block being added to
// an alternate chain.
difficulty_type Blockchain::get_next_difficulty_for_alternative_chain(const std::list<blocks_ext_by_hash::iterator>& alt_chain, block_extended_info& bei) const
{
  if (m_fixed_difficulty)
  {
    return m_db->height() ? m_fixed_difficulty : 1;
  }

  LOG_PRINT_L3("Blockchain::" << __func__);
  std::vector<uint64_t> timestamps;
  std::vector<difficulty_type> cumulative_difficulties;

  // if the alt chain isn't long enough to calculate the difficulty target
  // based on its blocks alone, need to get more blocks from the main chain
  if(alt_chain.size()< DIFFICULTY_BLOCKS_COUNT)
  {
    CRITICAL_REGION_LOCAL(m_blockchain_lock);

    // Figure out start and stop offsets for main chain blocks
    size_t main_chain_stop_offset = alt_chain.size() ? alt_chain.front()->second.height : bei.height;
    size_t main_chain_count = DIFFICULTY_BLOCKS_COUNT - std::min(static_cast<size_t>(DIFFICULTY_BLOCKS_COUNT), alt_chain.size());
    main_chain_count = std::min(main_chain_count, main_chain_stop_offset);
    size_t main_chain_start_offset = main_chain_stop_offset - main_chain_count;

    if(!main_chain_start_offset)
      ++main_chain_start_offset; //skip genesis block

    // get difficulties and timestamps from relevant main chain blocks
    for(; main_chain_start_offset < main_chain_stop_offset; ++main_chain_start_offset)
    {
      timestamps.push_back(m_db->get_block_timestamp(main_chain_start_offset));
      cumulative_difficulties.push_back(m_db->get_block_cumulative_difficulty(main_chain_start_offset));
    }

    // make sure we haven't accidentally grabbed too many blocks...maybe don't need this check?
    CHECK_AND_ASSERT_MES((alt_chain.size() + timestamps.size()) <= DIFFICULTY_BLOCKS_COUNT, false, "Internal error, alt_chain.size()[" << alt_chain.size() << "] + vtimestampsec.size()[" << timestamps.size() << "] NOT <= DIFFICULTY_WINDOW[]" << DIFFICULTY_BLOCKS_COUNT);

    for (auto it : alt_chain)
    {
      timestamps.push_back(it->second.bl.timestamp);
      cumulative_difficulties.push_back(it->second.cumulative_difficulty);
    }
  }
  // if the alt chain is long enough for the difficulty calc, grab difficulties
  // and timestamps from it alone
  else
  {
    timestamps.resize(static_cast<size_t>(DIFFICULTY_BLOCKS_COUNT));
    cumulative_difficulties.resize(static_cast<size_t>(DIFFICULTY_BLOCKS_COUNT));
    size_t count = 0;
    size_t max_i = timestamps.size()-1;
    // get difficulties and timestamps from most recent blocks in alt chain
    for(auto it: boost::adaptors::reverse(alt_chain))
    {
      timestamps[max_i - count] = it->second.bl.timestamp;
      cumulative_difficulties[max_i - count] = it->second.cumulative_difficulty;
      count++;
      if(count >= DIFFICULTY_BLOCKS_COUNT)
        break;
    }
  }

  // FIXME: This will fail if fork activation heights are subject to voting
  size_t target = get_ideal_hard_fork_version(bei.height) < 2 ? DIFFICULTY_TARGET_V1 : DIFFICULTY_TARGET_V2;

  // calculate the difficulty target for the block and return it
  return next_difficulty(timestamps, cumulative_difficulties, target);
}
//------------------------------------------------------------------
// This function does a sanity check on basic things that all miner
// transactions have in common, such as:
//   one input, of type txin_gen, with height set to the block's height
//   correct miner tx unlock time
//   a non-overflowing tx amount (dubious necessity on this check)
bool Blockchain::prevalidate_miner_transaction(const block& b, uint64_t height)
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CHECK_AND_ASSERT_MES(b.miner_tx.vin.size() == 1, false, "coinbase transaction in the block has no inputs");
  CHECK_AND_ASSERT_MES(b.miner_tx.vin[0].type() == typeid(txin_gen), false, "coinbase transaction in the block has the wrong type");
  if(boost::get<txin_gen>(b.miner_tx.vin[0]).height != height)
  {
    MWARNING("The miner transaction in block has invalid height: " << boost::get<txin_gen>(b.miner_tx.vin[0]).height << ", expected: " << height);
    return false;
  }
  MDEBUG("Miner tx hash: " << get_transaction_hash(b.miner_tx));
  CHECK_AND_ASSERT_MES(b.miner_tx.unlock_time == height + CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW, false, "coinbase transaction transaction has the wrong unlock time=" << b.miner_tx.unlock_time << ", expected " << height + CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW);

  //check outs overflow
  //NOTE: not entirely sure this is necessary, given that this function is
  //      designed simply to make sure the total amount for a transaction
  //      does not overflow a uint64_t, and this transaction *is* a uint64_t...
  if(!check_outs_overflow(b.miner_tx))
  {
    MERROR("miner transaction has money overflow in block " << get_block_hash(b));
    return false;
  }

  return true;
}
//------------------------------------------------------------------
// This function validates the miner transaction reward
bool Blockchain::validate_miner_transaction(const block& b, size_t cumulative_block_weight, uint64_t fee, uint64_t& base_reward, uint64_t already_generated_coins, bool &partial_block_reward, uint8_t version)
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  //validate reward
  uint64_t money_in_use = 0;
  for (auto& o: b.miner_tx.vout)
    money_in_use += o.amount;
  partial_block_reward = false;

  if (version == 3) {
    for (auto &o: b.miner_tx.vout) {
      if (!is_valid_decomposed_amount(o.amount)) {
        MERROR_VER("miner tx output " << print_money(o.amount) << " is not a valid decomposed amount");
        return false;
      }
    }
  }

  std::vector<size_t> last_blocks_weights;
  get_last_n_blocks_weights(last_blocks_weights, CRYPTONOTE_REWARD_BLOCKS_WINDOW);
  if (!get_block_reward(epee::misc_utils::median(last_blocks_weights), cumulative_block_weight, already_generated_coins, base_reward, version))
  {
    MERROR_VER("block weight " << cumulative_block_weight << " is bigger than allowed for this blockchain");
    return false;
  }
  if(base_reward + fee < money_in_use)
  {
    MERROR_VER("coinbase transaction spend too much money (" << print_money(money_in_use) << "). Block reward is " << print_money(base_reward + fee) << "(" << print_money(base_reward) << "+" << print_money(fee) << ")");
    return false;
  }
  // From hard fork 2, we allow a miner to claim less block reward than is allowed, in case a miner wants less dust
  if (version < 2)
  {
    if(base_reward + fee != money_in_use)
    {
      MDEBUG("coinbase transaction doesn't use full amount of block reward:  spent: " << money_in_use << ",  block reward " << base_reward + fee << "(" << base_reward << "+" << fee << ")");
      return false;
    }
  }
  else
  {
    // from hard fork 2, since a miner can claim less than the full block reward, we update the base_reward
    // to show the amount of coins that were actually generated, the remainder will be pushed back for later
    // emission. This modifies the emission curve very slightly.
    CHECK_AND_ASSERT_MES(money_in_use - fee <= base_reward, false, "base reward calculation bug");
    if(base_reward + fee != money_in_use)
      partial_block_reward = true;
    base_reward = money_in_use - fee;
  }
  return true;
}
//------------------------------------------------------------------
// get the block weights of the last <count> blocks, and return by reference <sz>.
void Blockchain::get_last_n_blocks_weights(std::vector<size_t>& weights, size_t count) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  auto h = m_db->height();

  // this function is meaningless for an empty blockchain...granted it should never be empty
  if(h == 0)
    return;

  m_db->block_txn_start(true);
  // add weight of last <count> blocks to vector <weights> (or less, if blockchain size < count)
  size_t start_offset = h - std::min<size_t>(h, count);
  weights.reserve(weights.size() + h - start_offset);
  for(size_t i = start_offset; i < h; i++)
  {
    weights.push_back(m_db->get_block_weight(i));
  }
  m_db->block_txn_stop();
}
//------------------------------------------------------------------
uint64_t Blockchain::get_current_cumulative_block_weight_limit() const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  return m_current_block_cumul_weight_limit;
}
//------------------------------------------------------------------
uint64_t Blockchain::get_current_cumulative_block_weight_median() const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  return m_current_block_cumul_weight_median;
}
//------------------------------------------------------------------
//TODO: This function only needed minor modification to work with BlockchainDB,
//      and *works*.  As such, to reduce the number of things that might break
//      in moving to BlockchainDB, this function will remain otherwise
//      unchanged for the time being.
//
// This function makes a new block for a miner to mine the hash for
//
// FIXME: this codebase references #if defined(DEBUG_CREATE_BLOCK_TEMPLATE)
// in a lot of places.  That flag is not referenced in any of the code
// nor any of the makefiles, howeve.  Need to look into whether or not it's
// necessary at all.
bool Blockchain::create_block_template(block& b, const account_public_address& miner_address, difficulty_type& diffic, uint64_t& height, uint64_t& expected_reward, const blobdata& ex_nonce)
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  size_t median_weight;
  uint64_t already_generated_coins;
  uint64_t pool_cookie;

  m_tx_pool.lock();
  const auto unlock_guard = epee::misc_utils::create_scope_leave_handler([&]() { m_tx_pool.unlock(); });
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  height = m_db->height();
  if (m_btc_valid) {
    // The pool cookie is atomic. The lack of locking is OK, as if it changes
    // just as we compare it, we'll just use a slightly old template, but
    // this would be the case anyway if we'd lock, and the change happened
    // just after the block template was created
    if (!memcmp(&miner_address, &m_btc_address, sizeof(cryptonote::account_public_address)) && m_btc_nonce == ex_nonce && m_btc_pool_cookie == m_tx_pool.cookie()) {
      MDEBUG("Using cached template");
      m_btc.timestamp = time(NULL); // update timestamp unconditionally
      b = m_btc;
      diffic = m_btc_difficulty;
      expected_reward = m_btc_expected_reward;
      return true;
    }
    MDEBUG("Not using cached template: address " << (!memcmp(&miner_address, &m_btc_address, sizeof(cryptonote::account_public_address))) << ", nonce " << (m_btc_nonce == ex_nonce) << ", cookie " << (m_btc_pool_cookie == m_tx_pool.cookie()));
    invalidate_block_template_cache();
  }

  b.major_version = m_hardfork->get_current_version();
  b.minor_version = m_hardfork->get_ideal_version();
  b.prev_id = get_tail_id();
  b.timestamp = time(NULL);

  uint64_t median_ts;
  if (!check_block_timestamp(b, median_ts))
  {
    b.timestamp = median_ts;
  }

  diffic = get_difficulty_for_next_block();
  CHECK_AND_ASSERT_MES(diffic, false, "difficulty overhead.");

  median_weight = m_current_block_cumul_weight_limit / 2;
  already_generated_coins = m_db->get_block_already_generated_coins(height - 1);

  size_t txs_weight;
  uint64_t fee;
  if (!m_tx_pool.fill_block_template(b, median_weight, already_generated_coins, txs_weight, fee, expected_reward, m_hardfork->get_current_version()))
  {
    return false;
  }
  pool_cookie = m_tx_pool.cookie();
#if defined(DEBUG_CREATE_BLOCK_TEMPLATE)
  size_t real_txs_weight = 0;
  uint64_t real_fee = 0;
  for(crypto::hash &cur_hash: b.tx_hashes)
  {
    auto cur_res = m_tx_pool.m_transactions.find(cur_hash);
    if (cur_res == m_tx_pool.m_transactions.end())
    {
      LOG_ERROR("Creating block template: error: transaction not found");
      continue;
    }
    tx_memory_pool::tx_details &cur_tx = cur_res->second;
    real_txs_weight += cur_tx.weight;
    real_fee += cur_tx.fee;
    if (cur_tx.weight != get_transaction_weight(cur_tx.tx))
    {
      LOG_ERROR("Creating block template: error: invalid transaction weight");
    }
    if (cur_tx.tx.version == 1)
    {
      uint64_t inputs_amount;
      if (!get_inputs_money_amount(cur_tx.tx, inputs_amount))
      {
        LOG_ERROR("Creating block template: error: cannot get inputs amount");
      }
      else if (cur_tx.fee != inputs_amount - get_outs_money_amount(cur_tx.tx))
      {
        LOG_ERROR("Creating block template: error: invalid fee");
      }
    }
    else
    {
      if (cur_tx.fee != cur_tx.tx.rct_signatures.txnFee)
      {
        LOG_ERROR("Creating block template: error: invalid fee");
      }
    }
  }
  if (txs_weight != real_txs_weight)
  {
    LOG_ERROR("Creating block template: error: wrongly calculated transaction weight");
  }
  if (fee != real_fee)
  {
    LOG_ERROR("Creating block template: error: wrongly calculated fee");
  }
  MDEBUG("Creating block template: height " << height <<
      ", median weight " << median_weight <<
      ", already generated coins " << already_generated_coins <<
      ", transaction weight " << txs_weight <<
      ", fee " << fee);
#endif

  /*
   two-phase miner transaction generation: we don't know exact block weight until we prepare block, but we don't know reward until we know
   block weight, so first miner transaction generated with fake amount of money, and with phase we know think we know expected block weight
   */
  //make blocks coin-base tx looks close to real coinbase tx to get truthful blob weight
  uint8_t hf_version = m_hardfork->get_current_version();
  size_t max_outs = hf_version >= 4 ? 1 : 11;
  bool r = construct_miner_tx(height, median_weight, already_generated_coins, txs_weight, fee, miner_address, b.miner_tx, ex_nonce, max_outs, hf_version);
  CHECK_AND_ASSERT_MES(r, false, "Failed to construct miner tx, first chance");
  size_t cumulative_weight = txs_weight + get_transaction_weight(b.miner_tx);
#if defined(DEBUG_CREATE_BLOCK_TEMPLATE)
  MDEBUG("Creating block template: miner tx weight " << get_transaction_weight(b.miner_tx) <<
      ", cumulative weight " << cumulative_weight);
#endif
  for (size_t try_count = 0; try_count != 10; ++try_count)
  {
    r = construct_miner_tx(height, median_weight, already_generated_coins, cumulative_weight, fee, miner_address, b.miner_tx, ex_nonce, max_outs, hf_version);

    CHECK_AND_ASSERT_MES(r, false, "Failed to construct miner tx, second chance");
    size_t coinbase_weight = get_transaction_weight(b.miner_tx);
    if (coinbase_weight > cumulative_weight - txs_weight)
    {
      cumulative_weight = txs_weight + coinbase_weight;
#if defined(DEBUG_CREATE_BLOCK_TEMPLATE)
      MDEBUG("Creating block template: miner tx weight " << coinbase_weight <<
          ", cumulative weight " << cumulative_weight << " is greater than before");
#endif
      continue;
    }

    if (coinbase_weight < cumulative_weight - txs_weight)
    {
      size_t delta = cumulative_weight - txs_weight - coinbase_weight;
#if defined(DEBUG_CREATE_BLOCK_TEMPLATE)
      MDEBUG("Creating block template: miner tx weight " << coinbase_weight <<
          ", cumulative weight " << txs_weight + coinbase_weight <<
          " is less than before, adding " << delta << " zero bytes");
#endif
      b.miner_tx.extra.insert(b.miner_tx.extra.end(), delta, 0);
      //here  could be 1 byte difference, because of extra field counter is varint, and it can become from 1-byte len to 2-bytes len.
      if (cumulative_weight != txs_weight + get_transaction_weight(b.miner_tx))
      {
        CHECK_AND_ASSERT_MES(cumulative_weight + 1 == txs_weight + get_transaction_weight(b.miner_tx), false, "unexpected case: cumulative_weight=" << cumulative_weight << " + 1 is not equal txs_cumulative_weight=" << txs_weight << " + get_transaction_weight(b.miner_tx)=" << get_transaction_weight(b.miner_tx));
        b.miner_tx.extra.resize(b.miner_tx.extra.size() - 1);
        if (cumulative_weight != txs_weight + get_transaction_weight(b.miner_tx))
        {
          //fuck, not lucky, -1 makes varint-counter size smaller, in that case we continue to grow with cumulative_weight
          MDEBUG("Miner tx creation has no luck with delta_extra size = " << delta << " and " << delta - 1);
          cumulative_weight += delta - 1;
          continue;
        }
        MDEBUG("Setting extra for block: " << b.miner_tx.extra.size() << ", try_count=" << try_count);
      }
    }
    CHECK_AND_ASSERT_MES(cumulative_weight == txs_weight + get_transaction_weight(b.miner_tx), false, "unexpected case: cumulative_weight=" << cumulative_weight << " is not equal txs_cumulative_weight=" << txs_weight << " + get_transaction_weight(b.miner_tx)=" << get_transaction_weight(b.miner_tx));
#if defined(DEBUG_CREATE_BLOCK_TEMPLATE)
    MDEBUG("Creating block template: miner tx weight " << coinbase_weight <<
        ", cumulative weight " << cumulative_weight << " is now good");
#endif

    cache_block_template(b, miner_address, ex_nonce, diffic, expected_reward, pool_cookie);
    return true;
  }
  LOG_ERROR("Failed to create_block_template with " << 10 << " tries");
  return false;
}
//------------------------------------------------------------------
// for an alternate chain, get the timestamps from the main chain to complete
// the needed number of timestamps for the BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW.
bool Blockchain::complete_timestamps_vector(uint64_t start_top_height, std::vector<uint64_t>& timestamps)
{
  LOG_PRINT_L3("Blockchain::" << __func__);

  if(timestamps.size() >= BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW)
    return true;

  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  size_t need_elements = BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW - timestamps.size();
  CHECK_AND_ASSERT_MES(start_top_height < m_db->height(), false, "internal error: passed start_height not < " << " m_db->height() -- " << start_top_height << " >= " << m_db->height());
  size_t stop_offset = start_top_height > need_elements ? start_top_height - need_elements : 0;
  timestamps.reserve(timestamps.size() + start_top_height - stop_offset);
  while (start_top_height != stop_offset)
  {
    timestamps.push_back(m_db->get_block_timestamp(start_top_height));
    --start_top_height;
  }
  return true;
}
//------------------------------------------------------------------
// If a block is to be added and its parent block is not the current
// main chain top block, then we need to see if we know about its parent block.
// If its parent block is part of a known forked chain, then we need to see
// if that chain is long enough to become the main chain and re-org accordingly
// if so.  If not, we need to hang on to the block in case it becomes part of
// a long forked chain eventually.
bool Blockchain::handle_alternative_block(const block& b, const crypto::hash& id, block_verification_context& bvc)
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  m_timestamps_and_difficulties_height = 0;
  uint64_t block_height = get_block_height(b);
  if(0 == block_height)
  {
    MERROR_VER("Block with id: " << epee::string_tools::pod_to_hex(id) << " (as alternative), but miner tx says height is 0.");
    bvc.m_verifivation_failed = true;
    return false;
  }
  // this basically says if the blockchain is smaller than the first
  // checkpoint then alternate blocks are allowed.  Alternatively, if the
  // last checkpoint *before* the end of the current chain is also before
  // the block to be added, then this is fine.
  if (!m_checkpoints.is_alternative_block_allowed(get_current_blockchain_height(), block_height))
  {
    MERROR_VER("Block with id: " << id << std::endl << " can't be accepted for alternative chain, block height: " << block_height << std::endl << " blockchain height: " << get_current_blockchain_height());
    bvc.m_verifivation_failed = true;
    return false;
  }

  // this is a cheap test
  if (!m_hardfork->check_for_height(b, block_height))
  {
    LOG_PRINT_L1("Block with id: " << id << std::endl << "has old version for height " << block_height);
    bvc.m_verifivation_failed = true;
    return false;
  }

  //block is not related with head of main chain
  //first of all - look in alternative chains container
  auto it_prev = m_alternative_chains.find(b.prev_id);
  bool parent_in_main = m_db->block_exists(b.prev_id);
  if(it_prev != m_alternative_chains.end() || parent_in_main)
  {
    //we have new block in alternative chain

    //build alternative subchain, front -> mainchain, back -> alternative head
    blocks_ext_by_hash::iterator alt_it = it_prev; //m_alternative_chains.find()
    std::list<blocks_ext_by_hash::iterator> alt_chain;
    std::vector<uint64_t> timestamps;
    while(alt_it != m_alternative_chains.end())
    {
      alt_chain.push_front(alt_it);
      timestamps.push_back(alt_it->second.bl.timestamp);
      alt_it = m_alternative_chains.find(alt_it->second.bl.prev_id);
    }

    // if block to be added connects to known blocks that aren't part of the
    // main chain -- that is, if we're adding on to an alternate chain
    if(!alt_chain.empty())
    {
      // make sure alt chain doesn't somehow start past the end of the main chain
      CHECK_AND_ASSERT_MES(m_db->height() > alt_chain.front()->second.height, false, "main blockchain wrong height");

      // make sure that the blockchain contains the block that should connect
      // this alternate chain with it.
      if (!m_db->block_exists(alt_chain.front()->second.bl.prev_id))
      {
        MERROR("alternate chain does not appear to connect to main chain...");
        return false;
      }

      // make sure block connects correctly to the main chain
      auto h = m_db->get_block_hash_from_height(alt_chain.front()->second.height - 1);
      CHECK_AND_ASSERT_MES(h == alt_chain.front()->second.bl.prev_id, false, "alternative chain has wrong connection to main chain");
      complete_timestamps_vector(m_db->get_block_height(alt_chain.front()->second.bl.prev_id), timestamps);
    }
    // if block not associated with known alternate chain
    else
    {
      // if block parent is not part of main chain or an alternate chain,
      // we ignore it
      CHECK_AND_ASSERT_MES(parent_in_main, false, "internal error: broken imperative condition: parent_in_main");

      complete_timestamps_vector(m_db->get_block_height(b.prev_id), timestamps);
    }

    // verify that the block's timestamp is within the acceptable range
    // (not earlier than the median of the last X blocks)
    if(!check_block_timestamp(timestamps, b))
    {
      MERROR_VER("Block with id: " << id << std::endl << " for alternative chain, has invalid timestamp: " << b.timestamp);
      bvc.m_verifivation_failed = true;
      return false;
    }

    // FIXME: consider moving away from block_extended_info at some point
    block_extended_info bei = boost::value_initialized<block_extended_info>();
    bei.bl = b;
    bei.height = alt_chain.size() ? it_prev->second.height + 1 : m_db->get_block_height(b.prev_id) + 1;

    bool is_a_checkpoint;
    if(!m_checkpoints.check_block(bei.height, id, is_a_checkpoint))
    {
      LOG_ERROR("CHECKPOINT VALIDATION FAILED");
      bvc.m_verifivation_failed = true;
      return false;
    }

    // Check the block's hash against the difficulty target for its alt chain
    difficulty_type current_diff = get_next_difficulty_for_alternative_chain(alt_chain, bei);
    CHECK_AND_ASSERT_MES(current_diff, false, "!!!!!!! DIFFICULTY OVERHEAD !!!!!!!");
    crypto::hash proof_of_work = null_hash;
    get_block_longhash(bei.bl, proof_of_work, bei.height);
    if(!check_hash(proof_of_work, current_diff))
    {
      MERROR_VER("Block with id: " << id << std::endl << " for alternative chain, does not have enough proof of work: " << proof_of_work << std::endl << " expected difficulty: " << current_diff);
      bvc.m_verifivation_failed = true;
      return false;
    }

    if(!prevalidate_miner_transaction(b, bei.height))
    {
      MERROR_VER("Block with id: " << epee::string_tools::pod_to_hex(id) << " (as alternative) has incorrect miner transaction.");
      bvc.m_verifivation_failed = true;
      return false;
    }

    // FIXME:
    // this brings up an interesting point: consider allowing to get block
    // difficulty both by height OR by hash, not just height.
    difficulty_type main_chain_cumulative_difficulty = m_db->get_block_cumulative_difficulty(m_db->height() - 1);
    if (alt_chain.size())
    {
      bei.cumulative_difficulty = it_prev->second.cumulative_difficulty;
    }
    else
    {
      // passed-in block's previous block's cumulative difficulty, found on the main chain
      bei.cumulative_difficulty = m_db->get_block_cumulative_difficulty(m_db->get_block_height(b.prev_id));
    }
    bei.cumulative_difficulty += current_diff;

    // add block to alternate blocks storage,
    // as well as the current "alt chain" container
    auto i_res = m_alternative_chains.insert(blocks_ext_by_hash::value_type(id, bei));
    CHECK_AND_ASSERT_MES(i_res.second, false, "insertion of new alternative block returned as it already exist");
    alt_chain.push_back(i_res.first);

    // FIXME: is it even possible for a checkpoint to show up not on the main chain?
    if(is_a_checkpoint)
    {
      //do reorganize!
      MGINFO_GREEN("###### REORGANIZE on height: " << alt_chain.front()->second.height << " of " << m_db->height() - 1 << ", checkpoint is found in alternative chain on height " << bei.height);

      bool r = switch_to_alternative_blockchain(alt_chain, true);

      if(r) bvc.m_added_to_main_chain = true;
      else bvc.m_verifivation_failed = true;

      return r;
    }
    else if(main_chain_cumulative_difficulty < bei.cumulative_difficulty) //check if difficulty bigger then in main chain
    {
      //do reorganize!
      MGINFO_GREEN("###### REORGANIZE on height: " << alt_chain.front()->second.height << " of " << m_db->height() - 1 << " with cum_difficulty " << m_db->get_block_cumulative_difficulty(m_db->height() - 1) << std::endl << " alternative blockchain size: " << alt_chain.size() << " with cum_difficulty " << bei.cumulative_difficulty);

      bool r = switch_to_alternative_blockchain(alt_chain, false);
      if (r)
        bvc.m_added_to_main_chain = true;
      else
        bvc.m_verifivation_failed = true;
      return r;
    }
    else
    {
      MGINFO_BLUE("----- BLOCK ADDED AS ALTERNATIVE ON HEIGHT " << bei.height << std::endl << "id:\t" << id << std::endl << "PoW:\t" << proof_of_work << std::endl << "difficulty:\t" << current_diff);
      return true;
    }
  }
  else
  {
    //block orphaned
    bvc.m_marked_as_orphaned = true;
    MERROR_VER("Block recognized as orphaned and rejected, id = " << id << ", height " << block_height
        << ", parent in alt " << (it_prev != m_alternative_chains.end()) << ", parent in main " << parent_in_main
        << " (parent " << b.prev_id << ", current top " << get_tail_id() << ", chain height " << get_current_blockchain_height() << ")");
  }

  return true;
}
//------------------------------------------------------------------
bool Blockchain::get_blocks(uint64_t start_offset, size_t count, std::vector<std::pair<cryptonote::blobdata,block>>& blocks, std::vector<cryptonote::blobdata>& txs) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  if(start_offset >= m_db->height())
    return false;

  if (!get_blocks(start_offset, count, blocks))
  {
    return false;
  }

  for(const auto& blk : blocks)
  {
    std::vector<crypto::hash> missed_ids;
    get_transactions_blobs(blk.second.tx_hashes, txs, missed_ids);
    CHECK_AND_ASSERT_MES(!missed_ids.size(), false, "has missed transactions in own block in main blockchain");
  }

  return true;
}
//------------------------------------------------------------------
bool Blockchain::get_blocks(uint64_t start_offset, size_t count, std::vector<std::pair<cryptonote::blobdata,block>>& blocks) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  const uint64_t height = m_db->height();
  if(start_offset >= height)
    return false;

  blocks.reserve(blocks.size() + height - start_offset);
  for(size_t i = start_offset; i < start_offset + count && i < height;i++)
  {
    blocks.push_back(std::make_pair(m_db->get_block_blob_from_height(i), block()));
    if (!parse_and_validate_block_from_blob(blocks.back().first, blocks.back().second))
    {
      LOG_ERROR("Invalid block");
      return false;
    }
  }
  return true;
}
//------------------------------------------------------------------
//TODO: This function *looks* like it won't need to be rewritten
//      to use BlockchainDB, as it calls other functions that were,
//      but it warrants some looking into later.
//
//FIXME: This function appears to want to return false if any transactions
//       that belong with blocks are missing, but not if blocks themselves
//       are missing.
bool Blockchain::handle_get_objects(NOTIFY_REQUEST_GET_OBJECTS::request& arg, NOTIFY_RESPONSE_GET_OBJECTS::request& rsp)
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  m_db->block_txn_start(true);
  rsp.current_blockchain_height = get_current_blockchain_height();
  std::vector<std::pair<cryptonote::blobdata,block>> blocks;
  get_blocks(arg.blocks, blocks, rsp.missed_ids);

  for (auto& bl: blocks)
  {
    std::vector<crypto::hash> missed_tx_ids;
    std::vector<cryptonote::blobdata> txs;

    rsp.blocks.push_back(block_complete_entry());
    block_complete_entry& e = rsp.blocks.back();

    // FIXME: s/rsp.missed_ids/missed_tx_id/ ?  Seems like rsp.missed_ids
    //        is for missed blocks, not missed transactions as well.
    get_transactions_blobs(bl.second.tx_hashes, e.txs, missed_tx_ids);

    if (missed_tx_ids.size() != 0)
    {
      LOG_ERROR("Error retrieving blocks, missed " << missed_tx_ids.size()
          << " transactions for block with hash: " << get_block_hash(bl.second)
          << std::endl
      );

      // append missed transaction hashes to response missed_ids field,
      // as done below if any standalone transactions were requested
      // and missed.
      rsp.missed_ids.insert(rsp.missed_ids.end(), missed_tx_ids.begin(), missed_tx_ids.end());
      m_db->block_txn_stop();
      return false;
    }

    //pack block
    e.block = std::move(bl.first);
  }
  //get and pack other transactions, if needed
  std::vector<cryptonote::blobdata> txs;
  get_transactions_blobs(arg.txs, rsp.txs, rsp.missed_ids);

  m_db->block_txn_stop();
  return true;
}
//------------------------------------------------------------------
bool Blockchain::get_alternative_blocks(std::vector<block>& blocks) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  blocks.reserve(m_alternative_chains.size());
  for (const auto& alt_bl: m_alternative_chains)
  {
    blocks.push_back(alt_bl.second.bl);
  }
  return true;
}
//------------------------------------------------------------------
size_t Blockchain::get_alternative_blocks_count() const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  return m_alternative_chains.size();
}
//------------------------------------------------------------------
// This function adds the output specified by <amount, i> to the result_outs container
// unlocked and other such checks should be done by here.
uint64_t Blockchain::get_num_mature_outputs(uint64_t amount) const
{
  uint64_t num_outs = m_db->get_num_outputs(amount);
  // ensure we don't include outputs that aren't yet eligible to be used
  // outpouts are sorted by height
  while (num_outs > 0)
  {
    const tx_out_index toi = m_db->get_output_tx_and_index(amount, num_outs - 1);
    const uint64_t height = m_db->get_tx_block_height(toi.first);
    if (height + CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE <= m_db->height())
      break;
    --num_outs;
  }

  return num_outs;
}

crypto::public_key Blockchain::get_output_key(uint64_t amount, uint64_t global_index) const
{
  output_data_t data = m_db->get_output_key(amount, global_index);
  return data.pubkey;
}

//------------------------------------------------------------------
bool Blockchain::get_outs(const COMMAND_RPC_GET_OUTPUTS_BIN::request& req, COMMAND_RPC_GET_OUTPUTS_BIN::response& res) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  res.outs.clear();
  res.outs.reserve(req.outputs.size());

  std::vector<cryptonote::output_data_t> data;
  try
  {
    std::vector<uint64_t> amounts, offsets;
    amounts.reserve(req.outputs.size());
    offsets.reserve(req.outputs.size());
    for (const auto &i: req.outputs)
    {
      amounts.push_back(i.amount);
      offsets.push_back(i.index);
    }
    m_db->get_output_key(epee::span<const uint64_t>(amounts.data(), amounts.size()), offsets, data);
    if (data.size() != req.outputs.size())
    {
      MERROR("Unexpected output data size: expected " << req.outputs.size() << ", got " << data.size());
      return false;
    }
    for (const auto &t: data)
      res.outs.push_back({t.pubkey, t.commitment, is_tx_spendtime_unlocked(t.unlock_time), t.height, crypto::null_hash});

    if (req.get_txid)
    {
      for (size_t i = 0; i < req.outputs.size(); ++i)
      {
        tx_out_index toi = m_db->get_output_tx_and_index(req.outputs[i].amount, req.outputs[i].index);
        res.outs[i].txid = toi.first;
      }
    }
  }
  catch (const std::exception &e)
  {
    return false;
  }
  return true;
}
//------------------------------------------------------------------
void Blockchain::get_output_key_mask_unlocked(const uint64_t& amount, const uint64_t& index, crypto::public_key& key, rct::key& mask, bool& unlocked) const
{
  const auto o_data = m_db->get_output_key(amount, index);
  key = o_data.pubkey;
  mask = o_data.commitment;
  tx_out_index toi = m_db->get_output_tx_and_index(amount, index);
  unlocked = is_tx_spendtime_unlocked(m_db->get_tx_unlock_time(toi.first));
}
//------------------------------------------------------------------
bool Blockchain::get_output_distribution(uint64_t amount, uint64_t from_height, uint64_t to_height, uint64_t &start_height, std::vector<uint64_t> &distribution, uint64_t &base) const
{
  // rct outputs don't exist before v4
  if (amount == 0)
  {
    switch (m_nettype)
    {
      case STAGENET: start_height = stagenet_hard_forks[3].height; break;
      case TESTNET: start_height = testnet_hard_forks[3].height; break;
      case MAINNET: start_height = mainnet_hard_forks[3].height; break;
      case FAKECHAIN: start_height = 0; break;
      default: return false;
    }
  }
  else
    start_height = 0;
  base = 0;

  if (to_height > 0 && to_height < from_height)
    return false;

  const uint64_t real_start_height = start_height;
  if (from_height > start_height)
    start_height = from_height;

  distribution.clear();
  uint64_t db_height = m_db->height();
  if (db_height == 0)
    return false;
  if (start_height >= db_height || to_height >= db_height)
    return false;
  if (amount == 0)
  {
    std::vector<uint64_t> heights;
    heights.reserve(to_height + 1 - start_height);
    uint64_t real_start_height = start_height > 0 ? start_height-1 : start_height;
    for (uint64_t h = real_start_height; h <= to_height; ++h)
      heights.push_back(h);
    distribution = m_db->get_block_cumulative_rct_outputs(heights);
    if (start_height > 0)
    {
      base = distribution[0];
      distribution.erase(distribution.begin());
    }
    return true;
  }
  else
  {
    return m_db->get_output_distribution(amount, start_height, to_height, distribution, base);
  }
}
//------------------------------------------------------------------
// This function takes a list of block hashes from another node
// on the network to find where the split point is between us and them.
// This is used to see what to send another node that needs to sync.
bool Blockchain::find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, uint64_t& starter_offset) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  // make sure the request includes at least the genesis block, otherwise
  // how can we expect to sync from the client that the block list came from?
  if(qblock_ids.empty())
  {
    MCERROR("net.p2p", "Client sent wrong NOTIFY_REQUEST_CHAIN: m_block_ids.size()=" << qblock_ids.size() << ", dropping connection");
    return false;
  }

  m_db->block_txn_start(true);
  // make sure that the last block in the request's block list matches
  // the genesis block
  auto gen_hash = m_db->get_block_hash_from_height(0);
  if(qblock_ids.back() != gen_hash)
  {
    MCERROR("net.p2p", "Client sent wrong NOTIFY_REQUEST_CHAIN: genesis block mismatch: " << std::endl << "id: " << qblock_ids.back() << ", " << std::endl << "expected: " << gen_hash << "," << std::endl << " dropping connection");
	m_db->block_txn_abort();
    return false;
  }

  // Find the first block the foreign chain has that we also have.
  // Assume qblock_ids is in reverse-chronological order.
  auto bl_it = qblock_ids.begin();
  uint64_t split_height = 0;
  for(; bl_it != qblock_ids.end(); bl_it++)
  {
    try
    {
      if (m_db->block_exists(*bl_it, &split_height))
        break;
    }
    catch (const std::exception& e)
    {
      MWARNING("Non-critical error trying to find block by hash in BlockchainDB, hash: " << *bl_it);
	  m_db->block_txn_abort();
      return false;
    }
  }
  m_db->block_txn_stop();

  // this should be impossible, as we checked that we share the genesis block,
  // but just in case...
  if(bl_it == qblock_ids.end())
  {
    MERROR("Internal error handling connection, can't find split point");
    return false;
  }

  //we start to put block ids INCLUDING last known id, just to make other side be sure
  starter_offset = split_height;
  return true;
}
//------------------------------------------------------------------
uint64_t Blockchain::block_difficulty(uint64_t i) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  // WARNING: this function does not take m_blockchain_lock, and thus should only call read only
  // m_db functions which do not depend on one another (ie, no getheight + gethash(height-1), as
  // well as not accessing class members, even read only (ie, m_invalid_blocks). The caller must
  // lock if it is otherwise needed.
  try
  {
    return m_db->get_block_difficulty(i);
  }
  catch (const BLOCK_DNE& e)
  {
    MERROR("Attempted to get block difficulty for height above blockchain height");
  }
  return 0;
}
//------------------------------------------------------------------
template<typename T> void reserve_container(std::vector<T> &v, size_t N) { v.reserve(N); }
template<typename T> void reserve_container(std::list<T> &v, size_t N) { }
//------------------------------------------------------------------
//TODO: return type should be void, throw on exception
//       alternatively, return true only if no blocks missed
template<class t_ids_container, class t_blocks_container, class t_missed_container>
bool Blockchain::get_blocks(const t_ids_container& block_ids, t_blocks_container& blocks, t_missed_container& missed_bs) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  reserve_container(blocks, block_ids.size());
  for (const auto& block_hash : block_ids)
  {
    try
    {
      uint64_t height = 0;
      if (m_db->block_exists(block_hash, &height))
      {
        blocks.push_back(std::make_pair(m_db->get_block_blob_from_height(height), block()));
        if (!parse_and_validate_block_from_blob(blocks.back().first, blocks.back().second))
        {
          LOG_ERROR("Invalid block: " << block_hash);
          blocks.pop_back();
          missed_bs.push_back(block_hash);
        }
      }
      else
        missed_bs.push_back(block_hash);
    }
    catch (const std::exception& e)
    {
      return false;
    }
  }
  return true;
}
//------------------------------------------------------------------
//TODO: return type should be void, throw on exception
//       alternatively, return true only if no transactions missed
template<class t_ids_container, class t_tx_container, class t_missed_container>
bool Blockchain::get_transactions_blobs(const t_ids_container& txs_ids, t_tx_container& txs, t_missed_container& missed_txs, bool pruned) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  reserve_container(txs, txs_ids.size());
  for (const auto& tx_hash : txs_ids)
  {
    try
    {
      cryptonote::blobdata tx;
      if (pruned && m_db->get_pruned_tx_blob(tx_hash, tx))
        txs.push_back(std::move(tx));
      else if (!pruned && m_db->get_tx_blob(tx_hash, tx))
        txs.push_back(std::move(tx));
      else
        missed_txs.push_back(tx_hash);
    }
    catch (const std::exception& e)
    {
      return false;
    }
  }
  return true;
}
//------------------------------------------------------------------
size_t get_transaction_version(const cryptonote::blobdata &bd)
{
  size_t version;
  const char* begin = static_cast<const char*>(bd.data());
  const char* end = begin + bd.size();
  int read = tools::read_varint(begin, end, version);
  if (read <= 0)
    throw std::runtime_error("Internal error getting transaction version");
  return version;
}
//------------------------------------------------------------------
template<class t_ids_container, class t_tx_container, class t_missed_container>
bool Blockchain::get_split_transactions_blobs(const t_ids_container& txs_ids, t_tx_container& txs, t_missed_container& missed_txs) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  reserve_container(txs, txs_ids.size());
  for (const auto& tx_hash : txs_ids)
  {
    try
    {
      cryptonote::blobdata tx;
      if (m_db->get_pruned_tx_blob(tx_hash, tx))
      {
        txs.push_back(std::make_tuple(tx_hash, std::move(tx), crypto::null_hash, cryptonote::blobdata()));
        if (!is_v1_tx(std::get<1>(txs.back())) && !m_db->get_prunable_tx_hash(tx_hash, std::get<2>(txs.back())))
        {
          MERROR("Prunable data hash not found for " << tx_hash);
          return false;
        }
        if (!m_db->get_prunable_tx_blob(tx_hash, std::get<3>(txs.back())))
          std::get<3>(txs.back()).clear();
      }
      else
        missed_txs.push_back(tx_hash);
    }
    catch (const std::exception& e)
    {
      return false;
    }
  }
  return true;
}
//------------------------------------------------------------------
template<class t_ids_container, class t_tx_container, class t_missed_container>
bool Blockchain::get_transactions(const t_ids_container& txs_ids, t_tx_container& txs, t_missed_container& missed_txs) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  reserve_container(txs, txs_ids.size());
  for (const auto& tx_hash : txs_ids)
  {
    try
    {
      cryptonote::blobdata tx;
      if (m_db->get_tx_blob(tx_hash, tx))
      {
        txs.push_back(transaction());
        if (!parse_and_validate_tx_from_blob(tx, txs.back()))
        {
          LOG_ERROR("Invalid transaction");
          return false;
        }
      }
      else
        missed_txs.push_back(tx_hash);
    }
    catch (const std::exception& e)
    {
      return false;
    }
  }
  return true;
}
//------------------------------------------------------------------
// Find the split point between us and foreign blockchain and return
// (by reference) the most recent common block hash along with up to
// BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT additional (more recent) hashes.
bool Blockchain::find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, std::vector<crypto::hash>& hashes, uint64_t& start_height, uint64_t& current_height) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  // if we can't find the split point, return false
  if(!find_blockchain_supplement(qblock_ids, start_height))
  {
    return false;
  }

  m_db->block_txn_start(true);
  current_height = get_current_blockchain_height();
  const uint32_t pruning_seed = get_blockchain_pruning_seed();
  start_height = tools::get_next_unpruned_block_height(start_height, current_height, pruning_seed);
  uint64_t stop_height = tools::get_next_pruned_block_height(start_height, current_height, pruning_seed);
  size_t count = 0;
  hashes.reserve(std::min((size_t)(stop_height - start_height), (size_t)BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT));
  for(size_t i = start_height; i < stop_height && count < BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT; i++, count++)
  {
    hashes.push_back(m_db->get_block_hash_from_height(i));
  }

  m_db->block_txn_stop();
  return true;
}

bool Blockchain::find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, NOTIFY_RESPONSE_CHAIN_ENTRY::request& resp) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  bool result = find_blockchain_supplement(qblock_ids, resp.m_block_ids, resp.start_height, resp.total_height);
  if (result)
    resp.cumulative_difficulty = m_db->get_block_cumulative_difficulty(resp.total_height - 1);

  return result;
}
//------------------------------------------------------------------
//FIXME: change argument to std::vector, low priority
// find split point between ours and foreign blockchain (or start at
// blockchain height <req_start_block>), and return up to max_count FULL
// blocks by reference.
bool Blockchain::find_blockchain_supplement(const uint64_t req_start_block, const std::list<crypto::hash>& qblock_ids, std::vector<std::pair<std::pair<cryptonote::blobdata, crypto::hash>, std::vector<std::pair<crypto::hash, cryptonote::blobdata> > > >& blocks, uint64_t& total_height, uint64_t& start_height, bool pruned, bool get_miner_tx_hash, size_t max_count) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  // if a specific start height has been requested
  if(req_start_block > 0)
  {
    // if requested height is higher than our chain, return false -- we can't help
    if (req_start_block >= m_db->height())
    {
      return false;
    }
    start_height = req_start_block;
  }
  else
  {
    if(!find_blockchain_supplement(qblock_ids, start_height))
    {
      return false;
    }
  }

  m_db->block_txn_start(true);
  total_height = get_current_blockchain_height();
  size_t count = 0, size = 0;
  blocks.reserve(std::min(std::min(max_count, (size_t)10000), (size_t)(total_height - start_height)));
  for(uint64_t i = start_height; i < total_height && count < max_count && (size < FIND_BLOCKCHAIN_SUPPLEMENT_MAX_SIZE || count < 3); i++, count++)
  {
    blocks.resize(blocks.size()+1);
    blocks.back().first.first = m_db->get_block_blob_from_height(i);
    block b;
    CHECK_AND_ASSERT_MES(parse_and_validate_block_from_blob(blocks.back().first.first, b), false, "internal error, invalid block");
    blocks.back().first.second = get_miner_tx_hash ? cryptonote::get_transaction_hash(b.miner_tx) : crypto::null_hash;
    std::vector<crypto::hash> mis;
    std::vector<cryptonote::blobdata> txs;
    get_transactions_blobs(b.tx_hashes, txs, mis, pruned);
    CHECK_AND_ASSERT_MES(!mis.size(), false, "internal error, transaction from block not found");
    size += blocks.back().first.first.size();
    for (const auto &t: txs)
      size += t.size();

    CHECK_AND_ASSERT_MES(txs.size() == b.tx_hashes.size(), false, "mismatched sizes of b.tx_hashes and txs");
    blocks.back().second.reserve(txs.size());
    for (size_t i = 0; i < txs.size(); ++i)
    {
      blocks.back().second.push_back(std::make_pair(b.tx_hashes[i], std::move(txs[i])));
    }
  }
  m_db->block_txn_stop();
  return true;
}
//------------------------------------------------------------------
bool Blockchain::add_block_as_invalid(const block& bl, const crypto::hash& h)
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  block_extended_info bei = AUTO_VAL_INIT(bei);
  bei.bl = bl;
  return add_block_as_invalid(bei, h);
}
//------------------------------------------------------------------
bool Blockchain::add_block_as_invalid(const block_extended_info& bei, const crypto::hash& h)
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  auto i_res = m_invalid_blocks.insert(std::map<crypto::hash, block_extended_info>::value_type(h, bei));
  CHECK_AND_ASSERT_MES(i_res.second, false, "at insertion invalid by tx returned status existed");
  MINFO("BLOCK ADDED AS INVALID: " << h << std::endl << ", prev_id=" << bei.bl.prev_id << ", m_invalid_blocks count=" << m_invalid_blocks.size());
  return true;
}
//------------------------------------------------------------------
bool Blockchain::have_block(const crypto::hash& id) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  if(m_db->block_exists(id))
  {
    LOG_PRINT_L2("block " << id << " found in main chain");
    return true;
  }

  if(m_alternative_chains.count(id))
  {
    LOG_PRINT_L2("block " << id << " found in m_alternative_chains");
    return true;
  }

  if(m_invalid_blocks.count(id))
  {
    LOG_PRINT_L2("block " << id << " found in m_invalid_blocks");
    return true;
  }

  return false;
}
//------------------------------------------------------------------
bool Blockchain::handle_block_to_main_chain(const block& bl, block_verification_context& bvc)
{
    LOG_PRINT_L3("Blockchain::" << __func__);
    crypto::hash id = get_block_hash(bl);
    return handle_block_to_main_chain(bl, id, bvc);
}
//------------------------------------------------------------------
size_t Blockchain::get_total_transactions() const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  // WARNING: this function does not take m_blockchain_lock, and thus should only call read only
  // m_db functions which do not depend on one another (ie, no getheight + gethash(height-1), as
  // well as not accessing class members, even read only (ie, m_invalid_blocks). The caller must
  // lock if it is otherwise needed.
  return m_db->get_tx_count();
}
//------------------------------------------------------------------
// This function checks each input in the transaction <tx> to make sure it
// has not been used already, and adds its key to the container <keys_this_block>.
//
// This container should be managed by the code that validates blocks so we don't
// have to store the used keys in a given block in the permanent storage only to
// remove them later if the block fails validation.
bool Blockchain::check_for_double_spend(const transaction& tx, key_images_container& keys_this_block) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  struct add_transaction_input_visitor: public boost::static_visitor<bool>
  {
    key_images_container& m_spent_keys;
    BlockchainDB* m_db;
    add_transaction_input_visitor(key_images_container& spent_keys, BlockchainDB* db) :
      m_spent_keys(spent_keys), m_db(db)
    {
    }
    bool operator()(const txin_to_key& in) const
    {
      const crypto::key_image& ki = in.k_image;

      // attempt to insert the newly-spent key into the container of
      // keys spent this block.  If this fails, the key was spent already
      // in this block, return false to flag that a double spend was detected.
      //
      // if the insert into the block-wide spent keys container succeeds,
      // check the blockchain-wide spent keys container and make sure the
      // key wasn't used in another block already.
      auto r = m_spent_keys.insert(ki);
      if(!r.second || m_db->has_key_image(ki))
      {
        //double spend detected
        return false;
      }

      // if no double-spend detected, return true
      return true;
    }

    bool operator()(const txin_gen& tx) const
    {
      return true;
    }
    bool operator()(const txin_to_script& tx) const
    {
      return false;
    }
    bool operator()(const txin_to_scripthash& tx) const
    {
      return false;
    }
  };

  for (const txin_v& in : tx.vin)
  {
    if(!boost::apply_visitor(add_transaction_input_visitor(keys_this_block, m_db), in))
    {
      LOG_ERROR("Double spend detected!");
      return false;
    }
  }

  return true;
}
//------------------------------------------------------------------
bool Blockchain::get_tx_outputs_gindexs(const crypto::hash& tx_id, size_t n_txes, std::vector<std::vector<uint64_t>>& indexs) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  uint64_t tx_index;
  if (!m_db->tx_exists(tx_id, tx_index))
  {
    MERROR_VER("get_tx_outputs_gindexs failed to find transaction with id = " << tx_id);
    return false;
  }
  indexs = m_db->get_tx_amount_output_indices(tx_index, n_txes);
  CHECK_AND_ASSERT_MES(n_txes == indexs.size(), false, "Wrong indexs size");

  return true;
}
//------------------------------------------------------------------
bool Blockchain::get_tx_outputs_gindexs(const crypto::hash& tx_id, std::vector<uint64_t>& indexs) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  uint64_t tx_index;
  if (!m_db->tx_exists(tx_id, tx_index))
  {
    MERROR_VER("get_tx_outputs_gindexs failed to find transaction with id = " << tx_id);
    return false;
  }
  std::vector<std::vector<uint64_t>> indices = m_db->get_tx_amount_output_indices(tx_index, 1);
  CHECK_AND_ASSERT_MES(indices.size() == 1, false, "Wrong indices size");
  indexs = indices.front();
  return true;
}
//------------------------------------------------------------------
void Blockchain::on_new_tx_from_block(const cryptonote::transaction &tx)
{
#if defined(PER_BLOCK_CHECKPOINT)
  // check if we're doing per-block checkpointing
  if (m_db->height() < m_blocks_hash_check.size())
  {
    TIME_MEASURE_START(a);
    m_blocks_txs_check.push_back(get_transaction_hash(tx));
    TIME_MEASURE_FINISH(a);
    if(m_show_time_stats)
    {
      size_t ring_size = !tx.vin.empty() && tx.vin[0].type() == typeid(txin_to_key) ? boost::get<txin_to_key>(tx.vin[0]).key_offsets.size() : 0;
      MINFO("HASH: " << "-" << " I/M/O: " << tx.vin.size() << "/" << ring_size << "/" << tx.vout.size() << " H: " << 0 << " chcktx: " << a);
    }
  }
#endif
}
//------------------------------------------------------------------
//FIXME: it seems this function is meant to be merely a wrapper around
//       another function of the same name, this one adding one bit of
//       functionality.  Should probably move anything more than that
//       (getting the hash of the block at height max_used_block_id)
//       to the other function to keep everything in one place.
// This function overloads its sister function with
// an extra value (hash of highest block that holds an output used as input)
// as a return-by-reference.
bool Blockchain::check_tx_inputs(transaction& tx, uint64_t& max_used_block_height, crypto::hash& max_used_block_id, tx_verification_context &tvc, bool kept_by_block)
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

#if defined(PER_BLOCK_CHECKPOINT)
  // check if we're doing per-block checkpointing
  if (m_db->height() < m_blocks_hash_check.size() && kept_by_block)
  {
    max_used_block_id = null_hash;
    max_used_block_height = 0;
    return true;
  }
#endif

  TIME_MEASURE_START(a);
  bool res = check_tx_inputs(tx, tvc, &max_used_block_height);
  TIME_MEASURE_FINISH(a);
  if(m_show_time_stats)
  {
    size_t ring_size = !tx.vin.empty() && tx.vin[0].type() == typeid(txin_to_key) ? boost::get<txin_to_key>(tx.vin[0]).key_offsets.size() : 0;
    MINFO("HASH: " <<  get_transaction_hash(tx) << " I/M/O: " << tx.vin.size() << "/" << ring_size << "/" << tx.vout.size() << " H: " << max_used_block_height << " ms: " << a + m_fake_scan_time << " B: " << get_object_blobsize(tx) << " W: " << get_transaction_weight(tx));
  }
  if (!res)
    return false;

  CHECK_AND_ASSERT_MES(max_used_block_height < m_db->height(), false,  "internal error: max used block index=" << max_used_block_height << " is not less then blockchain size = " << m_db->height());
  max_used_block_id = m_db->get_block_hash_from_height(max_used_block_height);
  return true;
}
//------------------------------------------------------------------
bool Blockchain::check_tx_outputs(const transaction& tx, tx_verification_context &tvc)
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  const uint8_t hf_version = m_hardfork->get_current_version();

  // from hard fork 2, we forbid dust and compound outputs
  if (hf_version >= 2) {
    for (auto &o: tx.vout) {
      if (tx.version == 1)
      {
        if (!is_valid_decomposed_amount(o.amount)) {
          tvc.m_invalid_output = true;
          return false;
        }
      }
    }
  }

  // in a v2 tx, all outputs must have 0 amount
  if (hf_version >= 3) {
    if (tx.version >= 2) {
      for (auto &o: tx.vout) {
        if (o.amount != 0) {
          tvc.m_invalid_output = true;
          return false;
        }
      }
    }
  }

  // from v4, forbid invalid pubkeys
  if (hf_version >= 4) {
    for (const auto &o: tx.vout) {
      if (o.target.type() == typeid(txout_to_key)) {
        const txout_to_key& out_to_key = boost::get<txout_to_key>(o.target);
        if (!crypto::check_key(out_to_key.key)) {
          tvc.m_invalid_output = true;
          return false;
        }
      }
    }
  }

  // from v8, allow bulletproofs
  if (hf_version < 8) {
    if (tx.version >= 2) {
      const bool bulletproof = rct::is_rct_bulletproof(tx.rct_signatures.type);
      if (bulletproof || !tx.rct_signatures.p.bulletproofs.empty())
      {
        MERROR_VER("Bulletproofs are not allowed before v8");
        tvc.m_invalid_output = true;
        return false;
      }
    }
  }

  // from v9, forbid borromean range proofs
  if (hf_version > 8) {
    if (tx.version >= 2) {
      const bool borromean = rct::is_rct_borromean(tx.rct_signatures.type);
      if (borromean)
      {
        MERROR_VER("Borromean range proofs are not allowed after v8");
        tvc.m_invalid_output = true;
        return false;
      }
    }
  }

  // from v10, allow bulletproofs v2
  if (hf_version < HF_VERSION_SMALLER_BP) {
    if (tx.version >= 2) {
      if (tx.rct_signatures.type == rct::RCTTypeBulletproof2)
      {
        MERROR_VER("Bulletproofs v2 are not allowed before v" << HF_VERSION_SMALLER_BP);
        tvc.m_invalid_output = true;
        return false;
      }
    }
  }

  return true;
}
//------------------------------------------------------------------
bool Blockchain::have_tx_keyimges_as_spent(const transaction &tx) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  for (const txin_v& in: tx.vin)
  {
    CHECKED_GET_SPECIFIC_VARIANT(in, const txin_to_key, in_to_key, true);
    if(have_tx_keyimg_as_spent(in_to_key.k_image))
      return true;
  }
  return false;
}
bool Blockchain::expand_transaction_2(transaction &tx, const crypto::hash &tx_prefix_hash, const std::vector<std::vector<rct::ctkey>> &pubkeys)
{
  PERF_TIMER(expand_transaction_2);
  CHECK_AND_ASSERT_MES(tx.version == 2, false, "Transaction version is not 2");

  rct::rctSig &rv = tx.rct_signatures;

  // message - hash of the transaction prefix
  rv.message = rct::hash2rct(tx_prefix_hash);

  // mixRing - full and simple store it in opposite ways
  if (rv.type == rct::RCTTypeFull)
  {
    CHECK_AND_ASSERT_MES(!pubkeys.empty() && !pubkeys[0].empty(), false, "empty pubkeys");
    rv.mixRing.resize(pubkeys[0].size());
    for (size_t m = 0; m < pubkeys[0].size(); ++m)
      rv.mixRing[m].clear();
    for (size_t n = 0; n < pubkeys.size(); ++n)
    {
      CHECK_AND_ASSERT_MES(pubkeys[n].size() <= pubkeys[0].size(), false, "More inputs that first ring");
      for (size_t m = 0; m < pubkeys[n].size(); ++m)
      {
        rv.mixRing[m].push_back(pubkeys[n][m]);
      }
    }
  }
  else if (rv.type == rct::RCTTypeSimple || rv.type == rct::RCTTypeBulletproof || rv.type == rct::RCTTypeBulletproof2)
  {
    CHECK_AND_ASSERT_MES(!pubkeys.empty() && !pubkeys[0].empty(), false, "empty pubkeys");
    rv.mixRing.resize(pubkeys.size());
    for (size_t n = 0; n < pubkeys.size(); ++n)
    {
      rv.mixRing[n].clear();
      for (size_t m = 0; m < pubkeys[n].size(); ++m)
      {
        rv.mixRing[n].push_back(pubkeys[n][m]);
      }
    }
  }
  else
  {
    CHECK_AND_ASSERT_MES(false, false, "Unsupported rct tx type: " + boost::lexical_cast<std::string>(rv.type));
  }

  // II
  if (rv.type == rct::RCTTypeFull)
  {
    rv.p.MGs.resize(1);
    rv.p.MGs[0].II.resize(tx.vin.size());
    for (size_t n = 0; n < tx.vin.size(); ++n)
      rv.p.MGs[0].II[n] = rct::ki2rct(boost::get<txin_to_key>(tx.vin[n]).k_image);
  }
  else if (rv.type == rct::RCTTypeSimple || rv.type == rct::RCTTypeBulletproof || rct::RCTTypeBulletproof2)
  {
    CHECK_AND_ASSERT_MES(rv.p.MGs.size() == tx.vin.size(), false, "Bad MGs size");
    for (size_t n = 0; n < tx.vin.size(); ++n)
    {
      rv.p.MGs[n].II.resize(1);
      rv.p.MGs[n].II[0] = rct::ki2rct(boost::get<txin_to_key>(tx.vin[n]).k_image);
    }
  }
  else
  {
    CHECK_AND_ASSERT_MES(false, false, "Unsupported rct tx type: " + boost::lexical_cast<std::string>(rv.type));
  }

  // outPk was already done by handle_incoming_tx

  return true;
}
//------------------------------------------------------------------
// This function validates transaction inputs and their keys.
// FIXME: consider moving functionality specific to one input into
//        check_tx_input() rather than here, and use this function simply
//        to iterate the inputs as necessary (splitting the task
//        using threads, etc.)
bool Blockchain::check_tx_inputs(transaction& tx, tx_verification_context &tvc, uint64_t* pmax_used_block_height)
{
  PERF_TIMER(check_tx_inputs);
  LOG_PRINT_L3("Blockchain::" << __func__);
  size_t sig_index = 0;
  if(pmax_used_block_height)
    *pmax_used_block_height = 0;

  crypto::hash tx_prefix_hash = get_transaction_prefix_hash(tx);

  const uint8_t hf_version = m_hardfork->get_current_version();

  // from hard fork 2, we require mixin at least 2 unless one output cannot mix with 2 others
  // if one output cannot mix with 2 others, we accept at most 1 output that can mix
  if (hf_version >= 2)
  {
    size_t n_unmixable = 0, n_mixable = 0;
    size_t mixin = std::numeric_limits<size_t>::max();
    const size_t min_mixin = hf_version >= HF_VERSION_MIN_MIXIN_10 ? 10 : hf_version >= HF_VERSION_MIN_MIXIN_6 ? 6 : hf_version >= HF_VERSION_MIN_MIXIN_4 ? 4 : 2;
    for (const auto& txin : tx.vin)
    {
      // non txin_to_key inputs will be rejected below
      if (txin.type() == typeid(txin_to_key))
      {
        const txin_to_key& in_to_key = boost::get<txin_to_key>(txin);
        if (in_to_key.amount == 0)
        {
          // always consider rct inputs mixable. Even if there's not enough rct
          // inputs on the chain to mix with, this is going to be the case for
          // just a few blocks right after the fork at most
          ++n_mixable;
        }
        else
        {
          uint64_t n_outputs = m_db->get_num_outputs(in_to_key.amount);
          MDEBUG("output size " << print_money(in_to_key.amount) << ": " << n_outputs << " available");
          // n_outputs includes the output we're considering
          if (n_outputs <= min_mixin)
            ++n_unmixable;
          else
            ++n_mixable;
        }
        if (in_to_key.key_offsets.size() - 1 < mixin)
          mixin = in_to_key.key_offsets.size() - 1;
      }
    }

    if (((hf_version == HF_VERSION_MIN_MIXIN_10 || hf_version == HF_VERSION_MIN_MIXIN_10+1) && mixin != 10) || (hf_version >= HF_VERSION_MIN_MIXIN_10+2 && mixin > 10))
    {
      MERROR_VER("Tx " << get_transaction_hash(tx) << " has invalid ring size (" << (mixin + 1) << "), it should be 11");
      tvc.m_low_mixin = true;
      return false;
    }

    if (mixin < min_mixin)
    {
      if (n_unmixable == 0)
      {
        MERROR_VER("Tx " << get_transaction_hash(tx) << " has too low ring size (" << (mixin + 1) << "), and no unmixable inputs");
        tvc.m_low_mixin = true;
        return false;
      }
      if (n_mixable > 1)
      {
        MERROR_VER("Tx " << get_transaction_hash(tx) << " has too low ring size (" << (mixin + 1) << "), and more than one mixable input with unmixable inputs");
        tvc.m_low_mixin = true;
        return false;
      }
    }

    // min/max tx version based on HF, and we accept v1 txes if having a non mixable
    const size_t max_tx_version = (hf_version <= 3) ? 1 : 2;
    if (tx.version > max_tx_version)
    {
      MERROR_VER("transaction version " << (unsigned)tx.version << " is higher than max accepted version " << max_tx_version);
      tvc.m_verifivation_failed = true;
      return false;
    }
    const size_t min_tx_version = (n_unmixable > 0 ? 1 : (hf_version >= HF_VERSION_ENFORCE_RCT) ? 2 : 1);
    if (tx.version < min_tx_version)
    {
      MERROR_VER("transaction version " << (unsigned)tx.version << " is lower than min accepted version " << min_tx_version);
      tvc.m_verifivation_failed = true;
      return false;
    }
  }

  // from v7, sorted ins
  if (hf_version >= 7) {
    const crypto::key_image *last_key_image = NULL;
    for (size_t n = 0; n < tx.vin.size(); ++n)
    {
      const txin_v &txin = tx.vin[n];
      if (txin.type() == typeid(txin_to_key))
      {
        const txin_to_key& in_to_key = boost::get<txin_to_key>(txin);
        if (last_key_image && memcmp(&in_to_key.k_image, last_key_image, sizeof(*last_key_image)) >= 0)
        {
          MERROR_VER("transaction has unsorted inputs");
          tvc.m_verifivation_failed = true;
          return false;
        }
        last_key_image = &in_to_key.k_image;
      }
    }
  }
  auto it = m_check_txin_table.find(tx_prefix_hash);
  if(it == m_check_txin_table.end())
  {
    m_check_txin_table.emplace(tx_prefix_hash, std::unordered_map<crypto::key_image, bool>());
    it = m_check_txin_table.find(tx_prefix_hash);
    assert(it != m_check_txin_table.end());
  }

  std::vector<std::vector<rct::ctkey>> pubkeys(tx.vin.size());
  std::vector < uint64_t > results;
  results.resize(tx.vin.size(), 0);

  tools::threadpool& tpool = tools::threadpool::getInstance();
  tools::threadpool::waiter waiter;
  const auto waiter_guard = epee::misc_utils::create_scope_leave_handler([&]() { waiter.wait(&tpool); });
  int threads = tpool.get_max_concurrency();

  for (const auto& txin : tx.vin)
  {
    // make sure output being spent is of type txin_to_key, rather than
    // e.g. txin_gen, which is only used for miner transactions
    CHECK_AND_ASSERT_MES(txin.type() == typeid(txin_to_key), false, "wrong type id in tx input at Blockchain::check_tx_inputs");
    const txin_to_key& in_to_key = boost::get<txin_to_key>(txin);

    // make sure tx output has key offset(s) (is signed to be used)
    CHECK_AND_ASSERT_MES(in_to_key.key_offsets.size(), false, "empty in_to_key.key_offsets in transaction with id " << get_transaction_hash(tx));

    if(have_tx_keyimg_as_spent(in_to_key.k_image))
    {
      MERROR_VER("Key image already spent in blockchain: " << epee::string_tools::pod_to_hex(in_to_key.k_image));
      tvc.m_double_spend = true;
      return false;
    }

    if (tx.version == 1)
    {
      // basically, make sure number of inputs == number of signatures
      CHECK_AND_ASSERT_MES(sig_index < tx.signatures.size(), false, "wrong transaction: not signature entry for input with index= " << sig_index);

#if defined(CACHE_VIN_RESULTS)
      auto itk = it->second.find(in_to_key.k_image);
      if(itk != it->second.end())
      {
        if(!itk->second)
        {
          MERROR_VER("Failed ring signature for tx " << get_transaction_hash(tx) << "  vin key with k_image: " << in_to_key.k_image << "  sig_index: " << sig_index);
          return false;
        }

        // txin has been verified already, skip
        sig_index++;
        continue;
      }
#endif
    }

    // make sure that output being spent matches up correctly with the
    // signature spending it.
    if (!check_tx_input(tx.version, in_to_key, tx_prefix_hash, tx.version == 1 ? tx.signatures[sig_index] : std::vector<crypto::signature>(), tx.rct_signatures, pubkeys[sig_index], pmax_used_block_height))
    {
      it->second[in_to_key.k_image] = false;
      MERROR_VER("Failed to check ring signature for tx " << get_transaction_hash(tx) << "  vin key with k_image: " << in_to_key.k_image << "  sig_index: " << sig_index);
      if (pmax_used_block_height) // a default value of NULL is used when called from Blockchain::handle_block_to_main_chain()
      {
        MERROR_VER("  *pmax_used_block_height: " << *pmax_used_block_height);
      }

      return false;
    }

    if (tx.version == 1)
    {
      if (threads > 1)
      {
        // ND: Speedup
        // 1. Thread ring signature verification if possible.
        tpool.submit(&waiter, boost::bind(&Blockchain::check_ring_signature, this, std::cref(tx_prefix_hash), std::cref(in_to_key.k_image), std::cref(pubkeys[sig_index]), std::cref(tx.signatures[sig_index]), std::ref(results[sig_index])), true);
      }
      else
      {
        check_ring_signature(tx_prefix_hash, in_to_key.k_image, pubkeys[sig_index], tx.signatures[sig_index], results[sig_index]);
        if (!results[sig_index])
        {
          it->second[in_to_key.k_image] = false;
          MERROR_VER("Failed to check ring signature for tx " << get_transaction_hash(tx) << "  vin key with k_image: " << in_to_key.k_image << "  sig_index: " << sig_index);

          if (pmax_used_block_height)  // a default value of NULL is used when called from Blockchain::handle_block_to_main_chain()
          {
            MERROR_VER("*pmax_used_block_height: " << *pmax_used_block_height);
          }

          return false;
        }
        it->second[in_to_key.k_image] = true;
      }
    }

    sig_index++;
  }
  if (tx.version == 1 && threads > 1)
    waiter.wait(&tpool);

  if (tx.version == 1)
  {
    if (threads > 1)
    {
      // save results to table, passed or otherwise
      bool failed = false;
      for (size_t i = 0; i < tx.vin.size(); i++)
      {
        const txin_to_key& in_to_key = boost::get<txin_to_key>(tx.vin[i]);
        it->second[in_to_key.k_image] = results[i];
        if(!failed && !results[i])
          failed = true;
      }

      if (failed)
      {
        MERROR_VER("Failed to check ring signatures!");
        return false;
      }
    }
  }
  else
  {
    if (!expand_transaction_2(tx, tx_prefix_hash, pubkeys))
    {
      MERROR_VER("Failed to expand rct signatures!");
      return false;
    }

    // from version 2, check ringct signatures
    // obviously, the original and simple rct APIs use a mixRing that's indexes
    // in opposite orders, because it'd be too simple otherwise...
    const rct::rctSig &rv = tx.rct_signatures;
    switch (rv.type)
    {
    case rct::RCTTypeNull: {
      // we only accept no signatures for coinbase txes
      MERROR_VER("Null rct signature on non-coinbase tx");
      return false;
    }
    case rct::RCTTypeSimple:
    case rct::RCTTypeBulletproof:
    case rct::RCTTypeBulletproof2:
    {
      // check all this, either reconstructed (so should really pass), or not
      {
        if (pubkeys.size() != rv.mixRing.size())
        {
          MERROR_VER("Failed to check ringct signatures: mismatched pubkeys/mixRing size");
          return false;
        }
        for (size_t i = 0; i < pubkeys.size(); ++i)
        {
          if (pubkeys[i].size() != rv.mixRing[i].size())
          {
            MERROR_VER("Failed to check ringct signatures: mismatched pubkeys/mixRing size");
            return false;
          }
        }

        for (size_t n = 0; n < pubkeys.size(); ++n)
        {
          for (size_t m = 0; m < pubkeys[n].size(); ++m)
          {
            if (pubkeys[n][m].dest != rct::rct2pk(rv.mixRing[n][m].dest))
            {
              MERROR_VER("Failed to check ringct signatures: mismatched pubkey at vin " << n << ", index " << m);
              return false;
            }
            if (pubkeys[n][m].mask != rct::rct2pk(rv.mixRing[n][m].mask))
            {
              MERROR_VER("Failed to check ringct signatures: mismatched commitment at vin " << n << ", index " << m);
              return false;
            }
          }
        }
      }

      if (rv.p.MGs.size() != tx.vin.size())
      {
        MERROR_VER("Failed to check ringct signatures: mismatched MGs/vin sizes");
        return false;
      }
      for (size_t n = 0; n < tx.vin.size(); ++n)
      {
        if (rv.p.MGs[n].II.empty() || memcmp(&boost::get<txin_to_key>(tx.vin[n]).k_image, &rv.p.MGs[n].II[0], 32))
        {
          MERROR_VER("Failed to check ringct signatures: mismatched key image");
          return false;
        }
      }

      if (!rct::verRctNonSemanticsSimple(rv))
      {
        MERROR_VER("Failed to check ringct signatures!");
        return false;
      }
      break;
    }
    case rct::RCTTypeFull:
    {
      // check all this, either reconstructed (so should really pass), or not
      {
        bool size_matches = true;
        for (size_t i = 0; i < pubkeys.size(); ++i)
          size_matches &= pubkeys[i].size() == rv.mixRing.size();
        for (size_t i = 0; i < rv.mixRing.size(); ++i)
          size_matches &= pubkeys.size() == rv.mixRing[i].size();
        if (!size_matches)
        {
          MERROR_VER("Failed to check ringct signatures: mismatched pubkeys/mixRing size");
          return false;
        }

        for (size_t n = 0; n < pubkeys.size(); ++n)
        {
          for (size_t m = 0; m < pubkeys[n].size(); ++m)
          {
            if (pubkeys[n][m].dest != rct::rct2pk(rv.mixRing[m][n].dest))
            {
              MERROR_VER("Failed to check ringct signatures: mismatched pubkey at vin " << n << ", index " << m);
              return false;
            }
            if (pubkeys[n][m].mask != rct::rct2pk(rv.mixRing[m][n].mask))
            {
              MERROR_VER("Failed to check ringct signatures: mismatched commitment at vin " << n << ", index " << m);
              return false;
            }
          }
        }
      }

      if (rv.p.MGs.size() != 1)
      {
        MERROR_VER("Failed to check ringct signatures: Bad MGs size");
        return false;
      }
      if (rv.p.MGs.empty() || rv.p.MGs[0].II.size() != tx.vin.size())
      {
        MERROR_VER("Failed to check ringct signatures: mismatched II/vin sizes");
        return false;
      }
      for (size_t n = 0; n < tx.vin.size(); ++n)
      {
        if (memcmp(&boost::get<txin_to_key>(tx.vin[n]).k_image, &rv.p.MGs[0].II[n], 32))
        {
          MERROR_VER("Failed to check ringct signatures: mismatched II/vin sizes");
          return false;
        }
      }

      if (!rct::verRct(rv, false))
      {
        MERROR_VER("Failed to check ringct signatures!");
        return false;
      }
      break;
    }
    default:
      MERROR_VER("Unsupported rct type: " << rv.type);
      return false;
    }

    // for bulletproofs, check they're only multi-output after v8
    if (rct::is_rct_bulletproof(rv.type))
    {
      if (hf_version < 8)
      {
        for (const rct::Bulletproof &proof: rv.p.bulletproofs)
        {
          if (proof.V.size() > 1)
          {
            MERROR_VER("Multi output bulletproofs are invalid before v8");
            return false;
          }
        }
      }
    }
  }
  return true;
}

//------------------------------------------------------------------
void Blockchain::check_ring_signature(const crypto::hash &tx_prefix_hash, const crypto::key_image &key_image, const std::vector<rct::ctkey> &pubkeys, const std::vector<crypto::signature>& sig, uint64_t &result)
{
  std::vector<const crypto::public_key *> p_output_keys;
  p_output_keys.reserve(pubkeys.size());
  for (auto &key : pubkeys)
  {
    // rct::key and crypto::public_key have the same structure, avoid object ctor/memcpy
    p_output_keys.push_back(&(const crypto::public_key&)key.dest);
  }

  result = crypto::check_ring_signature(tx_prefix_hash, key_image, p_output_keys, sig.data()) ? 1 : 0;
}

//------------------------------------------------------------------
uint64_t Blockchain::get_fee_quantization_mask()
{
  static uint64_t mask = 0;
  if (mask == 0)
  {
    mask = 1;
    for (size_t n = PER_KB_FEE_QUANTIZATION_DECIMALS; n < CRYPTONOTE_DISPLAY_DECIMAL_POINT; ++n)
      mask *= 10;
  }
  return mask;
}

//------------------------------------------------------------------
uint64_t Blockchain::get_dynamic_base_fee(uint64_t block_reward, size_t median_block_weight, uint8_t version)
{
  const uint64_t min_block_weight = get_min_block_weight(version);
  if (median_block_weight < min_block_weight)
    median_block_weight = min_block_weight;
  uint64_t hi, lo;

  if (version >= HF_VERSION_PER_BYTE_FEE)
  {
    lo = mul128(block_reward, DYNAMIC_FEE_REFERENCE_TRANSACTION_WEIGHT, &hi);
    div128_32(hi, lo, min_block_weight, &hi, &lo);
    div128_32(hi, lo, median_block_weight, &hi, &lo);
    assert(hi == 0);
    lo /= 5;
    return lo;
  }

  const uint64_t fee_base = version >= 5 ? DYNAMIC_FEE_PER_KB_BASE_FEE_V5 : DYNAMIC_FEE_PER_KB_BASE_FEE;

  uint64_t unscaled_fee_base = (fee_base * min_block_weight / median_block_weight);
  lo = mul128(unscaled_fee_base, block_reward, &hi);
  static_assert(DYNAMIC_FEE_PER_KB_BASE_BLOCK_REWARD % 1000000 == 0, "DYNAMIC_FEE_PER_KB_BASE_BLOCK_REWARD must be divisible by 1000000");
  static_assert(DYNAMIC_FEE_PER_KB_BASE_BLOCK_REWARD / 1000000 <= std::numeric_limits<uint32_t>::max(), "DYNAMIC_FEE_PER_KB_BASE_BLOCK_REWARD is too large");

  // divide in two steps, since the divisor must be 32 bits, but DYNAMIC_FEE_PER_KB_BASE_BLOCK_REWARD isn't
  div128_32(hi, lo, DYNAMIC_FEE_PER_KB_BASE_BLOCK_REWARD / 1000000, &hi, &lo);
  div128_32(hi, lo, 1000000, &hi, &lo);
  assert(hi == 0);

  // quantize fee up to 8 decimals
  uint64_t mask = get_fee_quantization_mask();
  uint64_t qlo = (lo + mask - 1) / mask * mask;
  MDEBUG("lo " << print_money(lo) << ", qlo " << print_money(qlo) << ", mask " << mask);

  return qlo;
}

//------------------------------------------------------------------
bool Blockchain::check_fee(size_t tx_weight, uint64_t fee) const
{
  const uint8_t version = get_current_hard_fork_version();

  uint64_t median = 0;
  uint64_t already_generated_coins = 0;
  uint64_t base_reward = 0;
  if (version >= HF_VERSION_DYNAMIC_FEE)
  {
    median = m_current_block_cumul_weight_limit / 2;
    already_generated_coins = m_db->height() ? m_db->get_block_already_generated_coins(m_db->height() - 1) : 0;
    if (!get_block_reward(median, 1, already_generated_coins, base_reward, version))
      return false;
  }

  uint64_t needed_fee;
  if (version >= HF_VERSION_PER_BYTE_FEE)
  {
    uint64_t fee_per_byte = get_dynamic_base_fee(base_reward, median, version);
    MDEBUG("Using " << print_money(fee_per_byte) << "/byte fee");
    needed_fee = tx_weight * fee_per_byte;
    // quantize fee up to 8 decimals
    const uint64_t mask = get_fee_quantization_mask();
    needed_fee = (needed_fee + mask - 1) / mask * mask;
  }
  else
  {
    uint64_t fee_per_kb;
    if (version < HF_VERSION_DYNAMIC_FEE)
    {
      fee_per_kb = FEE_PER_KB;
    }
    else
    {
      fee_per_kb = get_dynamic_base_fee(base_reward, median, version);
    }
    MDEBUG("Using " << print_money(fee_per_kb) << "/kB fee");

    needed_fee = tx_weight / 1024;
    needed_fee += (tx_weight % 1024) ? 1 : 0;
    needed_fee *= fee_per_kb;
  }

  if (fee < needed_fee - needed_fee / 50) // keep a little 2% buffer on acceptance - no integer overflow
  {
    MERROR_VER("transaction fee is not enough: " << print_money(fee) << ", minimum fee: " << print_money(needed_fee));
    return false;
  }
  return true;
}

//------------------------------------------------------------------
uint64_t Blockchain::get_dynamic_base_fee_estimate(uint64_t grace_blocks) const
{
  const uint8_t version = get_current_hard_fork_version();

  if (version < HF_VERSION_DYNAMIC_FEE)
    return FEE_PER_KB;

  if (grace_blocks >= CRYPTONOTE_REWARD_BLOCKS_WINDOW)
    grace_blocks = CRYPTONOTE_REWARD_BLOCKS_WINDOW - 1;

  const uint64_t min_block_weight = get_min_block_weight(version);
  std::vector<size_t> weights;
  get_last_n_blocks_weights(weights, CRYPTONOTE_REWARD_BLOCKS_WINDOW - grace_blocks);
  weights.reserve(grace_blocks);
  for (size_t i = 0; i < grace_blocks; ++i)
    weights.push_back(min_block_weight);

  uint64_t median = epee::misc_utils::median(weights);
  if(median <= min_block_weight)
    median = min_block_weight;

  uint64_t already_generated_coins = m_db->height() ? m_db->get_block_already_generated_coins(m_db->height() - 1) : 0;
  uint64_t base_reward;
  if (!get_block_reward(median, 1, already_generated_coins, base_reward, version))
  {
    MERROR("Failed to determine block reward, using placeholder " << print_money(BLOCK_REWARD_OVERESTIMATE) << " as a high bound");
    base_reward = BLOCK_REWARD_OVERESTIMATE;
  }

  uint64_t fee = get_dynamic_base_fee(base_reward, median, version);
  const bool per_byte = version < HF_VERSION_PER_BYTE_FEE;
  MDEBUG("Estimating " << grace_blocks << "-block fee at " << print_money(fee) << "/" << (per_byte ? "byte" : "kB"));
  return fee;
}

//------------------------------------------------------------------
// This function checks to see if a tx is unlocked.  unlock_time is either
// a block index or a unix time.
bool Blockchain::is_tx_spendtime_unlocked(uint64_t unlock_time) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  if(unlock_time < CRYPTONOTE_MAX_BLOCK_NUMBER)
  {
    // ND: Instead of calling get_current_blockchain_height(), call m_db->height()
    //    directly as get_current_blockchain_height() locks the recursive mutex.
    if(m_db->height()-1 + CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS >= unlock_time)
      return true;
    else
      return false;
  }
  else
  {
    //interpret as time
    uint64_t current_time = static_cast<uint64_t>(time(NULL));
    if(current_time + (get_current_hard_fork_version() < 2 ? CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS_V1 : CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS_V2) >= unlock_time)
      return true;
    else
      return false;
  }
  return false;
}
//------------------------------------------------------------------
// This function locates all outputs associated with a given input (mixins)
// and validates that they exist and are usable.  It also checks the ring
// signature for each input.
bool Blockchain::check_tx_input(size_t tx_version, const txin_to_key& txin, const crypto::hash& tx_prefix_hash, const std::vector<crypto::signature>& sig, const rct::rctSig &rct_signatures, std::vector<rct::ctkey> &output_keys, uint64_t* pmax_related_block_height)
{
  LOG_PRINT_L3("Blockchain::" << __func__);

  // ND:
  // 1. Disable locking and make method private.
  //CRITICAL_REGION_LOCAL(m_blockchain_lock);

  struct outputs_visitor
  {
    std::vector<rct::ctkey >& m_output_keys;
    const Blockchain& m_bch;
    outputs_visitor(std::vector<rct::ctkey>& output_keys, const Blockchain& bch) :
      m_output_keys(output_keys), m_bch(bch)
    {
    }
    bool handle_output(uint64_t unlock_time, const crypto::public_key &pubkey, const rct::key &commitment)
    {
      //check tx unlock time
      if (!m_bch.is_tx_spendtime_unlocked(unlock_time))
      {
        MERROR_VER("One of outputs for one of inputs has wrong tx.unlock_time = " << unlock_time);
        return false;
      }

      // The original code includes a check for the output corresponding to this input
      // to be a txout_to_key. This is removed, as the database does not store this info,
      // but only txout_to_key outputs are stored in the DB in the first place, done in
      // Blockchain*::add_output

      m_output_keys.push_back(rct::ctkey({rct::pk2rct(pubkey), commitment}));
      return true;
    }
  };

  output_keys.clear();

  // collect output keys
  outputs_visitor vi(output_keys, *this);
  if (!scan_outputkeys_for_indexes(tx_version, txin, vi, tx_prefix_hash, pmax_related_block_height))
  {
    MERROR_VER("Failed to get output keys for tx with amount = " << print_money(txin.amount) << " and count indexes " << txin.key_offsets.size());
    return false;
  }

  if(txin.key_offsets.size() != output_keys.size())
  {
    MERROR_VER("Output keys for tx with amount = " << txin.amount << " and count indexes " << txin.key_offsets.size() << " returned wrong keys count " << output_keys.size());
    return false;
  }
  if (tx_version == 1) {
    CHECK_AND_ASSERT_MES(sig.size() == output_keys.size(), false, "internal error: tx signatures count=" << sig.size() << " mismatch with outputs keys count for inputs=" << output_keys.size());
  }
  // rct_signatures will be expanded after this
  return true;
}
//------------------------------------------------------------------
//TODO: Is this intended to do something else?  Need to look into the todo there.
uint64_t Blockchain::get_adjusted_time() const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  //TODO: add collecting median time
  return time(NULL);
}
//------------------------------------------------------------------
//TODO: revisit, has changed a bit on upstream
bool Blockchain::check_block_timestamp(std::vector<uint64_t>& timestamps, const block& b, uint64_t& median_ts) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  median_ts = epee::misc_utils::median(timestamps);

  if(b.timestamp < median_ts)
  {
    MERROR_VER("Timestamp of block with id: " << get_block_hash(b) << ", " << b.timestamp << ", less than median of last " << BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW << " blocks, " << median_ts);
    return false;
  }

  return true;
}
//------------------------------------------------------------------
// This function grabs the timestamps from the most recent <n> blocks,
// where n = BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW.  If there are not those many
// blocks in the blockchain, the timestap is assumed to be valid.  If there
// are, this function returns:
//   true if the block's timestamp is not less than the timestamp of the
//       median of the selected blocks
//   false otherwise
bool Blockchain::check_block_timestamp(const block& b, uint64_t& median_ts) const
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  if(b.timestamp > get_adjusted_time() + CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT)
  {
    MERROR_VER("Timestamp of block with id: " << get_block_hash(b) << ", " << b.timestamp << ", bigger than adjusted time + 2 hours");
    return false;
  }

  // if not enough blocks, no proper median yet, return true
  if(m_db->height() < BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW)
  {
    return true;
  }

  std::vector<uint64_t> timestamps;
  auto h = m_db->height();

  // need most recent 60 blocks, get index of first of those
  size_t offset = h - BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW;
  timestamps.reserve(h - offset);
  for(;offset < h; ++offset)
  {
    timestamps.push_back(m_db->get_block_timestamp(offset));
  }

  return check_block_timestamp(timestamps, b, median_ts);
}
//------------------------------------------------------------------
void Blockchain::return_tx_to_pool(std::vector<transaction> &txs)
{
  uint8_t version = get_current_hard_fork_version();
  for (auto& tx : txs)
  {
    cryptonote::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
    // We assume that if they were in a block, the transactions are already
    // known to the network as a whole. However, if we had mined that block,
    // that might not be always true. Unlikely though, and always relaying
    // these again might cause a spike of traffic as many nodes re-relay
    // all the transactions in a popped block when a reorg happens.
    if (!m_tx_pool.add_tx(tx, tvc, true, true, false, version))
    {
      MERROR("Failed to return taken transaction with hash: " << get_transaction_hash(tx) << " to tx_pool");
    }
  }
}
//------------------------------------------------------------------
bool Blockchain::flush_txes_from_pool(const std::vector<crypto::hash> &txids)
{
  CRITICAL_REGION_LOCAL(m_tx_pool);

  bool res = true;
  for (const auto &txid: txids)
  {
    cryptonote::transaction tx;
    size_t tx_weight;
    uint64_t fee;
    bool relayed, do_not_relay, double_spend_seen;
    MINFO("Removing txid " << txid << " from the pool");
    if(m_tx_pool.have_tx(txid) && !m_tx_pool.take_tx(txid, tx, tx_weight, fee, relayed, do_not_relay, double_spend_seen))
    {
      MERROR("Failed to remove txid " << txid << " from the pool");
      res = false;
    }
  }
  return res;
}
//------------------------------------------------------------------
//      Needs to validate the block and acquire each transaction from the
//      transaction mem_pool, then pass the block and transactions to
//      m_db->add_block()
bool Blockchain::handle_block_to_main_chain(const block& bl, const crypto::hash& id, block_verification_context& bvc)
{
  LOG_PRINT_L3("Blockchain::" << __func__);

  TIME_MEASURE_START(block_processing_time);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  TIME_MEASURE_START(t1);

  static bool seen_future_version = false;

  m_db->block_txn_start(true);
  if(bl.prev_id != get_tail_id())
  {
    MERROR_VER("Block with id: " << id << std::endl << "has wrong prev_id: " << bl.prev_id << std::endl << "expected: " << get_tail_id());
    bvc.m_verifivation_failed = true;
leave:
    m_db->block_txn_stop();
    return false;
  }

  // warn users if they're running an old version
  if (!seen_future_version && bl.major_version > m_hardfork->get_ideal_version())
  {
    seen_future_version = true;
    const el::Level level = el::Level::Warning;
    MCLOG_RED(level, "global", "**********************************************************************");
    MCLOG_RED(level, "global", "A block was seen on the network with a version higher than the last");
    MCLOG_RED(level, "global", "known one. This may be an old version of the daemon, and a software");
    MCLOG_RED(level, "global", "update may be required to sync further. Try running: update check");
    MCLOG_RED(level, "global", "**********************************************************************");
  }

  // this is a cheap test
  if (!m_hardfork->check(bl))
  {
    MERROR_VER("Block with id: " << id << std::endl << "has old version: " << (unsigned)bl.major_version << std::endl << "current: " << (unsigned)m_hardfork->get_current_version());
    bvc.m_verifivation_failed = true;
    goto leave;
  }

  TIME_MEASURE_FINISH(t1);
  TIME_MEASURE_START(t2);

  // make sure block timestamp is not less than the median timestamp
  // of a set number of the most recent blocks.
  if(!check_block_timestamp(bl))
  {
    MERROR_VER("Block with id: " << id << std::endl << "has invalid timestamp: " << bl.timestamp);
    bvc.m_verifivation_failed = true;
    goto leave;
  }

  TIME_MEASURE_FINISH(t2);
  //check proof of work
  TIME_MEASURE_START(target_calculating_time);

  // get the target difficulty for the block.
  // the calculation can overflow, among other failure cases,
  // so we need to check the return type.
  // FIXME: get_difficulty_for_next_block can also assert, look into
  // changing this to throwing exceptions instead so we can clean up.
  difficulty_type current_diffic = get_difficulty_for_next_block();
  CHECK_AND_ASSERT_MES(current_diffic, false, "!!!!!!!!! difficulty overhead !!!!!!!!!");

  TIME_MEASURE_FINISH(target_calculating_time);

  TIME_MEASURE_START(longhash_calculating_time);

  crypto::hash proof_of_work = null_hash;

  // Formerly the code below contained an if loop with the following condition
  // !m_checkpoints.is_in_checkpoint_zone(get_current_blockchain_height())
  // however, this caused the daemon to not bother checking PoW for blocks
  // before checkpoints, which is very dangerous behaviour. We moved the PoW
  // validation out of the next chunk of code to make sure that we correctly
  // check PoW now.
  // FIXME: height parameter is not used...should it be used or should it not
  // be a parameter?
  // validate proof_of_work versus difficulty target
  bool precomputed = false;
  bool fast_check = false;
#if defined(PER_BLOCK_CHECKPOINT)
  if (m_db->height() < m_blocks_hash_check.size())
  {
    auto hash = get_block_hash(bl);
    const auto &expected_hash = m_blocks_hash_check[m_db->height()];
    if (expected_hash != crypto::null_hash)
    {
      if (memcmp(&hash, &expected_hash, sizeof(hash)) != 0)
      {
        MERROR_VER("Block with id is INVALID: " << id << ", expected " << expected_hash);
        bvc.m_verifivation_failed = true;
        goto leave;
      }
      fast_check = true;
    }
    else
    {
      MCINFO("verify", "No pre-validated hash at height " << m_db->height() << ", verifying fully");
    }
  }
  else
#endif
  {
    auto it = m_blocks_longhash_table.find(id);
    if (it != m_blocks_longhash_table.end())
    {
      precomputed = true;
      proof_of_work = it->second;
    }
    else
      proof_of_work = get_block_longhash(bl, m_db->height());

    // validate proof_of_work versus difficulty target
    if(!check_hash(proof_of_work, current_diffic))
    {
      MERROR_VER("Block with id: " << id << std::endl << "does not have enough proof of work: " << proof_of_work << std::endl << "unexpected difficulty: " << current_diffic);
      bvc.m_verifivation_failed = true;
      goto leave;
    }
  }

  // If we're at a checkpoint, ensure that our hardcoded checkpoint hash
  // is correct.
  if(m_checkpoints.is_in_checkpoint_zone(get_current_blockchain_height()))
  {
    if(!m_checkpoints.check_block(get_current_blockchain_height(), id))
    {
      LOG_ERROR("CHECKPOINT VALIDATION FAILED");
      bvc.m_verifivation_failed = true;
      goto leave;
    }
  }

  TIME_MEASURE_FINISH(longhash_calculating_time);
  if (precomputed)
    longhash_calculating_time += m_fake_pow_calc_time;

  TIME_MEASURE_START(t3);

  // sanity check basic miner tx properties;
  if(!prevalidate_miner_transaction(bl, m_db->height()))
  {
    MERROR_VER("Block with id: " << id << " failed to pass prevalidation");
    bvc.m_verifivation_failed = true;
    goto leave;
  }

  size_t coinbase_weight = get_transaction_weight(bl.miner_tx);
  size_t cumulative_block_weight = coinbase_weight;

  std::vector<transaction> txs;
  key_images_container keys;

  uint64_t fee_summary = 0;
  uint64_t t_checktx = 0;
  uint64_t t_exists = 0;
  uint64_t t_pool = 0;
  uint64_t t_dblspnd = 0;
  TIME_MEASURE_FINISH(t3);

// XXX old code adds miner tx here

  size_t tx_index = 0;
  // Iterate over the block's transaction hashes, grabbing each
  // from the tx_pool and validating them.  Each is then added
  // to txs.  Keys spent in each are added to <keys> by the double spend check.
  txs.reserve(bl.tx_hashes.size());
  for (const crypto::hash& tx_id : bl.tx_hashes)
  {
    transaction tx;
    size_t tx_weight = 0;
    uint64_t fee = 0;
    bool relayed = false, do_not_relay = false, double_spend_seen = false;
    TIME_MEASURE_START(aa);

// XXX old code does not check whether tx exists
    if (m_db->tx_exists(tx_id))
    {
      MERROR("Block with id: " << id << " attempting to add transaction already in blockchain with id: " << tx_id);
      bvc.m_verifivation_failed = true;
      return_tx_to_pool(txs);
      goto leave;
    }

    TIME_MEASURE_FINISH(aa);
    t_exists += aa;
    TIME_MEASURE_START(bb);

    // get transaction with hash <tx_id> from tx_pool
    if(!m_tx_pool.take_tx(tx_id, tx, tx_weight, fee, relayed, do_not_relay, double_spend_seen))
    {
      MERROR_VER("Block with id: " << id  << " has at least one unknown transaction with id: " << tx_id);
      bvc.m_verifivation_failed = true;
      return_tx_to_pool(txs);
      goto leave;
    }

    TIME_MEASURE_FINISH(bb);
    t_pool += bb;
    // add the transaction to the temp list of transactions, so we can either
    // store the list of transactions all at once or return the ones we've
    // taken from the tx_pool back to it if the block fails verification.
    txs.push_back(tx);
    TIME_MEASURE_START(dd);

    // FIXME: the storage should not be responsible for validation.
    //        If it does any, it is merely a sanity check.
    //        Validation is the purview of the Blockchain class
    //        - TW
    //
    // ND: this is not needed, db->add_block() checks for duplicate k_images and fails accordingly.
    // if (!check_for_double_spend(tx, keys))
    // {
    //     LOG_PRINT_L0("Double spend detected in transaction (id: " << tx_id);
    //     bvc.m_verifivation_failed = true;
    //     break;
    // }

    TIME_MEASURE_FINISH(dd);
    t_dblspnd += dd;
    TIME_MEASURE_START(cc);

#if defined(PER_BLOCK_CHECKPOINT)
    if (!fast_check)
#endif
    {
      // validate that transaction inputs and the keys spending them are correct.
      tx_verification_context tvc;
      if(!check_tx_inputs(tx, tvc))
      {
        MERROR_VER("Block with id: " << id  << " has at least one transaction (id: " << tx_id << ") with wrong inputs.");

        //TODO: why is this done?  make sure that keeping invalid blocks makes sense.
        add_block_as_invalid(bl, id);
        MERROR_VER("Block with id " << id << " added as invalid because of wrong inputs in transactions");
        bvc.m_verifivation_failed = true;
        return_tx_to_pool(txs);
        goto leave;
      }
    }
#if defined(PER_BLOCK_CHECKPOINT)
    else
    {
      // ND: if fast_check is enabled for blocks, there is no need to check
      // the transaction inputs, but do some sanity checks anyway.
      if (tx_index >= m_blocks_txs_check.size() || memcmp(&m_blocks_txs_check[tx_index++], &tx_id, sizeof(tx_id)) != 0)
      {
        MERROR_VER("Block with id: " << id << " has at least one transaction (id: " << tx_id << ") with wrong inputs.");
        //TODO: why is this done?  make sure that keeping invalid blocks makes sense.
        add_block_as_invalid(bl, id);
        MERROR_VER("Block with id " << id << " added as invalid because of wrong inputs in transactions");
        bvc.m_verifivation_failed = true;
        return_tx_to_pool(txs);
        goto leave;
      }
    }
#endif
    TIME_MEASURE_FINISH(cc);
    t_checktx += cc;
    fee_summary += fee;
    cumulative_block_weight += tx_weight;
  }

  m_blocks_txs_check.clear();

  TIME_MEASURE_START(vmt);
  uint64_t base_reward = 0;
  uint64_t already_generated_coins = m_db->height() ? m_db->get_block_already_generated_coins(m_db->height() - 1) : 0;
  if(!validate_miner_transaction(bl, cumulative_block_weight, fee_summary, base_reward, already_generated_coins, bvc.m_partial_block_reward, m_hardfork->get_current_version()))
  {
    MERROR_VER("Block with id: " << id << " has incorrect miner transaction");
    bvc.m_verifivation_failed = true;
    return_tx_to_pool(txs);
    goto leave;
  }

  TIME_MEASURE_FINISH(vmt);
  size_t block_weight;
  difficulty_type cumulative_difficulty;

  // populate various metadata about the block to be stored alongside it.
  block_weight = cumulative_block_weight;
  cumulative_difficulty = current_diffic;
  // In the "tail" state when the minimum subsidy (implemented in get_block_reward) is in effect, the number of
  // coins will eventually exceed MONEY_SUPPLY and overflow a uint64. To prevent overflow, cap already_generated_coins
  // at MONEY_SUPPLY. already_generated_coins is only used to compute the block subsidy and MONEY_SUPPLY yields a
  // subsidy of 0 under the base formula and therefore the minimum subsidy >0 in the tail state.
  already_generated_coins = base_reward < (MONEY_SUPPLY-already_generated_coins) ? already_generated_coins + base_reward : MONEY_SUPPLY;
  if(m_db->height())
    cumulative_difficulty += m_db->get_block_cumulative_difficulty(m_db->height() - 1);

  TIME_MEASURE_FINISH(block_processing_time);
  if(precomputed)
    block_processing_time += m_fake_pow_calc_time;

  m_db->block_txn_stop();
  TIME_MEASURE_START(addblock);
  uint64_t new_height = 0;
  if (!bvc.m_verifivation_failed)
  {
    try
    {
      new_height = m_db->add_block(bl, block_weight, cumulative_difficulty, already_generated_coins, txs);
    }
    catch (const KEY_IMAGE_EXISTS& e)
    {
      LOG_ERROR("Error adding block with hash: " << id << " to blockchain, what = " << e.what());
      bvc.m_verifivation_failed = true;
      return_tx_to_pool(txs);
      return false;
    }
    catch (const std::exception& e)
    {
      //TODO: figure out the best way to deal with this failure
      LOG_ERROR("Error adding block with hash: " << id << " to blockchain, what = " << e.what());
      bvc.m_verifivation_failed = true;
      return_tx_to_pool(txs);
      return false;
    }
  }
  else
  {
    LOG_ERROR("Blocks that failed verification should not reach here");
  }

  TIME_MEASURE_FINISH(addblock);

  // do this after updating the hard fork state since the weight limit may change due to fork
  update_next_cumulative_weight_limit();

  MINFO("+++++ BLOCK SUCCESSFULLY ADDED" << std::endl << "id:\t" << id << std::endl << "PoW:\t" << proof_of_work << std::endl << "HEIGHT " << new_height-1 << ", difficulty:\t" << current_diffic << std::endl << "block reward: " << print_money(fee_summary + base_reward) << "(" << print_money(base_reward) << " + " << print_money(fee_summary) << "), coinbase_weight: " << coinbase_weight << ", cumulative weight: " << cumulative_block_weight << ", " << block_processing_time << "(" << target_calculating_time << "/" << longhash_calculating_time << ")ms");
  if(m_show_time_stats)
  {
    MINFO("Height: " << new_height << " coinbase weight: " << coinbase_weight << " cumm: "
        << cumulative_block_weight << " p/t: " << block_processing_time << " ("
        << target_calculating_time << "/" << longhash_calculating_time << "/"
        << t1 << "/" << t2 << "/" << t3 << "/" << t_exists << "/" << t_pool
        << "/" << t_checktx << "/" << t_dblspnd << "/" << vmt << "/" << addblock << ")ms");
  }

  bvc.m_added_to_main_chain = true;
  ++m_sync_counter;

  // appears to be a NOP *and* is called elsewhere.  wat?
  m_tx_pool.on_blockchain_inc(new_height, id);
  get_difficulty_for_next_block(); // just to cache it
  invalidate_block_template_cache();

  std::shared_ptr<tools::Notify> block_notify = m_block_notify;
  if (block_notify)
    block_notify->notify("%s", epee::string_tools::pod_to_hex(id).c_str(), NULL);

  return true;
}
//------------------------------------------------------------------
bool Blockchain::prune_blockchain(uint32_t pruning_seed)
{
  uint8_t hf_version = m_hardfork->get_current_version();
  if (hf_version < 10)
  {
    MERROR("Most of the network will only be ready for pruned blockchains from v10, not pruning");
    return false;
  }
  return m_db->prune_blockchain(pruning_seed);
}
//------------------------------------------------------------------
bool Blockchain::update_blockchain_pruning()
{
  m_tx_pool.lock();
  epee::misc_utils::auto_scope_leave_caller unlocker = epee::misc_utils::create_scope_leave_handler([&](){m_tx_pool.unlock();});
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  return m_db->update_pruning();
}
//------------------------------------------------------------------
bool Blockchain::check_blockchain_pruning()
{
  m_tx_pool.lock();
  epee::misc_utils::auto_scope_leave_caller unlocker = epee::misc_utils::create_scope_leave_handler([&](){m_tx_pool.unlock();});
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  return m_db->check_pruning();
}
//------------------------------------------------------------------
bool Blockchain::update_next_cumulative_weight_limit()
{
  uint64_t full_reward_zone = get_min_block_weight(get_current_hard_fork_version());

  LOG_PRINT_L3("Blockchain::" << __func__);
  std::vector<size_t> weights;
  get_last_n_blocks_weights(weights, CRYPTONOTE_REWARD_BLOCKS_WINDOW);

  uint64_t median = epee::misc_utils::median(weights);
  m_current_block_cumul_weight_median = median;
  if(median <= full_reward_zone)
    median = full_reward_zone;

  m_current_block_cumul_weight_limit = median*2;
  return true;
}
//------------------------------------------------------------------
bool Blockchain::add_new_block(const block& bl_, block_verification_context& bvc)
{
  LOG_PRINT_L3("Blockchain::" << __func__);
  //copy block here to let modify block.target
  block bl = bl_;
  crypto::hash id = get_block_hash(bl);
  CRITICAL_REGION_LOCAL(m_tx_pool);//to avoid deadlock lets lock tx_pool for whole add/reorganize process
  CRITICAL_REGION_LOCAL1(m_blockchain_lock);
  m_db->block_txn_start(true);
  if(have_block(id))
  {
    LOG_PRINT_L3("block with id = " << id << " already exists");
    bvc.m_already_exists = true;
    m_db->block_txn_stop();
    m_blocks_txs_check.clear();
    return false;
  }

  //check that block refers to chain tail
  if(!(bl.prev_id == get_tail_id()))
  {
    //chain switching or wrong block
    bvc.m_added_to_main_chain = false;
    m_db->block_txn_stop();
    bool r = handle_alternative_block(bl, id, bvc);
    m_blocks_txs_check.clear();
    return r;
    //never relay alternative blocks
  }

  m_db->block_txn_stop();
  return handle_block_to_main_chain(bl, id, bvc);
}
//------------------------------------------------------------------
//TODO: Refactor, consider returning a failure height and letting
//      caller decide course of action.
void Blockchain::check_against_checkpoints(const checkpoints& points, bool enforce)
{
  const auto& pts = points.get_points();
  bool stop_batch;

  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  stop_batch = m_db->batch_start();
  for (const auto& pt : pts)
  {
    // if the checkpoint is for a block we don't have yet, move on
    if (pt.first >= m_db->height())
    {
      continue;
    }

    if (!points.check_block(pt.first, m_db->get_block_hash_from_height(pt.first)))
    {
      // if asked to enforce checkpoints, roll back to a couple of blocks before the checkpoint
      if (enforce)
      {
        LOG_ERROR("Local blockchain failed to pass a checkpoint, rolling back!");
        std::list<block> empty;
        rollback_blockchain_switching(empty, pt.first - 2);
      }
      else
      {
        LOG_ERROR("WARNING: local blockchain failed to pass a MoneroPulse checkpoint, and you could be on a fork. You should either sync up from scratch, OR download a fresh blockchain bootstrap, OR enable checkpoint enforcing with the --enforce-dns-checkpointing command-line option");
      }
    }
  }
  if (stop_batch)
    m_db->batch_stop();
}
//------------------------------------------------------------------
// returns false if any of the checkpoints loading returns false.
// That should happen only if a checkpoint is added that conflicts
// with an existing checkpoint.
bool Blockchain::update_checkpoints(const std::string& file_path, bool check_dns)
{
  if (!m_checkpoints.load_checkpoints_from_json(file_path))
  {
      return false;
  }

  // if we're checking both dns and json, load checkpoints from dns.
  // if we're not hard-enforcing dns checkpoints, handle accordingly
  if (m_enforce_dns_checkpoints && check_dns && !m_offline)
  {
    if (!m_checkpoints.load_checkpoints_from_dns())
    {
      return false;
    }
  }
  else if (check_dns && !m_offline)
  {
    checkpoints dns_points;
    dns_points.load_checkpoints_from_dns();
    if (m_checkpoints.check_for_conflicts(dns_points))
    {
      check_against_checkpoints(dns_points, false);
    }
    else
    {
      MERROR("One or more checkpoints fetched from DNS conflicted with existing checkpoints!");
    }
  }

  check_against_checkpoints(m_checkpoints, true);

  return true;
}
//------------------------------------------------------------------
void Blockchain::set_enforce_dns_checkpoints(bool enforce_checkpoints)
{
  m_enforce_dns_checkpoints = enforce_checkpoints;
}

//------------------------------------------------------------------
void Blockchain::block_longhash_worker(uint64_t height, const epee::span<const block> &blocks, std::unordered_map<crypto::hash, crypto::hash> &map) const
{
  TIME_MEASURE_START(t);
  slow_hash_allocate_state();

  for (const auto & block : blocks)
  {
    if (m_cancel)
       break;
    crypto::hash id = get_block_hash(block);
    crypto::hash pow = get_block_longhash(block, height++);
    map.emplace(id, pow);
  }

  slow_hash_free_state();
  TIME_MEASURE_FINISH(t);
}

//------------------------------------------------------------------
bool Blockchain::cleanup_handle_incoming_blocks(bool force_sync)
{
  bool success = false;

  MTRACE("Blockchain::" << __func__);
  CRITICAL_REGION_BEGIN(m_blockchain_lock);
  TIME_MEASURE_START(t1);

  try
  {
    m_db->batch_stop();
    success = true;
  }
  catch (const std::exception &e)
  {
    MERROR("Exception in cleanup_handle_incoming_blocks: " << e.what());
  }

  if (success && m_sync_counter > 0)
  {
    if (force_sync)
    {
      if(m_db_sync_mode != db_nosync)
        store_blockchain();
      m_sync_counter = 0;
    }
    else if (m_db_sync_threshold && ((m_db_sync_on_blocks && m_sync_counter >= m_db_sync_threshold) || (!m_db_sync_on_blocks && m_bytes_to_sync >= m_db_sync_threshold)))
    {
      MDEBUG("Sync threshold met, syncing");
      if(m_db_sync_mode == db_async)
      {
        m_sync_counter = 0;
        m_bytes_to_sync = 0;
        m_async_service.dispatch(boost::bind(&Blockchain::store_blockchain, this));
      }
      else if(m_db_sync_mode == db_sync)
      {
        store_blockchain();
      }
      else // db_nosync
      {
        // DO NOTHING, not required to call sync.
      }
    }
  }

  TIME_MEASURE_FINISH(t1);
  m_blocks_longhash_table.clear();
  m_scan_table.clear();
  m_blocks_txs_check.clear();
  m_check_txin_table.clear();

  // when we're well clear of the precomputed hashes, free the memory
  if (!m_blocks_hash_check.empty() && m_db->height() > m_blocks_hash_check.size() + 4096)
  {
    MINFO("Dumping block hashes, we're now 4k past " << m_blocks_hash_check.size());
    m_blocks_hash_check.clear();
    m_blocks_hash_check.shrink_to_fit();
  }

  CRITICAL_REGION_END();
  m_tx_pool.unlock();

  update_blockchain_pruning();

  return success;
}

//------------------------------------------------------------------
void Blockchain::output_scan_worker(const uint64_t amount, const std::vector<uint64_t> &offsets, std::vector<output_data_t> &outputs) const
{
  try
  {
    m_db->get_output_key(epee::span<const uint64_t>(&amount, 1), offsets, outputs, true);
  }
  catch (const std::exception& e)
  {
    MERROR_VER("EXCEPTION: " << e.what());
  }
  catch (...)
  {

  }
}

uint64_t Blockchain::prevalidate_block_hashes(uint64_t height, const std::vector<crypto::hash> &hashes)
{
  // new: . . . . . X X X X X . . . . . .
  // pre: A A A A B B B B C C C C D D D D

  // easy case: height >= hashes
  if (height >= m_blocks_hash_of_hashes.size() * HASH_OF_HASHES_STEP)
    return hashes.size();

  // if we're getting old blocks, we might have jettisoned the hashes already
  if (m_blocks_hash_check.empty())
    return hashes.size();

  // find hashes encompassing those block
  size_t first_index = height / HASH_OF_HASHES_STEP;
  size_t last_index = (height + hashes.size() - 1) / HASH_OF_HASHES_STEP;
  MDEBUG("Blocks " << height << " - " << (height + hashes.size() - 1) << " start at " << first_index << " and end at " << last_index);

  // case of not enough to calculate even a single hash
  if (first_index == last_index && hashes.size() < HASH_OF_HASHES_STEP && (height + hashes.size()) % HASH_OF_HASHES_STEP)
    return hashes.size();

  // build hashes vector to hash hashes together
  std::vector<crypto::hash> data;
  data.reserve(hashes.size() + HASH_OF_HASHES_STEP - 1); // may be a bit too much

  // we expect height to be either equal or a bit below db height
  bool disconnected = (height > m_db->height());
  size_t pop;
  if (disconnected && height % HASH_OF_HASHES_STEP)
  {
    ++first_index;
    pop = HASH_OF_HASHES_STEP - height % HASH_OF_HASHES_STEP;
  }
  else
  {
    // we might need some already in the chain for the first part of the first hash
    for (uint64_t h = first_index * HASH_OF_HASHES_STEP; h < height; ++h)
    {
      data.push_back(m_db->get_block_hash_from_height(h));
    }
    pop = 0;
  }

  // push the data to check
  for (const auto &h: hashes)
  {
    if (pop)
      --pop;
    else
      data.push_back(h);
  }

  // hash and check
  uint64_t usable = first_index * HASH_OF_HASHES_STEP - height; // may start negative, but unsigned under/overflow is not UB
  for (size_t n = first_index; n <= last_index; ++n)
  {
    if (n < m_blocks_hash_of_hashes.size())
    {
      // if the last index isn't fully filled, we can't tell if valid
      if (data.size() < (n - first_index) * HASH_OF_HASHES_STEP + HASH_OF_HASHES_STEP)
        break;

      crypto::hash hash;
      cn_fast_hash(data.data() + (n - first_index) * HASH_OF_HASHES_STEP, HASH_OF_HASHES_STEP * sizeof(crypto::hash), hash);
      bool valid = hash == m_blocks_hash_of_hashes[n];

      // add to the known hashes array
      if (!valid)
      {
        MDEBUG("invalid hash for blocks " << n * HASH_OF_HASHES_STEP << " - " << (n * HASH_OF_HASHES_STEP + HASH_OF_HASHES_STEP - 1));
        break;
      }

      size_t end = n * HASH_OF_HASHES_STEP + HASH_OF_HASHES_STEP;
      for (size_t i = n * HASH_OF_HASHES_STEP; i < end; ++i)
      {
        CHECK_AND_ASSERT_MES(m_blocks_hash_check[i] == crypto::null_hash || m_blocks_hash_check[i] == data[i - first_index * HASH_OF_HASHES_STEP],
            0, "Consistency failure in m_blocks_hash_check construction");
        m_blocks_hash_check[i] = data[i - first_index * HASH_OF_HASHES_STEP];
      }
      usable += HASH_OF_HASHES_STEP;
    }
    else
    {
      // if after the end of the precomputed blocks, accept anything
      usable += HASH_OF_HASHES_STEP;
      if (usable > hashes.size())
        usable = hashes.size();
    }
  }
  MDEBUG("usable: " << usable << " / " << hashes.size());
  CHECK_AND_ASSERT_MES(usable < std::numeric_limits<uint64_t>::max() / 2, 0, "usable is negative");
  return usable;
}

//------------------------------------------------------------------
// ND: Speedups:
// 1. Thread long_hash computations if possible (m_max_prepare_blocks_threads = nthreads, default = 4)
// 2. Group all amounts (from txs) and related absolute offsets and form a table of tx_prefix_hash
//    vs [k_image, output_keys] (m_scan_table). This is faster because it takes advantage of bulk queries
//    and is threaded if possible. The table (m_scan_table) will be used later when querying output
//    keys.
bool Blockchain::prepare_handle_incoming_blocks(const std::vector<block_complete_entry> &blocks_entry)
{
  MTRACE("Blockchain::" << __func__);
  TIME_MEASURE_START(prepare);
  bool stop_batch;
  uint64_t bytes = 0;
  size_t total_txs = 0;

  // Order of locking must be:
  //  m_incoming_tx_lock (optional)
  //  m_tx_pool lock
  //  blockchain lock
  //
  //  Something which takes the blockchain lock may never take the txpool lock
  //  if it has not provably taken the txpool lock earlier
  //
  //  The txpool lock is now taken in prepare_handle_incoming_blocks
  //  and released in cleanup_handle_incoming_blocks. This avoids issues
  //  when something uses the pool, which now uses the blockchain and
  //  needs a batch, since a batch could otherwise be active while the
  //  txpool and blockchain locks were not held

  m_tx_pool.lock();
  CRITICAL_REGION_LOCAL1(m_blockchain_lock);

  if(blocks_entry.size() == 0)
    return false;

  for (const auto &entry : blocks_entry)
  {
    bytes += entry.block.size();
    for (const auto &tx_blob : entry.txs)
    {
      bytes += tx_blob.size();
    }
    total_txs += entry.txs.size();
  }
  m_bytes_to_sync += bytes;
  while (!(stop_batch = m_db->batch_start(blocks_entry.size(), bytes))) {
    m_blockchain_lock.unlock();
    m_tx_pool.unlock();
    epee::misc_utils::sleep_no_w(1000);
    m_tx_pool.lock();
    m_blockchain_lock.lock();
  }

  const uint64_t height = m_db->height();
  if ((height + blocks_entry.size()) < m_blocks_hash_check.size())
    return true;

  bool blocks_exist = false;
  tools::threadpool& tpool = tools::threadpool::getInstance();
  unsigned threads = tpool.get_max_concurrency();
  std::vector<block> blocks;
  blocks.resize(blocks_entry.size());

  if (1)
  {
    // limit threads, default limit = 4
    if(threads > m_max_prepare_blocks_threads)
      threads = m_max_prepare_blocks_threads;

    unsigned int batches = blocks_entry.size() / threads;
    unsigned int extra = blocks_entry.size() % threads;
    MDEBUG("block_batches: " << batches);
    std::vector<std::unordered_map<crypto::hash, crypto::hash>> maps(threads);
    auto it = blocks_entry.begin();
    unsigned blockidx = 0;

    for (unsigned i = 0; i < threads; i++)
    {
      for (unsigned int j = 0; j < batches; j++, ++blockidx)
      {
        block &block = blocks[blockidx];

        if (!parse_and_validate_block_from_blob(it->block, block))
          return false;

        // check first block and skip all blocks if its not chained properly
        if (blockidx == 0)
        {
          crypto::hash tophash = m_db->top_block_hash();
          if (block.prev_id != tophash)
          {
            MDEBUG("Skipping prepare blocks. New blocks don't belong to chain.");
            return true;
          }
        }
        if (have_block(get_block_hash(block)))
        {
          blocks_exist = true;
          break;
        }

        std::advance(it, 1);
      }
    }

    for (unsigned i = 0; i < extra && !blocks_exist; i++, blockidx++)
    {
      block &block = blocks[blockidx];

      if (!parse_and_validate_block_from_blob(it->block, block))
        return false;

      if (have_block(get_block_hash(block)))
      {
        blocks_exist = true;
        break;
      }

      std::advance(it, 1);
    }

    if (!blocks_exist)
    {
      m_blocks_longhash_table.clear();
      uint64_t thread_height = height;
      tools::threadpool::waiter waiter;
      for (unsigned int i = 0; i < threads; i++)
      {
        unsigned nblocks = batches;
        if (i < extra)
          ++nblocks;
        tpool.submit(&waiter, boost::bind(&Blockchain::block_longhash_worker, this, thread_height, epee::span<const block>(&blocks[i], nblocks), std::ref(maps[i])), true);
        thread_height += nblocks;
      }

      waiter.wait(&tpool);

      if (m_cancel)
         return false;

      for (const auto & map : maps)
      {
        m_blocks_longhash_table.insert(map.begin(), map.end());
      }
    }
  }

  if (m_cancel)
    return false;

  if (blocks_exist)
  {
    MDEBUG("Skipping prepare blocks. Blocks exist.");
    return true;
  }

  m_fake_scan_time = 0;
  m_fake_pow_calc_time = 0;

  m_scan_table.clear();
  m_check_txin_table.clear();

  TIME_MEASURE_FINISH(prepare);
  m_fake_pow_calc_time = prepare / blocks_entry.size();

  if (blocks_entry.size() > 1 && threads > 1 && m_show_time_stats)
    MDEBUG("Prepare blocks took: " << prepare << " ms");

  TIME_MEASURE_START(scantable);

  // [input] stores all unique amounts found
  std::vector < uint64_t > amounts;
  // [input] stores all absolute_offsets for each amount
  std::map<uint64_t, std::vector<uint64_t>> offset_map;
  // [output] stores all output_data_t for each absolute_offset
  std::map<uint64_t, std::vector<output_data_t>> tx_map;
  std::vector<std::pair<cryptonote::transaction, crypto::hash>> txes(total_txs);

#define SCAN_TABLE_QUIT(m) \
        do { \
            MERROR_VER(m) ;\
            m_scan_table.clear(); \
            return false; \
        } while(0); \

  // generate sorted tables for all amounts and absolute offsets
  size_t tx_index = 0, block_index = 0;
  for (const auto &entry : blocks_entry)
  {
    if (m_cancel)
      return false;

    for (const auto &tx_blob : entry.txs)
    {
      if (tx_index >= txes.size())
        SCAN_TABLE_QUIT("tx_index is out of sync");
      transaction &tx = txes[tx_index].first;
      crypto::hash &tx_prefix_hash = txes[tx_index].second;
      ++tx_index;

      if (!parse_and_validate_tx_base_from_blob(tx_blob, tx))
        SCAN_TABLE_QUIT("Could not parse tx from incoming blocks.");
      cryptonote::get_transaction_prefix_hash(tx, tx_prefix_hash);

      auto its = m_scan_table.find(tx_prefix_hash);
      if (its != m_scan_table.end())
        SCAN_TABLE_QUIT("Duplicate tx found from incoming blocks.");

      m_scan_table.emplace(tx_prefix_hash, std::unordered_map<crypto::key_image, std::vector<output_data_t>>());
      its = m_scan_table.find(tx_prefix_hash);
      assert(its != m_scan_table.end());

      // get all amounts from tx.vin(s)
      for (const auto &txin : tx.vin)
      {
        const txin_to_key &in_to_key = boost::get < txin_to_key > (txin);

        // check for duplicate
        auto it = its->second.find(in_to_key.k_image);
        if (it != its->second.end())
          SCAN_TABLE_QUIT("Duplicate key_image found from incoming blocks.");

        amounts.push_back(in_to_key.amount);
      }

      // sort and remove duplicate amounts from amounts list
      std::sort(amounts.begin(), amounts.end());
      auto last = std::unique(amounts.begin(), amounts.end());
      amounts.erase(last, amounts.end());

      // add amount to the offset_map and tx_map
      for (const uint64_t &amount : amounts)
      {
        if (offset_map.find(amount) == offset_map.end())
          offset_map.emplace(amount, std::vector<uint64_t>());

        if (tx_map.find(amount) == tx_map.end())
          tx_map.emplace(amount, std::vector<output_data_t>());
      }

      // add new absolute_offsets to offset_map
      for (const auto &txin : tx.vin)
      {
        const txin_to_key &in_to_key = boost::get < txin_to_key > (txin);
        // no need to check for duplicate here.
        auto absolute_offsets = relative_output_offsets_to_absolute(in_to_key.key_offsets);
        for (const auto & offset : absolute_offsets)
          offset_map[in_to_key.amount].push_back(offset);

      }
    }
    ++block_index;
  }

  // sort and remove duplicate absolute_offsets in offset_map
  for (auto &offsets : offset_map)
  {
    std::sort(offsets.second.begin(), offsets.second.end());
    auto last = std::unique(offsets.second.begin(), offsets.second.end());
    offsets.second.erase(last, offsets.second.end());
  }

  // gather all the output keys
  threads = tpool.get_max_concurrency();
  if (!m_db->can_thread_bulk_indices())
    threads = 1;

  if (threads > 1 && amounts.size() > 1)
  {
    tools::threadpool::waiter waiter;

    for (size_t i = 0; i < amounts.size(); i++)
    {
      uint64_t amount = amounts[i];
      tpool.submit(&waiter, boost::bind(&Blockchain::output_scan_worker, this, amount, std::cref(offset_map[amount]), std::ref(tx_map[amount])), true);
    }
    waiter.wait(&tpool);
  }
  else
  {
    for (size_t i = 0; i < amounts.size(); i++)
    {
      uint64_t amount = amounts[i];
      output_scan_worker(amount, offset_map[amount], tx_map[amount]);
    }
  }

  // now generate a table for each tx_prefix and k_image hashes
  tx_index = 0;
  for (const auto &entry : blocks_entry)
  {
    if (m_cancel)
      return false;

    for (const auto &tx_blob : entry.txs)
    {
      if (tx_index >= txes.size())
        SCAN_TABLE_QUIT("tx_index is out of sync");
      const transaction &tx = txes[tx_index].first;
      const crypto::hash &tx_prefix_hash = txes[tx_index].second;
      ++tx_index;

      auto its = m_scan_table.find(tx_prefix_hash);
      if (its == m_scan_table.end())
        SCAN_TABLE_QUIT("Tx not found on scan table from incoming blocks.");

      for (const auto &txin : tx.vin)
      {
        const txin_to_key &in_to_key = boost::get < txin_to_key > (txin);
        auto needed_offsets = relative_output_offsets_to_absolute(in_to_key.key_offsets);

        std::vector<output_data_t> outputs;
        for (const uint64_t & offset_needed : needed_offsets)
        {
          size_t pos = 0;
          bool found = false;

          for (const uint64_t &offset_found : offset_map[in_to_key.amount])
          {
            if (offset_needed == offset_found)
            {
              found = true;
              break;
            }

            ++pos;
          }

          if (found && pos < tx_map[in_to_key.amount].size())
            outputs.push_back(tx_map[in_to_key.amount].at(pos));
          else
            break;
        }

        its->second.emplace(in_to_key.k_image, outputs);
      }
    }
  }

  TIME_MEASURE_FINISH(scantable);
  if (total_txs > 0)
  {
    m_fake_scan_time = scantable / total_txs;
    if(m_show_time_stats)
      MDEBUG("Prepare scantable took: " << scantable << " ms");
  }

  return true;
}

void Blockchain::add_txpool_tx(const crypto::hash &txid, const cryptonote::blobdata &blob, const txpool_tx_meta_t &meta)
{
  m_db->add_txpool_tx(txid, blob, meta);
}

void Blockchain::update_txpool_tx(const crypto::hash &txid, const txpool_tx_meta_t &meta)
{
  m_db->update_txpool_tx(txid, meta);
}

void Blockchain::remove_txpool_tx(const crypto::hash &txid)
{
  m_db->remove_txpool_tx(txid);
}

uint64_t Blockchain::get_txpool_tx_count(bool include_unrelayed_txes) const
{
  return m_db->get_txpool_tx_count(include_unrelayed_txes);
}

bool Blockchain::get_txpool_tx_meta(const crypto::hash& txid, txpool_tx_meta_t &meta) const
{
  return m_db->get_txpool_tx_meta(txid, meta);
}

bool Blockchain::get_txpool_tx_blob(const crypto::hash& txid, cryptonote::blobdata &bd) const
{
  return m_db->get_txpool_tx_blob(txid, bd);
}

cryptonote::blobdata Blockchain::get_txpool_tx_blob(const crypto::hash& txid) const
{
  return m_db->get_txpool_tx_blob(txid);
}

bool Blockchain::for_all_txpool_txes(std::function<bool(const crypto::hash&, const txpool_tx_meta_t&, const cryptonote::blobdata*)> f, bool include_blob, bool include_unrelayed_txes) const
{
  return m_db->for_all_txpool_txes(f, include_blob, include_unrelayed_txes);
}

void Blockchain::set_user_options(uint64_t maxthreads, bool sync_on_blocks, uint64_t sync_threshold, blockchain_db_sync_mode sync_mode, bool fast_sync)
{
  if (sync_mode == db_defaultsync)
  {
    m_db_default_sync = true;
    sync_mode = db_async;
  }
  m_db_sync_mode = sync_mode;
  m_fast_sync = fast_sync;
  m_db_sync_on_blocks = sync_on_blocks;
  m_db_sync_threshold = sync_threshold;
  m_max_prepare_blocks_threads = maxthreads;
}

void Blockchain::safesyncmode(const bool onoff)
{
  /* all of this is no-op'd if the user set a specific
   * --db-sync-mode at startup.
   */
  if (m_db_default_sync)
  {
    m_db->safesyncmode(onoff);
    m_db_sync_mode = onoff ? db_nosync : db_async;
  }
}

HardFork::State Blockchain::get_hard_fork_state() const
{
  return m_hardfork->get_state();
}

const std::vector<HardFork::Params>& Blockchain::get_hard_fork_heights(network_type nettype)
{
  static const std::vector<HardFork::Params> mainnet_heights = []()
  {
    std::vector<HardFork::Params> heights;
    for (const auto& i : mainnet_hard_forks)
      heights.emplace_back(i.version, i.height, i.threshold, i.time);
    return heights;
  }();
  static const std::vector<HardFork::Params> testnet_heights = []()
  {
    std::vector<HardFork::Params> heights;
    for (const auto& i : testnet_hard_forks)
      heights.emplace_back(i.version, i.height, i.threshold, i.time);
    return heights;
  }();
  static const std::vector<HardFork::Params> stagenet_heights = []()
  {
    std::vector<HardFork::Params> heights;
    for (const auto& i : stagenet_hard_forks)
      heights.emplace_back(i.version, i.height, i.threshold, i.time);
    return heights;
  }();
  static const std::vector<HardFork::Params> dummy;
  switch (nettype)
  {
    case MAINNET: return mainnet_heights;
    case TESTNET: return testnet_heights;
    case STAGENET: return stagenet_heights;
    default: return dummy;
  }
}

bool Blockchain::get_hard_fork_voting_info(uint8_t version, uint32_t &window, uint32_t &votes, uint32_t &threshold, uint64_t &earliest_height, uint8_t &voting) const
{
  return m_hardfork->get_voting_info(version, window, votes, threshold, earliest_height, voting);
}

uint64_t Blockchain::get_difficulty_target() const
{
  return get_current_hard_fork_version() < 2 ? DIFFICULTY_TARGET_V1 : DIFFICULTY_TARGET_V2;
}

std::map<uint64_t, std::tuple<uint64_t, uint64_t, uint64_t>> Blockchain:: get_output_histogram(const std::vector<uint64_t> &amounts, bool unlocked, uint64_t recent_cutoff, uint64_t min_count) const
{
  return m_db->get_output_histogram(amounts, unlocked, recent_cutoff, min_count);
}

std::list<std::pair<Blockchain::block_extended_info,std::vector<crypto::hash>>> Blockchain::get_alternative_chains() const
{
  std::list<std::pair<Blockchain::block_extended_info,std::vector<crypto::hash>>> chains;

  for (const auto &i: m_alternative_chains)
  {
    const crypto::hash &top = i.first;
    bool found = false;
    for (const auto &j: m_alternative_chains)
    {
      if (j.second.bl.prev_id == top)
      {
        found = true;
        break;
      }
    }
    if (!found)
    {
      std::vector<crypto::hash> chain;
      auto h = i.second.bl.prev_id;
      chain.push_back(top);
      blocks_ext_by_hash::const_iterator prev;
      while ((prev = m_alternative_chains.find(h)) != m_alternative_chains.end())
      {
        chain.push_back(h);
        h = prev->second.bl.prev_id;
      }
      chains.push_back(std::make_pair(i.second, chain));
    }
  }
  return chains;
}

void Blockchain::cancel()
{
  m_cancel = true;
}

#if defined(PER_BLOCK_CHECKPOINT)
static const char expected_block_hashes_hash[] = "954cb2bbfa2fe6f74b2cdd22a1a4c767aea249ad47ad4f7c9445f0f03260f511";
void Blockchain::load_compiled_in_block_hashes(const GetCheckpointsCallback& get_checkpoints)
{
  if (get_checkpoints == nullptr || !m_fast_sync)
  {
    return;
  }
  const epee::span<const unsigned char> &checkpoints = get_checkpoints(m_nettype);
  if (!checkpoints.empty())
  {
    MINFO("Loading precomputed blocks (" << checkpoints.size() << " bytes)");
    if (m_nettype == MAINNET)
    {
      // first check hash
      crypto::hash hash;
      if (!tools::sha256sum(checkpoints.data(), checkpoints.size(), hash))
      {
        MERROR("Failed to hash precomputed blocks data");
        return;
      }
      MINFO("precomputed blocks hash: " << hash << ", expected " << expected_block_hashes_hash);
      cryptonote::blobdata expected_hash_data;
      if (!epee::string_tools::parse_hexstr_to_binbuff(std::string(expected_block_hashes_hash), expected_hash_data) || expected_hash_data.size() != sizeof(crypto::hash))
      {
        MERROR("Failed to parse expected block hashes hash");
        return;
      }
      const crypto::hash expected_hash = *reinterpret_cast<const crypto::hash*>(expected_hash_data.data());
      if (hash != expected_hash)
      {
        MERROR("Block hash data does not match expected hash");
        return;
      }
    }

    if (checkpoints.size() > 4)
    {
      const unsigned char *p = checkpoints.data();
      const uint32_t nblocks = *p | ((*(p+1))<<8) | ((*(p+2))<<16) | ((*(p+3))<<24);
      if (nblocks > (std::numeric_limits<uint32_t>::max() - 4) / sizeof(hash))
      {
        MERROR("Block hash data is too large");
        return;
      }
      const size_t size_needed = 4 + nblocks * sizeof(crypto::hash);
      if(nblocks > 0 && nblocks > (m_db->height() + HASH_OF_HASHES_STEP - 1) / HASH_OF_HASHES_STEP && checkpoints.size() >= size_needed)
      {
        p += sizeof(uint32_t);
        m_blocks_hash_of_hashes.reserve(nblocks);
        for (uint32_t i = 0; i < nblocks; i++)
        {
          crypto::hash hash;
          memcpy(hash.data, p, sizeof(hash.data));
          p += sizeof(hash.data);
          m_blocks_hash_of_hashes.push_back(hash);
        }
        m_blocks_hash_check.resize(m_blocks_hash_of_hashes.size() * HASH_OF_HASHES_STEP, crypto::null_hash);
        MINFO(nblocks << " block hashes loaded");

        // FIXME: clear tx_pool because the process might have been
        // terminated and caused it to store txs kept by blocks.
        // The core will not call check_tx_inputs(..) for these
        // transactions in this case. Consequently, the sanity check
        // for tx hashes will fail in handle_block_to_main_chain(..)
        CRITICAL_REGION_LOCAL(m_tx_pool);

        std::vector<transaction> txs;
        m_tx_pool.get_transactions(txs);

        size_t tx_weight;
        uint64_t fee;
        bool relayed, do_not_relay, double_spend_seen;
        transaction pool_tx;
        for(const transaction &tx : txs)
        {
          crypto::hash tx_hash = get_transaction_hash(tx);
          m_tx_pool.take_tx(tx_hash, pool_tx, tx_weight, fee, relayed, do_not_relay, double_spend_seen);
        }
      }
    }
  }
}
#endif

bool Blockchain::is_within_compiled_block_hash_area(uint64_t height) const
{
#if defined(PER_BLOCK_CHECKPOINT)
  return height < m_blocks_hash_of_hashes.size() * HASH_OF_HASHES_STEP;
#else
  return false;
#endif
}

void Blockchain::lock()
{
  m_blockchain_lock.lock();
}

void Blockchain::unlock()
{
  m_blockchain_lock.unlock();
}

bool Blockchain::for_all_key_images(std::function<bool(const crypto::key_image&)> f) const
{
  return m_db->for_all_key_images(f);
}

bool Blockchain::for_blocks_range(const uint64_t& h1, const uint64_t& h2, std::function<bool(uint64_t, const crypto::hash&, const block&)> f) const
{
  return m_db->for_blocks_range(h1, h2, f);
}

bool Blockchain::for_all_transactions(std::function<bool(const crypto::hash&, const cryptonote::transaction&)> f, bool pruned) const
{
  return m_db->for_all_transactions(f, pruned);
}

bool Blockchain::for_all_outputs(std::function<bool(uint64_t amount, const crypto::hash &tx_hash, uint64_t height, size_t tx_idx)> f) const
{
  return m_db->for_all_outputs(f);;
}

bool Blockchain::for_all_outputs(uint64_t amount, std::function<bool(uint64_t height)> f) const
{
  return m_db->for_all_outputs(amount, f);;
}

void Blockchain::invalidate_block_template_cache()
{
  MDEBUG("Invalidating block template cache");
  m_btc_valid = false;
}

void Blockchain::cache_block_template(const block &b, const cryptonote::account_public_address &address, const blobdata &nonce, const difficulty_type &diff, uint64_t expected_reward, uint64_t pool_cookie)
{
  MDEBUG("Setting block template cache");
  m_btc = b;
  m_btc_address = address;
  m_btc_nonce = nonce;
  m_btc_difficulty = diff;
  m_btc_expected_reward = expected_reward;
  m_btc_pool_cookie = pool_cookie;
  m_btc_valid = true;
}

namespace cryptonote {
template bool Blockchain::get_transactions(const std::vector<crypto::hash>&, std::vector<transaction>&, std::vector<crypto::hash>&) const;
template bool Blockchain::get_transactions_blobs(const std::vector<crypto::hash>&, std::vector<cryptonote::blobdata>&, std::vector<crypto::hash>&, bool) const;
template bool Blockchain::get_split_transactions_blobs(const std::vector<crypto::hash>&, std::vector<std::tuple<crypto::hash, cryptonote::blobdata, crypto::hash, cryptonote::blobdata>>&, std::vector<crypto::hash>&) const;
}
