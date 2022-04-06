// Copyright (c) 2014-2022, The Monero Project
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

#include "chaingen.h"
#include "wallet/wallet2.h"

typedef struct {
  tools::wallet2::transfer_details * td;
  cryptonote::tx_source_entry * src;

  std::unordered_set<size_t> * selected_idx;
  std::unordered_set<crypto::key_image> * selected_kis;
  size_t ntrans;
  size_t iters;
  uint64_t sum;
  size_t cur_utxo;
} tx_source_info_crate_t;

typedef std::function<bool(const tx_source_info_crate_t &info, bool &abort)> fnc_accept_tx_source_t;

// Wallet friend, direct access to required fields and private methods
class wallet_accessor_test
{
public:
  static void set_account(tools::wallet2 * wallet, cryptonote::account_base& account);
  static tools::wallet2::transfer_container & get_transfers(tools::wallet2 * wallet) { return wallet->m_transfers; }
  static subaddresses_t & get_subaddresses(tools::wallet2 * wallet) { return wallet->m_subaddresses; }
  static void process_parsed_blocks(tools::wallet2 * wallet, uint64_t start_height, const std::vector<cryptonote::block_complete_entry> &blocks, const std::vector<tools::wallet2::parsed_block> &parsed_blocks, uint64_t& blocks_added);
};

class wallet_tools
{
public:
  static void gen_tx_src(size_t mixin, uint64_t cur_height, const tools::wallet2::transfer_details & td, cryptonote::tx_source_entry & src, block_tracker &bt);
  static void gen_block_data(block_tracker &bt, const cryptonote::block *bl, const map_hash2tx_t & mtx, cryptonote::block_complete_entry &bche, tools::wallet2::parsed_block &parsed_block, uint64_t &height);
  static void compute_subaddresses(std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddresses, cryptonote::account_base & creds, size_t account, size_t minors);
  static void process_transactions(tools::wallet2 * wallet, const std::vector<test_event_entry>& events, const cryptonote::block& blk_head, block_tracker &bt, const boost::optional<crypto::hash>& blk_tail=boost::none);
  static void process_transactions(tools::wallet2 * wallet, const std::vector<const cryptonote::block*>& blockchain, const map_hash2tx_t & mtx, block_tracker &bt);
  static bool fill_tx_sources(tools::wallet2 * wallet, std::vector<cryptonote::tx_source_entry>& sources, size_t mixin, const boost::optional<size_t>& num_utxo, const boost::optional<uint64_t>& min_amount, block_tracker &bt, std::vector<size_t> &selected, uint64_t cur_height, ssize_t offset=0, int step=1, const boost::optional<fnc_accept_tx_source_t>& fnc_accept=boost::none);
};

cryptonote::account_public_address get_address(const tools::wallet2*);

bool construct_tx_to_key(cryptonote::transaction& tx, tools::wallet2 * from_wallet, const var_addr_t& to, uint64_t amount,
                         std::vector<cryptonote::tx_source_entry> &sources,
                         uint64_t fee, bool rct=false, rct::RangeProofType range_proof_type=rct::RangeProofBorromean, int bp_version = 0);

bool construct_tx_to_key(cryptonote::transaction& tx, tools::wallet2 * sender_wallet, const std::vector<cryptonote::tx_destination_entry>& destinations,
                         std::vector<cryptonote::tx_source_entry> &sources,
                         uint64_t fee, bool rct, rct::RangeProofType range_proof_type, int bp_version = 0);

bool construct_tx_rct(tools::wallet2 * sender_wallet,
                      std::vector<cryptonote::tx_source_entry>& sources,
                      const std::vector<cryptonote::tx_destination_entry>& destinations,
                      const boost::optional<cryptonote::account_public_address>& change_addr,
                      std::vector<uint8_t> extra, cryptonote::transaction& tx, uint64_t unlock_time,
                      bool rct=false, rct::RangeProofType range_proof_type=rct::RangeProofBorromean, int bp_version = 0);
