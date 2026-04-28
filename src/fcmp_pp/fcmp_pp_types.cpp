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

#include "fcmp_pp_types.h"

#include "misc_log_ex.h"

namespace fcmp_pp
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Helpers
//----------------------------------------------------------------------------------------------------------------------
OutputTuple output_tuple_from_bytes(const crypto::ec_point &O, const crypto::ec_point &I, const crypto::ec_point &C)
{
    OutputTuple output_tuple;

    static_assert(sizeof(output_tuple.O) == sizeof(O), "unexpected sizeof O");
    static_assert(sizeof(output_tuple.I) == sizeof(I), "unexpected sizeof I");
    static_assert(sizeof(output_tuple.C) == sizeof(C), "unexpected sizeof C");

    memcpy(output_tuple.O, &O, sizeof(O));
    memcpy(output_tuple.I, &I, sizeof(I));
    memcpy(output_tuple.C, &C, sizeof(C));

    return output_tuple;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
const crypto::public_key &output_pubkey_cref(const OutputPair &output_pair)
{
    struct output_pair_visitor
    {
        const crypto::public_key &operator()(const LegacyOutputPair &o) const
        { return o.output_pubkey; }
        const crypto::public_key &operator()(const CarrotOutputPairV1 &o) const
        { return o.output_pubkey; }
    };

    return std::visit(output_pair_visitor{}, output_pair);
}
//----------------------------------------------------------------------------------------------------------------------
const crypto::ec_point &commitment_cref(const OutputPair &output_pair)
{
    struct output_pair_visitor
    {
        const crypto::ec_point &operator()(const LegacyOutputPair &o) const
        { return o.commitment; }
        const crypto::ec_point &operator()(const CarrotOutputPairV1 &o) const
        { return o.commitment; }
    };

    return std::visit(output_pair_visitor{}, output_pair);
}
//----------------------------------------------------------------------------------------------------------------------
bool output_checked_for_torsion(const OutputPair &output_pair)
{
    struct output_pair_visitor
    {
        bool operator()(const CarrotOutputPairV1&) const
        { return true; }
        bool operator()(const LegacyOutputPair&) const
        { return false; }
    };

    return std::visit(output_pair_visitor{}, output_pair);
}
//----------------------------------------------------------------------------------------------------------------------
bool use_biased_hash_to_point(const OutputPair &output_pair)
{
    struct output_pair_visitor
    {
        bool operator()(const CarrotOutputPairV1&) const
        { return false; }
        bool operator()(const LegacyOutputPair&) const
        { return true; }
    };

    return std::visit(output_pair_visitor{}, output_pair);
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
}//namespace fcmp_pp
