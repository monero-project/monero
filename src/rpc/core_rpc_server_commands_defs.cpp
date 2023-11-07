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

#include "core_rpc_server_commands_defs.h"

#include <string>
#include <vector>

#include "common/varint.h"
#include "cryptonote_config.h"
#include "serialization/wire/adapted/list.h"
#include "serialization/wire/adapted/static_vector.h"
#include "serialization/wire/adapted/vector.h"
#include "serialization/wire/epee.h"
#include "serialization/wire/error.h"
#include "serialization/wire/json.h"
#include "serialization/wire/wrappers.h"
#include "serialization/wire/wrappers_impl.h"

namespace cryptonote
{
  WIRE_EPEE_DEFINE_COMMAND(COMMAND_RPC_GET_BLOCKS_FAST)
  WIRE_EPEE_DEFINE_COMMAND(COMMAND_RPC_GET_BLOCKS_BY_HEIGHT)
  WIRE_EPEE_DEFINE_COMMAND(COMMAND_RPC_GET_HASHES_FAST)
  WIRE_EPEE_DEFINE_COMMAND(COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES)
  WIRE_EPEE_DEFINE_COMMAND(COMMAND_RPC_GET_OUTPUTS_BIN)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_GET_HEIGHT)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_GET_ALT_BLOCKS_HASHES)
  WIRE_EPEE_DEFINE_COMMAND(COMMAND_RPC_SUBMIT_RAW_TX)

  std::error_code convert_to_json(std::string& dest, const COMMAND_RPC_SUBMIT_RAW_TX::request& source)
  { return wire_write::to_bytes<wire::json_string_writer>(dest, source); }

  std::error_code convert_from_json(const epee::span<const char> source, COMMAND_RPC_SUBMIT_RAW_TX::response& dest)
  { return wire_read::from_bytes<wire::json_reader>(source, dest); }


  namespace
  {
    struct optional_entry_fields
    {
      boost::optional<std::uint64_t> block_height;
      boost::optional<std::uint64_t> confirmations;
      boost::optional<std::uint64_t> block_timestamp;
      boost::optional<std::uint64_t> received_timestamp;
      boost::optional<bool> relayed;
    };

    template<typename F, typename T>
    void get_transactions_entry_map(F& format, T& self, optional_entry_fields& optional)
    {
      wire::object(format,
        WIRE_FIELD(tx_hash),
        WIRE_FIELD(as_hex),
        WIRE_FIELD(pruned_as_hex),
        WIRE_FIELD(prunable_as_hex),
        WIRE_FIELD(prunable_hash),
        WIRE_FIELD(as_json),
        WIRE_FIELD(in_pool),
        WIRE_FIELD(double_spend_seen),
        wire::optional_field("block_height", std::ref(optional.block_height)),
        wire::optional_field("confirmations", std::ref(optional.confirmations)),
        wire::optional_field("block_timestamp", std::ref(optional.block_timestamp)),
        wire::optional_field("received_timestamp", std::ref(optional.received_timestamp)),
        WIRE_FIELD_ARRAY(output_indices, wire::max_element_count<512>),
        wire::optional_field("relayed", std::ref(optional.relayed))
      );
    }
  }

  void read_bytes(wire::json_reader& source, COMMAND_RPC_GET_TRANSACTIONS::entry& dest)
  {
    optional_entry_fields optional{};
    get_transactions_entry_map(source, dest, optional);

    if (dest.in_pool)
    {
      if (optional.received_timestamp && optional.relayed)
      {
        dest.received_timestamp = *optional.received_timestamp;
        dest.relayed = *optional.relayed;
        dest.block_height = 0;
        dest.confirmations = 0;
        dest.block_timestamp = 0;
        dest.output_indices.clear();
      }
      else
        WIRE_DLOG_THROW(wire::error::schema::missing_key, "expected keys \"received_timestamp\" and \"relayed\"");
    }
    else
    {
      if (optional.block_height && optional.confirmations && optional.block_timestamp)
      {
        dest.block_height = *optional.block_height;
        dest.confirmations = *optional.confirmations;
        dest.block_timestamp = *optional.block_timestamp;
        dest.received_timestamp = 0;
        dest.relayed = false;
      }
      else
        WIRE_DLOG_THROW(wire::error::schema::missing_key, "expected keys \"block_height\", \"confirmations\", and \"block_timestamp\"");
    }
  }
  void write_bytes(wire::json_writer& dest, const COMMAND_RPC_GET_TRANSACTIONS::entry& source)
  {
    optional_entry_fields optional{};

    if (source.in_pool)
    {
      optional.received_timestamp.emplace(source.received_timestamp);
      optional.relayed.emplace(source.relayed);
    }
    else
    {
      optional.block_height.emplace(source.block_height);
      optional.confirmations.emplace(source.confirmations);
      optional.block_timestamp.emplace(source.block_timestamp);
    }
    get_transactions_entry_map(dest, source, optional);
  }

  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_GET_TRANSACTIONS)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_IS_KEY_IMAGE_SPENT)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_GET_OUTPUTS)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_SEND_RAW_TX)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_START_MINING)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_GET_INFO)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_GET_NET_STATS)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_STOP_MINING)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_MINING_STATUS)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_SAVE_BC)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_GENERATEBLOCKS)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_GET_PEER_LIST)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_GET_PUBLIC_NODES)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_SET_LOG_HASH_RATE)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_SET_LOG_LEVEL)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_SET_LOG_CATEGORIES)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_GET_TRANSACTION_POOL)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_GET_TRANSACTION_POOL_HASHES_BIN)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_GET_TRANSACTION_POOL_HASHES)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_GET_TRANSACTION_POOL_STATS)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_SET_BOOTSTRAP_DAEMON)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_STOP_DAEMON)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_GET_LIMIT)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_SET_LIMIT)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_OUT_PEERS)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_IN_PEERS)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_UPDATE)

  namespace
  {
    std::vector<std::uint64_t> decompress_integer_array(const epee::span<const std::uint8_t> s)
    {
      std::vector<std::uint64_t> v;
      v.reserve(s.size());
      auto end = s.end();
      for (auto i = s.begin(); i != end;)
      {
        std::uint64_t val{};
        const int read = tools::read_varint(i, end, val);
        CHECK_AND_ASSERT_THROW_MES(read > 0 && read <= 256, "Error decompressing data");
        v.push_back(val);
      }
      return v;
    }
    epee::byte_slice compress_integer_array(const std::vector<std::uint64_t>& v)
    {
      epee::byte_stream s;
      s.reserve(v.size() * (sizeof(std::uint64_t) * 8 / 7 + 1));
      for (const std::uint64_t t: v)
        tools::write_varint(std::back_inserter(s), t);
      return epee::byte_slice{std::move(s)};
    }

    enum class is_blob { false_ = 0, true_ };
    void read_bytes(wire::json_reader& source, std::pair<std::vector<std::uint64_t>, is_blob>& dest)
    {
      static constexpr const unsigned mb20 = (20 * 1024 * 1024) / sizeof(std::uint64_t);
      dest.second = is_blob::false_;
      wire_read::array_unchecked(source, dest.first, 0, mb20);
    }
    void read_bytes(wire::epee_reader& source, std::pair<std::vector<std::uint64_t>, is_blob>& dest)
    {
      if ((source.last_tag() & SERIALIZE_TYPE_STRING) == SERIALIZE_TYPE_STRING)
      {
        wire_read::bytes(source, wire::array_as_blob(std::ref(dest.first)));
        dest.second = is_blob::true_;
      }
      else
      {
        using element_min = wire::min_element_sizeof<std::uint64_t>;
        wire_read::array(source, dest.first, element_min{});
        dest.second = is_blob::false_;
      }
    }
    template<typename W>
    void write_bytes(W& dest, const std::pair<const std::vector<std::uint64_t>&, is_blob> source)
    {
      if (source.second == is_blob::true_)
      {
        if (!std::is_same<W, wire::epee_writer>())
          WIRE_DLOG_THROW(wire::error::schema::binary, "Blob format only supported with epee format");
        wire_write::bytes(dest, wire::array_as_blob(std::cref(source.first)));
      }
      else
        wire_write::array(dest, source.first);
    }

    template<typename F, typename T, typename U>
    void output_distribution_map(F& format, T& self, boost::optional<epee::byte_slice>& compressed, U& binary)
    {
      wire::object(format,
        WIRE_FIELD(amount),
        wire::field("start_height", std::ref(self.data.start_height)),
        WIRE_FIELD(binary),
        WIRE_FIELD(compress),
        wire::optional_field("compressed_data", std::ref(compressed)),
        wire::optional_field("distribution", std::ref(binary)),
        wire::field("base", std::ref(self.data.base))
      );
    }
  } // anonymous

  template<typename R>
  static void read_bytes(R& source, COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::distribution& dest)
  {
    boost::optional<epee::byte_slice> compressed;
    boost::optional<std::pair<std::vector<std::uint64_t>, is_blob>> binary;
    output_distribution_map(source, dest, compressed, binary);

    if (compressed && dest.binary && dest.compress)
      dest.data.distribution = decompress_integer_array(epee::to_span(*compressed));
    else if (binary && bool(binary->second) == dest.binary && !dest.compress)
      dest.data.distribution = std::move(binary->first);
    else
      dest.data.distribution.clear();
  }
  template<typename W>
  static void write_bytes(W& dest, const COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::distribution& source)
  {
    if (!std::is_same<W, wire::epee_writer>() && (source.binary || source.compress))
      WIRE_DLOG_THROW(wire::error::schema::binary, "Compressed and binary data only supported with epee format");

    boost::optional<epee::byte_slice> compressed;
    boost::optional<std::pair<const std::vector<std::uint64_t>&, is_blob>> binary;
    if (source.binary && source.compress && !source.data.distribution.empty())
      compressed = compress_integer_array(source.data.distribution);
    else if (!source.data.distribution.empty())
      binary.emplace(source.data.distribution, is_blob(source.binary));

    output_distribution_map(dest, source, compressed, binary);
  }
  WIRE_EPEE_DEFINE_COMMAND(COMMAND_RPC_GET_OUTPUT_DISTRIBUTION)
  WIRE_JSON_DEFINE_COMMAND(COMMAND_RPC_POP_BLOCKS)
} // cryptonote

namespace epee { namespace json_rpc
{
  // GETBLOCKCOUNT and GETBLOCKHASH now use the same request
  using simple_static_vector = boost::container::static_vector<uint64_t, 1>;
  WIRE_JSON_DEFINE_CONVERSION(client_request<simple_static_vector>);
  WIRE_JSON_DEFINE_CONVERSION(request_specific<simple_static_vector>);

  WIRE_JSON_DEFINE_CONVERSION(response<cryptonote::COMMAND_RPC_GETBLOCKCOUNT::response>)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_GET_INFO)
  // GETBLOCKHASH::response uses std::string which has default behavior in epee
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_GETBLOCKTEMPLATE)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_GETMINERDATA)
  WIRE_JSON_DEFINE_CONVERSION(client_request<cryptonote::COMMAND_RPC_CALCPOW::request>)
  WIRE_JSON_DEFINE_CONVERSION(request_specific<cryptonote::COMMAND_RPC_CALCPOW::request>)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_ADD_AUX_POW)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_SUBMITBLOCK)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_GENERATEBLOCKS)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_GET_LAST_BLOCK_HEADER)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_GET_BLOCK)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_HARD_FORK_INFO)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_GETBANS)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_SETBANS)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_BANNED)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_FLUSH_TRANSACTION_POOL)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_GET_OUTPUT_HISTOGRAM)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL_BACKLOG)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_GET_CONNECTIONS)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_GET_BLOCK_HEADERS_RANGE)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_GET_VERSION)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_GET_COINBASE_TX_SUM)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_GET_BASE_FEE_ESTIMATE)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_GET_ALTERNATE_CHAINS)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_RELAY_TX)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_SYNC_INFO)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_GET_OUTPUT_DISTRIBUTION)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_ACCESS_INFO)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_ACCESS_SUBMIT_NONCE)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_ACCESS_PAY)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_ACCESS_TRACKING)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_ACCESS_DATA)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_ACCESS_ACCOUNT)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_PRUNE_BLOCKCHAIN)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_FLUSH_CACHE)
  EPEE_JSONRPC_DEFINE(cryptonote::COMMAND_RPC_GET_TXIDS_LOOSE)
}} // epee // jsonrpc
