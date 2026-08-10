// Copyright (c) 2025-2026, The Monero Project
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

//local headers
#include "cryptonote_config.h"
#include "wallet2.h"
#include "wallet2_basic/wallet2_types.h"

//third party headers
#include <boost/multiprecision/cpp_int.hpp>

//standard headers


namespace tools
{
namespace wallet
{
/**
 * brief: sanity_check_pending_tx - validate `pending_tx` consistency with itself and with with `transfer_details`
 * param: ptx - the pending_tx to validate
 * param: transfers - all enotes owned by the wallet that produced `ptx`; `ptx.selected_transfers` is expected to
 *        map into this vector
 * param: nettype - the network that the tx might be sent to (e.g. mainnet, stagenet, testnet); used to validated
 *        destination addresses
 */
void sanity_check_pending_tx(const wallet2::pending_tx &ptx,
    const std::vector<wallet2_basic::transfer_details> &transfers,
    const cryptonote::network_type nettype);
} //namespace wallet
} //namespace tools
