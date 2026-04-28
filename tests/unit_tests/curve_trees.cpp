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

#include "gtest/gtest.h"

#include "curve_trees.h"

#include "fcmp_pp/fcmp_pp_crypto.h"
#include "misc_log_ex.h"

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Test helpers
//----------------------------------------------------------------------------------------------------------------------
static const Selene::Scalar generate_random_selene_scalar()
{
    crypto::secret_key s;
    crypto::public_key S;

    crypto::generate_keys(S, s, s, false);

    crypto::ec_coord S_x, S_y;
    CHECK_AND_ASSERT_THROW_MES(fcmp_pp::point_to_wei_x_y((crypto::ec_point&)S, S_x, S_y), "failed to convert to wei x");
    return fcmp_pp::tower_cycle::selene_scalar_from_bytes(S_x);
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Test
//----------------------------------------------------------------------------------------------------------------------
TEST(curve_trees, hash_grow)
{
    const auto curve_trees = fcmp_pp::curve_trees::curve_trees_v1();

    // Start by hashing: {selene_scalar_0, selene_scalar_1}
    // Then grow 1:      {selene_scalar_0, selene_scalar_1, selene_scalar_2}
    // Then grow 1:      {selene_scalar_0, selene_scalar_1, selene_scalar_2, selene_scalar_3}
    const auto selene_scalar_0 = generate_random_selene_scalar();
    const auto selene_scalar_1 = generate_random_selene_scalar();

    // Get the initial hash of the 2 selene scalars
    std::vector<Selene::Scalar> all_children{selene_scalar_0, selene_scalar_1};
    const auto init_hash = curve_trees->m_c1->hash_grow(
        /*existing_hash*/            curve_trees->m_c1->hash_init_point(),
        /*offset*/                   0,
        /*existing_child_at_offset*/ curve_trees->m_c1->zero_scalar(),
        /*children*/                 Selene::Chunk{all_children.data(), all_children.size()});

    // Extend with a new child
    const auto selene_scalar_2 = generate_random_selene_scalar();
    std::vector<Selene::Scalar> new_children{selene_scalar_2};
    const auto ext_hash = curve_trees->m_c1->hash_grow(
        init_hash,
        all_children.size(),
        curve_trees->m_c1->zero_scalar(),
        Selene::Chunk{new_children.data(), new_children.size()});
    const auto ext_hash_str = curve_trees->m_c1->to_string(ext_hash);

    // Now compare to calling hash_grow{selene_scalar_0, selene_scalar_1, selene_scalar_2}
    all_children.push_back(selene_scalar_2);
    const auto grow_res = curve_trees->m_c1->hash_grow(
        /*existing_hash*/            curve_trees->m_c1->hash_init_point(),
        /*offset*/                   0,
        /*existing_child_at_offset*/ curve_trees->m_c1->zero_scalar(),
        /*children*/                 Selene::Chunk{all_children.data(), all_children.size()});
    const auto grow_res_str = curve_trees->m_c1->to_string(grow_res);

    ASSERT_EQ(ext_hash_str, grow_res_str);

    // Replace selene_scalar_2 with selene_scalar_3
    const auto selene_scalar_3 = generate_random_selene_scalar();
    new_children.clear();
    new_children = {selene_scalar_3};
    const auto ext_hash2 = curve_trees->m_c1->hash_grow(
        ext_hash,
        all_children.size() - 1,
        selene_scalar_2,
        Selene::Chunk{new_children.data(), new_children.size()});
    const auto ext_hash_str2 = curve_trees->m_c1->to_string(ext_hash2);

    // Now compare to calling hash_grow{selene_scalar_0, selene_scalar_1, selene_scalar_3}
    all_children.clear();
    all_children = {selene_scalar_0, selene_scalar_1, selene_scalar_3};
    const auto grow_res2 = curve_trees->m_c1->hash_grow(
        /*existing_hash*/            curve_trees->m_c1->hash_init_point(),
        /*offset*/                   0,
        /*existing_child_at_offset*/ curve_trees->m_c1->zero_scalar(),
        /*children*/                 Selene::Chunk{all_children.data(), all_children.size()});
    const auto grow_res_str2 = curve_trees->m_c1->to_string(grow_res2);

    ASSERT_EQ(ext_hash_str2, grow_res_str2);
}
//----------------------------------------------------------------------------------------------------------------------
