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

#include "daemon_messages.h"
#include "serialization/json_object.h"

namespace
{

inline rapidjson::Value emptyRequest()
{
  rapidjson::Value val;
  val.SetObject();

  return val;
}

}  // anonymous namespace

namespace cryptonote
{

namespace rpc
{

const char* GetHeight::name = "get_height";
const char* GetTransactions::name = "get_transactions";
const char* KeyImagesSpent::name = "key_images_spent";




rapidjson::Value GetHeight::Request::toJson(rapidjson::Document& doc)
{
  return emptyRequest();
}

void GetHeight::Request::fromJson(rapidjson::Value& val)
{
}

rapidjson::Value GetHeight::Response::toJson(rapidjson::Document& doc)
{
  rapidjson::Value val(rapidjson::kObjectType);

  auto& al = doc.GetAllocator();

  val.AddMember("height", height, al);

  return val;
}

void GetHeight::Response::fromJson(rapidjson::Value& val)
{
  height = cryptonote::json::fromJsonValue<uint64_t>(val["height"]);
}


rapidjson::Value GetBlocksFast::Request::toJson(rapidjson::Document& doc)
{
  rapidjson::Value val(rapidjson::kObjectType);

  auto& al = doc.GetAllocator();

  return val;
}

void GetBlocksFast::Request::fromJson(rapidjson::Value& val)
{
}

rapidjson::Value GetBlocksFast::Response::toJson(rapidjson::Document& doc)
{
  rapidjson::Value val(rapidjson::kObjectType);

  auto& al = doc.GetAllocator();

  return val;
}

void GetBlocksFast::Response::fromJson(rapidjson::Value& val)
{
}


rapidjson::Value GetHashesFast::Request::toJson(rapidjson::Document& doc)
{
  rapidjson::Value val(rapidjson::kObjectType);

  auto& al = doc.GetAllocator();

  return val;
}

void GetHashesFast::Request::fromJson(rapidjson::Value& val)
{
}

rapidjson::Value GetHashesFast::Response::toJson(rapidjson::Document& doc)
{
  rapidjson::Value val(rapidjson::kObjectType);

  auto& al = doc.GetAllocator();

  return val;
}

void GetHashesFast::Response::fromJson(rapidjson::Value& val)
{
}


rapidjson::Value GetTransactions::Request::toJson(rapidjson::Document& doc)
{
  rapidjson::Value val(rapidjson::kObjectType);

  auto& al = doc.GetAllocator();

  val.AddMember("tx_hashes", cryptonote::json::toJsonValue<decltype(tx_hashes)>(doc, tx_hashes), al);

  return val;
}

void GetTransactions::Request::fromJson(rapidjson::Value& val)
{
  tx_hashes = cryptonote::json::fromJsonValue<decltype(tx_hashes)>(val["tx_hashes"]);
}

rapidjson::Value GetTransactions::Response::toJson(rapidjson::Document& doc)
{
  rapidjson::Value val(rapidjson::kObjectType);

  auto& al = doc.GetAllocator();

  val.AddMember("txs",
                cryptonote::json::toJsonValue<decltype(txs)>(doc, txs),
                al);
  val.AddMember("missed_hashes",
                cryptonote::json::toJsonValue<decltype(missed_hashes)>(doc, missed_hashes),
                al);

  return val;
}

void GetTransactions::Response::fromJson(rapidjson::Value& val)
{
  txs = cryptonote::json::fromJsonValue<decltype(txs)>(val["txs"]);
  missed_hashes = cryptonote::json::fromJsonValue<decltype(missed_hashes)>(val["missed_hashes"]);
}

rapidjson::Value KeyImagesSpent::Request::toJson(rapidjson::Document& doc)
{
  rapidjson::Value val(rapidjson::kObjectType);

  auto& al = doc.GetAllocator();

  val.AddMember("key_images", cryptonote::json::toJsonValue<decltype(key_images)>(doc, key_images), al);

  return val;
}

void KeyImagesSpent::Request::fromJson(rapidjson::Value& val)
{
  key_images = cryptonote::json::fromJsonValue<decltype(key_images)>(val["key_images"]);
}

rapidjson::Value KeyImagesSpent::Response::toJson(rapidjson::Document& doc)
{
  rapidjson::Value val(rapidjson::kObjectType);

  auto& al = doc.GetAllocator();

  val.AddMember("spent_status", cryptonote::json::toJsonValue<decltype(spent_status)>(doc, spent_status), al);

  return val;
}

void KeyImagesSpent::Response::fromJson(rapidjson::Value& val)
{
  spent_status = cryptonote::json::fromJsonValue<decltype(spent_status)>(val["spent_status"]);
}


}  // namespace rpc

}  // namespace cryptonote
