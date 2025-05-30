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

//paired header
#include "tx_builder_inputs.h"

//local headers
#include "carrot_core/account_secrets.h"
#include "carrot_core/address_utils.h"
#include "carrot_core/config.h"
#include "carrot_core/enote_utils.h"
#include "carrot_core/scan.h"
#include "crypto/generators.h"
#include "fcmp_pp/prove.h"
#include "misc_log_ex.h"
#include "ringct/rctOps.h"

//third party headers

//standard headers
#include <algorithm>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "carrot_impl"

namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
rct::key load_key(const std::uint8_t bytes[32])
{
    rct::key k;
    memcpy(k.bytes, bytes, sizeof(k));
    return k;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static FcmpInputCompressed calculate_fcmp_input_for_rerandomizations(const crypto::public_key &onetime_address,
    const rct::key &amount_commitment,
    const rct::key &r_o,
    const rct::key &r_i,
    const rct::key &r_r_i,
    const rct::key &r_c)
{
    return fcmp_pp::calculate_fcmp_input_for_rerandomizations(onetime_address,
        rct::rct2pt(amount_commitment),
        rct::rct2sk(r_o),
        rct::rct2sk(r_i),
        rct::rct2sk(r_r_i),
        rct::rct2sk(r_c));
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static void make_sal_proof_nominal_address(const crypto::hash &signable_tx_hash,
    const FcmpRerandomizedOutputCompressed &rerandomized_output,
    const crypto::secret_key &address_privkey_g,
    const crypto::secret_key &address_privkey_t,
    const OutputOpeningHintVariant &opening_hint,
    const epee::span<const crypto::public_key> main_address_spend_pubkeys,
    const view_incoming_key_device *k_view_incoming_dev,
    const view_balance_secret_device *s_view_balance_dev,
    fcmp_pp::FcmpPpSalProof &sal_proof_out,
    crypto::key_image &key_image_out)
{
    // O = x G + y T
    CHECK_AND_ASSERT_THROW_MES(verify_rerandomized_output_basic(rerandomized_output,
            onetime_address_ref(opening_hint),
            amount_commitment_ref(opening_hint)),
        "Could not make SA/L proof: failed to verify rerandomized output against opening hint");

    // scan k^g_o, k^t_o
    crypto::secret_key sender_extension_g;
    crypto::secret_key sender_extension_t;
    CHECK_AND_ASSERT_THROW_MES(try_scan_opening_hint(opening_hint,
            main_address_spend_pubkeys,
            k_view_incoming_dev,
            s_view_balance_dev,
            sender_extension_g,
            sender_extension_t),
        "Could not make SA/L proof: failed to scan opening hint");

    // x = k^{j,g}_addr + k^g_o
    crypto::secret_key x;
    sc_add(to_bytes(x),
        to_bytes(address_privkey_g),
        to_bytes(sender_extension_g));

    // y = k^{j,t}_addr + k^t_o
    crypto::secret_key y;
    sc_add(to_bytes(y),
        to_bytes(address_privkey_t),
        to_bytes(sender_extension_t));

    std::tie(sal_proof_out, key_image_out) = fcmp_pp::prove_sal(signable_tx_hash,
        x,
        y,
        rerandomized_output);
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_rerandomized_outputs_nonrefundable(const std::vector<crypto::public_key> &input_onetime_addresses,
    const std::vector<rct::key> &input_amount_commitments,
    const std::vector<rct::key> &input_amount_blinding_factors,
    const std::vector<rct::key> &output_amount_blinding_factors,
    std::vector<FcmpRerandomizedOutputCompressed> &rerandomized_outputs_out)
{
    // collect input_amount_commitments as crypto::ec_point
    std::vector<crypto::ec_point> input_amount_commitments_pt;
    input_amount_commitments_pt.reserve(input_amount_commitments.size());
    for (const rct::key &input_amount_commitment : input_amount_commitments)
        input_amount_commitments_pt.push_back(rct::rct2pt(input_amount_commitment));

    // collect input_amount_blinding_factors as crypto::secret_key
    std::vector<crypto::secret_key> input_amount_blinding_factors_sk;
    input_amount_blinding_factors_sk.reserve(input_amount_blinding_factors_sk.size());
    for (const rct::key &input_amount_blinding_factor : input_amount_blinding_factors)
        input_amount_blinding_factors_sk.push_back(rct::rct2sk(input_amount_blinding_factor));

    // generate random r_o
    std::vector<crypto::secret_key> r_o(input_onetime_addresses.size());
    for (size_t i = 0; i < input_onetime_addresses.size(); ++i)
        crypto::random32_unbiased(to_bytes(r_o[i]));

    // calculate output_amount_blinding_factor_sum = sum(output_amount_blinding_factors)
    crypto::secret_key output_amount_blinding_factor_sum;
    sc_0(to_bytes(output_amount_blinding_factor_sum));
    for (const rct::key &output_amount_blinding_factor : output_amount_blinding_factors)
        sc_add(to_bytes(output_amount_blinding_factor_sum),
            to_bytes(output_amount_blinding_factor_sum),
            output_amount_blinding_factor.bytes);

    fcmp_pp::make_balanced_rerandomized_output_set(input_onetime_addresses,
        input_amount_commitments_pt,
        input_amount_blinding_factors_sk,
        r_o,
        output_amount_blinding_factor_sum,
        rerandomized_outputs_out);
}
//-------------------------------------------------------------------------------------------------------------------
bool verify_rerandomized_output_basic(const FcmpRerandomizedOutputCompressed &rerandomized_output,
    const crypto::public_key &onetime_address,
    const rct::key &amount_commitment)
{
    const FcmpInputCompressed recomputed_input = calculate_fcmp_input_for_rerandomizations(
        onetime_address,
        amount_commitment,
        load_key(rerandomized_output.r_o),
        load_key(rerandomized_output.r_i),
        load_key(rerandomized_output.r_r_i),
        load_key(rerandomized_output.r_c));

    return 0 == memcmp(&recomputed_input, &rerandomized_output.input, sizeof(FcmpInputCompressed));
}
//-------------------------------------------------------------------------------------------------------------------
void make_sal_proof_any_to_legacy_v1(const crypto::hash &signable_tx_hash,
    const FcmpRerandomizedOutputCompressed &rerandomized_output,
    const OutputOpeningHintVariant &opening_hint,
    const crypto::secret_key &k_spend,
    const cryptonote_hierarchy_address_device &addr_dev,
    fcmp_pp::FcmpPpSalProof &sal_proof_out,
    crypto::key_image &key_image_out)
{
    // get K_s
    const crypto::public_key main_address_spend_pubkey = addr_dev.get_cryptonote_account_spend_pubkey();

    // k^j_subext = ScalarDeriveLegacy("SubAddr" || IntToBytes8(0) || k_v || IntToBytes32(j_major) || IntToBytes32(j_minor))
    const subaddress_index_extended subaddr_index = subaddress_index_ref(opening_hint);
    crypto::secret_key address_privkey_g;
    addr_dev.make_legacy_subaddress_extension(subaddr_index.index.major,
        subaddr_index.index.minor, address_privkey_g);

    // k^j_g = k^j_subext + k_s
    sc_add(to_bytes(address_privkey_g), to_bytes(address_privkey_g), to_bytes(k_spend));

    make_sal_proof_nominal_address(signable_tx_hash,
        rerandomized_output,
        address_privkey_g,
        crypto::null_skey,
        opening_hint,
        {&main_address_spend_pubkey, 1},
        &addr_dev,
        /*s_view_balance_dev=*/nullptr,
        sal_proof_out,
        key_image_out);
}
//-------------------------------------------------------------------------------------------------------------------
void make_sal_proof_any_to_carrot_v1(const crypto::hash &signable_tx_hash,
    const FcmpRerandomizedOutputCompressed &rerandomized_output,
    const OutputOpeningHintVariant &opening_hint,
    const crypto::secret_key &k_prove_spend,
    const crypto::secret_key &k_generate_image,
    const view_balance_secret_device &s_view_balance_dev,
    const view_incoming_key_device &k_view_incoming_dev,
    const generate_address_secret_device &s_generate_address_dev,
    fcmp_pp::FcmpPpSalProof &sal_proof_out,
    crypto::key_image &key_image_out)
{
    // K_s = k_gi G + k_ps T
    crypto::public_key main_address_spend_pubkey;
    carrot::make_carrot_spend_pubkey(k_generate_image, k_prove_spend, main_address_spend_pubkey);

    // s^j_gen = H_32[s_ga](j_major, j_minor)
    const subaddress_index_extended subaddr_index = subaddress_index_ref(opening_hint);
    crypto::secret_key address_index_extension_generator;
    s_generate_address_dev.make_index_extension_generator(subaddr_index.index.major,
        subaddr_index.index.minor,
        address_index_extension_generator);

    // k^j_subscal = H_n(K_s, j_major, j_minor, s^j_gen)
    crypto::secret_key subaddress_scalar;
    carrot::make_carrot_subaddress_scalar(main_address_spend_pubkey,
        address_index_extension_generator,
        subaddr_index.index.major,
        subaddr_index.index.minor,
        subaddress_scalar);

    // k^j_g = k_gi * k^j_subscal
    crypto::secret_key address_privkey_g;
    sc_mul(to_bytes(address_privkey_g), to_bytes(k_generate_image), to_bytes(subaddress_scalar));

    // k^j_t = k_ps * k^j_subscal
    crypto::secret_key address_privkey_t;
    sc_mul(to_bytes(address_privkey_t), to_bytes(k_prove_spend), to_bytes(subaddress_scalar));

    make_sal_proof_nominal_address(signable_tx_hash,
        rerandomized_output,
        address_privkey_g,
        address_privkey_t,
        opening_hint,
        {&main_address_spend_pubkey, 1},
        &k_view_incoming_dev,
        &s_view_balance_dev,
        sal_proof_out,
        key_image_out);
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
