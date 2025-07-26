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
#include "carrot_impl/carrot_offchain_serialization.h"
#include "tx_builder.h"
#include "serialization/containers.h"
#include "serialization/string.h"
#include "serialization/tuple.h"
#include "serialization/variant.h"
#include "wallet2_basic/wallet2_cocobo_serialization.h"

//third party headers

//standard headers

//forward declarations

namespace tools
{
namespace wallet
{
//-------------------------------------------------------------------------------------------------------------------
BEGIN_SERIALIZE_OBJECT_FN(multisig_sig)
    VERSION_FIELD(1)
    if (version < 1)
        return false;
    FIELD_F(sigs)
    FIELD_F(ignore)
    FIELD_F(used_L)
    FIELD_F(signing_keys)
    FIELD_F(msout)
    FIELD_F(total_alpha_G)
    FIELD_F(total_alpha_H)
    FIELD_F(c_0)
    FIELD_F(s)
END_SERIALIZE()
//-------------------------------------------------------------------------------------------------------------------
BEGIN_SERIALIZE_OBJECT_FN(PreCarrotTransactionProposal)
    FIELD_F(sources)
    FIELD_F(change_dts)
    FIELD_F(splitted_dsts)
    FIELD_F(selected_transfers)
    FIELD_F(extra)
    FIELD_F(unlock_time)

    // converted `use_rct` field into construction_flags when view tags
    // were introduced to maintain backwards compatibility
    if (!typename Archive<W>::is_saving())
    {
        FIELD_N("use_rct", v.construction_flags)
        v.use_rct = (v.construction_flags & v._use_rct) > 0;
        v.use_view_tags = (v.construction_flags & v._use_view_tags) > 0;
    }
    else
    {
        v.construction_flags = 0;
        if (v.use_rct)
            v.construction_flags ^= v._use_rct;
        if (v.use_view_tags)
            v.construction_flags ^= v._use_view_tags;

        FIELD_N("use_rct", v.construction_flags)
    }

    FIELD_F(rct_config)
    FIELD_F(dests)
    FIELD_F(subaddr_account)
    FIELD_F(subaddr_indices)
END_SERIALIZE()
//-------------------------------------------------------------------------------------------------------------------
BEGIN_SERIALIZE_OBJECT_FN(pending_tx)
    VERSION_FIELD(2)
    FIELD_F(tx)
    FIELD_F(dust)
    FIELD_F(fee)
    FIELD_F(dust_added_to_fee)
    FIELD_F(change_dts)
    if (version < 2)
    {
        std::vector<std::size_t> selected_transfers;
        FIELD(selected_transfers)
    }
    FIELD_F(key_images)
    FIELD_F(tx_key)
    FIELD_F(additional_tx_keys)
    FIELD_F(dests)
    if (version < 2)
    {
        PreCarrotTransactionProposal pre_carrot_construction_data;
        FIELD_N("construction_data", pre_carrot_construction_data)
        v.construction_data = pre_carrot_construction_data;
        v.subaddr_account = pre_carrot_construction_data.subaddr_account;
        v.subaddr_indices = pre_carrot_construction_data.subaddr_indices;
    }
    else // version >= 2
    {
        FIELD_F(construction_data)
        FIELD_F(subaddr_account)
        FIELD_F(subaddr_indices)
    }
    FIELD_F(multisig_sigs)
    if (version < 1)
    {
        v.multisig_tx_key_entropy = crypto::null_skey;
        return true;
    }
    FIELD_F(multisig_tx_key_entropy)
END_SERIALIZE()
//-------------------------------------------------------------------------------------------------------------------
} //namespace wallet
} //namespace tools

VARIANT_TAG(binary_archive, tools::wallet::PreCarrotTransactionProposal, 0x21);
VARIANT_TAG(binary_archive, carrot::CarrotTransactionProposalV1, 0x22);
