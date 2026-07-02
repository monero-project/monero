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
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

#include <cstddef>

#include "crypto/hash.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "ringct/rctOps.h"
#include "ringct/rctTypes.h"

/**
 * @brief: Test perf of calculating a transaction hash from a blob on a BP+ tx
 * @tparam hash_from_blob true for calculate_transaction_hash_from_blob(), otherwise get_transaction_hash()
 * @tparam n_inputs number of tx inputs
 * @tparam n_outputs number of tx outputs
 */
template<bool hash_from_blob, std::size_t n_inputs, std::size_t n_outputs>
class test_bpp_tx_blob_to_hash 
{
public:
    static const std::size_t loop_count = 1000;

    static constexpr std::size_t extra_len = 1 + 32 + 1 + 1 + 1 + 8; // main tx pubkey and dummy encrypted payment ID
    static constexpr std::size_t ring_size = 16;

    bool init()
    {
        static const cryptonote::txin_to_key inp{
            .amount = 0,
            .key_offsets = {90195203, 6886484, 54104233, 42303, 691774,
                456173, 1271021, 18026, 333412, 316502, 95847, 61774,
                3078, 391, 1485, 158},
            .k_image = {}
        };

        cryptonote::transaction tx;
        tx.version = 2;
        tx.unlock_time = 0;
        tx.vin.resize(n_inputs, inp);
        tx.vout.resize(n_outputs,
            cryptonote::tx_out{.amount = 0, .target = cryptonote::txout_to_tagged_key{}});
        tx.extra.resize(extra_len);

        tx.rct_signatures.type = rct::RCTTypeBulletproofPlus;
        tx.rct_signatures.ecdhInfo.resize(n_outputs);
        tx.rct_signatures.outPk.resize(n_outputs, {{}, rct::I});
        tx.rct_signatures.txnFee = 122960000;

        std::size_t n_lr = 0;
        while ((1 << n_lr) < n_outputs) ++n_lr;
        n_lr += 6;
        auto &bpp = tx.rct_signatures.p.bulletproofs_plus.emplace_back();
        bpp.L.resize(n_lr);
        bpp.R.resize(n_lr);
        tx.rct_signatures.p.CLSAGs.resize(n_inputs, {
            .s = rct::keyV(ring_size)
        });
        tx.rct_signatures.p.pseudoOuts.resize(n_inputs);

        m_tx_blob = cryptonote::tx_to_blob(tx);

        return true;
    }

    bool test()
    {
        crypto::hash tx_hash;
        if constexpr (hash_from_blob)
        {
            if (!cryptonote::calculate_transaction_hash_from_blob(m_tx_blob, crypto::null_hash, tx_hash))
                return false;
        }
        else // blob -> tx -> hash
        {
            cryptonote::transaction tx;
            if (!cryptonote::parse_and_validate_tx_from_blob(m_tx_blob, tx))
                return false;

            if (!cryptonote::get_transaction_hash(tx, tx_hash))
                return false;
        }

        return true;
    }

private:
    cryptonote::blobdata m_tx_blob;
};
