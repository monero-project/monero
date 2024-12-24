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

#pragma once

//local headers
#include "legacy_enote_types.h"

//third party headers

//standard headers

//forward declarations


namespace carrot
{

struct IntermediateEnoteV1Record final
{
    /// original enote
    LegacyEnoteV1 enote;

    /// i: legacy address index
    cryptonote::subaddress_index subaddr_index;
    /// pid: payment ID
    jamtis::payment_id_t payment_id;
};

struct IntermediateEnoteV2Record final
{
    /// original enote
    LegacyEnoteV2 enote;

    /// i: legacy address index (false if unknown)
    std::optional<cryptonote::subaddress_index> subaddr_index;
    /// pid: payment ID
    jamtis::payment_id_t payment_id;
    /// a: the enote's amount
    rct::xmr_amount amount;
};

struct IntermediateEnoteV3Record final
{
    /// original enote
    LegacyEnoteV3 enote;

    /// i: legacy address index (false if unknown)
    std::optional<cryptonote::subaddress_index> subaddr_index;
    /// pid: payment ID
    jamtis::payment_id_t payment_id;
    /// a: the enote's amount
    rct::xmr_amount amount;
};

struct IntermediateEnoteV4Record final
{
    /// original enote
    LegacyEnoteV4 enote;

    /// i: legacy address index
    cryptonote::subaddress_index subaddr_index;
    /// pid: payment ID
    jamtis::payment_id_t payment_id;
};

struct IntermediateEnoteV5Record final
{
    /// original enote
    LegacyEnoteV5 enote;

    /// i: legacy address index
    std::optional<cryptonote::subaddress_index> subaddr_index;
    /// pid: payment ID
    jamtis::payment_id_t payment_id;
    /// a: the enote's amount
    rct::xmr_amount amount;
};

struct IntermediateEnoteV6Record final
{
    /// original enote
    SpCoinbaseEnoteV1 enote;
    /// the enote's ephemeral pubkey
    crypto::x25519_pubkey enote_ephemeral_pubkey;
    /// the enote's input context
    jamtis::input_context_t input_context;
};

struct IntermediateEnoteV7Record final
{
    /// original enote
    SpCoinbaseEnoteV1 enote;
    /// the enote's ephemeral pubkey
    crypto::x25519_pubkey enote_ephemeral_pubkey;
    /// the enote's input context
    jamtis::input_context_t input_context;

    /// i: legacy address index
    std::optional<cryptonote::subaddress_index> subaddr_index;
    /// pid: payment ID
    jamtis::payment_id_t payment_id;
    /// a: the enote's amount
    rct::xmr_amount amount;
    /// enote_type: the enote's type
    jamtis::JamtisEnoteType enote_type;
    /// is internal: true if used an internal secret for enote scanning
    bool is_internal;
};

using IntermediateEnoteRecordVariant = tools::variant<
    IntermediateEnoteV1Record,
    IntermediateEnoteV2Record,
    IntermediateEnoteV3Record,
    IntermediateEnoteV4Record,
    IntermediateEnoteV5Record,
    IntermediateEnoteV6Record,
    IntermediateEnoteV7Record>;

rct::xmr_amount amount_ref(const IntermediateEnoteRecordVariant &enote_record)
{
    struct mierv_amount_visitor: tools::variant_static_visitor<rct::xmr_amount>
    {
        rct::xmr_amount operator()(const IntermediateEnoteV1Record &v) const { return v.enote.amount; }
        rct::xmr_amount operator()(const IntermediateEnoteV2Record &v) const { return v.amount; }
        rct::xmr_amount operator()(const IntermediateEnoteV3Record &v) const { return v.amount; }
        rct::xmr_amount operator()(const IntermediateEnoteV4Record &v) const { return v.enote.amount; }
        rct::xmr_amount operator()(const IntermediateEnoteV5Record &v) const { return v.amount; }
        rct::xmr_amount operator()(const IntermediateEnoteV6Record &v) const { return v.enote.core.amount; }
        rct::xmr_amount operator()(const IntermediateEnoteV7Record &v) const { return v.amount; }
    };

    return enote_record.visit(mierv_amount_visitor());
}

const crypto::public_key& onetime_address_ref(const IntermediateEnoteRecordVariant &enote_record)
{
    struct mierv_onetime_address_visitor: tools::variant_static_visitor<const crypto::public_key&>
    {
        const crypto::public_key& operator()(const IntermediateEnoteV1Record &v) const { return v.enote.onetime_address; }
        const crypto::public_key& operator()(const IntermediateEnoteV2Record &v) const { return v.enote.onetime_address; }
        const crypto::public_key& operator()(const IntermediateEnoteV3Record &v) const { return v.enote.onetime_address; }
        const crypto::public_key& operator()(const IntermediateEnoteV4Record &v) const { return v.enote.onetime_address; }
        const crypto::public_key& operator()(const IntermediateEnoteV5Record &v) const { return v.enote.onetime_address; }
        const crypto::public_key& operator()(const IntermediateEnoteV6Record &v) const { return v.enote.core.onetime_address; }
        const crypto::public_key& operator()(const IntermediateEnoteV7Record &v) const { return v.enote.core.onetime_address; }
    };

    return enote_record.visit(mierv_amount_visitor());
}

void get_legacy_enote_identifier(const IntermediateEnoteRecordVariant &enote_record, crypto::hash &identifier_out)
{
    rct::key identifier_rk;
    get_legacy_enote_identifier(onetime_address_ref(enote_record), amount_ref(enote_record), identifier_rk);

    memcpy(&identifier_out, identifier_rk, sizeof(crypto::hash));
} 

} //namespace carrot
