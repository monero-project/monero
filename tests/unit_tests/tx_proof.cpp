// Copyright (c) 2018-2022, The Monero Project

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

#include "crypto/crypto.h"
extern "C" {
#include "crypto/crypto-ops.h"
}
#include "crypto/hash.h"
#include <boost/algorithm/string.hpp>

static inline unsigned char *operator &(crypto::ec_point &point) {
    return &reinterpret_cast<unsigned char &>(point);
  }

static inline unsigned char *operator &(crypto::ec_scalar &scalar) {
    return &reinterpret_cast<unsigned char &>(scalar);
  }

TEST(tx_proof, prove_verify_v2)
{
    crypto::secret_key r;
    crypto::random32_unbiased(&r);

    // A = aG
    // B = bG
    crypto::secret_key a,b;
    crypto::public_key A,B;
    crypto::generate_keys(A, a, a, false);
    crypto::generate_keys(B, b, b, false);

    // R_B = rB
    crypto::public_key R_B;
    ge_p3 B_p3;
    ASSERT_EQ(ge_frombytes_vartime(&B_p3,&B), 0);
    ge_p2 R_B_p2;
    ge_scalarmult(&R_B_p2, &unwrap(r), &B_p3);
    ge_tobytes(&R_B, &R_B_p2);

    // R_G = rG
    crypto::public_key R_G;
    ASSERT_EQ(ge_frombytes_vartime(&B_p3,&B), 0);
    ge_p3 R_G_p3;
    ge_scalarmult_base(&R_G_p3, &unwrap(r));
    ge_p3_tobytes(&R_G, &R_G_p3);

    // D = rA
    crypto::public_key D;
    ge_p3 A_p3;
    ASSERT_EQ(ge_frombytes_vartime(&A_p3,&A), 0);
    ge_p2 D_p2;
    ge_scalarmult(&D_p2, &unwrap(r), &A_p3);
    ge_tobytes(&D, &D_p2);

    crypto::signature sig;

    // Message data
    crypto::hash prefix_hash;
    char data[] = "hash input";
    crypto::cn_fast_hash(data,sizeof(data)-1,prefix_hash);

    // Generate/verify valid v1 proof with standard address
    crypto::generate_tx_proof_v1(prefix_hash, R_G, A, boost::none, D, r, sig);
    ASSERT_TRUE(crypto::check_tx_proof(prefix_hash, R_G, A, boost::none, D, sig, 1));

    // Generate/verify valid v1 proof with subaddress
    crypto::generate_tx_proof_v1(prefix_hash, R_B, A, B, D, r, sig);
    ASSERT_TRUE(crypto::check_tx_proof(prefix_hash, R_B, A, B, D, sig, 1));

    // Generate/verify valid v2 proof with standard address
    crypto::generate_tx_proof(prefix_hash, R_G, A, boost::none, D, r, sig);
    ASSERT_TRUE(crypto::check_tx_proof(prefix_hash, R_G, A, boost::none, D, sig, 2));

    // Generate/verify valid v2 proof with subaddress
    crypto::generate_tx_proof(prefix_hash, R_B, A, B, D, r, sig);
    ASSERT_TRUE(crypto::check_tx_proof(prefix_hash, R_B, A, B, D, sig, 2));

    // Try to verify valid v2 proofs as v1 proof (bad)
    crypto::generate_tx_proof(prefix_hash, R_G, A, boost::none, D, r, sig);
    ASSERT_FALSE(crypto::check_tx_proof(prefix_hash, R_G, A, boost::none, D, sig, 1));
    crypto::generate_tx_proof(prefix_hash, R_B, A, B, D, r, sig);
    ASSERT_FALSE(crypto::check_tx_proof(prefix_hash, R_B, A, B, D, sig, 1));

    // Randomly-distributed test points
    crypto::secret_key evil_a, evil_b, evil_d, evil_r;
    crypto::public_key evil_A, evil_B, evil_D, evil_R;
    crypto::generate_keys(evil_A, evil_a, evil_a, false);
    crypto::generate_keys(evil_B, evil_b, evil_b, false);
    crypto::generate_keys(evil_D, evil_d, evil_d, false);
    crypto::generate_keys(evil_R, evil_r, evil_r, false);

    // Selectively choose bad point in v2 proof (bad)
    crypto::generate_tx_proof(prefix_hash, R_B, A, B, D, r, sig);
    ASSERT_FALSE(crypto::check_tx_proof(prefix_hash, evil_R, A, B, D, sig, 2));
    ASSERT_FALSE(crypto::check_tx_proof(prefix_hash, R_B, evil_A, B, D, sig, 2));
    ASSERT_FALSE(crypto::check_tx_proof(prefix_hash, R_B, A, evil_B, D, sig, 2));
    ASSERT_FALSE(crypto::check_tx_proof(prefix_hash, R_B, A, B, evil_D, sig, 2));

    // Try to verify valid v1 proofs as v2 proof (bad)
    crypto::generate_tx_proof_v1(prefix_hash, R_G, A, boost::none, D, r, sig);
    ASSERT_FALSE(crypto::check_tx_proof(prefix_hash, R_G, A, boost::none, D, sig, 2));
    crypto::generate_tx_proof_v1(prefix_hash, R_B, A, B, D, r, sig);
    ASSERT_FALSE(crypto::check_tx_proof(prefix_hash, R_B, A, B, D, sig, 2));
}
