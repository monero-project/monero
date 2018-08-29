// Copyright (c) 2018, The Monero Project
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

#include <string>
#include <vector>
#include <lmdb.h>
#include "wipeable_string.h"
#include "crypto/crypto.h"
#include "cryptonote_basic/cryptonote_basic.h"

namespace tools
{
  class ringdb
  {
  public:
    ringdb(std::string filename, const std::string &genesis);
    void close();
    ~ringdb();

    bool add_rings(const crypto::chacha_key &chacha_key, const cryptonote::transaction_prefix &tx);
    bool remove_rings(const crypto::chacha_key &chacha_key, const cryptonote::transaction_prefix &tx);
    bool get_ring(const crypto::chacha_key &chacha_key, const crypto::key_image &key_image, std::vector<uint64_t> &outs);
    bool set_ring(const crypto::chacha_key &chacha_key, const crypto::key_image &key_image, const std::vector<uint64_t> &outs, bool relative);

    bool blackball(const std::pair<uint64_t, uint64_t> &output);
    bool blackball(const std::vector<std::pair<uint64_t, uint64_t>> &outputs);
    bool unblackball(const std::pair<uint64_t, uint64_t> &output);
    bool blackballed(const std::pair<uint64_t, uint64_t> &output);
    bool clear_blackballs();

  private:
    bool blackball_worker(const std::vector<std::pair<uint64_t, uint64_t>> &outputs, int op);

  private:
    std::string filename;
    MDB_env *env;
    MDB_dbi dbi_rings;
    MDB_dbi dbi_blackballs;
  };
}
