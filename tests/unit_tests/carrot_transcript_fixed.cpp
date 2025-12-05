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

#include "carrot_core/config.h"
#include "carrot_core/core_types.h"
#include "carrot_core/transcript_fixed.h"
#include "crypto/crypto.h"

#include <cstdint>

TEST(carrot_transcript_fixed, sizeof_sum)
{
    EXPECT_EQ(0, sp::detail::sizeof_sum<>());
    EXPECT_EQ(1, sp::detail::sizeof_sum<unsigned char>());
    EXPECT_EQ(12, (sp::detail::sizeof_sum<uint64_t, uint32_t>()));
}

static constexpr const unsigned char DS1[] = "perspicacious";
static constexpr const unsigned char DS2[] = "recrudescence";

TEST(carrot_transcript_fixed, ts_size)
{
    const auto transcript1 = sp::make_fixed_transcript<DS1>((uint32_t)32);
    EXPECT_EQ(1 + 13 + 4, transcript1.size());

    const auto transcript2 = sp::make_fixed_transcript<DS2>((uint32_t)32, (uint64_t)64);
    EXPECT_EQ(1 + 13 + 4 + 8, transcript2.size());

    // vt = H_3(s_sr || input_context || Ko)
    const auto transcript_vt = sp::make_fixed_transcript<carrot::CARROT_DOMAIN_SEP_VIEW_TAG>(
        carrot::input_context_t{},
        crypto::public_key{});
    EXPECT_EQ(1 + 15 + 33 + 32, transcript_vt.size());
}
