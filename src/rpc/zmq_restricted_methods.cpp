// Copyright (c) 2016-2026, The Monero Project
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

#include "rpc/zmq_restricted_methods.h"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

namespace cryptonote
{
namespace rpc
{
  namespace
  {
    constexpr std::array<std::string_view, 10> blocked_in_restricted_mode{{
      "flush_txpool",
      "get_peer_list",
      "get_transaction_pool",
      "mining_status",
      "relay_tx",
      "save_bc",
      "set_log_categories",
      "set_log_level",
      "start_mining",
      "stop_mining"
    }};
  }

  bool is_blocked_in_restricted_mode(const std::string_view method) noexcept
  {
    return std::binary_search(
      blocked_in_restricted_mode.begin(),
      blocked_in_restricted_mode.end(),
      method
    );
  }

  void check_blocked_methods_sorted()
  {
    const auto last =
      std::is_sorted_until(blocked_in_restricted_mode.begin(), blocked_in_restricted_mode.end());

    if (last != blocked_in_restricted_mode.end())
      throw std::logic_error{
        std::string{"ZMQ restricted-method map is not properly sorted, see "} + std::string{*last}
      };
  }
} // rpc
} // cryptonote
