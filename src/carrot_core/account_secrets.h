// Copyright (c) 2024-2026, The Monero Project
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

/// @file Utilities for deriving new key hierarchy account components: privkeys, secrets, and pubkeys

#pragma once

//local headers
#include "crypto/crypto.h"

//third party headers

//standard headers

//forward declarations


namespace carrot
{

/**
 * @brief Derive prove-spend key, for signing input proofs to spend enotes
 *   k_ps = H_n[s_m]()
 * @param s_master s_m
 * @param[out] k_prove_spend_out k_ps
 */
void make_carrot_provespend_key(const crypto::secret_key &s_master,
    crypto::secret_key &k_prove_spend_out);
/**
 * @brief Derive partial spend pubkey, for deriving the generate-image key
 *   K_ps = k_ps T
 * @param k_prove_spend k_ps
 * @param[out] partial_spend_pubkey_out K_ps
 */
void make_carrot_partial_spend_pubkey(const crypto::secret_key &k_prove_spend,
    crypto::public_key &partial_spend_pubkey_out);
/**
 * @brief Derive view-balance secret, for viewing all balance information
 *   s_vb = H_n[s_m]()
 * @param s_master s_m
 * @param[out] s_view_balance_out s_vb
 */
void make_carrot_viewbalance_secret(const crypto::secret_key &s_master,
    crypto::secret_key &s_view_balance_out);
/**
 * @brief Derive generate-image key preimage, for deriving the generate-image key
 *   s_gp = H_32[s_vb]()
 * @param s_view_balance s_vb
 * @param[out] s_generate_image_preimage_out s_gp
 */
void make_carrot_generateimage_preimage(const crypto::secret_key &s_view_balance,
    crypto::secret_key &s_generate_image_preimage_out);
/**
 * @brief Derive generate-image key, for identifying enote spends
 *   k_gi = H_n[s_gp](K_ps)
 * @param s_generate_image_preimage s_gp
 * @param partial_spend_pubkey K_ps
 * @param[out] k_generate_image_out k_gi
 */
void make_carrot_generateimage_key(const crypto::secret_key &s_generate_image_preimage,
    const crypto::public_key &partial_spend_pubkey,
    crypto::secret_key &k_generate_image_out);
/**
 * @brief Derive view-incoming key, for identifying received external enotes
 *   k_v = H_n[s_vb]()
 * @param s_view_balance s_vb
 * @param[out] k_view_out k_v
 */
void make_carrot_viewincoming_key(const crypto::secret_key &s_view_balance,
    crypto::secret_key &k_view_out);
/**
 * @brief Derive generate-address secret, for generating addresses
 *   s_ga = H_32[s_vb]()
 * @param s_view_balance - s_vb
 * @param[out] s_generate_address_out - s_ga
 */
void make_carrot_generateaddress_secret(const crypto::secret_key &s_view_balance,
    crypto::secret_key &s_generate_address_out);
/**
 * @brief Derive base public spendkey for rerandomizable RingCT
 *   K_s = k_gi G + k_ps T
 * @param k_generate_image k_gi
 * @param k_prove_spend k_ps
 * @param[out] spend_pubkey_out K_s
 */
void make_carrot_spend_pubkey(const crypto::secret_key &k_generate_image,
    const crypto::secret_key &k_prove_spend,
    crypto::public_key &spend_pubkey_out);

} //namespace carrot
