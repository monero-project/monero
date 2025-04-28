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

#include "carrot_core/output_set_finalization.h"
#include "carrot_core/payment_proposal.h"
#include "carrot_impl/carrot_tx_builder_utils.h"
#include "carrot_impl/carrot_tx_format_utils.h"
#include "carrot_impl/input_selection.h"
#include "carrot_mock_helpers.h"
#include "common/container_helpers.h"
#include "crypto/generators.h"
#include "cryptonote_basic/account.h"
#include "cryptonote_basic/subaddress_index.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_core/blockchain.h"
#include "ringct/bulletproofs_plus.h"
#include "ringct/rctOps.h"
#include "ringct/rctSigs.h"

using namespace carrot;

namespace
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static void unittest_scan_enote_set_multi_account(const std::vector<CarrotEnoteV1> &enotes,
    const encrypted_payment_id_t encrypted_payment_id,
    const epee::span<const mock::mock_carrot_and_legacy_keys * const> accounts,
    std::vector<std::vector<mock::mock_scan_result_t>> &res)
{
    res.clear();
    res.reserve(accounts.size());

    for (const auto *account : accounts)
        mock_scan_enote_set(enotes, encrypted_payment_id, *account, tools::add_element(res));
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
struct unittest_transaction_preproposal
{
    using per_payment_proposal = std::pair<CarrotPaymentProposalV1, /*is subtractble?*/bool>;
    using per_ss_payment_proposal = std::pair<CarrotPaymentProposalVerifiableSelfSendV1, /*is subtractble?*/bool>;
    using per_account = std::pair<mock::mock_carrot_and_legacy_keys, std::vector<per_payment_proposal>>;
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
    const mock::mock_carrot_and_legacy_keys &ss_keys =
        tx_preproposal.per_account_payments.at(tx_preproposal.self_sender_index).first;

    // make transaction proposal
    CarrotTransactionProposalV1 tx_proposal;
    make_carrot_transaction_proposal_v1_transfer(normal_payment_proposals,
        selfsend_payment_proposals,
        tx_preproposal.fee_per_weight,
        tx_preproposal.extra_extra,
        make_fake_input_selection_callback(),
        &ss_keys.s_view_balance_dev,
        &ss_keys.k_view_incoming_dev,
        ss_keys.carrot_account_spend_pubkey,
        {},
        {},
        tx_proposal);

    // make unsigned transaction
    cryptonote::transaction tx;
    make_pruned_transaction_from_proposal_v1(tx_proposal,
        &ss_keys.s_view_balance_dev,
        &ss_keys.k_view_incoming_dev,
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
        &ss_keys.s_view_balance_dev,
        &ss_keys.k_view_incoming_dev,
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
    std::vector<const mock::mock_carrot_and_legacy_keys *> accounts;
    for (const auto &pa : tx_preproposal.per_account_payments)
        accounts.push_back(&pa.first);

    // do scanning of all accounts on every enotes
    std::vector<std::vector<mock::mock_scan_result_t>> scan_results;
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

        const std::vector<mock::mock_scan_result_t> &account_scan_results = scan_results.at(account_idx);
        const auto &account_payment_proposals = tx_preproposal.per_account_payments.at(account_idx).second;
        ASSERT_EQ(account_payment_proposals.size(), account_scan_results.size());
        std::set<size_t> matched_payment_proposals;

        // for each scan result assigned to this account...
        for (const mock::mock_scan_result_t &single_scan_res : account_scan_results)
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
                    // try opening Ko
                    const CarrotEnoteV1 &enote =  parsed_enotes.at(single_scan_res.output_index);
                    EXPECT_TRUE(accounts.at(account_idx)->can_open_fcmp_onetime_address(single_scan_res.address_spend_pubkey,
                        single_scan_res.sender_extension_g,
                        single_scan_res.sender_extension_t,
                        enote.onetime_address));

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
    const std::vector<mock::mock_scan_result_t> &account_scan_results = scan_results.at(tx_preproposal.self_sender_index);
    ASSERT_EQ(selfsend_payment_proposals.size() + 1, account_scan_results.size());
    std::set<size_t> matched_payment_proposals;
    const mock::mock_scan_result_t* implicit_change_scan_res = nullptr;
    // for each scan result assigned to the self-sender account...
    for (const mock::mock_scan_result_t &single_scan_res : account_scan_results)
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
                // try opening Ko
                const CarrotEnoteV1 &enote = parsed_enotes.at(single_scan_res.output_index);
                EXPECT_TRUE(ss_keys.can_open_fcmp_onetime_address(single_scan_res.address_spend_pubkey,
                    single_scan_res.sender_extension_g,
                    single_scan_res.sender_extension_t,
                    enote.onetime_address));

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
} //namespace
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_impl, multi_account_transfer_over_transaction_1)
{
    // two accounts, both carrot
    // 1/2 tx
    // 1 normal payment to main address
    // 0 explicit selfsend payments

    unittest_transaction_preproposal tx_proposal;
    tx_proposal.per_account_payments = std::vector<unittest_transaction_preproposal::per_account>(2);
    auto &acc0 = tx_proposal.per_account_payments[0].first;
    auto &acc1 = tx_proposal.per_account_payments[1].first;
    acc0.generate();
    acc1.generate();

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
    tx_proposal.per_account_payments = std::vector<unittest_transaction_preproposal::per_account>(4);
    auto &acc0 = tx_proposal.per_account_payments[0];
    auto &acc1 = tx_proposal.per_account_payments[1];
    auto &acc2 = tx_proposal.per_account_payments[2];
    auto &acc3 = tx_proposal.per_account_payments[3];
    acc0.first.generate();
    acc1.first.generate();
    acc2.first.generate();
    acc3.first.generate();

    // specify self-sender
    tx_proposal.self_sender_index = 2;

    // 1 subaddress payment
    tools::add_element(acc0.second).first = CarrotPaymentProposalV1{
        .destination = acc0.first.subaddress({{2, 3}}),
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
    tx_proposal.per_account_payments = std::vector<unittest_transaction_preproposal::per_account>(4);
    auto &acc0 = tx_proposal.per_account_payments[0];
    auto &acc1 = tx_proposal.per_account_payments[1];
    auto &acc2 = tx_proposal.per_account_payments[2];
    auto &acc3 = tx_proposal.per_account_payments[3];
    acc0.first.generate();
    acc1.first.generate();
    acc2.first.generate();
    acc3.first.generate();

    // specify self-sender
    tx_proposal.self_sender_index = 2;

    // 2 subaddress payment
    tools::add_element(acc0.second).first = CarrotPaymentProposalV1{
        .destination = acc0.first.subaddress({{2, 3}}),
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
    tx_proposal.per_account_payments = std::vector<unittest_transaction_preproposal::per_account>(4);
    auto &acc0 = tx_proposal.per_account_payments[0];
    auto &acc1 = tx_proposal.per_account_payments[1];
    auto &acc2 = tx_proposal.per_account_payments[2];
    auto &acc3 = tx_proposal.per_account_payments[3];
    acc0.first.generate();
    acc1.first.generate();
    acc2.first.generate();
    acc3.first.generate();

    // specify self-sender
    tx_proposal.self_sender_index = 2;

    // 2 subaddress payment
    tools::add_element(acc0.second).first = CarrotPaymentProposalV1{
        .destination = acc0.first.subaddress({{2, 3}}),
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
        .destination_address_spend_pubkey = acc2.first.carrot_account_spend_pubkey,
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .enote_type = CarrotEnoteType::PAYMENT,
        .internal_message = gen_janus_anchor()
    };

    // 1 subaddress selfsend
    tools::add_element(tx_proposal.explicit_selfsend_proposals).first = CarrotPaymentProposalVerifiableSelfSendV1{
        .proposal = CarrotPaymentProposalSelfSendV1{
            .destination_address_spend_pubkey = acc2.first.subaddress({{4, 19}}).address_spend_pubkey,
            .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
            .enote_type = CarrotEnoteType::CHANGE
        },
        .subaddr_index = {{4, 19}}
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
    tx_proposal.per_account_payments = std::vector<unittest_transaction_preproposal::per_account>(2);
    auto &acc0 = tx_proposal.per_account_payments[0].first;
    auto &acc1 = tx_proposal.per_account_payments[1].first;
    acc0.generate(AddressDeriveType::PreCarrot);
    acc1.generate(AddressDeriveType::PreCarrot);

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
    tx_proposal.per_account_payments = std::vector<unittest_transaction_preproposal::per_account>(4);
    auto &acc0 = tx_proposal.per_account_payments[0];
    auto &acc1 = tx_proposal.per_account_payments[1];
    auto &acc2 = tx_proposal.per_account_payments[2];
    auto &acc3 = tx_proposal.per_account_payments[3];
    acc0.first.generate();
    acc1.first.generate();
    acc2.first.generate();
    acc3.first.generate();

    // specify self-sender
    tx_proposal.self_sender_index = 2;

    // 1 subaddress payment
    tools::add_element(acc0.second).first = CarrotPaymentProposalV1{
        .destination = acc0.first.subaddress({{2, 3}}),
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
    tx_proposal.per_account_payments = std::vector<unittest_transaction_preproposal::per_account>(4);
    auto &acc0 = tx_proposal.per_account_payments[0];
    auto &acc1 = tx_proposal.per_account_payments[1];
    auto &acc2 = tx_proposal.per_account_payments[2];
    auto &acc3 = tx_proposal.per_account_payments[3];
    acc0.first.generate(AddressDeriveType::PreCarrot);
    acc1.first.generate(AddressDeriveType::PreCarrot);
    acc2.first.generate(AddressDeriveType::PreCarrot);
    acc3.first.generate(AddressDeriveType::PreCarrot);

    // specify self-sender
    tx_proposal.self_sender_index = 2;

    // 2 subaddress payment
    tools::add_element(acc0.second).first = CarrotPaymentProposalV1{
        .destination = acc0.first.subaddress({{2, 3}}),
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
    tx_proposal.per_account_payments = std::vector<unittest_transaction_preproposal::per_account>(4);
    auto &acc0 = tx_proposal.per_account_payments[0];
    auto &acc1 = tx_proposal.per_account_payments[1];
    auto &acc2 = tx_proposal.per_account_payments[2];
    auto &acc3 = tx_proposal.per_account_payments[3];
    acc0.first.generate(AddressDeriveType::PreCarrot);
    acc1.first.generate(AddressDeriveType::PreCarrot);
    acc2.first.generate(AddressDeriveType::PreCarrot);
    acc3.first.generate(AddressDeriveType::PreCarrot);

    // specify self-sender
    tx_proposal.self_sender_index = 2;

    // 2 subaddress payment
    tools::add_element(acc0.second).first = CarrotPaymentProposalV1{
        .destination = acc0.first.subaddress({{2, 3}}),
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
        .destination_address_spend_pubkey = acc2.first.carrot_account_spend_pubkey,
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .enote_type = CarrotEnoteType::PAYMENT,
        // no internal messages for legacy self-sends
    };

    // 1 subaddress selfsend
    tools::add_element(tx_proposal.explicit_selfsend_proposals).first = CarrotPaymentProposalVerifiableSelfSendV1{
        .proposal = CarrotPaymentProposalSelfSendV1{
            .destination_address_spend_pubkey = acc2.first.subaddress({{4, 19}}).address_spend_pubkey,
            .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
            .enote_type = CarrotEnoteType::CHANGE
        },
        .subaddr_index = {{4, 19}}
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
    tx_proposal.per_account_payments = std::vector<unittest_transaction_preproposal::per_account>(2);
    auto &acc0 = tx_proposal.per_account_payments[0].first;
    auto &acc1 = tx_proposal.per_account_payments[1].first;
    acc0.generate();
    acc1.generate();

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
    tx_proposal.per_account_payments = std::vector<unittest_transaction_preproposal::per_account>(4);
    auto &acc0 = tx_proposal.per_account_payments[0];
    auto &acc1 = tx_proposal.per_account_payments[1];
    auto &acc2 = tx_proposal.per_account_payments[2];
    auto &acc3 = tx_proposal.per_account_payments[3];
    acc0.first.generate();
    acc1.first.generate();
    acc2.first.generate();
    acc3.first.generate();

    // specify self-sender
    tx_proposal.self_sender_index = 2;

    // 1 subaddress payment (subtractable)
    tools::add_element(acc0.second) = {CarrotPaymentProposalV1{
        .destination = acc0.first.subaddress({{2, 3}}),
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
    tx_proposal.per_account_payments = std::vector<unittest_transaction_preproposal::per_account>(4);
    auto &acc0 = tx_proposal.per_account_payments[0];
    auto &acc1 = tx_proposal.per_account_payments[1];
    auto &acc2 = tx_proposal.per_account_payments[2];
    auto &acc3 = tx_proposal.per_account_payments[3];
    acc0.first.generate();
    acc1.first.generate();
    acc2.first.generate();
    acc3.first.generate();

    // specify self-sender
    tx_proposal.self_sender_index = 2;

    // 2 subaddress payment
    tools::add_element(acc0.second).first = CarrotPaymentProposalV1{
        .destination = acc0.first.subaddress({{2, 3}}),
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
    tx_proposal.per_account_payments = std::vector<unittest_transaction_preproposal::per_account>(4);
    auto &acc0 = tx_proposal.per_account_payments[0];
    auto &acc1 = tx_proposal.per_account_payments[1];
    auto &acc2 = tx_proposal.per_account_payments[2];
    auto &acc3 = tx_proposal.per_account_payments[3];
    acc0.first.generate();
    acc1.first.generate();
    acc2.first.generate();
    acc3.first.generate();

    // specify self-sender
    tx_proposal.self_sender_index = 2;

    // 2 subaddress payment (1 subtractable)
    tools::add_element(acc0.second) = {CarrotPaymentProposalV1{
        .destination = acc0.first.subaddress({{2, 3}}),
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
        .destination_address_spend_pubkey = acc2.first.carrot_account_spend_pubkey,
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .enote_type = CarrotEnoteType::PAYMENT,
        .internal_message = gen_janus_anchor()
    };

    // 1 subaddress selfsend (subtractable)
    tools::add_element(tx_proposal.explicit_selfsend_proposals) = {CarrotPaymentProposalVerifiableSelfSendV1{
        .proposal = CarrotPaymentProposalSelfSendV1{
            .destination_address_spend_pubkey = acc2.first.subaddress({{4, 19}}).address_spend_pubkey,
            .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
            .enote_type = CarrotEnoteType::CHANGE
        },
        .subaddr_index = {{4, 19}}
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
    tx_proposal.per_account_payments = std::vector<unittest_transaction_preproposal::per_account>(2);
    auto &acc0 = tx_proposal.per_account_payments[0].first;
    auto &acc1 = tx_proposal.per_account_payments[1].first;
    acc0.generate(AddressDeriveType::PreCarrot);
    acc1.generate(AddressDeriveType::PreCarrot);

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
    tx_proposal.per_account_payments = std::vector<unittest_transaction_preproposal::per_account>(4);
    auto &acc0 = tx_proposal.per_account_payments[0];
    auto &acc1 = tx_proposal.per_account_payments[1];
    auto &acc2 = tx_proposal.per_account_payments[2];
    auto &acc3 = tx_proposal.per_account_payments[3];
    acc0.first.generate(AddressDeriveType::PreCarrot);
    acc1.first.generate(AddressDeriveType::PreCarrot);
    acc2.first.generate(AddressDeriveType::PreCarrot);
    acc3.first.generate(AddressDeriveType::PreCarrot);

    // specify self-sender
    tx_proposal.self_sender_index = 2;

    // 1 subaddress payment (subtractable)
    tools::add_element(acc0.second) = {CarrotPaymentProposalV1{
        .destination = acc0.first.subaddress({{2, 3}}),
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
    tx_proposal.per_account_payments = std::vector<unittest_transaction_preproposal::per_account>(4);
    auto &acc0 = tx_proposal.per_account_payments[0];
    auto &acc1 = tx_proposal.per_account_payments[1];
    auto &acc2 = tx_proposal.per_account_payments[2];
    auto &acc3 = tx_proposal.per_account_payments[3];
    acc0.first.generate(AddressDeriveType::PreCarrot);
    acc1.first.generate(AddressDeriveType::PreCarrot);
    acc2.first.generate(AddressDeriveType::PreCarrot);
    acc3.first.generate(AddressDeriveType::PreCarrot);

    // specify self-sender
    tx_proposal.self_sender_index = 2;

    // 2 subaddress payment (subtractable)
    tools::add_element(acc0.second) = {CarrotPaymentProposalV1{
        .destination = acc0.first.subaddress({{2, 3}}),
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
    tx_proposal.per_account_payments = std::vector<unittest_transaction_preproposal::per_account>(4);
    auto &acc0 = tx_proposal.per_account_payments[0];
    auto &acc1 = tx_proposal.per_account_payments[1];
    auto &acc2 = tx_proposal.per_account_payments[2];
    auto &acc3 = tx_proposal.per_account_payments[3];
    acc0.first.generate(AddressDeriveType::PreCarrot);
    acc1.first.generate(AddressDeriveType::PreCarrot);
    acc2.first.generate(AddressDeriveType::PreCarrot);
    acc3.first.generate(AddressDeriveType::PreCarrot);

    // specify self-sender
    tx_proposal.self_sender_index = 2;

    // 2 subaddress payment (subtractable)
    tools::add_element(acc0.second) = {CarrotPaymentProposalV1{
        .destination = acc0.first.subaddress({{2, 3}}),
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
        .destination_address_spend_pubkey = acc2.first.carrot_account_spend_pubkey,
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .enote_type = CarrotEnoteType::PAYMENT,
        // no internal messages for legacy self-sends
    }}, true};

    // 1 subaddress selfsend (subtractable)
    tools::add_element(tx_proposal.explicit_selfsend_proposals) = {CarrotPaymentProposalVerifiableSelfSendV1{
        .proposal = CarrotPaymentProposalSelfSendV1{
            .destination_address_spend_pubkey = acc2.first.subaddress({{4, 19}}).address_spend_pubkey,
            .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
            .enote_type = CarrotEnoteType::CHANGE
        },
        .subaddr_index = {{4, 19}}
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
                .key_image = mock::gen_key_image(),
            },
            .is_external = false,
            .block_index = 72
        },
        CarrotPreSelectedInput {
            .core = CarrotSelectedInput {
                .amount = 200,
                .key_image = mock::gen_key_image(),
            },
            .is_external = false,
            .block_index = 34
        }
    };

    const std::vector<input_selection_policy_t> policies = { &carrot::ispolicy::select_two_inputs_prefer_oldest };

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
TEST(carrot_impl, make_single_transfer_input_selector_greedy_aging_1)
{
    const std::vector<CarrotPreSelectedInput> input_candidates = {
        CarrotPreSelectedInput {
            .core = CarrotSelectedInput {
                .amount = 500,
                .key_image = mock::gen_key_image(),
            },
            .is_external = false,
            .block_index = 72
        },
        CarrotPreSelectedInput {
            .core = CarrotSelectedInput {
                .amount = 200,
                .key_image = mock::gen_key_image(),
            },
            .is_external = false,
            .block_index = 34
        }
    };

    const std::vector<input_selection_policy_t> policies = { &carrot::ispolicy::select_greedy_aging };

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

    // sometimes we choose 2 for default, sometimes 1 for default
    ASSERT_EQ(2, input_candidates.size());
    ASSERT_EQ(selected_inputs.size(), selected_input_indices.size());
    if (selected_inputs.size() == 2)
    {
        EXPECT_NE(input_candidates.at(0).core, input_candidates.at(1).core);
        EXPECT_NE(selected_inputs.at(0), selected_inputs.at(1));
        EXPECT_TRUE((selected_inputs.at(0) == input_candidates.at(0).core) ^ (selected_inputs.at(0) == input_candidates[1].core));
        EXPECT_TRUE((selected_inputs.at(1) == input_candidates.at(0).core) ^ (selected_inputs.at(1) == input_candidates.at(1).core));
    }
    else
    {
        ASSERT_EQ(std::set<size_t>{0}, selected_input_indices);
        ASSERT_EQ(input_candidates.at(0).core, selected_inputs.at(0));
    }
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_impl, make_single_transfer_input_selector_greedy_aging_2)
{
    const std::vector<CarrotPreSelectedInput> input_candidates = {
        CarrotPreSelectedInput {
            .core = CarrotSelectedInput {
                .amount = 500,
                .key_image = mock::gen_key_image(),
            },
            .is_external = false,
            .block_index = 72
        }
    };

    const std::vector<input_selection_policy_t> policies = { &carrot::ispolicy::select_greedy_aging };

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

    ASSERT_EQ(1, input_candidates.size());
    ASSERT_EQ(std::set<size_t>{0}, selected_input_indices);
    ASSERT_EQ(std::vector<CarrotSelectedInput>{input_candidates.at(0).core}, selected_inputs);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_impl, make_single_transfer_input_selector_greedy_aging_3)
{
    // Give 5 input candidates, only 3 are needed to fill order. 4 should be used since it's the next power of 2
    // They should be the four oldest

    const std::vector<CarrotPreSelectedInput> input_candidates = {
        CarrotPreSelectedInput {
            .core = CarrotSelectedInput {
                .amount = 100,
                .key_image = mock::gen_key_image(),
            },
            .is_external = false,
            .block_index = 55
        },
        CarrotPreSelectedInput {
            .core = CarrotSelectedInput {
                .amount = 100,
                .key_image = mock::gen_key_image(),
            },
            .is_external = false,
            .block_index = 22
        },
        CarrotPreSelectedInput {
            .core = CarrotSelectedInput {
                .amount = 100,
                .key_image = mock::gen_key_image(),
            },
            .is_external = false,
            .block_index = 11
        },
        CarrotPreSelectedInput {
            .core = CarrotSelectedInput {
                .amount = 100,
                .key_image = mock::gen_key_image(),
            },
            .is_external = false,
            .block_index = 88
        },
        CarrotPreSelectedInput {
            .core = CarrotSelectedInput {
                .amount = 100,
                .key_image = mock::gen_key_image(),
            },
            .is_external = false,
            .block_index = 72
        }
    };

    const std::vector<input_selection_policy_t> policies = { &carrot::ispolicy::select_greedy_aging };

    const uint32_t flags = 0;

    std::set<size_t> selected_input_indices;
    select_inputs_func_t input_selector = make_single_transfer_input_selector(epee::to_span(input_candidates),
        epee::to_span(policies),
        flags,
        &selected_input_indices);

    boost::multiprecision::int128_t nominal_output_sum = 223;

    const std::map<size_t, rct::xmr_amount> fee_by_input_count = {
        {1, 10},
        {2, 20},
        {3, 29},
        {4, 37},
        {5, 44},
        {6, 50},
        {7, 55},
        {8, 59},
    };

    const size_t num_normal_payment_proposals = 1;
    const size_t num_selfsend_payment_proposals = 1;

    std::vector<CarrotSelectedInput> selected_inputs;
    input_selector(nominal_output_sum,
        fee_by_input_count,
        num_normal_payment_proposals,
        num_selfsend_payment_proposals,
        selected_inputs);

    ASSERT_EQ(4, selected_input_indices.size()); // discretization yay
    ASSERT_EQ(4, selected_inputs.size());
    std::set<crypto::key_image> selected_kis;
    for (const CarrotSelectedInput &selected_input : selected_inputs)
        selected_kis.insert(selected_input.key_image);
    ASSERT_EQ(4, selected_kis.size());

    // this particular set of 4 indices is the indices of the 4 oldest inputs
    ASSERT_EQ(std::set<size_t>({0, 1, 2, 4}), selected_input_indices);
}
//----------------------------------------------------------------------------------------------------------------------
