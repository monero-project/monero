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
#include "hot_cold.h"
#include "tx_builder_serialization.h"

//third party headers

//standard headers

//forward declarations

#define PASSTHROUGH_VERSION(ver_lo, ver_hi) if (!handle_version_passthrough(ar, ver_lo, ver_hi, version)) return false;

#define BEGIN_SERIALIZE_VERSIONED_VARIANT(vtype)                                                     \
    DISABLE_DEFAULT_VARIANT_SERIALIZATION(vtype)                                                     \
    template <template <bool> class Archive> bool do_serialize(Archive<true> &ar, vtype &v) {        \
        ar.begin_object();                                                                           \
        const bool r = std::visit([&ar](auto &x) -> bool { return do_serialize_object(ar, x); }, v); \
        ar.end_object();                                                                             \
        return r && ar.good(); }                                                                     \
    template <template <bool> class Archive> bool do_serialize(Archive<false> &ar, vtype &v) {       \
        ar.begin_object();

#define END_SERIALIZE_VERSIONED_VARIANT() do { ar.end_object(); return ar.good(); } while (0); }

#define LOAD_VARIANT_AS_OBJECT_OF_TYPE(ty)                       \
    do {                                                         \
        ty vv;                                                   \
        if (!do_serialize_object(ar, vv, version) || !ar.good()) \
            return false;                                        \
        v = std::move(vv);                                       \
        return true;                                             \
    } while (0);

namespace tools
{
namespace wallet
{
namespace cold
{
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
template <bool W, template <bool> class Archive>
static bool handle_version_passthrough(Archive<W> &ar,
    const std::uint32_t min_version,
    const std::uint32_t max_version,
    std::uint32_t &version_inout)
{
    if (version_inout == (std::uint32_t)-1)
    {
        version_inout = max_version;
        VARINT_FIELD_N("version", version_inout)
    }
    else if (W && version_inout != max_version) {
        // passed conflicting version
        return false;
    }
    return min_version <= version_inout && version_inout <= max_version;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
BEGIN_SERIALIZE_OBJECT_FN(exported_pre_carrot_transfer_details, uint32_t version = (uint32_t)-1)
    PASSTHROUGH_VERSION(1, 1)
    FIELD_F(m_pubkey)
    VARINT_FIELD_F(m_internal_output_index)
    VARINT_FIELD_F(m_global_output_index)
    FIELD_F(m_tx_pubkey)
    FIELD_F(m_flags.flags)
    VARINT_FIELD_F(m_amount)
    FIELD_F(m_additional_tx_keys)
    VARINT_FIELD_F(m_subaddr_index_major)
    VARINT_FIELD_F(m_subaddr_index_minor)
END_SERIALIZE()
//-------------------------------------------------------------------------------------------------------------------
BEGIN_SERIALIZE_OBJECT_FN(exported_carrot_transfer_details, uint32_t version = (uint32_t)-1)
    PASSTHROUGH_VERSION(2, 2)
    VARINT_FIELD_N("flags", v.flags.flags)
    if (v.flags.m_coinbase)
    {
        FIELD_F(block_index)
        v.tx_first_key_image = crypto::key_image{};
    }
    else
    {
        v.block_index = 0;
        FIELD_F(tx_first_key_image)
    }
    if (v.flags.m_has_pid)
    {
        FIELD_F(payment_id)
        v.subaddr_index = {0, 0};
    }
    else // !m_has_pid
    {
        if (v.flags.m_coinbase)
        {
            v.subaddr_index = {0, 0};
        }
        else
        {
            FIELD_F(subaddr_index)
        }
        v.payment_id = carrot::null_payment_id;
    }
    VARINT_FIELD_F(amount)
    FIELD_F(janus_anchor)
    if (v.flags.m_selfsend)
        FIELD_F(selfsend_enote_ephemeral_pubkey)
    else
        v.selfsend_enote_ephemeral_pubkey = mx25519_pubkey{{0}};
END_SERIALIZE()
//-------------------------------------------------------------------------------------------------------------------
BEGIN_SERIALIZE_OBJECT_FN(UnsignedPreCarrotTransactionSet, uint32_t version = (uint32_t)-1)
    PASSTHROUGH_VERSION(0, 2)
    FIELD_F(txes)
    if (version == 0)
    {
        std::pair<size_t, wallet2_basic::transfer_container> v0_transfers;
        FIELD(v0_transfers);
        std::get<0>(v.transfers) = std::get<0>(v0_transfers);
        std::get<1>(v.transfers) = std::get<0>(v0_transfers) + std::get<1>(v0_transfers).size();
        std::get<2>(v.transfers) = std::get<1>(v0_transfers);
        return true;
    }
    if (version == 1)
    {
        std::pair<size_t, std::vector<exported_pre_carrot_transfer_details>> v1_transfers;
        FIELD(v1_transfers);
        std::get<0>(v.new_transfers) = std::get<0>(v1_transfers);
        std::get<1>(v.new_transfers) = std::get<0>(v1_transfers) + std::get<1>(v1_transfers).size();
        std::get<2>(v.new_transfers) = std::get<1>(v1_transfers);
        return true;
    }
    FIELD_F(new_transfers)
END_SERIALIZE()
//-------------------------------------------------------------------------------------------------------------------
BEGIN_SERIALIZE_OBJECT_FN(SignedFullTransactionSet, uint32_t version = (uint32_t)-1)
    PASSTHROUGH_VERSION(0, 0)
    FIELD_F(ptx)
    FIELD_F(key_images)
    FIELD_F(tx_key_images)
END_SERIALIZE()
//-------------------------------------------------------------------------------------------------------------------
} //namespace cold
} //namespace wallet
} //namespace tools
//-------------------------------------------------------------------------------------------------------------------
BEGIN_SERIALIZE_VERSIONED_VARIANT(tools::wallet::cold::exported_transfer_details_variant)
    using namespace tools::wallet::cold;
    VERSION_FIELD(0)
    if (version == 0 || version > 2)
        return false;

    if (version == 1)
        LOAD_VARIANT_AS_OBJECT_OF_TYPE(exported_pre_carrot_transfer_details)
    else
        LOAD_VARIANT_AS_OBJECT_OF_TYPE(exported_carrot_transfer_details)
END_SERIALIZE_VERSIONED_VARIANT()
//-------------------------------------------------------------------------------------------------------------------
