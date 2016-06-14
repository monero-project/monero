// Copyright (c) 2016, The Monero Project
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

#include "rapidjson/document.h"
#include "cryptonote_core/cryptonote_basic.h"
#include "cryptonote_protocol/cryptonote_protocol_defs.h"

namespace cryptonote
{

namespace json
{

// POD to json value
template <class Type>
rapidjson::Value toJsonValue(rapidjson::Document& doc, const Type& pod);

template <class Type>
Type fromJsonValue(const rapidjson::Value& val);

template <>
rapidjson::Value toJsonValue(rapidjson::Document& doc, const std::string& i);

template <>
std::string fromJsonValue(const rapidjson::Value& i);

rapidjson::Value toJsonValue(rapidjson::Document& doc, bool i);

bool fromJsonValue(const rapidjson::Value& i);

template <>
rapidjson::Value toJsonValue(rapidjson::Document& doc, const uint8_t& i);

template <>
uint8_t fromJsonValue(const rapidjson::Value& i);

template <>
rapidjson::Value toJsonValue(rapidjson::Document& doc, const uint16_t& i);

template <>
uint16_t fromJsonValue(const rapidjson::Value& i);

template <>
rapidjson::Value toJsonValue(rapidjson::Document& doc, const uint32_t& i);

template <>
uint32_t fromJsonValue(const rapidjson::Value& i);

template <>
rapidjson::Value toJsonValue(rapidjson::Document& doc, const uint64_t& i);

template <>
uint64_t fromJsonValue(const rapidjson::Value& i);

template <>
rapidjson::Value toJsonValue<cryptonote::transaction>(rapidjson::Document& doc, const cryptonote::transaction& tx);

template <>
cryptonote::transaction fromJsonValue<cryptonote::transaction>(const rapidjson::Value& val);

template <>
rapidjson::Value toJsonValue<cryptonote::block>(rapidjson::Document& doc, const cryptonote::block& b);

template <>
cryptonote::block fromJsonValue<cryptonote::block>(const rapidjson::Value& val);

template <>
rapidjson::Value toJsonValue<cryptonote::txin_v>(rapidjson::Document& doc, const cryptonote::txin_v& txin);

template <>
cryptonote::txin_v fromJsonValue<cryptonote::txin_v>(const rapidjson::Value& val);

template <>
rapidjson::Value toJsonValue<cryptonote::txin_gen>(rapidjson::Document& doc, const cryptonote::txin_gen& txin);

template <>
cryptonote::txin_gen fromJsonValue<cryptonote::txin_gen>(const rapidjson::Value& val);

template <>
rapidjson::Value toJsonValue<cryptonote::txin_to_script>(rapidjson::Document& doc, const cryptonote::txin_to_script& txin);

template <>
cryptonote::txin_to_script fromJsonValue<cryptonote::txin_to_script>(const rapidjson::Value& val);

template <>
rapidjson::Value toJsonValue<cryptonote::txin_to_scripthash>(rapidjson::Document& doc, const cryptonote::txin_to_scripthash& txin);

template <>
cryptonote::txin_to_scripthash fromJsonValue<cryptonote::txin_to_scripthash>(const rapidjson::Value& val);

template <>
rapidjson::Value toJsonValue<cryptonote::txin_to_key>(rapidjson::Document& doc, const cryptonote::txin_to_key& txin);

template <>
cryptonote::txin_to_key fromJsonValue<cryptonote::txin_to_key>(const rapidjson::Value& val);

template <>
rapidjson::Value toJsonValue<cryptonote::txout_target_v>(rapidjson::Document& doc, const cryptonote::txout_target_v& txout);

template <>
cryptonote::txout_target_v fromJsonValue<cryptonote::txout_target_v>(const rapidjson::Value& val);

template <>
rapidjson::Value toJsonValue<cryptonote::txout_to_script>(rapidjson::Document& doc, const cryptonote::txout_to_script& txout);

template <>
cryptonote::txout_to_script fromJsonValue<cryptonote::txout_to_script>(const rapidjson::Value& val);

template <>
rapidjson::Value toJsonValue<cryptonote::txout_to_scripthash>(rapidjson::Document& doc, const cryptonote::txout_to_scripthash& txout);

template <>
cryptonote::txout_to_scripthash fromJsonValue<cryptonote::txout_to_scripthash>(const rapidjson::Value& val);

template <>
rapidjson::Value toJsonValue<cryptonote::txout_to_key>(rapidjson::Document& doc, const cryptonote::txout_to_key& txout);

template <>
cryptonote::txout_to_key fromJsonValue<cryptonote::txout_to_key>(const rapidjson::Value& val);

template <>
rapidjson::Value toJsonValue<cryptonote::tx_out>(rapidjson::Document& doc, const cryptonote::tx_out& txout);

template <>
cryptonote::tx_out fromJsonValue<cryptonote::tx_out>(const rapidjson::Value& val);

template <>
rapidjson::Value toJsonValue<cryptonote::connection_info>(rapidjson::Document& doc, const cryptonote::connection_info& info);

template <>
cryptonote::connection_info fromJsonValue<cryptonote::connection_info>(const rapidjson::Value& val);

template <>
rapidjson::Value toJsonValue<cryptonote::block_complete_entry>(rapidjson::Document& doc, const cryptonote::block_complete_entry& blk);

template <>
cryptonote::block_complete_entry fromJsonValue<cryptonote::block_complete_entry>(const rapidjson::Value& val);

template <>
rapidjson::Value toJsonValue<cryptonote::block_with_transactions>(rapidjson::Document& doc, const cryptonote::block_with_transactions& blk);

template <>
cryptonote::block_with_transactions fromJsonValue<cryptonote::block_with_transactions>(const rapidjson::Value& val);


// ideally would like to have the below functions in the .cpp file, but
// unfortunately because of how templates work they have to be here.

template <class Key, class Value, class Map = std::unordered_map<Key, Value> >
rapidjson::Value toJsonValue(rapidjson::Document& doc, const Map& val)
{
  rapidjson::Value obj(rapidjson::kObjectType);

  auto& al = doc.GetAllocator();

  for (auto& i : val)
  {
    obj.AddMember(toJsonValue<Key>(doc, i.first), toJsonValue<Value>(doc, i.second), al);
  }

  return obj;
}

template <class Key, class Value, class Map = std::unordered_map<Key, Value> >
Map fromJsonValue(const rapidjson::Value& val)
{
  Map retmap;

  auto itr = val.MemberBegin();

  while (itr != val.MemberEnd())
  {
    retmap.emplace(fromJsonValue<Key>(itr->name), fromJsonValue<Value>(itr->value));
    ++itr;
  }

  return retmap;
}

template <class Type, template <typename, typename=std::allocator<Type> > class Container>
rapidjson::Value toJsonValue(rapidjson::Document& doc, const Container<Type>& vec)
{
  rapidjson::Value arr(rapidjson::kArrayType);

  for (const Type& t : vec)
  {
    arr.PushBack(toJsonValue(doc, t), doc.GetAllocator());
  }

  return arr;
}

template <class Type, template <typename, typename=std::allocator<Type> > class Container>
Container<Type> fromJsonValue(const rapidjson::Value& val)
{
  Container<Type> vec;

  for (rapidjson::SizeType i=0; i < val.Size(); i++)
  {
    vec.push_back(fromJsonValue<Type>(val[i]));
  }

  return vec;
}

template <class Type,
          template <typename, typename=std::allocator<Type> > class Inner,
          template <typename, typename=std::allocator<Inner<Type> > > class Outer>
Outer<Inner<Type>> fromJsonValue(const rapidjson::Value& val)
{
  Outer<Inner<Type> > vec;

  for (rapidjson::SizeType i=0; i < val.Size(); i++)
  {
    vec.push_back(fromJsonValue<Type, Inner>(val[i]));
  }

  return vec;
}






}  // namespace json

}  // namespace cryptonote
