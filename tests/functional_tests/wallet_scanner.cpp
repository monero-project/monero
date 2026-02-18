// Copyright (c) 2014-2024, The Monero Project
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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

//paired header
#include "wallet_scanner.h"

//local headers
#include "misc_language.h"
#include "rpc/core_rpc_server_commands_defs.h"

//standard headers
#include <algorithm>


namespace test
{
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
const std::size_t SENDR_WALLET_IDX  = 0;
const std::size_t RECVR_WALLET_IDX  = 1;
const std::size_t NUM_WALLETS       = 2;

const std::uint64_t FAKE_OUTS_COUNT = 15;
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static std::unique_ptr<tools::wallet2> generate_wallet(const std::string &daemon_addr,
    const boost::optional<epee::net_utils::http::login> &daemon_login,
    const epee::net_utils::ssl_options_t ssl_support)
{
    std::unique_ptr<tools::wallet2> wal(new tools::wallet2(
        /*network*/                              cryptonote::MAINNET,
        /*kdf rounds*/                           1,
        /*unattended keeps spend key decrypted*/ true
    ));

    wal->init(daemon_addr, daemon_login, "", 0UL, true/*trusted_daemon*/, ssl_support);
    wal->allow_mismatched_daemon_version(true);
    wal->set_refresh_from_block_height(1); // setting to 1 skips height estimate in wal->generate()

    // Generate wallet in memory with empty wallet file name
    wal->generate("", "");

    return wal;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
void WalletScannerTest::reset()
{
    printf("Resetting blockchain\n");
    std::uint64_t height = this->daemon()->get_height().height;
    this->daemon()->pop_blocks(height - 1);
    this->daemon()->flush_txpool();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletScannerTest::mine(const std::size_t wallet_idx, const std::uint64_t num_blocks)
{
    const std::string addr = this->wallet(wallet_idx)->get_account().get_public_address_str(cryptonote::MAINNET);
    this->daemon()->generateblocks(addr, num_blocks);
}
//-------------------------------------------------------------------------------------------------------------------
void WalletScannerTest::transfer(const std::size_t wallet_idx,
    const cryptonote::account_public_address &dest_addr,
    const bool is_subaddress,
    const std::uint64_t amount_to_transfer,
    cryptonote::transaction &tx_out)
{
    std::vector<cryptonote::tx_destination_entry> dsts;
    dsts.reserve(1);

    cryptonote::tx_destination_entry de;
    de.addr = dest_addr;
    de.is_subaddress = is_subaddress;
    de.amount = amount_to_transfer;
    dsts.push_back(de);

    std::vector<tools::wallet2::pending_tx> ptx;
    ptx = this->wallet(wallet_idx)->create_transactions_2(dsts, FAKE_OUTS_COUNT, 0, std::vector<uint8_t>(), 0, {});
    CHECK_AND_ASSERT_THROW_MES(ptx.size() == 1, "unexpected num pending txs");
    this->wallet(wallet_idx)->commit_tx(ptx[0]);

    tx_out = std::move(ptx[0].tx);
}
//-------------------------------------------------------------------------------------------------------------------
std::uint64_t WalletScannerTest::mine_tx(const crypto::hash &tx_hash, const std::string &miner_addr_str)
{
    const std::string txs_hash = epee::string_tools::pod_to_hex(tx_hash);

    // Make sure tx is in the pool
    auto res = this->daemon()->get_transactions(std::vector<std::string>{txs_hash});
    CHECK_AND_ASSERT_THROW_MES(res.txs.size() == 1 && res.txs[0].tx_hash == txs_hash && res.txs[0].in_pool,
        "tx not found in pool");

    // Mine the tx
    const std::uint64_t height = this->daemon()->generateblocks(miner_addr_str, 1).height;

    // Make sure tx was mined
    res = this->daemon()->get_transactions(std::vector<std::string>{txs_hash});
    CHECK_AND_ASSERT_THROW_MES(res.txs.size() == 1 && res.txs[0].tx_hash == txs_hash
        && res.txs[0].block_height == height, "tx not yet mined");

    return this->daemon()->get_last_block_header().block_header.reward;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
void WalletScannerTest::check_wallet2_scan(const ExpectedScanResults &res)
{
    auto &sendr_wallet = this->wallet(SENDR_WALLET_IDX);
    auto &recvr_wallet = this->wallet(RECVR_WALLET_IDX);

    sendr_wallet->refresh(true);
    recvr_wallet->refresh(true);
    const std::uint64_t sendr_final_balance = sendr_wallet->balance(0, true);
    const std::uint64_t recvr_final_balance = recvr_wallet->balance(0, true);

    CHECK_AND_ASSERT_THROW_MES(sendr_final_balance == res.sendr_expected_balance,
        "sendr_wallet has unexpected balance");
    CHECK_AND_ASSERT_THROW_MES(recvr_final_balance == res.recvr_expected_balance,
        "recvr_wallet has unexpected balance");

    // Find all transfers with matching tx hash
    tools::wallet2::transfer_container recvr_wallet_incoming_transfers;
    recvr_wallet->get_transfers(recvr_wallet_incoming_transfers);

    std::uint64_t received_amount = 0;
    auto it = recvr_wallet_incoming_transfers.begin();
    const auto end = recvr_wallet_incoming_transfers.end();
    const auto is_same_hash = [l_tx_hash = res.tx_hash](const tools::wallet2::transfer_details& td)
        { return td.m_txid == l_tx_hash; };
    while ((it = std::find_if(it, end, is_same_hash)) != end)
    {
        CHECK_AND_ASSERT_THROW_MES(it->m_block_height > 0, "recvr_wallet did not see tx in chain");
        received_amount += it->m_amount;
        it++;
    }
    CHECK_AND_ASSERT_THROW_MES(received_amount == res.transfer_amount,
        "recvr_wallet did not receive correct amount");
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
ExpectedScanResults WalletScannerTest::init_normal_transfer_test()
{
    auto &sendr_wallet = this->wallet(SENDR_WALLET_IDX);
    auto &recvr_wallet = this->wallet(RECVR_WALLET_IDX);

    // Assert sendr_wallet has enough money to send to recvr_wallet
    std::uint64_t amount_to_transfer = 1000000000000;
    sendr_wallet->refresh(true);
    recvr_wallet->refresh(true);
    CHECK_AND_ASSERT_THROW_MES(sendr_wallet->unlocked_balance(0, true) > (amount_to_transfer*2)/*2x for fee*/,
        "sendr_wallet does not have enough money");

    // Save initial state
    std::uint64_t sendr_init_balance = sendr_wallet->balance(0, true);
    std::uint64_t recvr_init_balance = recvr_wallet->balance(0, true);

    // Send from sendr_wallet to recvr_wallet's primary adddress
    cryptonote::transaction tx;
    cryptonote::account_public_address dest_addr = recvr_wallet->get_account().get_keys().m_account_address;
    this->transfer(SENDR_WALLET_IDX, dest_addr, false/*is_subaddress*/, amount_to_transfer, tx);
    std::uint64_t fee = cryptonote::get_tx_fee(tx);
    crypto::hash tx_hash = cryptonote::get_transaction_hash(tx);

    // Mine the tx
    std::string sender_addr = sendr_wallet->get_account().get_public_address_str(cryptonote::MAINNET);
    std::uint64_t block_reward = this->mine_tx(tx_hash, sender_addr);

    // Calculate expected balances
    std::uint64_t sendr_expected_balance = sendr_init_balance - amount_to_transfer - fee + block_reward;
    std::uint64_t recvr_expected_balance = recvr_init_balance + amount_to_transfer;

    return ExpectedScanResults{
        .sendr_expected_balance = sendr_expected_balance,
        .recvr_expected_balance = recvr_expected_balance,
        .tx_hash                = std::move(tx_hash),
        .transfer_amount        = amount_to_transfer
    };
}
//-------------------------------------------------------------------------------------------------------------------
ExpectedScanResults WalletScannerTest::init_sweep_single_test()
{
    auto &sendr_wallet = this->wallet(SENDR_WALLET_IDX);
    auto &recvr_wallet = this->wallet(RECVR_WALLET_IDX);

    sendr_wallet->refresh(true);
    recvr_wallet->refresh(true);

    // Find a spendable output
    crypto::key_image ki;
    std::uint64_t amount;
    {
        tools::wallet2::transfer_container tc;
        sendr_wallet->get_transfers(tc);
        bool found = false;
        for (const auto &td : tc)
        {
            if (td.m_amount > 0 && !td.m_spent && sendr_wallet->is_transfer_unlocked(td))
            {
                ki = td.m_key_image;
                amount = td.m_amount;
                found = true;
                break;
            }
        }
        CHECK_AND_ASSERT_THROW_MES(found, "did not find spendable output");
    }

    // Save initial state
    std::uint64_t sendr_init_balance = sendr_wallet->balance(0, true);
    std::uint64_t recvr_init_balance = recvr_wallet->balance(0, true);

    // Sweep single output from sendr_wallet to recvr_wallet so no change
    cryptonote::transaction tx;
    {
        std::vector<tools::wallet2::pending_tx> ptx = sendr_wallet->create_transactions_single(ki,
            recvr_wallet->get_account().get_keys().m_account_address,
            false /*is_subaddress*/,
            1 /*outputs*/,
            FAKE_OUTS_COUNT,
            0 /*priority*/,
            std::vector<uint8_t>() /*extra*/
        );
        CHECK_AND_ASSERT_THROW_MES(ptx.size() == 1, "unexpected num pending txs");
        sendr_wallet->commit_tx(ptx[0]);
        tx = std::move(ptx[0].tx);
    }
    std::uint64_t fee = cryptonote::get_tx_fee(tx);
    crypto::hash tx_hash = cryptonote::get_transaction_hash(tx);

    // Mine the tx
    const std::string sender_addr = sendr_wallet->get_account().get_public_address_str(cryptonote::MAINNET);
    std::uint64_t block_reward = this->mine_tx(tx_hash, sender_addr);

    // Calculate expected balances
    std::uint64_t sendr_expected_balance = sendr_init_balance - amount + block_reward;
    std::uint64_t recvr_expected_balance = recvr_init_balance + (amount - fee);

    return ExpectedScanResults{
        .sendr_expected_balance = sendr_expected_balance,
        .recvr_expected_balance = recvr_expected_balance,
        .tx_hash                = std::move(tx_hash),
        .transfer_amount        = (amount - fee)
    };
}
//-------------------------------------------------------------------------------------------------------------------
ExpectedScanResults WalletScannerTest::init_subaddress_transfer_test()
{
    auto &sendr_wallet = this->wallet(SENDR_WALLET_IDX);
    auto &recvr_wallet = this->wallet(RECVR_WALLET_IDX);

    // Assert sendr_wallet has enough money to send to recvr_wallet
    std::uint64_t amount_to_transfer = 1000000000000;
    sendr_wallet->refresh(true);
    recvr_wallet->refresh(true);
    CHECK_AND_ASSERT_THROW_MES(sendr_wallet->unlocked_balance(0, true) > (amount_to_transfer*2)/*2x for fee*/,
        "sendr_wallet does not have enough money");

    // Save initial state
    std::uint64_t sendr_init_balance = sendr_wallet->balance(0, true);
    std::uint64_t recvr_init_balance = recvr_wallet->balance(0, true);

    // Send from sendr_wallet to recvr_wallet subaddress major idx 0, minor idx 1
    cryptonote::transaction tx;
    cryptonote::account_public_address dest_addr = recvr_wallet->get_subaddress({0, 1});
    this->transfer(SENDR_WALLET_IDX, dest_addr, true/*is_subaddress*/, amount_to_transfer, tx);
    std::uint64_t fee = cryptonote::get_tx_fee(tx);
    crypto::hash tx_hash = cryptonote::get_transaction_hash(tx);

    // Mine the tx
    const std::string sender_addr = sendr_wallet->get_account().get_public_address_str(cryptonote::MAINNET);
    std::uint64_t block_reward = this->mine_tx(tx_hash, sender_addr);

    // Calculate expected balances
    std::uint64_t sendr_expected_balance = sendr_init_balance - amount_to_transfer - fee + block_reward;
    std::uint64_t recvr_expected_balance = recvr_init_balance + amount_to_transfer;

    return ExpectedScanResults{
        .sendr_expected_balance = sendr_expected_balance,
        .recvr_expected_balance = recvr_expected_balance,
        .tx_hash                = std::move(tx_hash),
        .transfer_amount        = amount_to_transfer
    };
}
//-------------------------------------------------------------------------------------------------------------------
ExpectedScanResults WalletScannerTest::init_multiple_subaddresses_test()
{
    auto &sendr_wallet = this->wallet(SENDR_WALLET_IDX);
    auto &recvr_wallet = this->wallet(RECVR_WALLET_IDX);

    // Assert sendr_wallet has enough money to send to recvr_wallet
    std::uint64_t amount_to_transfer = 1000000000000;
    sendr_wallet->refresh(true);
    recvr_wallet->refresh(true);
    CHECK_AND_ASSERT_THROW_MES(sendr_wallet->unlocked_balance(0, true) > (amount_to_transfer*2)/*2x for fee*/,
        "sendr_wallet does not have enough money");

    // Save initial state
    std::uint64_t sendr_init_balance = sendr_wallet->balance(0, true);
    std::uint64_t recvr_init_balance = recvr_wallet->balance(0, true);

    // Send from sendr_wallet to 2 recvr_wallet subaddresses
    cryptonote::transaction tx;
    {
        const uint32_t num_subaddress = 2;

        std::vector<cryptonote::tx_destination_entry> dsts;
        dsts.reserve(num_subaddress);
        for (uint32_t i = 1; i <= num_subaddress; ++i)
        {
            cryptonote::tx_destination_entry de;
            de.addr = recvr_wallet->get_subaddress({0, i});
            de.is_subaddress = true;
            de.amount = amount_to_transfer / num_subaddress;
            dsts.push_back(de);
        }

        std::vector<tools::wallet2::pending_tx> ptx;
        ptx = sendr_wallet->create_transactions_2(dsts, FAKE_OUTS_COUNT, 0, std::vector<uint8_t>(), 0, {});
        CHECK_AND_ASSERT_THROW_MES(ptx.size() == 1, "unexpected num pending txs");
        sendr_wallet->commit_tx(ptx[0]);

        tx = std::move(ptx[0].tx);

        // Ensure tx has correct num additional pub keys
        const auto additional_pub_keys = cryptonote::get_additional_tx_pub_keys_from_extra(tx);
        CHECK_AND_ASSERT_THROW_MES(additional_pub_keys.size() == (num_subaddress + 1),
            "unexpected num additional pub keys");
    }
    std::uint64_t fee = cryptonote::get_tx_fee(tx);
    crypto::hash tx_hash = cryptonote::get_transaction_hash(tx);

    // Mine the tx
    const std::string sender_addr = sendr_wallet->get_account().get_public_address_str(cryptonote::MAINNET);
    std::uint64_t block_reward = this->mine_tx(tx_hash, sender_addr);

    // Use wallet2 to scan tx and make sure it's in the chain
    std::uint64_t sendr_expected_balance = sendr_init_balance - amount_to_transfer - fee + block_reward;
    std::uint64_t recvr_expected_balance = recvr_init_balance + amount_to_transfer;

    return ExpectedScanResults{
        .sendr_expected_balance = sendr_expected_balance,
        .recvr_expected_balance = recvr_expected_balance,
        .tx_hash                = std::move(tx_hash),
        .transfer_amount        = amount_to_transfer
    };
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
// Tests
//-------------------------------------------------------------------------------------------------------------------
void WalletScannerTest::check_normal_transfer()
{
    printf("Checking normal transfer\n");
    const ExpectedScanResults res = this->init_normal_transfer_test();
    this->check_wallet2_scan(res);
}
//-------------------------------------------------------------------------------------------------------------------
void WalletScannerTest::check_sweep_single()
{
    printf("Checking sweep single\n");
    const ExpectedScanResults res = this->init_sweep_single_test();
    this->check_wallet2_scan(res);
}
//-------------------------------------------------------------------------------------------------------------------
void WalletScannerTest::check_subaddress_transfer()
{
    printf("Checking transfer to subaddress\n");
    const ExpectedScanResults res = this->init_subaddress_transfer_test();
    this->check_wallet2_scan(res);
}
//-------------------------------------------------------------------------------------------------------------------
void WalletScannerTest::check_multiple_subaddresses_transfer()
{
    printf("Checking transfer to multiple subaddresses\n");
    const ExpectedScanResults res = this->init_multiple_subaddresses_test();
    this->check_wallet2_scan(res);
}
//-------------------------------------------------------------------------------------------------------------------
bool WalletScannerTest::run()
{
    // Reset chain
    this->reset();

    // Mine to sender
    printf("Mining to sender wallet\n");
    this->mine(SENDR_WALLET_IDX, 80);

    // Run the tests
    this->check_normal_transfer();
    this->check_sweep_single();
    this->check_subaddress_transfer();
    this->check_multiple_subaddresses_transfer();

    // TODO: add test that advances chain AFTER scanner starts (use conditional variables)
    // TODO: add reorg tests (both after scanning and while scanning)

    return true;
}
//-------------------------------------------------------------------------------------------------------------------
WalletScannerTest::WalletScannerTest(const std::string &daemon_addr):
    m_daemon_addr(daemon_addr)
{
    const boost::optional<epee::net_utils::http::login> daemon_login = boost::none;
    const epee::net_utils::ssl_options_t ssl_support = epee::net_utils::ssl_support_t::e_ssl_support_disabled;

    m_daemon = std::make_unique<tools::t_daemon_rpc_client>(m_daemon_addr, daemon_login, ssl_support);

    m_wallets.reserve(NUM_WALLETS);
    for (std::size_t i = 0; i < NUM_WALLETS; ++i)
    {
        m_wallets.push_back(generate_wallet(m_daemon_addr, daemon_login, ssl_support));
    }
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace test
