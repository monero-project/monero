// Copyright (c) 2016-2018, The Monero Project
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

#include "json_object.h"

#include <boost/range/adaptor/transformed.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <limits>
#include <type_traits>
#include "string_tools.h"

namespace cryptonote
{

namespace json
{

namespace
{
  template<typename Source, typename Destination>
  constexpr bool precision_loss()
  {
    return
      std::numeric_limits<Destination>::is_signed != std::numeric_limits<Source>::is_signed ||
      std::numeric_limits<Destination>::min() > std::numeric_limits<Source>::min() ||
      std::numeric_limits<Destination>::max() < std::numeric_limits<Source>::max();
  }

  template<typename Source, typename Type>
  void convert_numeric(Source source, Type& i)
  {
    static_assert(
      (std::is_same<Type, char>() && std::is_same<Source, int>()) ||
      std::numeric_limits<Source>::is_signed == std::numeric_limits<Type>::is_signed,
      "comparisons below may have undefined behavior"
    );
    if (source < std::numeric_limits<Type>::min())
    {
      throw WRONG_TYPE{"numeric underflow"};
    }
    if (std::numeric_limits<Type>::max() < source)
    {
      throw WRONG_TYPE{"numeric overflow"};
    }
    i = Type(source);
  }

  template<typename Type>
  void to_int(const rapidjson::Value& val, Type& i)
  {
    if (!val.IsInt())
    {
      throw WRONG_TYPE{"integer"};
    }
    convert_numeric(val.GetInt(), i);
  }
  template<typename Type>
  void to_int64(const rapidjson::Value& val, Type& i)
  {
    if (!val.IsInt64())
    {
      throw WRONG_TYPE{"integer"};
    }
    convert_numeric(val.GetInt64(), i);
  }

  template<typename Type>
  void to_uint(const rapidjson::Value& val, Type& i)
  {
    if (!val.IsUint())
    {
      throw WRONG_TYPE{"unsigned integer"};
    }
    convert_numeric(val.GetUint(), i);
  }
  template<typename Type>
  void to_uint64(const rapidjson::Value& val, Type& i)
  {
    if (!val.IsUint64())
    {
      throw WRONG_TYPE{"unsigned integer"};
    }
    convert_numeric(val.GetUint64(), i);
  }
}

void toJsonValue(rapidjson::Document& doc, const std::string& i, rapidjson::Value& val)
{
  val = rapidjson::Value(i.c_str(), doc.GetAllocator());
}

void fromJsonValue(const rapidjson::Value& val, std::string& str)
{
  if (!val.IsString())
  {
    throw WRONG_TYPE("string");
  }

  str = val.GetString();
}

void toJsonValue(rapidjson::Document& doc, bool i, rapidjson::Value& val)
{
  val.SetBool(i);
}

void fromJsonValue(const rapidjson::Value& val, bool& b)
{
  if (!val.IsBool())
  {
    throw WRONG_TYPE("boolean");
  }
  b = val.GetBool();
}

void fromJsonValue(const rapidjson::Value& val, unsigned char& i)
{
  to_uint(val, i);
}

void fromJsonValue(const rapidjson::Value& val, char& i)
{
  to_int(val, i);
}

void fromJsonValue(const rapidjson::Value& val, signed char& i)
{
  to_int(val, i);
}

void fromJsonValue(const rapidjson::Value& val, unsigned short& i)
{
  to_uint(val, i);
}

void fromJsonValue(const rapidjson::Value& val, short& i)
{
  to_int(val, i);
}

void toJsonValue(rapidjson::Document& doc, const unsigned int i, rapidjson::Value& val)
{
  val = rapidjson::Value(i);
}

void fromJsonValue(const rapidjson::Value& val, unsigned int& i)
{
  to_uint(val, i);
}

void toJsonValue(rapidjson::Document& doc, const int i, rapidjson::Value& val)
{
  val = rapidjson::Value(i);
}

void fromJsonValue(const rapidjson::Value& val, int& i)
{
  to_int(val, i);
}

void toJsonValue(rapidjson::Document& doc, const unsigned long long i, rapidjson::Value& val)
{
  static_assert(!precision_loss<unsigned long long, std::uint64_t>(), "precision loss");
  val = rapidjson::Value(std::uint64_t(i));
}

void fromJsonValue(const rapidjson::Value& val, unsigned long long& i)
{
  to_uint64(val, i);
}

void toJsonValue(rapidjson::Document& doc, const long long i, rapidjson::Value& val)
{
  static_assert(!precision_loss<long long, std::int64_t>(), "precision loss");
  val = rapidjson::Value(std::int64_t(i));
}

void fromJsonValue(const rapidjson::Value& val, long long& i)
{
  to_int64(val, i);
}

void fromJsonValue(const rapidjson::Value& val, unsigned long& i)
{
  to_uint64(val, i);
}

void fromJsonValue(const rapidjson::Value& val, long& i)
{
  to_int64(val, i);
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::transaction& tx, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, version, tx.version);
  INSERT_INTO_JSON_OBJECT(val, doc, unlock_time, tx.unlock_time);
  INSERT_INTO_JSON_OBJECT(val, doc, inputs, tx.vin);
  INSERT_INTO_JSON_OBJECT(val, doc, outputs, tx.vout);
  INSERT_INTO_JSON_OBJECT(val, doc, extra, tx.extra);
  INSERT_INTO_JSON_OBJECT(val, doc, signatures, tx.signatures);
  INSERT_INTO_JSON_OBJECT(val, doc, ringct, tx.rct_signatures);
}


void fromJsonValue(const rapidjson::Value& val, cryptonote::transaction& tx)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, tx.version, version);
  GET_FROM_JSON_OBJECT(val, tx.unlock_time, unlock_time);
  GET_FROM_JSON_OBJECT(val, tx.vin, inputs);
  GET_FROM_JSON_OBJECT(val, tx.vout, outputs);
  GET_FROM_JSON_OBJECT(val, tx.extra, extra);
  GET_FROM_JSON_OBJECT(val, tx.signatures, signatures);
  GET_FROM_JSON_OBJECT(val, tx.rct_signatures, ringct);
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::block& b, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, major_version, b.major_version);
  INSERT_INTO_JSON_OBJECT(val, doc, minor_version, b.minor_version);
  INSERT_INTO_JSON_OBJECT(val, doc, timestamp, b.timestamp);
  INSERT_INTO_JSON_OBJECT(val, doc, prev_id, b.prev_id);
  INSERT_INTO_JSON_OBJECT(val, doc, nonce, b.nonce);
  INSERT_INTO_JSON_OBJECT(val, doc, miner_tx, b.miner_tx);
  INSERT_INTO_JSON_OBJECT(val, doc, tx_hashes, b.tx_hashes);
}


void fromJsonValue(const rapidjson::Value& val, cryptonote::block& b)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, b.major_version, major_version);
  GET_FROM_JSON_OBJECT(val, b.minor_version, minor_version);
  GET_FROM_JSON_OBJECT(val, b.timestamp, timestamp);
  GET_FROM_JSON_OBJECT(val, b.prev_id, prev_id);
  GET_FROM_JSON_OBJECT(val, b.nonce, nonce);
  GET_FROM_JSON_OBJECT(val, b.miner_tx, miner_tx);
  GET_FROM_JSON_OBJECT(val, b.tx_hashes, tx_hashes);
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::txin_v& txin, rapidjson::Value& val)
{
  val.SetObject();

  struct add_input
  {
    using result_type = void;

    rapidjson::Document& doc;
    rapidjson::Value& val;

    void operator()(cryptonote::txin_to_key const& input) const
    {
      INSERT_INTO_JSON_OBJECT(val, doc, to_key, input);
    }
    void operator()(cryptonote::txin_gen const& input) const
    {
      INSERT_INTO_JSON_OBJECT(val, doc, gen, input);
    }
    void operator()(cryptonote::txin_to_script const& input) const
    {
      INSERT_INTO_JSON_OBJECT(val, doc, to_script, input);
    }
    void operator()(cryptonote::txin_to_scripthash const& input) const
    {
      INSERT_INTO_JSON_OBJECT(val, doc, to_scripthash, input);
    }
  };
  boost::apply_visitor(add_input{doc, val}, txin);
}


void fromJsonValue(const rapidjson::Value& val, cryptonote::txin_v& txin)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  if (val.MemberCount() != 1)
  {
    throw MISSING_KEY("Invalid input object");
  }

  for (auto const& elem : val.GetObject())
  {
    if (elem.name == "to_key")
    {
      cryptonote::txin_to_key tmpVal;
      fromJsonValue(elem.value, tmpVal);
      txin = std::move(tmpVal);
    }
    else if (elem.name == "gen")
    {
      cryptonote::txin_gen tmpVal;
      fromJsonValue(elem.value, tmpVal);
      txin = std::move(tmpVal);
    }
    else if (elem.name == "to_script")
    {
      cryptonote::txin_to_script tmpVal;
      fromJsonValue(elem.value, tmpVal);
      txin = std::move(tmpVal);
    }
    else if (elem.name == "to_scripthash")
    {
      cryptonote::txin_to_scripthash tmpVal;
      fromJsonValue(elem.value, tmpVal);
      txin = std::move(tmpVal);
    }
  }
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::txin_gen& txin, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, height, txin.height);
}


void fromJsonValue(const rapidjson::Value& val, cryptonote::txin_gen& txin)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, txin.height, height);
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::txin_to_script& txin, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, prev, txin.prev);
  INSERT_INTO_JSON_OBJECT(val, doc, prevout, txin.prevout);
  INSERT_INTO_JSON_OBJECT(val, doc, sigset, txin.sigset);
}


void fromJsonValue(const rapidjson::Value& val, cryptonote::txin_to_script& txin)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, txin.prev, prev);
  GET_FROM_JSON_OBJECT(val, txin.prevout, prevout);
  GET_FROM_JSON_OBJECT(val, txin.sigset, sigset);
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::txin_to_scripthash& txin, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, prev, txin.prev);
  INSERT_INTO_JSON_OBJECT(val, doc, prevout, txin.prevout);
  INSERT_INTO_JSON_OBJECT(val, doc, script, txin.script);
  INSERT_INTO_JSON_OBJECT(val, doc, sigset, txin.sigset);
}


void fromJsonValue(const rapidjson::Value& val, cryptonote::txin_to_scripthash& txin)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, txin.prev, prev);
  GET_FROM_JSON_OBJECT(val, txin.prevout, prevout);
  GET_FROM_JSON_OBJECT(val, txin.script, script);
  GET_FROM_JSON_OBJECT(val, txin.sigset, sigset);
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::txin_to_key& txin, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, amount, txin.amount);
  INSERT_INTO_JSON_OBJECT(val, doc, key_offsets, txin.key_offsets);
  INSERT_INTO_JSON_OBJECT(val, doc, key_image, txin.k_image);
}


void fromJsonValue(const rapidjson::Value& val, cryptonote::txin_to_key& txin)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, txin.amount, amount);
  GET_FROM_JSON_OBJECT(val, txin.key_offsets, key_offsets);
  GET_FROM_JSON_OBJECT(val, txin.k_image, key_image);
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::txout_to_script& txout, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, keys, txout.keys);
  INSERT_INTO_JSON_OBJECT(val, doc, script, txout.script);
}


void fromJsonValue(const rapidjson::Value& val, cryptonote::txout_to_script& txout)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, txout.keys, keys);
  GET_FROM_JSON_OBJECT(val, txout.script, script);
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::txout_to_scripthash& txout, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, hash, txout.hash);
}


void fromJsonValue(const rapidjson::Value& val, cryptonote::txout_to_scripthash& txout)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, txout.hash, hash);
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::txout_to_key& txout, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, key, txout.key);
}


void fromJsonValue(const rapidjson::Value& val, cryptonote::txout_to_key& txout)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, txout.key, key);
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::tx_out& txout, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, amount, txout.amount);

  struct add_output
  {
    using result_type = void;

    rapidjson::Document& doc;
    rapidjson::Value& val;

    void operator()(cryptonote::txout_to_key const& output) const
    {
      INSERT_INTO_JSON_OBJECT(val, doc, to_key, output);
    }
    void operator()(cryptonote::txout_to_script const& output) const
    {
      INSERT_INTO_JSON_OBJECT(val, doc, to_script, output);
    }
    void operator()(cryptonote::txout_to_scripthash const& output) const
    {
      INSERT_INTO_JSON_OBJECT(val, doc, to_scripthash, output);
    }
  };
  boost::apply_visitor(add_output{doc, val}, txout.target);
}

void fromJsonValue(const rapidjson::Value& val, cryptonote::tx_out& txout)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  if (val.MemberCount() != 2)
  {
    throw MISSING_KEY("Invalid input object");
  }

  for (auto const& elem : val.GetObject())
  {
    if (elem.name == "amount")
    {
      fromJsonValue(elem.value, txout.amount);
    }

    if (elem.name == "to_key")
    {
      cryptonote::txout_to_key tmpVal;
      fromJsonValue(elem.value, tmpVal);
      txout.target = std::move(tmpVal);
    }
    else if (elem.name == "to_script")
    {
      cryptonote::txout_to_script tmpVal;
      fromJsonValue(elem.value, tmpVal);
      txout.target = std::move(tmpVal);
    }
    else if (elem.name == "to_scripthash")
    {
      cryptonote::txout_to_scripthash tmpVal;
      fromJsonValue(elem.value, tmpVal);
      txout.target = std::move(tmpVal);
    }
  }
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::connection_info& info, rapidjson::Value& val)
{
  val.SetObject();

  auto& al = doc.GetAllocator();
  INSERT_INTO_JSON_OBJECT(val, doc, incoming, info.incoming);
  INSERT_INTO_JSON_OBJECT(val, doc, localhost, info.localhost);
  INSERT_INTO_JSON_OBJECT(val, doc, local_ip, info.local_ip);

  INSERT_INTO_JSON_OBJECT(val, doc, ip, info.ip);
  INSERT_INTO_JSON_OBJECT(val, doc, port, info.port);

  INSERT_INTO_JSON_OBJECT(val, doc, peer_id, info.peer_id);

  INSERT_INTO_JSON_OBJECT(val, doc, recv_count, info.recv_count);
  INSERT_INTO_JSON_OBJECT(val, doc, recv_idle_time, info.recv_idle_time);

  INSERT_INTO_JSON_OBJECT(val, doc, send_count, info.send_count);
  INSERT_INTO_JSON_OBJECT(val, doc, send_idle_time, info.send_idle_time);

  INSERT_INTO_JSON_OBJECT(val, doc, state, info.state);

  INSERT_INTO_JSON_OBJECT(val, doc, live_time, info.live_time);

  INSERT_INTO_JSON_OBJECT(val, doc, avg_download, info.avg_download);
  INSERT_INTO_JSON_OBJECT(val, doc, current_download, info.current_download);

  INSERT_INTO_JSON_OBJECT(val, doc, avg_upload, info.avg_upload);
  INSERT_INTO_JSON_OBJECT(val, doc, current_upload, info.current_upload);
}


void fromJsonValue(const rapidjson::Value& val, cryptonote::connection_info& info)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, info.incoming, incoming);
  GET_FROM_JSON_OBJECT(val, info.localhost, localhost);
  GET_FROM_JSON_OBJECT(val, info.local_ip, local_ip);

  GET_FROM_JSON_OBJECT(val, info.ip, ip);
  GET_FROM_JSON_OBJECT(val, info.port, port);

  GET_FROM_JSON_OBJECT(val, info.peer_id, peer_id);

  GET_FROM_JSON_OBJECT(val, info.recv_count, recv_count);
  GET_FROM_JSON_OBJECT(val, info.recv_idle_time, recv_idle_time);

  GET_FROM_JSON_OBJECT(val, info.send_count, send_count);
  GET_FROM_JSON_OBJECT(val, info.send_idle_time, send_idle_time);

  GET_FROM_JSON_OBJECT(val, info.state, state);

  GET_FROM_JSON_OBJECT(val, info.live_time, live_time);

  GET_FROM_JSON_OBJECT(val, info.avg_download, avg_download);
  GET_FROM_JSON_OBJECT(val, info.current_download, current_download);

  GET_FROM_JSON_OBJECT(val, info.avg_upload, avg_upload);
  GET_FROM_JSON_OBJECT(val, info.current_upload, current_upload);
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::block_complete_entry& blk, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, block, blk.block);
  INSERT_INTO_JSON_OBJECT(val, doc, transactions, blk.txs);
}


void fromJsonValue(const rapidjson::Value& val, cryptonote::block_complete_entry& blk)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, blk.block, block);
  GET_FROM_JSON_OBJECT(val, blk.txs, transactions);
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::block_with_transactions& blk, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, block, blk.block);
  INSERT_INTO_JSON_OBJECT(val, doc, transactions, blk.transactions);
}


void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::block_with_transactions& blk)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, blk.block, block);
  GET_FROM_JSON_OBJECT(val, blk.transactions, transactions);
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::transaction_info& tx_info, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, height, tx_info.height);
  INSERT_INTO_JSON_OBJECT(val, doc, in_pool, tx_info.in_pool);
  INSERT_INTO_JSON_OBJECT(val, doc, transaction, tx_info.transaction);
}


void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::transaction_info& tx_info)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, tx_info.height, height);
  GET_FROM_JSON_OBJECT(val, tx_info.in_pool, in_pool);
  GET_FROM_JSON_OBJECT(val, tx_info.transaction, transaction);
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::output_key_and_amount_index& out, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, amount_index, out.amount_index);
  INSERT_INTO_JSON_OBJECT(val, doc, key, out.key);
}


void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::output_key_and_amount_index& out)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, out.amount_index, amount_index);
  GET_FROM_JSON_OBJECT(val, out.key, key);
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::amount_with_random_outputs& out, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, amount, out.amount);
  INSERT_INTO_JSON_OBJECT(val, doc, outputs, out.outputs);
}


void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::amount_with_random_outputs& out)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, out.amount, amount);
  GET_FROM_JSON_OBJECT(val, out.outputs, outputs);
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::peer& peer, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, id, peer.id);
  INSERT_INTO_JSON_OBJECT(val, doc, ip, peer.ip);
  INSERT_INTO_JSON_OBJECT(val, doc, port, peer.port);
  INSERT_INTO_JSON_OBJECT(val, doc, last_seen, peer.last_seen);
}


void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::peer& peer)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, peer.id, id);
  GET_FROM_JSON_OBJECT(val, peer.ip, ip);
  GET_FROM_JSON_OBJECT(val, peer.port, port);
  GET_FROM_JSON_OBJECT(val, peer.last_seen, last_seen);
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::tx_in_pool& tx, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, tx, tx.tx);
  INSERT_INTO_JSON_OBJECT(val, doc, tx_hash, tx.tx_hash);
  INSERT_INTO_JSON_OBJECT(val, doc, blob_size, tx.blob_size);
  INSERT_INTO_JSON_OBJECT(val, doc, weight, tx.weight);
  INSERT_INTO_JSON_OBJECT(val, doc, fee, tx.fee);
  INSERT_INTO_JSON_OBJECT(val, doc, max_used_block_hash, tx.max_used_block_hash);
  INSERT_INTO_JSON_OBJECT(val, doc, max_used_block_height, tx.max_used_block_height);
  INSERT_INTO_JSON_OBJECT(val, doc, kept_by_block, tx.kept_by_block);
  INSERT_INTO_JSON_OBJECT(val, doc, last_failed_block_hash, tx.last_failed_block_hash);
  INSERT_INTO_JSON_OBJECT(val, doc, last_failed_block_height, tx.last_failed_block_height);
  INSERT_INTO_JSON_OBJECT(val, doc, receive_time, tx.receive_time);
  INSERT_INTO_JSON_OBJECT(val, doc, last_relayed_time, tx.last_relayed_time);
  INSERT_INTO_JSON_OBJECT(val, doc, relayed, tx.relayed);
  INSERT_INTO_JSON_OBJECT(val, doc, do_not_relay, tx.do_not_relay);
  INSERT_INTO_JSON_OBJECT(val, doc, double_spend_seen, tx.double_spend_seen);
}


void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::tx_in_pool& tx)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, tx.tx, tx);
  GET_FROM_JSON_OBJECT(val, tx.blob_size, blob_size);
  GET_FROM_JSON_OBJECT(val, tx.weight, weight);
  GET_FROM_JSON_OBJECT(val, tx.fee, fee);
  GET_FROM_JSON_OBJECT(val, tx.max_used_block_hash, max_used_block_hash);
  GET_FROM_JSON_OBJECT(val, tx.max_used_block_height, max_used_block_height);
  GET_FROM_JSON_OBJECT(val, tx.kept_by_block, kept_by_block);
  GET_FROM_JSON_OBJECT(val, tx.last_failed_block_hash, last_failed_block_hash);
  GET_FROM_JSON_OBJECT(val, tx.last_failed_block_height, last_failed_block_height);
  GET_FROM_JSON_OBJECT(val, tx.receive_time, receive_time);
  GET_FROM_JSON_OBJECT(val, tx.last_relayed_time, last_relayed_time);
  GET_FROM_JSON_OBJECT(val, tx.relayed, relayed);
  GET_FROM_JSON_OBJECT(val, tx.do_not_relay, do_not_relay);
  GET_FROM_JSON_OBJECT(val, tx.double_spend_seen, double_spend_seen);
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::hard_fork_info& info, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, version, info.version);
  INSERT_INTO_JSON_OBJECT(val, doc, enabled, info.enabled);
  INSERT_INTO_JSON_OBJECT(val, doc, window, info.window);
  INSERT_INTO_JSON_OBJECT(val, doc, votes, info.votes);
  INSERT_INTO_JSON_OBJECT(val, doc, threshold, info.threshold);
  INSERT_INTO_JSON_OBJECT(val, doc, voting, info.voting);
  INSERT_INTO_JSON_OBJECT(val, doc, state, info.state);
  INSERT_INTO_JSON_OBJECT(val, doc, earliest_height, info.earliest_height);
}


void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::hard_fork_info& info)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, info.version, version);
  GET_FROM_JSON_OBJECT(val, info.enabled, enabled);
  GET_FROM_JSON_OBJECT(val, info.window, window);
  GET_FROM_JSON_OBJECT(val, info.votes, votes);
  GET_FROM_JSON_OBJECT(val, info.threshold, threshold);
  GET_FROM_JSON_OBJECT(val, info.voting, voting);
  GET_FROM_JSON_OBJECT(val, info.state, state);
  GET_FROM_JSON_OBJECT(val, info.earliest_height, earliest_height);
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::output_amount_count& out, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, amount, out.amount);
  INSERT_INTO_JSON_OBJECT(val, doc, total_count, out.total_count);
  INSERT_INTO_JSON_OBJECT(val, doc, unlocked_count, out.unlocked_count);
  INSERT_INTO_JSON_OBJECT(val, doc, recent_count, out.recent_count);
}


void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::output_amount_count& out)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, out.amount, amount);
  GET_FROM_JSON_OBJECT(val, out.total_count, total_count);
  GET_FROM_JSON_OBJECT(val, out.unlocked_count, unlocked_count);
  GET_FROM_JSON_OBJECT(val, out.recent_count, recent_count);
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::output_amount_and_index& out, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, amount, out.amount);
  INSERT_INTO_JSON_OBJECT(val, doc, index, out.index);
}


void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::output_amount_and_index& out)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, out.amount, amount);
  GET_FROM_JSON_OBJECT(val, out.index, index);
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::output_key_mask_unlocked& out, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, key, out.key);
  INSERT_INTO_JSON_OBJECT(val, doc, mask, out.mask);
  INSERT_INTO_JSON_OBJECT(val, doc, unlocked, out.unlocked);
}

void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::output_key_mask_unlocked& out)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, out.key, key);
  GET_FROM_JSON_OBJECT(val, out.mask, mask);
  GET_FROM_JSON_OBJECT(val, out.unlocked, unlocked);
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::error& err, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, code, err.code);
  INSERT_INTO_JSON_OBJECT(val, doc, error_str, err.error_str);
  INSERT_INTO_JSON_OBJECT(val, doc, message, err.message);
}

void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::error& error)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, error.code, code);
  GET_FROM_JSON_OBJECT(val, error.error_str, error_str);
  GET_FROM_JSON_OBJECT(val, error.message, message);
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::BlockHeaderResponse& response, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, major_version, response.major_version);
  INSERT_INTO_JSON_OBJECT(val, doc, minor_version, response.minor_version);
  INSERT_INTO_JSON_OBJECT(val, doc, timestamp, response.timestamp);
  INSERT_INTO_JSON_OBJECT(val, doc, prev_id, response.prev_id);
  INSERT_INTO_JSON_OBJECT(val, doc, nonce, response.nonce);
  INSERT_INTO_JSON_OBJECT(val, doc, height, response.height);
  INSERT_INTO_JSON_OBJECT(val, doc, depth, response.depth);
  INSERT_INTO_JSON_OBJECT(val, doc, hash, response.hash);
  INSERT_INTO_JSON_OBJECT(val, doc, difficulty, response.difficulty);
  INSERT_INTO_JSON_OBJECT(val, doc, reward, response.reward);
}

void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::BlockHeaderResponse& response)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, response.major_version, major_version);
  GET_FROM_JSON_OBJECT(val, response.minor_version, minor_version);
  GET_FROM_JSON_OBJECT(val, response.timestamp, timestamp);
  GET_FROM_JSON_OBJECT(val, response.prev_id, prev_id);
  GET_FROM_JSON_OBJECT(val, response.nonce, nonce);
  GET_FROM_JSON_OBJECT(val, response.height, height);
  GET_FROM_JSON_OBJECT(val, response.depth, depth);
  GET_FROM_JSON_OBJECT(val, response.hash, hash);
  GET_FROM_JSON_OBJECT(val, response.difficulty, difficulty);
  GET_FROM_JSON_OBJECT(val, response.reward, reward);
}

void toJsonValue(rapidjson::Document& doc, const rct::rctSig& sig, rapidjson::Value& val)
{
  using boost::adaptors::transform;

  val.SetObject();

  const auto just_mask = [] (rct::ctkey const& key) -> rct::key const&
  {
    return key.mask;
  };

  INSERT_INTO_JSON_OBJECT(val, doc, type, sig.type);
  INSERT_INTO_JSON_OBJECT(val, doc, encrypted, sig.ecdhInfo);
  INSERT_INTO_JSON_OBJECT(val, doc, commitments, transform(sig.outPk, just_mask));
  INSERT_INTO_JSON_OBJECT(val, doc, fee, sig.txnFee);

  // prunable
  {
    rapidjson::Value prunable;
    prunable.SetObject();

    INSERT_INTO_JSON_OBJECT(prunable, doc, range_proofs, sig.p.rangeSigs);
    INSERT_INTO_JSON_OBJECT(prunable, doc, bulletproofs, sig.p.bulletproofs);
    INSERT_INTO_JSON_OBJECT(prunable, doc, mlsags, sig.p.MGs);
    INSERT_INTO_JSON_OBJECT(prunable, doc, pseudo_outs, sig.get_pseudo_outs());

    val.AddMember("prunable", prunable, doc.GetAllocator());
  }
}

void fromJsonValue(const rapidjson::Value& val, rct::rctSig& sig)
{
  using boost::adaptors::transform;

  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  std::vector<rct::key> commitments;

  GET_FROM_JSON_OBJECT(val, sig.type, type);
  GET_FROM_JSON_OBJECT(val, sig.ecdhInfo, encrypted);
  GET_FROM_JSON_OBJECT(val, commitments, commitments);
  GET_FROM_JSON_OBJECT(val, sig.txnFee, fee);

  // prunable
  {
    OBJECT_HAS_MEMBER_OR_THROW(val, "prunable");
    const auto& prunable = val["prunable"];

    rct::keyV pseudo_outs;

    GET_FROM_JSON_OBJECT(prunable, sig.p.rangeSigs, range_proofs);
    GET_FROM_JSON_OBJECT(prunable, sig.p.bulletproofs, bulletproofs);
    GET_FROM_JSON_OBJECT(prunable, sig.p.MGs, mlsags);
    GET_FROM_JSON_OBJECT(prunable, pseudo_outs, pseudo_outs);

    sig.get_pseudo_outs() = std::move(pseudo_outs);
  }

  sig.outPk.reserve(commitments.size());
  for (rct::key const& commitment : commitments)
  {
    sig.outPk.push_back({{}, commitment});
  }
}

void toJsonValue(rapidjson::Document& doc, const rct::ecdhTuple& tuple, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, mask, tuple.mask);
  INSERT_INTO_JSON_OBJECT(val, doc, amount, tuple.amount);
}

void fromJsonValue(const rapidjson::Value& val, rct::ecdhTuple& tuple)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, tuple.mask, mask);
  GET_FROM_JSON_OBJECT(val, tuple.amount, amount);
}

void toJsonValue(rapidjson::Document& doc, const rct::rangeSig& sig, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, asig, sig.asig);

  std::vector<rct::key> keyVector(sig.Ci, std::end(sig.Ci));
  INSERT_INTO_JSON_OBJECT(val, doc, Ci, keyVector);
}

void fromJsonValue(const rapidjson::Value& val, rct::rangeSig& sig)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  const auto ci = val.FindMember("Ci");
  if (ci == val.MemberEnd())
  {
    throw MISSING_KEY("Ci");
  }

  GET_FROM_JSON_OBJECT(val, sig.asig, asig);

  std::vector<rct::key> keyVector;
  cryptonote::json::fromJsonValue(ci->value, keyVector);
  if (!(keyVector.size() == 64))
  {
    throw WRONG_TYPE("key64 (rct::key[64])");
  }
  for (size_t i=0; i < 64; i++)
  {
    sig.Ci[i] = keyVector[i];
  }
}

void toJsonValue(rapidjson::Document& doc, const rct::Bulletproof& p, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, V, p.V);
  INSERT_INTO_JSON_OBJECT(val, doc, A, p.A);
  INSERT_INTO_JSON_OBJECT(val, doc, S, p.S);
  INSERT_INTO_JSON_OBJECT(val, doc, T1, p.T1);
  INSERT_INTO_JSON_OBJECT(val, doc, T2, p.T2);
  INSERT_INTO_JSON_OBJECT(val, doc, taux, p.taux);
  INSERT_INTO_JSON_OBJECT(val, doc, mu, p.mu);
  INSERT_INTO_JSON_OBJECT(val, doc, L, p.L);
  INSERT_INTO_JSON_OBJECT(val, doc, R, p.R);
  INSERT_INTO_JSON_OBJECT(val, doc, a, p.a);
  INSERT_INTO_JSON_OBJECT(val, doc, b, p.b);
  INSERT_INTO_JSON_OBJECT(val, doc, t, p.t);
}

void fromJsonValue(const rapidjson::Value& val, rct::Bulletproof& p)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, p.V, V);
  GET_FROM_JSON_OBJECT(val, p.A, A);
  GET_FROM_JSON_OBJECT(val, p.S, S);
  GET_FROM_JSON_OBJECT(val, p.T1, T1);
  GET_FROM_JSON_OBJECT(val, p.T2, T2);
  GET_FROM_JSON_OBJECT(val, p.taux, taux);
  GET_FROM_JSON_OBJECT(val, p.mu, mu);
  GET_FROM_JSON_OBJECT(val, p.L, L);
  GET_FROM_JSON_OBJECT(val, p.R, R);
  GET_FROM_JSON_OBJECT(val, p.a, a);
  GET_FROM_JSON_OBJECT(val, p.b, b);
  GET_FROM_JSON_OBJECT(val, p.t, t);
}

void toJsonValue(rapidjson::Document& doc, const rct::boroSig& sig, rapidjson::Value& val)
{
  val.SetObject();

  std::vector<rct::key> keyVector(sig.s0, std::end(sig.s0));
  INSERT_INTO_JSON_OBJECT(val, doc, s0, keyVector);

  keyVector.assign(sig.s1, std::end(sig.s1));
  INSERT_INTO_JSON_OBJECT(val, doc, s1, keyVector);

  INSERT_INTO_JSON_OBJECT(val, doc, ee, sig.ee);
}

void fromJsonValue(const rapidjson::Value& val, rct::boroSig& sig)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  OBJECT_HAS_MEMBER_OR_THROW(val, "s0")
  std::vector<rct::key> keyVector;
  cryptonote::json::fromJsonValue(val["s0"], keyVector);
  if (!(keyVector.size() == 64))
  {
    throw WRONG_TYPE("key64 (rct::key[64])");
  }
  for (size_t i=0; i < 64; i++)
  {
    sig.s0[i] = keyVector[i];
  }

  OBJECT_HAS_MEMBER_OR_THROW(val, "s1")
  keyVector.clear();
  cryptonote::json::fromJsonValue(val["s1"], keyVector);
  if (!(keyVector.size() == 64))
  {
    throw WRONG_TYPE("key64 (rct::key[64])");
  }
  for (size_t i=0; i < 64; i++)
  {
    sig.s1[i] = keyVector[i];
  }

  GET_FROM_JSON_OBJECT(val, sig.ee, ee);
}

void toJsonValue(rapidjson::Document& doc, const rct::mgSig& sig, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, ss, sig.ss);
  INSERT_INTO_JSON_OBJECT(val, doc, cc, sig.cc);
}

void fromJsonValue(const rapidjson::Value& val, rct::mgSig& sig)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("key64 (rct::key[64])");
  }

  GET_FROM_JSON_OBJECT(val, sig.ss, ss);
  GET_FROM_JSON_OBJECT(val, sig.cc, cc);
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::DaemonInfo& info, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, height, info.height);
  INSERT_INTO_JSON_OBJECT(val, doc, target_height, info.target_height);
  INSERT_INTO_JSON_OBJECT(val, doc, difficulty, info.difficulty);
  INSERT_INTO_JSON_OBJECT(val, doc, target, info.target);
  INSERT_INTO_JSON_OBJECT(val, doc, tx_count, info.tx_count);
  INSERT_INTO_JSON_OBJECT(val, doc, tx_pool_size, info.tx_pool_size);
  INSERT_INTO_JSON_OBJECT(val, doc, alt_blocks_count, info.alt_blocks_count);
  INSERT_INTO_JSON_OBJECT(val, doc, outgoing_connections_count, info.outgoing_connections_count);
  INSERT_INTO_JSON_OBJECT(val, doc, incoming_connections_count, info.incoming_connections_count);
  INSERT_INTO_JSON_OBJECT(val, doc, white_peerlist_size, info.white_peerlist_size);
  INSERT_INTO_JSON_OBJECT(val, doc, grey_peerlist_size, info.grey_peerlist_size);
  INSERT_INTO_JSON_OBJECT(val, doc, mainnet, info.mainnet);
  INSERT_INTO_JSON_OBJECT(val, doc, testnet, info.testnet);
  INSERT_INTO_JSON_OBJECT(val, doc, stagenet, info.stagenet);
  INSERT_INTO_JSON_OBJECT(val, doc, nettype, info.nettype);
  INSERT_INTO_JSON_OBJECT(val, doc, top_block_hash, info.top_block_hash);
  INSERT_INTO_JSON_OBJECT(val, doc, cumulative_difficulty, info.cumulative_difficulty);
  INSERT_INTO_JSON_OBJECT(val, doc, block_size_limit, info.block_size_limit);
  INSERT_INTO_JSON_OBJECT(val, doc, block_weight_limit, info.block_weight_limit);
  INSERT_INTO_JSON_OBJECT(val, doc, block_size_median, info.block_size_median);
  INSERT_INTO_JSON_OBJECT(val, doc, block_weight_median, info.block_weight_median);
  INSERT_INTO_JSON_OBJECT(val, doc, start_time, info.start_time);
}

void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::DaemonInfo& info)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, info.height, height);
  GET_FROM_JSON_OBJECT(val, info.target_height, target_height);
  GET_FROM_JSON_OBJECT(val, info.difficulty, difficulty);
  GET_FROM_JSON_OBJECT(val, info.target, target);
  GET_FROM_JSON_OBJECT(val, info.tx_count, tx_count);
  GET_FROM_JSON_OBJECT(val, info.tx_pool_size, tx_pool_size);
  GET_FROM_JSON_OBJECT(val, info.alt_blocks_count, alt_blocks_count);
  GET_FROM_JSON_OBJECT(val, info.outgoing_connections_count, outgoing_connections_count);
  GET_FROM_JSON_OBJECT(val, info.incoming_connections_count, incoming_connections_count);
  GET_FROM_JSON_OBJECT(val, info.white_peerlist_size, white_peerlist_size);
  GET_FROM_JSON_OBJECT(val, info.grey_peerlist_size, grey_peerlist_size);
  GET_FROM_JSON_OBJECT(val, info.mainnet, mainnet);
  GET_FROM_JSON_OBJECT(val, info.testnet, testnet);
  GET_FROM_JSON_OBJECT(val, info.stagenet, stagenet);
  GET_FROM_JSON_OBJECT(val, info.nettype, nettype);
  GET_FROM_JSON_OBJECT(val, info.top_block_hash, top_block_hash);
  GET_FROM_JSON_OBJECT(val, info.cumulative_difficulty, cumulative_difficulty);
  GET_FROM_JSON_OBJECT(val, info.block_size_limit, block_size_limit);
  GET_FROM_JSON_OBJECT(val, info.block_weight_limit, block_weight_limit);
  GET_FROM_JSON_OBJECT(val, info.block_size_median, block_size_median);
  GET_FROM_JSON_OBJECT(val, info.block_weight_median, block_weight_median);
  GET_FROM_JSON_OBJECT(val, info.start_time, start_time);
}

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::output_distribution& dist, rapidjson::Value& val)
{
  val.SetObject();

  INSERT_INTO_JSON_OBJECT(val, doc, distribution, dist.data.distribution);
  INSERT_INTO_JSON_OBJECT(val, doc, amount, dist.amount);
  INSERT_INTO_JSON_OBJECT(val, doc, start_height, dist.data.start_height);
  INSERT_INTO_JSON_OBJECT(val, doc, base, dist.data.base);
}

void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::output_distribution& dist)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  GET_FROM_JSON_OBJECT(val, dist.data.distribution, distribution);
  GET_FROM_JSON_OBJECT(val, dist.amount, amount);
  GET_FROM_JSON_OBJECT(val, dist.data.start_height, start_height);
  GET_FROM_JSON_OBJECT(val, dist.data.base, base);
}

}  // namespace json

}  // namespace cryptonote
