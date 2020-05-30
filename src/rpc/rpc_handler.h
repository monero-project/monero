// Copyright (c) 2016-2020, The Monero Project
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

#include <boost/optional/optional.hpp>
#include <cstdint>
#include <string>
#include <vector>
#include "byte_slice.h"
#include "crypto/hash.h"

namespace cryptonote
{
class core;

namespace rpc
{

struct output_distribution_data
{
  std::vector<std::uint64_t> distribution;
  std::uint64_t start_height;
  std::uint64_t base;
};

class RpcHandler
{
  public:
    RpcHandler() { }
    virtual ~RpcHandler() { }

    virtual epee::byte_slice handle(std::string&& request) = 0;

    static boost::optional<output_distribution_data>
      get_output_distribution(const std::function<bool(uint64_t, uint64_t, uint64_t, uint64_t&, std::vector<uint64_t>&, uint64_t&)> &f, uint64_t amount, uint64_t from_height, uint64_t to_height, const std::function<crypto::hash(uint64_t)> &get_hash, bool cumulative, uint64_t blockchain_height);
};


}  // rpc

}  // cryptonote
