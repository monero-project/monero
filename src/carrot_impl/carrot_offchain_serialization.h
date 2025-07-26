// Copyright (c) 2025, The Monero Project
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
#include "carrot_chain_serialization.h"
#include "carrot_core/payment_proposal.h"
#include "serialization/binary_archive.h"
#include "serialization/crypto.h"
#include "serialization/optional.h"
#include "serialization/variant.h"
#include "subaddress_index.h"
#include "tx_proposal.h"

//third party headers

//standard headers

//forward declarations

BLOB_SERIALIZER(carrot::encrypted_amount_t);
BLOB_SERIALIZER(carrot::payment_id_t);

BEGIN_SERIALIZE_OBJECT_FN(carrot::CarrotDestinationV1)
    static constexpr uint8_t flag_is_subaddr = 1 << 0;
    static constexpr uint8_t flag_has_pid    = 1 << 1;
    uint8_t flags = 0;
    if (v.is_subaddress) flags |= flag_is_subaddr;
    if (v.payment_id != carrot::null_payment_id) flags |= flag_has_pid;
    FIELD(flags)
    v.is_subaddress = flags & flag_is_subaddr;

    FIELD_F(address_spend_pubkey)
    FIELD_F(address_view_pubkey)
    if (flags & flag_has_pid)
    {
        FIELD_F(payment_id)
    }
    else // no pid
    {
        v.payment_id = carrot::null_payment_id;
    }
END_SERIALIZE()

BEGIN_SERIALIZE_OBJECT_FN(carrot::CarrotPaymentProposalV1)
    FIELD_F(destination)
    VARINT_FIELD_F(amount)
    FIELD_F(randomness)
END_SERIALIZE()

BEGIN_SERIALIZE_OBJECT_FN(carrot::CarrotPaymentProposalSelfSendV1)
    FIELD_F(destination_address_spend_pubkey)
    VARINT_FIELD_F(amount)
    VARINT_FIELD_F(enote_type)
    FIELD_F(enote_ephemeral_pubkey)
    FIELD_F(internal_message)
END_SERIALIZE()

BEGIN_SERIALIZE_OBJECT_FN(carrot::subaddress_index)
    VARINT_FIELD_F(major)
    VARINT_FIELD_F(minor)
END_SERIALIZE()

BEGIN_SERIALIZE_OBJECT_FN(carrot::subaddress_index_extended)
    FIELD_F(index)
    VARINT_FIELD_F(derive_type)
END_SERIALIZE()

BEGIN_SERIALIZE_OBJECT_FN(carrot::CarrotEnoteV1)
    FIELD_F(onetime_address)
    FIELD_F(amount_commitment)
    FIELD_F(amount_enc)
    FIELD_F(anchor_enc)
    FIELD_F(view_tag)
    FIELD_F(enote_ephemeral_pubkey)
    FIELD_F(tx_first_key_image)
END_SERIALIZE()

BEGIN_SERIALIZE_OBJECT_FN(carrot::CarrotCoinbaseEnoteV1)
    FIELD_F(onetime_address)
    VARINT_FIELD_F(amount)
    FIELD_F(anchor_enc)
    FIELD_F(view_tag)
    FIELD_F(enote_ephemeral_pubkey)
    VARINT_FIELD_F(block_index)
END_SERIALIZE()

BEGIN_SERIALIZE_OBJECT_FN(carrot::LegacyOutputOpeningHintV1)
    FIELD_F(onetime_address)
    FIELD_F(ephemeral_tx_pubkey)
    FIELD_F(subaddr_index)
    VARINT_FIELD_F(amount)
    FIELD_F(amount_blinding_factor)
    VARINT_FIELD_F(local_output_index)
END_SERIALIZE()

BEGIN_SERIALIZE_OBJECT_FN(carrot::CarrotOutputOpeningHintV1)
    FIELD_F(source_enote)
    FIELD_F(encrypted_payment_id)
    FIELD_F(subaddr_index)
END_SERIALIZE()

BEGIN_SERIALIZE_OBJECT_FN(carrot::CarrotOutputOpeningHintV2)
    FIELD_F(onetime_address)
    FIELD_F(amount_commitment)
    FIELD_F(anchor_enc)
    FIELD_F(view_tag)
    FIELD_F(enote_ephemeral_pubkey)
    FIELD_F(tx_first_key_image)
    VARINT_FIELD_F(amount)
    FIELD_F(encrypted_payment_id)
    FIELD_F(subaddr_index)
END_SERIALIZE()

BEGIN_SERIALIZE_OBJECT_FN(carrot::CarrotCoinbaseOutputOpeningHintV1)
    FIELD_F(source_enote)
    VARINT_FIELD_F(derive_type)
END_SERIALIZE()

BEGIN_SERIALIZE_OBJECT_FN(carrot::CarrotPaymentProposalVerifiableSelfSendV1)
    FIELD_F(proposal)
    FIELD_F(subaddr_index)
END_SERIALIZE()

BEGIN_SERIALIZE_OBJECT_FN(carrot::CarrotTransactionProposalV1)
    FIELD_F(input_proposals)
    FIELD_F(normal_payment_proposals)
    FIELD_F(selfsend_payment_proposals)
    FIELD_F(dummy_encrypted_payment_id)
    VARINT_FIELD_F(fee)
    FIELD_F(extra)
END_SERIALIZE()

VARIANT_TAG(binary_archive, carrot::LegacyOutputOpeningHintV1, 0x80);
VARIANT_TAG(binary_archive, carrot::CarrotOutputOpeningHintV1, 0x81);
VARIANT_TAG(binary_archive, carrot::CarrotOutputOpeningHintV2, 0x82);
VARIANT_TAG(binary_archive, carrot::CarrotCoinbaseOutputOpeningHintV1, 0x83);
