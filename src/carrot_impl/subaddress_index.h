// Copyright (c) 2025, The Monero Project
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

//local headers

//third party headers

//standard headers
#include <cstdint>

//forward declarations

namespace carrot
{
/**
 * brief: subaddress_index -
 */
struct subaddress_index
{
    // j_major
    std::uint32_t major;
    // j_minor
    std::uint32_t minor;

    bool is_subaddress() const
    {
        return major || minor;
    }
};
static inline bool operator==(const subaddress_index a, const subaddress_index b)
{
    return a.major == b.major && a.minor == b.minor;
}
static inline bool operator!=(const subaddress_index a, const subaddress_index b)
{
    return !(a == b);
}

/**
 * brief: AddressDeriveType - used in hybrid key hierarchies to specify how to derive subaddresses for the same index
 */
enum class AddressDeriveType
{
    Auto,
    PreCarrot,
    Carrot
};

struct subaddress_index_extended
{
    subaddress_index index;
    AddressDeriveType derive_type;
};
static inline bool operator==(const subaddress_index_extended &a, const subaddress_index_extended &b)
{
    return a.index == b.index && a.derive_type == b.derive_type;
}
static inline bool operator!=(const subaddress_index_extended &a, const subaddress_index_extended &b)
{
    return !(a == b);
}
} //namespace carrot
