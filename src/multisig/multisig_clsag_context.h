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

////
// References
// - CLSAG (base signature scheme): https://eprint.iacr.org/2019/654
// - MuSig2 (style for multisig signing): https://eprint.iacr.org/2020/1261
///


#pragma once

#include "ringct/rctTypes.h"

#include <vector>


namespace multisig {

namespace signing {

class CLSAG_context_t final {
private:
  // is the CLSAG context initialized?
  bool initialized;
  // challenge components: c = H(domain-separator, {P}, {C}, C_offset, message, L, R)
  rct::keyV c_params;
  // indices in c_params where L and R will be
  std::size_t c_params_L_offset;
  std::size_t c_params_R_offset;
  // musig2-style nonce combination factor components for multisig signing
  //   b = H(domain-separator, {P}, {C}, C_offset, message, {L_combined_alphas}, {R_combined_alphas}, I, D, {s_non_l}, l, k, n)
  //   - {P} = ring of one-time addresses
  //   - {C} = ring of amount commitments (1:1 with one-time addresses)
  //   - C_offset = pseudo-output commitment to offset all amount commitments with
  //   - message = message the CLSAG will sign
  //   - {L_combined_alphas} = set of summed-together public nonces from all multisig signers for this CLSAG's L component
  //   - {R_combined_alphas} = set of summed-together public nonces from all multisig signers for this CLSAG's R component
  //   - I = key image for one-time address at {P}[l]
  //   - D = auxiliary key image for the offsetted amount commitment '{C}[l] - C_offset'
  //   - {s_non_l} = fake responses for this proof
  //   - l = real signing index in {P} and '{C} - C_offset'
  //   - k = number of parallel nonces that each participant provides
  //   - n = number of ring members
  rct::keyV b_params;
  // indices in b_params where L and R 'alpha' components will be
  std::size_t b_params_L_offset;
  std::size_t b_params_R_offset;
  // CLSAG 'concise' coefficients for {P} and '{C} - C_offset'
  //   mu_x = H(domain-separator, {P}, {C}, I, (1/8)*D, C_offset)
  //   - note: 'D' is stored in the form '(1/8)*D' in transaction data
  rct::key mu_P;
  rct::key mu_C;
  // ring size
  std::size_t n;
  // aggregate key image: mu_P*I + mu_C*D
  rct::geDsmp wH_l_precomp;
  // aggregate ring members: mu_P*P_i + mu_C*(C_i - C_offset)
  std::vector<rct::geDsmp> W_precomp;
  // key image component base keys: H_p(P_i)
  std::vector<rct::geDsmp> H_precomp;
  // cache for later: generator 'G' in 'precomp' representation
  rct::geDsmp G_precomp;
  // real signing index in this CLSAG
  std::size_t l;
  // signature responses
  rct::keyV s;
  // number of signing nonces expected per signer
  std::size_t num_alpha_components;
public:
  CLSAG_context_t() : initialized{false} {}

  // prepare CLSAG challenge context
  bool init(
    const rct::keyV& P,
    const rct::keyV& C_nonzero,
    const rct::key& C_offset,
    const rct::key& message,
    const rct::key& I,
    const rct::key& D,
    const unsigned int l,
    const rct::keyV& s,
    const std::size_t num_alpha_components
  );

  // get the local signer's combined musig2-style private nonce and compute the CLSAG challenge
  bool combine_alpha_and_compute_challenge(
    // set of summed-together musig2-style public nonces from all multisig signers for this CLSAG's L component
    const rct::keyV& total_alpha_G,
    // set of summed-together musig2-style public nonces from all multisig signers for this CLSAG's R component
    const rct::keyV& total_alpha_H,
    // local signer's private musig2-style nonces
    const rct::keyV& alpha,
    // local signer's final private nonce, using musig2-style combination with factor 'b'
    //    alpha_combined = sum_i(b^i * alpha[i])
    rct::key& alpha_combined,
    // CLSAG challenge to store in the proof
    rct::key& c_0,
    // final CLSAG challenge to respond to (need this to make multisig partial signatures)
    rct::key& c
  );

  // getter for CLSAG 'concise' coefficients
  bool get_mu(
    rct::key& mu_P,
    rct::key& mu_C
  ) const;
};

} //namespace signing

} //namespace multisig
