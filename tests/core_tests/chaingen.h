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

#include <functional>
#include <vector>
#include <iostream>
#include <stdint.h>

#include <boost/program_options.hpp>
#include <boost/optional.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/serialization/optional.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/functional/hash.hpp>

#include "include_base_utils.h"
#include "chaingen_serialization.h"
#include "common/command_line.h"
#include "common/threadpool.h"

#include "cryptonote_basic/account_boost_serialization.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_protocol/enums.h"
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
  int mask;

  enum settings
  {
    set_txs_keeped_by_block = 1 << 0,
    set_txs_do_not_relay = 1 << 1,
    set_local_relay = 1 << 2,
    set_txs_stem = 1 << 3
  };

  event_visitor_settings(int a_mask = 0)
    : mask(a_mask)
  {
  }

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/)
  {
    ar & mask;
  }
};

typedef std::vector<std::pair<uint8_t, uint64_t>> v_hardforks_t;
struct event_replay_settings
{
  boost::optional<v_hardforks_t> hard_forks;

  event_replay_settings() = default;

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/)
  {
    ar & hard_forks;
  }
};


VARIANT_TAG(binary_archive, callback_entry, 0xcb);
VARIANT_TAG(binary_archive, cryptonote::account_base, 0xcc);
VARIANT_TAG(binary_archive, serialized_block, 0xcd);
VARIANT_TAG(binary_archive, serialized_transaction, 0xce);
VARIANT_TAG(binary_archive, event_visitor_settings, 0xcf);
VARIANT_TAG(binary_archive, event_replay_settings, 0xda);

typedef boost::variant<cryptonote::block, cryptonote::transaction, std::vector<cryptonote::transaction>, cryptonote::account_base, callback_entry, serialized_block, serialized_transaction, event_visitor_settings, event_replay_settings> test_event_entry;
typedef std::unordered_map<crypto::hash, const cryptonote::transaction*> map_hash2tx_t;

class test_chain_unit_base
{
public:
  typedef boost::function<bool (cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events)> verify_callback;
  typedef std::map<std::string, verify_callback> callbacks_map;

  void register_callback(const std::string& cb_name, verify_callback cb);
  bool verify(const std::string& cb_name, cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
  bool check_block_verification_context(const cryptonote::block_verification_context& bvc, size_t event_idx, const cryptonote::block& /*blk*/);
  bool check_tx_verification_context(const cryptonote::tx_verification_context& tvc, bool /*tx_added*/, size_t /*event_index*/, const cryptonote::transaction& /*tx*/);
  bool check_tx_verification_context_array(const std::vector<cryptonote::tx_verification_context>& tvcs, size_t /*tx_added*/, size_t /*event_index*/, const std::vector<cryptonote::transaction>& /*txs*/);

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

  private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /*version*/)
    {
      ar & prev_id;
      ar & already_generated_coins;
      ar & block_weight;
    }
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
    bf_hf_version= 1 << 8,
    bf_tx_fees   = 1 << 9
  };

  test_generator(): m_events(nullptr) {}
  test_generator(const test_generator &other): m_blocks_info(other.m_blocks_info), m_events(other.m_events), m_nettype(other.m_nettype) {}
  void get_block_chain(std::vector<block_info>& blockchain, const crypto::hash& head, size_t n) const;
  void get_last_n_block_weights(std::vector<size_t>& block_weights, const crypto::hash& head, size_t n) const;
  uint64_t get_already_generated_coins(const crypto::hash& blk_id) const;
  uint64_t get_already_generated_coins(const cryptonote::block& blk) const;

  void add_block(const cryptonote::block& blk, size_t tsx_size, std::vector<size_t>& block_weights, uint64_t already_generated_coins, uint64_t block_reward,
    uint8_t hf_version = 1);
  bool construct_block(cryptonote::block& blk, uint64_t height, const crypto::hash& prev_id,
    const cryptonote::account_base& miner_acc, uint64_t timestamp, uint64_t already_generated_coins,
    std::vector<size_t>& block_weights, const std::list<cryptonote::transaction>& tx_list,
    const boost::optional<uint8_t>& hf_ver = boost::none);
  bool construct_block(cryptonote::block& blk, const cryptonote::account_base& miner_acc, uint64_t timestamp);
  bool construct_block(cryptonote::block& blk, const cryptonote::block& blk_prev, const cryptonote::account_base& miner_acc,
    const std::list<cryptonote::transaction>& tx_list = std::list<cryptonote::transaction>(),
    const boost::optional<uint8_t>& hf_ver = boost::none);

  bool construct_block_manually(cryptonote::block& blk, const cryptonote::block& prev_block,
    const cryptonote::account_base& miner_acc, int actual_params = bf_none, uint8_t major_ver = 0,
    uint8_t minor_ver = 0, uint64_t timestamp = 0, const crypto::hash& prev_id = crypto::hash(),
    const cryptonote::difficulty_type& diffic = 1, const cryptonote::transaction& miner_tx = cryptonote::transaction(),
    const std::vector<crypto::hash>& tx_hashes = std::vector<crypto::hash>(), size_t txs_sizes = 0, size_t max_outs = 999,
    uint8_t hf_version = 1, uint64_t fees = 0, const uint8_t fcmp_pp_n_tree_layres = 0,
    const crypto::ec_point& fcmp_pp_tree_root = crypto::ec_point{});
  bool construct_block_manually_tx(cryptonote::block& blk, const cryptonote::block& prev_block,
    const cryptonote::account_base& miner_acc, const std::vector<crypto::hash>& tx_hashes, size_t txs_size);
  void fill_nonce(cryptonote::block& blk, const cryptonote::difficulty_type& diffic, uint64_t height);
  void set_events(const std::vector<test_event_entry> * events) { m_events = events; }
  void set_network_type(const cryptonote::network_type nettype) { m_nettype = nettype; }

private:
  std::unordered_map<crypto::hash, block_info> m_blocks_info;
  const std::vector<test_event_entry> * m_events;
  cryptonote::network_type m_nettype;

  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/)
  {
    ar & m_blocks_info;
  }
};

template<typename T>
std::string dump_keys(T * buff32)
{
  std::ostringstream ss;
  char buff[10];

  ss << "[";
  for(int i = 0; i < 32; i++)
  {
    snprintf(buff, 10, "0x%02x", ((uint8_t)buff32[i] & 0xff));
    ss << buff;
    if (i < 31)
      ss << ",";
  }
  ss << "]";
  return ss.str();
}

struct output_index {
  const cryptonote::txout_target_v out;
  uint64_t amount;
  size_t blk_height; // block height
  size_t tx_no; // index of transaction in block
  size_t out_no; // index of out in transaction
  size_t idx;
  uint64_t unlock_time;
  bool is_coin_base;
  bool spent;
  bool rct;
  rct::key comm;
  const cryptonote::block *p_blk;
  const cryptonote::transaction *p_tx;

  output_index(const cryptonote::txout_target_v &_out, uint64_t _a, size_t _h, size_t tno, size_t ono, const cryptonote::block *_pb, const cryptonote::transaction *_pt)
      : out(_out), amount(_a), blk_height(_h), tx_no(tno), out_no(ono), idx(0), unlock_time(0),
      is_coin_base(false), spent(false), rct(false), p_blk(_pb), p_tx(_pt)
  {

  }

  output_index(const output_index &other)
      : out(other.out), amount(other.amount), blk_height(other.blk_height), tx_no(other.tx_no), rct(other.rct),
      out_no(other.out_no), idx(other.idx), unlock_time(other.unlock_time), is_coin_base(other.is_coin_base),
      spent(other.spent), comm(other.comm), p_blk(other.p_blk), p_tx(other.p_tx) {  }

  void set_rct(bool arct) {
    rct = arct;
    if (rct &&  p_tx->rct_signatures.outPk.size() > out_no)
      comm = p_tx->rct_signatures.outPk[out_no].mask;
    else
      comm = rct::commit(amount, rct::identity());
  }

  rct::key commitment() const {
    return comm;
  }

  const std::string toString() const {
    std::stringstream ss;

    ss << "output_index{blk_height=" << blk_height
       << " tx_no=" << tx_no
       << " out_no=" << out_no
       << " amount=" << amount
       << " idx=" << idx
       << " unlock_time=" << unlock_time
       << " spent=" << spent
       << " is_coin_base=" << is_coin_base
       << " rct=" << rct
       << " comm=" << dump_keys(comm.bytes)
       << "}";

    return ss.str();
  }

  output_index& operator=(const output_index& other)
  {
    new(this) output_index(other);
    return *this;
  }
};

typedef std::tuple<uint64_t, crypto::public_key, rct::key> get_outs_entry;
typedef std::pair<crypto::hash, size_t> output_hasher;
typedef boost::hash<output_hasher> output_hasher_hasher;
typedef std::map<uint64_t, std::vector<size_t> > map_output_t;
typedef std::map<uint64_t, std::vector<output_index> > map_output_idx_t;
typedef std::unordered_map<crypto::hash, cryptonote::block> map_block_t;
typedef std::unordered_map<output_hasher, output_index, output_hasher_hasher> map_txid_output_t;
typedef std::unordered_map<crypto::public_key, cryptonote::subaddress_index> subaddresses_t;
typedef std::pair<uint64_t, size_t>  outloc_t;

typedef boost::variant<cryptonote::account_public_address, cryptonote::account_keys, cryptonote::account_base, cryptonote::tx_destination_entry> var_addr_t;
typedef struct {
  const var_addr_t addr;
  bool is_subaddr;
  uint64_t amount;
} dest_wrapper_t;

typedef struct {
  const output_index &oi;
  uint64_t cur_height;
} fnc_accept_output_crate_t;

typedef std::function<bool(const fnc_accept_output_crate_t &info)> fnc_accept_output_t;

// Daemon functionality
class block_tracker
{
public:
  map_output_idx_t m_outs;
  map_txid_output_t m_map_outs;  // mapping (txid, out) -> output_index
  map_block_t m_blocks;

  block_tracker() = default;
  block_tracker(const block_tracker &bt): m_outs(bt.m_outs), m_map_outs(bt.m_map_outs), m_blocks(bt.m_blocks) {};
  map_txid_output_t::iterator find_out(const crypto::hash &txid, size_t out);
  map_txid_output_t::iterator find_out(const output_hasher &id);
  void process(const std::vector<cryptonote::block>& blockchain, const map_hash2tx_t& mtx);
  void process(const std::vector<const cryptonote::block*>& blockchain, const map_hash2tx_t& mtx);
  void process(const cryptonote::block* blk, const cryptonote::transaction * tx, size_t i);
  void global_indices(const cryptonote::transaction *tx, std::vector<uint64_t> &indices);
  void get_fake_outs(size_t num_outs, uint64_t amount, uint64_t global_index, uint64_t cur_height, std::vector<get_outs_entry> &outs, const boost::optional<fnc_accept_output_t>& fnc_accept = boost::none);

  std::string dump_data();
  void dump_data(const std::string & fname);

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/)
  {
    ar & m_outs;
    ar & m_map_outs;
    ar & m_blocks;
  }
};

class tx_construct_error : public std::runtime_error
{
public:
  tx_construct_error(const char *s) : runtime_error(s) { }
};

class tx_construct_tx_fill_error : public tx_construct_error
{
public:
  tx_construct_tx_fill_error() : tx_construct_error("Couldn't fill transaction sources") { }
  tx_construct_tx_fill_error(const char *s) : tx_construct_error(s) { }
};

std::string dump_data(const cryptonote::transaction &tx);
cryptonote::account_public_address get_address(const var_addr_t& inp);
cryptonote::account_public_address get_address(const cryptonote::account_public_address& inp);
cryptonote::account_public_address get_address(const cryptonote::account_keys& inp);
cryptonote::account_public_address get_address(const cryptonote::account_base& inp);
cryptonote::account_public_address get_address(const cryptonote::tx_destination_entry& inp);

inline cryptonote::difficulty_type get_test_difficulty(const boost::optional<uint8_t>& hf_ver=boost::none) {return !hf_ver || hf_ver.get() <= 1 ? 1 : 2;}
inline uint64_t current_difficulty_window(const boost::optional<uint8_t>& hf_ver=boost::none){ return !hf_ver || hf_ver.get() <= 1 ? DIFFICULTY_TARGET_V1 : DIFFICULTY_TARGET_V2; }

cryptonote::tx_destination_entry build_dst(const var_addr_t& to, bool is_subaddr=false, uint64_t amount=0);
std::vector<cryptonote::tx_destination_entry> build_dsts(const var_addr_t& to1, bool sub1=false, uint64_t am1=0, size_t repeat=1);
std::vector<cryptonote::tx_destination_entry> build_dsts(std::initializer_list<dest_wrapper_t> inps);
uint64_t sum_amount(const std::vector<cryptonote::tx_destination_entry>& destinations);
uint64_t sum_amount(const std::vector<cryptonote::tx_source_entry>& sources);

bool construct_miner_tx_manually(size_t height, uint64_t already_generated_coins,
                                 const cryptonote::account_public_address& miner_address, cryptonote::transaction& tx,
                                 uint64_t fee, uint8_t hf_version = 1,
                                 cryptonote::keypair* p_txkey = nullptr);

bool construct_tx_to_key(const std::vector<test_event_entry>& events, cryptonote::transaction& tx,
                         const cryptonote::block& blk_head, const cryptonote::account_base& from, const var_addr_t& to, uint64_t amount,
                         uint64_t fee, size_t nmix, bool rct=false, rct::RangeProofType range_proof_type=rct::RangeProofBorromean, int bp_version = 0,
                         bool check_unlock_time = true, const boost::optional<fnc_accept_output_t>& fnc_tx_in_accept = boost::none);

bool construct_tx_to_key(const std::vector<test_event_entry>& events, cryptonote::transaction& tx, const cryptonote::block& blk_head,
                         const cryptonote::account_base& from, const std::vector<cryptonote::tx_destination_entry>& destinations,
                         uint64_t fee, size_t nmix, bool rct=false, rct::RangeProofType range_proof_type=rct::RangeProofBorromean, int bp_version = 0,
                         bool check_unlock_time = true, const boost::optional<fnc_accept_output_t>& fnc_tx_in_accept = boost::none);

bool construct_tx_to_key(cryptonote::transaction& tx, const cryptonote::account_base& from, const var_addr_t& to, uint64_t amount,
                         std::vector<cryptonote::tx_source_entry> &sources,
                         uint64_t fee, bool rct=false, rct::RangeProofType range_proof_type=rct::RangeProofBorromean, int bp_version = 0);

bool construct_tx_to_key(cryptonote::transaction& tx, const cryptonote::account_base& from, const std::vector<cryptonote::tx_destination_entry>& destinations,
                         std::vector<cryptonote::tx_source_entry> &sources,
                         uint64_t fee, bool rct, rct::RangeProofType range_proof_type, int bp_version = 0);

cryptonote::transaction construct_tx_with_fee(std::vector<test_event_entry>& events, const cryptonote::block& blk_head,
                                            const cryptonote::account_base& acc_from, const var_addr_t& to,
                                            uint64_t amount, uint64_t fee);

bool construct_tx_rct(const cryptonote::account_keys& sender_account_keys,
    std::vector<cryptonote::tx_source_entry>& sources,
    const std::vector<cryptonote::tx_destination_entry>& destinations,
    const boost::optional<cryptonote::account_public_address>& change_addr,
    std::vector<uint8_t> extra, cryptonote::transaction& tx,
    bool rct=false, rct::RangeProofType range_proof_type=rct::RangeProofBorromean, int bp_version = 0);


uint64_t num_blocks(const std::vector<test_event_entry>& events);
cryptonote::block get_head_block(const std::vector<test_event_entry>& events);

void get_confirmed_txs(const std::vector<cryptonote::block>& blockchain, const map_hash2tx_t& mtx, map_hash2tx_t& confirmed_txs);
bool trim_block_chain(std::vector<cryptonote::block>& blockchain, const crypto::hash& tail);
bool trim_block_chain(std::vector<const cryptonote::block*>& blockchain, const crypto::hash& tail);
bool find_block_chain(const std::vector<test_event_entry>& events, std::vector<cryptonote::block>& blockchain, map_hash2tx_t& mtx, const crypto::hash& head);
bool find_block_chain(const std::vector<test_event_entry>& events, std::vector<const cryptonote::block*>& blockchain, map_hash2tx_t& mtx, const crypto::hash& head);

bool fill_tx_sources(std::vector<cryptonote::tx_source_entry>& sources, const std::vector<test_event_entry>& events,
                     const cryptonote::block& blk_head, const cryptonote::account_base& from, uint64_t amount, size_t nmix,
                     bool check_unlock_time = true, const boost::optional<fnc_accept_output_t>& fnc_accept = boost::none);

void fill_tx_destinations(const var_addr_t& from, const cryptonote::account_public_address& to,
                          uint64_t amount, uint64_t fee,
                          const std::vector<cryptonote::tx_source_entry> &sources,
                          std::vector<cryptonote::tx_destination_entry>& destinations, bool always_change=false);

void fill_tx_destinations(const var_addr_t& from, const std::vector<cryptonote::tx_destination_entry>& dests,
                          uint64_t fee,
                          const std::vector<cryptonote::tx_source_entry> &sources,
                          std::vector<cryptonote::tx_destination_entry>& destinations,
                          bool always_change);

void fill_tx_destinations(const var_addr_t& from, const cryptonote::account_public_address& to,
                          uint64_t amount, uint64_t fee,
                          const std::vector<cryptonote::tx_source_entry> &sources,
                          std::vector<cryptonote::tx_destination_entry>& destinations,
                          std::vector<cryptonote::tx_destination_entry>& destinations_pure,
                          bool always_change=false);


void fill_tx_sources_and_destinations(const std::vector<test_event_entry>& events, const cryptonote::block& blk_head,
                                      const cryptonote::account_base& from, const cryptonote::account_public_address& to,
                                      uint64_t amount, uint64_t fee, size_t nmix,
                                      std::vector<cryptonote::tx_source_entry>& sources,
                                      std::vector<cryptonote::tx_destination_entry>& destinations,
                                      bool check_unlock_time = true,
                                      const boost::optional<fnc_accept_output_t>& fnc_tx_in_accept = boost::none);

void fill_tx_sources_and_destinations(const std::vector<test_event_entry>& events, const cryptonote::block& blk_head,
                                      const cryptonote::account_base& from, const cryptonote::account_base& to,
                                      uint64_t amount, uint64_t fee, size_t nmix,
                                      std::vector<cryptonote::tx_source_entry>& sources,
                                      std::vector<cryptonote::tx_destination_entry>& destinations,
                                      bool check_unlock_time = true,
                                      const boost::optional<fnc_accept_output_t>& fnc_tx_in_accept = boost::none);

uint64_t get_balance(const cryptonote::account_base& addr, const std::vector<cryptonote::block>& blockchain, const map_hash2tx_t& mtx);

bool extract_hard_forks(const std::vector<test_event_entry>& events, v_hardforks_t& hard_forks);
bool extract_hard_forks_from_blocks(const std::vector<test_event_entry>& events, v_hardforks_t& hard_forks);

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

  cryptonote::relay_method m_tx_relay;

public:
  push_core_event_visitor(cryptonote::core& c, const std::vector<test_event_entry>& events, t_test_class& validator)
    : m_c(c)
    , m_events(events)
    , m_validator(validator)
    , m_ev_index(0)
    , m_tx_relay(cryptonote::relay_method::fluff)
  {
  }

  void event_index(size_t ev_index)
  {
    m_ev_index = ev_index;
  }

  bool operator()(const event_replay_settings& settings)
  {
    log_event("event_replay_settings");
    return true;
  }

  bool operator()(const event_visitor_settings& settings)
  {
    log_event("event_visitor_settings");

    if (settings.mask & event_visitor_settings::set_txs_keeped_by_block)
    {
      m_tx_relay = cryptonote::relay_method::block;
    }
    else if (settings.mask & event_visitor_settings::set_local_relay)
    {
      m_tx_relay = cryptonote::relay_method::local;
    }
    else if (settings.mask & event_visitor_settings::set_txs_do_not_relay)
    {
      m_tx_relay = cryptonote::relay_method::none;
    }
    else if (settings.mask & event_visitor_settings::set_txs_stem)
    {
      m_tx_relay = cryptonote::relay_method::stem;
    }
    else
    {
      m_tx_relay = cryptonote::relay_method::fluff;
    }

    return true;
  }

  bool operator()(const cryptonote::transaction& tx) const
  {
    log_event("cryptonote::transaction");

    cryptonote::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
    size_t pool_size = m_c.get_pool_transactions_count();
    m_c.handle_incoming_tx(t_serializable_object_to_blob(tx), tvc, m_tx_relay, false);
    bool tx_added = pool_size + 1 == m_c.get_pool_transactions_count();
    bool r = m_validator.check_tx_verification_context(tvc, tx_added, m_ev_index, tx);
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
      tx_blobs.emplace_back(t_serializable_object_to_blob(tx));
      tvcs.push_back(tvc0);
    }
    size_t pool_size = m_c.get_pool_transactions_count();
    for (size_t i = 0; i < tx_blobs.size(); ++i)
      m_c.handle_incoming_tx(tx_blobs[i], tvcs[i], m_tx_relay, false);
    size_t tx_added = m_c.get_pool_transactions_count() - pool_size;
    bool r = m_validator.check_tx_verification_context_array(tvcs, tx_added, m_ev_index, txs);
    CHECK_AND_NO_ASSERT_MES(r, false, "tx verification context check failed");
    return true;
  }

  bool operator()(const cryptonote::block& b) const
  {
    log_event("cryptonote::block");

    cryptonote::block_verification_context bvc = AUTO_VAL_INIT(bvc);
    cryptonote::blobdata bd = t_serializable_object_to_blob(b);
    std::vector<cryptonote::block> pblocks;
    cryptonote::block_complete_entry bce;
    bce.pruned = false;
    bce.block = bd;
    bce.txs = {};
    if (m_c.prepare_handle_incoming_blocks(std::vector<cryptonote::block_complete_entry>(1, bce), pblocks))
    {
      m_c.handle_incoming_block(bd, &b, bvc);
      m_c.cleanup_handle_incoming_blocks();
    }
    else
      bvc.m_verifivation_failed = true;
    bool r = m_validator.check_block_verification_context(bvc, m_ev_index, b);
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
    std::vector<cryptonote::block> pblocks;
    cryptonote::block_complete_entry bce;
    bce.pruned = false;
    bce.block = sr_block.data;
    bce.txs = {};
    if (m_c.prepare_handle_incoming_blocks(std::vector<cryptonote::block_complete_entry>(1, bce), pblocks))
    {
      m_c.handle_incoming_block(sr_block.data, NULL, bvc);
      m_c.cleanup_handle_incoming_blocks();
    }
    else
      bvc.m_verifivation_failed = true;

    cryptonote::block blk;
    binary_archive<false> ba{epee::strspan<std::uint8_t>(sr_block.data)};
    ::serialization::serialize(ba, blk);
    if (!ba.good())
    {
      blk = cryptonote::block();
    }
    bool r = m_validator.check_block_verification_context(bvc, m_ev_index, blk);
    CHECK_AND_NO_ASSERT_MES(r, false, "block verification context check failed");
    return true;
  }

  bool operator()(const serialized_transaction& sr_tx) const
  {
    log_event("serialized_transaction");

    cryptonote::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
    size_t pool_size = m_c.get_pool_transactions_count();
    m_c.handle_incoming_tx(sr_tx.data, tvc, m_tx_relay, false);
    bool tx_added = pool_size + 1 == m_c.get_pool_transactions_count();

    cryptonote::transaction tx;
    binary_archive<false> ba{epee::strspan<std::uint8_t>(sr_tx.data)};
    ::serialization::serialize(ba, tx);
    if (!ba.good())
    {
      tx = cryptonote::transaction();
    }

    bool r = m_validator.check_tx_verification_context(tvc, tx_added, m_ev_index, tx);
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
  return replay_events_through_core_plain(cr, events, validator, true);
}
//--------------------------------------------------------------------------
template<class t_test_class>
inline bool replay_events_through_core_plain(cryptonote::core& cr, const std::vector<test_event_entry>& events, t_test_class& validator, bool reinit=true)
{
  TRY_ENTRY();

  //init core here
  if (reinit) {
    CHECK_AND_ASSERT_MES(typeid(cryptonote::block) == events[0].type(), false,
                         "First event must be genesis block creation");
    cr.set_genesis_block(boost::get<cryptonote::block>(events[0]));
  }

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
    hard_forks, 0
  };
  get_test_options():hard_forks{std::make_pair((uint8_t)1, (uint64_t)0), std::make_pair((uint8_t)0, (uint64_t)0)}{}
};
//--------------------------------------------------------------------------
template<class t_test_class>
inline bool do_replay_events_get_core(std::vector<test_event_entry>& events, cryptonote::core *core)
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

  auto & c = *core;

  // FIXME: make sure that vm has arg_testnet_on set to true or false if
  // this test needs for it to be so.
  get_test_options<t_test_class> gto;

  // Hardforks can be specified in events.
  v_hardforks_t hardforks;
  cryptonote::test_options test_options_tmp{nullptr, 0};
  const cryptonote::test_options * test_options_ = &gto.test_options;
  if (extract_hard_forks(events, hardforks)){
    hardforks.push_back(std::make_pair((uint8_t)0, (uint64_t)0));  // terminator
    test_options_tmp.hard_forks = hardforks.data();
    test_options_ = &test_options_tmp;
  }

  if (!c.init(vm, test_options_))
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
  tools::threadpool::getInstanceForCompute().recycle();
//  c.deinit();
  return ret;
}
//--------------------------------------------------------------------------
template<class t_test_class>
inline bool replay_events_through_core_validate(std::vector<test_event_entry>& events, cryptonote::core & c)
{
  std::vector<crypto::hash> pool_txs;
  if (!c.get_pool_transaction_hashes(pool_txs))
  {
    MERROR("Failed to flush txpool");
    return false;
  }
  c.get_blockchain_storage().flush_txes_from_pool(pool_txs);

  t_test_class validator;
  return replay_events_through_core_plain<t_test_class>(c, events, validator, false);
}
//--------------------------------------------------------------------------
template<class t_test_class>
inline bool do_replay_events(std::vector<test_event_entry>& events)
{
  cryptonote::core core(nullptr);
  bool ret = do_replay_events_get_core<t_test_class>(events, &core);
  core.deinit();
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
#define DEFAULT_HARDFORKS(HARDFORKS) do { \
  HARDFORKS.push_back(std::make_pair((uint8_t)1, (uint64_t)0)); \
} while(0)

#define ADD_HARDFORK(HARDFORKS, FORK, HEIGHT) HARDFORKS.push_back(std::make_pair((uint8_t)FORK, (uint64_t)HEIGHT))

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
      CHECK_AND_ASSERT_MES(make_multisig_accounts(account, threshold), false, "Failed to make multisig accounts."); \
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
  register_callback(CB_NAME, std::bind(&CLBACK, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

#define REGISTER_CALLBACK_METHOD(CLASS, METHOD) \
  register_callback(#METHOD, std::bind(&CLASS::METHOD, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

#define MAKE_GENESIS_BLOCK(VEC_EVENTS, BLK_NAME, MINER_ACC, TS)                       \
  test_generator generator;                                                           \
  cryptonote::block BLK_NAME;                                                           \
  generator.construct_block(BLK_NAME, MINER_ACC, TS);                                 \
  VEC_EVENTS.push_back(BLK_NAME);

#define MAKE_NEXT_BLOCK(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC)                  \
  cryptonote::block BLK_NAME;                                                           \
  generator.construct_block(BLK_NAME, PREV_BLOCK, MINER_ACC);                         \
  VEC_EVENTS.push_back(BLK_NAME);

#define MAKE_NEXT_BLOCK_HF(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, HF)           \
  cryptonote::block BLK_NAME;                                                         \
  generator.construct_block(BLK_NAME, PREV_BLOCK, MINER_ACC, std::list<cryptonote::transaction>(), HF);                     \
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

#define MAKE_NEXT_BLOCK_TX_LIST_HF(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, TXLIST, HF)  \
  cryptonote::block BLK_NAME;                                                           \
  generator.construct_block(BLK_NAME, PREV_BLOCK, MINER_ACC, TXLIST, HF);                 \
  VEC_EVENTS.push_back(BLK_NAME);

#define REWIND_BLOCKS_N_HF(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, COUNT, HF)    \
  cryptonote::block BLK_NAME;                                                         \
  {                                                                                   \
    cryptonote::block blk_last = PREV_BLOCK;                                          \
    for (size_t i = 0; i < COUNT; ++i)                                                \
    {                                                                                 \
      MAKE_NEXT_BLOCK_HF(VEC_EVENTS, blk, blk_last, MINER_ACC, HF);                   \
      blk_last = blk;                                                                 \
    }                                                                                 \
    BLK_NAME = blk_last;                                                              \
  }

#define REWIND_BLOCKS_N(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, COUNT) REWIND_BLOCKS_N_HF(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, COUNT, boost::none)
#define REWIND_BLOCKS(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC) REWIND_BLOCKS_N(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW)

#define MAKE_TX_MIX(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, NMIX, HEAD)                       \
  cryptonote::transaction TX_NAME;                                                             \
  construct_tx_to_key(VEC_EVENTS, TX_NAME, HEAD, FROM, TO, AMOUNT, TESTS_DEFAULT_FEE, NMIX); \
  VEC_EVENTS.push_back(TX_NAME);

#define MAKE_TX(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, HEAD) MAKE_TX_MIX(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, 0, HEAD)

#define MAKE_TX_MIX_LIST(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, NMIX, HEAD)             \
  {                                                                                      \
    cryptonote::transaction t;                                                           \
    construct_tx_to_key(VEC_EVENTS, t, HEAD, FROM, TO, AMOUNT, TESTS_DEFAULT_FEE, NMIX); \
    SET_NAME.push_back(t);                                                               \
    VEC_EVENTS.push_back(t);                                                             \
  }

#define MAKE_TX_LIST(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, HEAD) MAKE_TX_MIX_LIST(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, 0, HEAD)

#define MAKE_TX_LIST_START(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, HEAD) \
    std::list<cryptonote::transaction> SET_NAME; \
    MAKE_TX_LIST(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, HEAD);

#define MAKE_MINER_TX_AND_KEY_AT_HF_MANUALLY(TX, BLK, HF_VERSION, KEY)                                    \
  transaction TX;                                                                                         \
  if (!construct_miner_tx_manually(get_block_height(BLK) + 1, generator.get_already_generated_coins(BLK), \
    miner_account.get_keys().m_account_address, TX, 0, HF_VERSION, KEY))                                  \
    return false;

#define MAKE_MINER_TX_AND_KEY_MANUALLY(TX, BLK, KEY) MAKE_MINER_TX_AND_KEY_AT_HF_MANUALLY(TX, BLK, 1, KEY)

#define MAKE_MINER_TX_MANUALLY(TX, BLK) MAKE_MINER_TX_AND_KEY_MANUALLY(TX, BLK, 0)

#define SET_EVENT_VISITOR_SETT(VEC_EVENTS, SETT) VEC_EVENTS.push_back(event_visitor_settings(SETT));

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

#define CATCH_REPLAY(genclass)                                                                             \
    catch (const std::exception& ex)                                                                       \
    {                                                                                                      \
      MERROR(#genclass << " generation failed: what=" << ex.what());                                       \
    }                                                                                                      \
    catch (...)                                                                                            \
    {                                                                                                      \
      MERROR(#genclass << " generation failed: generic exception");                                        \
    }

#define REPLAY_CORE(genclass)                                                                              \
    if (generated && do_replay_events< genclass >(events))                                                 \
    {                                                                                                      \
      MGINFO_GREEN("#TEST# Succeeded " << #genclass);                                                      \
    }                                                                                                      \
    else                                                                                                   \
    {                                                                                                      \
      MERROR("#TEST# Failed " << #genclass);                                                               \
      failed_tests.push_back(#genclass);                                                                   \
    }

#define REPLAY_WITH_CORE(genclass, CORE)                                                                   \
    if (generated && replay_events_through_core_validate< genclass >(events, CORE))                        \
    {                                                                                                      \
      MGINFO_GREEN("#TEST# Succeeded " << #genclass);                                                      \
    }                                                                                                      \
    else                                                                                                   \
    {                                                                                                      \
      MERROR("#TEST# Failed " << #genclass);                                                               \
      failed_tests.push_back(#genclass);                                                                   \
    }

#define CATCH_GENERATE_REPLAY(genclass)                                                                    \
    CATCH_REPLAY(genclass);                                                                                \
    REPLAY_CORE(genclass);

#define CATCH_GENERATE_REPLAY_CORE(genclass, CORE)                                                         \
    CATCH_REPLAY(genclass);                                                                                \
    REPLAY_WITH_CORE(genclass, CORE);

#define GENERATE_AND_PLAY(genclass) \
  if (list_tests)                                                                                          \
    std::cout << #genclass << std::endl;                                                                   \
  else if (filter.empty() || boost::regex_match(std::string(#genclass), match, boost::regex(filter)))      \
  {                                                                                                        \
    std::vector<test_event_entry> events;                                                                  \
    ++tests_count;                                                                                         \
    bool generated = false;                                                                                \
    try                                                                                                    \
    {                                                                                                      \
      genclass g;                                                                                          \
      generated = g.generate(events);                                                                      \
    }                                                                                                      \
    CATCH_GENERATE_REPLAY(genclass);                                                                       \
  }

#define GENERATE_AND_PLAY_INSTANCE(genclass, ins, CORE)                                                    \
  if (filter.empty() || boost::regex_match(std::string(#genclass), match, boost::regex(filter)))           \
  {                                                                                                        \
    std::vector<test_event_entry> events;                                                                  \
    ++tests_count;                                                                                         \
    bool generated = false;                                                                                \
    try                                                                                                    \
    {                                                                                                      \
      generated = ins.generate(events);                                                                    \
    }                                                                                                      \
    CATCH_GENERATE_REPLAY_CORE(genclass, CORE);                                                            \
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
#define DEFINE_TESTS_ERROR_CONTEXT(text) const char* perr_context = text; (void) perr_context;
#define CHECK_TEST_CONDITION(cond) CHECK_AND_ASSERT_MES(cond, false, "[" << perr_context << "] failed: \"" << QUOTEME(cond) << "\"")
#define CHECK_EQ(v1, v2) CHECK_AND_ASSERT_MES(v1 == v2, false, "[" << perr_context << "] failed: \"" << QUOTEME(v1) << " == " << QUOTEME(v2) << "\", " << v1 << " != " << v2)
#define CHECK_NOT_EQ(v1, v2) CHECK_AND_ASSERT_MES(!(v1 == v2), false, "[" << perr_context << "] failed: \"" << QUOTEME(v1) << " != " << QUOTEME(v2) << "\", " << v1 << " == " << v2)
#define MK_COINS(amount) (UINT64_C(amount) * COIN)
#define TESTS_DEFAULT_FEE ((uint64_t)20000000000) // 2 * pow(10, 10)
