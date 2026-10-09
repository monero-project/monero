// Copyright (c) 2023-2026, The Monero Project
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

#include <functional>

#include "wallet/wallet2.h"

class wallet_pool_test
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
        wallet.process_unconfirmed(txid, tx, 0);
        wallet.process_outgoing(txid, tx, 0, 200, spent, received, 0, indices);
        return wallet.m_confirmed_txs.at(txid);
    }

    static void scan_transaction(tools::wallet2 &wallet, const cryptonote::transaction &tx)
    {
        wallet.process_new_transaction(cryptonote::get_transaction_hash(tx), tx,
            std::vector<uint64_t>(tx.vout.size()), 1, 1, 1, false, false, false, {});
    }

    static void detach_incoming(tools::wallet2 &wallet)
    {
        wallet.detach_blockchain(1);
    }

    static void append_block(tools::wallet2 &wallet)
    {
        wallet.m_blockchain.push_back(crypto::rand<crypto::hash>());
    }

    static uint64_t pool_query_time(const tools::wallet2 &wallet)
    {
        return wallet.m_pool_info_query_time;
    }

    static cryptonote::block genesis(tools::wallet2 &wallet)
    {
        cryptonote::block block;
        wallet.generate_genesis(block);
        return block;
    }

    static void prepare_refresh(tools::wallet2 &wallet)
    {
        wallet.set_offline(true);
        wallet.generate("", "");
        wallet.set_offline(false);
        wallet.set_refresh_from_block_height(0);
        wallet.m_first_refresh_done = true;
        wallet.m_pool_info_query_time = 1;
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

TEST(wallet_pool, reconciles_partial_outgoing_on_confirmation)
{
    tools::wallet2 w;
    cryptonote::transaction tx;
    tx.version = 2;
    tx.rct_signatures.txnFee = 1;
    const crypto::hash txid = crypto::null_hash;
    auto &pending = wallet_pool_test::unconfirmed_tx(w, txid);
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

    const std::set<uint32_t> indices = {1};
    const auto &confirmed = wallet_pool_test::confirm_outgoing(w, txid, tx, 9, 2, indices);
    EXPECT_EQ(9, confirmed.m_amount_in);
    EXPECT_EQ(8, confirmed.m_amount_out);
    EXPECT_EQ(6, confirmed.m_amount_out - confirmed.m_change);
    EXPECT_EQ(std::set<uint32_t>({0, 1}), confirmed.m_subaddr_indices);
    ASSERT_EQ(1, confirmed.m_dests.size());
    EXPECT_EQ(6, confirmed.m_dests[0].amount);
    EXPECT_EQ(payment_id, confirmed.m_payment_id);

    // A later scan with fewer known key images must preserve the complete totals.
    wallet_pool_test::confirm_outgoing(w, txid, tx, 4, 2, {0});
    EXPECT_EQ(9, confirmed.m_amount_in);
    EXPECT_EQ(8, confirmed.m_amount_out);
    EXPECT_EQ(2, confirmed.m_change);
}

TEST(wallet_pool, skips_confirmed_outgoing_in_stale_snapshot)
{
    tools::wallet2 w(cryptonote::MAINNET, 1, true);
    w.set_offline(true);
    w.generate("", "");
    cryptonote::transaction tx;
    tx.version = 2;
    tx.rct_signatures.txnFee = 1;
    const crypto::hash txid = crypto::null_hash;
    const auto &transfer = wallet_pool_test::add_spent_output(w, tx);
    wallet_pool_test::confirm_outgoing(w, txid, tx, 9, 2, {0});
    w.process_pool_state({std::make_tuple(tx, txid, false)});
    EXPECT_EQ(100, transfer.m_spent_height);
    std::list<std::pair<crypto::hash, tools::wallet2::unconfirmed_transfer_details>> pending;
    w.get_unconfirmed_payments_out(pending);
    EXPECT_TRUE(pending.empty());
}

namespace
{
    cryptonote::transaction make_pool_payment(const tools::wallet2 &wallet, size_t outputs = 1)
    {
        cryptonote::transaction tx;
        tx.version = 1;
        cryptonote::txin_to_key input = AUTO_VAL_INIT(input);
        input.amount = 8 * outputs + 1;
        input.k_image = crypto::rand<crypto::key_image>();
        input.key_offsets = {0};
        tx.vin.push_back(input);
        const auto &address = wallet.get_account().get_keys().m_account_address;
        crypto::public_key public_key;
        crypto::secret_key secret_key;
        crypto::generate_keys(public_key, secret_key);
        EXPECT_TRUE(cryptonote::add_tx_pub_key_to_extra(tx, public_key));
        crypto::key_derivation derivation;
        EXPECT_TRUE(crypto::generate_key_derivation(address.m_view_public_key, secret_key, derivation));
        for (size_t i = 0; i < outputs; ++i)
        {
            cryptonote::txout_to_key output;
            EXPECT_TRUE(crypto::derive_public_key(derivation, i, address.m_spend_public_key, output.key));
            tx.vout.push_back({8, output});
        }
        return tx;
    }

    class pool_callback : public tools::i_wallet2_callback
    {
    public:
        size_t pool_receipts = 0;
        size_t password_requests = 0;
        std::function<void()> on_pool_receipt;
        void on_unconfirmed_money_received(uint64_t height, const crypto::hash &txid,
            const cryptonote::transaction &tx, uint64_t amount,
            const cryptonote::subaddress_index &index) override
        {
            ++pool_receipts;
            if (on_pool_receipt)
                on_pool_receipt();
        }

        boost::optional<epee::wipeable_string> on_get_password(const char *reason) override
        {
            ++password_requests;
            return boost::none;
        }
    };
}

TEST(wallet_pool, skips_confirmed_incoming_in_stale_snapshot)
{
    tools::wallet2 w(cryptonote::MAINNET, 1, true);
    w.set_offline(true);
    w.generate("", "");
    const auto tx = make_pool_payment(w);
    const auto txid = cryptonote::get_transaction_hash(tx);
    wallet_pool_test::scan_transaction(w, tx);
    ASSERT_EQ(8, w.balance_all(true));
    pool_callback callback;
    w.callback(&callback);

    w.process_pool_state({std::make_tuple(tx, txid, false)});
    std::list<std::pair<crypto::hash, tools::wallet2::pool_payment_details>> pending;
    w.get_unconfirmed_payments(pending);
    EXPECT_TRUE(pending.empty());
    EXPECT_EQ(0, callback.pool_receipts);
    EXPECT_EQ(8, w.balance_all(true));
}

TEST(wallet_pool, processes_incoming_with_reused_output_key)
{
    tools::wallet2 w(cryptonote::MAINNET, 1, true);
    w.set_offline(true);
    w.generate("", "");
    const auto pool_tx = make_pool_payment(w, 2);
    auto confirmed_tx = pool_tx;
    confirmed_tx.vout.pop_back();
    wallet_pool_test::scan_transaction(w, confirmed_tx);
    ASSERT_EQ(8, w.balance_all(true));
    const auto txid = cryptonote::get_transaction_hash(pool_tx);
    ASSERT_NE(cryptonote::get_transaction_hash(confirmed_tx), txid);
    pool_callback callback;
    w.callback(&callback);

    w.process_pool_state({std::make_tuple(pool_tx, txid, false)});
    std::list<std::pair<crypto::hash, tools::wallet2::pool_payment_details>> pending;
    w.get_unconfirmed_payments(pending);
    ASSERT_EQ(1, pending.size());
    EXPECT_EQ(txid, pending.front().second.m_pd.m_tx_hash);
    EXPECT_EQ(8, pending.front().second.m_pd.m_amount);
    EXPECT_EQ(1, callback.pool_receipts);
    EXPECT_EQ(8, w.balance_all(true));
}

TEST(wallet_pool, processes_incoming_after_detach)
{
    tools::wallet2 w(cryptonote::MAINNET, 1, true);
    w.set_offline(true);
    w.generate("", "");
    const auto tx = make_pool_payment(w);
    const auto txid = cryptonote::get_transaction_hash(tx);
    wallet_pool_test::scan_transaction(w, tx);
    ASSERT_EQ(8, w.balance_all(true));
    wallet_pool_test::detach_incoming(w);
    ASSERT_EQ(0, w.balance_all(true));
    pool_callback callback;
    w.callback(&callback);

    w.process_pool_state({std::make_tuple(tx, txid, false)});
    std::list<std::pair<crypto::hash, tools::wallet2::pool_payment_details>> pending;
    w.get_unconfirmed_payments(pending);
    ASSERT_EQ(1, pending.size());
    EXPECT_EQ(txid, pending.front().second.m_pd.m_tx_hash);
    EXPECT_EQ(8, pending.front().second.m_pd.m_amount);
    EXPECT_EQ(1, callback.pool_receipts);
    EXPECT_EQ(0, w.balance_all(true));
}

namespace
{
    using pool_reply = std::function<void(const cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request &,
        cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response &)>;
    using transaction_reply = std::function<bool(const cryptonote::COMMAND_RPC_GET_TRANSACTIONS::request &,
        cryptonote::COMMAND_RPC_GET_TRANSACTIONS::response &)>;

    class pool_http_client : public net::http::client
    {
        epee::net_utils::http::http_response_info response;
        pool_reply reply;
        transaction_reply tx_reply;

    public:
        pool_http_client(const pool_reply &reply, const transaction_reply &tx_reply)
            : reply(reply), tx_reply(tx_reply) {}

        bool invoke(const boost::string_ref uri, const boost::string_ref method, const boost::string_ref body,
            std::chrono::milliseconds timeout, const epee::net_utils::http::http_response_info **result,
            const epee::net_utils::http::fields_list &headers) override
        {
            if (uri == "/gettransactions")
            {
                EXPECT_TRUE(bool(tx_reply));
                if (!tx_reply)
                    return false;
                cryptonote::COMMAND_RPC_GET_TRANSACTIONS::request request = AUTO_VAL_INIT(request);
                EXPECT_TRUE(epee::serialization::load_t_from_json(request, std::string(body.data(), body.size())));
                cryptonote::COMMAND_RPC_GET_TRANSACTIONS::response txs = AUTO_VAL_INIT(txs);
                txs.status = CORE_RPC_STATUS_OK;
                if (!tx_reply(request, txs))
                    return false;
                response.m_response_code = 200;
                response.m_body = epee::serialization::store_t_to_json(txs);
                *result = &response;
                return true;
            }
            const bool hashes_only = uri == "/get_transaction_pool_hashes.bin";
            EXPECT_TRUE(hashes_only || uri == "/getblocks.bin");
            cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request request = AUTO_VAL_INIT(request);
            if (hashes_only)
                request.requested_info = cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::POOL_ONLY;
            else
                EXPECT_TRUE(epee::serialization::load_t_from_binary(request, std::string(body.data(), body.size())));
            cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response pool = AUTO_VAL_INIT(pool);
            pool.status = CORE_RPC_STATUS_OK;
            pool.pool_info_extent = request.pool_info_since
                ? cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::INCREMENTAL
                : cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::FULL;
            pool.daemon_time = 2;
            reply(request, pool);
            response.m_response_code = 200;
            if (hashes_only)
            {
                cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL_HASHES_BIN::response hashes = AUTO_VAL_INIT(hashes);
                hashes.status = pool.status;
                hashes.tx_hashes = pool.remaining_added_pool_txids;
                for (const auto &info : pool.added_pool_txs)
                    hashes.tx_hashes.push_back(info.tx_hash);
                response.m_body = epee::serialization::store_t_to_json(hashes);
            }
            else
            {
                const auto blob = epee::serialization::store_t_to_binary(pool);
                response.m_body.assign(reinterpret_cast<const char *>(blob.data()), blob.size());
            }
            *result = &response;
            return true;
        }
    };

    class pool_http_client_factory : public epee::net_utils::http::http_client_factory
    {
        pool_reply reply;
        transaction_reply tx_reply;

    public:
        explicit pool_http_client_factory(const pool_reply &reply, const transaction_reply &tx_reply = {})
            : reply(reply), tx_reply(tx_reply) {}

        std::unique_ptr<epee::net_utils::http::abstract_http_client> create() override
        {
            return std::unique_ptr<epee::net_utils::http::abstract_http_client>(new pool_http_client(reply, tx_reply));
        }
    };

    cryptonote::block make_refresh_block(uint64_t height, const crypto::hash &previous,
        const cryptonote::account_public_address &address)
    {
        cryptonote::block block;
        block.major_version = 1;
        block.minor_version = 1;
        block.timestamp = std::time(nullptr);
        block.prev_id = previous;
        EXPECT_TRUE(cryptonote::construct_miner_tx(height, 0, 0, 0, 0, address,
            block.miner_tx));
        return block;
    }

    void add_refresh_block(cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response &response,
        const cryptonote::block &block)
    {
        if (response.blocks.empty())
            response.start_height = cryptonote::get_block_height(block);
        response.blocks.emplace_back();
        response.blocks.back().block = cryptonote::block_to_blob(block);
        response.output_indices.emplace_back();
        response.output_indices.back().indices.resize(1);
        response.output_indices.back().indices[0].indices.resize(block.miner_tx.vout.size());
    }

    void add_pool_payment(cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response &response,
        const cryptonote::transaction &tx)
    {
        cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::pool_tx_info info = AUTO_VAL_INIT(info);
        info.tx_hash = cryptonote::get_transaction_hash(tx);
        info.tx_blob = cryptonote::tx_to_blob(tx);
        response.added_pool_txs.push_back(info);
    }
}

TEST(wallet_pool, advances_cursor_on_successful_refresh)
{
    cryptonote::transaction pool_tx;
    const auto reply = [&](const cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request &request,
        cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response &response)
    {
        EXPECT_EQ(cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::BLOCKS_AND_POOL, request.requested_info);
        EXPECT_EQ(1, request.pool_info_since);
        response.start_height = response.current_height = 1;
        add_pool_payment(response, pool_tx);
    };
    tools::wallet2 w(cryptonote::MAINNET, 1, true,
        std::unique_ptr<epee::net_utils::http::http_client_factory>(new pool_http_client_factory(reply)));
    wallet_pool_test::prepare_refresh(w);
    pool_tx = make_pool_payment(w);
    pool_callback callback;
    callback.on_pool_receipt = [&] { EXPECT_EQ(2, wallet_pool_test::pool_query_time(w)); };
    w.callback(&callback);

    uint64_t fetched = 0;
    bool received = false;
    w.refresh(true, 0, fetched, received);
    EXPECT_EQ(1, callback.pool_receipts);
    EXPECT_EQ(2, wallet_pool_test::pool_query_time(w));
}

TEST(wallet_pool, retries_pool_after_stopped_refresh)
{
    tools::wallet2 *wallet = nullptr;
    size_t queries = 0;
    cryptonote::transaction pool_tx;
    const auto reply = [&](const cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request &request,
        cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response &response)
    {
        EXPECT_EQ(queries == 0 ? 1 : 0, request.pool_info_since);
        response.start_height = response.current_height = 1;
        add_pool_payment(response, pool_tx);
        if (queries++ == 0)
            wallet->stop();
    };
    tools::wallet2 w(cryptonote::MAINNET, 1, true,
        std::unique_ptr<epee::net_utils::http::http_client_factory>(new pool_http_client_factory(reply)));
    wallet = &w;
    wallet_pool_test::prepare_refresh(w);
    pool_tx = make_pool_payment(w);
    pool_callback callback;
    w.callback(&callback);

    uint64_t fetched = 0;
    bool received = false;
    w.refresh(true, 0, fetched, received);
    EXPECT_EQ(0, callback.pool_receipts);
    EXPECT_EQ(0, wallet_pool_test::pool_query_time(w));
    w.refresh(true, 0, fetched, received);
    EXPECT_EQ(2, queries);
    EXPECT_EQ(1, callback.pool_receipts);
    EXPECT_EQ(2, wallet_pool_test::pool_query_time(w));
}

TEST(wallet_pool, retries_pool_after_password_failure)
{
    size_t queries = 0;
    cryptonote::transaction pool_tx;
    const auto reply = [&](const cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request &request,
        cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response &response)
    {
        EXPECT_EQ(queries == 0 ? 1 : 0, request.pool_info_since);
        ++queries;
        response.start_height = response.current_height = 1;
        add_pool_payment(response, pool_tx);
    };
    tools::wallet2 w(cryptonote::MAINNET, 1, false,
        std::unique_ptr<epee::net_utils::http::http_client_factory>(new pool_http_client_factory(reply)));
    wallet_pool_test::prepare_refresh(w);
    pool_tx = make_pool_payment(w);
    pool_callback callback;
    w.callback(&callback);

    uint64_t fetched = 0;
    bool received = false;
    w.refresh(true, 0, fetched, received);
    EXPECT_EQ(1, callback.password_requests);
    EXPECT_EQ(0, callback.pool_receipts);
    EXPECT_EQ(0, wallet_pool_test::pool_query_time(w));
    w.decrypt_keys(epee::wipeable_string(""));
    w.ask_password(tools::wallet2::AskPasswordNever);
    w.refresh(true, 0, fetched, received);
    EXPECT_EQ(2, queries);
    EXPECT_EQ(1, callback.pool_receipts);
    EXPECT_EQ(2, wallet_pool_test::pool_query_time(w));
}

TEST(wallet_pool, advances_skipped_pool_before_block_password_failure)
{
    for (const bool check_pool : {false, true})
    {
        size_t queries = 0;
        std::vector<cryptonote::block> blocks;
        const auto reply = [&](const cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request &request,
            cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response &response)
        {
            EXPECT_EQ(cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::BLOCKS_AND_POOL, request.requested_info);
            EXPECT_EQ(queries == 0 ? 1 : check_pool ? 0 : 2, request.pool_info_since);
            ++queries;
            response.current_height = blocks.size();
            for (const auto &block : blocks)
                add_refresh_block(response, block);
        };
        tools::wallet2 w(cryptonote::MAINNET, 1, false,
            std::unique_ptr<epee::net_utils::http::http_client_factory>(new pool_http_client_factory(reply)));
        wallet_pool_test::prepare_refresh(w);
        blocks.push_back(wallet_pool_test::genesis(w));
        blocks.push_back(make_refresh_block(1, cryptonote::get_block_hash(blocks[0]),
            w.get_account().get_keys().m_account_address));
        pool_callback callback;
        w.callback(&callback);

        uint64_t fetched = 0;
        bool received = false;
        EXPECT_THROW(w.refresh(true, 0, fetched, received, check_pool), tools::error::password_needed);
        EXPECT_EQ(check_pool ? 0 : 2, wallet_pool_test::pool_query_time(w));
        EXPECT_THROW(w.refresh(true, 0, fetched, received, check_pool), tools::error::password_needed);
        EXPECT_EQ(2, queries);
        EXPECT_EQ(2, callback.password_requests);
        EXPECT_EQ(0, callback.pool_receipts);
    }
}

TEST(wallet_pool, preserves_daemon_reset_during_pool_processing)
{
    for (const bool switch_back : {false, true})
    {
        size_t queries = 0;
        cryptonote::transaction pool_tx;
        const auto reply = [&](const cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request &request,
            cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response &response)
        {
            EXPECT_EQ(queries++ == 0 ? 1 : 0, request.pool_info_since);
            response.start_height = response.current_height = 1;
            add_pool_payment(response, pool_tx);
        };
        tools::wallet2 w(cryptonote::MAINNET, 1, true,
            std::unique_ptr<epee::net_utils::http::http_client_factory>(new pool_http_client_factory(reply)));
        ASSERT_TRUE(w.set_daemon("http://127.0.0.1:18081"));
        wallet_pool_test::prepare_refresh(w);
        pool_tx = make_pool_payment(w);
        pool_callback callback;
        callback.on_pool_receipt = [&]
        {
            EXPECT_TRUE(w.set_daemon("http://127.0.0.1:28081"));
            if (switch_back)
                EXPECT_TRUE(w.set_daemon("http://127.0.0.1:18081"));
        };
        w.callback(&callback);

        uint64_t fetched = 0;
        bool received = false;
        w.refresh(true, 0, fetched, received);
        EXPECT_EQ(1, callback.pool_receipts);
        EXPECT_EQ(0, wallet_pool_test::pool_query_time(w));
        callback.on_pool_receipt = nullptr;
        w.refresh(true, 0, fetched, received);
        EXPECT_EQ(2, queries);
        EXPECT_EQ(2, wallet_pool_test::pool_query_time(w));
    }
}

TEST(wallet_pool, preserves_pool_membership_when_blobs_are_malformed)
{
    size_t queries = 0;
    cryptonote::transaction pool_tx, outgoing_tx;
    std::vector<cryptonote::block> blocks;
    const auto reply = [&](const cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request &request,
        cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response &response)
    {
        response.pool_info_extent = cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::FULL;
        response.start_height = response.current_height = blocks.size();
        if (queries == 0)
            for (const auto &block : blocks)
                add_refresh_block(response, block);
        add_pool_payment(response, pool_tx);
        add_pool_payment(response, outgoing_tx);
        if (queries++ == 1)
            for (auto &info : response.added_pool_txs)
                info.tx_blob.clear();
        response.added_pool_txs.emplace_back();
    };
    tools::wallet2 w(cryptonote::MAINNET, 1, true,
        std::unique_ptr<epee::net_utils::http::http_client_factory>(new pool_http_client_factory(reply)));
    wallet_pool_test::prepare_refresh(w);
    pool_tx = make_pool_payment(w);
    outgoing_tx.version = 1;
    wallet_pool_test::add_spent_output(w, outgoing_tx);
    const auto outgoing_txid = cryptonote::get_transaction_hash(outgoing_tx);
    auto &pending_out = wallet_pool_test::unconfirmed_tx(w, outgoing_txid);
    pending_out.m_tx = outgoing_tx;
    pending_out.m_sent_time = 0;
    blocks.push_back(wallet_pool_test::genesis(w));
    blocks.push_back(make_refresh_block(1, cryptonote::get_block_hash(blocks[0]),
        w.get_account().get_keys().m_account_address));
    pool_callback callback;
    w.callback(&callback);

    for (size_t i = 0; i < 3; ++i)
    {
        uint64_t fetched = 0;
        bool received = false;
        w.refresh(true, 0, fetched, received);
        EXPECT_EQ(i + 1, queries);
        EXPECT_EQ(i == 0 ? 1 : 0, fetched);
        EXPECT_EQ(cryptonote::get_outs_money_amount(blocks[1].miner_tx), w.balance_all(true));
        EXPECT_EQ(1, callback.pool_receipts);
        EXPECT_EQ(0, wallet_pool_test::pool_query_time(w));
        std::list<std::pair<crypto::hash, tools::wallet2::pool_payment_details>> pending;
        w.get_unconfirmed_payments(pending);
        ASSERT_EQ(1, pending.size());
        EXPECT_EQ(8, pending.front().second.m_pd.m_amount);
        EXPECT_EQ(tools::wallet2::unconfirmed_transfer_details::pending_in_pool, pending_out.m_state);
        tools::wallet2::transfer_container transfers;
        w.get_transfers(transfers);
        EXPECT_TRUE(transfers.front().m_spent);
    }
}

TEST(wallet_pool, restores_pending_from_unappended_block)
{
    for (const bool with_change : {false, true})
    {
        tools::wallet2 w(cryptonote::MAINNET, 1, true);
        w.set_offline(true);
        w.generate("", "");
        auto tx = make_pool_payment(w, with_change ? 1 : 0);
        tx.vin.clear();
        wallet_pool_test::add_spent_output(w, tx);
        boost::get<cryptonote::txin_to_key>(tx.vin[0]).amount = 9;
        const auto txid = cryptonote::get_transaction_hash(tx);
        wallet_pool_test::unconfirmed_tx(w, txid).m_tx = tx;
        wallet_pool_test::scan_transaction(w, tx);
        ASSERT_EQ(1, w.get_blockchain_current_height());
        std::list<std::pair<crypto::hash, tools::wallet2::unconfirmed_transfer_details>> pending;
        w.get_unconfirmed_payments_out(pending);
        ASSERT_TRUE(pending.empty());

        w.process_pool_state({std::make_tuple(tx, txid, false)});
        w.get_unconfirmed_payments_out(pending);
        ASSERT_EQ(1, pending.size());
        EXPECT_EQ(txid, pending.front().first);
        std::list<std::pair<crypto::hash, tools::wallet2::pool_payment_details>> incoming;
        w.get_unconfirmed_payments(incoming);
        EXPECT_TRUE(incoming.empty());
    }
}

TEST(wallet_pool, retries_pool_after_standalone_processing_failure)
{
    size_t queries = 0;
    cryptonote::transaction tx;
    const auto reply = [&](const cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request &request,
        cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response &response)
    {
        EXPECT_EQ(queries == 0 ? 1 : 0, request.pool_info_since);
        EXPECT_EQ(queries++ == 0 ? cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::POOL_ONLY
            : cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::BLOCKS_AND_POOL, request.requested_info);
        response.start_height = response.current_height = 1;
        add_pool_payment(response, tx);
    };
    tools::wallet2 w(cryptonote::MAINNET, 1, false,
        std::unique_ptr<epee::net_utils::http::http_client_factory>(new pool_http_client_factory(reply)));
    wallet_pool_test::prepare_refresh(w);
    tx = make_pool_payment(w);
    pool_callback callback;
    w.callback(&callback);

    std::vector<std::tuple<cryptonote::transaction, crypto::hash, bool>> txs;
    w.update_pool_state(txs, false, true);
    ASSERT_EQ(1, txs.size());
    EXPECT_THROW(w.process_pool_state(txs), tools::error::password_needed);
    EXPECT_EQ(0, wallet_pool_test::pool_query_time(w));
    EXPECT_EQ(0, callback.pool_receipts);

    w.decrypt_keys(epee::wipeable_string(""));
    w.ask_password(tools::wallet2::AskPasswordNever);
    uint64_t fetched = 0;
    bool received = false;
    w.refresh(true, 0, fetched, received);
    EXPECT_EQ(2, queries);
    EXPECT_EQ(1, callback.pool_receipts);
    EXPECT_EQ(2, wallet_pool_test::pool_query_time(w));
}

TEST(wallet_pool, retries_skipped_incoming_after_pool_only_query)
{
    size_t queries = 0;
    cryptonote::transaction tx;
    const auto reply = [&](const cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request &request,
        cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response &response)
    {
        EXPECT_EQ(queries == 0 ? 1 : 0, request.pool_info_since);
        ++queries;
        response.start_height = response.current_height = 1;
        add_pool_payment(response, tx);
    };
    tools::wallet2 w(cryptonote::MAINNET, 1, true,
        std::unique_ptr<epee::net_utils::http::http_client_factory>(new pool_http_client_factory(reply)));
    wallet_pool_test::prepare_refresh(w);
    tx = make_pool_payment(w);
    wallet_pool_test::scan_transaction(w, tx);
    wallet_pool_test::append_block(w);
    pool_callback callback;
    w.callback(&callback);

    std::vector<std::tuple<cryptonote::transaction, crypto::hash, bool>> txs;
    w.update_pool_state(txs, false, true);
    ASSERT_EQ(1, txs.size());
    w.process_pool_state(txs);
    EXPECT_EQ(0, callback.pool_receipts);
    EXPECT_EQ(0, wallet_pool_test::pool_query_time(w));
    wallet_pool_test::detach_incoming(w);
    uint64_t fetched = 0;
    bool received = false;
    w.refresh(true, 0, fetched, received);
    EXPECT_EQ(2, queries);
    EXPECT_EQ(1, callback.pool_receipts);
}

TEST(wallet_pool, retries_incomplete_remaining_pool_transactions)
{
    // Exercise refresh, incremental standalone queries, and legacy standalone queries.
    for (unsigned mode = 0; mode < 3; ++mode)
    for (unsigned failure = 0; failure < 5; ++failure)
    {
        SCOPED_TRACE(::testing::Message() << "mode=" << mode << ", failure=" << failure);
        size_t queries = 0, body_queries = 0;
        cryptonote::transaction incoming, outgoing;
        const auto reply = [&](const cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request &request,
            cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response &response)
        {
            EXPECT_EQ(queries == 0 && mode != 2 ? 1 : 0, request.pool_info_since);
            response.start_height = response.current_height = 1;
            if (queries++ == 0)
            {
                response.pool_info_extent = cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::FULL;
                response.remaining_added_pool_txids = {cryptonote::get_transaction_hash(incoming),
                    cryptonote::get_transaction_hash(outgoing)};
            }
            else
            {
                add_pool_payment(response, incoming);
                add_pool_payment(response, outgoing);
            }
        };
        const auto tx_reply = [&](const cryptonote::COMMAND_RPC_GET_TRANSACTIONS::request &request,
            cryptonote::COMMAND_RPC_GET_TRANSACTIONS::response &response)
        {
            ++body_queries;
            EXPECT_EQ(mode == 2 ? 1 : 2, request.txs_hashes.size());
            if (failure == 0)
                return false;
            if (failure == 1)
                response.status = CORE_RPC_STATUS_BUSY;
            if (failure >= 3)
            {
                response.txs.resize(request.txs_hashes.size());
                for (auto &entry : response.txs)
                {
                    entry.in_pool = failure == 3;
                    entry.tx_hash = epee::string_tools::pod_to_hex(crypto::rand<crypto::hash>());
                }
            }
            return true;
        };
        tools::wallet2 w(cryptonote::MAINNET, 1, true,
            std::unique_ptr<epee::net_utils::http::http_client_factory>(new pool_http_client_factory(reply, tx_reply)));
        wallet_pool_test::prepare_refresh(w);
        incoming = make_pool_payment(w);
        outgoing.version = 1;
        wallet_pool_test::add_spent_output(w, outgoing);
        auto &pending = wallet_pool_test::unconfirmed_tx(w, cryptonote::get_transaction_hash(outgoing));
        pending.m_tx = outgoing;
        pending.m_sent_time = 0;
        pool_callback callback;
        w.callback(&callback);

        uint64_t fetched = 0;
        bool received = false;
        if (mode != 0)
        {
            std::vector<std::tuple<cryptonote::transaction, crypto::hash, bool>> txs;
            w.update_pool_state(txs, true, mode == 1);
            w.process_pool_state(txs);
        }
        else
            w.refresh(true, 0, fetched, received);
        EXPECT_EQ(1, body_queries);
        EXPECT_EQ(0, callback.pool_receipts);
        EXPECT_EQ(0, wallet_pool_test::pool_query_time(w));
        EXPECT_EQ(tools::wallet2::unconfirmed_transfer_details::pending_in_pool, pending.m_state);
        tools::wallet2::transfer_container transfers;
        w.get_transfers(transfers);
        ASSERT_EQ(1, transfers.size());
        EXPECT_TRUE(transfers.front().m_spent);

        w.refresh(true, 0, fetched, received);
        EXPECT_EQ(2, queries);
        EXPECT_EQ(1, callback.pool_receipts);
        EXPECT_EQ(2, wallet_pool_test::pool_query_time(w));
    }
}

TEST(wallet_pool, preserves_cursor_when_requested_transaction_leaves_pool)
{
    for (unsigned mode = 0; mode < 3; ++mode)
    {
        SCOPED_TRACE(::testing::Message() << "mode=" << mode);
        size_t queries = 0, body_queries = 0;
        crypto::hash txid;
        const auto reply = [&](const cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request &request,
            cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response &response)
        {
            EXPECT_EQ(queries == 0 ? (mode == 2 ? 0 : 1) : (mode == 2 ? 1 : 2), request.pool_info_since);
            response.start_height = response.current_height = 1;
            if (queries++ == 0)
                response.remaining_added_pool_txids = {txid};
            else
                response.removed_pool_txids = {txid};
        };
        const auto tx_reply = [&](const cryptonote::COMMAND_RPC_GET_TRANSACTIONS::request &request,
            cryptonote::COMMAND_RPC_GET_TRANSACTIONS::response &response)
        {
            ++body_queries;
            EXPECT_EQ(1, request.txs_hashes.size());
            for (const auto &hash : request.txs_hashes)
            {
                response.txs.emplace_back();
                response.txs.back().tx_hash = hash;
                response.txs.back().in_pool = false;
            }
            return true;
        };
        tools::wallet2 w(cryptonote::MAINNET, 1, true,
            std::unique_ptr<epee::net_utils::http::http_client_factory>(new pool_http_client_factory(reply, tx_reply)));
        wallet_pool_test::prepare_refresh(w);
        txid = cryptonote::get_transaction_hash(make_pool_payment(w));
        pool_callback callback;
        w.callback(&callback);

        uint64_t fetched = 0;
        bool received = false;
        if (mode != 0)
        {
            std::vector<std::tuple<cryptonote::transaction, crypto::hash, bool>> txs;
            w.update_pool_state(txs, true, mode == 1);
            EXPECT_TRUE(txs.empty());
            w.process_pool_state(txs);
        }
        else
            w.refresh(true, 0, fetched, received);
        EXPECT_EQ(mode == 2 ? 1 : 2, wallet_pool_test::pool_query_time(w));

        w.refresh(true, 0, fetched, received);
        EXPECT_EQ(2, queries);
        EXPECT_EQ(1, body_queries);
        EXPECT_EQ(0, callback.pool_receipts);
        EXPECT_EQ(2, wallet_pool_test::pool_query_time(w));
    }
}

TEST(wallet_pool, retries_full_pool_after_block_parse_failure)
{
    size_t queries = 0;
    cryptonote::transaction tx;
    const auto reply = [&](const cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request &request,
        cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response &response)
    {
        EXPECT_EQ(queries == 0 ? 1 : 0, request.pool_info_since);
        response.start_height = response.current_height = 1;
        add_pool_payment(response, tx);
        if (queries++ == 0)
        {
            response.current_height = 2;
            response.blocks.emplace_back();
            response.output_indices.emplace_back();
        }
    };
    tools::wallet2 w(cryptonote::MAINNET, 1, true,
        std::unique_ptr<epee::net_utils::http::http_client_factory>(new pool_http_client_factory(reply)));
    wallet_pool_test::prepare_refresh(w);
    tx = make_pool_payment(w);
    pool_callback callback;
    w.callback(&callback);

    uint64_t fetched = 0;
    bool received = false;
    w.refresh(true, 0, fetched, received);
    EXPECT_EQ(2, queries);
    EXPECT_EQ(1, callback.pool_receipts);
    EXPECT_EQ(2, wallet_pool_test::pool_query_time(w));
}
