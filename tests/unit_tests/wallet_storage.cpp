// Copyright (c) 2023, The Monero Project
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

#include "unit_tests_utils.h"
#include "gtest/gtest.h"

#include "file_io_utils.h"
#include "wallet/wallet2.h"
#include "common/util.h"

using namespace boost::filesystem;
using namespace epee::file_io_utils;

static constexpr const char WALLET_00fd416a_PRIMARY_ADDRESS[] =
    "45p2SngJAPSJbqSiUvYfS3BfhEdxZmv8pDt25oW1LzxrZv9Uq6ARagiFViMGUE3gJk5VPWingCXVf1p2tyAy6SUeSHPhbve";

class wallet_accessor_test
{
public:
    static tools::wallet2::unconfirmed_transfer_details &unconfirmed_tx(tools::wallet2 &wallet, const crypto::hash &txid)
    {
        return wallet.m_unconfirmed_txs[txid];
    }

    static const tools::wallet2::confirmed_transfer_details &confirm_outgoing(tools::wallet2 &wallet,
        const crypto::hash &txid, const cryptonote::transaction &tx, uint64_t spent, uint64_t received,
        const std::set<uint32_t> &indices)
    {
        wallet.process_unconfirmed(txid, tx, 100);
        wallet.process_outgoing(txid, tx, 100, 200, spent, received, 0, indices);
        return wallet.m_confirmed_txs.at(txid);
    }

    static void set_pool_state(tools::wallet2 &wallet, uint64_t query_time, uint64_t daemon_height)
    {
        wallet.m_pool_info_query_time = query_time;
        wallet.m_node_rpc_proxy.set_height(daemon_height);
    }

    static const tools::wallet2::transfer_details &add_spent_output(tools::wallet2 &wallet, cryptonote::transaction &tx)
    {
        cryptonote::txin_to_key input = AUTO_VAL_INIT(input);
        input.k_image = crypto::rand<crypto::key_image>();
        tx.vin.push_back(input);
        wallet.m_transfers.emplace_back();
        auto &transfer = wallet.m_transfers.back();
        transfer.m_amount = 9;
        transfer.m_spent = true;
        transfer.m_spent_height = 100;
        transfer.m_key_image = input.k_image;
        transfer.m_key_image_known = true;
        transfer.m_subaddr_index = {0, 0};
        wallet.m_key_images[input.k_image] = wallet.m_transfers.size() - 1;
        return transfer;
    }
};

TEST(wallet_storage, store_to_file2file)
{
    const path source_wallet_file = unit_test::data_dir / "wallet_00fd416a";
    const path interm_wallet_file = unit_test::data_dir / "wallet_00fd416a_copy_file2file";
    const path target_wallet_file = unit_test::data_dir / "wallet_00fd416a_new_file2file";

    ASSERT_TRUE(is_file_exist(source_wallet_file.string()));
    ASSERT_TRUE(is_file_exist(source_wallet_file.string() + ".keys"));

    tools::copy_file(source_wallet_file.string(), interm_wallet_file.string());
    tools::copy_file(source_wallet_file.string() + ".keys", interm_wallet_file.string() + ".keys");

    ASSERT_TRUE(is_file_exist(interm_wallet_file.string()));
    ASSERT_TRUE(is_file_exist(interm_wallet_file.string() + ".keys"));

    if (is_file_exist(target_wallet_file.string()))
        remove(target_wallet_file);
    if (is_file_exist(target_wallet_file.string() + ".keys"))
        remove(target_wallet_file.string() + ".keys");
    ASSERT_FALSE(is_file_exist(target_wallet_file.string()));
    ASSERT_FALSE(is_file_exist(target_wallet_file.string() + ".keys"));

    epee::wipeable_string password("beepbeep");

    const auto files_are_expected = [&]()
    {
        EXPECT_FALSE(is_file_exist(interm_wallet_file.string()));
        EXPECT_FALSE(is_file_exist(interm_wallet_file.string() + ".keys"));
        EXPECT_TRUE(is_file_exist(target_wallet_file.string()));
        EXPECT_TRUE(is_file_exist(target_wallet_file.string() + ".keys"));
    };

    {
        tools::wallet2 w;
        w.load(interm_wallet_file.string(), password);
        const std::string primary_address = w.get_address_as_str();
        EXPECT_EQ(WALLET_00fd416a_PRIMARY_ADDRESS, primary_address);
        w.store_to(target_wallet_file.string(), password);
        files_are_expected();
    }

    files_are_expected();

    {
        tools::wallet2 w;
        w.load(target_wallet_file.string(), password);
        const std::string primary_address = w.get_address_as_str();
        EXPECT_EQ(WALLET_00fd416a_PRIMARY_ADDRESS, primary_address);
        w.store_to("", "");
        files_are_expected();
    }

    files_are_expected();
}

TEST(wallet_storage, store_to_mem2file)
{
    const path target_wallet_file = unit_test::data_dir / "wallet_mem2file";

    if (is_file_exist(target_wallet_file.string()))
        remove(target_wallet_file);
    if (is_file_exist(target_wallet_file.string() + ".keys"))
        remove(target_wallet_file.string() + ".keys");
    ASSERT_FALSE(is_file_exist(target_wallet_file.string()));
    ASSERT_FALSE(is_file_exist(target_wallet_file.string() + ".keys"));

    epee::wipeable_string password("beepbeep2");

    {
        tools::wallet2 w;
        w.generate("", password);
        w.store_to(target_wallet_file.string(), password);

        EXPECT_TRUE(is_file_exist(target_wallet_file.string()));
        EXPECT_TRUE(is_file_exist(target_wallet_file.string() + ".keys"));
    }

    EXPECT_TRUE(is_file_exist(target_wallet_file.string()));
    EXPECT_TRUE(is_file_exist(target_wallet_file.string() + ".keys"));

    {
        tools::wallet2 w;
        w.load(target_wallet_file.string(), password);

        EXPECT_TRUE(is_file_exist(target_wallet_file.string()));
        EXPECT_TRUE(is_file_exist(target_wallet_file.string() + ".keys"));
    }

    EXPECT_TRUE(is_file_exist(target_wallet_file.string()));
    EXPECT_TRUE(is_file_exist(target_wallet_file.string() + ".keys"));
}

TEST(wallet_storage, change_password_same_file)
{
    const path source_wallet_file = unit_test::data_dir / "wallet_00fd416a";
    const path interm_wallet_file = unit_test::data_dir / "wallet_00fd416a_copy_change_password_same";

    ASSERT_TRUE(is_file_exist(source_wallet_file.string()));
    ASSERT_TRUE(is_file_exist(source_wallet_file.string() + ".keys"));

    tools::copy_file(source_wallet_file.string(), interm_wallet_file.string());
    tools::copy_file(source_wallet_file.string() + ".keys", interm_wallet_file.string() + ".keys");

    ASSERT_TRUE(is_file_exist(interm_wallet_file.string()));
    ASSERT_TRUE(is_file_exist(interm_wallet_file.string() + ".keys"));

    epee::wipeable_string old_password("beepbeep");
    epee::wipeable_string new_password("meepmeep");

    {
        tools::wallet2 w;
        w.load(interm_wallet_file.string(), old_password);
        const std::string primary_address = w.get_address_as_str();
        EXPECT_EQ(WALLET_00fd416a_PRIMARY_ADDRESS, primary_address);
        w.change_password(w.get_wallet_file(), old_password, new_password);
    }

    {
        tools::wallet2 w;
        w.load(interm_wallet_file.string(), new_password);
        const std::string primary_address = w.get_address_as_str();
        EXPECT_EQ(WALLET_00fd416a_PRIMARY_ADDRESS, primary_address);
    }

    {
        tools::wallet2 w;
        EXPECT_THROW(w.load(interm_wallet_file.string(), old_password), tools::error::invalid_password);
    }
}

TEST(wallet_storage, change_password_different_file)
{
    const path source_wallet_file = unit_test::data_dir / "wallet_00fd416a";
    const path interm_wallet_file = unit_test::data_dir / "wallet_00fd416a_copy_change_password_diff";
    const path target_wallet_file = unit_test::data_dir / "wallet_00fd416a_new_change_password_diff";

    ASSERT_TRUE(is_file_exist(source_wallet_file.string()));
    ASSERT_TRUE(is_file_exist(source_wallet_file.string() + ".keys"));

    tools::copy_file(source_wallet_file.string(), interm_wallet_file.string());
    tools::copy_file(source_wallet_file.string() + ".keys", interm_wallet_file.string() + ".keys");

    ASSERT_TRUE(is_file_exist(interm_wallet_file.string()));
    ASSERT_TRUE(is_file_exist(interm_wallet_file.string() + ".keys"));

    if (is_file_exist(target_wallet_file.string()))
        remove(target_wallet_file);
    if (is_file_exist(target_wallet_file.string() + ".keys"))
        remove(target_wallet_file.string() + ".keys");
    ASSERT_FALSE(is_file_exist(target_wallet_file.string()));
    ASSERT_FALSE(is_file_exist(target_wallet_file.string() + ".keys"));

    epee::wipeable_string old_password("beepbeep");
    epee::wipeable_string new_password("meepmeep");

    {
        tools::wallet2 w;
        w.load(interm_wallet_file.string(), old_password);
        const std::string primary_address = w.get_address_as_str();
        EXPECT_EQ(WALLET_00fd416a_PRIMARY_ADDRESS, primary_address);
        w.change_password(target_wallet_file.string(), old_password, new_password);
    }

    EXPECT_FALSE(is_file_exist(interm_wallet_file.string()));
    EXPECT_FALSE(is_file_exist(interm_wallet_file.string() + ".keys"));
    EXPECT_TRUE(is_file_exist(target_wallet_file.string()));
    EXPECT_TRUE(is_file_exist(target_wallet_file.string() + ".keys"));

    {
        tools::wallet2 w;
        w.load(target_wallet_file.string(), new_password);
        const std::string primary_address = w.get_address_as_str();
        EXPECT_EQ(WALLET_00fd416a_PRIMARY_ADDRESS, primary_address);
    }
}

TEST(wallet_storage, change_password_in_memory)
{
    const epee::wipeable_string password1("monero");
    const epee::wipeable_string password2("means money");
    const epee::wipeable_string password_wrong("is traceable");

    tools::wallet2 w;
    w.generate("", password1);
    const std::string primary_address_1 = w.get_address_as_str();
    w.change_password("", password1, password2);
    const std::string primary_address_2 = w.get_address_as_str();
    EXPECT_EQ(primary_address_1, primary_address_2);

    EXPECT_THROW(w.change_password("", password_wrong, password1), tools::error::invalid_password);
}

TEST(wallet_storage, change_password_mem2file)
{
    const path target_wallet_file = unit_test::data_dir / "wallet_change_password_mem2file";

    if (is_file_exist(target_wallet_file.string()))
        remove(target_wallet_file);
    if (is_file_exist(target_wallet_file.string() + ".keys"))
        remove(target_wallet_file.string() + ".keys");
    ASSERT_FALSE(is_file_exist(target_wallet_file.string()));
    ASSERT_FALSE(is_file_exist(target_wallet_file.string() + ".keys"));

    const epee::wipeable_string password1("https://safecurves.cr.yp.to/rigid.html");
    const epee::wipeable_string password2(
        "https://csrc.nist.gov/csrc/media/projects/crypto-standards-development-process/documents/dualec_in_x982_and_sp800-90.pdf");
    
    std::string primary_address_1, primary_address_2;
    {
        tools::wallet2 w;
        w.generate("", password1);
        primary_address_1 = w.get_address_as_str();
        w.change_password(target_wallet_file.string(), password1, password2);
    }

    EXPECT_TRUE(is_file_exist(target_wallet_file.string()));
    EXPECT_TRUE(is_file_exist(target_wallet_file.string() + ".keys"));

    {
        tools::wallet2 w;
        w.load(target_wallet_file.string(), password2);
        primary_address_2 = w.get_address_as_str();
    }

    EXPECT_EQ(primary_address_1, primary_address_2);
}

TEST(wallet_pool, reconciles_partial_outgoing_on_confirmation)
{
    tools::wallet2 w;
    cryptonote::transaction tx;
    tx.version = 2;
    tx.rct_signatures.txnFee = 1;
    const crypto::hash txid = crypto::null_hash;
    auto &pending = wallet_accessor_test::unconfirmed_tx(w, txid);
    pending.m_tx = tx;
    pending.m_amount_in = 4;
    pending.m_amount_out = 3;
    pending.m_change = 2;
    pending.m_subaddr_account = 0;
    pending.m_subaddr_indices = {0};
    pending.m_dests.resize(1);
    pending.m_dests[0].amount = 6;
    pending.m_payment_id = crypto::rand<crypto::hash>();
    const crypto::hash payment_id = pending.m_payment_id;

    const auto &confirmed = wallet_accessor_test::confirm_outgoing(w, txid, tx, 9, 2, {1});
    EXPECT_EQ(9, confirmed.m_amount_in);
    EXPECT_EQ(8, confirmed.m_amount_out);
    EXPECT_EQ(6, confirmed.m_amount_out - confirmed.m_change);
    EXPECT_EQ(std::set<uint32_t>({0, 1}), confirmed.m_subaddr_indices);
    ASSERT_EQ(1, confirmed.m_dests.size());
    EXPECT_EQ(6, confirmed.m_dests[0].amount);
    EXPECT_EQ(payment_id, confirmed.m_payment_id);

    // A later scan with fewer known key images must preserve the complete totals.
    wallet_accessor_test::confirm_outgoing(w, txid, tx, 4, 2, {0});
    EXPECT_EQ(9, confirmed.m_amount_in);
    EXPECT_EQ(8, confirmed.m_amount_out);
    EXPECT_EQ(2, confirmed.m_change);
}

TEST(wallet_pool, skips_confirmed_outgoing_in_stale_snapshot)
{
    tools::wallet2 w;
    cryptonote::transaction tx;
    tx.version = 2;
    tx.rct_signatures.txnFee = 1;
    const crypto::hash txid = crypto::null_hash;
    const auto &transfer = wallet_accessor_test::add_spent_output(w, tx);
    wallet_accessor_test::confirm_outgoing(w, txid, tx, 9, 2, {0});
    w.process_pool_state({std::make_tuple(tx, txid, false)});
    EXPECT_EQ(100, transfer.m_spent_height);
    std::list<std::pair<crypto::hash, tools::wallet2::unconfirmed_transfer_details>> pending;
    w.get_unconfirmed_payments_out(pending);
    EXPECT_TRUE(pending.empty());
}

namespace
{
    class pool_http_client : public net::http::client
    {
        epee::net_utils::http::http_response_info response;
        size_t &queries;

    public:
        explicit pool_http_client(size_t &queries) : queries(queries) {}

        bool invoke(const boost::string_ref uri, const boost::string_ref method, const boost::string_ref body,
            std::chrono::milliseconds timeout, const epee::net_utils::http::http_response_info **result,
            const epee::net_utils::http::fields_list &headers) override
        {
            EXPECT_EQ("/getblocks.bin", uri);
            cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request request = AUTO_VAL_INIT(request);
            EXPECT_TRUE(epee::serialization::load_t_from_binary(request, std::string(body.data(), body.size())));
            EXPECT_EQ(cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::POOL_ONLY, request.requested_info);
            EXPECT_EQ(1, request.pool_info_since);
            ++queries;
            cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response pool = AUTO_VAL_INIT(pool);
            pool.status = CORE_RPC_STATUS_OK;
            pool.pool_info_extent = cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::FULL;
            pool.daemon_time = 2;
            response.m_response_code = 200;
            const auto blob = epee::serialization::store_t_to_binary(pool);
            response.m_body.assign(reinterpret_cast<const char *>(blob.data()), blob.size());
            *result = &response;
            return true;
        }
    };

    class pool_http_client_factory : public epee::net_utils::http::http_client_factory
    {
        size_t &queries;

    public:
        explicit pool_http_client_factory(size_t &queries) : queries(queries) {}

        std::unique_ptr<epee::net_utils::http::abstract_http_client> create() override
        {
            return std::unique_ptr<epee::net_utils::http::abstract_http_client>(new pool_http_client(queries));
        }
    };
}

TEST(wallet_pool, queries_pool_before_chain_catches_up)
{
    size_t queries = 0;
    tools::wallet2 w(cryptonote::MAINNET, 1, true,
        std::unique_ptr<epee::net_utils::http::http_client_factory>(new pool_http_client_factory(queries)));
    wallet_accessor_test::set_pool_state(w, 1, 100);
    std::vector<std::tuple<cryptonote::transaction, crypto::hash, bool>> pool_txs;
    w.update_pool_state(pool_txs, true, true);
    EXPECT_EQ(1, queries);
    EXPECT_TRUE(pool_txs.empty());
}
