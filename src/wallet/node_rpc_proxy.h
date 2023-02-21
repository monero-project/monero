// Copyright (c) 2017-2022, The Monero Project
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

#include <string>

#include "connection_settings.h"
#include "cryptonote_basic/blobdatatype.h"
#include "include_base_utils.h"
#include "net/http.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "storages/http_abstract_invoke.h"

namespace
{
template <class Res> void clear_status(Res& res);
}

namespace tools
{
class NodeRPCProxy
{
public:
  class InvokeMgmt
  {
  public:
    operator bool() const { return m_returned; }

  private:
    bool m_returned;
    friend class NodeRPCProxy;
  };

  NodeRPCProxy();

  void invalidate();
  bool is_offline() const;
  void set_offline(bool offline = true);
  bool set_daemon(connection_settings&& conn_setts);
  uint64_t get_bytes_sent() const;
  uint64_t get_bytes_received() const;
  bool try_connection_start(bool* using_ssl = nullptr);
  const connection_settings& get_connection_settings() const;

  boost::optional<std::string> get_rpc_version(uint32_t &rpc_version, std::vector<std::pair<uint8_t, uint64_t>> &daemon_hard_forks, uint64_t &height, uint64_t &target_height);
  boost::optional<std::string> get_height(uint64_t &height);
  void set_height(uint64_t h);
  boost::optional<std::string> get_target_height(uint64_t &height);
  boost::optional<std::string> get_block_weight_limit(uint64_t &block_weight_limit);
  boost::optional<std::string> get_adjusted_time(uint64_t &adjusted_time);
  boost::optional<std::string> get_earliest_height(uint8_t version, uint64_t &earliest_height);
  boost::optional<std::string> get_dynamic_base_fee_estimate(uint64_t grace_blocks, uint64_t &fee);
  boost::optional<std::string> get_dynamic_base_fee_estimate_2021_scaling(uint64_t grace_blocks, std::vector<uint64_t> &fees);
  boost::optional<std::string> get_fee_quantization_mask(uint64_t &fee_quantization_mask);

  template <class Req, class Res>
  InvokeMgmt invoke_http_json
  (
    const std::string& uri,
    Req& req,
    Res& res,
    std::chrono::milliseconds timeout = default_timeout
  )
  {
    InvokeMgmt im;
    clear_status(res);
    im.m_returned = epee::net_utils::invoke_http_json(uri, req, res, m_http_client, timeout);
    return im;
  }

  template <class Req, class Res>
  InvokeMgmt invoke_http_json_rpc
  (
    const std::string& rpc_method,
    Req& req,
    Res& res,
    epee::json_rpc::error& error,
    std::chrono::milliseconds timeout = default_timeout
  )
  {
    InvokeMgmt im;
    clear_status(res);
    im.m_returned = epee::net_utils::invoke_http_json_rpc("/json_rpc", rpc_method, req, res, error,
      m_http_client, timeout);
    return im;
  }

  template <class Req, class Res>
  InvokeMgmt invoke_http_bin
  (
    const std::string& uri,
    Req& req,
    Res& res,
    std::chrono::milliseconds timeout = default_timeout
  )
  {
    InvokeMgmt im;
    clear_status(res);
    im.m_returned = epee::net_utils::invoke_http_bin(uri, req, res, m_http_client, timeout);
    return im;
  }

private:
  boost::optional<std::string> get_info();

  std::string sanitize_daemon_message(const std::string& msg) const;

  static std::chrono::milliseconds default_timeout;

  net::http::client m_http_client;
  connection_settings m_connection_settings;
  bool m_offline;

  uint64_t m_height;
  uint64_t m_earliest_height[256];
  uint64_t m_dynamic_base_fee_estimate;
  uint64_t m_dynamic_base_fee_estimate_cached_height;
  uint64_t m_dynamic_base_fee_estimate_grace_blocks;
  std::vector<uint64_t> m_dynamic_base_fee_estimate_vector;
  uint64_t m_fee_quantization_mask;
  uint64_t m_adjusted_time;
  uint32_t m_rpc_version;
  uint64_t m_target_height;
  uint64_t m_block_weight_limit;
  time_t m_get_info_time;
  time_t m_height_time;
  time_t m_target_height_time;
  std::vector<std::pair<uint8_t, uint64_t>> m_daemon_hard_forks;
};

} // namespace tools

namespace
{
// credits to yrp (https://stackoverflow.com/questions/87372/check-if-a-class-has-a-member-function-of-a-given-signature
template <class Res>
struct Statusable
{
  template<typename U, std::string (U::*)> struct SFINAE {};
  template<typename U> static char Test(SFINAE<U, &U::status>*);
  template<typename U> static int Test(...);
  static constexpr bool is_rpc_resp_derived = std::is_base_of<cryptonote::rpc_response_base, Res>();
  static constexpr bool has_status_member = sizeof(Test<Res>(0)) == sizeof(char);
  static constexpr bool value = is_rpc_resp_derived || has_status_member;
  using type = std::integral_constant<bool, value>;
};

template <class Res>
void clear_status(Res& res, std::true_type)
{
  res.status.clear();
}

template <class Res>
void clear_status(Res& res, std::false_type)
{}

template <class Res>
void clear_status(Res& res)
{
  return clear_status(res, typename Statusable<Res>::type());
}
} // anonymous namespace
