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

#include "curve_trees.h"

#include "crypto/crypto.h"
#include "fcmp_pp_crypto.h"
#include "fcmp_pp_types.h"
#include "profile_tools.h"


namespace fcmp_pp
{
namespace curve_trees
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
OutputTuple output_to_tuple(const OutputPair &output_pair)
{
    const crypto::public_key &output_pubkey = output_pubkey_cref(output_pair);
    const crypto::ec_point &commitment      = commitment_cref(output_pair);

    crypto::ec_point O = output_pubkey;
    crypto::ec_point C = commitment;

    // If the output has already been checked for torsion, then we don't need to clear torsion here
    if (!output_checked_for_torsion(output_pair))
    {
        TIME_MEASURE_NS_START(clear_torsion_ns);

        if (!fcmp_pp::get_valid_torsion_cleared_point(output_pubkey, O))
            throw std::runtime_error("O is invalid for insertion to tree");
        if (!fcmp_pp::get_valid_torsion_cleared_point(commitment, C))
            throw std::runtime_error("C is invalid for insertion to tree");

        if (O != output_pubkey)
            LOG_PRINT_L2("Output pubkey has torsion: " << output_pubkey);
        if (C != commitment)
            LOG_PRINT_L2("Commitment has torsion: " << commitment);

        TIME_MEASURE_NS_FINISH(clear_torsion_ns);

        LOG_PRINT_L3("clear_torsion_ns: " << clear_torsion_ns);
    }

#if !defined(NDEBUG)
    {
        // Debug build safety checks
        crypto::ec_point O_debug;
        crypto::ec_point C_debug;
        assert(fcmp_pp::get_valid_torsion_cleared_point(output_pubkey, O_debug));
        assert(fcmp_pp::get_valid_torsion_cleared_point(commitment, C_debug));
        assert(O == O_debug);
        assert(C == C_debug);
    }
#endif

    // Redundant check for safety
    if (O == crypto::EC_I)
        throw std::runtime_error("O cannot equal identity");
    if (C == crypto::EC_I)
        throw std::runtime_error("C cannot equal identity");

    TIME_MEASURE_NS_START(derive_key_image_generator_ns);

    // Derive key image generator using original output pubkey
    crypto::ec_point I;
    crypto::derive_key_image_generator(output_pubkey, use_biased_hash_to_point(output_pair), I);

    TIME_MEASURE_NS_FINISH(derive_key_image_generator_ns);

    LOG_PRINT_L3("derive_key_image_generator_ns: " << derive_key_image_generator_ns);

    return output_tuple_from_bytes(O, I, C);
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
} //namespace curve_trees
} //namespace fcmp_pp
