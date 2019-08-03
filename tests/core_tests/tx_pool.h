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

#pragma once

#include "chaingen.h"

class txpool_base : public test_chain_unit_base
{
  size_t m_public_tx_count;
  size_t m_all_tx_count;

public:
  txpool_base();

  bool increase_public_tx_count(cryptonote::core& c, size_t /*ev_index*/, const std::vector<test_event_entry>& events);
  bool increase_all_tx_count(cryptonote::core& c, size_t /*ev_index*/, const std::vector<test_event_entry>& events);
  bool check_txpool_spent_keys(cryptonote::core& c, size_t /*ev_index*/, const std::vector<test_event_entry>& events);
};

struct txpool_spend_key_public : txpool_base
{
  txpool_spend_key_public() : txpool_base()
  {}

  bool generate(std::vector<test_event_entry>& events) const;
};

struct txpool_spend_key_all : txpool_base
{
  txpool_spend_key_all() : txpool_base()
  {}

  bool generate(std::vector<test_event_entry>& events);
};
