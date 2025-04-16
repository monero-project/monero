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

#include "carrot_core/device_ram_borrowed.h"
#include "carrot_core/enote_utils.h"
#include "carrot_core/payment_proposal.h"
#include "carrot_core/scan.h"
#include "crypto/crypto.h"
#include "device/device.hpp"
#include "performance_tests.h"
#include "ringct/rctOps.h"
#include "ringct/rctTypes.h"

//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------

struct ParamsShuttleViewScan final : public ParamsShuttle
{
    bool test_view_tag_check{false};
};

/// cryptonote view key scanning (with optional view tag check)
/// test:
/// - sender-receiver secret: kv*R_t
/// - view tag: H1(kv*R_t)
/// - (optional): return here to mimick a view tag check failure
/// - Ks_nom = Ko - H(kv*R_t)*G
/// - Ks ?= Ks_nom
class test_view_scan_cn
{
public:
    static const size_t loop_count = 1000;

    bool init(const ParamsShuttleViewScan &params)
    {
        m_test_view_tag_check = params.test_view_tag_check;

        // kv, Ks = ks*G, R_t = r_t*G
        m_view_secret_key = rct::rct2sk(rct::skGen());
        m_spendkey = rct::rct2pk(rct::pkGen());
        m_tx_pub_key = rct::rct2pk(rct::pkGen());

        // kv*R_t (i.e. r_t*Kv)
        crypto::key_derivation derivation;
        crypto::generate_key_derivation(m_tx_pub_key, m_view_secret_key, derivation);

        // Ko = H(kv*R_t, t)*G + Ks
        crypto::derive_public_key(derivation, 0, m_spendkey, m_onetime_address);

        return true;
    }

    bool test()
    {
        // kv*R_t
        crypto::key_derivation derivation;
        crypto::generate_key_derivation(m_tx_pub_key, m_view_secret_key, derivation);

        // view tag: H1(kv*R_t, t)
        crypto::view_tag mock_view_tag;
        crypto::derive_view_tag(derivation, 0, mock_view_tag);

        // check: early return after computing a view tag (e.g. if nominal view tag doesn't match enote view tag)
        if (m_test_view_tag_check)
            return true;

        // Ks_nom = Ko - H(kv*R_t, t)*G
        crypto::public_key nominal_spendkey;
        crypto::derive_subaddress_public_key(m_onetime_address, derivation, 0, nominal_spendkey);

        // Ks_nom ?= Ks
        return nominal_spendkey == m_spendkey;
    }

private:
    /// kv
    crypto::secret_key m_view_secret_key;
    /// Ks = ks*G
    crypto::public_key m_spendkey;

    /// R_t = r_t*G
    crypto::public_key m_tx_pub_key;
    /// Ko = H(kv*R_t, t)*G + Ks
    crypto::public_key m_onetime_address;

    bool m_test_view_tag_check;
};

//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------

////
// cryptonote view key scanning using optimized crypto library (with optional view tag check)
// note: this relies on 'default hwdev' to auto-find the current machine's best available crypto implementation
/// 
class test_view_scan_cn_optimized
{
public:
    static const size_t loop_count = 1000;

    bool init(const ParamsShuttleViewScan &params)
    {
        m_test_view_tag_check = params.test_view_tag_check;

        // kv, Ks = ks*G, R_t = r_t*G
        m_view_secret_key = rct::rct2sk(rct::skGen());
        m_spendkey = rct::rct2pk(rct::pkGen());
        m_tx_pub_key = rct::rct2pk(rct::pkGen());

        // kv*R_t (i.e. r_t*Kv)
        crypto::key_derivation derivation;
        m_hwdev.generate_key_derivation(m_tx_pub_key, m_view_secret_key, derivation);

        // Ko = H(kv*R_t, t)*G + Ks
        m_hwdev.derive_public_key(derivation, 0, m_spendkey, m_onetime_address);

        return true;
    }

    bool test()
    {
        // kv*R_t
        crypto::key_derivation derivation;
        m_hwdev.generate_key_derivation(m_tx_pub_key, m_view_secret_key, derivation);

        // view tag: H1(kv*R_t, t)
        crypto::view_tag mock_view_tag;
        m_hwdev.derive_view_tag(derivation, 0, mock_view_tag);

        // check: early return after computing a view tag (e.g. if nominal view tag doesn't match enote view tag)
        if (m_test_view_tag_check)
            return true;

        // Ks_nom = Ko - H(kv*R_t, t)*G
        crypto::public_key nominal_spendkey;
        m_hwdev.derive_subaddress_public_key(m_onetime_address, derivation, 0, nominal_spendkey);

        // Ks_nom ?= Ks
        return nominal_spendkey == m_spendkey;
    }

private:
    hw::device &m_hwdev{hw::get_device("default")};

    /// kv
    crypto::secret_key m_view_secret_key;
    /// Ks = ks*G
    crypto::public_key m_spendkey;

    /// R_t = r_t*G
    crypto::public_key m_tx_pub_key;
    /// Ko = H(kv*R_t, t)*G + Ks
    crypto::public_key m_onetime_address;

    bool m_test_view_tag_check;
};

//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------

/// carrot view key scanning
class test_view_scan_carrot
{
public:
    static const size_t loop_count = 1000;

    test_view_scan_carrot(): m_k_view_dev(m_k_view_incoming) {}

    bool init(const ParamsShuttleViewScan &params)
    {
        m_test_view_tag_check = params.test_view_tag_check;

        m_k_view_incoming = rct::rct2sk(rct::skGen());

        m_account_spend_pubkey = rct::rct2pk(rct::pkGen());
        const crypto::public_key account_view_pubkey = rct::rct2pk(rct::scalarmultKey(rct::pk2rct(m_account_spend_pubkey), rct::sk2rct(m_k_view_incoming)));

        carrot::CarrotDestinationV1 subaddress;
        carrot::make_carrot_subaddress_v1(m_account_spend_pubkey,
                account_view_pubkey,
                carrot::generate_address_secret_ram_borrowed_device({}),
                88,
                88,
                subaddress);

        const carrot::CarrotPaymentProposalV1 payment_proposal{
            .destination = subaddress,
            .amount = crypto::rand<rct::xmr_amount>(),
            .randomness = carrot::gen_janus_anchor()
        };

        carrot::RCTOutputEnoteProposal output_proposal;
        carrot::get_output_proposal_normal_v1(payment_proposal,
            {},
            output_proposal,
            m_encrypted_payment_id);
        m_enote = output_proposal.enote;

        mx25519_pubkey s_sender_receiver_unctx;
        carrot::make_carrot_uncontextualized_shared_key_receiver(m_k_view_incoming,
            m_enote.enote_ephemeral_pubkey,
            s_sender_receiver_unctx);

        crypto::secret_key _1, _2, _3;
        crypto::public_key recovered_address_spend_pubkey;
        rct::xmr_amount recovered_amount;
        carrot::payment_id_t recovered_payment_id;
        carrot::CarrotEnoteType recovered_enote_type;
        if (!carrot::try_scan_carrot_enote_external_receiver(m_enote,
                m_encrypted_payment_id,
                s_sender_receiver_unctx,
                {&m_account_spend_pubkey, 1},
                m_k_view_dev,
                _1,
                _2,
                recovered_address_spend_pubkey,
                recovered_amount,
                _3,
                recovered_payment_id,
                recovered_enote_type))
            return false;

        if (recovered_address_spend_pubkey != subaddress.address_spend_pubkey)
            return false;

        if (recovered_amount != payment_proposal.amount)
            return false;
        
        if (recovered_payment_id != carrot::null_payment_id)
            return false;
        
        if (recovered_enote_type != carrot::CarrotEnoteType::PAYMENT)
            return false;

        if (m_test_view_tag_check)
            m_enote.view_tag.bytes[0] ^= 1;

        return true;
    }

    bool test()
    {
        mx25519_pubkey s_sender_receiver_unctx;
        carrot::make_carrot_uncontextualized_shared_key_receiver(m_k_view_incoming,
            m_enote.enote_ephemeral_pubkey,
            s_sender_receiver_unctx);

        crypto::secret_key _1, _2, _3;
        crypto::public_key recovered_address_spend_pubkey;
        rct::xmr_amount recovered_amount;
        carrot::payment_id_t recovered_payment_id;
        carrot::CarrotEnoteType recovered_enote_type;
        const bool scan_success = carrot::try_scan_carrot_enote_external_receiver(m_enote,
            m_encrypted_payment_id,
            s_sender_receiver_unctx,
            {&m_account_spend_pubkey, 1},
            m_k_view_dev,
            _1,
            _2,
            recovered_address_spend_pubkey,
            recovered_amount,
            _3,
            recovered_payment_id,
            recovered_enote_type);

        return scan_success ^ m_test_view_tag_check;
    }

private:
    crypto::public_key m_account_spend_pubkey;
    crypto::secret_key m_k_view_incoming;
    carrot::view_incoming_key_ram_borrowed_device m_k_view_dev;

    carrot::CarrotEnoteV1 m_enote;
    carrot::encrypted_payment_id_t m_encrypted_payment_id;

    bool m_test_view_tag_check;
};

//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
