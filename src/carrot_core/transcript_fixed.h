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

// Transcript class for assembling data that needs to be hashed.

#pragma once

//local headers
#include "int-util.h"
#include "memwipe.h"

//third party headers

//standard headers
#include <cstddef>
#include <string>
#include <type_traits>

//forward declarations


namespace sp
{
namespace detail
{
template <typename... Ts>
constexpr size_t sizeof_sum()
{
    return (sizeof(Ts) + ...);
}

template <>
constexpr size_t sizeof_sum<>()
{
    return 0;
}
} //namespace detail

////
// SpFixedTranscript
// - build a transcript of a fixed bytesize and input types, enforced at compile time
// - written to be the simplest correct transcript of data possible
// - requires domain separators at compile-time as well
// - ensures that no two transcripts with different domain separators will ever be equal
// - does not use dynamic allocation
// - unsigned integers are added to the transcript in little-endian form
// - signed integers are not allowed
// - domain separator is length-prefixed with a single unsigned byte at the beginning
// - passed domain separator can be null terminated or not, null bytes and after will be dropped
///
template <std::size_t N, const unsigned char domain_sep[N], typename... Ts>
class SpFixedTranscript final
{
public:
//constructors
    /// normal constructor
    SpFixedTranscript(const Ts&... args)
    {
        // copy domain separator length prefix
        m_transcript[0] = static_cast<unsigned char>(domain_sep_size());

        // copy domain separator
        memcpy(m_transcript + 1, domain_sep, domain_sep_size());

        // copy types into buffer
        append<1 + domain_sep_size()>(args...);
    }

//overloaded operators
    /// disable copy/move
    SpFixedTranscript& operator=(const SpFixedTranscript&) = delete;
    SpFixedTranscript& operator=(SpFixedTranscript&&) = delete;

//member functions
    constexpr const void* data() const noexcept { return m_transcript; }

    static constexpr std::size_t size()
    {
        return 1 + domain_sep_size() + detail::sizeof_sum<Ts...>();
    }

//destructors
    ~SpFixedTranscript()
    {
        // wipe the buffer on leave in case it contains sensitive data
        memwipe(m_transcript, sizeof(m_transcript));
    }

private:
//member functions
    template <std::size_t>
    void append() {}

    template <std::size_t offset, typename U0, typename... Us>
    void append(const U0 &arg0, const Us&... args)
    {
        // write current argument to buffer
        write<offset>(arg0);

        // call append for next argument
        static constexpr size_t new_offset = offset + sizeof(arg0);
        append<new_offset>(args...);
    }

    template <std::size_t offset, typename U>
    void write(const U &val)
    {
        static_assert(std::has_unique_object_representations_v<U>);
        static_assert(std::is_standard_layout_v<U>);
        static_assert(!std::is_signed_v<U> || std::is_same_v<U, char>);
        static_assert(alignof(U) == 1);
        static_assert(!std::is_pointer_v<U>);

        memcpy(m_transcript + offset, &val, sizeof(val));
    }

    template <std::size_t offset>
    void write(std::uint16_t val)
    {
        val = SWAP16LE(val);
        memcpy(m_transcript + offset, &val, sizeof(val));
    }

    template <std::size_t offset>
    void write(std::uint32_t val)
    {
        val = SWAP32LE(val);
        memcpy(m_transcript + offset, &val, sizeof(val));
    }

    template <std::size_t offset>
    void write(std::uint64_t val)
    {
        val = SWAP64LE(val);
        memcpy(m_transcript + offset, &val, sizeof(val));
    }

    static constexpr std::size_t domain_sep_size()
    {
        for (std::size_t i = 0; i < N; ++i)
            if (domain_sep[i] == '\0')
                return i;

        return N;
    }

    static_assert(domain_sep_size() <= 255, "domain separator must be less than 256 characters long");

//member variables
    /// the transcript buffer
    unsigned char m_transcript[size()];
};

template <const auto & domain_sep, typename... Ts>
auto make_fixed_transcript(const Ts&... args)
{
    return SpFixedTranscript<std::size(domain_sep), domain_sep, Ts...>(args...);
}

} //namespace sp
