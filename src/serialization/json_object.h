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

#pragma once

#include "string_tools.h"
#include "rapidjson/document.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "rpc/message_data_structs.h"
#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "common/sfinae_helpers.h"

#define OBJECT_HAS_MEMBER_OR_THROW(val, key) \
  do \
  { \
    if (!val.HasMember(key)) \
    { \
      throw cryptonote::json::MISSING_KEY(key); \
    } \
  } while (0);

#define INSERT_INTO_JSON_OBJECT(jsonVal, doc, key, source) \
    rapidjson::Value key##Val; \
    cryptonote::json::toJsonValue(doc, source, key##Val); \
    jsonVal.AddMember(#key, key##Val, doc.GetAllocator());

#define GET_FROM_JSON_OBJECT(source, dst, key) \
    OBJECT_HAS_MEMBER_OR_THROW(source, #key) \
    decltype(dst) dstVal##key; \
    cryptonote::json::fromJsonValue(source[#key], dstVal##key); \
    dst = dstVal##key;

namespace cryptonote
{

namespace json
{

struct JSON_ERROR : public std::exception
{
  protected:
    JSON_ERROR() { }
    std::string m;

  public:
    virtual ~JSON_ERROR() { }

    const char* what() const throw()
    {
      return m.c_str();
    }
};

struct MISSING_KEY : public JSON_ERROR
{
  MISSING_KEY(const char* key)
  {
    m = std::string("Key \"") + key + "\" missing from object.";
  }
};

struct WRONG_TYPE : public JSON_ERROR
{
  WRONG_TYPE(const char* type)
  {
    m = std::string("Json value has incorrect type, expected: ") + type;
  }
};

struct BAD_INPUT : public JSON_ERROR
{
  BAD_INPUT()
  {
    m = "An item failed to convert from json object to native object";
  }
};

struct PARSE_FAIL : public JSON_ERROR
{
  PARSE_FAIL()
  {
    m = "Failed to parse the json request";
  }
};

template<typename Type>
inline constexpr bool is_to_hex()
{
  return std::is_pod<Type>() && !std::is_integral<Type>();
}


// POD to json value
template <class Type>
typename std::enable_if<is_to_hex<Type>()>::type toJsonValue(rapidjson::Document& doc, const Type& pod, rapidjson::Value& value)
{
  value = rapidjson::Value(epee::string_tools::pod_to_hex(pod).c_str(), doc.GetAllocator());
}

template <class Type>
typename std::enable_if<is_to_hex<Type>()>::type fromJsonValue(const rapidjson::Value& val, Type& t)
{
  if (!val.IsString())
  {
    throw WRONG_TYPE("string");
  }

  //TODO: handle failure to convert hex string to POD type
  bool success = epee::string_tools::hex_to_pod(val.GetString(), t);

  if (!success)
  {
    throw BAD_INPUT();
  }
}

void toJsonValue(rapidjson::Document& doc, const std::string& i, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, std::string& str);

void toJsonValue(rapidjson::Document& doc, bool i, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, bool& b);

// integers overloads for toJsonValue are not needed for standard promotions

void fromJsonValue(const rapidjson::Value& val, unsigned char& i);

void fromJsonValue(const rapidjson::Value& val, signed char& i);

void fromJsonValue(const rapidjson::Value& val, char& i);

void fromJsonValue(const rapidjson::Value& val, unsigned short& i);

void fromJsonValue(const rapidjson::Value& val, short& i);

void toJsonValue(rapidjson::Document& doc, const unsigned i, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, unsigned& i);

void toJsonValue(rapidjson::Document& doc, const int, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, int& i);


void toJsonValue(rapidjson::Document& doc, const unsigned long long i, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, unsigned long long& i);

void toJsonValue(rapidjson::Document& doc, const long long i, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, long long& i);

inline void toJsonValue(rapidjson::Document& doc, const unsigned long i, rapidjson::Value& val) {
    toJsonValue(doc, static_cast<unsigned long long>(i), val);
}
void fromJsonValue(const rapidjson::Value& val, unsigned long& i);

inline void toJsonValue(rapidjson::Document& doc, const long i, rapidjson::Value& val) {
    toJsonValue(doc, static_cast<long long>(i), val);
}
void fromJsonValue(const rapidjson::Value& val, long& i);

// end integers

void toJsonValue(rapidjson::Document& doc, const cryptonote::transaction& tx, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::transaction& tx);

void toJsonValue(rapidjson::Document& doc, const cryptonote::block& b, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::block& b);

void toJsonValue(rapidjson::Document& doc, const cryptonote::txin_v& txin, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::txin_v& txin);

void toJsonValue(rapidjson::Document& doc, const cryptonote::txin_gen& txin, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::txin_gen& txin);

void toJsonValue(rapidjson::Document& doc, const cryptonote::txin_to_script& txin, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::txin_to_script& txin);

void toJsonValue(rapidjson::Document& doc, const cryptonote::txin_to_scripthash& txin, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::txin_to_scripthash& txin);

void toJsonValue(rapidjson::Document& doc, const cryptonote::txin_to_key& txin, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::txin_to_key& txin);

void toJsonValue(rapidjson::Document& doc, const cryptonote::txout_target_v& txout, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::txout_target_v& txout);

void toJsonValue(rapidjson::Document& doc, const cryptonote::txout_to_script& txout, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::txout_to_script& txout);

void toJsonValue(rapidjson::Document& doc, const cryptonote::txout_to_scripthash& txout, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::txout_to_scripthash& txout);

void toJsonValue(rapidjson::Document& doc, const cryptonote::txout_to_key& txout, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::txout_to_key& txout);

void toJsonValue(rapidjson::Document& doc, const cryptonote::tx_out& txout, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::tx_out& txout);

void toJsonValue(rapidjson::Document& doc, const cryptonote::connection_info& info, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::connection_info& info);

void toJsonValue(rapidjson::Document& doc, const cryptonote::block_complete_entry& blk, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::block_complete_entry& blk);

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::block_with_transactions& blk, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::block_with_transactions& blk);

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::transaction_info& tx_info, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::transaction_info& tx_info);

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::output_key_and_amount_index& out, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::output_key_and_amount_index& out);

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::amount_with_random_outputs& out, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::amount_with_random_outputs& out);

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::peer& peer, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::peer& peer);

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::tx_in_pool& tx, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::tx_in_pool& tx);

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::hard_fork_info& info, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::hard_fork_info& info);

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::output_amount_count& out, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::output_amount_count& out);

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::output_amount_and_index& out, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::output_amount_and_index& out);

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::output_key_mask_unlocked& out, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::output_key_mask_unlocked& out);

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::error& err, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::error& error);

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::BlockHeaderResponse& response, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::BlockHeaderResponse& response);

void toJsonValue(rapidjson::Document& doc, const rct::rctSig& i, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& i, rct::rctSig& sig);

void toJsonValue(rapidjson::Document& doc, const rct::ecdhTuple& tuple, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, rct::ecdhTuple& tuple);

void toJsonValue(rapidjson::Document& doc, const rct::rangeSig& sig, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, rct::rangeSig& sig);

void toJsonValue(rapidjson::Document& doc, const rct::Bulletproof& p, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, rct::Bulletproof& p);

void toJsonValue(rapidjson::Document& doc, const rct::boroSig& sig, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, rct::boroSig& sig);

void toJsonValue(rapidjson::Document& doc, const rct::mgSig& sig, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, rct::mgSig& sig);

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::DaemonInfo& info, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::DaemonInfo& info);

void toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::output_distribution& dist, rapidjson::Value& val);
void fromJsonValue(const rapidjson::Value& val, cryptonote::rpc::output_distribution& dist);

template <typename Map>
typename std::enable_if<sfinae::is_map_like<Map>::value, void>::type toJsonValue(rapidjson::Document& doc, const Map& map, rapidjson::Value& val);

template <typename Map>
typename std::enable_if<sfinae::is_map_like<Map>::value, void>::type fromJsonValue(const rapidjson::Value& val, Map& map);

template <typename Vec>
typename std::enable_if<sfinae::is_vector_like<Vec>::value, void>::type toJsonValue(rapidjson::Document& doc, const Vec &vec, rapidjson::Value& val);

template <typename Vec>
typename std::enable_if<sfinae::is_vector_like<Vec>::value, void>::type fromJsonValue(const rapidjson::Value& val, Vec& vec);


// ideally would like to have the below functions in the .cpp file, but
// unfortunately because of how templates work they have to be here.

template <typename Map>
typename std::enable_if<sfinae::is_map_like<Map>::value, void>::type toJsonValue(rapidjson::Document& doc, const Map& map, rapidjson::Value& val)
{
  val.SetObject();

  auto& al = doc.GetAllocator();

  for (const auto& i : map)
  {
    rapidjson::Value k;
    rapidjson::Value m;
    toJsonValue(doc, i.first, k);
    toJsonValue(doc, i.second, m);
    val.AddMember(k, m, al);
  }
}

template <typename Map>
typename std::enable_if<sfinae::is_map_like<Map>::value, void>::type fromJsonValue(const rapidjson::Value& val, Map& map)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  auto itr = val.MemberBegin();

  while (itr != val.MemberEnd())
  {
    typename Map::key_type k;
    typename Map::mapped_type m;
    fromJsonValue(itr->name, k);
    fromJsonValue(itr->value, m);
    map.emplace(k, m);
    ++itr;
  }
}

template <typename Vec>
typename std::enable_if<sfinae::is_vector_like<Vec>::value, void>::type toJsonValue(rapidjson::Document& doc, const Vec &vec, rapidjson::Value& val)
{
  val.SetArray();

  for (const auto& t : vec)
  {
    rapidjson::Value v;
    toJsonValue(doc, t, v);
    val.PushBack(v, doc.GetAllocator());
  }
}

template <typename Vec>
typename std::enable_if<sfinae::is_vector_like<Vec>::value, void>::type fromJsonValue(const rapidjson::Value& val, Vec& vec)
{
  if (!val.IsArray())
  {
    throw WRONG_TYPE("json array");
  }

  for (rapidjson::SizeType i=0; i < val.Size(); i++)
  {
    typename Vec::value_type v;
    fromJsonValue(val[i], v);
    vec.push_back(v);
  }
}

}  // namespace json

}  // namespace cryptonote
