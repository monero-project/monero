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

//! @file Constants used in Carrot

#pragma once

//local headers

//third party headers

//standard headers

//forward declarations

namespace carrot
{

// Carrot addressing protocol domain separators
static constexpr const unsigned char CARROT_DOMAIN_SEP_AMOUNT_BLINDING_FACTOR[] = "Carrot commitment mask";
static constexpr const unsigned char CARROT_DOMAIN_SEP_ONETIME_EXTENSION_G[] = "Carrot key extension G";
static constexpr const unsigned char CARROT_DOMAIN_SEP_ONETIME_EXTENSION_T[] = "Carrot key extension T";
static constexpr const unsigned char CARROT_DOMAIN_SEP_ENCRYPTION_MASK_ANCHOR[] = "Carrot encryption mask anchor";
static constexpr const unsigned char CARROT_DOMAIN_SEP_ENCRYPTION_MASK_AMOUNT[] = "Carrot encryption mask a";
static constexpr const unsigned char CARROT_DOMAIN_SEP_ENCRYPTION_MASK_PAYMENT_ID[] = "Carrot encryption mask pid";
static constexpr const unsigned char CARROT_DOMAIN_SEP_JANUS_ANCHOR_SPECIAL[] = "Carrot janus anchor special";
static constexpr const unsigned char CARROT_DOMAIN_SEP_EPHEMERAL_PRIVKEY[] = "Carrot sending key normal";
static constexpr const unsigned char CARROT_DOMAIN_SEP_VIEW_TAG[] = "Carrot view tag";
static constexpr const unsigned char CARROT_DOMAIN_SEP_SENDER_RECEIVER_SECRET[] = "Carrot sender-receiver secret";
static constexpr const unsigned char CARROT_DOMAIN_SEP_INPUT_CONTEXT_COINBASE = 'C';
static constexpr const unsigned char CARROT_DOMAIN_SEP_INPUT_CONTEXT_RINGCT = 'R';

// Carrot account secret domain separators
static constexpr const unsigned char CARROT_DOMAIN_SEP_PROVE_SPEND_KEY[] = "Carrot prove-spend key";
static constexpr const unsigned char CARROT_DOMAIN_SEP_VIEW_BALANCE_SECRET[] = "Carrot view-balance secret";
static constexpr const unsigned char CARROT_DOMAIN_SEP_GENERATE_IMAGE_KEY[] = "Carrot generate-image key";
static constexpr const unsigned char CARROT_DOMAIN_SEP_INCOMING_VIEW_KEY[] = "Carrot incoming view key";
static constexpr const unsigned char CARROT_DOMAIN_SEP_GENERATE_ADDRESS_SECRET[] = "Carrot generate-address secret";

// Carrot address domain separators
static constexpr const unsigned char CARROT_DOMAIN_SEP_ADDRESS_INDEX_GEN[] = "Carrot address index generator";
static constexpr const unsigned char CARROT_DOMAIN_SEP_SUBADDRESS_SCALAR[] = "Carrot subaddress scalar";

// Carrot misc constants
static constexpr const unsigned int CARROT_MIN_TX_OUTPUTS = 2;
static constexpr const unsigned int CARROT_MAX_TX_OUTPUTS = 16;
static constexpr const unsigned int CARROT_MIN_TX_INPUTS = 1;
static constexpr const unsigned int CARROT_MAX_TX_INPUTS = 16;
} //namespace carrot
