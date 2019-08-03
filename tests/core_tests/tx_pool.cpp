// Copyright (c) 2019, The Monero Project
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

#include "txpool.h"

#define INIT_MEMPOOL_TEST()                                                                                         \
  uint64_t send_amount = 1000;                                                                                      \
  uint64_t ts_start = 1338224400;                                                                                   \
  GENERATE_ACCOUNT(miner_account);                                                                                  \
  GENERATE_ACCOUNT(bob_account);                                                                                    \
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);                                                       \
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);                                                              \

  /*
  MAKE_TX(events, tx_0, miner_account, bob_account, send_amount, blk_0);                                            \
                                                                                                                    \
  std::vector<cryptonote::tx_source_entry> sources;                                                                 \
  cryptonote::tx_source_entry se;                                                                                   \
  se.amount = tx_0.vout[0].amount;                                                                                  \
  se.push_output(0, boost::get<cryptonote::txout_to_key>(tx_0.vout[0].target).key, se.amount);                      \
  se.real_output = 0;                                                                                               \
  se.rct = false;                                                                                                   \
  se.real_out_tx_key = get_tx_pub_key_from_extra(tx_0);                                                             \
  se.real_output_in_tx_index = 0;                                                                                   \
  sources.push_back(se);                                                                                            \
                                                                                                                    \
  cryptonote::tx_destination_entry de;                                                                              \
  de.addr = miner_account.get_keys().m_account_address;                                                             \
  de.amount = se.amount - TESTS_DEFAULT_FEE;                                                                        \
  std::vector<cryptonote::tx_destination_entry> destinations;                                                       \
  destinations.push_back(de);                                                                                       \
                                                                                                                    \
  cryptonote::transaction tx_1;                                                                                     \
  if (!construct_tx(bob_account.get_keys(), sources, destinations, boost::none, std::vector<uint8_t>(), tx_1, 0))   \
    return false; */


txpool_base::txpool_base()
  : test_chain_unit_base()
  , m_public_tx_count(0)
  , m_all_tx_count(0)
{
  REGISTER_CALLBACK_METHOD(txpool_spend_key_public, increase_public_tx_count);
  REGISTER_CALLBACK_METHOD(txpool_spend_key_public, increase_all_tx_count);
  REGISTER_CALLBACK_METHOD(txpool_spend_key_public, check_txpool_spent_keys);
}

bool txpool_base::increase_public_tx_count(cryptonote::core& /*c*/, size_t /*ev_index*/, const std::vector<test_event_entry>& /*events*/)
{
  ++m_public_tx_count;
  ++m_all_tx_count;
  return true;
}

bool txpool_base::increase_all_tx_count(cryptonote::core& /*c*/, size_t /*ev_index*/, const std::vector<test_event_entry>& /*events*/)
{
  ++m_all_tx_count;
  return true;
}

bool txpool_base::check_txpool_spent_keys(cryptonote::core& c, size_t /*ev_index*/, const std::vector<test_event_entry>& events)
{
  std::vector<cryptonote::tx_info> infos{};
  std::vector<cryptonote::spent_key_image_info> key_images{};
  if (!c.get_pool_transactions_and_spent_keys_info(infos, key_images) || infos.size() != m_all_tx_count || key_images.size() != m_all_tx_count)
    return false;

  infos.clear();
  key_images.clear();
  if (!c.get_pool_transactions_and_spent_keys_info(infos, key_images, true) || infos.size() != m_all_tx_count || key_images.size() != m_all_tx_count)
    return false;

  infos.clear();
  key_images.clear();
  if (!c.get_pool_transactions_and_spent_keys_info(infos, key_images, false) || infos.size() != m_public_tx_count || key_images.size() != m_public_tx_count)
    return false;
  
  return true;
}

bool txpool_spend_key_public::generate(std::vector<test_event_entry>& events) const
{
  INIT_MEMPOOL_TEST();

  DO_CALLBACK(events, "check_txpool_spent_keys");
  MAKE_TX(events, tx_0, miner_account, bob_account, send_amount, blk_0);
  DO_CALLBACK(events, "increase_public_tx_count");
  DO_CALLBACK(events, "check_txpool_spent_keys");
 
  return true;
}

bool txpool_spend_key_all::generate(std::vector<test_event_entry>& events)
{
  INIT_MEMPOOL_TEST();
  SET_EVENT_VISITOR_SETT(events, event_visitor_settings::set_txs_do_not_relay, true);

  DO_CALLBACK(events, "check_txpool_spent_keys");
  MAKE_TX(events, tx_0, miner_account, bob_account, send_amount, blk_0);                                            
  DO_CALLBACK(events, "increase_all_tx_count");
  DO_CALLBACK(events, "check_txpool_spent_keys");
 
  return true;
}


