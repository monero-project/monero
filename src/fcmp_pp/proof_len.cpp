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

#include "proof_len.h"

#include "fcmp_pp_rust/fcmp++.h"
#include "misc_log_ex.h"

namespace fcmp_pp
{
//----------------------------------------------------------------------------------------------------------------------
std::size_t membership_proof_len(const std::size_t n_inputs, const uint8_t n_layers)
{
    CHECK_AND_ASSERT_THROW_MES(n_inputs > 0, "n_inputs must be >0");
    CHECK_AND_ASSERT_THROW_MES(n_layers > 0, "n_layers must be >0");
    CHECK_AND_ASSERT_THROW_MES(n_inputs <= FCMP_PLUS_PLUS_MAX_INPUTS, "n_inputs must be <= FCMP_PLUS_PLUS_MAX_INPUTS");
    CHECK_AND_ASSERT_THROW_MES(n_layers <= FCMP_PLUS_PLUS_MAX_LAYERS, "n_layers must be <= FCMP_PLUS_PLUS_MAX_LAYERS");

    static_assert(sizeof(uint16_t) * FCMP_PLUS_PLUS_MAX_INPUTS * FCMP_PLUS_PLUS_MAX_LAYERS == sizeof(PROOF_LEN_TABLE),
        "unexpected table size");

    // This will break platforms with < 16-bit word size. One solution is to use uint16_t for the proof len everywhere
    static_assert(sizeof(std::size_t) >= sizeof(uint16_t), "cannot cast uint16_t to size_t");
    return (std::size_t) PROOF_LEN_TABLE[n_inputs-1][n_layers-1];
};

std::size_t fcmp_pp_proof_len(const std::size_t n_inputs, const uint8_t n_layers)
{
    return membership_proof_len(n_inputs, n_layers)
        + (n_inputs * (FCMP_PP_INPUT_TUPLE_SIZE_V1 + FCMP_PP_SAL_PROOF_SIZE_V1));
};
//----------------------------------------------------------------------------------------------------------------------
}//namespace fcmp_pp
