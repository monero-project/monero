// Copyright (c) 2021, The Monero Project
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

#include "cryptonote_protocol_defs.h"

#include <boost/range/adaptor/transformed.hpp>
#include <tuple>

#include "serialization/wire/epee.h"
#include "serialization/wire/traits.h"
#include "serialization/wire/wrappers.h"
#include "serialization/wire/wrappers_impl.h"
#include "storages/portable_storage_base.h"

namespace cryptonote
{
  namespace
  {
    template<typename F, typename T>
    void tx_blob_entry_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(blob), WIRE_FIELD(prunable_hash));
    }

    //! `is_pruned` type is a tag for custom serialization
    enum class is_pruned { false_ = 0, true_ };
    void read_bytes(wire::epee_reader& source, std::pair<std::vector<tx_blob_entry>, is_pruned>& dest)
    {
      // read function for `tx_blob_entry` detects whether pruned (object) or !pruned (string)
      wire_read::array(source, dest.first, tx_blob_min{});
    }
    void write_bytes(wire::epee_writer& dest, const std::pair<const std::vector<tx_blob_entry>&, is_pruned> source)
    {
      const auto get_blob = [] (const tx_blob_entry& e) -> std::string { return e.blob; };
      if (source.second == is_pruned::true_) // write array of `tx_blob_entry` objects
        wire_write::array(dest, source.first);
      else // !pruned -> write array of string blobs
        wire_write::array(dest, boost::adaptors::transform(source.first, get_blob));
    }

    template<typename F, typename T, typename U>
    void block_complete_entry_map(F& format, T& self, boost::optional<U>& txs)
    {
      wire::object(format,
        WIRE_FIELD_DEFAULTED(pruned, false),
        WIRE_FIELD(block),
        WIRE_FIELD_DEFAULTED(block_weight, unsigned(0)),
        wire::optional_field("txs", std::ref(txs))
      );
      const bool is_schema_mismatch =
        txs && !txs->first.empty() &&
        txs->second == is_pruned::false_ &&
        txs->first.back().prunable_hash != crypto::null_hash;
      if (is_schema_mismatch)
        WIRE_DLOG_THROW(wire::error::schema::object, "Schema mismatch with pruned flag set to " << self.pruned);
    }
  } // anonymous

  void read_bytes(wire::epee_reader& source, tx_blob_entry& dest)
  {
    if ((source.last_tag() & SERIALIZE_TYPE_STRING) == SERIALIZE_TYPE_STRING)
    {
      wire_read::bytes(source, dest.blob);
      dest.prunable_hash = crypto::null_hash;
    }
    else
      tx_blob_entry_map(source, dest);
  }
  void write_bytes(wire::epee_writer& dest, const tx_blob_entry& source)
  {
    tx_blob_entry_map(dest, source);
  }
  void read_bytes(wire::epee_reader& source, block_complete_entry& dest)
  {
    boost::optional<std::pair<std::vector<tx_blob_entry>, is_pruned>> txs;
    block_complete_entry_map(source, dest, txs);
    if (txs)
      dest.txs = std::move(txs->first);
    else
      dest.txs.clear();
  }
  void write_bytes(wire::epee_writer& dest, const block_complete_entry& source)
  {
    auto txs = boost::make_optional(!source.txs.empty(), std::make_pair(std::cref(source.txs), is_pruned(source.pruned)));
    block_complete_entry_map(dest, source, txs);
  }
  WIRE_EPEE_DEFINE_CONVERSION(NOTIFY_NEW_BLOCK::request);
  WIRE_EPEE_DEFINE_CONVERSION(NOTIFY_NEW_TRANSACTIONS::request)
  WIRE_EPEE_DEFINE_CONVERSION(NOTIFY_REQUEST_GET_OBJECTS::request);
  WIRE_EPEE_DEFINE_CONVERSION(NOTIFY_RESPONSE_GET_OBJECTS::request);
  WIRE_EPEE_DEFINE_CONVERSION(CORE_SYNC_DATA);
  WIRE_EPEE_DEFINE_CONVERSION(NOTIFY_REQUEST_CHAIN::request);
  WIRE_EPEE_DEFINE_CONVERSION(NOTIFY_RESPONSE_CHAIN_ENTRY::request);
  WIRE_EPEE_DEFINE_CONVERSION(NOTIFY_NEW_FLUFFY_BLOCK::request);
  WIRE_EPEE_DEFINE_CONVERSION(NOTIFY_REQUEST_FLUFFY_MISSING_TX::request);
  WIRE_EPEE_DEFINE_CONVERSION(NOTIFY_GET_TXPOOL_COMPLEMENT::request);
} // cryptonote
