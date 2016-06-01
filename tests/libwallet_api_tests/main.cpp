// Copyright (c) 2014-2016, The Monero Project
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

#include "gtest/gtest.h"

#include "wallet/wallet2_api.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <iostream>
#include <vector>
#include <mutex>
#include <thread>


using namespace std;
//unsigned int epee::g_test_dbg_lock_sleep = 0;

namespace Consts
{


// TODO: get rid of hardcoded paths

const char * WALLET_NAME = "testwallet";
const char * WALLET_NAME_COPY = "testwallet_copy";
const char * WALLET_NAME_WITH_DIR = "walletdir/testwallet_test";
const char * WALLET_NAME_WITH_DIR_NON_WRITABLE = "/var/walletdir/testwallet_test";
const char * WALLET_PASS = "password";
const char * WALLET_PASS2 = "password22";
const char * WALLET_LANG = "English";

// change this according your environment

const std::string WALLETS_ROOT_DIR = "/home/mbg033/dev/monero/testnet/";

//const char * TESTNET_WALLET1_NAME = "/home/mbg033/dev/monero/testnet/wallet_01.bin";
//const char * TESTNET_WALLET2_NAME = "/home/mbg033/dev/monero/testnet/wallet_02.bin";
//const char * TESTNET_WALLET3_NAME = "/home/mbg033/dev/monero/testnet/wallet_03.bin";
//const char * TESTNET_WALLET4_NAME = "/home/mbg033/dev/monero/testnet/wallet_04.bin";
//const char * TESTNET_WALLET5_NAME = "/home/mbg033/dev/monero/testnet/wallet_05.bin";

const std::string TESTNET_WALLET1_NAME = WALLETS_ROOT_DIR + "wallet_01.bin";
const std::string TESTNET_WALLET2_NAME = WALLETS_ROOT_DIR + "wallet_02.bin";
const std::string TESTNET_WALLET3_NAME = WALLETS_ROOT_DIR + "wallet_03.bin";
const std::string TESTNET_WALLET4_NAME = WALLETS_ROOT_DIR + "wallet_04.bin";
const std::string TESTNET_WALLET5_NAME = WALLETS_ROOT_DIR + "wallet_05.bin";

const char * TESTNET_WALLET_PASS = "";

const std::string CURRENT_SRC_WALLET = TESTNET_WALLET1_NAME;
const std::string CURRENT_DST_WALLET = TESTNET_WALLET5_NAME;

const char * TESTNET_DAEMON_ADDRESS = "localhost:38081";
const uint64_t AMOUNT_10XMR =  10000000000000L;
const uint64_t AMOUNT_5XMR  =  5000000000000L;
const uint64_t AMOUNT_1XMR  =  1000000000000L;

}



using namespace Consts;

struct Utils
{
    static void deleteWallet(const std::string & walletname)
    {
        std::cout << "** deleting wallet: " << walletname << std::endl;
        boost::filesystem::remove(walletname);
        boost::filesystem::remove(walletname + ".address.txt");
        boost::filesystem::remove(walletname + ".keys");
    }

    static void deleteDir(const std::string &path)
    {
        std::cout << "** removing dir recursively: " << path  << std::endl;
        boost::filesystem::remove_all(path);
    }

    static void print_transaction(Bitmonero::TransactionInfo * t)
    {

        std::cout << "d: "
                  << (t->direction() == Bitmonero::TransactionInfo::Direction_In ? "in" : "out")
                  << ", pe: " << (t->isPending() ? "true" : "false")
                  << ", bh: " << t->blockHeight()
                  << ", a: " << Bitmonero::Wallet::displayAmount(t->amount())
                  << ", f: " << Bitmonero::Wallet::displayAmount(t->fee())
                  << ", h: " << t->hash()
                  << ", pid: " << t->paymentId()
                  << std::endl;
    }

    static std::string get_wallet_address(const std::string &filename, const std::string &password)
    {
        Bitmonero::WalletManager *wmgr = Bitmonero::WalletManagerFactory::getWalletManager();
        Bitmonero::Wallet * w = wmgr->openWallet(filename, password, true);
        std::string result = w->address();
        wmgr->closeWallet(w);
        return result;
    }
};


struct DISABLED_WalletManagerTest : public testing::Test
{
    Bitmonero::WalletManager * wmgr;


    DISABLED_WalletManagerTest()
    {
        std::cout << __FUNCTION__ << std::endl;
        wmgr = Bitmonero::WalletManagerFactory::getWalletManager();
        Utils::deleteWallet(WALLET_NAME);
        Utils::deleteDir(boost::filesystem::path(WALLET_NAME_WITH_DIR).parent_path().string());
    }


    ~DISABLED_WalletManagerTest()
    {
        std::cout << __FUNCTION__ << std::endl;
        //deleteWallet(WALLET_NAME);
    }

};


struct WalletTest1 : public testing::Test
{
    Bitmonero::WalletManager * wmgr;

    WalletTest1()
    {
        wmgr = Bitmonero::WalletManagerFactory::getWalletManager();
    }


};


struct WalletTest2 : public testing::Test
{
    Bitmonero::WalletManager * wmgr;

    WalletTest2()
    {
        wmgr = Bitmonero::WalletManagerFactory::getWalletManager();
    }


};


TEST_F(DISABLED_WalletManagerTest, WalletManagerCreatesWallet)
{

    Bitmonero::Wallet * wallet = wmgr->createWallet(WALLET_NAME, WALLET_PASS, WALLET_LANG);
    ASSERT_TRUE(wallet->status() == Bitmonero::Wallet::Status_Ok);
    ASSERT_TRUE(!wallet->seed().empty());
    std::vector<std::string> words;
    std::string seed = wallet->seed();
    boost::split(words, seed, boost::is_any_of(" "), boost::token_compress_on);
    ASSERT_TRUE(words.size() == 25);
    std::cout << "** seed: " << wallet->seed() << std::endl;
    ASSERT_FALSE(wallet->address().empty());
    std::cout << "** address: " << wallet->address() << std::endl;
    ASSERT_TRUE(wmgr->closeWallet(wallet));

}

TEST_F(DISABLED_WalletManagerTest, WalletManagerOpensWallet)
{

    Bitmonero::Wallet * wallet1 = wmgr->createWallet(WALLET_NAME, WALLET_PASS, WALLET_LANG);
    std::string seed1 = wallet1->seed();
    ASSERT_TRUE(wmgr->closeWallet(wallet1));
    Bitmonero::Wallet * wallet2 = wmgr->openWallet(WALLET_NAME, WALLET_PASS);
    ASSERT_TRUE(wallet2->status() == Bitmonero::Wallet::Status_Ok);
    ASSERT_TRUE(wallet2->seed() == seed1);
    std::cout << "** seed: " << wallet2->seed() << std::endl;
}


TEST_F(DISABLED_WalletManagerTest, WalletManagerChangesPassword)
{
    Bitmonero::Wallet * wallet1 = wmgr->createWallet(WALLET_NAME, WALLET_PASS, WALLET_LANG);
    std::string seed1 = wallet1->seed();
    ASSERT_TRUE(wallet1->setPassword(WALLET_PASS2));
    ASSERT_TRUE(wmgr->closeWallet(wallet1));
    Bitmonero::Wallet * wallet2 = wmgr->openWallet(WALLET_NAME, WALLET_PASS2);
    ASSERT_TRUE(wallet2->status() == Bitmonero::Wallet::Status_Ok);
    ASSERT_TRUE(wallet2->seed() == seed1);
    ASSERT_TRUE(wmgr->closeWallet(wallet2));
    Bitmonero::Wallet * wallet3 = wmgr->openWallet(WALLET_NAME, WALLET_PASS);
    ASSERT_FALSE(wallet3->status() == Bitmonero::Wallet::Status_Ok);
}



TEST_F(DISABLED_WalletManagerTest, WalletManagerRecoversWallet)
{
    Bitmonero::Wallet * wallet1 = wmgr->createWallet(WALLET_NAME, WALLET_PASS, WALLET_LANG);
    std::string seed1 = wallet1->seed();
    std::string address1 = wallet1->address();
    ASSERT_FALSE(address1.empty());
    ASSERT_TRUE(wmgr->closeWallet(wallet1));
    Utils::deleteWallet(WALLET_NAME);
    Bitmonero::Wallet * wallet2 = wmgr->recoveryWallet(WALLET_NAME, seed1);
    ASSERT_TRUE(wallet2->status() == Bitmonero::Wallet::Status_Ok);
    ASSERT_TRUE(wallet2->seed() == seed1);
    ASSERT_TRUE(wallet2->address() == address1);
    ASSERT_TRUE(wmgr->closeWallet(wallet2));
}


TEST_F(DISABLED_WalletManagerTest, WalletManagerStoresWallet1)
{
    Bitmonero::Wallet * wallet1 = wmgr->createWallet(WALLET_NAME, WALLET_PASS, WALLET_LANG);
    std::string seed1 = wallet1->seed();
    std::string address1 = wallet1->address();

    ASSERT_TRUE(wallet1->store(""));
    ASSERT_TRUE(wallet1->store(WALLET_NAME_COPY));
    ASSERT_TRUE(wmgr->closeWallet(wallet1));
    Bitmonero::Wallet * wallet2 = wmgr->openWallet(WALLET_NAME_COPY, WALLET_PASS);
    ASSERT_TRUE(wallet2->status() == Bitmonero::Wallet::Status_Ok);
    ASSERT_TRUE(wallet2->seed() == seed1);
    ASSERT_TRUE(wallet2->address() == address1);
    ASSERT_TRUE(wmgr->closeWallet(wallet2));
}


TEST_F(DISABLED_WalletManagerTest, WalletManagerStoresWallet2)
{
    Bitmonero::Wallet * wallet1 = wmgr->createWallet(WALLET_NAME, WALLET_PASS, WALLET_LANG);
    std::string seed1 = wallet1->seed();
    std::string address1 = wallet1->address();

    ASSERT_TRUE(wallet1->store(WALLET_NAME_WITH_DIR));
    ASSERT_TRUE(wmgr->closeWallet(wallet1));

    wallet1 = wmgr->openWallet(WALLET_NAME_WITH_DIR, WALLET_PASS);
    ASSERT_TRUE(wallet1->status() == Bitmonero::Wallet::Status_Ok);
    ASSERT_TRUE(wallet1->seed() == seed1);
    ASSERT_TRUE(wallet1->address() == address1);
    ASSERT_TRUE(wmgr->closeWallet(wallet1));
}

TEST_F(DISABLED_WalletManagerTest, WalletManagerStoresWallet3)
{
    Bitmonero::Wallet * wallet1 = wmgr->createWallet(WALLET_NAME, WALLET_PASS, WALLET_LANG);
    std::string seed1 = wallet1->seed();
    std::string address1 = wallet1->address();

    ASSERT_FALSE(wallet1->store(WALLET_NAME_WITH_DIR_NON_WRITABLE));
    ASSERT_TRUE(wmgr->closeWallet(wallet1));

    wallet1 = wmgr->openWallet(WALLET_NAME_WITH_DIR_NON_WRITABLE, WALLET_PASS);
    ASSERT_FALSE(wallet1->status() == Bitmonero::Wallet::Status_Ok);

    ASSERT_FALSE(wmgr->closeWallet(wallet1));

    wallet1 = wmgr->openWallet(WALLET_NAME, WALLET_PASS);
    ASSERT_TRUE(wallet1->status() == Bitmonero::Wallet::Status_Ok);
    ASSERT_TRUE(wallet1->seed() == seed1);
    ASSERT_TRUE(wallet1->address() == address1);
    ASSERT_TRUE(wmgr->closeWallet(wallet1));

}

TEST_F(DISABLED_WalletManagerTest, WalletManagerStoresWallet4)
{
    Bitmonero::Wallet * wallet1 = wmgr->createWallet(WALLET_NAME, WALLET_PASS, WALLET_LANG);
    std::string seed1 = wallet1->seed();
    std::string address1 = wallet1->address();

    ASSERT_TRUE(wallet1->store(""));
    ASSERT_TRUE(wallet1->status() == Bitmonero::Wallet::Status_Ok);

    ASSERT_TRUE(wallet1->store(""));
    ASSERT_TRUE(wallet1->status() == Bitmonero::Wallet::Status_Ok);

    ASSERT_TRUE(wmgr->closeWallet(wallet1));

    wallet1 = wmgr->openWallet(WALLET_NAME, WALLET_PASS);
    ASSERT_TRUE(wallet1->status() == Bitmonero::Wallet::Status_Ok);
    ASSERT_TRUE(wallet1->seed() == seed1);
    ASSERT_TRUE(wallet1->address() == address1);
    ASSERT_TRUE(wmgr->closeWallet(wallet1));
}



TEST_F(WalletTest1, WalletShowsBalance)
{
    // TODO: temporary disabled;
    return;
    Bitmonero::Wallet * wallet1 = wmgr->openWallet(CURRENT_SRC_WALLET, TESTNET_WALLET_PASS, true);
    ASSERT_TRUE(wallet1->balance() > 0);
    ASSERT_TRUE(wallet1->unlockedBalance() > 0);

    uint64_t balance1 = wallet1->balance();
    uint64_t unlockedBalance1 = wallet1->unlockedBalance();
    ASSERT_TRUE(wmgr->closeWallet(wallet1));
    Bitmonero::Wallet * wallet2 = wmgr->openWallet(CURRENT_SRC_WALLET, TESTNET_WALLET_PASS, true);

    ASSERT_TRUE(balance1 == wallet2->balance());
    std::cout << "wallet balance: " << wallet2->balance() << std::endl;
    ASSERT_TRUE(unlockedBalance1 == wallet2->unlockedBalance());
    std::cout << "wallet unlocked balance: " << wallet2->unlockedBalance() << std::endl;
    ASSERT_TRUE(wmgr->closeWallet(wallet2));
}

TEST_F(WalletTest1, WalletRefresh)
{
    Bitmonero::Wallet * wallet1 = wmgr->openWallet(CURRENT_SRC_WALLET, TESTNET_WALLET_PASS, true);
    // make sure testnet daemon is running
    ASSERT_TRUE(wallet1->init(TESTNET_DAEMON_ADDRESS, 0));
    ASSERT_TRUE(wallet1->refresh());
    ASSERT_TRUE(wmgr->closeWallet(wallet1));
}

TEST_F(WalletTest1, WalletTransaction)
{
    Bitmonero::Wallet * wallet1 = wmgr->openWallet(CURRENT_SRC_WALLET, TESTNET_WALLET_PASS, true);
    // make sure testnet daemon is running
    ASSERT_TRUE(wallet1->init(TESTNET_DAEMON_ADDRESS, 0));
    ASSERT_TRUE(wallet1->refresh());
    uint64_t balance = wallet1->balance();
    ASSERT_TRUE(wallet1->status() == Bitmonero::PendingTransaction::Status_Ok);

    std::string recepient_address = Utils::get_wallet_address(CURRENT_DST_WALLET, TESTNET_WALLET_PASS);

    Bitmonero::PendingTransaction * transaction = wallet1->createTransaction(
                recepient_address, AMOUNT_10XMR);
    ASSERT_TRUE(transaction->status() == Bitmonero::PendingTransaction::Status_Ok);
    wallet1->refresh();

    ASSERT_TRUE(wallet1->balance() == balance);
    ASSERT_TRUE(transaction->amount() == AMOUNT_10XMR);
    ASSERT_TRUE(transaction->commit());
    ASSERT_FALSE(wallet1->balance() == balance);
    ASSERT_TRUE(wmgr->closeWallet(wallet1));
}

TEST_F(WalletTest1, WalletHistory)
{
    Bitmonero::Wallet * wallet1 = wmgr->openWallet(CURRENT_SRC_WALLET, TESTNET_WALLET_PASS, true);
    // make sure testnet daemon is running
    ASSERT_TRUE(wallet1->init(TESTNET_DAEMON_ADDRESS, 0));
    ASSERT_TRUE(wallet1->refresh());
    Bitmonero::TransactionHistory * history = wallet1->history();
    history->refresh();
    ASSERT_TRUE(history->count() > 0);


    for (auto t: history->getAll()) {
        ASSERT_TRUE(t != nullptr);
        Utils::print_transaction(t);
    }
}

TEST_F(WalletTest1, WalletTransactionAndHistory)
{
    return;
    Bitmonero::Wallet * wallet_src = wmgr->openWallet(CURRENT_SRC_WALLET, TESTNET_WALLET_PASS, true);
    // make sure testnet daemon is running
    ASSERT_TRUE(wallet_src->init(TESTNET_DAEMON_ADDRESS, 0));
    ASSERT_TRUE(wallet_src->refresh());
    Bitmonero::TransactionHistory * history = wallet_src->history();
    history->refresh();
    ASSERT_TRUE(history->count() > 0);
    size_t count1 = history->count();

    std::cout << "**** Transactions before transfer (" << count1 << ")" << std::endl;
    for (auto t: history->getAll()) {
        ASSERT_TRUE(t != nullptr);
        Utils::print_transaction(t);
    }

    std::string wallet4_addr = Utils::get_wallet_address(CURRENT_DST_WALLET, TESTNET_WALLET_PASS);

    Bitmonero::PendingTransaction * tx = wallet_src->createTransaction(wallet4_addr, AMOUNT_10XMR * 5);
    ASSERT_TRUE(tx->status() == Bitmonero::PendingTransaction::Status_Ok);
    ASSERT_TRUE(tx->commit());
    history = wallet_src->history();
    history->refresh();
    ASSERT_TRUE(count1 != history->count());

    std::cout << "**** Transactions after transfer (" << history->count() << ")" << std::endl;
    for (auto t: history->getAll()) {
        ASSERT_TRUE(t != nullptr);
        Utils::print_transaction(t);
    }
}

struct MyWalletListener : public Bitmonero::WalletListener
{

    Bitmonero::Wallet * wallet;
    uint64_t total_tx;
    uint64_t total_rx;
    std::timed_mutex  guard;

    MyWalletListener(Bitmonero::Wallet * wallet)
        : total_tx(0), total_rx(0)
    {
        this->wallet = wallet;
        this->wallet->setListener(this);
    }

    virtual void moneySpent(const string &txId, uint64_t amount)
    {
        std::cout << "wallet: " << wallet->address() << " just spent money ("
                  << txId  << ", " << wallet->displayAmount(amount) << ")" << std::endl;
        total_tx += amount;
        guard.unlock();
    }

    virtual void moneyReceived(const string &txId, uint64_t amount)
    {
        std::cout << "wallet: " << wallet->address() << " just received money ("
                  << txId  << ", " << wallet->displayAmount(amount) << ")" << std::endl;
        total_rx += amount;
        guard.unlock();
    }
};

/*
TEST_F(WalletTest2, WalletCallbackSent)
{

    Bitmonero::Wallet * wallet_src = wmgr->openWallet(TESTNET_WALLET3_NAME, TESTNET_WALLET_PASS, true);
    // make sure testnet daemon is running
    ASSERT_TRUE(wallet_src->init(TESTNET_DAEMON_ADDRESS, 0));
    ASSERT_TRUE(wallet_src->refresh());
    MyWalletListener * wallet_src_listener = new MyWalletListener(wallet_src);
    std::cout << "** Balance: " << wallet_src->displayAmount(wallet_src->balance()) <<  std::endl;


    uint64_t amount = AMOUNT_10XMR * 5;
    std::cout << "** Sending " << Bitmonero::Wallet::displayAmount(amount) << " to " << TESTNET_WALLET4_ADDRESS;
    Bitmonero::PendingTransaction * tx = wallet_src->createTransaction(TESTNET_WALLET4_ADDRESS, AMOUNT_1XMR * 5);
    ASSERT_TRUE(tx->status() == Bitmonero::PendingTransaction::Status_Ok);
    ASSERT_TRUE(tx->commit());

    std::chrono::seconds wait_for = std::chrono::seconds(60*3);

    wallet_src_listener->guard.lock();
    wallet_src_listener->guard.try_lock_for(wait_for);

    ASSERT_TRUE(wallet_src_listener->total_tx != 0);
}
*/

/*
TEST_F(WalletTest2, WalletCallbackReceived)
{

    Bitmonero::Wallet * wallet_src = wmgr->openWallet(TESTNET_WALLET3_NAME, TESTNET_WALLET_PASS, true);
    // make sure testnet daemon is running
    ASSERT_TRUE(wallet_src->init(TESTNET_DAEMON_ADDRESS, 0));
    ASSERT_TRUE(wallet_src->refresh());
    std::cout << "** Balance: " << wallet_src->displayAmount(wallet_src->balance()) <<  std::endl;

    Bitmonero::Wallet * wallet_dst = wmgr->openWallet(TESTNET_WALLET4_NAME, TESTNET_WALLET_PASS, true);
    ASSERT_TRUE(wallet_dst->init(TESTNET_DAEMON_ADDRESS, 0));
    ASSERT_TRUE(wallet_dst->refresh());
    MyWalletListener * wallet_dst_listener = new MyWalletListener(wallet_dst);


    uint64_t amount = AMOUNT_1XMR * 5;
    std::cout << "** Sending " << Bitmonero::Wallet::displayAmount(amount) << " to " << TESTNET_WALLET4_ADDRESS;
    Bitmonero::PendingTransaction * tx = wallet_src->createTransaction(TESTNET_WALLET4_ADDRESS, AMOUNT_1XMR * 5);
    ASSERT_TRUE(tx->status() == Bitmonero::PendingTransaction::Status_Ok);
    ASSERT_TRUE(tx->commit());

    std::chrono::seconds wait_for = std::chrono::seconds(60*4);

    wallet_dst_listener->guard.lock();
    wallet_dst_listener->guard.try_lock_for(wait_for);

    ASSERT_TRUE(wallet_dst_listener->total_tx != 0);

}
*/

int main(int argc, char** argv)
{

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
