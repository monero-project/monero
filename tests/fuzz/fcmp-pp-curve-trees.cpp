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

// Grows an FCMP++ curve tree in fuzz-chosen batches of valid, torsioned and invalid outputs,
// then audits every layer against hashes recomputed from scratch.

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>

#include "include_base_utils.h"
#include "fcmp_pp/curve_trees.h"
#include "fcmp_pp/fcmp_pp_crypto.h"
#include "fcmp_pp/tower_cycle.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "fuzzer.h"

#define FUZZ_CHECK(cond) \
  do { if (!(cond)) { fprintf(stderr, "check failed: %s (line %d)\n", #cond, __LINE__); abort(); } } while (0)

using fcmp_pp::curve_trees::CurveTreesV1;
using fcmp_pp::curve_trees::Helios;
using fcmp_pp::curve_trees::Selene;

static constexpr size_t kMaxSteps = 4;
static constexpr size_t kMaxBatches = 3;
static constexpr size_t kMaxOutputsPerBatch = 6;
static constexpr size_t kMaxChunkWidth = 5;

// Order 2 and order 4 points, used to add torsion to valid points.
static std::vector<crypto::ec_point> torsion_points;

struct Tree
{
  std::vector<fcmp_pp::OutputPair> leaves;
  std::vector<std::vector<Selene::Point>> c1_layers;
  std::vector<std::vector<Helios::Point>> c2_layers;
};

template<typename C>
static bool same_point(const std::unique_ptr<C> &curve, const typename C::Point &a, const typename C::Point &b)
{
  const crypto::ec_point a_bytes = curve->to_bytes(a);
  const crypto::ec_point b_bytes = curve->to_bytes(b);
  return memcmp(&a_bytes, &b_bytes, sizeof(a_bytes)) == 0;
}

//----------------------------------------------------------------------------------------------------------------------
// Output generation
//----------------------------------------------------------------------------------------------------------------------
static crypto::ec_point clean_point(FuzzedDataProvider &fdp)
{
  const std::string seed = fdp.ConsumeBytesAsString(2);
  crypto::secret_key k;
  crypto::hash_to_scalar(seed.data(), seed.size(), k);
  crypto::public_key P;
  FUZZ_CHECK(crypto::secret_key_to_public_key(k, P));
  return P;
}

static crypto::ec_point add_points(const crypto::ec_point &a, const crypto::ec_point &b)
{
  ge_p3 a_p3, b_p3, sum_p3;
  FUZZ_CHECK(ge_frombytes_vartime(&a_p3, to_bytes(a)) == 0);
  FUZZ_CHECK(ge_frombytes_vartime(&b_p3, to_bytes(b)) == 0);
  ge_cached b_cached;
  ge_p3_to_cached(&b_cached, &b_p3);
  ge_p1p1 sum;
  ge_add(&sum, &a_p3, &b_cached);
  ge_p1p1_to_p3(&sum_p3, &sum);
  crypto::ec_point out;
  ge_p3_tobytes(to_bytes(out), &sum_p3);
  return out;
}

static crypto::ec_point consume_point(FuzzedDataProvider &fdp, bool allow_bad)
{
  if (!allow_bad)
    return clean_point(fdp);
  switch (fdp.ConsumeIntegralInRange<int>(0, 7))
  {
    case 0:
      return add_points(clean_point(fdp), fdp.PickValueInArray<crypto::ec_point>({torsion_points[0], torsion_points[1], torsion_points[2]}));
    case 1:
      return crypto::EC_I;
    case 2:
      return torsion_points[fdp.ConsumeIntegralInRange<size_t>(0, torsion_points.size() - 1)];
    case 3:
    {
      crypto::ec_point raw;
      memset(&raw, 0, sizeof(raw));
      const std::string bytes = fdp.ConsumeBytesAsString(sizeof(raw));
      memcpy(&raw, bytes.data(), bytes.size());
      return raw;
    }
    default:
      return clean_point(fdp);
  }
}

static fcmp_pp::OutputPair consume_output_pair(FuzzedDataProvider &fdp)
{
  // Carrot outputs are already torsion-checked by consensus, so only clean points are valid input.
  if (fdp.ConsumeIntegralInRange<int>(0, 3) == 0)
  {
    const crypto::ec_point O = clean_point(fdp);
    const crypto::ec_point C = clean_point(fdp);
    return fcmp_pp::CarrotOutputPairV1{{reinterpret_cast<const crypto::public_key&>(O), C}};
  }
  const crypto::ec_point O = consume_point(fdp, true);
  const crypto::ec_point C = consume_point(fdp, true);
  return fcmp_pp::LegacyOutputPair{{reinterpret_cast<const crypto::public_key&>(O), C}};
}

static bool expect_valid(const fcmp_pp::OutputPair &pair)
{
  crypto::ec_point cleared;
  return crypto::get_valid_torsion_cleared_point_vartime(fcmp_pp::output_pubkey_cref(pair), cleared)
    && crypto::get_valid_torsion_cleared_point_vartime(fcmp_pp::commitment_cref(pair), cleared);
}

//----------------------------------------------------------------------------------------------------------------------
// In-memory tree (mirrors the unit-test global tree, with checks that abort)
//----------------------------------------------------------------------------------------------------------------------
static CurveTreesV1::LastHashes get_last_hashes(const Tree &tree)
{
  CurveTreesV1::LastHashes last_hashes;
  for (const auto &layer : tree.c1_layers)
    last_hashes.c1_last_hashes.push_back(layer.back());
  for (const auto &layer : tree.c2_layers)
    last_hashes.c2_last_hashes.push_back(layer.back());
  return last_hashes;
}

template<typename C>
static void extend_layer(std::vector<typename C::Point> &layer, const fcmp_pp::curve_trees::LayerExtension<C> &ext)
{
  FUZZ_CHECK(!ext.hashes.empty());
  const bool started_after_tip = layer.size() == ext.start_idx;
  const bool started_at_tip = layer.size() == ext.start_idx + 1;
  FUZZ_CHECK(started_after_tip || started_at_tip);
  FUZZ_CHECK(ext.update_existing_last_hash == started_at_tip);
  if (started_at_tip)
    layer.back() = ext.hashes.front();
  for (size_t i = started_at_tip ? 1 : 0; i < ext.hashes.size(); ++i)
    layer.push_back(ext.hashes[i]);
}

static void extend_tree(Tree &tree, const CurveTreesV1::TreeExtension &ext)
{
  FUZZ_CHECK(ext.leaves.start_idx == tree.leaves.size());
  for (const auto &o : ext.leaves.tuples)
    tree.leaves.push_back(o.output_pair);
  if (ext.leaves.tuples.empty())
  {
    FUZZ_CHECK(ext.c1_layer_extensions.empty() && ext.c2_layer_extensions.empty());
    return;
  }

  const auto &c1_exts = ext.c1_layer_extensions;
  const auto &c2_exts = ext.c2_layer_extensions;
  FUZZ_CHECK(c1_exts.size() == c2_exts.size() || c1_exts.size() == c2_exts.size() + 1);
  for (size_t i = 0; i < c1_exts.size(); ++i)
  {
    FUZZ_CHECK(i <= tree.c1_layers.size());
    if (i == tree.c1_layers.size())
      tree.c1_layers.emplace_back();
    extend_layer<Selene>(tree.c1_layers[i], c1_exts[i]);
  }
  for (size_t i = 0; i < c2_exts.size(); ++i)
  {
    FUZZ_CHECK(i <= tree.c2_layers.size());
    if (i == tree.c2_layers.size())
      tree.c2_layers.emplace_back();
    extend_layer<Helios>(tree.c2_layers[i], c2_exts[i]);
  }
}

template<typename C>
static void audit_layer(const std::unique_ptr<C> &curve, const std::vector<typename C::Point> &parents,
    const std::vector<typename C::Scalar> &children, size_t chunk_width)
{
  FUZZ_CHECK(!children.empty());
  FUZZ_CHECK(parents.size() == (children.size() - 1) / chunk_width + 1);
  for (size_t i = 0; i < parents.size(); ++i)
  {
    const size_t start = i * chunk_width;
    const typename C::Chunk chunk{children.data() + start, std::min(chunk_width, children.size() - start)};
    FUZZ_CHECK(same_point(curve, parents[i], fcmp_pp::curve_trees::get_new_parent(curve, chunk)));
  }
}

// Recomputes every layer from the leaves, independently of the incremental extension logic.
static void audit_tree(const CurveTreesV1 &ct, const Tree &tree)
{
  const std::vector<uint64_t> sizes = ct.n_elems_per_layer(tree.leaves.size());
  FUZZ_CHECK(sizes.size() == ct.n_layers(tree.leaves.size()));
  FUZZ_CHECK(tree.c1_layers.size() == (sizes.size() + 1) / 2);
  FUZZ_CHECK(tree.c2_layers.size() == sizes.size() / 2);
  for (size_t i = 0; i < sizes.size(); ++i)
    FUZZ_CHECK((i % 2 == 0 ? tree.c1_layers[i / 2].size() : tree.c2_layers[i / 2].size()) == sizes[i]);
  if (tree.leaves.empty())
    return;
  FUZZ_CHECK(sizes.back() == 1);

  std::vector<CurveTreesV1::LeafTuple> tuples;
  for (const auto &leaf : tree.leaves)
    tuples.push_back(ct.leaf_tuple(leaf));
  audit_layer<Selene>(ct.m_c1, tree.c1_layers[0], ct.flatten_leaves(std::move(tuples)), ct.m_leaf_layer_chunk_width);

  for (size_t layer = 1; layer < sizes.size(); ++layer)
  {
    if (layer % 2 == 1)
    {
      std::vector<Helios::Scalar> children;
      fcmp_pp::tower_cycle::extend_scalars_from_cycle_points<Selene, Helios>(ct.m_c1, tree.c1_layers[layer / 2], children);
      audit_layer<Helios>(ct.m_c2, tree.c2_layers[layer / 2], children, ct.m_c2_width);
    }
    else
    {
      std::vector<Selene::Scalar> children;
      fcmp_pp::tower_cycle::extend_scalars_from_cycle_points<Helios, Selene>(ct.m_c2, tree.c2_layers[layer / 2 - 1], children);
      audit_layer<Selene>(ct.m_c1, tree.c1_layers[layer / 2], children, ct.m_c1_width);
    }
  }
}

// Growing in several steps must give the same tree as inserting every leaf at once.
static void compare_with_one_shot(const CurveTreesV1 &ct, const Tree &tree)
{
  std::vector<fcmp_pp::UnifiedOutput> outputs;
  for (size_t i = 0; i < tree.leaves.size(); ++i)
    outputs.push_back(fcmp_pp::UnifiedOutput{i, tree.leaves[i]});
  Tree one_shot;
  extend_tree(one_shot, ct.get_tree_extension(0, CurveTreesV1::LastHashes{}, {std::move(outputs)}));

  FUZZ_CHECK(one_shot.leaves.size() == tree.leaves.size());
  FUZZ_CHECK(one_shot.c1_layers.size() == tree.c1_layers.size());
  FUZZ_CHECK(one_shot.c2_layers.size() == tree.c2_layers.size());
  for (size_t i = 0; i < tree.c1_layers.size(); ++i)
  {
    FUZZ_CHECK(one_shot.c1_layers[i].size() == tree.c1_layers[i].size());
    for (size_t j = 0; j < tree.c1_layers[i].size(); ++j)
      FUZZ_CHECK(same_point(ct.m_c1, one_shot.c1_layers[i][j], tree.c1_layers[i][j]));
  }
  for (size_t i = 0; i < tree.c2_layers.size(); ++i)
  {
    FUZZ_CHECK(one_shot.c2_layers[i].size() == tree.c2_layers[i].size());
    for (size_t j = 0; j < tree.c2_layers[i].size(); ++j)
      FUZZ_CHECK(same_point(ct.m_c2, one_shot.c2_layers[i][j], tree.c2_layers[i][j]));
  }
}

static void check_layer_counts(FuzzedDataProvider &fdp, const CurveTreesV1 &ct)
{
  const uint64_t n = fdp.ConsumeIntegral<uint64_t>();
  const std::vector<uint64_t> sizes = ct.n_elems_per_layer(n);
  FUZZ_CHECK(sizes.size() == ct.n_layers(n));
  if (n == 0)
  {
    FUZZ_CHECK(sizes.empty());
    return;
  }
  FUZZ_CHECK(sizes.back() == 1);
  uint64_t children = n;
  for (size_t i = 0; i < sizes.size(); ++i)
  {
    const uint64_t width = i % 2 == 0 ? ct.m_c1_width : ct.m_c2_width;
    FUZZ_CHECK(sizes[i] == (children - 1) / width + 1);
    FUZZ_CHECK(i + 1 == sizes.size() || sizes[i] > 1);
    children = sizes[i];
  }
}

BEGIN_INIT_SIMPLE_FUZZER()
  unsigned char order2[32];
  memset(order2, 0xff, sizeof(order2));
  order2[0] = 0xec;
  order2[31] = 0x7f;
  unsigned char order4[32] = {0};
  unsigned char order4_neg[32] = {0};
  order4_neg[31] = 0x80;
  for (const unsigned char *bytes : {order2, order4, order4_neg})
  {
    crypto::ec_point p;
    memcpy(&p, bytes, sizeof(p));
    ge_p3 p3;
    if (ge_frombytes_vartime(&p3, bytes) != 0 || !fcmp_pp::mul8_is_identity_vartime(p3))
    {
      std::cerr << "bad torsion point" << std::endl;
      return 1;
    }
    torsion_points.push_back(p);
  }
END_INIT_SIMPLE_FUZZER()

BEGIN_SIMPLE_FUZZER()
  // END_SIMPLE_FUZZER deletes the log storage; recreate it here before threadpool workers race to do so.
  el::base::Storage::getELPP();

  FuzzedDataProvider fdp(buf, len);
  const size_t c1_width = fdp.ConsumeIntegralInRange<size_t>(2, kMaxChunkWidth);
  const size_t c2_width = fdp.ConsumeIntegralInRange<size_t>(2, kMaxChunkWidth);
  const auto ct = fcmp_pp::curve_trees::curve_trees_v1(c1_width, c2_width);

  check_layer_counts(fdp, *ct);

  Tree tree;
  uint64_t next_id = 0;
  const size_t n_steps = fdp.ConsumeIntegralInRange<size_t>(1, kMaxSteps);
  for (size_t step = 0; step < n_steps && fdp.remaining_bytes() > 0; ++step)
  {
    std::vector<std::vector<fcmp_pp::UnifiedOutput>> batches;
    std::vector<fcmp_pp::UnifiedOutput> expected;
    bool has_duplicate_id = false;
    const size_t n_batches = fdp.ConsumeIntegralInRange<size_t>(1, kMaxBatches);
    for (size_t b = 0; b < n_batches; ++b)
    {
      std::vector<fcmp_pp::UnifiedOutput> batch;
      const size_t n_outputs = fdp.ConsumeIntegralInRange<size_t>(0, kMaxOutputsPerBatch);
      for (size_t i = 0; i < n_outputs; ++i)
      {
        // Ids usually increase, but may repeat or arrive out of order within a batch.
        uint64_t id = next_id++;
        if (fdp.ConsumeIntegralInRange<int>(0, 7) == 0 && next_id > 1)
          id = fdp.ConsumeIntegralInRange<uint64_t>(0, next_id - 1);
        batch.push_back(fcmp_pp::UnifiedOutput{id, consume_output_pair(fdp)});
      }

      std::vector<fcmp_pp::UnifiedOutput> sorted = batch;
      std::sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b) { return a.unified_id < b.unified_id; });
      has_duplicate_id |= std::adjacent_find(sorted.begin(), sorted.end(),
          [](const auto &a, const auto &b) { return a.unified_id == b.unified_id; }) != sorted.end();
      for (const auto &o : sorted)
        if (expect_valid(o.output_pair))
          expected.push_back(o);
      batches.push_back(std::move(batch));
    }

    CurveTreesV1::TreeExtension ext;
    try
    {
      ext = ct->get_tree_extension(tree.leaves.size(), get_last_hashes(tree), std::move(batches));
    }
    catch (const std::exception &)
    {
      FUZZ_CHECK(has_duplicate_id);
      continue;
    }
    FUZZ_CHECK(!has_duplicate_id);

    FUZZ_CHECK(ext.leaves.tuples.size() == expected.size());
    for (size_t i = 0; i < expected.size(); ++i)
      FUZZ_CHECK(ext.leaves.tuples[i] == expected[i]);

    extend_tree(tree, ext);
    audit_tree(*ct, tree);
  }

  if (!tree.leaves.empty() && fdp.ConsumeBool())
    compare_with_one_shot(*ct, tree);
END_SIMPLE_FUZZER()
