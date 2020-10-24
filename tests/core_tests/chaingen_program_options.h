// Copyright (c) 2014-2020, The Monero Project
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

#include "fwd/boost_monero_program_options_fwd.h"
#include "chaingen.h"
#include "common/command_line_functions.h"

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
  tools::threadpool::getInstance().recycle();
//  c.deinit();
  return ret;
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

#define PLAY(filename, genclass) \
    if(!do_replay_file<genclass>(filename)) \
    { \
      MERROR("Failed to pass test : " << #genclass); \
      return 1; \
    }
    
