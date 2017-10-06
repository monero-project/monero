// Copyright (c) 2017, The Monero Project
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

#include <boost/range/algorithm/equal.hpp>
#include "crypto/crypto.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "wallet/crypto/import.h"

TEST(WalletCrypto, KeyDerivation) {
    for (unsigned i = 0; i < 50; ++i) {
        const auto one = cryptonote::keypair::generate();
        const auto two = cryptonote::keypair::generate();
        crypto::key_derivation reference{};
        ASSERT_TRUE(crypto::generate_key_derivation(one.pub, two.sec, reference));
    
        crypto::key_derivation first{};
        ASSERT_TRUE(tools::wallet_only::generate_key_derivation(one.pub, two.sec, first));
        ASSERT_TRUE(boost::equal(first.data, reference.data));

        crypto::key_derivation second{};
        ASSERT_TRUE(tools::wallet_only::generate_key_derivation(two.pub, one.sec, second));
        ASSERT_TRUE(boost::equal(first.data, second.data));
    }
}

TEST(WalletCrypto, DerivePublic) {
    for (unsigned i = 0; i < 50; ++i) {
        const auto one = cryptonote::keypair::generate();
        const auto two = cryptonote::keypair::generate();

        crypto::key_derivation derived{};
        static_assert(sizeof(derived) == sizeof(one.pub), "bad memcpy");
        std::memcpy(std::addressof(derived), std::addressof(one.pub), sizeof(derived));

        for (unsigned j = 0; j < 50; ++j) {
            crypto::public_key reference{};
            ASSERT_TRUE(crypto::derive_public_key(derived, j, two.pub, reference));
    
            crypto::public_key first{};
            ASSERT_TRUE(tools::wallet_only::derive_public_key(derived, j, two.pub, first));
            ASSERT_TRUE(boost::equal(first.data, reference.data));
        }
    }
}
/*
TEST(WalletCrypto, Perf) {
    const auto one = cryptonote::keypair::generate();
    const auto two = cryptonote::keypair::generate();
    for (unsigned i = 0; i < 10; ++i) {
        crypto::key_derivation reference{};
        ASSERT_TRUE(crypto::generate_key_derivation(one.pub, two.sec, reference));
    }

    unsigned count = 0;
    auto start = std::chrono::steady_clock::now();
    for (unsigned i = 0; i < 10000; ++i) {
        crypto::key_derivation reference{};
        count += crypto::generate_key_derivation(one.pub, two.sec, reference);
    }
    std::cout << "Standard Stage 1 (ns):\t\t" << (std::chrono::steady_clock::now() - start).count() << " - Iterations: " << count << std::endl;

    for (unsigned i = 0; i < 10; ++i) {
        crypto::key_derivation reference{};
        ASSERT_TRUE(tools::wallet_only::generate_key_derivation(one.pub, two.sec, reference));
    }

    count = 0;
    start = std::chrono::steady_clock::now();
    for (unsigned i = 0; i < 10000; ++i) {
        crypto::key_derivation reference{};
        count += tools::wallet_only::generate_key_derivation(one.pub, two.sec, reference);
    }
    std::cout << "Perf Stage 1 (ns):\t\t" << (std::chrono::steady_clock::now() - start).count() << " - Iterations: " << count << std::endl;
}

TEST(WalletCrypto, Perf2) {
    const auto one = cryptonote::keypair::generate();
    const auto two = cryptonote::keypair::generate();
    crypto::key_derivation derived{};
    static_assert(sizeof(derived) == sizeof(one.pub), "bad memcpy");
    std::memcpy(std::addressof(derived), std::addressof(one.pub), sizeof(derived));

    for (unsigned i = 0; i < 10; ++i) {
        crypto::public_key reference{};
        ASSERT_TRUE(crypto::derive_public_key(derived, i, two.pub, reference));
    }
    unsigned count = 0;
    auto start = std::chrono::steady_clock::now();
    for (unsigned i = 0; i < 10000; ++i) {
        crypto::public_key reference{};
        count += crypto::derive_public_key(derived, i, two.pub, reference);
    }
    std::cout << "Standard Stage 2 (ns):\t\t" << (std::chrono::steady_clock::now() - start).count() << " - Iterations: " << count << std::endl;

    for (unsigned i = 0; i < 10; ++i) {
        crypto::public_key reference{};
        ASSERT_TRUE(tools::wallet_only::derive_public_key(derived, i, two.pub, reference));
    }

    count = 0;
    start = std::chrono::steady_clock::now();
    for (unsigned i = 0; i < 10000; ++i) {
        crypto::public_key reference{};
        count += tools::wallet_only::derive_public_key(derived, i, two.pub, reference);
    }
    std::cout << "Perf Stage 2 (ns):\t\t" << (std::chrono::steady_clock::now() - start).count() << " - Iterations: " << count << std::endl;
}
*/
