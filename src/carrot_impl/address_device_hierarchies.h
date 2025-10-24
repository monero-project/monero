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
#include "address_device.h"

//third party headers

//standard headers
#include <memory>

//forward declarations

namespace carrot
{
/// @brief extends the k_v device to allow Cryptonote-style address derivation
struct cryptonote_view_incoming_key_device: virtual public view_incoming_key_device
{
    /**
     * brief: make_legacy_subaddress_extension - k^j_subext
     *   k^j_subext = ScalarDeriveLegacy("SubAddr" || IntToBytes8(0) || k_v || IntToBytes32(j_major) || IntToBytes32(j_minor))
     * param: major_index - j_major
     * param: minor_index - j_minor
     * outparam: legacy_subaddress_extension_out - k^j_subext
     */
    virtual void make_legacy_subaddress_extension(
        const std::uint32_t major_index,
        const std::uint32_t minor_index,
        crypto::secret_key &legacy_subaddress_extension_out) const = 0;
};

/// @brief takes a CN k_v device and K_s to derive addresses in Cryptonote style
/// @note will fail if passed derive type is not ::PreCarrot or ::Auto
class cryptonote_hierarchy_address_device: public address_device, public cryptonote_view_incoming_key_device
{
public:
//constructor
    cryptonote_hierarchy_address_device(
        std::shared_ptr<cryptonote_view_incoming_key_device> k_view_incoming_dev,
        const crypto::public_key &cryptonote_account_spend_pubkey);

//address_device
    void get_address_spend_pubkey(const subaddress_index_extended &subaddr_index,
        crypto::public_key &address_spend_pubkey_out) const override;

    void get_address_pubkeys(const subaddress_index_extended &subaddr_index,
        crypto::public_key &address_spend_pubkey_out,
        crypto::public_key &address_view_pubkey_out) const override;

    void get_address_openings(const subaddress_index_extended &subaddr_index,
        crypto::secret_key &address_extension_g_out,
        crypto::secret_key &address_scalar_out) const override;

//cryptonote_view_incoming_key_device
    bool view_key_scalar_mult_ed25519(const crypto::public_key &P, crypto::public_key &kvP) const override;

    bool view_key_scalar_mult8_ed25519(const crypto::public_key &P, crypto::public_key &kv8P) const override;

    bool view_key_scalar_mult_x25519(const mx25519_pubkey &D, mx25519_pubkey &kvD) const override;

    void make_janus_anchor_special(const mx25519_pubkey &enote_ephemeral_pubkey,
        const input_context_t &input_context,
        const crypto::public_key &onetime_address,
        janus_anchor_t &anchor_special_out) const override;

    void make_legacy_subaddress_extension(
        const std::uint32_t major_index,
        const std::uint32_t minor_index,
        crypto::secret_key &legacy_subaddress_extension_out) const override;

protected:
//member fields
    std::shared_ptr<cryptonote_view_incoming_key_device> m_k_view_incoming_dev;
    crypto::public_key m_cryptonote_account_spend_pubkey;

private:
//member functions
    void assert_derive_type(const subaddress_index_extended &subaddr_index,
        const char * const func_called) const;
};

/// @brief takes a s_ga device and (K_s, K_v) to derive addresses in Carrot style
/// @note will fail if passed derive type is not ::Carrot or ::Auto
class carrot_hierarchy_address_device: public address_device
{
public:
//constructor
    carrot_hierarchy_address_device(
        std::shared_ptr<generate_address_secret_device> s_generate_address_dev,
        const crypto::public_key &carrot_account_spend_pubkey,
        const crypto::public_key &carrot_account_view_pubkey);

//address_device
    void get_address_spend_pubkey(const subaddress_index_extended &subaddr_index,
        crypto::public_key &address_spend_pubkey_out) const override;

    void get_address_pubkeys(const subaddress_index_extended &subaddr_index,
        crypto::public_key &address_spend_pubkey_out,
        crypto::public_key &address_view_pubkey_out) const override;

    void get_address_openings(const subaddress_index_extended &subaddr_index,
        crypto::secret_key &address_extension_g_out,
        crypto::secret_key &address_scalar_out) const override;

protected:
//member fields
    std::shared_ptr<generate_address_secret_device> m_s_generate_address_dev;
    crypto::public_key m_carrot_account_spend_pubkey;
    crypto::public_key m_carrot_account_view_pubkey;

private:
//member functions
    crypto::secret_key get_subaddress_scalar(const subaddress_index &subaddr_index) const;

    void assert_derive_type(const subaddress_index_extended &subaddr_index,
        const char * const func_called) const;
};

/// @brief takes a CN and/or a Carrot address device and dispatches derivation according to the passed derive type
/// @note resolves to Carrot on ::Auto derive type if available
class hybrid_hierarchy_address_device: public address_device
{
public:
//constructor
    hybrid_hierarchy_address_device(std::shared_ptr<address_device> carrot_addr_dev,
        std::shared_ptr<address_device> cryptonote_addr_dev);

//address_device
    void get_address_spend_pubkey(const subaddress_index_extended &subaddr_index,
        crypto::public_key &address_spend_pubkey_out) const override;

    void get_address_pubkeys(const subaddress_index_extended &subaddr_index,
        crypto::public_key &address_spend_pubkey_out,
        crypto::public_key &address_view_pubkey_out) const override;

    void get_address_openings(const subaddress_index_extended &subaddr_index,
        crypto::secret_key &address_extension_g_out,
        crypto::secret_key &address_scalar_out) const override;

protected:
//member fields
    std::shared_ptr<address_device> m_carrot_addr_dev;
    std::shared_ptr<address_device> m_cryptonote_addr_dev;

private:
//member functions
    const address_device& resolve_address_device(const AddressDeriveType derive_type,
        const char * const func_called) const;
};
} //namespace carrot
