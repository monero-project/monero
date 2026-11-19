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

/**
 * The goal of these core tests are as follows:
 *   1. Test that the core can handle a reorg over a HF boundary correctly for
 *      both a valid alt chain, and an invalid alt chain.
 *   2. Test that a transaction in a higher height, when popped out of the chain
 *      during a reorg across a HF boundary, does not verify in the lower HF.
 */

#pragma once 

#include <optional>

#include "chaingen.h"

class gen_reorg_over_hf_1 : public test_chain_unit_base
{
public: 
    gen_reorg_over_hf_1();

    bool generate(std::vector<test_event_entry>& events) const;
};

class gen_reorg_over_hf_2 : public test_chain_unit_base
{
public: 
    gen_reorg_over_hf_2();

    bool generate(std::vector<test_event_entry>& events) const;

    bool check_block_verification_context(const cryptonote::block_verification_context& bvc, size_t event_idx,
        const cryptonote::block& /*blk*/);

private:
    bool cb_set_expect_block_success(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
    bool cb_set_expect_block_failure(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
    bool cb_set_expect_block_inconclusive(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
    bool cb_set_expect_main_block(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
    bool cb_set_expect_alt_block(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events);

    std::optional<bool> m_expect_block_failure = false;
    bool m_expect_alt_block = false;
};
