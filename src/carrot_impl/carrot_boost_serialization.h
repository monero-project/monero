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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

//local headers
#include "carrot_chain_serialization.h"
#include "carrot_core/payment_proposal.h"
#include "subaddress_index.h"
#include "tx_proposal.h"

//third party headers
#include <boost/serialization/optional_shim.hpp>
#include <boost/serialization/utility.hpp>

//standard headers

//forward declarations

namespace boost
{
namespace serialization
{
//---------------------------------------------------
template <class Archive>
inline void serialize(Archive &a, mx25519_pubkey &x, const boost::serialization::version_type ver)
{
    a & x.data;
}
//---------------------------------------------------
template <class Archive>
inline void serialize(Archive &a, carrot::view_tag_t &x, const boost::serialization::version_type ver)
{
    a & x.bytes;
}
//---------------------------------------------------
template <class Archive>
inline void serialize(Archive &a, carrot::encrypted_janus_anchor_t &x, const boost::serialization::version_type ver)
{
    a & x.bytes;
}
//---------------------------------------------------
template <class Archive>
inline void serialize(Archive &a, carrot::encrypted_amount_t &x, const boost::serialization::version_type ver)
{
    a & x.bytes;
}
//---------------------------------------------------
template <class Archive>
inline void serialize(Archive &a, carrot::payment_id_t &x, const boost::serialization::version_type ver)
{
    a & x.bytes;
}
//---------------------------------------------------
template <class Archive>
inline void serialize(Archive &a, carrot::encrypted_payment_id_t &x, const boost::serialization::version_type ver)
{
    a & x.bytes;
}
//---------------------------------------------------
template <class Archive>
inline void serialize(Archive &a, carrot::CarrotDestinationV1 &x, const boost::serialization::version_type ver)
{
    a & x.address_spend_pubkey;
    a & x.address_view_pubkey;
    a & x.is_subaddress;
    a & x.payment_id;
}
//---------------------------------------------------
template <class Archive>
inline void serialize(Archive &a, carrot::CarrotPaymentProposalV1 &x, const boost::serialization::version_type ver)
{
    a & x.destination;
    a & x.amount;
    a & x.randomness;
}
//---------------------------------------------------
template <class Archive>
inline void serialize(Archive &a, carrot::CarrotPaymentProposalSelfSendV1 &x, const boost::serialization::version_type ver)
{
    a & x.destination_address_spend_pubkey;
    a & x.amount;
    a & x.enote_type;
    a & x.enote_ephemeral_pubkey;
    a & x.internal_message;
}
//---------------------------------------------------
template <class Archive>
inline void serialize(Archive &a, carrot::subaddress_index &x, const boost::serialization::version_type ver)
{
    a & x.major;
    a & x.minor;
}
//---------------------------------------------------
template <class Archive>
inline void serialize(Archive &a, carrot::subaddress_index_extended &x, const boost::serialization::version_type ver)
{
    a & x.index;
    a & x.derive_type;
}
//---------------------------------------------------
template <class Archive>
inline void serialize(Archive &a, carrot::CarrotEnoteV1 &x, const boost::serialization::version_type ver)
{
    a & x.onetime_address;
    a & x.amount_commitment;
    a & x.amount_enc;
    a & x.anchor_enc;
    a & x.view_tag;
    a & x.enote_ephemeral_pubkey;
    a & x.tx_first_key_image;
}
//---------------------------------------------------
template <class Archive>
inline void serialize(Archive &a, carrot::CarrotCoinbaseEnoteV1 &x, const boost::serialization::version_type ver)
{
    a & x.onetime_address;
    a & x.amount;
    a & x.anchor_enc;
    a & x.view_tag;
    a & x.enote_ephemeral_pubkey;
    a & x.block_index;
}
//---------------------------------------------------
template <class Archive>
inline void serialize(Archive &a, carrot::LegacyOutputOpeningHintV1 &x, const boost::serialization::version_type ver)
{
    a & x.onetime_address;
    a & x.ephemeral_tx_pubkey;
    a & x.subaddr_index;
    a & x.amount;
    a & x.amount_blinding_factor;
    a & x.local_output_index;
}
//---------------------------------------------------
template <class Archive>
inline void serialize(Archive &a, carrot::CarrotOutputOpeningHintV1 &x, const boost::serialization::version_type ver)
{
    a & x.source_enote;
    a & x.encrypted_payment_id;
    a & x.subaddr_index;
}
//---------------------------------------------------
template <class Archive>
inline void serialize(Archive &a, carrot::CarrotOutputOpeningHintV2 &x, const boost::serialization::version_type ver)
{
    a & x.onetime_address;
    a & x.amount_commitment;
    a & x.anchor_enc;
    a & x.view_tag;
    a & x.enote_ephemeral_pubkey;
    a & x.tx_first_key_image;
    a & x.amount;
    a & x.encrypted_payment_id;
    a & x.subaddr_index;
}
//---------------------------------------------------
template <class Archive>
inline void serialize(Archive &a, carrot::CarrotCoinbaseOutputOpeningHintV1 &x, const boost::serialization::version_type ver)
{
    a & x.source_enote;
    a & x.derive_type;
}
//---------------------------------------------------
template <class Archive>
inline void serialize(Archive &a, carrot::CarrotPaymentProposalVerifiableSelfSendV1 &x, const boost::serialization::version_type ver)
{
    a & x.proposal;
    a & x.subaddr_index;
}
//---------------------------------------------------
template <class Archive>
inline void serialize(Archive &a, carrot::CarrotTransactionProposalV1 &x, const boost::serialization::version_type ver)
{
    a & x.input_proposals;
    a & x.normal_payment_proposals;
    a & x.selfsend_payment_proposals;
    a & x.dummy_encrypted_payment_id;
    a & x.fee;
    a & x.extra;
}
//---------------------------------------------------
} //namespace serialization
} //namespace boot
