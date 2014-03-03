// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "chaingen.h"
#include "block_reward.h"
#include "block_validation.h"
#include "chain_split_1.h"
#include "chain_switch_1.h"
#include "double_spend.h"
#include "integer_overflow.h"
#include "ring_signature_1.h"
#include "tx_validation.h"
/************************************************************************/
/*                                                                      */
/************************************************************************/
class gen_simple_chain_001: public test_chain_unit_base 
{
public: 
  gen_simple_chain_001();
  bool generate(std::vector<test_event_entry> &events);
  bool verify_callback_1(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events); 
  bool verify_callback_2(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events); 
};

class one_block: public test_chain_unit_base
{
  cryptonote::account_base alice;
public:
  one_block();
  bool generate(std::vector<test_event_entry> &events);
  bool verify_1(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
};
