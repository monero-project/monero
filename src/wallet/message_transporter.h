// Copyright (c) 2018-2022, The Monero Project

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
#include "serialization/keyvalue_serialization.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/cryptonote_boost_serialization.h"
#include "cryptonote_basic/account_boost_serialization.h"
#include "net/http_server_impl_base.h"
#include "net/http_client.h"
#include "net/abstract_http_client.h"
#include "common/util.h"
#include "wipeable_string.h"
#include <vector>

namespace mms
{

struct transport_message_t
{
  cryptonote::account_public_address source_monero_address;
  std::string source_transport_address;
  cryptonote::account_public_address destination_monero_address;
  std::string destination_transport_address;
  crypto::chacha_iv iv;
  crypto::public_key encryption_public_key;
  uint64_t timestamp;
  uint32_t type;
  std::string subject;
  std::string content;
  crypto::hash hash;
  crypto::signature signature;
  uint32_t round;
  uint32_t signature_count;
  std::string transport_id;

  BEGIN_KV_SERIALIZE_MAP()
    KV_SERIALIZE(source_monero_address)
    KV_SERIALIZE(source_transport_address)
    KV_SERIALIZE(destination_monero_address)
    KV_SERIALIZE(destination_transport_address)
    KV_SERIALIZE_VAL_POD_AS_BLOB(iv)
    KV_SERIALIZE_VAL_POD_AS_BLOB(encryption_public_key)
    KV_SERIALIZE(timestamp)
    KV_SERIALIZE(type)
    KV_SERIALIZE(subject)
    KV_SERIALIZE(content)
    KV_SERIALIZE_VAL_POD_AS_BLOB(hash)
    KV_SERIALIZE_VAL_POD_AS_BLOB(signature)
    KV_SERIALIZE(round)
    KV_SERIALIZE(signature_count)
    KV_SERIALIZE(transport_id)
  END_KV_SERIALIZE_MAP()
};
typedef epee::misc_utils::struct_init<transport_message_t> transport_message;

class message_transporter
{
public:
  message_transporter(std::unique_ptr<epee::net_utils::http::abstract_http_client> http_client);
  void set_options(const std::string &bitmessage_address, const epee::wipeable_string &bitmessage_login);
  bool send_message(const transport_message &message);
  bool receive_messages(const std::vector<std::string> &destination_transport_addresses,
                        std::vector<transport_message> &messages);
  bool delete_message(const std::string &transport_id);
  void stop() { m_run.store(false, std::memory_order_relaxed); }
  std::string derive_transport_address(const std::string &seed);
  bool delete_transport_address(const std::string &transport_address);

private:
  const std::unique_ptr<epee::net_utils::http::abstract_http_client> m_http_client;
  std::string m_bitmessage_url;
  epee::wipeable_string m_bitmessage_login;
  std::atomic<bool> m_run;

  bool post_request(const std::string &request, std::string &answer);
  static std::string get_str_between_tags(const std::string &s, const std::string &start_delim, const std::string &stop_delim);

  static void start_xml_rpc_cmd(std::string &xml, const std::string &method_name);
  static void add_xml_rpc_string_param(std::string &xml, const std::string &param);
  static void add_xml_rpc_base64_param(std::string &xml, const std::string &param);
  static void add_xml_rpc_integer_param(std::string &xml, const int32_t &param);
  static void end_xml_rpc_cmd(std::string &xml);

};

}
