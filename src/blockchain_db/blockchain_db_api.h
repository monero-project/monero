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

/// @file New API for blockchain/mempool databases

#pragma once

//local headers

//third party headers

//standard headers
#include <cstddef>
#include <cstdint>
#include <string>

//forward declarations

namespace cryptonote
{
namespace db
{
struct bytes32
{
    unsigned char data[32];
};

template <size_t N>
struct varbytes
{
    static_assert(N < UINT8_MAX);
    uint8_t size;
    unsigned char data[N];
};

class ChainDB
{
public:
    class ReadView
    {
    public:
        virtual ~ReadView() = 0;
    };

    class WriteView: public ReadView
    {
    public:
    };

    virtual ~ChainDB() = 0;

    virtual ReadView* read_chain() = 0;

    virtual WriteView* write_chain(std::size_t size_increase_hint = 0) = 0;
};

class PoolDB
{
public:
    class ReadView
    {
    public:
        virtual ~ReadView() = 0;
    };

    class WriteView: public ReadView
    {
    public:
    };

    virtual ~PoolDB() = 0;

    virtual ReadView* read_pool() = 0;

    virtual WriteView* write_pool(std::size_t size_increase_hint = 0) = 0;
};

} //namespace db
} //namespace cryptonote
