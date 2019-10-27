// Copyright (c) 2018-2019, The Monero Project
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

#include <type_traits>

namespace
{
  // credits to yrp (https://stackoverflow.com/questions/87372/check-if-a-class-has-a-member-function-of-a-given-signature
  template <typename T>
  struct HasCredits
  {
    template<typename U, uint64_t (U::*)> struct SFINAE {};
    template<typename U> static char Test(SFINAE<U, &U::credits>*);
    template<typename U> static int Test(...);
    static const bool Has = sizeof(Test<T>(0)) == sizeof(char);
  };
}

namespace tools
{
  struct rpc_payment_state_t
  {
    uint64_t credits;
    uint64_t expected_spent;
    uint64_t discrepancy;
    std::string top_hash;
    bool stale;

    rpc_payment_state_t(): credits(0), expected_spent(0), discrepancy(0), stale(true) {}
  };

  static inline void check_rpc_cost(rpc_payment_state_t &rpc_payment_state, const char *call, uint64_t post_call_credits, uint64_t pre_call_credits, double expected_cost)
  {
    uint64_t expected_credits = (uint64_t)expected_cost;
    if (expected_credits == 0)
      expected_credits = 1;

    rpc_payment_state.credits = post_call_credits;
    rpc_payment_state.expected_spent += expected_credits;

    if (pre_call_credits <= post_call_credits)
      return;

    uint64_t cost = pre_call_credits - post_call_credits;

    if (cost == expected_credits)
    {
      MDEBUG("Call " << call << " cost " << cost << " credits");
      return;
    }
    MWARNING("Call " << call << " cost " << cost << " credits, expected " << expected_credits);

    if (cost > expected_credits)
    {
      uint64_t d = cost - expected_credits;
      if (rpc_payment_state.discrepancy > std::numeric_limits<uint64_t>::max() - d)
      {
        MERROR("Integer overflow in credit discrepancy calculation, setting to max");
        rpc_payment_state.discrepancy = std::numeric_limits<uint64_t>::max();
      }
      else
      {
        rpc_payment_state.discrepancy += d;
      }
    }
  }
}
