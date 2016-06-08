#ifndef CRYPTONOTE_AS_JSON_H
#define CRYPTONOTE_AS_JSON_H

#include "rapidjson/document.h"
#include "cryptonote_core/cryptonote_basic.h"

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

template <class Type, template <typename, typename=std::allocator<Type> > class Container>
rapidjson::Value toJsonValue(rapidjson::Document& doc, const Container<Type>& vec);

template <class Type, template <typename, typename=std::allocator<Type> > class Container>
Container<Type> fromJsonValue(const rapidjson::Value& val);

template <class Type,
          template <typename, typename=std::allocator<Type> > class Inner,
          template <typename, typename=std::allocator<Inner<Type> > > class Outer>
Outer<Inner<Type> > fromJsonValue(const rapidjson::Value& val);

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









}  // namespace json

}  // namespace cryptonote

#endif  // CRYPTONOTE_AS_JSON_H
