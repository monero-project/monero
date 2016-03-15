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


using namespace std;
//unsigned int epee::g_test_dbg_lock_sleep = 0;



struct WalletManagerTest : public testing::Test
{
    Bitmonero::WalletManager * wmgr;

    const char * WALLET_NAME = "testwallet";
    const char * WALLET_NAME_COPY = "testwallet_copy";
    const char * WALLET_NAME_WITH_DIR = "walletdir/testwallet_test";
    const char * WALLET_NAME_WITH_DIR_NON_WRITABLE = "/var/walletdir/testwallet_test";
    const char * WALLET_PASS = "password";
    const char * WALLET_PASS2 = "password22";
    const char * WALLET_LANG = "English";

    WalletManagerTest()
    {
        std::cout << __FUNCTION__ << std::endl;
        wmgr = Bitmonero::WalletManagerFactory::getWalletManager();
        deleteWallet(WALLET_NAME);
        deleteDir(boost::filesystem::path(WALLET_NAME_WITH_DIR).parent_path().string());
    }


    ~WalletManagerTest()
    {
        std::cout << __FUNCTION__ << std::endl;
        //deleteWallet(WALLET_NAME);
    }


    void deleteWallet(const std::string & walletname)
    {
        std::cout << "** deleting wallet: " << walletname << std::endl;
        boost::filesystem::remove(walletname);
        boost::filesystem::remove(walletname + ".address.txt");
        boost::filesystem::remove(walletname + ".keys");
    }

    void deleteDir(const std::string &path)
    {
        std::cout << "** removing dir recursively: " << path  << std::endl;
        boost::filesystem::remove_all(path);
    }

};


TEST_F(WalletManagerTest, WalletManagerCreatesWallet)
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

TEST_F(WalletManagerTest, WalletManagerOpensWallet)
{

    Bitmonero::Wallet * wallet1 = wmgr->createWallet(WALLET_NAME, WALLET_PASS, WALLET_LANG);
    std::string seed1 = wallet1->seed();
    ASSERT_TRUE(wmgr->closeWallet(wallet1));
    Bitmonero::Wallet * wallet2 = wmgr->openWallet(WALLET_NAME, WALLET_PASS);
    ASSERT_TRUE(wallet2->status() == Bitmonero::Wallet::Status_Ok);
    ASSERT_TRUE(wallet2->seed() == seed1);
    std::cout << "** seed: " << wallet2->seed() << std::endl;
}


TEST_F(WalletManagerTest, WalletManagerChangesPassword)
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



TEST_F(WalletManagerTest, WalletManagerRecoversWallet)
{
    Bitmonero::Wallet * wallet1 = wmgr->createWallet(WALLET_NAME, WALLET_PASS, WALLET_LANG);
    std::string seed1 = wallet1->seed();
    std::string address1 = wallet1->address();
    ASSERT_FALSE(address1.empty());
    ASSERT_TRUE(wmgr->closeWallet(wallet1));
    deleteWallet(WALLET_NAME);
    Bitmonero::Wallet * wallet2 = wmgr->recoveryWallet(WALLET_NAME, seed1);
    ASSERT_TRUE(wallet2->status() == Bitmonero::Wallet::Status_Ok);
    ASSERT_TRUE(wallet2->seed() == seed1);
    ASSERT_TRUE(wallet2->address() == address1);
    ASSERT_TRUE(wmgr->closeWallet(wallet2));
}


TEST_F(WalletManagerTest, WalletManagerStoresWallet1)
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

TEST_F(WalletManagerTest, WalletManagerStoresWallet2)
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

TEST_F(WalletManagerTest, WalletManagerStoresWallet3)
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


int main(int argc, char** argv)
{
  //epee::debug::get_set_enable_assert(true, false);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
