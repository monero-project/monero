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
#include "carrot_tx_builder_types.h"
#include "serialization/crypto.h"
#include "serialization/optional.h"
#include "subaddress_index.h"

//third party headers

//standard headers

//forward declarations

BLOB_SERIALIZER(carrot::payment_id_t);

BEGIN_SERIALIZE_OBJECT_FN(carrot::CarrotDestinationV1)
    FIELD_F(address_spend_pubkey)
    FIELD_F(address_view_pubkey)
    FIELD_F(is_subaddress)
    FIELD_F(payment_id)
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

BEGIN_SERIALIZE_OBJECT_FN(carrot::CarrotPaymentProposalVerifiableSelfSendV1)
    FIELD_F(proposal)
    FIELD_F(subaddr_index)
END_SERIALIZE()

BEGIN_SERIALIZE_OBJECT_FN(carrot::CarrotTransactionProposalV1)
    FIELD_F(key_images_sorted)
    FIELD_F(normal_payment_proposals)
    FIELD_F(selfsend_payment_proposals)
    FIELD_F(dummy_encrypted_payment_id)
    VARINT_FIELD_F(fee)
    FIELD_F(extra)
END_SERIALIZE()
