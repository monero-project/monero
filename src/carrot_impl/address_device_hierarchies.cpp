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

//paired header
#include "address_device_hierarchies.h"

//local headers
#include "carrot_core/address_utils.h"
#include "carrot_core/exceptions.h"
#include "crypto/crypto.h"
#include "crypto/generators.h"
extern "C"
{
#include "crypto/crypto-ops.h"
}
#include "misc_log_ex.h"

//third party headers

//standard headers

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "carrot_impl"

namespace
{
static void assert_single_derive_type(const carrot::AddressDeriveType expected_derive_type,
    const carrot::AddressDeriveType actual_derive_type,
    const char * const dev_model,
    const char * const func_called)
{
    const auto make_error = [dev_model, func_called](std::string msg)
    {
        return carrot::device_error(std::string("Default"),
            std::string(dev_model),
            std::string(func_called),
            std::move(msg),
            carrot::E_UNSUPPORTED_ADDRESS_TYPE);
    };

    if (actual_derive_type != expected_derive_type && actual_derive_type != carrot::AddressDeriveType::Auto)
    {
        CARROT_THROW(make_error, "Expected derive type " << static_cast<int>(expected_derive_type)
            << ", got derive type " << static_cast<int>(actual_derive_type));
    }
}
} //anonymous namespace

namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
cryptonote_hierarchy_address_device::cryptonote_hierarchy_address_device(
    std::shared_ptr<cryptonote_view_incoming_key_device> k_view_incoming_dev,
    const crypto::public_key &cryptonote_account_spend_pubkey)
:
    m_k_view_incoming_dev(std::move(k_view_incoming_dev)),
    m_cryptonote_account_spend_pubkey(cryptonote_account_spend_pubkey)
{
    assert(this->m_k_view_incoming_dev);
}
//-------------------------------------------------------------------------------------------------------------------
void cryptonote_hierarchy_address_device::get_address_spend_pubkey(const subaddress_index_extended &subaddr_index,
    crypto::public_key &address_spend_pubkey_out) const
{
    this->assert_derive_type(subaddr_index, "get_address_spend_pubkey");

    if (!subaddr_index.index.is_subaddress())
    {
        address_spend_pubkey_out = this->m_cryptonote_account_spend_pubkey;
        return;
    }

    // k^j_subext = ScalarDeriveLegacy("SubAddr" || IntToBytes8(0) || k_v || IntToBytes32(j_major) || IntToBytes32(j_minor))
    crypto::secret_key k_subaddress_extension;
    this->m_k_view_incoming_dev->make_legacy_subaddress_extension(subaddr_index.index.major,
        subaddr_index.index.minor, k_subaddress_extension);

    // K^j_subext = k^j_subext G
    ge_p3 subaddress_extension_p3;
    ge_scalarmult_base(&subaddress_extension_p3, to_bytes(k_subaddress_extension));

    // decompress K_S
    ge_p3 account_spend_pubkey_p3;
    ge_frombytes_vartime(&account_spend_pubkey_p3, to_bytes(this->m_cryptonote_account_spend_pubkey)); // discard result
    ge_cached account_spend_pubkey_cached;
    ge_p3_to_cached(&account_spend_pubkey_cached, &account_spend_pubkey_p3);

    // K^j_s = K_s + K^j_subext
    ge_p1p1 address_spend_pubkey_p1p1;
    ge_add(&address_spend_pubkey_p1p1, &subaddress_extension_p3, &account_spend_pubkey_cached);
    ge_p2 address_spend_pubkey_p2;
    ge_p1p1_to_p2(&address_spend_pubkey_p2, &address_spend_pubkey_p1p1);
    ge_tobytes(to_bytes(address_spend_pubkey_out), &address_spend_pubkey_p2);
}
//-------------------------------------------------------------------------------------------------------------------
void cryptonote_hierarchy_address_device::get_address_pubkeys(const subaddress_index_extended &subaddr_index,
    crypto::public_key &address_spend_pubkey_out,
    crypto::public_key &address_view_pubkey_out) const
{
    this->assert_derive_type(subaddr_index, "get_address_pubkeys");
    this->get_address_spend_pubkey(subaddr_index, address_spend_pubkey_out);
    const crypto::public_key view_base_pubkey = subaddr_index.index.is_subaddress()
        ? address_spend_pubkey_out : crypto::get_G();
    this->m_k_view_incoming_dev->view_key_scalar_mult_ed25519(view_base_pubkey, address_view_pubkey_out);
}
//-------------------------------------------------------------------------------------------------------------------
void cryptonote_hierarchy_address_device::get_address_openings(const subaddress_index_extended &subaddr_index,
    crypto::secret_key &address_extension_g_out,
    crypto::secret_key &address_scalar_out) const
{
    this->assert_derive_type(subaddr_index, "get_address_openings");

    if (subaddr_index.index.is_subaddress())
    {
        // k^j_subext = ScalarDeriveLegacy("SubAddr" || IntToBytes8(0) || k_v || IntToBytes32(j_major) || IntToBytes32(j_minor))
        this->m_k_view_incoming_dev->make_legacy_subaddress_extension(subaddr_index.index.major,
            subaddr_index.index.minor,
            address_extension_g_out);
    }
    else // main address
    {
        // k^j_subext = 0
        address_extension_g_out = crypto::null_skey;
    }

    // k^j_subscal = 1
    address_scalar_out = crypto::secret_key{{1}};
}
//-------------------------------------------------------------------------------------------------------------------
bool cryptonote_hierarchy_address_device::view_key_scalar_mult_ed25519(const crypto::public_key &P,
    crypto::public_key &kvP) const
{
    return this->m_k_view_incoming_dev->view_key_scalar_mult_ed25519(P, kvP);
}
//-------------------------------------------------------------------------------------------------------------------
bool cryptonote_hierarchy_address_device::view_key_scalar_mult8_ed25519(const crypto::public_key &P,
    crypto::public_key &kv8P) const
{
    return this->m_k_view_incoming_dev->view_key_scalar_mult8_ed25519(P, kv8P);
}
//-------------------------------------------------------------------------------------------------------------------
bool cryptonote_hierarchy_address_device::view_key_scalar_mult_x25519(const mx25519_pubkey &D,
    mx25519_pubkey &kvD) const
{
    return this->m_k_view_incoming_dev->view_key_scalar_mult_x25519(D, kvD);
}
//-------------------------------------------------------------------------------------------------------------------
void cryptonote_hierarchy_address_device::make_janus_anchor_special(const mx25519_pubkey &enote_ephemeral_pubkey,
    const input_context_t &input_context,
    const crypto::public_key &onetime_address,
    janus_anchor_t &anchor_special_out) const
{
    return this->m_k_view_incoming_dev->make_janus_anchor_special(enote_ephemeral_pubkey, input_context,
        onetime_address, anchor_special_out);
}
//-------------------------------------------------------------------------------------------------------------------
void cryptonote_hierarchy_address_device::make_legacy_subaddress_extension(
    const std::uint32_t major_index,
    const std::uint32_t minor_index,
    crypto::secret_key &legacy_subaddress_extension_out) const
{
    return this->m_k_view_incoming_dev->make_legacy_subaddress_extension(major_index, minor_index,
        legacy_subaddress_extension_out);
}
//-------------------------------------------------------------------------------------------------------------------
void cryptonote_hierarchy_address_device::assert_derive_type(const subaddress_index_extended &subaddr_index,
    const char * const func_called) const
{
    assert_single_derive_type(AddressDeriveType::PreCarrot, subaddr_index.derive_type,
        "cryptonote_hierarchy_address_device", func_called);
}
//-------------------------------------------------------------------------------------------------------------------
carrot_hierarchy_address_device::carrot_hierarchy_address_device(
    std::shared_ptr<generate_address_secret_device> s_generate_address_dev,
    const crypto::public_key &carrot_account_spend_pubkey,
    const crypto::public_key &carrot_account_view_pubkey)
:
    m_s_generate_address_dev(std::move(s_generate_address_dev)),
    m_carrot_account_spend_pubkey(carrot_account_spend_pubkey),
    m_carrot_account_view_pubkey(carrot_account_view_pubkey)
{
    assert(this->m_s_generate_address_dev);
}
//-------------------------------------------------------------------------------------------------------------------
void carrot_hierarchy_address_device::get_address_spend_pubkey(const subaddress_index_extended &subaddr_index,
    crypto::public_key &address_spend_pubkey_out) const
{
    this->assert_derive_type(subaddr_index, "get_address_spend_pubkey");

    if (!subaddr_index.index.is_subaddress())
    {
        address_spend_pubkey_out = this->m_carrot_account_spend_pubkey;
        return;
    }

    const crypto::secret_key subaddress_scalar = this->get_subaddress_scalar(subaddr_index.index);

    // decompress K_s
    ge_p3 account_spend_pubkey_p3;
    ge_frombytes_vartime(&account_spend_pubkey_p3, to_bytes(this->m_carrot_account_spend_pubkey));

    // K^j_s = k^j_subscal K_s
    ge_p2 tmp_p2;
    ge_scalarmult(&tmp_p2, to_bytes(subaddress_scalar), &account_spend_pubkey_p3);
    ge_tobytes(to_bytes(address_spend_pubkey_out), &tmp_p2);
}
//-------------------------------------------------------------------------------------------------------------------
void carrot_hierarchy_address_device::get_address_pubkeys(const subaddress_index_extended &subaddr_index,
    crypto::public_key &address_spend_pubkey_out,
    crypto::public_key &address_view_pubkey_out) const
{
    this->assert_derive_type(subaddr_index, "get_address_pubkeys");

    if (!subaddr_index.index.is_subaddress())
    {
        address_spend_pubkey_out = this->m_carrot_account_spend_pubkey;
        return;
    }

    const crypto::secret_key subaddress_scalar = this->get_subaddress_scalar(subaddr_index.index);

    // decompress K_s
    ge_p3 tmp_p3;
    ge_frombytes_vartime(&tmp_p3, to_bytes(this->m_carrot_account_spend_pubkey));

    // K^j_s = k^j_subscal K_s
    ge_p2 tmp_p2;
    ge_scalarmult(&tmp_p2, to_bytes(subaddress_scalar), &tmp_p3);
    ge_tobytes(to_bytes(address_spend_pubkey_out), &tmp_p2);

    // decompress K_v
    ge_frombytes_vartime(&tmp_p3, to_bytes(this->m_carrot_account_view_pubkey));

    // K^j_v = k^j_subscal K_v
    ge_scalarmult(&tmp_p2, to_bytes(subaddress_scalar), &tmp_p3);
    ge_tobytes(to_bytes(address_view_pubkey_out), &tmp_p2);
}
//-------------------------------------------------------------------------------------------------------------------
void carrot_hierarchy_address_device::get_address_openings(const subaddress_index_extended &subaddr_index,
    crypto::secret_key &address_extension_g_out,
    crypto::secret_key &address_scalar_out) const
{
    this->assert_derive_type(subaddr_index, "get_address_openings");

    // k^j_subext = 0
    address_extension_g_out = crypto::null_skey;

    // [main]       k^j_subscal = 1
    // [subaddress] k^j_subscal = H_n[s^j_gen](K_s, K_v, j_major, j_minor)
    address_scalar_out = this->get_subaddress_scalar(subaddr_index.index);
}
//-------------------------------------------------------------------------------------------------------------------
crypto::secret_key carrot_hierarchy_address_device::get_subaddress_scalar(const subaddress_index &subaddr_index) const
{
    if (subaddr_index.is_subaddress())
    {
        // s^j_ap1 = H_32[s_ga](j_major, j_minor)
        crypto::secret_key res;
        this->m_s_generate_address_dev->make_address_index_preimage_1(subaddr_index.major,
            subaddr_index.minor,
            res);

        // s^j_ap2 = H_32[s^j_ap1](j_major, j_minor, K_s, K_v)
        make_carrot_address_index_preimage_2(res,
            subaddr_index.major,
            subaddr_index.minor,
            this->m_carrot_account_spend_pubkey,
            this->m_carrot_account_view_pubkey,
            res);

        // k^j_subscal = H_n[s^j_ap2](K_s)
        make_carrot_subaddress_scalar(res,
            this->m_carrot_account_spend_pubkey,
            res);

        return res;
    }
    else // main address
    {
        // k^j_subscal = 1
        return crypto::secret_key{{1}};
    }
}
//-------------------------------------------------------------------------------------------------------------------
void carrot_hierarchy_address_device::assert_derive_type(const subaddress_index_extended &subaddr_index,
    const char * const func_called) const
{
    assert_single_derive_type(AddressDeriveType::Carrot, subaddr_index.derive_type,
        "carrot_hierarchy_address_device", func_called);
}
//-------------------------------------------------------------------------------------------------------------------
hybrid_hierarchy_address_device::hybrid_hierarchy_address_device(std::shared_ptr<address_device> carrot_addr_dev,
    std::shared_ptr<address_device> cryptonote_addr_dev)
:
    m_carrot_addr_dev(std::move(carrot_addr_dev)),
    m_cryptonote_addr_dev(std::move(cryptonote_addr_dev))
{}
//-------------------------------------------------------------------------------------------------------------------
void hybrid_hierarchy_address_device::get_address_spend_pubkey(const subaddress_index_extended &subaddr_index,
    crypto::public_key &address_spend_pubkey_out) const
{
    this->resolve_address_device(subaddr_index.derive_type, "get_address_spend_pubkey")
        .get_address_spend_pubkey(subaddr_index, address_spend_pubkey_out);
}
//-------------------------------------------------------------------------------------------------------------------
void hybrid_hierarchy_address_device::get_address_pubkeys(const subaddress_index_extended &subaddr_index,
    crypto::public_key &address_spend_pubkey_out,
    crypto::public_key &address_view_pubkey_out) const
{
    this->resolve_address_device(subaddr_index.derive_type, "get_address_pubkeys")
        .get_address_pubkeys(subaddr_index, address_spend_pubkey_out, address_view_pubkey_out);
}
//-------------------------------------------------------------------------------------------------------------------
void hybrid_hierarchy_address_device::get_address_openings(const subaddress_index_extended &subaddr_index,
    crypto::secret_key &address_extension_g_out,
    crypto::secret_key &address_scalar_out) const
{
    this->resolve_address_device(subaddr_index.derive_type, "get_address_openings")
        .get_address_openings(subaddr_index, address_extension_g_out, address_scalar_out);
}
//-------------------------------------------------------------------------------------------------------------------
const address_device& hybrid_hierarchy_address_device::resolve_address_device(const AddressDeriveType derive_type,
    const char * const func_called) const
{
    const auto make_error = [func_called](std::string msg)
    {
        return carrot::device_error("Default",
            "hybrid_hierarchy_address_device",
            func_called,
            std::move(msg),
            E_UNSUPPORTED_ADDRESS_TYPE);
    };

    const address_device *p_resolved_addr_dev = nullptr;
    switch (derive_type)
    {
    case AddressDeriveType::Auto:
        if (this->m_carrot_addr_dev)
            p_resolved_addr_dev = this->m_carrot_addr_dev.get();
        else
            p_resolved_addr_dev = this->m_cryptonote_addr_dev.get();
        break;
    case AddressDeriveType::PreCarrot:
        p_resolved_addr_dev = this->m_cryptonote_addr_dev.get();
        break;
    case AddressDeriveType::Carrot:
        p_resolved_addr_dev = this->m_carrot_addr_dev.get();
        break;
    default:
        CARROT_THROW(make_error, "unrecognized derive type: " << static_cast<int>(derive_type));
        break;
    };

    CARROT_CHECK_AND_THROW(p_resolved_addr_dev,
        make_error, "Derive type not supported for this hybrid address device: " << static_cast<int>(derive_type));

    return *p_resolved_addr_dev;
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
