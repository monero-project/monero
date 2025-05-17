// Copyright (c) 2024, The Monero Project
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

// A 'payment proposal' is a proposal to make an enote sending funds to a Carrot address.
// Carrot: Cryptonote Address For Rerandomizable-RingCT-Output Transactions

#pragma once

//local headers
#include "core_types.h"
#include "crypto/crypto.h"
#include "device.h"

//third party headers

//standard headers

//forward declarations


namespace carrot
{

////
// CarrotDestinationV1
// - for creating an output proposal to send an amount to someone
///
struct CarrotDestinationV1 final
{
    /// K^j_s
    crypto::public_key address_spend_pubkey;
    /// K^j_v
    crypto::public_key address_view_pubkey;
    /// is a subaddress?
    bool is_subaddress;
    /// legacy payment id pid: null for main addresses and subaddresses
    payment_id_t payment_id;
};

/// equality operators
bool operator==(const CarrotDestinationV1 &a, const CarrotDestinationV1 &b);
static inline bool operator!=(const CarrotDestinationV1 &a, const CarrotDestinationV1 &b) { return !(a == b); }

/**
* brief: make_carrot_main_address_v1 - make a destination address
* param: account_spend_pubkey - K_s
* param: primary_address_view_pubkey - K^0_v = k_v G
* outparam: destination_out - the full main address
*/
void make_carrot_main_address_v1(const crypto::public_key &account_spend_pubkey,
    const crypto::public_key &primary_address_view_pubkey,
    CarrotDestinationV1 &destination_out);
/**
* brief: make_carrot_subaddress_v1 - make a destination address
* param: account_spend_pubkey - K_s
* param: account_view_pubkey - K_v = k_v K_s
* param: s_generate_address_dev - device for s_ga
* param: j_major -
* param: j_minor -
* outparam: destination_out - the full subaddress
*/
void make_carrot_subaddress_v1(const crypto::public_key &account_spend_pubkey,
    const crypto::public_key &account_view_pubkey,
    const generate_address_secret_device &s_generate_address_dev,
    const std::uint32_t j_major,
    const std::uint32_t j_minor,
    CarrotDestinationV1 &destination_out);
/**
* brief: make_carrot_integrated_address_v1 - make a destination address
* param: account_spend_pubkey - K_s
* param: primary_address_view_pubkey - K^0_v = k_v G
* param: payment_id - pid
* outparam: destination_out - the full main address
*/
void make_carrot_integrated_address_v1(const crypto::public_key &account_spend_pubkey,
    const crypto::public_key &primary_address_view_pubkey,
    const payment_id_t payment_id,
    CarrotDestinationV1 &destination_out);
/**
* brief: gen_carrot_main_address_v1 - generate a random main address
*/
CarrotDestinationV1 gen_carrot_main_address_v1();
/**
* brief: gen_carrot_main_address_v1 - generate a random subaddress
*/
CarrotDestinationV1 gen_carrot_subaddress_v1();
/**
* brief: gen_carrot_main_address_v1 - generate a random integrated address
*/
CarrotDestinationV1 gen_carrot_integrated_address_v1();

} //namespace carrot
