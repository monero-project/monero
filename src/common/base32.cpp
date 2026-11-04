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
//
// Parts of this file are originally copyright (c) 2021-2026 SChernykh

#include "base32.h"

namespace tools
{
namespace base32
{
namespace
{
    constexpr const char alphabet[] = u8"abcdefghijklmnopqrstuvwxyz234567";
    constexpr const std::uint16_t five_bit_mask{0x1F};
}

std::string encode(const epee::span<const std::uint8_t> data)
{
    if (data.empty()) return {};

    std::string result{};
    const std::size_t expected_size = ((data.size() * 8) + 4) / 5;
    result.reserve(expected_size);

    std::uint16_t val = 0;
    std::uint8_t valb = 0;

    for (const std::uint8_t byte : data)
    {
        val = (val << 8) | static_cast<std::uint16_t>(byte);
        valb += 8;

        while (valb >= 5)
        {
            valb -= 5;
            result.push_back(alphabet[(val >> valb) & five_bit_mask]);
        }
    }

    if (valb > 0)
    {
        result.push_back(alphabet[(val << (5 - valb)) & five_bit_mask]);
    }

    return result;
}

bool decode(const std::string_view input, std::vector<std::uint8_t>& data)
{
    data.clear();
    if (input.empty()) return true;

    data.reserve((input.size() * 5) / 8);

    std::uint16_t val = 0;
    std::uint8_t bits = 0;

    for (const char c : input)
    {
        std::uint16_t digit;
        if ('a' <= c && c <= 'z')
            digit = static_cast<std::uint16_t>(c - 'a');
        else if ('A' <= c && c <= 'Z')
            digit = static_cast<std::uint16_t>(c - 'A');
        else if ('2' <= c && c <= '7')
            digit = static_cast<std::uint16_t>(c - '2') + 26;
        else
            return false;

        val = (val << 5) | digit;
        bits += 5;

        if (bits >= 8)
        {
            bits -= 8;
            data.push_back(static_cast<std::uint8_t>(val >> bits));
        }
    }

    return true;
}

} // namespace base32
} // namespace tools
