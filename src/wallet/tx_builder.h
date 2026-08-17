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
#include "crypto/crypto.h"
#include "cryptonote_basic/account.h"
#include "cryptonote_basic/subaddress_index.h"
#include "cryptonote_config.h"
#include "wallet2.h"
#include "wallet2_basic/wallet2_types.h"

//third party headers

//standard headers
#include <unordered_map>
#include <vector>

namespace tools
{
namespace wallet
{
/**
 * brief: sanity_check_pending_tx - validate `pending_tx` consistency with itself and with with `transfer_details`
 *        Assumes `ptx` version is >= v16.
 *        WARNING: Unable to verify destination addresses that are URLs and not explicit addresses.
 *        WARNING: Due to upstream chaos, we are unable to reliably validate that `tx_construction_data::extra` matches
 *        with `ptx.tx.tx_extra`.
 *        NOTE: Due to upstream inconsistencies, we are unable to validate mixRing reliably (it is not serialized in
 *        final txs).
 * param: ptx - the pending_tx to validate
 * param: nettype - the network that will receive the tx (e.g. mainnet/stressnet/testnet)
 * param: account_keys - the keys of the tx author (only the private view key and base address are needed)
 * param: subaddresses - the tx author's subaddress map
 * param: transfers - transfer_details from inside `wallet2` (required because `ptx` includes transfer references)
 * param: redacted - if `true` then:
 *        - We assume `ptx.tx_key` and `ptx.additional_tx_keys` are nullified.
 * param: expect_imported_key_images - if `true` then we can verify key images line up to wallets' imported key images
 */
void sanity_check_pending_tx(const wallet2::pending_tx &ptx,
    const cryptonote::network_type nettype,
    const cryptonote::account_keys &account_keys,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddresses,
    const std::vector<wallet2_basic::transfer_details> &transfers,
    const bool redacted,
    const bool expect_imported_key_images);
} //namespace wallet
} //namespace tools
