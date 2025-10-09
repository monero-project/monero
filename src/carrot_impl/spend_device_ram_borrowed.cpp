// Copyright (c) 2025, The Monero Project
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
#include "spend_device_ram_borrowed.h"

//local headers
#include "address_device_ram_borrowed.h"
#include "carrot_core/device_ram_borrowed.h"
#include "carrot_core/exceptions.h"
#include "key_image_device_composed.h"
#include "misc_log_ex.h"
#include "tx_builder_inputs.h"
#include "tx_builder_outputs.h"

//third party headers

//standard headers

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "carrot_impl"

#define DEFINE_SUB_DEVICES()                                                                           \
    /* K_s = k_s G*/                                                                                   \
    crypto::public_key primary_spend_pubkey;                                                           \
    crypto::secret_key_to_public_key(m_k_spend, primary_spend_pubkey);                                 \
    /* devices */                                                                                      \
    carrot::cryptonote_hierarchy_address_device_ram_borrowed addr_dev(primary_spend_pubkey, m_k_view); \
    carrot::generate_image_key_ram_borrowed_device k_generate_image_dev(m_k_spend);                    \
    carrot::hybrid_hierarchy_address_device_composed hybrid_addr_dev(&addr_dev, nullptr);              \
    carrot::key_image_device_composed key_image_dev(k_generate_image_dev, hybrid_addr_dev, nullptr, &addr_dev);

namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
spend_device_ram_borrowed_legacy::spend_device_ram_borrowed_legacy(
    const crypto::secret_key &k_view,
    const crypto::secret_key &k_spend)
:
    m_k_view(k_view),
    m_k_spend(k_spend)
{}
//-------------------------------------------------------------------------------------------------------------------
bool spend_device_ram_borrowed_legacy::try_sign_carrot_transaction_proposal_v1(
    const CarrotTransactionProposalV1 &tx_proposal,
    const std::unordered_map<crypto::public_key, FcmpRerandomizedOutputCompressed> &rerandomized_outputs,
    crypto::hash &signable_tx_hash_out,
    std::unordered_map<crypto::public_key, std::pair<crypto::key_image, fcmp_pp::FcmpPpSalProof>> &sal_proofs_out) const
{
    signable_tx_hash_out = crypto::null_hash;
    sal_proofs_out.clear();

    DEFINE_SUB_DEVICES()

    // get sorted tx key images and insert into `sal_proofs_out`
    std::vector<crypto::key_image> sorted_input_key_images;
    std::vector<std::size_t> key_image_order;
    carrot::get_sorted_input_key_images_from_proposal_v1(tx_proposal,
        key_image_dev,
        sorted_input_key_images,
        &key_image_order);
    for (std::size_t tx_input_idx = 0; tx_input_idx < sorted_input_key_images.size(); ++tx_input_idx)
    {
        const std::size_t input_proposal_idx = key_image_order.at(tx_input_idx);
        const crypto::public_key ota = onetime_address_ref(tx_proposal.input_proposals.at(input_proposal_idx));
        sal_proofs_out[ota].first = sorted_input_key_images.at(tx_input_idx);
    }

    // calculate signable tx hash
    make_signable_tx_hash_from_proposal_v1(tx_proposal,
        /*s_view_balance_dev=*/nullptr,
        &addr_dev,
        sorted_input_key_images,
        signable_tx_hash_out);

    // prove SA/L
    for (auto &p : sal_proofs_out)
    {
        const crypto::public_key &onetime_address = p.first;

        const auto input_proposal_it = std::find_if(tx_proposal.input_proposals.cbegin(),
            tx_proposal.input_proposals.cend(),
            [&onetime_address](const auto &ip) { return onetime_address_ref(ip) == onetime_address; });
        CARROT_CHECK_AND_THROW(input_proposal_it != tx_proposal.input_proposals.cend(),
            carrot::component_out_of_order,
            "could not find input proposal for given one-time address");

        const auto rerandomized_output_it = rerandomized_outputs.find(onetime_address);
        CARROT_CHECK_AND_THROW(rerandomized_output_it != rerandomized_outputs.cend(),
            carrot::component_out_of_order,
            "could not find rerandomized output for given one-time address");

        crypto::key_image ki;
        carrot::make_sal_proof_any_to_legacy_v1(signable_tx_hash_out,
            rerandomized_output_it->second,
            *input_proposal_it,
            m_k_spend,
            addr_dev,
            p.second.second,
            ki);

        CARROT_CHECK_AND_THROW(ki == p.second.first,
            carrot::component_out_of_order, "key image mismatch during SA/L proving")
    }

    return true;
}
//-------------------------------------------------------------------------------------------------------------------
crypto::key_image spend_device_ram_borrowed_legacy::derive_key_image(const OutputOpeningHintVariant &opening_hint) const
{
    DEFINE_SUB_DEVICES()

    return key_image_dev.derive_key_image(opening_hint);
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
