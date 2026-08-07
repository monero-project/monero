// Copyright (c) 2026, The Monero Project
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

#include "gtest/gtest.h"

#include "crypto/crypto.h"
#include "cryptonote_basic/account.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/subaddress_index.h"
#include "cryptonote_basic/verification_context.h"
#include "cryptonote_config.h"
#include "cryptonote_core/cryptonote_tx_utils.h"
#include "cryptonote_core/tx_verification_utils.h"
#include "device/device.hpp"
#include "ledger_tests_utils.h"
#include "ringct/rctTypes.h"

#include <boost/asio/io_context.hpp>
#include <unordered_map>

//----------------------------------------------------------------------------------------------------------------------
TEST(ledger, sign_ringct_to_ringct_bpp_tx)
{
    const cryptonote::account_keys &alice = mock::get_global_ledger_account();
    const cryptonote::account_public_address &alice_addr = alice.m_account_address;

    cryptonote::account_base bob;
    bob.generate();

    const rct::xmr_amount input_amount = 10 * COIN;
    const rct::xmr_amount bob_amount = 6 * COIN;
    const rct::xmr_amount fee = COIN / 10;
    const rct::xmr_amount change_amount = input_amount - bob_amount - fee;

    // destinations
    std::vector<cryptonote::tx_destination_entry> dests{
        cryptonote::tx_destination_entry(bob_amount, bob.get_keys().m_account_address, false), // bob
        cryptonote::tx_destination_entry(change_amount, alice_addr, false), // change
    };

    // generate fake RingCT transaction input data to Alice
    ASSERT_TRUE(alice.get_device().set_mode(hw::device::TRANSACTION_PARSE));
    std::vector<cryptonote::tx_source_entry> srcs{
        mock::gen_ringed_tx_source_entry(input_amount, alice_addr , false)};

    // subaddress map for Alice
    std::unordered_map<crypto::public_key, cryptonote::subaddress_index> alice_subaddresses;
    alice_subaddresses[alice_addr.m_spend_public_key];

    // make tx from Alice to Bob
    cryptonote::transaction tx;
    crypto::secret_key tx_key;
    std::vector<crypto::secret_key> additional_tx_keys;
    ASSERT_TRUE(alice.get_device().set_mode(hw::device::TRANSACTION_CREATE_REAL));
    ASSERT_TRUE(cryptonote::construct_tx_and_get_tx_key(alice, alice_subaddresses, srcs, dests,
        alice_addr, {}, tx, tx_key, additional_tx_keys, true, {rct::RangeProofPaddedBulletproof, 0}, true));
    ASSERT_EQ(2, tx.version);
    ASSERT_EQ(1, tx.vin.size());
    ASSERT_EQ(2, tx.vout.size());
    ASSERT_EQ(rct::RCTTypeBulletproofPlus, tx.rct_signatures.type);
    ASSERT_EQ(fee, tx.rct_signatures.txnFee);
    cryptonote::tx_verification_context tvc{};
    ASSERT_TRUE(cryptonote::ver_non_input_consensus(tx, tvc, HF_VERSION_BULLETPROOF_PLUS));
}
//----------------------------------------------------------------------------------------------------------------------