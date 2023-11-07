// Copyright (c) 2022, The Monero Project
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

#include "message_data_structs.h"

#include "cryptonote_config.h"
#include "serialization/wire.h"
#include "serialization/wire/adapted/vector.h"
#include "serialization/wire/wrapper/array.h"
#include "serialization/wire/wrappers_impl.h"

namespace cryptonote { namespace rpc
{
  namespace
  {
    // make all arrays required for ZMQ backwards compability

    template<typename F, typename T>
    void block_with_transactions_map(F& format, T& self)
    {
      using max_txes = wire::max_element_count<500>;
      wire::object(format,
        WIRE_FIELD(block),
        wire::field("transactions", wire::array<max_txes>(std::ref(self.transactions)))
      );
    }

    template<typename F, typename T>
    void transaction_info_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(transaction), WIRE_FIELD(in_pool), WIRE_FIELD(height));
    }

    template<typename F, typename T>
    void output_key_and_amount_index_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(amount_index), WIRE_FIELD(key));
    }

    template<typename F, typename T>
    void amount_with_random_outputs_map(F& format, T& self)
    {
      using min_size = wire::min_element_sizeof<crypto::public_key>;
      wire::object(format,
        WIRE_FIELD(amount),
        wire::field("outputs", wire::array<min_size>(std::ref(self.outputs)))
      );
    }

    template<typename F, typename T>
    void peer_map(F& format, T& self)
    {
      wire::object(format,
        WIRE_FIELD(id),
        WIRE_FIELD(ip),
        WIRE_FIELD(port),
        WIRE_FIELD(rpc_port),
        WIRE_FIELD(rpc_credits_per_hash),
        WIRE_FIELD(last_seen),
        WIRE_FIELD(pruning_seed)
      );
    }

    template<typename F, typename T>
    void tx_in_pool_map(F& format, T& self)
    {
      wire::object(format,
        WIRE_FIELD(tx),
        WIRE_FIELD(tx_hash),
        WIRE_FIELD(blob_size),
        WIRE_FIELD(weight),
        WIRE_FIELD(fee),
        WIRE_FIELD(max_used_block_hash),
        WIRE_FIELD(kept_by_block),
        WIRE_FIELD(last_failed_block_hash),
        WIRE_FIELD(last_failed_block_height),
        WIRE_FIELD(receive_time),
        WIRE_FIELD(last_relayed_time),
        WIRE_FIELD(relayed),
        WIRE_FIELD(do_not_relay),
        WIRE_FIELD(double_spend_seen)
      );
    }

    template<typename F, typename T>
    void output_amount_count_map(F& format, T& self)
    {
      wire::object(format,
        WIRE_FIELD(amount),
        WIRE_FIELD(total_count),
        WIRE_FIELD(unlocked_count),
        WIRE_FIELD(recent_count)
      );
    }

    template<typename F, typename T>
    void output_amount_and_index_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(amount), WIRE_FIELD(index));
    }

    template<typename F, typename T>
    void output_key_mask_unlocked_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(key), WIRE_FIELD(mask), WIRE_FIELD(unlocked));
    }

    template<typename F, typename T>
    void hard_fork_info_map(F& format, T& self)
    {
      wire::object(format,
        WIRE_FIELD(version),
        WIRE_FIELD(enabled),
        WIRE_FIELD(window),
        WIRE_FIELD(votes),
        WIRE_FIELD(threshold),
        WIRE_FIELD(voting),
        WIRE_FIELD(state),
        WIRE_FIELD(earliest_height)
      );
    }

    template<typename F, typename T>
    void block_header_response_map(F& format, T& self)
    {
      wire::object(format,
        WIRE_FIELD(major_version),
        WIRE_FIELD(minor_version),
        WIRE_FIELD(timestamp),
        WIRE_FIELD(prev_id),
        WIRE_FIELD(nonce),
        WIRE_FIELD(height),
        WIRE_FIELD(depth),
        WIRE_FIELD(hash),
        WIRE_FIELD(difficulty),
        WIRE_FIELD(reward)
      );
    }

    template<typename F, typename T>
    void daemon_info_map(F& format, T& self)
    {
      wire::object(format,
        WIRE_FIELD(height),
        WIRE_FIELD(target_height),
        WIRE_FIELD(difficulty),
        WIRE_FIELD(target),
        WIRE_FIELD(tx_count),
        WIRE_FIELD(tx_pool_size),
        WIRE_FIELD(alt_blocks_count),
        WIRE_FIELD(outgoing_connections_count),
        WIRE_FIELD(incoming_connections_count),
        WIRE_FIELD(white_peerlist_size),
        WIRE_FIELD(grey_peerlist_size),
        WIRE_FIELD(mainnet),
        WIRE_FIELD(testnet),
        WIRE_FIELD(stagenet),
        WIRE_FIELD(nettype),
        WIRE_FIELD(top_block_hash),
        WIRE_FIELD(cumulative_difficulty),
        WIRE_FIELD(block_size_limit),
        WIRE_FIELD(block_size_median),
        WIRE_FIELD(adjusted_time),
        WIRE_FIELD(block_weight_median),
        WIRE_FIELD(start_time),
        WIRE_FIELD(version)
      );
    }

    template<typename F, typename T>
    void output_distribution_map(F& format, T& self)
    {
      using max_distributions = wire::max_element_count<65536>;
      wire::object(format,
        WIRE_FIELD(amount),
        WIRE_FIELD(cumulative),
        wire::field("distribution", wire::array<max_distributions>(std::ref(self.data.distribution))),
        wire::field("start_height", std::ref(self.data.start_height)),
        wire::field("base", std::ref(self.data.base))
      );
    }
  } // anonymous

  WIRE_DEFINE_OBJECT(block_with_transactions, block_with_transactions_map)
  WIRE_DEFINE_OBJECT(transaction_info, transaction_info_map)
  WIRE_DEFINE_OBJECT(output_key_and_amount_index, output_key_and_amount_index_map)
  WIRE_DEFINE_OBJECT(amount_with_random_outputs, amount_with_random_outputs_map)
  WIRE_DEFINE_OBJECT(peer, peer_map)
  WIRE_DEFINE_OBJECT(tx_in_pool, tx_in_pool_map)
  WIRE_DEFINE_OBJECT(output_amount_count, output_amount_count_map)
  WIRE_DEFINE_OBJECT(output_amount_and_index, output_amount_and_index_map)
  WIRE_DEFINE_OBJECT(output_key_mask_unlocked, output_key_mask_unlocked_map)
  WIRE_DEFINE_OBJECT(hard_fork_info, hard_fork_info_map)
  WIRE_DEFINE_OBJECT(BlockHeaderResponse, block_header_response_map)
  WIRE_DEFINE_OBJECT(DaemonInfo, daemon_info_map)
  WIRE_DEFINE_OBJECT(output_distribution, output_distribution_map)
}} // cryptonote // rpc
