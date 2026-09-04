// Copyright (c) 2017-2024, The Monero Project
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

#define IN_UNIT_TESTS
#include "cryptonote_basic/blobdatatype.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_core/blockchain.h"
#include "cryptonote_core/tx_verification_utils.h"
#include "fuzzer.h"

BEGIN_INIT_SIMPLE_FUZZER()
END_INIT_SIMPLE_FUZZER()

/**
 * `transaction` seed corpus format:
 *
 * first byte:        hard fork version to verify tx against
 * rest of the bytes: serialized transaction blob
 */

BEGIN_SIMPLE_FUZZER()
  size_t len_ = len;

  // pull hf_version as first byte
  std::uint8_t hf_version{};
  if (len_ > 0)
  {
    hf_version = *buf;
    --len_;
    ++buf;
  }
  
  // parse and validate tx from rest of buffer
  cryptonote::transaction tx;
  parse_and_validate_tx_from_blob(std::string((const char*)buf, len_), tx);

  // get coinbase height (or up to first 8 bytes of buffer if not applicable)
  std::uint64_t coinbase_height{};
  memcpy(&coinbase_height, buf, std::min(len_, sizeof(coinbase_height)));
  coinbase_height = coinbase_height % CRYPTONOTE_MAX_BLOCK_NUMBER;
  if (1 == tx.vin.size())
  {
    const cryptonote::txin_gen *in_gen = boost::strict_get<cryptonote::txin_gen>(tx.vin.data());
    if (in_gen) coinbase_height = in_gen->height;
  }

  // run tx through verification functions
  cryptonote::block b;
  b.miner_tx = tx;
  cryptonote::Blockchain::prevalidate_miner_transaction(b, coinbase_height, hf_version);
  cryptonote::tx_verification_context tvc{};
  cryptonote::ver_non_input_consensus(tx, tvc, hf_version);
END_SIMPLE_FUZZER()
