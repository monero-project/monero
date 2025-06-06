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

#include "prove.h"

#include "common/container_helpers.h"
#include "crypto/generators.h"
#include "fcmp_pp_crypto.h"
#include "misc_log_ex.h"
#include "proof_len.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "fcmp_pp"

#define HANDLE_RES_CODE(res_t, api_func, ...) do {            \
        res_t res;                                            \
        if (api_func(__VA_ARGS__, &res) < 0)                  \
            throw std::runtime_error("failed to " #api_func); \
        return res;                                           \
    } while (0);                                              \

namespace fcmp_pp
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static uint8_t *handle_res_ptr(const std::string func, const ::CResult &res)
{
    if (res.err != nullptr)
    {
        free(res.err);
        throw std::runtime_error("failed to " + func);
    }
    return (uint8_t *) res.value;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
FcmpRerandomizedOutputCompressed rerandomize_output(const OutputBytes output)
{
    HANDLE_RES_CODE(FcmpRerandomizedOutputCompressed, ::rerandomize_output, output);
}
//----------------------------------------------------------------------------------------------------------------------
FcmpRerandomizedOutputCompressed rerandomize_output(const crypto::public_key &onetime_address,
    const crypto::ec_point &amount_commitment)
{
    crypto::ec_point I;
    crypto::derive_key_image_generator(onetime_address, I);

    return rerandomize_output(OutputBytes{
        .O_bytes = to_bytes(onetime_address),
        .I_bytes = to_bytes(I),
        .C_bytes = to_bytes(amount_commitment)
    });
}
//----------------------------------------------------------------------------------------------------------------------
FcmpInputCompressed calculate_fcmp_input_for_rerandomizations(const crypto::public_key &onetime_address,
    const crypto::ec_point &amount_commitment,
    const crypto::secret_key &r_o,
    const crypto::secret_key &r_i,
    const crypto::secret_key &r_r_i,
    const crypto::secret_key &r_c)
{
    FcmpInputCompressed res;

    ge_p2 p2_1;
    ge_p3 p3_1, p3_2;
    ge_p1p1 p1p1_1;
    ge_cached c_1;
    int r;

    // O~ = O + r_o T
    r = ge_frombytes_vartime(&p3_1, to_bytes(onetime_address));
    CHECK_AND_ASSERT_THROW_MES(!r, "calculate fcmp input for rerandomizations: bad O");
    scalarmult_and_add(res.O_tilde, p3_1, to_bytes(r_o), crypto::get_T_p3());

    // I = Hp(O)
    crypto::ec_point I;
    crypto::derive_key_image_generator(onetime_address, I);

    // I~ = I + r_i U
    r = ge_frombytes_vartime(&p3_1, to_bytes(I));
    CHECK_AND_ASSERT_THROW_MES(!r, "calculate fcmp input for rerandomizations: bad I");
    scalarmult_and_add(res.I_tilde, p3_1, to_bytes(r_i), crypto::get_U_p3());

    // R = r_i V + r_r_i T
    p3_1 = crypto::get_V_p3();
    p3_2 = crypto::get_T_p3();
    ge_scalarmult_p3(&p3_1, to_bytes(r_i), &p3_1);
    ge_scalarmult_p3(&p3_2, to_bytes(r_r_i), &p3_2);
    ge_p3_to_cached(&c_1, &p3_1);
    ge_add(&p1p1_1, &p3_2, &c_1);
    ge_p1p1_to_p2(&p2_1, &p1p1_1);
    ge_tobytes(res.R, &p2_1);

    // C~ = C + r_c G
    r = ge_frombytes_vartime(&p3_1, to_bytes(amount_commitment));
    CHECK_AND_ASSERT_THROW_MES(!r, "calculate fcmp input for rerandomizations: bad C");
    scalarmult_and_add(res.C_tilde, p3_1, to_bytes(r_c), crypto::get_G_p3()); // @TODO: could be faster

    return res;
}
//----------------------------------------------------------------------------------------------------------------------
void make_balanced_rerandomized_output_set(
    const std::vector<crypto::public_key> &input_onetime_addresses,
    const std::vector<crypto::ec_point> &input_amount_commitments,
    const std::vector<crypto::secret_key> &input_amount_blinding_factors,
    const std::vector<crypto::secret_key> &r_o,
    const crypto::secret_key &output_amount_blinding_factor_sum,
    std::vector<FcmpRerandomizedOutputCompressed> &rerandomized_outputs_out)
{
    rerandomized_outputs_out.clear();

    const size_t nins = input_onetime_addresses.size();
    CHECK_AND_ASSERT_THROW_MES(nins, "make balanced rerandomized output set: no inputs provided");
    CHECK_AND_ASSERT_THROW_MES(input_amount_commitments.size() == nins,
        "make balanced rerandomized output set: wrong input amount commitments size");
    CHECK_AND_ASSERT_THROW_MES(input_amount_blinding_factors.size() == nins,
        "make balanced rerandomized output set: wrong input amount blinding factors size");
    CHECK_AND_ASSERT_THROW_MES(r_o.size() == nins,
        "make balanced rerandomized output set: wrong r_o size");

    // set blinding_factor_imbalance to sum(output amount blinding factors) - sum(input amount blinding factors)
    crypto::secret_key blinding_factor_imbalance = output_amount_blinding_factor_sum;
    for (const crypto::secret_key &ibf : input_amount_blinding_factors)
    {
        sc_sub(to_bytes(blinding_factor_imbalance),
            to_bytes(blinding_factor_imbalance),
            to_bytes(ibf));
    }

    rerandomized_outputs_out.reserve(nins);
    for (size_t i = 0; i < nins; ++i)
    {
        FcmpRerandomizedOutputCompressed &rerandomized_output = tools::add_element(rerandomized_outputs_out);

        // O
        const crypto::public_key &onetime_address = input_onetime_addresses.at(i);
        // C
        const crypto::ec_point &amount_commitment = input_amount_commitments.at(i);

        // I = Hp(O)
        crypto::ec_point I;
        crypto::derive_key_image_generator(onetime_address, I);

        // sample r_i, r_r_i randomly
        crypto::secret_key r_i, r_r_i;
        crypto::random32_unbiased(to_bytes(r_i));
        crypto::random32_unbiased(to_bytes(r_r_i));

        // sample r_c for all inputs except for the last one, set that one such that the tx balances
        crypto::secret_key r_c;
        if (i == nins - 1)
            r_c = blinding_factor_imbalance;
        else // not last input
            crypto::random32_unbiased(to_bytes(r_c));

        // update blinding_factor_imbalance with new rerandomization
        sc_sub(to_bytes(blinding_factor_imbalance), to_bytes(blinding_factor_imbalance), to_bytes(r_c));

        // calculate FCMP input
        rerandomized_output.input = calculate_fcmp_input_for_rerandomizations(onetime_address,
            amount_commitment, r_o.at(i), r_i, r_r_i, r_c);

        // copy rerandomizations
        memcpy(rerandomized_output.r_o, to_bytes(r_o.at(i)), 32);
        memcpy(rerandomized_output.r_i, to_bytes(r_i), 32);
        memcpy(rerandomized_output.r_r_i, to_bytes(r_r_i), 32);
        memcpy(rerandomized_output.r_c, to_bytes(r_c), 32);
    }
}
//----------------------------------------------------------------------------------------------------------------------
void balance_last_pseudo_out(const uint8_t *sum_input_masks,
    const uint8_t *sum_output_masks,
    std::vector<const uint8_t *> &fcmp_prove_inputs_inout)
{
    auto res = ::balance_last_pseudo_out(
        sum_input_masks,
        sum_output_masks,
        {fcmp_prove_inputs_inout.data(), fcmp_prove_inputs_inout.size()});

    if (res.err != nullptr)
    {
        free(res.err);
        throw std::runtime_error("failed to update last pseudo out");
    }

    fcmp_prove_inputs_inout.pop_back();
    fcmp_prove_inputs_inout.emplace_back((uint8_t *)res.value);
}

crypto::ec_point read_input_pseudo_out(const uint8_t *fcmp_prove_input)
{
    uint8_t * res_ptr = ::read_input_pseudo_out(fcmp_prove_input);
    crypto::ec_point res;
    static_assert(sizeof(crypto::ec_point) == 32, "unexpected size of crypto::ec_point");
    memcpy(&res, res_ptr, sizeof(crypto::ec_point));
    free(res_ptr);
    return res;
}
//----------------------------------------------------------------------------------------------------------------------
SeleneScalar o_blind(const FcmpRerandomizedOutputCompressed &rerandomized_output)
{
    HANDLE_RES_CODE(SeleneScalar, ::o_blind, &rerandomized_output);
}
//----------------------------------------------------------------------------------------------------------------------
SeleneScalar i_blind(const FcmpRerandomizedOutputCompressed &rerandomized_output)
{
    HANDLE_RES_CODE(SeleneScalar, ::i_blind, &rerandomized_output);
}
//----------------------------------------------------------------------------------------------------------------------
SeleneScalar i_blind_blind(const FcmpRerandomizedOutputCompressed &rerandomized_output)
{
    HANDLE_RES_CODE(SeleneScalar, ::i_blind_blind, &rerandomized_output);
}
//----------------------------------------------------------------------------------------------------------------------
SeleneScalar c_blind(const FcmpRerandomizedOutputCompressed &rerandomized_output)
{
    HANDLE_RES_CODE(SeleneScalar, ::c_blind, &rerandomized_output);
}
//----------------------------------------------------------------------------------------------------------------------
uint8_t *fcmp_prove_input_new(const FcmpRerandomizedOutputCompressed &rerandomized_output,
    const fcmp_pp::Path &path,
    const fcmp_pp::OutputBlinds &output_blinds,
    const std::vector<SeleneBranchBlind> &selene_branch_blinds,
    const std::vector<HeliosBranchBlind> &helios_branch_blinds)
{
    MAKE_TEMP_FFI_SLICE(SeleneBranchBlind, selene_branch_blinds, selene_branch_blind_slice);
    MAKE_TEMP_FFI_SLICE(HeliosBranchBlind, helios_branch_blinds, helios_branch_blind_slice);

    auto res = ::fcmp_prove_input_new(&rerandomized_output,
        path.get(),
        output_blinds.get(),
        selene_branch_blind_slice,
        helios_branch_blind_slice);

    return handle_res_ptr(__func__, res);
}
//----------------------------------------------------------------------------------------------------------------------
uint8_t *fcmp_pp_prove_input_new(const uint8_t *x,
    const uint8_t *y,
    const FcmpRerandomizedOutputCompressed &rerandomized_output,
    const fcmp_pp::Path &path,
    const fcmp_pp::OutputBlinds &output_blinds,
    const std::vector<SeleneBranchBlind> &selene_branch_blinds,
    const std::vector<HeliosBranchBlind> &helios_branch_blinds)
{
    MAKE_TEMP_FFI_SLICE(SeleneBranchBlind, selene_branch_blinds, selene_branch_blind_slice);
    MAKE_TEMP_FFI_SLICE(HeliosBranchBlind, helios_branch_blinds, helios_branch_blind_slice);

    auto res = ::fcmp_pp_prove_input_new(x,
        y,
        &rerandomized_output,
        path.get(),
        output_blinds.get(),
        selene_branch_blind_slice,
        helios_branch_blind_slice);

    return handle_res_ptr(__func__, res);
}
//----------------------------------------------------------------------------------------------------------------------
FcmpPpProof prove(const crypto::hash &signable_tx_hash,
    const std::vector<const uint8_t *> &fcmp_prove_inputs,
    const std::size_t n_tree_layers)
{
    auto res = ::prove(reinterpret_cast<const uint8_t*>(&signable_tx_hash),
        {fcmp_prove_inputs.data(), fcmp_prove_inputs.size()},
        n_tree_layers);

    if (res.err != nullptr)
    {
        free(res.err);
        throw std::runtime_error("failed to construct FCMP++ proof");
    }

    // res.value is a void * pointing to a uint8_t *, so cast as a double pointer
    uint8_t **buf = (uint8_t**) res.value;

    const std::size_t proof_size = fcmp_pp_proof_len(fcmp_prove_inputs.size(), n_tree_layers);
    const FcmpPpProof proof{*buf, *buf + proof_size};

    // Free both pointers
    free(*buf);
    free(res.value);

    return proof;
}
//----------------------------------------------------------------------------------------------------------------------
std::pair<FcmpPpSalProof, crypto::key_image> prove_sal(const crypto::hash &signable_tx_hash,
    const crypto::secret_key &x,
    const crypto::secret_key &y,
    const FcmpRerandomizedOutputCompressed &rerandomized_output)
{
    FcmpPpSalProof p;
    p.resize(FCMP_PP_SAL_PROOF_SIZE_V1);

    crypto::key_image L;

    const int r = ::fcmp_pp_prove_sal(to_bytes(signable_tx_hash),
            to_bytes(x),
            to_bytes(y),
            &rerandomized_output,
            &p[0],
            to_bytes(L));

    if (r < 0)
        throw std::runtime_error("fcmp_pp_prove_sal failed with code: " + std::to_string(r));

    return {std::move(p), L};
}
//----------------------------------------------------------------------------------------------------------------------
FcmpMembershipProof prove_membership(const std::vector<uint8_t *> &fcmp_prove_inputs,
    const std::size_t n_tree_layers)
{
    FcmpPpSalProof p;
    const std::size_t proof_len = membership_proof_len(fcmp_prove_inputs.size(), n_tree_layers);
    p.resize(proof_len);

    size_t proof_size = p.size();
    auto res = ::fcmp_pp_prove_membership({fcmp_prove_inputs.data(), fcmp_prove_inputs.size()},
        n_tree_layers,
        proof_len,
        &p[0],
        &proof_size);

    handle_res_ptr(__func__, res);

    p.resize(proof_size);

    // No `free()` since result type `()` is zero-sized

    return p;
}
//----------------------------------------------------------------------------------------------------------------------
uint8_t *fcmp_pp_verify_input_new(const crypto::hash &signable_tx_hash,
    const FcmpPpProof &fcmp_pp_proof,
    const std::size_t n_tree_layers,
    const fcmp_pp::TreeRoot &tree_root,
    const std::vector<crypto::ec_point> &pseudo_outs,
    const std::vector<crypto::key_image> &key_images)
{
    std::vector<const uint8_t *> pseudo_outs_ptrs;
    pseudo_outs_ptrs.reserve(pseudo_outs.size());
    for (const auto &po : pseudo_outs)
        pseudo_outs_ptrs.emplace_back((const uint8_t *)&po);

    std::vector<const uint8_t *> key_images_ptrs;
    key_images_ptrs.reserve(key_images.size());
    for (const auto &ki : key_images)
        key_images_ptrs.emplace_back((const uint8_t *)&ki.data);

    auto res = ::fcmp_pp_verify_input_new(
            reinterpret_cast<const uint8_t*>(&signable_tx_hash),
            fcmp_pp_proof.data(),
            fcmp_pp_proof.size(),
            n_tree_layers,
            tree_root.get(),
            {pseudo_outs_ptrs.data(), pseudo_outs_ptrs.size()},
            {key_images_ptrs.data(), key_images_ptrs.size()}
        );

    return handle_res_ptr(__func__, res);
}
//----------------------------------------------------------------------------------------------------------------------
bool verify(const crypto::hash &signable_tx_hash,
    const FcmpPpProof &fcmp_pp_proof,
    const std::size_t n_tree_layers,
    const fcmp_pp::TreeRoot &tree_root,
    const std::vector<crypto::ec_point> &pseudo_outs,
    const std::vector<crypto::key_image> &key_images)
{
    std::vector<const uint8_t *> pseudo_outs_ptrs;
    pseudo_outs_ptrs.reserve(pseudo_outs.size());
    for (const auto &po : pseudo_outs)
        pseudo_outs_ptrs.emplace_back((const uint8_t *)&po);

    std::vector<const uint8_t *> key_images_ptrs;
    key_images_ptrs.reserve(key_images.size());
    for (const auto &ki : key_images)
        key_images_ptrs.emplace_back((const uint8_t *)&ki.data);

    return ::verify(
            reinterpret_cast<const uint8_t*>(&signable_tx_hash),
            fcmp_pp_proof.data(),
            fcmp_pp_proof.size(),
            n_tree_layers,
            tree_root.get(),
            {pseudo_outs_ptrs.data(), pseudo_outs_ptrs.size()},
            {key_images_ptrs.data(), key_images_ptrs.size()}
        );
}
//----------------------------------------------------------------------------------------------------------------------
bool verify_sal(const crypto::hash &signable_tx_hash,
    const FcmpInputCompressed &input,
    const crypto::key_image &key_image,
    const FcmpPpSalProof &sal_proof)
{
    if (sal_proof.size() != FCMP_PP_SAL_PROOF_SIZE_V1)
        return false;

    return ::fcmp_pp_verify_sal(to_bytes(signable_tx_hash),
        &input,
        to_bytes(key_image),
        sal_proof.data());
}
//----------------------------------------------------------------------------------------------------------------------
bool verify_membership(const FcmpMembershipProof &fcmp_proof,
    const std::size_t n_tree_layers,
    const fcmp_pp::TreeRoot &tree_root,
    const std::vector<FcmpInputCompressed> &inputs)
{
    return ::fcmp_pp_verify_membership(
        {inputs.data(), inputs.size()},
        tree_root.get(),
        n_tree_layers,
        fcmp_proof.data(),
        fcmp_proof.size());
}
//----------------------------------------------------------------------------------------------------------------------
bool batch_verify(const std::vector<const uint8_t *> &fcmp_pp_verify_inputs)
{
    return ::fcmp_pp_batch_verify({fcmp_pp_verify_inputs.data(), fcmp_pp_verify_inputs.size()});
}
//----------------------------------------------------------------------------------------------------------------------
}//namespace fcmp_pp
