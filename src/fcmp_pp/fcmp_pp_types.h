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

#pragma once

#include <cstring>
#include <type_traits>
#include <variant>

#include "crypto/crypto.h"
#include "fcmp_pp_rust/fcmp++.h"


namespace fcmp_pp
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Rust types
//----------------------------------------------------------------------------------------------------------------------
using SeleneScalar = ::SeleneScalar;
static_assert(sizeof(SeleneScalar) == 32, "unexpected size of selene scalar");
using HeliosScalar = ::HeliosScalar;
static_assert(sizeof(HeliosScalar) == 32, "unexpected size of helios scalar");
//----------------------------------------------------------------------------------------------------------------------
struct SeleneT final
{
    using Scalar       = SeleneScalar;
    using Point        = ::SelenePoint;
    using Chunk        = ::SeleneScalarSlice;
    using CycleScalar  = HeliosScalar;
    using ScalarChunks = ::SeleneScalarChunks;
};
//----------------------------------------------------------------------------------------------------------------------
struct HeliosT final
{
    using Scalar       = HeliosScalar;
    using Point        = ::HeliosPoint;
    using Chunk        = ::HeliosScalarSlice;
    using CycleScalar  = SeleneScalar;
    using ScalarChunks = ::HeliosScalarChunks;
};
//----------------------------------------------------------------------------------------------------------------------
using OutputTuple = ::OutputTuple;
//----------------------------------------------------------------------------------------------------------------------
OutputTuple output_tuple_from_bytes(const crypto::ec_point &O, const crypto::ec_point &I, const crypto::ec_point &C);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// C++ types
//----------------------------------------------------------------------------------------------------------------------
//   Curve trees types
//----------------------------------------------------------------------------------------------------------------------
// Output pubkey and commitment, ready to be converted to a leaf tuple
// - From {output_pubkey,commitment} -> {O,C} -> {O,I,C} -> {O.x,O.y,I.x,I.y,C.x,C.y}
// - Output pairs do NOT necessarily have torsion cleared. We need the output pubkey as it exists in the chain in order
//   to derive the correct I (when deriving {O.x,O.y,I.x,I.y,C.x,C.y}). Torsion clearing O before deriving I from O
//   would enable spending a torsioned output once before FCMP++ fork and again with a different key image via FCMP++.
template<typename T>
struct OutputPairTemplate
{
    crypto::public_key output_pubkey;
    // Uses the ec_point type to avoid a circular dep to ringct/rctTypes.h, and to differentiate from output_pubkey
    crypto::ec_point commitment;

    OutputPairTemplate(const crypto::public_key &_output_pubkey, const crypto::ec_point &_commitment):
        output_pubkey(_output_pubkey),
        commitment(_commitment)
    {};

    OutputPairTemplate():
        output_pubkey{},
        commitment{}
    {};

    // WARNING: not constant time
    bool operator==(const OutputPairTemplate &other) const
    {
        return output_pubkey == other.output_pubkey
            && commitment == other.commitment;
    }
};

// May have torsion, use biased key image generator for I
struct LegacyOutputPair : public OutputPairTemplate<LegacyOutputPair>{};
// No torsion, use unbiased key image generator for I
struct CarrotOutputPairV1 : public OutputPairTemplate<CarrotOutputPairV1>{};

static_assert(sizeof(LegacyOutputPair)   == (32+32), "sizeof LegacyOutputPair unexpected");
static_assert(sizeof(CarrotOutputPairV1) == (32+32), "sizeof CarrotOutputPairV1 unexpected");

static_assert(std::has_unique_object_representations_v<LegacyOutputPair>);
static_assert(std::has_unique_object_representations_v<CarrotOutputPairV1>);

using OutputPair = std::variant<LegacyOutputPair, CarrotOutputPairV1>;

const crypto::public_key &output_pubkey_cref(const OutputPair &output_pair);
const crypto::ec_point &commitment_cref(const OutputPair &output_pair);

bool output_checked_for_torsion(const OutputPair &output_pair);
bool use_biased_hash_to_point(const OutputPair &output_pair);

// Wrapper for outputs with context to insert the output into the FCMP++ curve tree
struct UnifiedOutput final
{
    // Output's unique id in the chain, used to insert the output in the tree in the order it entered the chain
    uint64_t unified_id{0};
    OutputPair output_pair;

    bool operator==(const UnifiedOutput &other) const
    {
        return unified_id == other.unified_id && output_pair == other.output_pair;
    }
};
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
}//namespace fcmp_pp

// WARNING: not constant time
inline bool operator==(const fcmp_pp::OutputTuple &a, const fcmp_pp::OutputTuple &b)
{
    static_assert(sizeof(fcmp_pp::OutputTuple) == (sizeof(a.O) + sizeof(a.I) + sizeof(a.C)),
        "unexpected sizeof OutputTuple for == implementation");
    return
        (memcmp(a.O, b.O, sizeof(a.O)) == 0) &&
        (memcmp(a.I, b.I, sizeof(a.I)) == 0) &&
        (memcmp(a.C, b.C, sizeof(a.C)) == 0);
}
