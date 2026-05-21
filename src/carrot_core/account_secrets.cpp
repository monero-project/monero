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
#include "account_secrets.h"

//local headers
#include "config.h"
#include "crypto/crypto.h"
#include "crypto/generators.h"
extern "C"
{
#include "crypto/crypto-ops.h"
}
#include "hash_functions.h"
#include "transcript_fixed.h"

//third party headers

//standard headers

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "carrot"

namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_provespend_key(const crypto::secret_key &s_master,
    crypto::secret_key &k_prove_spend_out)
{
    // k_ps = H_n[s_m]()
    const auto transcript = make_fixed_transcript<CARROT_DOMAIN_SEP_PROVE_SPEND_KEY>();
    derive_scalar(transcript.data(), transcript.size(), &s_master, to_bytes(k_prove_spend_out));
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_partial_spend_pubkey(const crypto::secret_key &k_prove_spend,
    crypto::public_key &partial_spend_pubkey_out)
{
    // K_ps = k_ps T
    ge_p3 T_p3 = crypto::get_T_p3();
    ge_p2 K_ps;
    ge_scalarmult(&K_ps, to_bytes(k_prove_spend), &T_p3);
    ge_tobytes(to_bytes(partial_spend_pubkey_out), &K_ps);
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_viewbalance_secret(const crypto::secret_key &s_master,
    crypto::secret_key &s_view_balance_out)
{
    // s_vb = H_32[s_m]()
    const auto transcript = make_fixed_transcript<CARROT_DOMAIN_SEP_VIEW_BALANCE_SECRET>();
    derive_bytes_32(transcript.data(), transcript.size(), &s_master, to_bytes(s_view_balance_out));
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_generateimage_preimage(const crypto::secret_key &s_view_balance,
    crypto::secret_key &s_generate_image_preimage_out)
{
    // s_gp = H_n[s_vb]()
    const auto transcript = make_fixed_transcript<CARROT_DOMAIN_SEP_GENERATE_IMAGE_PREIMAGE>();
    derive_bytes_32(transcript.data(), transcript.size(), &s_view_balance, to_bytes(s_generate_image_preimage_out));
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_generateimage_key(const crypto::secret_key &s_generate_image_preimage,
    const crypto::public_key &partial_spend_pubkey,
    crypto::secret_key &k_generate_image_out)
{
    // k_gi = H_n[s_gp](K_ps)
    const auto transcript = make_fixed_transcript<CARROT_DOMAIN_SEP_GENERATE_IMAGE_KEY>(partial_spend_pubkey);
    derive_scalar(transcript.data(), transcript.size(), &s_generate_image_preimage, to_bytes(k_generate_image_out));
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_viewincoming_key(const crypto::secret_key &s_view_balance,
    crypto::secret_key &k_view_out)
{
    // k_v = H_n[s_vb]()
    const auto transcript = make_fixed_transcript<CARROT_DOMAIN_SEP_INCOMING_VIEW_KEY>();
    derive_scalar(transcript.data(), transcript.size(), &s_view_balance, to_bytes(k_view_out));
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_generateaddress_secret(const crypto::secret_key &s_view_balance,
    crypto::secret_key &s_generate_address_out)
{
    // s_ga = H_32[s_vb]()
    const auto transcript = make_fixed_transcript<CARROT_DOMAIN_SEP_GENERATE_ADDRESS_SECRET>();
    derive_bytes_32(transcript.data(), transcript.size(), &s_view_balance, to_bytes(s_generate_address_out));
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_spend_pubkey(const crypto::secret_key &k_generate_image,
    const crypto::secret_key &k_prove_spend,
    crypto::public_key &spend_pubkey_out)
{
    // k_ps T
    ge_p3 tmp1 = crypto::get_T_p3();
    ge_cached tmp2;
    ge_scalarmult_p3(&tmp1, to_bytes(k_prove_spend), &tmp1);
    ge_p3_to_cached(&tmp2, &tmp1);

    // k_gi G
    ge_scalarmult_base(&tmp1, to_bytes(k_generate_image));

    // K_s = k_gi G + k_ps T
    ge_p1p1 tmp3;
    ge_p2 tmp4;
    ge_add(&tmp3, &tmp1, &tmp2);
    ge_p1p1_to_p2(&tmp4, &tmp3);
    ge_tobytes(to_bytes(spend_pubkey_out), &tmp4);
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
