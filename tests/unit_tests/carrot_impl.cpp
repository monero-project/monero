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

#include "gtest/gtest.h"

#include <boost/multiprecision/cpp_int.hpp>

#include "carrot_core/account_secrets.h"
#include "carrot_core/address_utils.h"
#include "carrot_core/carrot_enote_scan.h"
#include "carrot_core/destination.h"
#include "carrot_core/device_ram_borrowed.h"
#include "carrot_core/enote_utils.h"
#include "carrot_core/output_set_finalization.h"
#include "carrot_core/payment_proposal.h"
#include "carrot_impl/carrot_tx_format_utils.h"
#include "common/container_helpers.h"
#include "crypto/generators.h"
#include "cryptonote_basic/account.h"
#include "cryptonote_basic/subaddress_index.h"
#include "ringct/rctOps.h"

using namespace carrot;

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
namespace
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static constexpr std::uint32_t MAX_SUBADDRESS_MAJOR_INDEX = 50;
static constexpr std::uint32_t MAX_SUBADDRESS_MINOR_INDEX = 200;
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
struct mock_carrot_or_legacy_keys
{
    bool is_carrot;

    crypto::secret_key s_master;
    crypto::secret_key k_prove_spend;
    crypto::secret_key s_view_balance;
    crypto::secret_key k_generate_image;
    crypto::secret_key k_view;
    crypto::secret_key s_generate_address;
    crypto::public_key account_spend_pubkey;
    crypto::public_key account_view_pubkey;
    crypto::public_key main_address_view_pubkey;

    cryptonote::account_base legacy_acb;

    view_incoming_key_ram_borrowed_device k_view_dev;
    view_balance_secret_ram_borrowed_device s_view_balance_dev;

    mock_carrot_or_legacy_keys(): k_view_dev(k_view), s_view_balance_dev(s_view_balance) {}

    void generate_carrot()
    {
        is_carrot = true;
        crypto::generate_random_bytes_thread_safe(sizeof(crypto::secret_key), to_bytes(s_master));
        make_carrot_provespend_key(s_master, k_prove_spend);
        make_carrot_viewbalance_secret(s_master, s_view_balance);
        make_carrot_generateimage_key(s_view_balance, k_generate_image);
        make_carrot_viewincoming_key(s_view_balance, k_view);
        make_carrot_generateaddress_secret(s_view_balance, s_generate_address);
        make_carrot_spend_pubkey(k_generate_image, k_prove_spend, account_spend_pubkey);
        account_view_pubkey = rct::rct2pk(rct::scalarmultKey(rct::pk2rct(account_spend_pubkey),
            rct::sk2rct(k_view)));
        main_address_view_pubkey = rct::rct2pk(rct::scalarmultBase(rct::sk2rct(k_view)));
    }

    void generate_legacy()
    {
        is_carrot = false;
        legacy_acb.generate();
        k_view = legacy_acb.get_keys().m_view_secret_key;
    }

    CarrotDestinationV1 cryptonote_address(const payment_id_t payment_id = null_payment_id) const
    {
        CarrotDestinationV1 addr;
        if (is_carrot)
        {
            make_carrot_integrated_address_v1(account_spend_pubkey,
                main_address_view_pubkey,
                payment_id,
                addr);
        }
        else
        {
            make_carrot_integrated_address_v1(legacy_acb.get_keys().m_account_address.m_spend_public_key,
                legacy_acb.get_keys().m_account_address.m_view_public_key,
                payment_id,
                addr);
        }
        return addr;
    }

    CarrotDestinationV1 subaddress(const uint32_t major_index, const uint32_t minor_index) const
    {
        if (!major_index && !minor_index)
            return cryptonote_address();

        CarrotDestinationV1 addr;
        if (is_carrot)
        {
            make_carrot_subaddress_v1(account_spend_pubkey,
                account_view_pubkey,
                s_generate_address,
                major_index,
                minor_index,
                addr);
        }
        else
        {
            const cryptonote::account_keys &ks = legacy_acb.get_keys();
            const cryptonote::account_public_address cnaddr =
                ks.m_device->get_subaddress(ks, {major_index, minor_index});
            addr = CarrotDestinationV1{
                .address_spend_pubkey = cnaddr.m_spend_public_key,
                .address_view_pubkey = cnaddr.m_view_public_key,
                .is_subaddress = true,
                .payment_id = null_payment_id
            };
        }
        return addr;
    }

    void get_output_enote_proposals_as_self_sender(std::vector<CarrotPaymentProposalV1> &&normal_payment_proposals,
        std::vector<CarrotPaymentProposalSelfSendV1> &&selfsend_payment_proposals,
        const crypto::key_image &tx_first_key_image,
        std::vector<RCTOutputEnoteProposal> &output_enote_proposals_out,
        encrypted_payment_id_t &encrypted_payment_id_out) const
    {
        const crypto::public_key &account_spend_pubkey = is_carrot
            ? this->account_spend_pubkey : legacy_acb.get_keys().m_account_address.m_spend_public_key;

        get_output_enote_proposals(std::forward<std::vector<CarrotPaymentProposalV1>>(normal_payment_proposals),
            std::forward<std::vector<CarrotPaymentProposalSelfSendV1>>(selfsend_payment_proposals),
            is_carrot ? &s_view_balance_dev : nullptr,
            is_carrot ? nullptr : &k_view_dev,
            account_spend_pubkey,
            tx_first_key_image,
            output_enote_proposals_out,
            encrypted_payment_id_out);
    }

    // brief: opening_for_subaddress - return (k^g_a, k^t_a) for j s.t. K^j_s = (k^g_a * G + k^t_a * T)
    void opening_for_subaddress(const uint32_t major_index,
        const uint32_t minor_index,
        crypto::secret_key &address_privkey_g_out,
        crypto::secret_key &address_privkey_t_out,
        crypto::public_key &address_spend_pubkey_out) const
    {
        const bool is_subaddress = major_index || minor_index;

        if (is_carrot)
        {
            // s^j_gen = H_32[s_ga](j_major, j_minor)
            crypto::secret_key address_index_generator;
            make_carrot_index_extension_generator(s_generate_address, minor_index, minor_index, address_index_generator);

            crypto::secret_key subaddress_scalar;
            if (is_subaddress)
            {
                // k^j_subscal = H_n(K_s, j_major, j_minor, s^j_gen)
                make_carrot_subaddress_scalar(account_spend_pubkey, address_index_generator, major_index, minor_index, subaddress_scalar);
            }
            else
            {
                subaddress_scalar.data[0] = 1;
            }

            // k^g_a = k_gi * k^j_subscal
            sc_mul(to_bytes(address_privkey_g_out), to_bytes(k_generate_image), to_bytes(subaddress_scalar));

            // k^t_a = k_ps * k^j_subscal
            sc_mul(to_bytes(address_privkey_t_out), to_bytes(k_prove_spend), to_bytes(subaddress_scalar));
        }
        else // legacy keys
        {
            // m = Hn(k_v || j_major || j_minor)
            const cryptonote::account_keys &ks = legacy_acb.get_keys();
            const crypto::secret_key subaddress_extension =
                ks.get_device().get_subaddress_secret_key(ks.m_view_secret_key, {major_index, minor_index});

            // k^g_a = k_s + m
            sc_add(to_bytes(address_privkey_g_out), to_bytes(ks.m_spend_secret_key), to_bytes(subaddress_extension));

            // k^t_a = 0
            memset(address_privkey_t_out.data, 0, sizeof(address_privkey_t_out));
        }

        // perform sanity check
        const CarrotDestinationV1 addr = subaddress(major_index, minor_index);
        rct::key recomputed_address_spend_pubkey;
        rct::addKeys2(recomputed_address_spend_pubkey,
            rct::sk2rct(address_privkey_g_out),
            rct::sk2rct(address_privkey_t_out),
            rct::pk2rct(crypto::get_T()));
        CHECK_AND_ASSERT_THROW_MES(rct::rct2pk(recomputed_address_spend_pubkey) == addr.address_spend_pubkey,
            "mock carrot or legacy keys: opening for subaddress: failed sanity check");
        address_spend_pubkey_out = addr.address_spend_pubkey;
    }

    bool try_searching_for_opening_for_subaddress(const crypto::public_key &address_spend_pubkey,
        const uint32_t max_major_index,
        const uint32_t max_minor_index,
        uint32_t major_index_out,
        uint32_t minor_index_out,
        crypto::secret_key &address_privkey_g_out,
        crypto::secret_key &address_privkey_t_out) const
    {
        // shittier version of a subaddress lookahead table

        for (major_index_out = 0; major_index_out < max_major_index; ++major_index_out)
        {
            for (minor_index_out = 0; minor_index_out < max_minor_index; ++minor_index_out)
            {
                crypto::public_key recomputed_address_spend_pubkey;
                opening_for_subaddress(major_index_out,
                    minor_index_out,
                    address_privkey_g_out,
                    address_privkey_t_out,
                    recomputed_address_spend_pubkey);
                if (address_spend_pubkey == recomputed_address_spend_pubkey)
                    return true;
            }
        }

        return false;
    }
};
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static bool can_open_fcmp_onetime_address(const crypto::secret_key &address_privkey_g,
    const crypto::secret_key &address_privkey_t,
    const crypto::secret_key &sender_extension_g,
    const crypto::secret_key &sender_extension_t,
    const crypto::public_key &onetime_address)
{
    rct::key combined_g;
    sc_add(combined_g.bytes, to_bytes(address_privkey_g), to_bytes(sender_extension_g));

    rct::key combined_t;
    sc_add(combined_t.bytes, to_bytes(address_privkey_t), to_bytes(sender_extension_t));

    // Ko' = combined_g G + combined_t T
    rct::key recomputed_onetime_address;
    rct::addKeys2(recomputed_onetime_address, combined_g, combined_t, rct::pk2rct(crypto::get_T()));

    // Ko' ?= Ko
    return recomputed_onetime_address == onetime_address;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
struct unittest_carrot_scan_result_t
{
    crypto::public_key address_spend_pubkey = rct::rct2pk(rct::I);
    crypto::secret_key sender_extension_g = rct::rct2sk(rct::I);
    crypto::secret_key sender_extension_t = rct::rct2sk(rct::I);

    rct::xmr_amount amount = 0;
    crypto::secret_key amount_blinding_factor = rct::rct2sk(rct::I);

    CarrotEnoteType enote_type = CarrotEnoteType::PAYMENT;

    payment_id_t payment_id = null_payment_id;

    janus_anchor_t internal_message = janus_anchor_t{};

    size_t output_index = 0;
};
static void unittest_scan_enote_set(const std::vector<CarrotEnoteV1> &enotes,
    const encrypted_payment_id_t encrypted_payment_id,
    const mock_carrot_or_legacy_keys keys,
    std::vector<unittest_carrot_scan_result_t> &res)
{
    res.clear();

    // for each enote...
    for (size_t output_index = 0; output_index < enotes.size(); ++output_index)
    {
        const CarrotEnoteV1 &enote = enotes.at(output_index);

        // s_sr = k_v D_e
        mx25519_pubkey s_sr;
        make_carrot_uncontextualized_shared_key_receiver(keys.k_view, enote.enote_ephemeral_pubkey, s_sr);

        // external scan
        unittest_carrot_scan_result_t scan_result{};
        bool r = try_scan_carrot_enote_external(enote,
            encrypted_payment_id,
            s_sr,
            keys.k_view_dev,
            keys.account_spend_pubkey,
            scan_result.sender_extension_g,
            scan_result.sender_extension_t,
            scan_result.address_spend_pubkey,
            scan_result.amount,
            scan_result.amount_blinding_factor,
            scan_result.payment_id,
            scan_result.enote_type);
        
        // internal scan
        r = r || try_scan_carrot_enote_internal(enote,
            keys.s_view_balance_dev,
            scan_result.sender_extension_g,
            scan_result.sender_extension_t,
            scan_result.address_spend_pubkey,
            scan_result.amount,
            scan_result.amount_blinding_factor,
            scan_result.enote_type,
            scan_result.internal_message);
        
        scan_result.output_index = output_index;

        if (r)
            res.push_back(scan_result);
    }
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static void unittest_scan_enote_set_multi_account(const std::vector<CarrotEnoteV1> &enotes,
    const encrypted_payment_id_t encrypted_payment_id,
    const epee::span<const mock_carrot_or_legacy_keys * const> accounts,
    std::vector<std::vector<unittest_carrot_scan_result_t>> &res)
{
    res.clear();
    res.reserve(accounts.size());

    for (const mock_carrot_or_legacy_keys *account : accounts)
        unittest_scan_enote_set(enotes, encrypted_payment_id, *account, tools::add_element(res));
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static bool compare_scan_result(const unittest_carrot_scan_result_t &scan_res,
    const CarrotPaymentProposalV1 &normal_payment_proposal)
{
    if (scan_res.address_spend_pubkey != normal_payment_proposal.destination.address_spend_pubkey)
        return false;
    
    if (scan_res.amount != normal_payment_proposal.amount)
        return false;
    
    if (scan_res.enote_type != CarrotEnoteType::PAYMENT)
        return false;
    
    if (scan_res.payment_id != normal_payment_proposal.destination.payment_id)
        return false;
    
    if (scan_res.internal_message != janus_anchor_t{})
        return false;
    
    return true;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static bool compare_scan_result(const unittest_carrot_scan_result_t &scan_res,
    const CarrotPaymentProposalSelfSendV1 &selfsend_payment_proposal)
{
    if (scan_res.address_spend_pubkey != selfsend_payment_proposal.destination_address_spend_pubkey)
        return false;
    
    if (scan_res.amount != selfsend_payment_proposal.amount)
        return false;
    
    if (scan_res.enote_type != selfsend_payment_proposal.enote_type)
        return false;
    
    if (scan_res.payment_id != null_payment_id)
        return false;
    
    if (scan_res.internal_message != selfsend_payment_proposal.internal_message.value_or(janus_anchor_t{}))
        return false;

    return true;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
struct unittest_transaction_proposal
{
    using per_account = std::pair<mock_carrot_or_legacy_keys, std::vector<CarrotPaymentProposalV1>>;
    using per_input = std::pair<crypto::key_image, rct::xmr_amount>;

    std::vector<per_account> per_account_payments;
    std::vector<CarrotPaymentProposalSelfSendV1> explicit_selfsend_proposals;
    size_t self_sender_index{0};
    rct::xmr_amount fee;
    std::vector<per_input> inputs;

    tools::optional_variant<CarrotPaymentProposalV1, CarrotPaymentProposalSelfSendV1>
        get_additional_output_proposal() const
    {
        boost::multiprecision::uint128_t input_sum = 0;
        for (const per_input &input : inputs)
            input_sum += input.second;

        CHECK_AND_ASSERT_THROW_MES(inputs.size(), "we need at least one input");
        CHECK_AND_ASSERT_THROW_MES(per_account_payments.at(self_sender_index).second.empty(),
            "self-sender shouldn't contain any normal payment proposals in their own tx");
        
        input_context_t input_context;
        make_carrot_input_context(inputs.front().first, input_context);

        size_t num_payment_proposals = 0;
        boost::multiprecision::uint128_t output_sum = fee;
        bool has_payment_selfsend = false;
        mx25519_pubkey other_enote_ephemeral_pubkey;
        for (const per_account &per_acc : per_account_payments)
        {
            for (const CarrotPaymentProposalV1 &payment_proposal : per_acc.second)
            {
                output_sum += payment_proposal.amount;
                other_enote_ephemeral_pubkey = get_enote_ephemeral_pubkey(payment_proposal, input_context);
                num_payment_proposals++;
            }
        }
        for (const CarrotPaymentProposalSelfSendV1 &selfsend_proposal : explicit_selfsend_proposals)
        {
            output_sum += selfsend_proposal.amount;
            other_enote_ephemeral_pubkey = selfsend_proposal.enote_ephemeral_pubkey;
            if (selfsend_proposal.enote_type == CarrotEnoteType::PAYMENT)
                has_payment_selfsend = true;
        }

        CHECK_AND_ASSERT_THROW_MES(input_sum >= output_sum, "not enough funds");

        const rct::xmr_amount remaining_change = boost::numeric_cast<rct::xmr_amount>(input_sum - output_sum);

        return carrot::get_additional_output_proposal(num_payment_proposals,
            explicit_selfsend_proposals.size(),
            remaining_change,
            has_payment_selfsend,
            per_account_payments.at(self_sender_index).first.cryptonote_address().address_spend_pubkey,
            other_enote_ephemeral_pubkey);
    }

    void finalize_payment_proposals(std::vector<CarrotPaymentProposalV1> &normal_payment_proposals_out,
        std::vector<CarrotPaymentProposalSelfSendV1> &selfsend_payment_proposals_out) const 
    {
        for (const per_account &pa : per_account_payments)
        {
            normal_payment_proposals_out.insert(normal_payment_proposals_out.end(), 
                pa.second.cbegin(),
                pa.second.cend());
        }

        selfsend_payment_proposals_out = explicit_selfsend_proposals;

        const auto additional_proposal = get_additional_output_proposal();
        struct additional_proposal_visitor
        {
            void operator()(boost::blank) {}
            void operator()(const CarrotPaymentProposalV1 &p) { normal_payment_proposals_out.push_back(p); }
            void operator()(const CarrotPaymentProposalSelfSendV1 &p) { selfsend_payment_proposals_out.push_back(p); }

            std::vector<CarrotPaymentProposalV1> &normal_payment_proposals_out;
            std::vector<CarrotPaymentProposalSelfSendV1> &selfsend_payment_proposals_out;
        };

        additional_proposal.visit(additional_proposal_visitor{normal_payment_proposals_out, selfsend_payment_proposals_out});
    }
};
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
} // namespace
static inline bool operator==(const mx25519_pubkey &a, const mx25519_pubkey &b)
{
    return 0 == memcmp(&a, &b, sizeof(mx25519_pubkey));
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static void subtest_multi_account_transfer_over_transaction(const unittest_transaction_proposal &tx_proposal)
{
    // finalize payment proposals
    std::vector<CarrotPaymentProposalV1> normal_payment_proposals;
    std::vector<CarrotPaymentProposalSelfSendV1> selfsend_payment_proposals;
    tx_proposal.finalize_payment_proposals(normal_payment_proposals, selfsend_payment_proposals);

    // convert to enotes and pid_enc
    std::vector<RCTOutputEnoteProposal> enote_output_proposals;
    encrypted_payment_id_t encrypted_payment_id;
    tx_proposal.per_account_payments.at(tx_proposal.self_sender_index).first.get_output_enote_proposals_as_self_sender(
        std::move(normal_payment_proposals),                                       // move
        std::vector<CarrotPaymentProposalSelfSendV1>(selfsend_payment_proposals),  // copy (tested later)
        tx_proposal.inputs.at(0).first,
        enote_output_proposals,
        encrypted_payment_id);

    // collect enotes
    std::vector<CarrotEnoteV1> enotes;
    for (const RCTOutputEnoteProposal &oep : enote_output_proposals)
        enotes.push_back(oep.enote);

    // collect key images
    std::vector<crypto::key_image> key_images;
    for (const auto &i : tx_proposal.inputs)
        key_images.push_back(i.first);

    // stuff carrot info into tx
    const cryptonote::transaction tx = store_carrot_to_transaction_v1(enotes,
        key_images,
        tx_proposal.fee,
        encrypted_payment_id);

    // load carrot stuff from tx
    std::vector<CarrotEnoteV1> parsed_enotes;
    std::vector<crypto::key_image> parsed_key_images;
    rct::xmr_amount parsed_fee;
    std::optional<encrypted_payment_id_t> parsed_encrypted_payment_id;
    ASSERT_TRUE(try_load_carrot_from_transaction_v1(tx,
        parsed_enotes,
        parsed_key_images,
        parsed_fee,
        parsed_encrypted_payment_id));

    // check loaded carrot stuff == stored carrot stuff
    EXPECT_EQ(enotes, parsed_enotes);
    EXPECT_EQ(key_images, parsed_key_images);
    EXPECT_EQ(tx_proposal.fee, parsed_fee);
    ASSERT_TRUE(parsed_encrypted_payment_id);
    EXPECT_EQ(encrypted_payment_id, *parsed_encrypted_payment_id);

    // collect accounts
    std::vector<const mock_carrot_or_legacy_keys*> accounts;
    for (const auto &pa : tx_proposal.per_account_payments)
        accounts.push_back(&pa.first);

    // do scanning of all accounts on every enotes
    std::vector<std::vector<unittest_carrot_scan_result_t>> scan_results;
    unittest_scan_enote_set_multi_account(parsed_enotes,
        *parsed_encrypted_payment_id,
        epee::to_span(accounts),
        scan_results);

    // assert properties of finalized selfsend payment proposals as compared to explicit selfsend payment proposals
    ASSERT_GE(selfsend_payment_proposals.size(), tx_proposal.explicit_selfsend_proposals.size());
    for (size_t i = 0; i < tx_proposal.explicit_selfsend_proposals.size(); ++i)
    {
        EXPECT_EQ(tx_proposal.explicit_selfsend_proposals.at(i), selfsend_payment_proposals.at(i));
    }

    // check that the scan results for each account match the corresponding payment proposals for each account
    // also check that the accounts can each open their corresponding onetime outut pubkeys
    ASSERT_EQ(scan_results.size(), accounts.size());
    for (size_t account_idx = 0; account_idx < accounts.size(); ++account_idx)
    {
        const std::vector<unittest_carrot_scan_result_t> &account_scan_results = scan_results.at(account_idx);
        if (account_idx == tx_proposal.self_sender_index)
        {
            ASSERT_EQ(selfsend_payment_proposals.size(), account_scan_results.size());
            for (const unittest_carrot_scan_result_t &single_scan_res : account_scan_results)
            {
                bool matched_payment = false;
                for (const CarrotPaymentProposalSelfSendV1 &account_payment_proposal : selfsend_payment_proposals)
                {
                    if (compare_scan_result(single_scan_res, account_payment_proposal))
                    {
                        crypto::secret_key address_privkey_g;
                        crypto::secret_key address_privkey_t;
                        uint32_t _1{}, _2{};
                        EXPECT_TRUE(accounts.at(account_idx)->try_searching_for_opening_for_subaddress(
                            single_scan_res.address_spend_pubkey,
                            MAX_SUBADDRESS_MAJOR_INDEX,
                            MAX_SUBADDRESS_MINOR_INDEX,
                            _1,
                            _2,
                            address_privkey_g,
                            address_privkey_t));

                        EXPECT_TRUE(can_open_fcmp_onetime_address(address_privkey_g,
                            address_privkey_t,
                            single_scan_res.sender_extension_g,
                            single_scan_res.sender_extension_t,
                            parsed_enotes.at(single_scan_res.output_index).onetime_address));

                        matched_payment = true;
                        break;
                    }
                }
                EXPECT_TRUE(matched_payment);
            }
        }
        else
        {
            const std::vector<CarrotPaymentProposalV1> &account_payment_proposals = tx_proposal.per_account_payments.at(account_idx).second;
            ASSERT_EQ(account_payment_proposals.size(), account_scan_results.size());
            for (const unittest_carrot_scan_result_t &single_scan_res : account_scan_results)
            {
                bool matched_payment = false;
                for (const CarrotPaymentProposalV1 &account_payment_proposal : account_payment_proposals)
                {
                    if (compare_scan_result(single_scan_res, account_payment_proposal))
                    {
                        crypto::secret_key address_privkey_g;
                        crypto::secret_key address_privkey_t;
                        uint32_t _1{}, _2{};
                        EXPECT_TRUE(accounts.at(account_idx)->try_searching_for_opening_for_subaddress(
                            single_scan_res.address_spend_pubkey,
                            MAX_SUBADDRESS_MAJOR_INDEX,
                            MAX_SUBADDRESS_MINOR_INDEX,
                            _1,
                            _2,
                            address_privkey_g,
                            address_privkey_t));

                        EXPECT_TRUE(can_open_fcmp_onetime_address(address_privkey_g,
                            address_privkey_t,
                            single_scan_res.sender_extension_g,
                            single_scan_res.sender_extension_t,
                            parsed_enotes.at(single_scan_res.output_index).onetime_address));

                        matched_payment = true;
                        break;
                    }
                }
                EXPECT_TRUE(matched_payment);
            }
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_impl, multi_account_transfer_over_transaction_1)
{
    // two accounts, both carrot
    // 1/2 tx
    // 1 normal payment to main address
    // 0 explicit selfsend payments

    unittest_transaction_proposal tx_proposal;
    tx_proposal.per_account_payments.resize(2);
    mock_carrot_or_legacy_keys &acc0 = tx_proposal.per_account_payments[0].first;
    mock_carrot_or_legacy_keys &acc1 = tx_proposal.per_account_payments[1].first;
    acc0.generate_carrot();
    acc1.generate_carrot();

    // 1 normal payment
    CarrotPaymentProposalV1 &normal_payment_proposal = tools::add_element( tx_proposal.per_account_payments[0].second);
    normal_payment_proposal = CarrotPaymentProposalV1{
        .destination = acc0.cryptonote_address(),
        .amount = crypto::rand_idx((rct::xmr_amount) 1ull << 63),
        .randomness = gen_janus_anchor()
    };

    // specify self-sender
    tx_proposal.self_sender_index = 1;

    // specify input
    tx_proposal.inputs.emplace_back(rct::rct2ki(rct::pkGen()), normal_payment_proposal.amount | (1ull << 63));

    // specify fee
    tx_proposal.fee = 3853481201;

    // test
    subtest_multi_account_transfer_over_transaction(tx_proposal);
}
//----------------------------------------------------------------------------------------------------------------------
