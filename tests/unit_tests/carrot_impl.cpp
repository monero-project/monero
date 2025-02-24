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
#include "carrot_impl/carrot_tx_builder_inputs.h"
#include "carrot_impl/carrot_tx_builder_utils.h"
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
static constexpr std::uint32_t MAX_SUBADDRESS_MAJOR_INDEX = 5;
static constexpr std::uint32_t MAX_SUBADDRESS_MINOR_INDEX = 20;
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
    generate_address_secret_ram_borrowed_device s_generate_address_dev;

    std::unordered_map<crypto::public_key, cryptonote::subaddress_index> subaddress_map;

    mock_carrot_or_legacy_keys(): k_view_dev(k_view),
        s_view_balance_dev(s_view_balance),
        s_generate_address_dev(s_generate_address)
    {}

    const view_balance_secret_device* get_view_balance_device() const
    {
        return is_carrot ? &s_view_balance_dev : nullptr;
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
                s_generate_address_dev,
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
            make_carrot_index_extension_generator(s_generate_address, major_index, minor_index, address_index_generator);

            crypto::secret_key subaddress_scalar{};
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
            // m = Hn(k_v || j_major || j_minor) if subaddress else 0
            const cryptonote::account_keys &ks = legacy_acb.get_keys();
            const crypto::secret_key subaddress_extension = is_subaddress
                ? ks.get_device().get_subaddress_secret_key(ks.m_view_secret_key, {major_index, minor_index})
                : crypto::null_skey;

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
        const auto it = subaddress_map.find(address_spend_pubkey);
        if (it == subaddress_map.cend())
            return false;

        crypto::public_key recomputed_address_spend_pubkey;
        opening_for_subaddress(it->second.major,
            it->second.minor,
            address_privkey_g_out,
            address_privkey_t_out,
            recomputed_address_spend_pubkey);

        return address_spend_pubkey == recomputed_address_spend_pubkey;
    }

    void generate_subaddress_map()
    {
        for (uint32_t major_index = 0; major_index < MAX_SUBADDRESS_MAJOR_INDEX; ++major_index)
        {
            for (uint32_t minor_index = 0; minor_index < MAX_SUBADDRESS_MINOR_INDEX; ++minor_index)
            {
                const CarrotDestinationV1 addr = subaddress(major_index, minor_index);
                subaddress_map.insert({addr.address_spend_pubkey, {major_index, minor_index}});
            }
        }
    }

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

        generate_subaddress_map();
    }

    void generate_legacy()
    {
        is_carrot = false;
        legacy_acb.generate();
        k_view = legacy_acb.get_keys().m_view_secret_key;
        account_spend_pubkey = legacy_acb.get_keys().m_account_address.m_spend_public_key;
        main_address_view_pubkey = legacy_acb.get_keys().m_account_address.m_view_public_key;

        generate_subaddress_map();
    }
};
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static crypto::key_image gen_key_image()
{
    return rct::rct2ki(rct::pkGen());
}
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
    const CarrotPaymentProposalV1 &normal_payment_proposal,
    const rct::xmr_amount allowed_fee_margin_opt = 0)
{
    if (scan_res.address_spend_pubkey != normal_payment_proposal.destination.address_spend_pubkey)
        return false;

    if (scan_res.amount > normal_payment_proposal.amount)
        return false;

    if (normal_payment_proposal.amount - scan_res.amount > allowed_fee_margin_opt)
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
    const CarrotPaymentProposalSelfSendV1 &selfsend_payment_proposal,
    const rct::xmr_amount allowed_fee_margin_opt = 0)
{
    if (scan_res.address_spend_pubkey != selfsend_payment_proposal.destination_address_spend_pubkey)
        return false;

    if (scan_res.amount > selfsend_payment_proposal.amount)
        return false;

    if (selfsend_payment_proposal.amount - scan_res.amount > allowed_fee_margin_opt)
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
struct unittest_transaction_preproposal
{
    using per_payment_proposal = std::pair<CarrotPaymentProposalV1, /*is subtractble?*/bool>;
    using per_ss_payment_proposal = std::pair<CarrotPaymentProposalVerifiableSelfSendV1, /*is subtractble?*/bool>;
    using per_account = std::pair<mock_carrot_or_legacy_keys, std::vector<per_payment_proposal>>;
    using per_input = std::pair<crypto::key_image, rct::xmr_amount>;

    std::vector<per_account> per_account_payments;
    std::vector<per_ss_payment_proposal> explicit_selfsend_proposals;
    size_t self_sender_index{0};
    rct::xmr_amount fee_per_weight;
    std::vector<std::uint8_t> extra_extra;

    void get_flattened_payment_proposals(std::vector<CarrotPaymentProposalV1> &normal_payment_proposals_out,
        std::vector<CarrotPaymentProposalVerifiableSelfSendV1> &selfsend_payment_proposals_out,
        std::set<size_t> &subtractable_normal_payment_proposals,
        std::set<size_t> &subtractable_selfsend_payment_proposals) const
    {
        size_t norm_idx = 0;
        for (const per_account &pa : per_account_payments)
        {
            for (const per_payment_proposal &ppp : pa.second)
            {
                normal_payment_proposals_out.push_back(ppp.first);
                if (ppp.second)
                    subtractable_normal_payment_proposals.insert(norm_idx);

                norm_idx++;
            }
        }

        for (size_t ss_idx = 0; ss_idx < explicit_selfsend_proposals.size(); ++ss_idx)
        {
            const per_ss_payment_proposal &pspp = explicit_selfsend_proposals.at(ss_idx);
            selfsend_payment_proposals_out.push_back(pspp.first);
            if (pspp.second)
                subtractable_selfsend_payment_proposals.insert(ss_idx);
        }
    }
};
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
select_inputs_func_t make_fake_input_selection_callback(size_t num_ins = 0)
{
    return [num_ins](const boost::multiprecision::int128_t &nominal_output_sum,
        const std::map<std::size_t, rct::xmr_amount> &fee_per_input_count,
        size_t,
        size_t,
        std::vector<CarrotSelectedInput> &selected_inputs)
    {
        const size_t nins = num_ins ? num_ins : 1;
        selected_inputs.clear();
        selected_inputs.reserve(nins);

        const rct::xmr_amount fee = fee_per_input_count.at(nins);
        rct::xmr_amount in_amount_sum_64 = boost::numeric_cast<rct::xmr_amount>(nominal_output_sum + fee);

        for (size_t i = 0; i < nins - 1; ++i)
        {
            const rct::xmr_amount current_in_amount = in_amount_sum_64 ? crypto::rand_idx(in_amount_sum_64) : 0;
            const crypto::key_image current_key_image = rct::rct2ki(rct::pkGen());
            selected_inputs.push_back({current_in_amount, current_key_image});
            in_amount_sum_64 -= current_in_amount;
        }

        selected_inputs.push_back({in_amount_sum_64, rct::rct2ki(rct::pkGen())});
    };
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
} // namespace
static inline bool operator==(const mx25519_pubkey &a, const mx25519_pubkey &b)
{
    return 0 == memcmp(&a, &b, sizeof(mx25519_pubkey));
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static void subtest_multi_account_transfer_over_transaction(const unittest_transaction_preproposal &tx_preproposal)
{
    // get payment proposals
    std::vector<CarrotPaymentProposalV1> normal_payment_proposals;
    std::vector<CarrotPaymentProposalVerifiableSelfSendV1> selfsend_payment_proposals;
    std::set<size_t> subtractable_normal_payment_proposals;
    std::set<size_t> subtractable_selfsend_payment_proposals;
    tx_preproposal.get_flattened_payment_proposals(normal_payment_proposals,
        selfsend_payment_proposals,
        subtractable_normal_payment_proposals,
        subtractable_selfsend_payment_proposals);

    // get self-sender account
    const mock_carrot_or_legacy_keys &ss_keys =
    tx_preproposal.per_account_payments.at(tx_preproposal.self_sender_index).first;

    // make transaction proposal
    CarrotTransactionProposalV1 tx_proposal;
    make_carrot_transaction_proposal_v1_transfer(normal_payment_proposals,
        selfsend_payment_proposals,
        tx_preproposal.fee_per_weight,
        tx_preproposal.extra_extra,
        make_fake_input_selection_callback(),
        ss_keys.get_view_balance_device(),
        &ss_keys.k_view_dev,
        ss_keys.account_spend_pubkey,
        tx_proposal);

    // make unsigned transaction
    cryptonote::transaction tx;
    make_pruned_transaction_from_carrot_proposal_v1(tx_proposal,
        ss_keys.get_view_balance_device(),
        &ss_keys.k_view_dev,
        ss_keys.account_spend_pubkey,
        tx);

    // calculate acceptable fee margin between proposed amount and actual amount for subtractable outputs
    const size_t num_subtractable = subtractable_normal_payment_proposals.size() +
        subtractable_selfsend_payment_proposals.size();
    const rct::xmr_amount acceptable_fee_margin = num_subtractable
        ? (tx.rct_signatures.txnFee / num_subtractable) + 1
        : 0;

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
    ASSERT_TRUE(parsed_encrypted_payment_id);

    // collect modified selfsend payment proposal cores
    std::vector<CarrotPaymentProposalSelfSendV1> modified_selfsend_payment_proposals;
    for (const auto &p : tx_proposal.selfsend_payment_proposals)
        modified_selfsend_payment_proposals.push_back(p.proposal);

    // sanity check that the enotes and pid_enc loaded from the transaction are equal to the enotes
    // and pic_enc returned from get_output_enote_proposals() when called with the modified payment
    // proposals. we do this so that the modified payment proposals from make_unsigned_transaction()
    // can be passed to a hardware device for deterministic verification of the signable tx hash
    std::vector<RCTOutputEnoteProposal> rederived_output_enote_proposals;
    encrypted_payment_id_t rederived_encrypted_payment_id;
    get_output_enote_proposals(tx_proposal.normal_payment_proposals,
        modified_selfsend_payment_proposals,
        *parsed_encrypted_payment_id,
        ss_keys.get_view_balance_device(),
        &ss_keys.k_view_dev,
        ss_keys.account_spend_pubkey,
        parsed_key_images.at(0),
        rederived_output_enote_proposals,
        rederived_encrypted_payment_id);
    EXPECT_EQ(*parsed_encrypted_payment_id, rederived_encrypted_payment_id);
    ASSERT_EQ(parsed_enotes.size(), rederived_output_enote_proposals.size());
    for (size_t enote_idx = 0; enote_idx < parsed_enotes.size(); ++enote_idx)
    {
        EXPECT_EQ(parsed_enotes.at(enote_idx), rederived_output_enote_proposals.at(enote_idx).enote);
    }

    // collect accounts
    std::vector<const mock_carrot_or_legacy_keys*> accounts;
    for (const auto &pa : tx_preproposal.per_account_payments)
        accounts.push_back(&pa.first);

    // do scanning of all accounts on every enotes
    std::vector<std::vector<unittest_carrot_scan_result_t>> scan_results;
    unittest_scan_enote_set_multi_account(parsed_enotes,
        *parsed_encrypted_payment_id,
        epee::to_span(accounts),
        scan_results);

    // check that the scan results for each *normal* account match the corresponding payment
    // proposals for each account. also check that the accounts can each open their corresponding
    // onetime outut pubkeys
    ASSERT_EQ(scan_results.size(), accounts.size());
    // for each normal account...
    for (size_t account_idx = 0; account_idx < accounts.size(); ++account_idx)
    {
        // skip self-sender account
        if (account_idx == tx_preproposal.self_sender_index)
            continue;

        const std::vector<unittest_carrot_scan_result_t> &account_scan_results = scan_results.at(account_idx);
        const auto &account_payment_proposals = tx_preproposal.per_account_payments.at(account_idx).second;
        ASSERT_EQ(account_payment_proposals.size(), account_scan_results.size());
        std::set<size_t> matched_payment_proposals;

        // for each scan result assigned to this account...
        for (const unittest_carrot_scan_result_t &single_scan_res : account_scan_results)
        {
            // for each normal payment proposal to this account...
            for (size_t norm_prop_idx = 0; norm_prop_idx < account_payment_proposals.size(); ++norm_prop_idx)
            {
                // calculate acceptable loss from fee subtraction
                const CarrotPaymentProposalV1 &account_payment_proposal = account_payment_proposals.at(norm_prop_idx).first;
                const bool is_subtractable = subtractable_normal_payment_proposals.count(norm_prop_idx);
                const rct::xmr_amount acceptable_fee_margin_for_proposal = is_subtractable ? acceptable_fee_margin : 0;

                // if the scan result matches the payment proposal...
                if (compare_scan_result(single_scan_res, account_payment_proposal, acceptable_fee_margin_for_proposal))
                {
                    // try searching for subaddress opening
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

                    // try opening Ko
                    EXPECT_TRUE(can_open_fcmp_onetime_address(address_privkey_g,
                        address_privkey_t,
                        single_scan_res.sender_extension_g,
                        single_scan_res.sender_extension_t,
                        parsed_enotes.at(single_scan_res.output_index).onetime_address));

                    // if this payment proposal isn't already marked as scanned, mark as scanned
                    if (!matched_payment_proposals.count(norm_prop_idx))
                    {
                        matched_payment_proposals.insert(norm_prop_idx);
                        break;
                    }
                }
            }
        }
        // check that the number of matched payment proposals is equal to the original number of them
        // doing it this way checks that the same payment proposal isn't marked twice and another left out
        EXPECT_EQ(account_payment_proposals.size(), matched_payment_proposals.size());
    }

    // check that the scan results for the selfsend account match the corresponding payment
    // proposals. also check that the accounts can each open their corresponding onetime outut pubkeys
    const std::vector<unittest_carrot_scan_result_t> &account_scan_results = scan_results.at(tx_preproposal.self_sender_index);
    ASSERT_EQ(selfsend_payment_proposals.size() + 1, account_scan_results.size());
    std::set<size_t> matched_payment_proposals;
    const unittest_carrot_scan_result_t* implicit_change_scan_res = nullptr;
    // for each scan result assigned to the self-sender account...
    for (const unittest_carrot_scan_result_t &single_scan_res : account_scan_results)
    {
        bool matched_payment = false;
        // for each self-send payment proposal...
        for (size_t ss_prop_idx = 0; ss_prop_idx < selfsend_payment_proposals.size(); ++ss_prop_idx)
        {
            // calculate acceptable loss from fee subtraction
            const CarrotPaymentProposalSelfSendV1 &account_payment_proposal = selfsend_payment_proposals.at(ss_prop_idx).proposal;
            const bool is_subtractable = subtractable_selfsend_payment_proposals.count(ss_prop_idx);
            const rct::xmr_amount acceptable_fee_margin_for_proposal = is_subtractable ? acceptable_fee_margin : 0;

            // if the scan result matches the payment proposal...
            if (compare_scan_result(single_scan_res, account_payment_proposal, acceptable_fee_margin_for_proposal))
            {
                // try searching for subaddress opening
                crypto::secret_key address_privkey_g;
                crypto::secret_key address_privkey_t;
                uint32_t _1{}, _2{};
                EXPECT_TRUE(ss_keys.try_searching_for_opening_for_subaddress(
                    single_scan_res.address_spend_pubkey,
                    MAX_SUBADDRESS_MAJOR_INDEX,
                    MAX_SUBADDRESS_MINOR_INDEX,
                    _1,
                    _2,
                    address_privkey_g,
                    address_privkey_t));

                // try opening Ko
                EXPECT_TRUE(can_open_fcmp_onetime_address(address_privkey_g,
                    address_privkey_t,
                    single_scan_res.sender_extension_g,
                    single_scan_res.sender_extension_t,
                    parsed_enotes.at(single_scan_res.output_index).onetime_address));

                // if this payment proposal isn't already marked as scanned, mark as scanned
                if (!matched_payment_proposals.count(ss_prop_idx))
                {
                    matched_payment = true;
                    matched_payment_proposals.insert(ss_prop_idx);
                    break;
                }
            }
        }

        // if this scan result has no matching payment...
        if (!matched_payment)
        {
            EXPECT_EQ(nullptr, implicit_change_scan_res); // only one non-matched scan result is allowed
            implicit_change_scan_res = &single_scan_res; // save the implicit change scan result for later 
        }
    }
    EXPECT_EQ(selfsend_payment_proposals.size(), matched_payment_proposals.size());
    EXPECT_NE(nullptr, implicit_change_scan_res);
    // @TODO: assert properties of `implicit_change_scan_res`
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_impl, multi_account_transfer_over_transaction_1)
{
    // two accounts, both carrot
    // 1/2 tx
    // 1 normal payment to main address
    // 0 explicit selfsend payments

    unittest_transaction_preproposal tx_proposal;
    tx_proposal.per_account_payments.resize(2);
    mock_carrot_or_legacy_keys &acc0 = tx_proposal.per_account_payments[0].first;
    mock_carrot_or_legacy_keys &acc1 = tx_proposal.per_account_payments[1].first;
    acc0.generate_carrot();
    acc1.generate_carrot();

    // 1 normal payment
    CarrotPaymentProposalV1 &normal_payment_proposal = tools::add_element( tx_proposal.per_account_payments[0].second).first;
    normal_payment_proposal = CarrotPaymentProposalV1{
        .destination = acc0.cryptonote_address(),
        .amount = crypto::rand_idx((rct::xmr_amount) 1ull << 63),
        .randomness = gen_janus_anchor()
    };

    // specify self-sender
    tx_proposal.self_sender_index = 1;

    // specify fee per weight
    tx_proposal.fee_per_weight = 20250510;

    // test
    subtest_multi_account_transfer_over_transaction(tx_proposal);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_impl, multi_account_transfer_over_transaction_2)
{
    // four accounts, all carrot
    // 1/4 tx
    // 1 normal payment to main address, integrated address, and subaddress each
    // 0 explicit selfsend payments

    unittest_transaction_preproposal tx_proposal;
    tx_proposal.per_account_payments.resize(4);
    auto &acc0 = tx_proposal.per_account_payments[0];
    auto &acc1 = tx_proposal.per_account_payments[1];
    auto &acc2 = tx_proposal.per_account_payments[2];
    auto &acc3 = tx_proposal.per_account_payments[3];
    acc0.first.generate_carrot();
    acc1.first.generate_carrot();
    acc2.first.generate_carrot();
    acc3.first.generate_carrot();

    // specify self-sender
    tx_proposal.self_sender_index = 2;

    // 1 subaddress payment
    tools::add_element(acc0.second).first = CarrotPaymentProposalV1{
        .destination = acc0.first.subaddress(2, 3),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    };

    // 1 main address payment
    tools::add_element(acc1.second).first = CarrotPaymentProposalV1{
        .destination = acc1.first.cryptonote_address(),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    };

    // 1 integrated address payment
    tools::add_element(acc3.second).first = CarrotPaymentProposalV1{
        .destination = acc3.first.cryptonote_address(gen_payment_id()),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    };

    // specify fee per weight
    tx_proposal.fee_per_weight = 314159;

    // test
    subtest_multi_account_transfer_over_transaction(tx_proposal);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_impl, multi_account_transfer_over_transaction_3)
{
    // four accounts, all carrot
    // 1/6 tx
    // 2 normal payment to main address, 1 integrated address, and 2 subaddress, each copied except integrated
    // 0 explicit selfsend payments

    unittest_transaction_preproposal tx_proposal;
    tx_proposal.per_account_payments.resize(4);
    auto &acc0 = tx_proposal.per_account_payments[0];
    auto &acc1 = tx_proposal.per_account_payments[1];
    auto &acc2 = tx_proposal.per_account_payments[2];
    auto &acc3 = tx_proposal.per_account_payments[3];
    acc0.first.generate_carrot();
    acc1.first.generate_carrot();
    acc2.first.generate_carrot();
    acc3.first.generate_carrot();

    // specify self-sender
    tx_proposal.self_sender_index = 2;

    // 2 subaddress payment
    tools::add_element(acc0.second).first = CarrotPaymentProposalV1{
        .destination = acc0.first.subaddress(2, 3),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    };
    acc0.second.push_back(acc0.second.front());
    acc0.second.back().first.randomness = gen_janus_anchor(); //mangle anchor_norm

    // 2 main address payment
    tools::add_element(acc1.second).first = CarrotPaymentProposalV1{
        .destination = acc1.first.cryptonote_address(),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    };
    acc1.second.push_back(acc1.second.front());
    acc1.second.back().first.randomness = gen_janus_anchor(); //mangle anchor_norm

    // 1 integrated address payment
    tools::add_element(acc3.second).first = CarrotPaymentProposalV1{
        .destination = acc3.first.cryptonote_address(gen_payment_id()),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    };

    // specify fee per weight
    tx_proposal.fee_per_weight = 314159;

    // test
    subtest_multi_account_transfer_over_transaction(tx_proposal);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_impl, multi_account_transfer_over_transaction_4)
{
    // four accounts, all carrot
    // 1/8 tx
    // 2 normal payment to main address, 1 integrated address, and 2 subaddress, each copied except integrated
    // 2 explicit selfsend payments: 1 main address destination, 1 subaddress destination

    unittest_transaction_preproposal tx_proposal;
    tx_proposal.per_account_payments.resize(4);
    auto &acc0 = tx_proposal.per_account_payments[0];
    auto &acc1 = tx_proposal.per_account_payments[1];
    auto &acc2 = tx_proposal.per_account_payments[2];
    auto &acc3 = tx_proposal.per_account_payments[3];
    acc0.first.generate_carrot();
    acc1.first.generate_carrot();
    acc2.first.generate_carrot();
    acc3.first.generate_carrot();

    // specify self-sender
    tx_proposal.self_sender_index = 2;

    // 2 subaddress payment
    tools::add_element(acc0.second).first = CarrotPaymentProposalV1{
        .destination = acc0.first.subaddress(2, 3),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    };
    acc0.second.push_back(acc0.second.front());
    acc0.second.back().first.randomness = gen_janus_anchor(); //mangle anchor_norm

    // 2 main address payment
    tools::add_element(acc1.second).first = CarrotPaymentProposalV1{
        .destination = acc1.first.cryptonote_address(),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    };
    acc1.second.push_back(acc1.second.front());
    acc1.second.back().first.randomness = gen_janus_anchor(); //mangle anchor_norm

    // 1 integrated address payment
    tools::add_element(acc3.second).first = CarrotPaymentProposalV1{
        .destination = acc3.first.cryptonote_address(gen_payment_id()),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    };

    // 1 main address selfsend
    tools::add_element(tx_proposal.explicit_selfsend_proposals).first.proposal = CarrotPaymentProposalSelfSendV1{
        .destination_address_spend_pubkey = acc2.first.account_spend_pubkey,
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .enote_type = CarrotEnoteType::PAYMENT,
        .internal_message = gen_janus_anchor()
    };

    // 1 subaddress selfsend
    tools::add_element(tx_proposal.explicit_selfsend_proposals).first = CarrotPaymentProposalVerifiableSelfSendV1{
        .proposal = CarrotPaymentProposalSelfSendV1{
            .destination_address_spend_pubkey = acc2.first.subaddress(4, 19).address_spend_pubkey,
            .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
            .enote_type = CarrotEnoteType::CHANGE
        },
        .subaddr_index = {4, 19}
    };

    // specify fee per weight
    tx_proposal.fee_per_weight = 314159;

    // test
    subtest_multi_account_transfer_over_transaction(tx_proposal);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_impl, multi_account_transfer_over_transaction_5)
{
    // two accounts, both legacy
    // 1/2 tx
    // 1 normal payment to main address
    // 0 explicit selfsend payments

    unittest_transaction_preproposal tx_proposal;
    tx_proposal.per_account_payments.resize(2);
    mock_carrot_or_legacy_keys &acc0 = tx_proposal.per_account_payments[0].first;
    mock_carrot_or_legacy_keys &acc1 = tx_proposal.per_account_payments[1].first;
    acc0.generate_legacy();
    acc1.generate_legacy();

    // 1 normal payment
    CarrotPaymentProposalV1 &normal_payment_proposal = tools::add_element( tx_proposal.per_account_payments[0].second).first;
    normal_payment_proposal = CarrotPaymentProposalV1{
        .destination = acc0.cryptonote_address(),
        .amount = crypto::rand_idx((rct::xmr_amount) 1ull << 63),
        .randomness = gen_janus_anchor()
    };

    // specify self-sender
    tx_proposal.self_sender_index = 1;

    // specify fee per weight
    tx_proposal.fee_per_weight = 20250510;

    // test
    subtest_multi_account_transfer_over_transaction(tx_proposal);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_impl, multi_account_transfer_over_transaction_6)
{
    // four accounts, all legacy
    // 1/4 tx
    // 1 normal payment to main address, integrated address, and subaddress each
    // 0 explicit selfsend payments

    unittest_transaction_preproposal tx_proposal;
    tx_proposal.per_account_payments.resize(4);
    auto &acc0 = tx_proposal.per_account_payments[0];
    auto &acc1 = tx_proposal.per_account_payments[1];
    auto &acc2 = tx_proposal.per_account_payments[2];
    auto &acc3 = tx_proposal.per_account_payments[3];
    acc0.first.generate_legacy();
    acc1.first.generate_legacy();
    acc2.first.generate_legacy();
    acc3.first.generate_legacy();

    // specify self-sender
    tx_proposal.self_sender_index = 2;

    // 1 subaddress payment
    tools::add_element(acc0.second).first = CarrotPaymentProposalV1{
        .destination = acc0.first.subaddress(2, 3),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    };

    // 1 main address payment
    tools::add_element(acc1.second).first = CarrotPaymentProposalV1{
        .destination = acc1.first.cryptonote_address(),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    };

    // 1 integrated address payment
    tools::add_element(acc3.second).first = CarrotPaymentProposalV1{
        .destination = acc3.first.cryptonote_address(gen_payment_id()),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    };

    // specify fee per weight
    tx_proposal.fee_per_weight = 314159;

    // test
    subtest_multi_account_transfer_over_transaction(tx_proposal);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_impl, multi_account_transfer_over_transaction_7)
{
    // four accounts, all legacy
    // 1/6 tx
    // 2 normal payment to main address, 1 integrated address, and 2 subaddress, each copied except integrated
    // 0 explicit selfsend payments

    unittest_transaction_preproposal tx_proposal;
    tx_proposal.per_account_payments.resize(4);
    auto &acc0 = tx_proposal.per_account_payments[0];
    auto &acc1 = tx_proposal.per_account_payments[1];
    auto &acc2 = tx_proposal.per_account_payments[2];
    auto &acc3 = tx_proposal.per_account_payments[3];
    acc0.first.generate_legacy();
    acc1.first.generate_legacy();
    acc2.first.generate_legacy();
    acc3.first.generate_legacy();

    // specify self-sender
    tx_proposal.self_sender_index = 2;

    // 2 subaddress payment
    tools::add_element(acc0.second).first = CarrotPaymentProposalV1{
        .destination = acc0.first.subaddress(2, 3),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    };
    acc0.second.push_back(acc0.second.front());
    acc0.second.back().first.randomness = gen_janus_anchor(); //mangle anchor_norm

    // 2 main address payment
    tools::add_element(acc1.second).first = CarrotPaymentProposalV1{
        .destination = acc1.first.cryptonote_address(),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    };
    acc1.second.push_back(acc1.second.front());
    acc1.second.back().first.randomness = gen_janus_anchor(); //mangle anchor_norm

    // 1 integrated address payment
    tools::add_element(acc3.second).first = CarrotPaymentProposalV1{
        .destination = acc3.first.cryptonote_address(gen_payment_id()),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    };

    // specify fee per weight
    tx_proposal.fee_per_weight = 314159;

    // test
    subtest_multi_account_transfer_over_transaction(tx_proposal);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_impl, multi_account_transfer_over_transaction_8)
{
    // four accounts, all legacy
    // 1/8 tx
    // 2 normal payment to main address, 1 integrated address, and 2 subaddress, each copied except integrated
    // 2 explicit selfsend payments: 1 main address destination, 1 subaddress destination

    unittest_transaction_preproposal tx_proposal;
    tx_proposal.per_account_payments.resize(4);
    auto &acc0 = tx_proposal.per_account_payments[0];
    auto &acc1 = tx_proposal.per_account_payments[1];
    auto &acc2 = tx_proposal.per_account_payments[2];
    auto &acc3 = tx_proposal.per_account_payments[3];
    acc0.first.generate_legacy();
    acc1.first.generate_legacy();
    acc2.first.generate_legacy();
    acc3.first.generate_legacy();

    // specify self-sender
    tx_proposal.self_sender_index = 2;

    // 2 subaddress payment
    tools::add_element(acc0.second).first = CarrotPaymentProposalV1{
        .destination = acc0.first.subaddress(2, 3),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    };
    acc0.second.push_back(acc0.second.front());
    acc0.second.back().first.randomness = gen_janus_anchor(); //mangle anchor_norm

    // 2 main address payment
    tools::add_element(acc1.second).first = CarrotPaymentProposalV1{
        .destination = acc1.first.cryptonote_address(),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    };
    acc1.second.push_back(acc1.second.front());
    acc1.second.back().first.randomness = gen_janus_anchor(); //mangle anchor_norm

    // 1 integrated address payment
    tools::add_element(acc3.second).first = CarrotPaymentProposalV1{
        .destination = acc3.first.cryptonote_address(gen_payment_id()),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    };

    // 1 main address selfsend
    tools::add_element(tx_proposal.explicit_selfsend_proposals).first.proposal = CarrotPaymentProposalSelfSendV1{
        .destination_address_spend_pubkey = acc2.first.account_spend_pubkey,
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .enote_type = CarrotEnoteType::PAYMENT,
        // no internal messages for legacy self-sends
    };

    // 1 subaddress selfsend
    tools::add_element(tx_proposal.explicit_selfsend_proposals).first = CarrotPaymentProposalVerifiableSelfSendV1{
        .proposal = CarrotPaymentProposalSelfSendV1{
            .destination_address_spend_pubkey = acc2.first.subaddress(4, 19).address_spend_pubkey,
            .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
            .enote_type = CarrotEnoteType::CHANGE
        },
        .subaddr_index = {4, 19}
    };

    // specify fee per weight
    tx_proposal.fee_per_weight = 314159;

    // test
    subtest_multi_account_transfer_over_transaction(tx_proposal);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_impl, multi_account_transfer_over_transaction_9)
{
    // two accounts, both carrot
    // 1/2 tx
    // 1 normal payment to main address
    // 0 explicit selfsend payments
    // subtractable

    unittest_transaction_preproposal tx_proposal;
    tx_proposal.per_account_payments.resize(2);
    mock_carrot_or_legacy_keys &acc0 = tx_proposal.per_account_payments[0].first;
    mock_carrot_or_legacy_keys &acc1 = tx_proposal.per_account_payments[1].first;
    acc0.generate_carrot();
    acc1.generate_carrot();

    // 1 normal payment (subtractable)
    CarrotPaymentProposalV1 &normal_payment_proposal = tools::add_element( tx_proposal.per_account_payments[0].second).first;
    normal_payment_proposal = CarrotPaymentProposalV1{
        .destination = acc0.cryptonote_address(),
        .amount = crypto::rand_idx((rct::xmr_amount) 1ull << 63),
        .randomness = gen_janus_anchor()
    };
    tx_proposal.per_account_payments[0].second.back().second = true;

    // specify self-sender
    tx_proposal.self_sender_index = 1;

    // specify fee per weight
    tx_proposal.fee_per_weight = 20250510;

    // test
    subtest_multi_account_transfer_over_transaction(tx_proposal);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_impl, multi_account_transfer_over_transaction_10)
{
    // four accounts, all carrot
    // 1/4 tx
    // 1 normal payment to main address, integrated address, and subaddress each
    // 0 explicit selfsend payments
    // subaddress and integrated address are subtractable

    unittest_transaction_preproposal tx_proposal;
    tx_proposal.per_account_payments.resize(4);
    auto &acc0 = tx_proposal.per_account_payments[0];
    auto &acc1 = tx_proposal.per_account_payments[1];
    auto &acc2 = tx_proposal.per_account_payments[2];
    auto &acc3 = tx_proposal.per_account_payments[3];
    acc0.first.generate_carrot();
    acc1.first.generate_carrot();
    acc2.first.generate_carrot();
    acc3.first.generate_carrot();

    // specify self-sender
    tx_proposal.self_sender_index = 2;

    // 1 subaddress payment (subtractable)
    tools::add_element(acc0.second) = {CarrotPaymentProposalV1{
        .destination = acc0.first.subaddress(2, 3),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    }, true};

    // 1 main address payment
    tools::add_element(acc1.second).first = CarrotPaymentProposalV1{
        .destination = acc1.first.cryptonote_address(),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    };

    // 1 integrated address payment
    tools::add_element(acc3.second) = {CarrotPaymentProposalV1{
        .destination = acc3.first.cryptonote_address(gen_payment_id()),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    }, true};

    // specify fee per weight
    tx_proposal.fee_per_weight = 314159;

    // test
    subtest_multi_account_transfer_over_transaction(tx_proposal);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_impl, multi_account_transfer_over_transaction_11)
{
    // four accounts, all carrot
    // 1/6 tx
    // 2 normal payment to main address, 1 integrated address, and 2 subaddress, each copied except integrated
    // 0 explicit selfsend payments
    // 1 main and 1 subaddress is subtractable

    unittest_transaction_preproposal tx_proposal;
    tx_proposal.per_account_payments.resize(4);
    auto &acc0 = tx_proposal.per_account_payments[0];
    auto &acc1 = tx_proposal.per_account_payments[1];
    auto &acc2 = tx_proposal.per_account_payments[2];
    auto &acc3 = tx_proposal.per_account_payments[3];
    acc0.first.generate_carrot();
    acc1.first.generate_carrot();
    acc2.first.generate_carrot();
    acc3.first.generate_carrot();

    // specify self-sender
    tx_proposal.self_sender_index = 2;

    // 2 subaddress payment
    tools::add_element(acc0.second).first = CarrotPaymentProposalV1{
        .destination = acc0.first.subaddress(2, 3),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    };
    acc0.second.push_back(acc0.second.front());
    acc0.second.back().first.randomness = gen_janus_anchor(); //mangle anchor_norm
    acc0.second.back().second = true;                         //set copy as subtractable

    // 2 main address payment
    tools::add_element(acc1.second).first = CarrotPaymentProposalV1{
        .destination = acc1.first.cryptonote_address(),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    };
    acc1.second.push_back(acc1.second.front());
    acc1.second.back().first.randomness = gen_janus_anchor(); //mangle anchor_norm
    acc1.second.back().second = true;                         //set copy as subtractable

    // 1 integrated address payment
    tools::add_element(acc3.second).first = CarrotPaymentProposalV1{
        .destination = acc3.first.cryptonote_address(gen_payment_id()),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    };

    // specify fee per weight
    tx_proposal.fee_per_weight = 314159;

    // test
    subtest_multi_account_transfer_over_transaction(tx_proposal);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_impl, multi_account_transfer_over_transaction_12)
{
    // four accounts, all carrot
    // 1/8 tx
    // 2 normal payment to main address, 1 integrated address, and 2 subaddress, each copied except integrated
    // 2 explicit selfsend payments: 1 main address destination, 1 subaddress destination
    // 1 normal main address, 1 integrated, and 1 self-send subaddress is subtractable

    unittest_transaction_preproposal tx_proposal;
    tx_proposal.per_account_payments.resize(4);
    auto &acc0 = tx_proposal.per_account_payments[0];
    auto &acc1 = tx_proposal.per_account_payments[1];
    auto &acc2 = tx_proposal.per_account_payments[2];
    auto &acc3 = tx_proposal.per_account_payments[3];
    acc0.first.generate_carrot();
    acc1.first.generate_carrot();
    acc2.first.generate_carrot();
    acc3.first.generate_carrot();

    // specify self-sender
    tx_proposal.self_sender_index = 2;

    // 2 subaddress payment (1 subtractable)
    tools::add_element(acc0.second) = {CarrotPaymentProposalV1{
        .destination = acc0.first.subaddress(2, 3),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    }, true};
    acc0.second.push_back(acc0.second.front());
    acc0.second.back().first.randomness = gen_janus_anchor(); //mangle anchor_norm
    acc0.second.back().second = false;                        //set not subtractable, first already is

    // 2 main address payment
    tools::add_element(acc1.second).first = CarrotPaymentProposalV1{
        .destination = acc1.first.cryptonote_address(),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    };
    acc1.second.push_back(acc1.second.front());
    acc1.second.back().first.randomness = gen_janus_anchor(); //mangle anchor_norm

    // 1 integrated address payment (subtractable)
    tools::add_element(acc3.second) = {CarrotPaymentProposalV1{
        .destination = acc3.first.cryptonote_address(gen_payment_id()),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    }, true};

    // 1 main address selfsend
    tools::add_element(tx_proposal.explicit_selfsend_proposals).first.proposal = CarrotPaymentProposalSelfSendV1{
        .destination_address_spend_pubkey = acc2.first.account_spend_pubkey,
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .enote_type = CarrotEnoteType::PAYMENT,
        .internal_message = gen_janus_anchor()
    };

    // 1 subaddress selfsend (subtractable)
    tools::add_element(tx_proposal.explicit_selfsend_proposals) = {CarrotPaymentProposalVerifiableSelfSendV1{
        .proposal = CarrotPaymentProposalSelfSendV1{
            .destination_address_spend_pubkey = acc2.first.subaddress(4, 19).address_spend_pubkey,
            .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
            .enote_type = CarrotEnoteType::CHANGE
        },
        .subaddr_index = {4, 19}
    }, true};

    // specify fee per weight
    tx_proposal.fee_per_weight = 314159;

    // test
    subtest_multi_account_transfer_over_transaction(tx_proposal);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_impl, multi_account_transfer_over_transaction_13)
{
    // two accounts, both legacy
    // 1/2 tx
    // 1 normal payment to main address
    // 0 explicit selfsend payments
    // subtractable

    unittest_transaction_preproposal tx_proposal;
    tx_proposal.per_account_payments.resize(2);
    mock_carrot_or_legacy_keys &acc0 = tx_proposal.per_account_payments[0].first;
    mock_carrot_or_legacy_keys &acc1 = tx_proposal.per_account_payments[1].first;
    acc0.generate_legacy();
    acc1.generate_legacy();

    // 1 normal payment (subtractable)
    CarrotPaymentProposalV1 &normal_payment_proposal = tools::add_element( tx_proposal.per_account_payments[0].second).first;
    normal_payment_proposal = CarrotPaymentProposalV1{
        .destination = acc0.cryptonote_address(),
        .amount = crypto::rand_idx((rct::xmr_amount) 1ull << 63),
        .randomness = gen_janus_anchor()
    };
    tx_proposal.per_account_payments[0].second.back().second = true;

    // specify self-sender
    tx_proposal.self_sender_index = 1;

    // specify fee per weight
    tx_proposal.fee_per_weight = 20250510;

    // test
    subtest_multi_account_transfer_over_transaction(tx_proposal);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_impl, multi_account_transfer_over_transaction_14)
{
    // four accounts, all legacy
    // 1/4 tx
    // 1 normal payment to main address, integrated address, and subaddress each
    // 0 explicit selfsend payments
    // 1 integrated and 1 subaddress subtractable

    unittest_transaction_preproposal tx_proposal;
    tx_proposal.per_account_payments.resize(4);
    auto &acc0 = tx_proposal.per_account_payments[0];
    auto &acc1 = tx_proposal.per_account_payments[1];
    auto &acc2 = tx_proposal.per_account_payments[2];
    auto &acc3 = tx_proposal.per_account_payments[3];
    acc0.first.generate_legacy();
    acc1.first.generate_legacy();
    acc2.first.generate_legacy();
    acc3.first.generate_legacy();

    // specify self-sender
    tx_proposal.self_sender_index = 2;

    // 1 subaddress payment (subtractable)
    tools::add_element(acc0.second) = {CarrotPaymentProposalV1{
        .destination = acc0.first.subaddress(2, 3),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    }, true};

    // 1 main address payment
    tools::add_element(acc1.second).first = CarrotPaymentProposalV1{
        .destination = acc1.first.cryptonote_address(),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    };

    // 1 integrated address payment (subtractable)
    tools::add_element(acc3.second) = {CarrotPaymentProposalV1{
        .destination = acc3.first.cryptonote_address(gen_payment_id()),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    }, true};

    // specify fee per weight
    tx_proposal.fee_per_weight = 314159;

    // test
    subtest_multi_account_transfer_over_transaction(tx_proposal);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_impl, multi_account_transfer_over_transaction_15)
{
    // four accounts, all legacy
    // 1/6 tx
    // 2 normal payment to main address, 1 integrated address, and 2 subaddress, each copied except integrated
    // 0 explicit selfsend payments
    // all subtractable

    unittest_transaction_preproposal tx_proposal;
    tx_proposal.per_account_payments.resize(4);
    auto &acc0 = tx_proposal.per_account_payments[0];
    auto &acc1 = tx_proposal.per_account_payments[1];
    auto &acc2 = tx_proposal.per_account_payments[2];
    auto &acc3 = tx_proposal.per_account_payments[3];
    acc0.first.generate_legacy();
    acc1.first.generate_legacy();
    acc2.first.generate_legacy();
    acc3.first.generate_legacy();

    // specify self-sender
    tx_proposal.self_sender_index = 2;

    // 2 subaddress payment (subtractable)
    tools::add_element(acc0.second) = {CarrotPaymentProposalV1{
        .destination = acc0.first.subaddress(2, 3),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    }, true};
    acc0.second.push_back(acc0.second.front());
    acc0.second.back().first.randomness = gen_janus_anchor(); //mangle anchor_norm

    // 2 main address payment (subtractable)
    tools::add_element(acc1.second) = {CarrotPaymentProposalV1{
        .destination = acc1.first.cryptonote_address(),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    }, true};
    acc1.second.push_back(acc1.second.front());
    acc1.second.back().first.randomness = gen_janus_anchor(); //mangle anchor_norm

    // 1 integrated address payment (subtractable)
    tools::add_element(acc3.second) = {CarrotPaymentProposalV1{
        .destination = acc3.first.cryptonote_address(gen_payment_id()),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    }, true};

    // specify fee per weight
    tx_proposal.fee_per_weight = 314159;

    // test
    subtest_multi_account_transfer_over_transaction(tx_proposal);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_impl, multi_account_transfer_over_transaction_16)
{
    // four accounts, all legacy
    // 1/8 tx
    // 2 normal payment to main address, 1 integrated address, and 2 subaddress, each copied except integrated
    // 2 explicit selfsend payments: 1 main address destination, 1 subaddress destination
    // all subtractable

    unittest_transaction_preproposal tx_proposal;
    tx_proposal.per_account_payments.resize(4);
    auto &acc0 = tx_proposal.per_account_payments[0];
    auto &acc1 = tx_proposal.per_account_payments[1];
    auto &acc2 = tx_proposal.per_account_payments[2];
    auto &acc3 = tx_proposal.per_account_payments[3];
    acc0.first.generate_legacy();
    acc1.first.generate_legacy();
    acc2.first.generate_legacy();
    acc3.first.generate_legacy();

    // specify self-sender
    tx_proposal.self_sender_index = 2;

    // 2 subaddress payment (subtractable)
    tools::add_element(acc0.second) = {CarrotPaymentProposalV1{
        .destination = acc0.first.subaddress(2, 3),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    }, true};
    acc0.second.push_back(acc0.second.front());
    acc0.second.back().first.randomness = gen_janus_anchor(); //mangle anchor_norm

    // 2 main address payment (subtractable)
    tools::add_element(acc1.second) = {CarrotPaymentProposalV1{
        .destination = acc1.first.cryptonote_address(),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    }, true};
    acc1.second.push_back(acc1.second.front());
    acc1.second.back().first.randomness = gen_janus_anchor(); //mangle anchor_norm

    // 1 integrated address payment (subtractable)
    tools::add_element(acc3.second) = {CarrotPaymentProposalV1{
        .destination = acc3.first.cryptonote_address(gen_payment_id()),
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    }, true};

    // 1 main address selfsend (subtractable)
    tools::add_element(tx_proposal.explicit_selfsend_proposals) = {{CarrotPaymentProposalSelfSendV1{
        .destination_address_spend_pubkey = acc2.first.account_spend_pubkey,
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .enote_type = CarrotEnoteType::PAYMENT,
        // no internal messages for legacy self-sends
    }}, true};

    // 1 subaddress selfsend (subtractable)
    tools::add_element(tx_proposal.explicit_selfsend_proposals) = {CarrotPaymentProposalVerifiableSelfSendV1{
        .proposal = CarrotPaymentProposalSelfSendV1{
            .destination_address_spend_pubkey = acc2.first.subaddress(4, 19).address_spend_pubkey,
            .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
            .enote_type = CarrotEnoteType::CHANGE
        },
        .subaddr_index = {4, 19}
    }, true};

    // specify fee per weight
    tx_proposal.fee_per_weight = 314159;

    // test
    subtest_multi_account_transfer_over_transaction(tx_proposal);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_impl, make_single_transfer_input_selector_TwoInputsPreferOldest_1)
{
    const std::vector<CarrotPreSelectedInput> input_candidates = {
        CarrotPreSelectedInput {
            .core = CarrotSelectedInput {
                .amount = 500,
                .key_image = gen_key_image(),
            },
            .is_external = false,
            .block_index = 72
        },
        CarrotPreSelectedInput {
            .core = CarrotSelectedInput {
                .amount = 200,
                .key_image = gen_key_image(),
            },
            .is_external = false,
            .block_index = 34
        }
    };

    const std::vector<InputSelectionPolicy> policies = { InputSelectionPolicy::TwoInputsPreferOldest };

    const uint32_t flags = 0;

    std::set<size_t> selected_input_indices;
    select_inputs_func_t input_selector = make_single_transfer_input_selector(epee::to_span(input_candidates),
        epee::to_span(policies),
        flags,
        &selected_input_indices);
    
    boost::multiprecision::int128_t nominal_output_sum = 369;

    const std::map<size_t, rct::xmr_amount> fee_by_input_count = {
        {1, 50},
        {2, 75}
    };

    const size_t num_normal_payment_proposals = 1;
    const size_t num_selfsend_payment_proposals = 1;

    ASSERT_GT(input_candidates[0].core.amount, nominal_output_sum + fee_by_input_count.crbegin()->second);

    std::vector<CarrotSelectedInput> selected_inputs;
    input_selector(nominal_output_sum,
        fee_by_input_count,
        num_normal_payment_proposals,
        num_selfsend_payment_proposals,
        selected_inputs);

    ASSERT_EQ(2, input_candidates.size());
    ASSERT_EQ(2, selected_inputs.size());
    EXPECT_NE(input_candidates.at(0).core, input_candidates.at(1).core);
    EXPECT_NE(selected_inputs.at(0), selected_inputs.at(1));
    EXPECT_TRUE((selected_inputs.at(0) == input_candidates.at(0).core) ^ (selected_inputs.at(0) == input_candidates[1].core));
    EXPECT_TRUE((selected_inputs.at(1) == input_candidates.at(0).core) ^ (selected_inputs.at(1) == input_candidates.at(1).core));
}
//----------------------------------------------------------------------------------------------------------------------
