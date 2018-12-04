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

#pragma once

#include <vector>
#include <iostream>
#include <stdint.h>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/variant.hpp>

#include "include_base_utils.h"
#include "common/boost_serialization_helper.h"
#include "common/command_line.h"

#include "cryptonote_basic/account_boost_serialization.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_basic/cryptonote_boost_serialization.h"
#include "misc_language.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "tests.core"



struct callback_entry
{
  std::string callback_name;
  BEGIN_SERIALIZE_OBJECT()
    FIELD(callback_name)
  END_SERIALIZE()

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/)
  {
    ar & callback_name;
  }
};

template<typename T>
struct serialized_object
{
  serialized_object() { }

  serialized_object(const cryptonote::blobdata& a_data)
    : data(a_data)
  {
  }

  cryptonote::blobdata data;
  BEGIN_SERIALIZE_OBJECT()
    FIELD(data)
    END_SERIALIZE()

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/)
  {
    ar & data;
  }
};

typedef serialized_object<cryptonote::block> serialized_block;
typedef serialized_object<cryptonote::transaction> serialized_transaction;

struct event_visitor_settings
{
  int valid_mask;
  bool txs_keeped_by_block;

  enum settings
  {
    set_txs_keeped_by_block = 1 << 0
  };

  event_visitor_settings(int a_valid_mask = 0, bool a_txs_keeped_by_block = false)
    : valid_mask(a_valid_mask)
    , txs_keeped_by_block(a_txs_keeped_by_block)
  {
  }

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/)
  {
    ar & valid_mask;
    ar & txs_keeped_by_block;
  }
};

VARIANT_TAG(binary_archive, callback_entry, 0xcb);
VARIANT_TAG(binary_archive, cryptonote::account_base, 0xcc);
VARIANT_TAG(binary_archive, serialized_block, 0xcd);
VARIANT_TAG(binary_archive, serialized_transaction, 0xce);
VARIANT_TAG(binary_archive, event_visitor_settings, 0xcf);

typedef boost::variant<cryptonote::block, cryptonote::transaction, std::vector<cryptonote::transaction>, cryptonote::account_base, callback_entry, serialized_block, serialized_transaction, event_visitor_settings> test_event_entry;
typedef std::unordered_map<crypto::hash, const cryptonote::transaction*> map_hash2tx_t;

class test_chain_unit_base
{
public:
  typedef boost::function<bool (cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events)> verify_callback;
  typedef std::map<std::string, verify_callback> callbacks_map;

  void register_callback(const std::string& cb_name, verify_callback cb);
  bool verify(const std::string& cb_name, cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
private:
  callbacks_map m_callbacks;
};


class test_generator
{
public:
  struct block_info
  {
    block_info()
      : prev_id()
      , already_generated_coins(0)
      , block_weight(0)
    {
    }

    block_info(crypto::hash a_prev_id, uint64_t an_already_generated_coins, size_t a_block_weight)
      : prev_id(a_prev_id)
      , already_generated_coins(an_already_generated_coins)
      , block_weight(a_block_weight)
    {
    }

    crypto::hash prev_id;
    uint64_t already_generated_coins;
    size_t block_weight;
  };

  enum block_fields
  {
    bf_none      = 0,
    bf_major_ver = 1 << 0,
    bf_minor_ver = 1 << 1,
    bf_timestamp = 1 << 2,
    bf_prev_id   = 1 << 3,
    bf_miner_tx  = 1 << 4,
    bf_tx_hashes = 1 << 5,
    bf_diffic    = 1 << 6,
    bf_max_outs  = 1 << 7,
    bf_hf_version= 1 << 8
  };

  void get_block_chain(std::vector<block_info>& blockchain, const crypto::hash& head, size_t n) const;
  void get_last_n_block_weights(std::vector<size_t>& block_weights, const crypto::hash& head, size_t n) const;
  uint64_t get_already_generated_coins(const crypto::hash& blk_id) const;
  uint64_t get_already_generated_coins(const cryptonote::block& blk) const;

  void add_block(const cryptonote::block& blk, size_t tsx_size, std::vector<size_t>& block_weights, uint64_t already_generated_coins,
    uint8_t hf_version = 1);
  bool construct_block(cryptonote::block& blk, uint64_t height, const crypto::hash& prev_id,
    const cryptonote::account_base& miner_acc, uint64_t timestamp, uint64_t already_generated_coins,
    std::vector<size_t>& block_weights, const std::list<cryptonote::transaction>& tx_list);
  bool construct_block(cryptonote::block& blk, const cryptonote::account_base& miner_acc, uint64_t timestamp);
  bool construct_block(cryptonote::block& blk, const cryptonote::block& blk_prev, const cryptonote::account_base& miner_acc,
    const std::list<cryptonote::transaction>& tx_list = std::list<cryptonote::transaction>());

  bool construct_block_manually(cryptonote::block& blk, const cryptonote::block& prev_block,
    const cryptonote::account_base& miner_acc, int actual_params = bf_none, uint8_t major_ver = 0,
    uint8_t minor_ver = 0, uint64_t timestamp = 0, const crypto::hash& prev_id = crypto::hash(),
    const cryptonote::difficulty_type& diffic = 1, const cryptonote::transaction& miner_tx = cryptonote::transaction(),
    const std::vector<crypto::hash>& tx_hashes = std::vector<crypto::hash>(), size_t txs_sizes = 0, size_t max_outs = 999,
    uint8_t hf_version = 1);
  bool construct_block_manually_tx(cryptonote::block& blk, const cryptonote::block& prev_block,
    const cryptonote::account_base& miner_acc, const std::vector<crypto::hash>& tx_hashes, size_t txs_size);

private:
  std::unordered_map<crypto::hash, block_info> m_blocks_info;
};

inline cryptonote::difficulty_type get_test_difficulty() {return 1;}
void fill_nonce(cryptonote::block& blk, const cryptonote::difficulty_type& diffic, uint64_t height);

bool construct_miner_tx_manually(size_t height, uint64_t already_generated_coins,
                                 const cryptonote::account_public_address& miner_address, cryptonote::transaction& tx,
                                 uint64_t fee, cryptonote::keypair* p_txkey = 0);
bool construct_tx_to_key(const std::vector<test_event_entry>& events, cryptonote::transaction& tx,
                         const cryptonote::block& blk_head, const cryptonote::account_base& from, const cryptonote::account_base& to,
                         uint64_t amount, uint64_t fee, size_t nmix);
cryptonote::transaction construct_tx_with_fee(std::vector<test_event_entry>& events, const cryptonote::block& blk_head,
                                            const cryptonote::account_base& acc_from, const cryptonote::account_base& acc_to,
                                            uint64_t amount, uint64_t fee);

void get_confirmed_txs(const std::vector<cryptonote::block>& blockchain, const map_hash2tx_t& mtx, map_hash2tx_t& confirmed_txs);
bool find_block_chain(const std::vector<test_event_entry>& events, std::vector<cryptonote::block>& blockchain, map_hash2tx_t& mtx, const crypto::hash& head);
void fill_tx_sources_and_destinations(const std::vector<test_event_entry>& events, const cryptonote::block& blk_head,
                                      const cryptonote::account_base& from, const cryptonote::account_base& to,
                                      uint64_t amount, uint64_t fee, size_t nmix,
                                      std::vector<cryptonote::tx_source_entry>& sources,
                                      std::vector<cryptonote::tx_destination_entry>& destinations);
uint64_t get_balance(const cryptonote::account_base& addr, const std::vector<cryptonote::block>& blockchain, const map_hash2tx_t& mtx);

//--------------------------------------------------------------------------
template<class t_test_class>
auto do_check_tx_verification_context(const cryptonote::tx_verification_context& tvc, bool tx_added, size_t event_index, const cryptonote::transaction& tx, t_test_class& validator, int)
  -> decltype(validator.check_tx_verification_context(tvc, tx_added, event_index, tx))
{
  return validator.check_tx_verification_context(tvc, tx_added, event_index, tx);
}
//--------------------------------------------------------------------------
template<class t_test_class>
bool do_check_tx_verification_context(const cryptonote::tx_verification_context& tvc, bool tx_added, size_t /*event_index*/, const cryptonote::transaction& /*tx*/, t_test_class&, long)
{
  // Default block verification context check
  if (tvc.m_verifivation_failed)
    throw std::runtime_error("Transaction verification failed");
  return true;
}
//--------------------------------------------------------------------------
template<class t_test_class>
bool check_tx_verification_context(const cryptonote::tx_verification_context& tvc, bool tx_added, size_t event_index, const cryptonote::transaction& tx, t_test_class& validator)
{
  // SFINAE in action
  return do_check_tx_verification_context(tvc, tx_added, event_index, tx, validator, 0);
}
//--------------------------------------------------------------------------
template<class t_test_class>
auto do_check_tx_verification_context(const std::vector<cryptonote::tx_verification_context>& tvcs, size_t tx_added, size_t event_index, const std::vector<cryptonote::transaction>& txs, t_test_class& validator, int)
  -> decltype(validator.check_tx_verification_context(tvcs, tx_added, event_index, txs))
{
  return validator.check_tx_verification_context(tvcs, tx_added, event_index, txs);
}
//--------------------------------------------------------------------------
template<class t_test_class>
bool do_check_tx_verification_context(const std::vector<cryptonote::tx_verification_context>& tvcs, size_t tx_added, size_t /*event_index*/, const std::vector<cryptonote::transaction>& /*txs*/, t_test_class&, long)
{
  // Default block verification context check
  for (const cryptonote::tx_verification_context &tvc: tvcs)
    if (tvc.m_verifivation_failed)
      throw std::runtime_error("Transaction verification failed");
  return true;
}
//--------------------------------------------------------------------------
template<class t_test_class>
bool check_tx_verification_context(const std::vector<cryptonote::tx_verification_context>& tvcs, size_t tx_added, size_t event_index, const std::vector<cryptonote::transaction>& txs, t_test_class& validator)
{
  // SFINAE in action
  return do_check_tx_verification_context(tvcs, tx_added, event_index, txs, validator, 0);
}
//--------------------------------------------------------------------------
template<class t_test_class>
auto do_check_block_verification_context(const cryptonote::block_verification_context& bvc, size_t event_index, const cryptonote::block& blk, t_test_class& validator, int)
  -> decltype(validator.check_block_verification_context(bvc, event_index, blk))
{
  return validator.check_block_verification_context(bvc, event_index, blk);
}
//--------------------------------------------------------------------------
template<class t_test_class>
bool do_check_block_verification_context(const cryptonote::block_verification_context& bvc, size_t /*event_index*/, const cryptonote::block& /*blk*/, t_test_class&, long)
{
  // Default block verification context check
  if (bvc.m_verifivation_failed)
    throw std::runtime_error("Block verification failed");
  return true;
}
//--------------------------------------------------------------------------
template<class t_test_class>
bool check_block_verification_context(const cryptonote::block_verification_context& bvc, size_t event_index, const cryptonote::block& blk, t_test_class& validator)
{
  // SFINAE in action
  return do_check_block_verification_context(bvc, event_index, blk, validator, 0);
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
template<class t_test_class>
struct push_core_event_visitor: public boost::static_visitor<bool>
{
private:
  cryptonote::core& m_c;
  const std::vector<test_event_entry>& m_events;
  t_test_class& m_validator;
  size_t m_ev_index;

  bool m_txs_keeped_by_block;

public:
  push_core_event_visitor(cryptonote::core& c, const std::vector<test_event_entry>& events, t_test_class& validator)
    : m_c(c)
    , m_events(events)
    , m_validator(validator)
    , m_ev_index(0)
    , m_txs_keeped_by_block(false)
  {
  }

  void event_index(size_t ev_index)
  {
    m_ev_index = ev_index;
  }

  bool operator()(const event_visitor_settings& settings)
  {
    log_event("event_visitor_settings");

    if (settings.valid_mask & event_visitor_settings::set_txs_keeped_by_block)
    {
      m_txs_keeped_by_block = settings.txs_keeped_by_block;
    }

    return true;
  }

  bool operator()(const cryptonote::transaction& tx) const
  {
    log_event("cryptonote::transaction");

    cryptonote::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
    size_t pool_size = m_c.get_pool_transactions_count();
    m_c.handle_incoming_tx(t_serializable_object_to_blob(tx), tvc, m_txs_keeped_by_block, false, false);
    bool tx_added = pool_size + 1 == m_c.get_pool_transactions_count();
    bool r = check_tx_verification_context(tvc, tx_added, m_ev_index, tx, m_validator);
    CHECK_AND_NO_ASSERT_MES(r, false, "tx verification context check failed");
    return true;
  }

  bool operator()(const std::vector<cryptonote::transaction>& txs) const
  {
    log_event("cryptonote::transaction");

    std::vector<cryptonote::blobdata> tx_blobs;
    std::vector<cryptonote::tx_verification_context> tvcs;
     cryptonote::tx_verification_context tvc0 = AUTO_VAL_INIT(tvc0);
    for (const auto &tx: txs)
    {
      tx_blobs.push_back(t_serializable_object_to_blob(tx));
      tvcs.push_back(tvc0);
    }
    size_t pool_size = m_c.get_pool_transactions_count();
    m_c.handle_incoming_txs(tx_blobs, tvcs, m_txs_keeped_by_block, false, false);
    size_t tx_added = m_c.get_pool_transactions_count() - pool_size;
    bool r = check_tx_verification_context(tvcs, tx_added, m_ev_index, txs, m_validator);
    CHECK_AND_NO_ASSERT_MES(r, false, "tx verification context check failed");
    return true;
  }

  bool operator()(const cryptonote::block& b) const
  {
    log_event("cryptonote::block");

    cryptonote::block_verification_context bvc = AUTO_VAL_INIT(bvc);
    m_c.handle_incoming_block(t_serializable_object_to_blob(b), bvc);
    bool r = check_block_verification_context(bvc, m_ev_index, b, m_validator);
    CHECK_AND_NO_ASSERT_MES(r, false, "block verification context check failed");
    return r;
  }

  bool operator()(const callback_entry& cb) const
  {
    log_event(std::string("callback_entry ") + cb.callback_name);
    return m_validator.verify(cb.callback_name, m_c, m_ev_index, m_events);
  }

  bool operator()(const cryptonote::account_base& ab) const
  {
    log_event("cryptonote::account_base");
    return true;
  }

  bool operator()(const serialized_block& sr_block) const
  {
    log_event("serialized_block");

    cryptonote::block_verification_context bvc = AUTO_VAL_INIT(bvc);
    m_c.handle_incoming_block(sr_block.data, bvc);

    cryptonote::block blk;
    std::stringstream ss;
    ss << sr_block.data;
    binary_archive<false> ba(ss);
    ::serialization::serialize(ba, blk);
    if (!ss.good())
    {
      blk = cryptonote::block();
    }
    bool r = check_block_verification_context(bvc, m_ev_index, blk, m_validator);
    CHECK_AND_NO_ASSERT_MES(r, false, "block verification context check failed");
    return true;
  }

  bool operator()(const serialized_transaction& sr_tx) const
  {
    log_event("serialized_transaction");

    cryptonote::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
    size_t pool_size = m_c.get_pool_transactions_count();
    m_c.handle_incoming_tx(sr_tx.data, tvc, m_txs_keeped_by_block, false, false);
    bool tx_added = pool_size + 1 == m_c.get_pool_transactions_count();

    cryptonote::transaction tx;
    std::stringstream ss;
    ss << sr_tx.data;
    binary_archive<false> ba(ss);
    ::serialization::serialize(ba, tx);
    if (!ss.good())
    {
      tx = cryptonote::transaction();
    }

    bool r = check_tx_verification_context(tvc, tx_added, m_ev_index, tx, m_validator);
    CHECK_AND_NO_ASSERT_MES(r, false, "transaction verification context check failed");
    return true;
  }

private:
  void log_event(const std::string& event_type) const
  {
    MGINFO_YELLOW("=== EVENT # " << m_ev_index << ": " << event_type);
  }
};
//--------------------------------------------------------------------------
template<class t_test_class>
inline bool replay_events_through_core(cryptonote::core& cr, const std::vector<test_event_entry>& events, t_test_class& validator)
{
  TRY_ENTRY();

  //init core here

  CHECK_AND_ASSERT_MES(typeid(cryptonote::block) == events[0].type(), false, "First event must be genesis block creation");
  cr.set_genesis_block(boost::get<cryptonote::block>(events[0]));

  bool r = true;
  push_core_event_visitor<t_test_class> visitor(cr, events, validator);
  for(size_t i = 1; i < events.size() && r; ++i)
  {
    visitor.event_index(i);
    r = boost::apply_visitor(visitor, events[i]);
  }

  return r;

  CATCH_ENTRY_L0("replay_events_through_core", false);
}
//--------------------------------------------------------------------------
template<typename t_test_class>
struct get_test_options {
  const std::pair<uint8_t, uint64_t> hard_forks[2];
  const cryptonote::test_options test_options = {
    hard_forks
  };
  get_test_options():hard_forks{std::make_pair((uint8_t)1, (uint64_t)0), std::make_pair((uint8_t)0, (uint64_t)0)}{}
};

//--------------------------------------------------------------------------
template<class t_test_class>
inline bool do_replay_events(std::vector<test_event_entry>& events)
{
  boost::program_options::options_description desc("Allowed options");
  cryptonote::core::init_options(desc);
  boost::program_options::variables_map vm;
  bool r = command_line::handle_error_helper(desc, [&]()
  {
    boost::program_options::store(boost::program_options::basic_parsed_options<char>(&desc), vm);
    boost::program_options::notify(vm);
    return true;
  });
  if (!r)
    return false;

  cryptonote::cryptonote_protocol_stub pr; //TODO: stub only for this kind of test, make real validation of relayed objects
  cryptonote::core c(&pr);
  // FIXME: make sure that vm has arg_testnet_on set to true or false if
  // this test needs for it to be so.
  get_test_options<t_test_class> gto;
  if (!c.init(vm, &gto.test_options))
  {
    MERROR("Failed to init core");
    return false;
  }
  c.get_blockchain_storage().get_db().set_batch_transactions(true);

  // start with a clean pool
  std::vector<crypto::hash> pool_txs;
  if (!c.get_pool_transaction_hashes(pool_txs))
  {
    MERROR("Failed to flush txpool");
    return false;
  }
  c.get_blockchain_storage().flush_txes_from_pool(pool_txs);

  t_test_class validator;
  bool ret = replay_events_through_core<t_test_class>(c, events, validator);
  c.deinit();
  return ret;
}
//--------------------------------------------------------------------------
template<class t_test_class>
inline bool do_replay_file(const std::string& filename)
{
  std::vector<test_event_entry> events;
  if (!tools::unserialize_obj_from_file(events, filename))
  {
    MERROR("Failed to deserialize data from file: ");
    return false;
  }
  return do_replay_events<t_test_class>(events);
}

//--------------------------------------------------------------------------
#define GENERATE_ACCOUNT(account) \
    cryptonote::account_base account; \
    account.generate();

#define GENERATE_MULTISIG_ACCOUNT(account, threshold, total) \
    CHECK_AND_ASSERT_MES(threshold >= 2 && threshold <= total, false, "Invalid multisig scheme"); \
    std::vector<cryptonote::account_base> account(total); \
    do \
    { \
      for (size_t msidx = 0; msidx < total; ++msidx) \
        account[msidx].generate(); \
      make_multisig_accounts(account, threshold); \
    } while(0)

#define MAKE_ACCOUNT(VEC_EVENTS, account) \
  cryptonote::account_base account; \
  account.generate(); \
  VEC_EVENTS.push_back(account);

#define DO_CALLBACK(VEC_EVENTS, CB_NAME) \
{ \
  callback_entry CALLBACK_ENTRY; \
  CALLBACK_ENTRY.callback_name = CB_NAME; \
  VEC_EVENTS.push_back(CALLBACK_ENTRY); \
}

#define REGISTER_CALLBACK(CB_NAME, CLBACK) \
  register_callback(CB_NAME, boost::bind(&CLBACK, this, _1, _2, _3));

#define REGISTER_CALLBACK_METHOD(CLASS, METHOD) \
  register_callback(#METHOD, boost::bind(&CLASS::METHOD, this, _1, _2, _3));

#define MAKE_GENESIS_BLOCK(VEC_EVENTS, BLK_NAME, MINER_ACC, TS)                       \
  test_generator generator;                                                           \
  cryptonote::block BLK_NAME;                                                           \
  generator.construct_block(BLK_NAME, MINER_ACC, TS);                                 \
  VEC_EVENTS.push_back(BLK_NAME);

#define MAKE_NEXT_BLOCK(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC)                  \
  cryptonote::block BLK_NAME;                                                           \
  generator.construct_block(BLK_NAME, PREV_BLOCK, MINER_ACC);                         \
  VEC_EVENTS.push_back(BLK_NAME);

#define MAKE_NEXT_BLOCK_TX1(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, TX1)         \
  cryptonote::block BLK_NAME;                                                           \
  {                                                                                   \
    std::list<cryptonote::transaction> tx_list;                                         \
    tx_list.push_back(TX1);                                                           \
    generator.construct_block(BLK_NAME, PREV_BLOCK, MINER_ACC, tx_list);              \
  }                                                                                   \
  VEC_EVENTS.push_back(BLK_NAME);

#define MAKE_NEXT_BLOCK_TX_LIST(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, TXLIST)  \
  cryptonote::block BLK_NAME;                                                           \
  generator.construct_block(BLK_NAME, PREV_BLOCK, MINER_ACC, TXLIST);                 \
  VEC_EVENTS.push_back(BLK_NAME);

#define REWIND_BLOCKS_N(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, COUNT)           \
  cryptonote::block BLK_NAME;                                                           \
  {                                                                                   \
    cryptonote::block blk_last = PREV_BLOCK;                                            \
    for (size_t i = 0; i < COUNT; ++i)                                                \
    {                                                                                 \
      MAKE_NEXT_BLOCK(VEC_EVENTS, blk, blk_last, MINER_ACC);                          \
      blk_last = blk;                                                                 \
    }                                                                                 \
    BLK_NAME = blk_last;                                                              \
  }

#define REWIND_BLOCKS(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC) REWIND_BLOCKS_N(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW)

#define MAKE_TX_MIX(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, NMIX, HEAD)                       \
  cryptonote::transaction TX_NAME;                                                             \
  construct_tx_to_key(VEC_EVENTS, TX_NAME, HEAD, FROM, TO, AMOUNT, TESTS_DEFAULT_FEE, NMIX); \
  VEC_EVENTS.push_back(TX_NAME);

#define MAKE_TX(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, HEAD) MAKE_TX_MIX(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, 0, HEAD)

#define MAKE_TX_MIX_LIST(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, NMIX, HEAD)             \
  {                                                                                      \
    cryptonote::transaction t;                                                             \
    construct_tx_to_key(VEC_EVENTS, t, HEAD, FROM, TO, AMOUNT, TESTS_DEFAULT_FEE, NMIX); \
    SET_NAME.push_back(t);                                                               \
    VEC_EVENTS.push_back(t);                                                             \
  }

#define MAKE_TX_LIST(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, HEAD) MAKE_TX_MIX_LIST(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, 0, HEAD)

#define MAKE_TX_LIST_START(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, HEAD) \
    std::list<cryptonote::transaction> SET_NAME; \
    MAKE_TX_LIST(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, HEAD);

#define MAKE_MINER_TX_AND_KEY_MANUALLY(TX, BLK, KEY)                                                      \
  transaction TX;                                                                                         \
  if (!construct_miner_tx_manually(get_block_height(BLK) + 1, generator.get_already_generated_coins(BLK), \
    miner_account.get_keys().m_account_address, TX, 0, KEY))                                              \
    return false;

#define MAKE_MINER_TX_MANUALLY(TX, BLK) MAKE_MINER_TX_AND_KEY_MANUALLY(TX, BLK, 0)

#define SET_EVENT_VISITOR_SETT(VEC_EVENTS, SETT, VAL) VEC_EVENTS.push_back(event_visitor_settings(SETT, VAL));

#define GENERATE(filename, genclass) \
    { \
        std::vector<test_event_entry> events; \
        genclass g; \
        g.generate(events); \
        if (!tools::serialize_obj_to_file(events, filename)) \
        { \
            MERROR("Failed to serialize data to file: " << filename); \
            throw std::runtime_error("Failed to serialize data to file"); \
        } \
    }


#define PLAY(filename, genclass) \
    if(!do_replay_file<genclass>(filename)) \
    { \
      MERROR("Failed to pass test : " << #genclass); \
      return 1; \
    }

#define GENERATE_AND_PLAY(genclass)                                                                        \
  if (filter.empty() || boost::regex_match(std::string(#genclass), match, boost::regex(filter)))           \
  {                                                                                                        \
    std::vector<test_event_entry> events;                                                                  \
    ++tests_count;                                                                                         \
    bool generated = false;                                                                                \
    try                                                                                                    \
    {                                                                                                      \
      genclass g;                                                                                          \
      generated = g.generate(events);;                                                                     \
    }                                                                                                      \
    catch (const std::exception& ex)                                                                       \
    {                                                                                                      \
      MERROR(#genclass << " generation failed: what=" << ex.what());                                       \
    }                                                                                                      \
    catch (...)                                                                                            \
    {                                                                                                      \
      MERROR(#genclass << " generation failed: generic exception");                                        \
    }                                                                                                      \
    if (generated && do_replay_events< genclass >(events))                                                 \
    {                                                                                                      \
      MGINFO_GREEN("#TEST# Succeeded " << #genclass);                                                      \
    }                                                                                                      \
    else                                                                                                   \
    {                                                                                                      \
      MERROR("#TEST# Failed " << #genclass);                                                               \
      failed_tests.push_back(#genclass);                                                                   \
    }                                                                                                      \
  }

#define CALL_TEST(test_name, function)                                                                     \
  {                                                                                                        \
    if(!function())                                                                                        \
    {                                                                                                      \
      MERROR("#TEST# Failed " << test_name);                                                               \
      return 1;                                                                                            \
    }                                                                                                      \
    else                                                                                                   \
    {                                                                                                      \
      MGINFO_GREEN("#TEST# Succeeded " << test_name);                                                      \
    }                                                                                                      \
  }

#define QUOTEME(x) #x
#define DEFINE_TESTS_ERROR_CONTEXT(text) const char* perr_context = text;
#define CHECK_TEST_CONDITION(cond) CHECK_AND_ASSERT_MES(cond, false, "[" << perr_context << "] failed: \"" << QUOTEME(cond) << "\"")
#define CHECK_EQ(v1, v2) CHECK_AND_ASSERT_MES(v1 == v2, false, "[" << perr_context << "] failed: \"" << QUOTEME(v1) << " == " << QUOTEME(v2) << "\", " << v1 << " != " << v2)
#define CHECK_NOT_EQ(v1, v2) CHECK_AND_ASSERT_MES(!(v1 == v2), false, "[" << perr_context << "] failed: \"" << QUOTEME(v1) << " != " << QUOTEME(v2) << "\", " << v1 << " == " << v2)
#define MK_COINS(amount) (UINT64_C(amount) * COIN)
#define TESTS_DEFAULT_FEE ((uint64_t)20000000000) // 2 * pow(10, 10)
