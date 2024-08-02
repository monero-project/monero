// Copyright (c) 2022, The Monero Project
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

//paired header
#include "sp_crypto_utils.h"

//local headers
#include "crypto/crypto.h"
extern "C"
{
#include "crypto/crypto-ops.h"
}
#include "misc_log_ex.h"
#include "ringct/rctOps.h"
#include "ringct/rctTypes.h"

//third party headers
#include "boost/multiprecision/cpp_int.hpp"

//standard headers
#include <vector>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "seraphis"

namespace sp
{

/// File-scope data

// Useful scalar and group constants
static const rct::key ZERO = rct::zero();
static const rct::key ONE = rct::identity();
/// scalar: -1 mod q
static const rct::key MINUS_ONE = { {0xec, 0xd3, 0xf5, 0x5c, 0x1a, 0x63, 0x12, 0x58, 0xd6, 0x9c, 0xf7, 0xa2, 0xde, 0xf9,
    0xde, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10} };

//-------------------------------------------------------------------------------------------------------------------
// Helper function for scalar inversion
// return: x*(y^2^n)
//-------------------------------------------------------------------------------------------------------------------
static rct::key sm(rct::key y, int n, const rct::key &x)
{
    while (n--)
        sc_mul(y.bytes, y.bytes, y.bytes);
    sc_mul(y.bytes, y.bytes, x.bytes);
    return y;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
rct::key minus_one()
{
    return MINUS_ONE;
}
//-------------------------------------------------------------------------------------------------------------------
rct::key invert(const rct::key &x)
{
    CHECK_AND_ASSERT_THROW_MES(!(x == ZERO), "Cannot invert zero!");

    rct::key _1, _10, _100, _11, _101, _111, _1001, _1011, _1111;

    _1 = x;
    sc_mul(_10.bytes, _1.bytes, _1.bytes);
    sc_mul(_100.bytes, _10.bytes, _10.bytes);
    sc_mul(_11.bytes, _10.bytes, _1.bytes);
    sc_mul(_101.bytes, _10.bytes, _11.bytes);
    sc_mul(_111.bytes, _10.bytes, _101.bytes);
    sc_mul(_1001.bytes, _10.bytes, _111.bytes);
    sc_mul(_1011.bytes, _10.bytes, _1001.bytes);
    sc_mul(_1111.bytes, _100.bytes, _1011.bytes);

    rct::key inv;
    sc_mul(inv.bytes, _1111.bytes, _1.bytes);

    inv = sm(inv, 123 + 3, _101);
    inv = sm(inv, 2 + 2, _11);
    inv = sm(inv, 1 + 4, _1111);
    inv = sm(inv, 1 + 4, _1111);
    inv = sm(inv, 4, _1001);
    inv = sm(inv, 2, _11);
    inv = sm(inv, 1 + 4, _1111);
    inv = sm(inv, 1 + 3, _101);
    inv = sm(inv, 3 + 3, _101);
    inv = sm(inv, 3, _111);
    inv = sm(inv, 1 + 4, _1111);
    inv = sm(inv, 2 + 3, _111);
    inv = sm(inv, 2 + 2, _11);
    inv = sm(inv, 1 + 4, _1011);
    inv = sm(inv, 2 + 4, _1011);
    inv = sm(inv, 6 + 4, _1001);
    inv = sm(inv, 2 + 2, _11);
    inv = sm(inv, 3 + 2, _11);
    inv = sm(inv, 3 + 2, _11);
    inv = sm(inv, 1 + 4, _1001);
    inv = sm(inv, 1 + 3, _111);
    inv = sm(inv, 2 + 4, _1111);
    inv = sm(inv, 1 + 4, _1011);
    inv = sm(inv, 3, _101);
    inv = sm(inv, 2 + 4, _1111);
    inv = sm(inv, 3, _101);
    inv = sm(inv, 1 + 2, _11);

    // Confirm inversion
    rct::key temp;
    sc_mul(temp.bytes, x.bytes, inv.bytes);
    CHECK_AND_ASSERT_THROW_MES(temp == ONE, "Scalar inversion failed!");

    return inv;
}
//-------------------------------------------------------------------------------------------------------------------
void decompose(const std::size_t val, const std::size_t base, const std::size_t size, std::vector<std::size_t> &r_out)
{
    CHECK_AND_ASSERT_THROW_MES(base > 1, "Bad decomposition parameters!");
    CHECK_AND_ASSERT_THROW_MES(r_out.size() == size, "Bad decomposition result vector size!");

    std::size_t temp = val;

    for (std::size_t i = 0; i < size; ++i)
    {
        r_out[i] = temp % base;
        temp /= base;
    }
}
//-------------------------------------------------------------------------------------------------------------------
rct::key kronecker_delta(const std::size_t x, const std::size_t y)
{
    if (x == y)
        return ONE;
    else
        return ZERO;
}
//-------------------------------------------------------------------------------------------------------------------
rct::keyV convolve(const rct::keyV &x, const rct::keyV &y, const std::size_t m)
{
    CHECK_AND_ASSERT_THROW_MES(x.size() >= m, "Bad convolution parameters!");
    CHECK_AND_ASSERT_THROW_MES(y.size() == 2, "Bad convolution parameters!");

    rct::key temp;
    rct::keyV result;
    result.resize(m + 1, ZERO);

    for (std::size_t i = 0; i < m; ++i)
    {
        for (std::size_t j = 0; j < 2; ++j)
        {
            sc_mul(temp.bytes, x[i].bytes, y[j].bytes);
            sc_add(result[i + j].bytes, result[i + j].bytes, temp.bytes);
        }
    }

    return result;
}
//-------------------------------------------------------------------------------------------------------------------
rct::keyV powers_of_scalar(const rct::key &scalar, const std::size_t num_pows, const bool negate_all)
{
    if (num_pows == 0)
        return rct::keyV{};

    rct::keyV pows;
    pows.resize(num_pows);

    if (negate_all)
        pows[0] = MINUS_ONE;
    else
        pows[0] = ONE;

    for (std::size_t i = 1; i < num_pows; ++i)
    {
        sc_mul(pows[i].bytes, pows[i - 1].bytes, scalar.bytes);
    }

    return pows;
}
//-------------------------------------------------------------------------------------------------------------------
void generate_proof_nonce(const rct::key &base, crypto::secret_key &nonce_out, rct::key &nonce_pub_out)
{
    // make proof nonce as crypto::secret_key
    CHECK_AND_ASSERT_THROW_MES(!(base == rct::identity()), "Bad base for generating proof nonce!");

    nonce_out = rct::rct2sk(ZERO);

    while (nonce_out == rct::rct2sk(ZERO) || nonce_pub_out == rct::identity())
    {
        nonce_out = rct::rct2sk(rct::skGen());
        rct::scalarmultKey(nonce_pub_out, base, rct::sk2rct(nonce_out));
    }
}
//-------------------------------------------------------------------------------------------------------------------
void generate_proof_nonce(const rct::key &base, rct::key &nonce_out, rct::key &nonce_pub_out)
{
    // make proof nonce as rct::key
    crypto::secret_key temp;
    generate_proof_nonce(base, temp, nonce_pub_out);
    nonce_out = rct::sk2rct(temp);
}
//-------------------------------------------------------------------------------------------------------------------
void subtract_secret_key_vectors(const std::vector<crypto::secret_key> &keys_A,
    const std::vector<crypto::secret_key> &keys_B,
    crypto::secret_key &result_out)
{
    result_out = rct::rct2sk(rct::zero());

    // add keys_A
    for (const crypto::secret_key &key_A : keys_A)
        sc_add(to_bytes(result_out), to_bytes(result_out), to_bytes(key_A));

    // subtract keys_B
    for (const crypto::secret_key &key_B : keys_B)
        sc_sub(to_bytes(result_out), to_bytes(result_out), to_bytes(key_B));
}
//-------------------------------------------------------------------------------------------------------------------
void mask_key(const crypto::secret_key &mask, const rct::key &key, rct::key &masked_key_out)
{
    // K" = mask G + K
    rct::addKeys1(masked_key_out, rct::sk2rct(mask), key);
}
//-------------------------------------------------------------------------------------------------------------------
crypto::secret_key add_secrets(const crypto::secret_key &a, const crypto::secret_key &b)
{
    crypto::secret_key temp;
    sc_add(to_bytes(temp), to_bytes(a), to_bytes(b));
    return temp;
}
//-------------------------------------------------------------------------------------------------------------------
bool key_domain_is_prime_subgroup(const rct::key &check_key)
{
    // l*K ?= identity
    ge_p3 check_key_p3;
    CHECK_AND_ASSERT_THROW_MES(ge_frombytes_vartime(&check_key_p3, check_key.bytes) == 0, "ge_frombytes_vartime failed");
    ge_scalarmult_p3(&check_key_p3, rct::curveOrder().bytes, &check_key_p3);

    return (ge_p3_is_point_at_infinity_vartime(&check_key_p3) != 0);
}
//-------------------------------------------------------------------------------------------------------------------
bool balance_check_equality(const rct::keyV &commitment_set1, const rct::keyV &commitment_set2)
{
    // balance check method chosen from perf test: tests/performance_tests/balance_check.h
    return rct::equalKeys(rct::addKeys(commitment_set1), rct::addKeys(commitment_set2));
}
//-------------------------------------------------------------------------------------------------------------------
bool balance_check_in_out_amnts(const std::vector<rct::xmr_amount> &input_amounts,
    const std::vector<rct::xmr_amount> &output_amounts,
    const rct::xmr_amount transaction_fee)
{
    boost::multiprecision::uint128_t input_sum{0};
    boost::multiprecision::uint128_t output_sum{0};

    for (const auto amnt : input_amounts)
        input_sum += amnt;

    for (const auto amnt : output_amounts)
        output_sum += amnt;
    output_sum += transaction_fee;

    return input_sum == output_sum;
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace sp
