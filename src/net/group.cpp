// Copyright (c) 2014-2021, The Monero Project
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
// Parts of this file are originally Copyright (c) 2020 The Bitcoin Core developers

#include "group.h"

namespace net
{
namespace group
{

namespace {

constexpr uint32_t INVALID = 0xFFFFFFFF;
static const unsigned char pchIPv4[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff };

// NB: from bitcoin's <crypto/common.h>
/** Return the smallest number n such that (x >> n) == 0 (or 64 if the highest bit in x is set. */
uint64_t static inline CountBits(uint64_t x)
{
#if HAVE_BUILTIN_CLZL
    if (sizeof(unsigned long) >= sizeof(uint64_t)) {
        return x ? 8 * sizeof(unsigned long) - __builtin_clzl(x) : 0;
    }
#endif
#if HAVE_BUILTIN_CLZLL
    if (sizeof(unsigned long long) >= sizeof(uint64_t)) {
        return x ? 8 * sizeof(unsigned long long) - __builtin_clzll(x) : 0;
    }
#endif
    int ret = 0;
    while (x) {
        x >>= 1;
        ++ret;
    }
    return ret;
}

uint32_t DecodeBits(std::vector<bool>::const_iterator& bitpos, const std::vector<bool>::const_iterator& endpos, uint8_t minval, const std::vector<uint8_t> &bit_sizes)
{
    uint32_t val = minval;
    bool bit;
    for (std::vector<uint8_t>::const_iterator bit_sizes_it = bit_sizes.begin();
        bit_sizes_it != bit_sizes.end(); ++bit_sizes_it) {
        if (bit_sizes_it + 1 != bit_sizes.end()) {
            if (bitpos == endpos) break;
            bit = *bitpos;
            bitpos++;
        } else {
            bit = 0;
        }
        if (bit) {
            val += (1 << *bit_sizes_it);
        } else {
            for (int b = 0; b < *bit_sizes_it; b++) {
                if (bitpos == endpos) return INVALID; // Reached EOF in mantissa
                bit = *bitpos;
                bitpos++;
                val += bit << (*bit_sizes_it - 1 - b);
            }
            return val;
        }
    }
    return INVALID; // Reached EOF in exponent
}

// See https://github.com/bitcoin/bitcoin/blob/master/contrib/asmap/asmap.py#L148
enum class Instruction : uint32_t
{
    /**One instruction in the binary asmap format.
       A return instruction, encoded as [0], returns a constant ASN. It is followed by
       an integer using the ASN encoding.*/
    RETURN = 0,
    /**A jump instruction, encoded as [1,0] inspects the next unused bit in the input
       and either continues execution (if 0), or skips a specified number of bits (if 1).
       It is followed by an integer, and then two subprograms. The integer uses jump encoding
       and corresponds to the length of the first subprogram (so it can be skipped).*/
    JUMP = 1,
    /**A match instruction, encoded as [1,1,0] inspects 1 or more of the next unused bits
       in the input with its argument. If they all match, execution continues. If they do
       not, failure is returned. If a default instruction has been executed before, instead
       of failure the default instruction's argument is returned. It is followed by an
       integer in match encoding, and a subprogram. That value is at least 2 bits and at
       most 9 bits. An n-bit value signifies matching (n-1) bits in the input with the lower
       (n-1) bits in the match value.*/
    MATCH = 2,
    /**A default instruction, encoded as [1,1,1] sets the default variable to its argument,
       and continues execution. It is followed by an integer in ASN encoding, and a subprogram.*/
    DEFAULT = 3,
    /**Not an actual instruction, but a way to encode the empty program that fails. In the
       encoder, it is used more generally to represent the failure case inside MATCH instructions,
       which may (if used inside the context of a DEFAULT instruction) actually correspond to
       a successful return. In this usage, they're always converted to an actual MATCH or RETURN
      before the top level is reached (see make_default below).*/
    END = 4
};

const std::vector<uint8_t> TYPE_BIT_SIZES{0, 0, 1};
Instruction DecodeType(std::vector<bool>::const_iterator& bitpos, const std::vector<bool>::const_iterator& endpos)
{
    return Instruction(DecodeBits(bitpos, endpos, 0, TYPE_BIT_SIZES));
}

const std::vector<uint8_t> ASN_BIT_SIZES{15, 16, 17, 18, 19, 20, 21, 22, 23, 24};
uint32_t DecodeASN(std::vector<bool>::const_iterator& bitpos, const std::vector<bool>::const_iterator& endpos)
{
    return DecodeBits(bitpos, endpos, 1, ASN_BIT_SIZES);
}


const std::vector<uint8_t> MATCH_BIT_SIZES{1, 2, 3, 4, 5, 6, 7, 8};
uint32_t DecodeMatch(std::vector<bool>::const_iterator& bitpos, const std::vector<bool>::const_iterator& endpos)
{
    return DecodeBits(bitpos, endpos, 2, MATCH_BIT_SIZES);
}


const std::vector<uint8_t> JUMP_BIT_SIZES{5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30};
uint32_t DecodeJump(std::vector<bool>::const_iterator& bitpos, const std::vector<bool>::const_iterator& endpos)
{
    return DecodeBits(bitpos, endpos, 17, JUMP_BIT_SIZES);
}

}

uint32_t Interpret(const std::vector<bool> &asmap, const std::vector<bool> &ip)
{
    std::vector<bool>::const_iterator pos = asmap.begin();
    const std::vector<bool>::const_iterator endpos = asmap.end();
    uint8_t bits = ip.size();
    uint32_t default_asn = 0;
    uint32_t jump, match, matchlen;
    Instruction opcode;
    while (pos != endpos) {
        opcode = DecodeType(pos, endpos);
        if (opcode == Instruction::RETURN) {
            default_asn = DecodeASN(pos, endpos);
            if (default_asn == INVALID) break; // ASN straddles EOF
            return default_asn;
        } else if (opcode == Instruction::JUMP) {
            jump = DecodeJump(pos, endpos);
            if (jump == INVALID) break; // Jump offset straddles EOF
            if (bits == 0) break; // No input bits left
            if (int64_t{jump} >= int64_t{endpos - pos}) break; // Jumping past EOF
            if (ip[ip.size() - bits]) {
                pos += jump;
            }
            bits--;
        } else if (opcode == Instruction::MATCH) {
            match = DecodeMatch(pos, endpos);
            if (match == INVALID) break; // Match bits straddle EOF
            matchlen = CountBits(match) - 1;
            if (bits < matchlen) break; // Not enough input bits
            for (uint32_t bit = 0; bit < matchlen; bit++) {
                if ((ip[ip.size() - bits]) != ((match >> (matchlen - 1 - bit)) & 1)) {
                    return default_asn;
                }
                bits--;
            }
        } else if (opcode == Instruction::DEFAULT) {
            default_asn = DecodeASN(pos, endpos);
            if (default_asn == INVALID) break; // ASN straddles EOF
        } else {
            break; // Instruction straddles EOF
        }
    }
    assert(false); // Reached EOF without RETURN, or aborted (see any of the breaks above) - should have been caught by SanityCheckASMap below
    return 0; // 0 is not a valid ASN
}

bool SanityCheckASMap(const std::vector<bool>& asmap, int bits)
{
    const std::vector<bool>::const_iterator begin = asmap.begin(), endpos = asmap.end();
    std::vector<bool>::const_iterator pos = begin;
    std::vector<std::pair<uint32_t, int>> jumps; // All future positions we may jump to (bit offset in asmap -> bits to consume left)
    jumps.reserve(bits);
    Instruction prevopcode = Instruction::JUMP;
    bool had_incomplete_match = false;
    while (pos != endpos) {
        uint32_t offset = pos - begin;
        if (!jumps.empty() && offset >= jumps.back().first) return false; // There was a jump into the middle of the previous instruction
        Instruction opcode = DecodeType(pos, endpos);
        if (opcode == Instruction::RETURN) {
            if (prevopcode == Instruction::DEFAULT) return false; // There should not be any RETURN immediately after a DEFAULT (could be combined into just RETURN)
            uint32_t asn = DecodeASN(pos, endpos);
            if (asn == INVALID) return false; // ASN straddles EOF
            if (jumps.empty()) {
                // Nothing to execute anymore
                if (endpos - pos > 7) return false; // Excessive padding
                while (pos != endpos) {
                    if (*pos) return false; // Nonzero padding bit
                    ++pos;
                }
                return true; // Sanely reached EOF
            } else {
                // Continue by pretending we jumped to the next instruction
                offset = pos - begin;
                if (offset != jumps.back().first) return false; // Unreachable code
                bits = jumps.back().second; // Restore the number of bits we would have had left after this jump
                jumps.pop_back();
                prevopcode = Instruction::JUMP;
            }
        } else if (opcode == Instruction::JUMP) {
            uint32_t jump = DecodeJump(pos, endpos);
            if (jump == INVALID) return false; // Jump offset straddles EOF
            if (int64_t{jump} > int64_t{endpos - pos}) return false; // Jump out of range
            if (bits == 0) return false; // Consuming bits past the end of the input
            --bits;
            uint32_t jump_offset = pos - begin + jump;
            if (!jumps.empty() && jump_offset >= jumps.back().first) return false; // Intersecting jumps
            jumps.emplace_back(jump_offset, bits);
            prevopcode = Instruction::JUMP;
        } else if (opcode == Instruction::MATCH) {
            uint32_t match = DecodeMatch(pos, endpos);
            if (match == INVALID) return false; // Match bits straddle EOF
            int matchlen = CountBits(match) - 1;
            if (prevopcode != Instruction::MATCH) had_incomplete_match = false;
            if (matchlen < 8 && had_incomplete_match) return false; // Within a sequence of matches only at most one should be incomplete
            had_incomplete_match = (matchlen < 8);
            if (bits < matchlen) return false; // Consuming bits past the end of the input
            bits -= matchlen;
            prevopcode = Instruction::MATCH;
        } else if (opcode == Instruction::DEFAULT) {
            if (prevopcode == Instruction::DEFAULT) return false; // There should not be two successive DEFAULTs (they could be combined into one)
            uint32_t asn = DecodeASN(pos, endpos);
            if (asn == INVALID) return false; // ASN straddles EOF
            prevopcode = Instruction::DEFAULT;
        } else {
            return false; // Instruction straddles EOF
        }
    }
    return false; // Reached EOF without RETURN instruction
}

// Equivalent to bitcoin's CAddrMan::DecodeAsmap(fs::path path)
bool read_asmap(const std::string &path, std::vector<bool>& bits)
{
    MINFO("Using AS mapping file: " << path);
    std::ifstream ifs(path, std::ios::binary | std::ios::in | std::ios::ate);
    if (!ifs.is_open())
    {
        MERROR("Failed to open asmap file " << path);
        return false;
    }
    bits.clear();
    bits.reserve(8 * ifs.tellg());
    ifs.seekg(std::ios_base::beg);
    char cur_byte;
    while (ifs.get(cur_byte))
    {
        for (int bit = 0; bit < 8; ++bit) {
            bits.push_back((cur_byte >> bit) & 1);
        }
    }
    if (!SanityCheckASMap(bits, 128)) {
        MERROR("Sanity check of asmap file failed, path: " << path);
        return false;
    }
    return true;
}

uint32_t get_group(const uint32_t ip, const std::vector<bool> &asmap)
{
    if (asmap.size() == 0)
        return ip & 0x0000ffff;

    std::vector<bool> ip_bits(128);
    // For lookup, treat as if it was just an IPv4 address (pchIPv4 prefix + IPv4 bits)
    for (int8_t byte_i = 0; byte_i < 12; ++byte_i) {
        for (uint8_t bit_i = 0; bit_i < 8; ++bit_i) {
            ip_bits[byte_i * 8 + bit_i] = (pchIPv4[byte_i] >> (7 - bit_i)) & 1;
        }
    }
    // Bitcoin's code uses big endian internally
    auto ip_be = swap32(ip);
    for (int i = 0; i < 32; ++i) {
        ip_bits[96 + i] = (ip_be >> (31 - i)) & 1;
    }
    auto group = Interpret(asmap, ip_bits);
    if (group) return group;
    return ip & 0x0000ffff;
}

uint32_t get_group(const boost::asio::ip::address_v6& ip, const std::vector<bool> &asmap)
{
    if (ip.is_v4_mapped())
    {
        boost::asio::ip::address_v4 v4ip = make_address_v4_from_v6(ip);
        uint32_t actual_ipv4;
        memcpy(&actual_ipv4, v4ip.to_bytes().data(), sizeof(actual_ipv4));
        return get_group(actual_ipv4, asmap);
    }
    return 0;
}

uint32_t get_group(const epee::net_utils::network_address& address, const std::vector<bool> &asmap)
{
    switch(address.get_type_id())
    {
        case epee::net_utils::ipv4_network_address::get_type_id():
        {
            const epee::net_utils::network_address na = address;
            return get_group(na.as<const epee::net_utils::ipv4_network_address>().ip(), asmap);
        }
        case epee::net_utils::ipv6_network_address::get_type_id():
        {
            const epee::net_utils::network_address na = address;
            return get_group(na.as<const epee::net_utils::ipv6_network_address>().ip(), asmap);
        }
        default:
            return 0;
    }
}

} // group
} // net
