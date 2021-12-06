// Copyright (c) 2021, The Monero Project
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

#include "multisig_clsag_context.h"

#include "int-util.h"

#include "crypto/crypto.h"
#include "cryptonote_config.h"
#include "ringct/rctOps.h"
#include "ringct/rctTypes.h"

#include <cstring>
#include <string>
#include <vector>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "multisig"

namespace multisig {

namespace signing {
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
template<std::size_t N>
static rct::key string_to_key(const unsigned char (&str)[N]) {
  rct::key tmp{};
  static_assert(sizeof(tmp.bytes) >= N, "");
  std::memcpy(tmp.bytes, str, N);
  return tmp;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static void encode_int_to_key_le(const unsigned int i, rct::key &k_out)
{
  static_assert(sizeof(unsigned int) <= sizeof(std::uint64_t), "unsigned int max too large");
  static_assert(sizeof(std::uint64_t) <= sizeof(rct::key), "");
  std::uint64_t temp_i{SWAP64LE(i)};
  std::memcpy(k_out.bytes, &temp_i, sizeof(temp_i));
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
bool CLSAG_context_t::init(
  const rct::keyV& P,
  const rct::keyV& C_nonzero,
  const rct::key& C_offset,
  const rct::key& message,
  const rct::key& I,
  const rct::key& D,
  const unsigned int l,
  const rct::keyV& s,
  const std::size_t num_alpha_components
)
{
  initialized = false;

  n = P.size();
  if (n <= 0)
    return false;
  if (C_nonzero.size() != n)
    return false;
  if (s.size() != n)
    return false;
  if (l >= n)
    return false;

  c_params.clear();
  c_params.reserve(n * 2 + 5);
  b_params.clear();
  b_params.reserve(n * 3 + 2 * num_alpha_components + 7);

  c_params.push_back(string_to_key(config::HASH_KEY_CLSAG_ROUND));
  b_params.push_back(string_to_key(config::HASH_KEY_CLSAG_ROUND_MULTISIG));
  c_params.insert(c_params.end(), P.begin(), P.end());
  b_params.insert(b_params.end(), P.begin(), P.end());
  c_params.insert(c_params.end(), C_nonzero.begin(), C_nonzero.end());
  b_params.insert(b_params.end(), C_nonzero.begin(), C_nonzero.end());
  c_params.emplace_back(C_offset);
  b_params.emplace_back(C_offset);
  c_params.emplace_back(message);
  b_params.emplace_back(message);
  c_params_L_offset = c_params.size();
  b_params_L_offset = b_params.size();
  c_params.resize(c_params.size() + 1);  //this is where L will be inserted later
  b_params.resize(b_params.size() + num_alpha_components);  //multisig aggregate public nonces for L will be inserted here later
  c_params_R_offset = c_params.size();
  b_params_R_offset = b_params.size();
  c_params.resize(c_params.size() + 1);  //this is where R will be inserted later
  b_params.resize(b_params.size() + num_alpha_components);  //multisig aggregate public nonces for R will be inserted here later
  b_params.emplace_back(I);
  b_params.emplace_back(D);
  b_params.insert(b_params.end(), s.begin(), s.begin() + l);    //fake responses before 'l'
  b_params.insert(b_params.end(), s.begin() + l + 1, s.end());  //fake responses after 'l'
  b_params.emplace_back();
  encode_int_to_key_le(l, b_params.back());  //real signing index 'l'
  b_params.emplace_back();
  encode_int_to_key_le(num_alpha_components, b_params.back());  //number of parallel nonces
  b_params.emplace_back();
  encode_int_to_key_le(n, b_params.back());  //number of ring members

  rct::keyV mu_P_params;
  rct::keyV mu_C_params;
  mu_P_params.reserve(n * 2 + 4);
  mu_C_params.reserve(n * 2 + 4);

  mu_P_params.push_back(string_to_key(config::HASH_KEY_CLSAG_AGG_0));
  mu_C_params.push_back(string_to_key(config::HASH_KEY_CLSAG_AGG_1));
  mu_P_params.insert(mu_P_params.end(), P.begin(), P.end());
  mu_C_params.insert(mu_C_params.end(), P.begin(), P.end());
  mu_P_params.insert(mu_P_params.end(), C_nonzero.begin(), C_nonzero.end());
  mu_C_params.insert(mu_C_params.end(), C_nonzero.begin(), C_nonzero.end());
  mu_P_params.emplace_back(I);
  mu_C_params.emplace_back(I);
  mu_P_params.emplace_back(scalarmultKey(D, rct::INV_EIGHT));
  mu_C_params.emplace_back(mu_P_params.back());
  mu_P_params.emplace_back(C_offset);
  mu_C_params.emplace_back(C_offset);
  mu_P = hash_to_scalar(mu_P_params);
  mu_C = hash_to_scalar(mu_C_params);

  rct::geDsmp I_precomp;
  rct::geDsmp D_precomp;
  rct::precomp(I_precomp.k, I);
  rct::precomp(D_precomp.k, D);
  rct::key wH_l;
  rct::addKeys3(wH_l, mu_P, I_precomp.k, mu_C, D_precomp.k);
  rct::precomp(wH_l_precomp.k, wH_l);
  W_precomp.resize(n);
  H_precomp.resize(n);
  for (std::size_t i = 0; i < n; ++i) {
    rct::geDsmp P_precomp;
    rct::geDsmp C_precomp;
    rct::key C;
    rct::subKeys(C, C_nonzero[i], C_offset);
    rct::precomp(P_precomp.k, P[i]);
    rct::precomp(C_precomp.k, C);
    rct::key W;
    rct::addKeys3(W, mu_P, P_precomp.k, mu_C, C_precomp.k);
    rct::precomp(W_precomp[i].k, W);
    ge_p3 Hi_p3;
    rct::hash_to_p3(Hi_p3, P[i]);
    ge_dsm_precomp(H_precomp[i].k, &Hi_p3);
  }
  rct::precomp(G_precomp.k, rct::G);
  this->l = l;
  this->s = s;
  this->num_alpha_components = num_alpha_components;

  initialized = true;
  return true;
}
//----------------------------------------------------------------------------------------------------------------------
bool CLSAG_context_t::combine_alpha_and_compute_challenge(
  const rct::keyV& total_alpha_G,
  const rct::keyV& total_alpha_H,
  const rct::keyV& alpha,
  rct::key& alpha_combined,
  rct::key& c_0,
  rct::key& c
)
{
  if (not initialized)
    return false;

  if (num_alpha_components != total_alpha_G.size())
    return false;
  if (num_alpha_components != total_alpha_H.size())
    return false;
  if (num_alpha_components != alpha.size())
    return false;

  // insert aggregate public nonces for L and R components
  for (std::size_t i = 0; i < num_alpha_components; ++i) {
    b_params[b_params_L_offset + i] = total_alpha_G[i];
    b_params[b_params_R_offset + i] = total_alpha_H[i];
  }

  // musig2-style combination factor 'b'
  const rct::key b = rct::hash_to_scalar(b_params);

  // 1) store combined public nonces in the 'L' and 'R' slots for computing the initial challenge
  //    - L = sum_i(b^i total_alpha_G[i])
  //    - R = sum_i(b^i total_alpha_H[i])
  // 2) compute the local signer's combined private nonce
  //    - alpha_combined = sum_i(b^i * alpha[i])
  rct::key& L_l = c_params[c_params_L_offset];
  rct::key& R_l = c_params[c_params_R_offset];
  rct::key b_i = rct::identity();
  L_l = rct::identity();
  R_l = rct::identity();
  alpha_combined = rct::zero();
  for (std::size_t i = 0; i < num_alpha_components; ++i) {
    rct::addKeys(L_l, L_l, rct::scalarmultKey(total_alpha_G[i], b_i));
    rct::addKeys(R_l, R_l, rct::scalarmultKey(total_alpha_H[i], b_i));
    sc_muladd(alpha_combined.bytes, alpha[i].bytes, b_i.bytes, alpha_combined.bytes);
    sc_mul(b_i.bytes, b_i.bytes, b.bytes);
  }

  // compute initial challenge from real spend components
  c = rct::hash_to_scalar(c_params);

  // 1) c_0: find the CLSAG's challenge for index '0', which will be stored in the proof
  // note: in the CLSAG implementation in ringct/rctSigs, c_0 is denoted 'c1' (a notation error)
  // 2) c:   find the final challenge for the multisig signers to respond to
  for (std::size_t i = (l + 1) % n; i != l; i = (i + 1) % n) {
    if (i == 0)
      c_0 = c;
    rct::addKeys3(c_params[c_params_L_offset], s[i], G_precomp.k, c, W_precomp[i].k);
    rct::addKeys3(c_params[c_params_R_offset], s[i], H_precomp[i].k, c, wH_l_precomp.k);
    c = rct::hash_to_scalar(c_params);
  }
  if (l == 0)
    c_0 = c;

  return true;
}
//----------------------------------------------------------------------------------------------------------------------
bool CLSAG_context_t::get_mu(
  rct::key& mu_P,
  rct::key& mu_C
) const
{
  if (not initialized)
    return false;
  mu_P = this->mu_P;
  mu_C = this->mu_C;
  return true;
}
//----------------------------------------------------------------------------------------------------------------------
} //namespace signing

} //namespace multisig
