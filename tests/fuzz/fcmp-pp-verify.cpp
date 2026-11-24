// Copyright (c) 2026, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of
//    conditions and the following disclaimer in the documentation and/or other materials
//    provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific prior
//    written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
// TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Fuzzes the FCMP++ proof verification entry: attacker-controlled proof bytes are
// handed to fcmp_pp::verify() through the same FFI path a tx verifier uses. The
// harness sizes the proof via the public fcmp_pp_proof_len() so mutations mutate
// proof *contents*, not just trip the length gate. Also round-trips the FCMP++
// output-pair serializations (parse -> dump -> parse -> dump must be stable).

#include <cstring>
#include <string>
#include <vector>

#include "include_base_utils.h"
#include "crypto/crypto.h"
#include "serialization/binary_utils.h"
#include "fcmp_pp/fcmp_pp_serialization.h"
#include "fcmp_pp/fcmp_pp_types.h"
#include "fcmp_pp/verify.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "fuzzer.h"

#define FUZZ_CHECK(cond) \
  do { if (!(cond)) { fprintf(stderr, "check failed: %s (line %d)\n", #cond, __LINE__); abort(); } } while (0)

namespace
{
  void copy32(void *dst, const std::string &src)
  {
    memset(dst, 0, 32);
    memcpy(dst, src.data(), src.size() < 32 ? src.size() : 32);
  }
} // anonymous namespace

BEGIN_INIT_SIMPLE_FUZZER()
END_INIT_SIMPLE_FUZZER()

BEGIN_SIMPLE_FUZZER()
  FuzzedDataProvider fdp(buf, len);

  // Configuration (consumed from the back): up to 8 inputs and 12 layers (toward the
// production FCMP++ parameter ranges; MAX_INPUTS=128/MAX_LAYERS=12), up to 4 pseudo-outs,
  // up to 4 key images, and which output-pair type to round-trip.
  const std::size_t n_inputs = 1 + fdp.ConsumeIntegralInRange<std::size_t>(0, 7);
  const uint8_t n_layers = 1 + fdp.ConsumeIntegralInRange<uint8_t>(0, 11);
  // The verifier requires the same input count for the proof, the pseudo
  // outs and the key images; derive all three from the input count so
  // mutated inputs and corpus headers stay consistent (zero-filled configs
  // with mismatched counts stop at the input count check and never reach
  // verify()).
  const std::size_t n_pseudo = n_inputs;
  const std::size_t n_ki = n_inputs;
  const unsigned rt_sel = fdp.ConsumeIntegralInRange<unsigned>(0, 2);

  // The tower alternates curves per layer: even layer counts produce a
  // Helios root and odd counts a Selene root; passing the wrong root type
  // can panic the verifier, so pick the root by parity (fallback: hash
  // init point on decompress failure).
  uint8_t root32[32];
  copy32(root32, fdp.ConsumeBytesAsString(32));
  fcmp_pp::TreeRootShared tree_root;
  if (n_layers % 2 == 0)
  {
    HeliosPoint helios_point;
    if (::helios_point_from_bytes(root32, &helios_point) != 0)
      helios_point = ::helios_hash_init_point();
    tree_root = fcmp_pp::helios_tree_root(helios_point);
  }
  else
  {
    SelenePoint selene_point;
    if (::selene_point_from_bytes(root32, &selene_point) != 0)
      selene_point = ::selene_hash_init_point();
    tree_root = fcmp_pp::selene_tree_root(selene_point);
  }

  // Signable tx hash.
  crypto::hash signable_tx_hash;
  copy32(signable_tx_hash.data, fdp.ConsumeBytesAsString(32));

  // Pseudo-outs and key images.
  std::vector<crypto::ec_point> pseudo_outs;
  pseudo_outs.reserve(n_pseudo);
  for (std::size_t i = 0; i < n_pseudo && fdp.remaining_bytes() > 0; ++i)
    copy32(pseudo_outs.emplace_back().data, fdp.ConsumeBytesAsString(32));
  std::vector<crypto::key_image> key_images;
  key_images.reserve(n_ki);
  for (std::size_t i = 0; i < n_ki && fdp.remaining_bytes() > 0; ++i)
    copy32(key_images.emplace_back().data, fdp.ConsumeBytesAsString(32));

  // Proof: sized to the public proof-length formula so mutations hit the contents.
  std::size_t proof_len = fcmp_pp::fcmp_pp_proof_len(n_inputs, n_layers);
  if (proof_len > 262144)
    return 0;
  fcmp_pp::FcmpPpProof proof = fdp.ConsumeBytes<uint8_t>(
      std::min<std::size_t>(proof_len, fdp.remaining_bytes()));
  proof.resize(proof_len, 0x5a);

  fcmp_pp::FcmpPpVerifyInput input = fcmp_pp::fcmp_pp_verify_input_new(
      signable_tx_hash, proof, n_layers, tree_root, pseudo_outs, key_images);
  if (input == nullptr)
    return 0;
  std::vector<fcmp_pp::FcmpPpVerifyInput> inputs;
  inputs.push_back(std::move(input));
  const bool verified = fcmp_pp::verify(inputs);
  (void)verified;

  // Output-pair serialization round-trips (parse -> dump -> parse -> dump, stable bytes).
  const std::string blob = fdp.ConsumeBytesAsString(std::min<std::size_t>(fdp.remaining_bytes(), 96));
  if (blob.size() >= 2)
  {
    try
    {
      std::string re1, re2;
      if (rt_sel == 0)
      {
        fcmp_pp::LegacyOutputPair obj{};
        if (serialization::parse_binary(blob, obj) && serialization::dump_binary(obj, re1))
        {
          fcmp_pp::LegacyOutputPair obj2{};
          FUZZ_CHECK(serialization::parse_binary(re1, obj2));
          FUZZ_CHECK(serialization::dump_binary(obj2, re2));
          FUZZ_CHECK(re1 == re2);
        }
      }
      else if (rt_sel == 1)
      {
        fcmp_pp::CarrotOutputPairV1 obj{};
        if (serialization::parse_binary(blob, obj) && serialization::dump_binary(obj, re1))
        {
          fcmp_pp::CarrotOutputPairV1 obj2{};
          FUZZ_CHECK(serialization::parse_binary(re1, obj2));
          FUZZ_CHECK(serialization::dump_binary(obj2, re2));
          FUZZ_CHECK(re1 == re2);
        }
      }
      else
      {
        fcmp_pp::UnifiedOutput obj{};
        if (serialization::parse_binary(blob, obj) && serialization::dump_binary(obj, re1))
        {
          fcmp_pp::UnifiedOutput obj2{};
          FUZZ_CHECK(serialization::parse_binary(re1, obj2));
          FUZZ_CHECK(serialization::dump_binary(obj2, re2));
          FUZZ_CHECK(re1 == re2);
        }
      }
    }
    catch (const std::exception &e)
    {
      (void)e; // serialization may legitimately throw on malformed input
    }
  }
END_SIMPLE_FUZZER()
