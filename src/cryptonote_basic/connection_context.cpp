// Copyright (c) 2020-2022, The Monero Project

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

#include "connection_context.h"

#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "p2p/p2p_protocol_defs.h"

namespace cryptonote
{
  std::size_t cryptonote_connection_context::get_max_bytes(const int command) noexcept
  {
    switch (command)
    {
    case nodetool::COMMAND_HANDSHAKE_T<cryptonote::CORE_SYNC_DATA>::ID:
      return 65536;
    case nodetool::COMMAND_TIMED_SYNC_T<cryptonote::CORE_SYNC_DATA>::ID:
      return 65536;
    case nodetool::COMMAND_PING::ID:
      return 4096;
    case nodetool::COMMAND_REQUEST_SUPPORT_FLAGS::ID:
      return 4096;
    case cryptonote::NOTIFY_NEW_BLOCK::ID:
      return 1024 * 1024 * 128; // 128 MB (max packet is a bit less than 100 MB though)
    case cryptonote::NOTIFY_NEW_TRANSACTIONS::ID:
      return 1024 * 1024 * 128; // 128 MB (max packet is a bit less than 100 MB though)
    case cryptonote::NOTIFY_REQUEST_GET_OBJECTS::ID:
      return 1024 * 1024 * 2; // 2 MB
    case cryptonote::NOTIFY_RESPONSE_GET_OBJECTS::ID:
      return 1024 * 1024 * 128; // 128 MB (max packet is a bit less than 100 MB though)
    case cryptonote::NOTIFY_REQUEST_CHAIN::ID:
      return 512 * 1024; // 512 kB
    case cryptonote::NOTIFY_RESPONSE_CHAIN_ENTRY::ID:
      return 1024 * 1024 * 4; // 4 MB
    case cryptonote::NOTIFY_NEW_FLUFFY_BLOCK::ID:
      return 1024 * 1024 * 4; // 4 MB, but it does not includes transaction data
    case cryptonote::NOTIFY_REQUEST_FLUFFY_MISSING_TX::ID:
      return 1024 * 1024; // 1 MB
    case cryptonote::NOTIFY_GET_TXPOOL_COMPLEMENT::ID:
      return 1024 * 1024 * 4; // 4 MB
    default:
      break;
    };
    return std::numeric_limits<size_t>::max();
  }
} // cryptonote
