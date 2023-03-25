// Copyright (c) 2014-2022, The Monero Project
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
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

// IP blocking adapted from Boolberry

#include <algorithm>
#include <boost/bind/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/optional/optional.hpp>
#include <boost/thread/thread.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string.hpp>
#include <atomic>
#include <functional>
#include <limits>
#include <memory>
#include <tuple>
#include <vector>

#include "version.h"
#include "string_tools.h"
#include "common/util.h"
#include "common/dns_utils.h"
#include "common/pruning.h"
#include "net/error.h"
#include "net/net_helper.h"
#include "math_helper.h"
#include "misc_log_ex.h"
#include "p2p_protocol_defs.h"
#include "crypto/crypto.h"
#include "storages/levin_abstract_invoke2.h"
#include "cryptonote_core/cryptonote_core.h"
#include "net/parse.h"

#include <miniupnp/miniupnpc/miniupnpc.h>
#include <miniupnp/miniupnpc/upnpcommands.h>
#include <miniupnp/miniupnpc/upnperrors.h>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.p2p"

#define NET_MAKE_IP(b1,b2,b3,b4)  ((LPARAM)(((DWORD)(b1)<<24)+((DWORD)(b2)<<16)+((DWORD)(b3)<<8)+((DWORD)(b4))))

#define MIN_WANTED_SEED_NODES 12

static inline boost::asio::ip::address_v4 make_address_v4_from_v6(const boost::asio::ip::address_v6& a)
{
  const auto &bytes = a.to_bytes();
  uint32_t v4 = 0;
  v4 = (v4 << 8) | bytes[12];
  v4 = (v4 << 8) | bytes[13];
  v4 = (v4 << 8) | bytes[14];
  v4 = (v4 << 8) | bytes[15];
  return boost::asio::ip::address_v4(v4);
}

namespace nodetool
{
  template<class t_payload_net_handler>
  node_server<t_payload_net_handler>::~node_server()
  {
    // tcp server uses io_service in destructor, and every zone uses
    // io_service from public zone.
    for (auto current = m_network_zones.begin(); current != m_network_zones.end(); /* below */)
    {
      if (current->first != epee::net_utils::zone::public_)
        current = m_network_zones.erase(current);
      else
        ++current;
    }
  }
  //-----------------------------------------------------------------------------------
  inline bool append_net_address(std::vector<epee::net_utils::network_address> & seed_nodes, std::string const & addr, uint16_t default_port);
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  void node_server<t_payload_net_handler>::init_options(boost::program_options::options_description& desc)
  {
    command_line::add_arg(desc, arg_p2p_bind_ip);
    command_line::add_arg(desc, arg_p2p_bind_ipv6_address);
    command_line::add_arg(desc, arg_p2p_bind_port, false);
    command_line::add_arg(desc, arg_p2p_bind_port_ipv6, false);
    command_line::add_arg(desc, arg_p2p_use_ipv6);
    command_line::add_arg(desc, arg_p2p_ignore_ipv4);
    command_line::add_arg(desc, arg_p2p_external_port);
    command_line::add_arg(desc, arg_p2p_allow_local_ip);
    command_line::add_arg(desc, arg_p2p_add_peer);
    command_line::add_arg(desc, arg_p2p_add_priority_node);
    command_line::add_arg(desc, arg_p2p_add_exclusive_node);
    command_line::add_arg(desc, arg_p2p_seed_node);
    command_line::add_arg(desc, arg_tx_proxy);
    command_line::add_arg(desc, arg_anonymous_inbound);
    command_line::add_arg(desc, arg_ban_list);
    command_line::add_arg(desc, arg_p2p_hide_my_port);
    command_line::add_arg(desc, arg_no_sync);
    command_line::add_arg(desc, arg_enable_dns_blocklist);
    command_line::add_arg(desc, arg_no_igd);
    command_line::add_arg(desc, arg_igd);
    command_line::add_arg(desc, arg_out_peers);
    command_line::add_arg(desc, arg_in_peers);
    command_line::add_arg(desc, arg_tos_flag);
    command_line::add_arg(desc, arg_limit_rate_up);
    command_line::add_arg(desc, arg_limit_rate_down);
    command_line::add_arg(desc, arg_limit_rate);
    command_line::add_arg(desc, arg_pad_transactions);
    command_line::add_arg(desc, arg_max_connections_per_ip);
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::init_config()
  {
    TRY_ENTRY();
    auto storage = peerlist_storage::open(m_config_folder + "/" + P2P_NET_DATA_FILENAME);
    if (storage)
      m_peerlist_storage = std::move(*storage);

    network_zone& public_zone = m_network_zones[epee::net_utils::zone::public_];
    public_zone.m_config.m_support_flags = P2P_SUPPORT_FLAGS;
    public_zone.m_config.m_peer_id = crypto::rand<uint64_t>();
    m_first_connection_maker_call = true;

    CATCH_ENTRY_L0("node_server::init_config", false);
    return true;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  void node_server<t_payload_net_handler>::for_each_connection(std::function<bool(typename t_payload_net_handler::connection_context&, peerid_type, uint32_t)> f)
  {
    for(auto& zone : m_network_zones)
    {
      zone.second.m_net_server.get_config_object().foreach_connection([&](p2p_connection_context& cntx){
        return f(cntx, cntx.peer_id, cntx.support_flags);
      });
    }
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::for_connection(const boost::uuids::uuid &connection_id, std::function<bool(typename t_payload_net_handler::connection_context&, peerid_type, uint32_t)> f)
  {
    for(auto& zone : m_network_zones)
    {
      const bool result = zone.second.m_net_server.get_config_object().for_connection(connection_id, [&](p2p_connection_context& cntx){
        return f(cntx, cntx.peer_id, cntx.support_flags);
      });
      if (result)
        return true;
    }
    return false;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::is_remote_host_allowed(const epee::net_utils::network_address &address, time_t *t)
  {
    CRITICAL_REGION_LOCAL(m_blocked_hosts_lock);

    const time_t now = time(nullptr);

    // look in the hosts list
    auto it = m_blocked_hosts.find(address.host_str());
    if (it != m_blocked_hosts.end())
    {
      if (now >= it->second)
      {
        m_blocked_hosts.erase(it);
        MCLOG_CYAN(el::Level::Info, "global", "Host " << address.host_str() << " unblocked.");
        it = m_blocked_hosts.end();
      }
      else
      {
        if (t)
          *t = it->second - now;
        return false;
      }
    }

    // manually loop in subnets
    if (address.get_type_id() == epee::net_utils::address_type::ipv4)
    {
      auto ipv4_address = address.template as<epee::net_utils::ipv4_network_address>();
      std::map<epee::net_utils::ipv4_network_subnet, time_t>::iterator it;
      for (it = m_blocked_subnets.begin(); it != m_blocked_subnets.end(); )
      {
        if (now >= it->second)
        {
          it = m_blocked_subnets.erase(it);
          MCLOG_CYAN(el::Level::Info, "global", "Subnet " << it->first.host_str() << " unblocked.");
          continue;
        }
        if (it->first.matches(ipv4_address))
        {
          if (t)
            *t = it->second - now;
          return false;
        }
        ++it;
      }
    }

    // not found in hosts or subnets, allowed
    return true;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::block_host(epee::net_utils::network_address addr, time_t seconds, bool add_only)
  {
    if(!addr.is_blockable())
      return false;

    const time_t now = time(nullptr);
    bool added = false;

    CRITICAL_REGION_LOCAL(m_blocked_hosts_lock);
    time_t limit;
    if (now > std::numeric_limits<time_t>::max() - seconds)
      limit = std::numeric_limits<time_t>::max();
    else
      limit = now + seconds;
    const std::string host_str = addr.host_str();
    auto it = m_blocked_hosts.find(host_str);
    if (it == m_blocked_hosts.end())
    {
      m_blocked_hosts[host_str] = limit;

      // if the host was already blocked due to being in a blocked subnet, let it be silent
      bool matches_blocked_subnet = false;
      if (addr.get_type_id() == epee::net_utils::address_type::ipv4)
      {
        auto ipv4_address = addr.template as<epee::net_utils::ipv4_network_address>();
        for (auto jt = m_blocked_subnets.begin(); jt != m_blocked_subnets.end(); ++jt)
        {
          if (jt->first.matches(ipv4_address))
          {
            matches_blocked_subnet = true;
            break;
          }
        }
      }
      if (!matches_blocked_subnet)
        added = true;
    }
    else if (it->second < limit || !add_only)
      it->second = limit;

    // drop any connection to that address. This should only have to look into
    // the zone related to the connection, but really make sure everything is
    // swept ...
    std::vector<boost::uuids::uuid> conns;
    for(auto& zone : m_network_zones)
    {
      zone.second.m_net_server.get_config_object().foreach_connection([&](const p2p_connection_context& cntxt)
      {
        if (cntxt.m_remote_address.is_same_host(addr))
        {
          conns.push_back(cntxt.m_connection_id);
        }
        return true;
      });

      peerlist_entry pe{};
      pe.adr = addr;
      if (addr.port() == 0)
      {
        zone.second.m_peerlist.evict_host_from_peerlist(true, pe);
        zone.second.m_peerlist.evict_host_from_peerlist(false, pe);
      }
      else
      {
        zone.second.m_peerlist.remove_from_peer_white(pe);
        zone.second.m_peerlist.remove_from_peer_gray(pe);
        zone.second.m_peerlist.remove_from_peer_anchor(addr);
     }

      for (const auto &c: conns)
        zone.second.m_net_server.get_config_object().close(c);

      conns.clear();
    }

    if (added)
      MCLOG_CYAN(el::Level::Info, "global", "Host " << host_str << " blocked.");
    else
      MINFO("Host " << host_str << " block time updated.");
    return true;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::unblock_host(const epee::net_utils::network_address &address)
  {
    CRITICAL_REGION_LOCAL(m_blocked_hosts_lock);
    auto i = m_blocked_hosts.find(address.host_str());
    if (i == m_blocked_hosts.end())
      return false;
    m_blocked_hosts.erase(i);
    MCLOG_CYAN(el::Level::Info, "global", "Host " << address.host_str() << " unblocked.");
    return true;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::block_subnet(const epee::net_utils::ipv4_network_subnet &subnet, time_t seconds)
  {
    const time_t now = time(nullptr);

    CRITICAL_REGION_LOCAL(m_blocked_hosts_lock);
    time_t limit;
    if (now > std::numeric_limits<time_t>::max() - seconds)
      limit = std::numeric_limits<time_t>::max();
    else
      limit = now + seconds;
    const bool added = m_blocked_subnets.find(subnet) == m_blocked_subnets.end();
    m_blocked_subnets[subnet] = limit;

    // drop any connection to that subnet. This should only have to look into
    // the zone related to the connection, but really make sure everything is
    // swept ...
    std::vector<boost::uuids::uuid> conns;
    for(auto& zone : m_network_zones)
    {
      zone.second.m_net_server.get_config_object().foreach_connection([&](const p2p_connection_context& cntxt)
      {
        if (cntxt.m_remote_address.get_type_id() != epee::net_utils::ipv4_network_address::get_type_id())
          return true;
        auto ipv4_address = cntxt.m_remote_address.template as<epee::net_utils::ipv4_network_address>();
        if (subnet.matches(ipv4_address))
        {
          conns.push_back(cntxt.m_connection_id);
        }
        return true;
      });
      for (const auto &c: conns)
        zone.second.m_net_server.get_config_object().close(c);

      for (int i = 0; i < 2; ++i)
        zone.second.m_peerlist.filter(i == 0, [&subnet](const peerlist_entry &pe){
          if (pe.adr.get_type_id() != epee::net_utils::ipv4_network_address::get_type_id())
            return false;
          return subnet.matches(pe.adr.as<const epee::net_utils::ipv4_network_address>());
        });

      conns.clear();
    }

    if (added)
      MCLOG_CYAN(el::Level::Info, "global", "Subnet " << subnet.host_str() << " blocked.");
    else
      MINFO("Subnet " << subnet.host_str() << " blocked.");
    return true;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::unblock_subnet(const epee::net_utils::ipv4_network_subnet &subnet)
  {
    CRITICAL_REGION_LOCAL(m_blocked_hosts_lock);
    auto i = m_blocked_subnets.find(subnet);
    if (i == m_blocked_subnets.end())
      return false;
    m_blocked_subnets.erase(i);
    MCLOG_CYAN(el::Level::Info, "global", "Subnet " << subnet.host_str() << " unblocked.");
    return true;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::add_host_fail(const epee::net_utils::network_address &address, unsigned int score)
  {
    if(!address.is_blockable())
      return false;

    CRITICAL_REGION_LOCAL(m_host_fails_score_lock);
    uint64_t fails = m_host_fails_score[address.host_str()] += score;
    MDEBUG("Host " << address.host_str() << " fail score=" << fails);
    if(fails > P2P_IP_FAILS_BEFORE_BLOCK)
    {
      auto it = m_host_fails_score.find(address.host_str());
      CHECK_AND_ASSERT_MES(it != m_host_fails_score.end(), false, "internal error");
      it->second = P2P_IP_FAILS_BEFORE_BLOCK/2;
      block_host(address);
    }
    return true;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::handle_command_line(
      const boost::program_options::variables_map& vm
    )
  {
    bool testnet = command_line::get_arg(vm, cryptonote::arg_testnet_on);
    bool stagenet = command_line::get_arg(vm, cryptonote::arg_stagenet_on);
    const bool pad_txs = command_line::get_arg(vm, arg_pad_transactions);
    m_nettype = testnet ? cryptonote::TESTNET : stagenet ? cryptonote::STAGENET : cryptonote::MAINNET;

    network_zone& public_zone = m_network_zones[epee::net_utils::zone::public_];
    public_zone.m_connect = &public_connect;
    public_zone.m_bind_ip = command_line::get_arg(vm, arg_p2p_bind_ip);
    public_zone.m_bind_ipv6_address = command_line::get_arg(vm, arg_p2p_bind_ipv6_address);
    public_zone.m_port = command_line::get_arg(vm, arg_p2p_bind_port);
    public_zone.m_port_ipv6 = command_line::get_arg(vm, arg_p2p_bind_port_ipv6);
    public_zone.m_can_pingback = true;
    m_external_port = command_line::get_arg(vm, arg_p2p_external_port);
    m_allow_local_ip = command_line::get_arg(vm, arg_p2p_allow_local_ip);
    const bool has_no_igd = command_line::get_arg(vm, arg_no_igd);
    const std::string sigd = command_line::get_arg(vm, arg_igd);
    if (sigd == "enabled")
    {
      if (has_no_igd)
      {
        MFATAL("Cannot have both --" << arg_no_igd.name << " and --" << arg_igd.name << " enabled");
        return false;
      }
      m_igd = igd;
    }
    else if (sigd == "disabled")
    {
      m_igd =  no_igd;
    }
    else if (sigd == "delayed")
    {
      if (has_no_igd && !command_line::is_arg_defaulted(vm, arg_igd))
      {
        MFATAL("Cannot have both --" << arg_no_igd.name << " and --" << arg_igd.name << " delayed");
        return false;
      }
      m_igd = has_no_igd ? no_igd : delayed_igd;
    }
    else
    {
      MFATAL("Invalid value for --" << arg_igd.name << ", expected enabled, disabled or delayed");
      return false;
    }
    m_offline = command_line::get_arg(vm, cryptonote::arg_offline);
    m_use_ipv6 = command_line::get_arg(vm, arg_p2p_use_ipv6);
    m_require_ipv4 = !command_line::get_arg(vm, arg_p2p_ignore_ipv4);
    public_zone.m_notifier = cryptonote::levin::notify{
      public_zone.m_net_server.get_io_service(), public_zone.m_net_server.get_config_shared(), nullptr, epee::net_utils::zone::public_, pad_txs, m_payload_handler.get_core()
    };

    if (command_line::has_arg(vm, arg_p2p_add_peer))
    {
      std::vector<std::string> perrs = command_line::get_arg(vm, arg_p2p_add_peer);
      for(const std::string& pr_str: perrs)
      {
        nodetool::peerlist_entry pe = AUTO_VAL_INIT(pe);
        pe.id = crypto::rand<uint64_t>();
        const uint16_t default_port = cryptonote::get_config(m_nettype).P2P_DEFAULT_PORT;
        expect<epee::net_utils::network_address> adr = net::get_network_address(pr_str, default_port);
        if (adr)
        {
          add_zone(adr->get_zone());
          pe.adr = std::move(*adr);
          m_command_line_peers.push_back(std::move(pe));
          continue;
        }
        CHECK_AND_ASSERT_MES(
          adr == net::error::unsupported_address, false, "Bad address (\"" << pr_str << "\"): " << adr.error().message()
        );

        std::vector<epee::net_utils::network_address> resolved_addrs;
        bool r = append_net_address(resolved_addrs, pr_str, default_port);
        CHECK_AND_ASSERT_MES(r, false, "Failed to parse or resolve address from string: " << pr_str);
        for (const epee::net_utils::network_address& addr : resolved_addrs)
        {
          pe.id = crypto::rand<uint64_t>();
          pe.adr = addr;
          m_command_line_peers.push_back(pe);
        }
      }
    }

    if (command_line::has_arg(vm,arg_p2p_add_exclusive_node))
    {
      if (!parse_peers_and_add_to_container(vm, arg_p2p_add_exclusive_node, m_exclusive_peers))
        return false;
    }

    if (command_line::has_arg(vm, arg_p2p_add_priority_node))
    {
      if (!parse_peers_and_add_to_container(vm, arg_p2p_add_priority_node, m_priority_peers))
        return false;
    }

    if (command_line::has_arg(vm, arg_p2p_seed_node))
    {
      boost::unique_lock<boost::shared_mutex> lock(public_zone.m_seed_nodes_lock);

      if (!parse_peers_and_add_to_container(vm, arg_p2p_seed_node, public_zone.m_seed_nodes))
        return false;
    }

    if (!command_line::is_arg_defaulted(vm, arg_ban_list))
    {
      const std::string ban_list = command_line::get_arg(vm, arg_ban_list);

      const boost::filesystem::path ban_list_path(ban_list);
      boost::system::error_code ec;
      if (!boost::filesystem::exists(ban_list_path, ec))
      {
        throw std::runtime_error("Can't find ban list file " + ban_list + " - " + ec.message());
      }

      std::string banned_ips;
      if (!epee::file_io_utils::load_file_to_string(ban_list_path.string(), banned_ips))
      {
        throw std::runtime_error("Failed to read ban list file " + ban_list);
      }

      std::istringstream iss(banned_ips);
      for (std::string line; std::getline(iss, line); )
      {
        auto subnet = net::get_ipv4_subnet_address(line);
        if (subnet)
        {
          block_subnet(*subnet, std::numeric_limits<time_t>::max());
          continue;
        }
        const expect<epee::net_utils::network_address> parsed_addr = net::get_network_address(line, 0);
        if (parsed_addr)
        {
          block_host(*parsed_addr, std::numeric_limits<time_t>::max());
          continue;
        }
        MERROR("Invalid IP address or IPv4 subnet: " << line);
      }
    }

    if(command_line::has_arg(vm, arg_p2p_hide_my_port))
      m_hide_my_port = true;

    if (command_line::has_arg(vm, arg_no_sync))
      m_payload_handler.set_no_sync(true);

    m_enable_dns_blocklist = command_line::get_arg(vm, arg_enable_dns_blocklist);

    if ( !set_max_out_peers(public_zone, command_line::get_arg(vm, arg_out_peers) ) )
      return false;
    else
      m_payload_handler.set_max_out_peers(epee::net_utils::zone::public_, public_zone.m_config.m_net_config.max_out_connection_count);


    if ( !set_max_in_peers(public_zone, command_line::get_arg(vm, arg_in_peers) ) )
      return false;

    if ( !set_tos_flag(vm, command_line::get_arg(vm, arg_tos_flag) ) )
      return false;

    if ( !set_rate_up_limit(vm, command_line::get_arg(vm, arg_limit_rate_up) ) )
      return false;

    if ( !set_rate_down_limit(vm, command_line::get_arg(vm, arg_limit_rate_down) ) )
      return false;

    if ( !set_rate_limit(vm, command_line::get_arg(vm, arg_limit_rate) ) )
      return false;


    epee::byte_slice noise = nullptr;
    auto proxies = get_proxies(vm);
    if (!proxies)
      return false;

    for (auto& proxy : *proxies)
    {
      network_zone& zone = add_zone(proxy.zone);
      if (zone.m_connect != nullptr)
      {
        MERROR("Listed --" << arg_tx_proxy.name << " twice with " << epee::net_utils::zone_to_string(proxy.zone));
        return false;
      }
      zone.m_connect = &socks_connect;
      zone.m_proxy_address = std::move(proxy.address);

      if (!set_max_out_peers(zone, proxy.max_connections))
        return false;
      else
        m_payload_handler.set_max_out_peers(proxy.zone, proxy.max_connections);

      epee::byte_slice this_noise = nullptr;
      if (proxy.noise)
      {
        static_assert(sizeof(epee::levin::bucket_head2) < CRYPTONOTE_NOISE_BYTES, "noise bytes too small");
        if (noise.empty())
          noise = epee::levin::make_noise_notify(CRYPTONOTE_NOISE_BYTES);

        this_noise = noise.clone();
      }

      zone.m_notifier = cryptonote::levin::notify{
        zone.m_net_server.get_io_service(), zone.m_net_server.get_config_shared(), std::move(this_noise), proxy.zone, pad_txs, m_payload_handler.get_core()
      };
    }

    for (const auto& zone : m_network_zones)
    {
      if (zone.second.m_connect == nullptr)
      {
        MERROR("Set outgoing peer for " << epee::net_utils::zone_to_string(zone.first) << " but did not set --" << arg_tx_proxy.name);
        return false;
      }
    }

    auto inbounds = get_anonymous_inbounds(vm);
    if (!inbounds)
      return false;

    const std::size_t tx_relay_zones = m_network_zones.size();
    for (auto& inbound : *inbounds)
    {
      network_zone& zone = add_zone(inbound.our_address.get_zone());

      if (!zone.m_bind_ip.empty())
      {
        MERROR("Listed --" << arg_anonymous_inbound.name << " twice with " << epee::net_utils::zone_to_string(inbound.our_address.get_zone()) << " network");
        return false;
      }

      if (zone.m_connect == nullptr && tx_relay_zones <= 1)
      {
        MERROR("Listed --" << arg_anonymous_inbound.name << " without listing any --" << arg_tx_proxy.name << ". The latter is necessary for sending local txes over anonymity networks");
        return false;
      }

      zone.m_bind_ip = std::move(inbound.local_ip);
      zone.m_port = std::move(inbound.local_port);
      zone.m_net_server.set_default_remote(std::move(inbound.default_remote));
      zone.m_our_address = std::move(inbound.our_address);

      if (!set_max_in_peers(zone, inbound.max_connections))
        return false;
    }

    max_connections = command_line::get_arg(vm, arg_max_connections_per_ip);

    return true;
  }
  //-----------------------------------------------------------------------------------
  inline bool append_net_address(
      std::vector<epee::net_utils::network_address> & seed_nodes
    , std::string const & addr
    , uint16_t default_port
    )
  {
    using namespace boost::asio;

    // Split addr string into host string and port string
    std::string host;
    std::string port = std::to_string(default_port);
    net::get_network_address_host_and_port(addr, host, port);
    MINFO("Resolving node address: host=" << host << ", port=" << port);

    io_service io_srv;
    ip::tcp::resolver resolver(io_srv);
    ip::tcp::resolver::query query(host, port, boost::asio::ip::tcp::resolver::query::canonical_name);
    boost::system::error_code ec;
    ip::tcp::resolver::iterator i = resolver.resolve(query, ec);
    CHECK_AND_ASSERT_MES(!ec, false, "Failed to resolve host name '" << host << "': " << ec.message() << ':' << ec.value());

    ip::tcp::resolver::iterator iend;
    for (; i != iend; ++i)
    {
      ip::tcp::endpoint endpoint = *i;
      if (endpoint.address().is_v4())
      {
        epee::net_utils::network_address na{epee::net_utils::ipv4_network_address{boost::asio::detail::socket_ops::host_to_network_long(endpoint.address().to_v4().to_ulong()), endpoint.port()}};
        seed_nodes.push_back(na);
        MINFO("Added node: " << na.str());
      }
      else
      {
        epee::net_utils::network_address na{epee::net_utils::ipv6_network_address{endpoint.address().to_v6(), endpoint.port()}};
        seed_nodes.push_back(na);
        MINFO("Added node: " << na.str());
      }
    }
    return true;
  }

  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  std::set<std::string> node_server<t_payload_net_handler>::get_ip_seed_nodes() const
  {
    std::set<std::string> full_addrs;
    if (m_nettype == cryptonote::TESTNET)
    {
      full_addrs.insert("176.9.0.187:28080");
      full_addrs.insert("88.99.173.38:28080");
      full_addrs.insert("51.79.173.165:28080");
      full_addrs.insert("192.99.8.110:28080");
      full_addrs.insert("37.187.74.171:28080");
    }
    else if (m_nettype == cryptonote::STAGENET)
    {
      full_addrs.insert("176.9.0.187:38080");
      full_addrs.insert("88.99.173.38:38080");
      full_addrs.insert("51.79.173.165:38080");
      full_addrs.insert("192.99.8.110:38080");
      full_addrs.insert("37.187.74.171:38080");
    }
    else if (m_nettype == cryptonote::FAKECHAIN)
    {
    }
    else
    {
      full_addrs.insert("176.9.0.187:18080");
      full_addrs.insert("88.198.163.90:18080");
      full_addrs.insert("66.85.74.134:18080");
      full_addrs.insert("88.99.173.38:18080");
      full_addrs.insert("51.79.173.165:18080");
      full_addrs.insert("192.99.8.110:18080");
      full_addrs.insert("37.187.74.171:18080");
    }
    return full_addrs;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  std::set<std::string> node_server<t_payload_net_handler>::get_dns_seed_nodes()
  {
    if (!m_exclusive_peers.empty() || m_offline)
    {
      return {};
    }
    if (m_nettype == cryptonote::TESTNET)
    {
      return get_ip_seed_nodes();
    }
    if (m_nettype == cryptonote::STAGENET)
    {
      return get_ip_seed_nodes();
    }
    if (!m_enable_dns_seed_nodes)
    {
      // TODO: a domain can be set through socks, so that the remote side does the lookup for the DNS seed nodes.
      m_fallback_seed_nodes_added.test_and_set();
      return get_ip_seed_nodes();
    }

    std::set<std::string> full_addrs;

    // for each hostname in the seed nodes list, attempt to DNS resolve and
    // add the result addresses as seed nodes
    // TODO: at some point add IPv6 support, but that won't be relevant
    // for some time yet.

    std::vector<std::vector<std::string>> dns_results;
    dns_results.resize(m_seed_nodes_list.size());

    // some libc implementation provide only a very small stack
    // for threads, e.g. musl only gives +- 80kb, which is not
    // enough to do a resolve with unbound. we request a stack
    // of 1 mb, which should be plenty
    boost::thread::attributes thread_attributes;
    thread_attributes.set_stack_size(1024*1024);

    std::list<boost::thread> dns_threads;
    uint64_t result_index = 0;
    for (const std::string& addr_str : m_seed_nodes_list)
    {
      boost::thread th = boost::thread(thread_attributes, [=, &dns_results, &addr_str]
      {
        MDEBUG("dns_threads[" << result_index << "] created for: " << addr_str);
        // TODO: care about dnssec avail/valid
        bool avail, valid;
        std::vector<std::string> addr_list;

        try
        {
          addr_list = tools::DNSResolver::instance().get_ipv4(addr_str, avail, valid);
          MDEBUG("dns_threads[" << result_index << "] DNS resolve done");
          boost::this_thread::interruption_point();
        }
        catch(const boost::thread_interrupted&)
        {
          // thread interruption request
          // even if we now have results, finish thread without setting
          // result variables, which are now out of scope in main thread
          MWARNING("dns_threads[" << result_index << "] interrupted");
          return;
        }

        MINFO("dns_threads[" << result_index << "] addr_str: " << addr_str << "  number of results: " << addr_list.size());
        dns_results[result_index] = addr_list;
      });

      dns_threads.push_back(std::move(th));
      ++result_index;
    }

    MDEBUG("dns_threads created, now waiting for completion or timeout of " << CRYPTONOTE_DNS_TIMEOUT_MS << "ms");
    boost::chrono::system_clock::time_point deadline = boost::chrono::system_clock::now() + boost::chrono::milliseconds(CRYPTONOTE_DNS_TIMEOUT_MS);
    uint64_t i = 0;
    for (boost::thread& th : dns_threads)
    {
      if (! th.try_join_until(deadline))
      {
        MWARNING("dns_threads[" << i << "] timed out, sending interrupt");
        th.interrupt();
      }
      ++i;
    }

    i = 0;
    for (const auto& result : dns_results)
    {
      MDEBUG("DNS lookup for " << m_seed_nodes_list[i] << ": " << result.size() << " results");
      // if no results for node, thread's lookup likely timed out
      if (result.size())
      {
        for (const auto& addr_string : result)
          full_addrs.insert(addr_string + ":" + std::to_string(cryptonote::get_config(m_nettype).P2P_DEFAULT_PORT));
      }
      ++i;
    }

    // append the fallback nodes if we have too few seed nodes to start with
    if (full_addrs.size() < MIN_WANTED_SEED_NODES)
    {
      if (full_addrs.empty())
        MINFO("DNS seed node lookup either timed out or failed, falling back to defaults");
      else
        MINFO("Not enough DNS seed nodes found, using fallback defaults too");

      for (const auto &peer: get_ip_seed_nodes())
        full_addrs.insert(peer);
      m_fallback_seed_nodes_added.test_and_set();
    }

    return full_addrs;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  std::set<std::string> node_server<t_payload_net_handler>::get_seed_nodes(epee::net_utils::zone zone)
  {
    switch (zone)
    {
    case epee::net_utils::zone::public_:
      return get_dns_seed_nodes();
    case epee::net_utils::zone::tor:
      if (m_nettype == cryptonote::MAINNET)
      {
        return {
          "xwvz3ekocr3dkyxfkmgm2hvbpzx2ysqmaxgter7znnqrhoicygkfswid.onion:18083",
          "4pixvbejrvihnkxmduo2agsnmc3rrulrqc7s3cbwwrep6h6hrzsibeqd.onion:18083",
          "zbjkbsxc5munw3qusl7j2hpcmikhqocdf4pqhnhtpzw5nt5jrmofptid.onion:18083",
          "qz43zul2x56jexzoqgkx2trzwcfnr6l3hbtfcfx54g4r3eahy3bssjyd.onion:18083",
          "plowsof3t5hogddwabaeiyrno25efmzfxyro2vligremt7sxpsclfaid.onion:18083",
          "plowsoffjexmxalw73tkjmf422gq6575fc7vicuu4javzn2ynnte6tyd.onion:18083",
        };
      }
      return {};
    case epee::net_utils::zone::i2p:
      if (m_nettype == cryptonote::MAINNET)
      {
        return {
          "s3l6ke4ed3df466khuebb4poienoingwof7oxtbo6j4n56sghe3a.b32.i2p:18080",
          "sel36x6fibfzujwvt4hf5gxolz6kd3jpvbjqg6o3ud2xtionyl2q.b32.i2p:18080",
          "uqj3aphckqtjsitz7kxx5flqpwjlq5ppr3chazfued7xucv3nheq.b32.i2p:18080",
          "vdmnehdjkpkg57nthgnjfuaqgku673r5bpbqg56ix6fyqoywgqrq.b32.i2p:18080",
        };
      }
      return {};
    default:
      break;
    }
    throw std::logic_error{"Bad zone given to get_seed_nodes"};
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  typename node_server<t_payload_net_handler>::network_zone& node_server<t_payload_net_handler>::add_zone(const epee::net_utils::zone zone)
  {
    const auto zone_ = m_network_zones.lower_bound(zone);
    if (zone_ != m_network_zones.end() && zone_->first == zone)
      return zone_->second;

    network_zone& public_zone = m_network_zones[epee::net_utils::zone::public_];
    return m_network_zones.emplace_hint(zone_, std::piecewise_construct, std::make_tuple(zone), std::tie(public_zone.m_net_server.get_io_service()))->second;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::init(const boost::program_options::variables_map& vm, const std::string& proxy, bool proxy_dns_leaks_allowed)
  {
    bool res = handle_command_line(vm);
    CHECK_AND_ASSERT_MES(res, false, "Failed to handle command line");
    if (proxy.size())
    {
      const auto endpoint = net::get_tcp_endpoint(proxy);
      CHECK_AND_ASSERT_MES(endpoint, false, "Failed to parse proxy: " << proxy << " - " << endpoint.error());
      network_zone& public_zone = m_network_zones[epee::net_utils::zone::public_];
      public_zone.m_connect = &socks_connect;
      public_zone.m_proxy_address = *endpoint;
      public_zone.m_can_pingback = false;
      m_enable_dns_seed_nodes &= proxy_dns_leaks_allowed;
      m_enable_dns_blocklist &= proxy_dns_leaks_allowed;
    }

    if (m_nettype == cryptonote::TESTNET)
    {
      memcpy(&m_network_id, &::config::testnet::NETWORK_ID, 16);
    }
    else if (m_nettype == cryptonote::STAGENET)
    {
      memcpy(&m_network_id, &::config::stagenet::NETWORK_ID, 16);
    }
    else
    {
      memcpy(&m_network_id, &::config::NETWORK_ID, 16);
    }

    m_config_folder = command_line::get_arg(vm, cryptonote::arg_data_dir);
    network_zone& public_zone = m_network_zones.at(epee::net_utils::zone::public_);

    if ((m_nettype == cryptonote::MAINNET && public_zone.m_port != std::to_string(::config::P2P_DEFAULT_PORT))
        || (m_nettype == cryptonote::TESTNET && public_zone.m_port != std::to_string(::config::testnet::P2P_DEFAULT_PORT))
        || (m_nettype == cryptonote::STAGENET && public_zone.m_port != std::to_string(::config::stagenet::P2P_DEFAULT_PORT))) {
      m_config_folder = m_config_folder + "/" + public_zone.m_port;
    }

    res = init_config();
    CHECK_AND_ASSERT_MES(res, false, "Failed to init config.");

    for (auto& zone : m_network_zones)
    {
      res = zone.second.m_peerlist.init(m_peerlist_storage.take_zone(zone.first), m_allow_local_ip);
      CHECK_AND_ASSERT_MES(res, false, "Failed to init peerlist.");
    }

    for(const auto& p: m_command_line_peers)
      m_network_zones.at(p.adr.get_zone()).m_peerlist.append_with_peer_white(p);

    //only in case if we really sure that we have external visible ip
    m_have_address = true;

    //configure self

    public_zone.m_net_server.set_threads_prefix("P2P"); // all zones use these threads/asio::io_service

    // from here onwards, it's online stuff
    if (m_offline)
      return res;

    //try to bind
    m_ssl_support = epee::net_utils::ssl_support_t::e_ssl_support_disabled;
    for (auto& zone : m_network_zones)
    {
      zone.second.m_net_server.get_config_object().set_handler(this);
      zone.second.m_net_server.get_config_object().m_invoke_timeout = P2P_DEFAULT_INVOKE_TIMEOUT;

      if (!zone.second.m_bind_ip.empty())
      {
        std::string ipv6_addr = "";
        std::string ipv6_port = "";
        zone.second.m_net_server.set_connection_filter(this);
        MINFO("Binding (IPv4) on " << zone.second.m_bind_ip << ":" << zone.second.m_port);
        if (!zone.second.m_bind_ipv6_address.empty() && m_use_ipv6)
        {
          ipv6_addr = zone.second.m_bind_ipv6_address;
          ipv6_port = zone.second.m_port_ipv6;
          MINFO("Binding (IPv6) on " << zone.second.m_bind_ipv6_address << ":" << zone.second.m_port_ipv6);
        }
        res = zone.second.m_net_server.init_server(zone.second.m_port, zone.second.m_bind_ip, ipv6_port, ipv6_addr, m_use_ipv6, m_require_ipv4, epee::net_utils::ssl_support_t::e_ssl_support_disabled);
        CHECK_AND_ASSERT_MES(res, false, "Failed to bind server");
      }
    }

    m_listening_port = public_zone.m_net_server.get_binded_port();
    MLOG_GREEN(el::Level::Info, "Net service bound (IPv4) to " << public_zone.m_bind_ip << ":" << m_listening_port);
    if (m_use_ipv6)
    {
      m_listening_port_ipv6 = public_zone.m_net_server.get_binded_port_ipv6();
      MLOG_GREEN(el::Level::Info, "Net service bound (IPv6) to " << public_zone.m_bind_ipv6_address << ":" << m_listening_port_ipv6);
    }
    if(m_external_port)
      MDEBUG("External port defined as " << m_external_port);

    // add UPnP port mapping
    if(m_igd == igd)
    {
      add_upnp_port_mapping_v4(m_listening_port);
      if (m_use_ipv6)
      {
        add_upnp_port_mapping_v6(m_listening_port_ipv6);
      }
    }

    return res;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  typename node_server<t_payload_net_handler>::payload_net_handler& node_server<t_payload_net_handler>::get_payload_object()
  {
    return m_payload_handler;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::run()
  {
    // creating thread to log number of connections
    mPeersLoggerThread.reset(new boost::thread([&]()
    {
      _note("Thread monitor number of peers - start");
      const network_zone& public_zone = m_network_zones.at(epee::net_utils::zone::public_);
      while (!is_closing && !public_zone.m_net_server.is_stop_signal_sent())
      { // main loop of thread
        //number_of_peers = m_net_server.get_config_object().get_connections_count();
        for (auto& zone : m_network_zones)
        {
          unsigned int number_of_in_peers = 0;
          unsigned int number_of_out_peers = 0;
          zone.second.m_net_server.get_config_object().foreach_connection([&](const p2p_connection_context& cntxt)
          {
            if (cntxt.m_is_income)
            {
              ++number_of_in_peers;
            }
            else
            {
              ++number_of_out_peers;
            }
            return true;
          }); // lambda
          zone.second.m_current_number_of_in_peers = number_of_in_peers;
          zone.second.m_current_number_of_out_peers = number_of_out_peers;
        }
        boost::this_thread::sleep_for(boost::chrono::seconds(1));
      } // main loop of thread
      _note("Thread monitor number of peers - done");
    })); // lambda

    network_zone& public_zone = m_network_zones.at(epee::net_utils::zone::public_);
    public_zone.m_net_server.add_idle_handler(boost::bind(&node_server<t_payload_net_handler>::idle_worker, this), 1000);
    public_zone.m_net_server.add_idle_handler(boost::bind(&t_payload_net_handler::on_idle, &m_payload_handler), 1000);

    //here you can set worker threads count
    int thrds_count = 10;
    boost::thread::attributes attrs;
    attrs.set_stack_size(THREAD_STACK_SIZE);
    //go to loop
    MINFO("Run net_service loop( " << thrds_count << " threads)...");
    if(!public_zone.m_net_server.run_server(thrds_count, true, attrs))
    {
      LOG_ERROR("Failed to run net tcp server!");
    }

    MINFO("net_service loop stopped.");
    return true;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  uint64_t node_server<t_payload_net_handler>::get_public_connections_count()
  {
    auto public_zone = m_network_zones.find(epee::net_utils::zone::public_);
    if (public_zone == m_network_zones.end())
      return 0;
    return public_zone->second.m_net_server.get_config_object().get_connections_count();
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::deinit()
  {
    kill();

    if (!m_offline)
    {
      for(auto& zone : m_network_zones)
        zone.second.m_net_server.deinit_server();
      // remove UPnP port mapping
      if(m_igd == igd)
        delete_upnp_port_mapping(m_listening_port);
    }
    return store_config();
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::store_config()
  {
    TRY_ENTRY();

    if (!tools::create_directories_if_necessary(m_config_folder))
    {
      MWARNING("Failed to create data directory \"" << m_config_folder);
      return false;
    }

    peerlist_types active{};
    for (auto& zone : m_network_zones)
      zone.second.m_peerlist.get_peerlist(active);

    const std::string state_file_path = m_config_folder + "/" + P2P_NET_DATA_FILENAME;
    if (!m_peerlist_storage.store(state_file_path, active))
    {
      MWARNING("Failed to save config to file " << state_file_path);
      return false;
    }
    CATCH_ENTRY_L0("node_server::store", false);
    return true;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::send_stop_signal()
  {
    MDEBUG("[node] sending stop signal");
    for (auto& zone : m_network_zones)
        zone.second.m_net_server.send_stop_signal();
    MDEBUG("[node] Stop signal sent");

    for (auto& zone : m_network_zones)
    {
      std::list<boost::uuids::uuid> connection_ids;
      zone.second.m_net_server.get_config_object().foreach_connection([&](const p2p_connection_context& cntxt) {
        connection_ids.push_back(cntxt.m_connection_id);
        return true;
      });
      for (const auto &connection_id: connection_ids)
        zone.second.m_net_server.get_config_object().close(connection_id);
    }
    m_payload_handler.stop();
    return true;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::do_handshake_with_peer(peerid_type& pi, p2p_connection_context& context_, bool just_take_peerlist)
  {
    network_zone& zone = m_network_zones.at(context_.m_remote_address.get_zone());

    typename COMMAND_HANDSHAKE::request arg;
    typename COMMAND_HANDSHAKE::response rsp;
    get_local_node_data(arg.node_data, zone);
    m_payload_handler.get_payload_sync_data(arg.payload_data);

    epee::simple_event ev;
    std::atomic<bool> hsh_result(false);
    bool timeout = false;

    bool r = epee::net_utils::async_invoke_remote_command2<typename COMMAND_HANDSHAKE::response>(context_, COMMAND_HANDSHAKE::ID, arg, zone.m_net_server.get_config_object(),
      [this, &pi, &ev, &hsh_result, &just_take_peerlist, &context_, &timeout](int code, const typename COMMAND_HANDSHAKE::response& rsp, p2p_connection_context& context)
    {
      epee::misc_utils::auto_scope_leave_caller scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&](){ev.raise();});

      if(code < 0)
      {
        LOG_WARNING_CC(context, "COMMAND_HANDSHAKE invoke failed. (" << code <<  ", " << epee::levin::get_err_descr(code) << ")");
        if (code == LEVIN_ERROR_CONNECTION_TIMEDOUT || code == LEVIN_ERROR_CONNECTION_DESTROYED)
          timeout = true;
        return;
      }

      if(rsp.node_data.network_id != m_network_id)
      {
        LOG_WARNING_CC(context, "COMMAND_HANDSHAKE Failed, wrong network!  (" << rsp.node_data.network_id << "), closing connection.");
        return;
      }

      if(!handle_remote_peerlist(rsp.local_peerlist_new, context))
      {
        LOG_WARNING_CC(context, "COMMAND_HANDSHAKE: failed to handle_remote_peerlist(...), closing connection.");
        add_host_fail(context.m_remote_address);
        return;
      }
      hsh_result = true;
      if(!just_take_peerlist)
      {
        if(!m_payload_handler.process_payload_sync_data(rsp.payload_data, context, true))
        {
          LOG_WARNING_CC(context, "COMMAND_HANDSHAKE invoked, but process_payload_sync_data returned false, dropping connection.");
          hsh_result = false;
          return;
        }

        pi = context.peer_id = rsp.node_data.peer_id;
        context.m_rpc_port = rsp.node_data.rpc_port;
        context.m_rpc_credits_per_hash = rsp.node_data.rpc_credits_per_hash;
        context.support_flags = rsp.node_data.support_flags;
        const auto azone = context.m_remote_address.get_zone();
        network_zone& zone = m_network_zones.at(azone);
        zone.m_peerlist.set_peer_just_seen(rsp.node_data.peer_id, context.m_remote_address, context.m_pruning_seed, context.m_rpc_port, context.m_rpc_credits_per_hash);

        // move
        if(azone == epee::net_utils::zone::public_ && rsp.node_data.peer_id == zone.m_config.m_peer_id)
        {
          LOG_DEBUG_CC(context, "Connection to self detected, dropping connection");
          hsh_result = false;
          return;
        }
        LOG_INFO_CC(context, "New connection handshaked, pruning seed " << epee::string_tools::to_string_hex(context.m_pruning_seed));
        LOG_DEBUG_CC(context, " COMMAND_HANDSHAKE INVOKED OK");
      }else
      {
        LOG_DEBUG_CC(context, " COMMAND_HANDSHAKE(AND CLOSE) INVOKED OK");
      }
      context_ = context;
    }, P2P_DEFAULT_HANDSHAKE_INVOKE_TIMEOUT);

    if(r)
    {
      ev.wait();
    }

    if(!hsh_result)
    {
      LOG_WARNING_CC(context_, "COMMAND_HANDSHAKE Failed");
      if (!timeout)
        zone.m_net_server.get_config_object().close(context_.m_connection_id);
    }
    else if (!just_take_peerlist)
    {
      if (context_.support_flags == 0)
        try_get_support_flags(context_, [](p2p_connection_context& flags_context, const uint32_t& support_flags) 
        {
          flags_context.support_flags = support_flags;
        });
    }

    return hsh_result;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::do_peer_timed_sync(const epee::net_utils::connection_context_base& context_, peerid_type peer_id)
  {
    typename COMMAND_TIMED_SYNC::request arg = AUTO_VAL_INIT(arg);
    m_payload_handler.get_payload_sync_data(arg.payload_data);

    network_zone& zone = m_network_zones.at(context_.m_remote_address.get_zone());
    bool r = epee::net_utils::async_invoke_remote_command2<typename COMMAND_TIMED_SYNC::response>(context_, COMMAND_TIMED_SYNC::ID, arg, zone.m_net_server.get_config_object(),
      [this](int code, const typename COMMAND_TIMED_SYNC::response& rsp, p2p_connection_context& context)
    {
      context.m_in_timedsync = false;
      if(code < 0)
      {
        LOG_WARNING_CC(context, "COMMAND_TIMED_SYNC invoke failed. (" << code <<  ", " << epee::levin::get_err_descr(code) << ")");
        return;
      }

      if(!handle_remote_peerlist(rsp.local_peerlist_new, context))
      {
        LOG_WARNING_CC(context, "COMMAND_TIMED_SYNC: failed to handle_remote_peerlist(...), closing connection.");
        m_network_zones.at(context.m_remote_address.get_zone()).m_net_server.get_config_object().close(context.m_connection_id );
        add_host_fail(context.m_remote_address);
      }
      if(!context.m_is_income)
        m_network_zones.at(context.m_remote_address.get_zone()).m_peerlist.set_peer_just_seen(context.peer_id, context.m_remote_address, context.m_pruning_seed, context.m_rpc_port, context.m_rpc_credits_per_hash);
      if (!m_payload_handler.process_payload_sync_data(rsp.payload_data, context, false))
      {
        m_network_zones.at(context.m_remote_address.get_zone()).m_net_server.get_config_object().close(context.m_connection_id );
      }
    });

    if(!r)
    {
      LOG_WARNING_CC(context_, "COMMAND_TIMED_SYNC Failed");
      return false;
    }
    return true;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  size_t node_server<t_payload_net_handler>::get_random_index_with_fixed_probability(size_t max_index)
  {
    //divide by zero workaround
    if(!max_index)
      return 0;

    size_t x = crypto::rand<size_t>()%(16*max_index+1);
    size_t res = (x*x*x)/(max_index*max_index*16*16*16); //parabola \/
    MDEBUG("Random connection index=" << res << "(x="<< x << ", max_index=" << max_index << ")");
    return res;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::is_peer_used(const peerlist_entry& peer)
  {
    const auto zone = peer.adr.get_zone();
    const auto server = m_network_zones.find(zone);
    if (server == m_network_zones.end())
      return false;

    const bool is_public = (zone == epee::net_utils::zone::public_);
    if(is_public && server->second.m_config.m_peer_id == peer.id)
      return true;//dont make connections to ourself

    bool used = false;
    server->second.m_net_server.get_config_object().foreach_connection([&, is_public](const p2p_connection_context& cntxt)
    {
      if((is_public && cntxt.peer_id == peer.id) || (!cntxt.m_is_income && peer.adr == cntxt.m_remote_address))
      {
        used = true;
        return false;//stop enumerating
      }
      return true;
    });
    return used;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::is_peer_used(const anchor_peerlist_entry& peer)
  {
    const auto zone = peer.adr.get_zone();
    const auto server = m_network_zones.find(zone);
    if (server == m_network_zones.end())
      return false;

    const bool is_public = (zone == epee::net_utils::zone::public_);
    if(is_public && server->second.m_config.m_peer_id == peer.id)
      return true;//dont make connections to ourself

    bool used = false;
    server->second.m_net_server.get_config_object().foreach_connection([&, is_public](const p2p_connection_context& cntxt)
    {
      if((is_public && cntxt.peer_id == peer.id) || (!cntxt.m_is_income && peer.adr == cntxt.m_remote_address))
      {
        used = true;
        return false;//stop enumerating
      }
      return true;
    });
    return used;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::is_addr_connected(const epee::net_utils::network_address& peer)
  {
    const auto zone = m_network_zones.find(peer.get_zone());
    if (zone == m_network_zones.end())
      return false;

    bool connected = false;
    zone->second.m_net_server.get_config_object().foreach_connection([&](const p2p_connection_context& cntxt)
    {
      if(!cntxt.m_is_income && peer == cntxt.m_remote_address)
      {
        connected = true;
        return false;//stop enumerating
      }
      return true;
    });

    return connected;
  }

#define LOG_PRINT_CC_PRIORITY_NODE(priority, con, msg) \
  do { \
    if (priority) {\
      LOG_INFO_CC(con, "[priority]" << msg); \
    } else {\
      LOG_INFO_CC(con, msg); \
    } \
  } while(0)

  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::try_to_connect_and_handshake_with_new_peer(const epee::net_utils::network_address& na, bool just_take_peerlist, uint64_t last_seen_stamp, PeerType peer_type, uint64_t first_seen_stamp)
  {
    network_zone& zone = m_network_zones.at(na.get_zone());
    if (zone.m_connect == nullptr) // outgoing connections in zone not possible
      return false;

    if (zone.m_our_address == na)
      return false;

    if (zone.m_current_number_of_out_peers == zone.m_config.m_net_config.max_out_connection_count) // out peers limit
    {
      return false;
    }
    else if (zone.m_current_number_of_out_peers > zone.m_config.m_net_config.max_out_connection_count)
    {
      zone.m_net_server.get_config_object().del_out_connections(1);
      --(zone.m_current_number_of_out_peers); // atomic variable, update time = 1s
      return false;
    }


    MDEBUG("Connecting to " << na.str() << "(peer_type=" << peer_type << ", last_seen: "
        << (last_seen_stamp ? epee::misc_utils::get_time_interval_string(time(NULL) - last_seen_stamp):"never")
        << ")...");

    auto con = zone.m_connect(zone, na, m_ssl_support);
    if(!con)
    {
      bool is_priority = is_priority_node(na);
      LOG_PRINT_CC_PRIORITY_NODE(is_priority, bool(con), "Connect failed to " << na.str()
        /*<< ", try " << try_count*/);
      record_addr_failed(na);
      return false;
    }

    con->m_anchor = peer_type == anchor;
    peerid_type pi = AUTO_VAL_INIT(pi);
    bool res = do_handshake_with_peer(pi, *con, just_take_peerlist);

    if(!res)
    {
      bool is_priority = is_priority_node(na);
      LOG_PRINT_CC_PRIORITY_NODE(is_priority, *con, "Failed to HANDSHAKE with peer "
        << na.str()
        /*<< ", try " << try_count*/);
      record_addr_failed(na);
      return false;
    }

    if(just_take_peerlist)
    {
      zone.m_net_server.get_config_object().close(con->m_connection_id);
      LOG_DEBUG_CC(*con, "CONNECTION HANDSHAKED OK AND CLOSED.");
      return true;
    }

    peerlist_entry pe_local = AUTO_VAL_INIT(pe_local);
    pe_local.adr = na;
    pe_local.id = pi;
    time_t last_seen;
    time(&last_seen);
    pe_local.last_seen = static_cast<int64_t>(last_seen);
    pe_local.pruning_seed = con->m_pruning_seed;
    pe_local.rpc_port = con->m_rpc_port;
    pe_local.rpc_credits_per_hash = con->m_rpc_credits_per_hash;
    zone.m_peerlist.append_with_peer_white(pe_local);
    //update last seen and push it to peerlist manager

    anchor_peerlist_entry ape = AUTO_VAL_INIT(ape);
    ape.adr = na;
    ape.id = pi;
    ape.first_seen = first_seen_stamp ? first_seen_stamp : time(nullptr);

    zone.m_peerlist.append_with_peer_anchor(ape);
    zone.m_notifier.on_handshake_complete(con->m_connection_id, con->m_is_income);
    zone.m_notifier.new_out_connection();

    LOG_DEBUG_CC(*con, "CONNECTION HANDSHAKED OK.");
    return true;
  }

  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::check_connection_and_handshake_with_peer(const epee::net_utils::network_address& na, uint64_t last_seen_stamp)
  {
    network_zone& zone = m_network_zones.at(na.get_zone());
    if (zone.m_connect == nullptr)
      return false;

    LOG_PRINT_L1("Connecting to " << na.str() << "(last_seen: "
                                  << (last_seen_stamp ? epee::misc_utils::get_time_interval_string(time(NULL) - last_seen_stamp):"never")
                                  << ")...");

    auto con = zone.m_connect(zone, na, m_ssl_support);
    if (!con) {
      bool is_priority = is_priority_node(na);

      LOG_PRINT_CC_PRIORITY_NODE(is_priority, p2p_connection_context{}, "Connect failed to " << na.str());
      record_addr_failed(na);

      return false;
    }

    con->m_anchor = false;
    peerid_type pi = AUTO_VAL_INIT(pi);
    const bool res = do_handshake_with_peer(pi, *con, true);
    if (!res) {
      bool is_priority = is_priority_node(na);

      LOG_PRINT_CC_PRIORITY_NODE(is_priority, *con, "Failed to HANDSHAKE with peer " << na.str());
      record_addr_failed(na);
      return false;
    }

    zone.m_net_server.get_config_object().close(con->m_connection_id);

    LOG_DEBUG_CC(*con, "CONNECTION HANDSHAKED OK AND CLOSED.");

    return true;
  }

#undef LOG_PRINT_CC_PRIORITY_NODE

  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  void node_server<t_payload_net_handler>::record_addr_failed(const epee::net_utils::network_address& addr)
  {
    CRITICAL_REGION_LOCAL(m_conn_fails_cache_lock);
    m_conn_fails_cache[addr.host_str()] = time(NULL);
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::is_addr_recently_failed(const epee::net_utils::network_address& addr)
  {
    CRITICAL_REGION_LOCAL(m_conn_fails_cache_lock);
    auto it = m_conn_fails_cache.find(addr.host_str());
    if(it == m_conn_fails_cache.end())
      return false;

    if(time(NULL) - it->second > P2P_FAILED_ADDR_FORGET_SECONDS)
      return false;
    else
      return true;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::make_new_connection_from_anchor_peerlist(const std::vector<anchor_peerlist_entry>& anchor_peerlist)
  {
    for (const auto& pe: anchor_peerlist) {
      _note("Considering connecting (out) to anchor peer: " << peerid_to_string(pe.id) << " " << pe.adr.str());

      if(is_peer_used(pe)) {
        _note("Peer is used");
        continue;
      }

      if(!is_remote_host_allowed(pe.adr)) {
        continue;
      }

      if(is_addr_recently_failed(pe.adr)) {
        continue;
      }

      MDEBUG("Selected peer: " << peerid_to_string(pe.id) << " " << pe.adr.str()
                               << "[peer_type=" << anchor
                               << "] first_seen: " << epee::misc_utils::get_time_interval_string(time(NULL) - pe.first_seen));

      if(!try_to_connect_and_handshake_with_new_peer(pe.adr, false, 0, anchor, pe.first_seen)) {
        _note("Handshake failed");
        continue;
      }

      return true;
    }

    return false;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::make_new_connection_from_peerlist(network_zone& zone, bool use_white_list)
  {
    size_t max_random_index = 0;

    std::set<size_t> tried_peers;

    size_t try_count = 0;
    size_t rand_count = 0;
    while(rand_count < (max_random_index+1)*3 &&  try_count < 10 && !zone.m_net_server.is_stop_signal_sent())
    {
      ++rand_count;
      size_t random_index;
      const uint32_t next_needed_pruning_stripe = m_payload_handler.get_next_needed_pruning_stripe().second;

      // build a set of all the /16 we're connected to, and prefer a peer that's not in that set
      std::set<uint32_t> classB;
      if (&zone == &m_network_zones.at(epee::net_utils::zone::public_)) // at returns reference, not copy
      {
        zone.m_net_server.get_config_object().foreach_connection([&](const p2p_connection_context& cntxt)
        {
          if (cntxt.m_remote_address.get_type_id() == epee::net_utils::ipv4_network_address::get_type_id())
          {

            const epee::net_utils::network_address na = cntxt.m_remote_address;
            const uint32_t actual_ip = na.as<const epee::net_utils::ipv4_network_address>().ip();
            classB.insert(actual_ip & 0x0000ffff);
          }
          else if (cntxt.m_remote_address.get_type_id() == epee::net_utils::ipv6_network_address::get_type_id())
          {
            const epee::net_utils::network_address na = cntxt.m_remote_address;
            const boost::asio::ip::address_v6 &actual_ip = na.as<const epee::net_utils::ipv6_network_address>().ip();
            if (actual_ip.is_v4_mapped())
            {
              boost::asio::ip::address_v4 v4ip = make_address_v4_from_v6(actual_ip);
              uint32_t actual_ipv4;
              memcpy(&actual_ipv4, v4ip.to_bytes().data(), sizeof(actual_ipv4));
              classB.insert(actual_ipv4 & ntohl(0xffff0000));
            }
          }
          return true;
        });
      }

      auto get_host_string = [](const epee::net_utils::network_address &address) {
        if (address.get_type_id() == epee::net_utils::ipv6_network_address::get_type_id())
        {
          boost::asio::ip::address_v6 actual_ip = address.as<const epee::net_utils::ipv6_network_address>().ip();
          if (actual_ip.is_v4_mapped())
          {
            boost::asio::ip::address_v4 v4ip = make_address_v4_from_v6(actual_ip);
            uint32_t actual_ipv4;
            memcpy(&actual_ipv4, v4ip.to_bytes().data(), sizeof(actual_ipv4));
            return epee::net_utils::ipv4_network_address(actual_ipv4, 0).host_str();
          }
        }
        return address.host_str();
      };
      std::unordered_set<std::string> hosts_added;
      std::deque<size_t> filtered;
      const size_t limit = use_white_list ? 20 : std::numeric_limits<size_t>::max();
      for (int step = 0; step < 2; ++step)
      {
        bool skip_duplicate_class_B = step == 0;
        size_t idx = 0, skipped = 0;
        zone.m_peerlist.foreach (use_white_list, [&classB, &filtered, &idx, &skipped, skip_duplicate_class_B, limit, next_needed_pruning_stripe, &hosts_added, &get_host_string](const peerlist_entry &pe){
          if (filtered.size() >= limit)
            return false;
          bool skip = false;
          if (skip_duplicate_class_B && pe.adr.get_type_id() == epee::net_utils::ipv4_network_address::get_type_id())
          {
            const epee::net_utils::network_address na = pe.adr;
            uint32_t actual_ip = na.as<const epee::net_utils::ipv4_network_address>().ip();
            skip = classB.find(actual_ip & 0x0000ffff) != classB.end();
          }
          else if (skip_duplicate_class_B && pe.adr.get_type_id() == epee::net_utils::ipv6_network_address::get_type_id())
          {
            const epee::net_utils::network_address na = pe.adr;
            const boost::asio::ip::address_v6 &actual_ip = na.as<const epee::net_utils::ipv6_network_address>().ip();
            if (actual_ip.is_v4_mapped())
            {
              boost::asio::ip::address_v4 v4ip = make_address_v4_from_v6(actual_ip);
              uint32_t actual_ipv4;
              memcpy(&actual_ipv4, v4ip.to_bytes().data(), sizeof(actual_ipv4));
              skip = classB.find(actual_ipv4 & ntohl(0xffff0000)) != classB.end();
            }
          }

          // consider each host once, to avoid giving undue inflence to hosts running several nodes
          if (!skip)
          {
            const auto i = hosts_added.find(get_host_string(pe.adr));
            if (i != hosts_added.end())
              skip = true;
          }

          if (skip)
            ++skipped;
          else if (next_needed_pruning_stripe == 0 || pe.pruning_seed == 0)
            filtered.push_back(idx);
          else if (next_needed_pruning_stripe == tools::get_pruning_stripe(pe.pruning_seed))
            filtered.push_front(idx);
          ++idx;
          hosts_added.insert(get_host_string(pe.adr));
          return true;
        });
        if (skipped == 0 || !filtered.empty())
          break;
        if (skipped)
          MDEBUG("Skipping " << skipped << " possible peers as they share a class B with existing peers");
      }
      if (filtered.empty())
      {
        MINFO("No available peer in " << (use_white_list ? "white" : "gray") << " list filtered by " << next_needed_pruning_stripe);
        return false;
      }
      if (use_white_list)
      {
        // if using the white list, we first pick in the set of peers we've already been using earlier
        random_index = get_random_index_with_fixed_probability(std::min<uint64_t>(filtered.size() - 1, 20));
        CRITICAL_REGION_LOCAL(m_used_stripe_peers_mutex);
        if (next_needed_pruning_stripe > 0 && next_needed_pruning_stripe <= (1ul << CRYPTONOTE_PRUNING_LOG_STRIPES) && !m_used_stripe_peers[next_needed_pruning_stripe-1].empty())
        {
          const epee::net_utils::network_address na = m_used_stripe_peers[next_needed_pruning_stripe-1].front();
          m_used_stripe_peers[next_needed_pruning_stripe-1].pop_front();
          for (size_t i = 0; i < filtered.size(); ++i)
          {
            peerlist_entry pe;
            if (zone.m_peerlist.get_white_peer_by_index(pe, filtered[i]) && pe.adr == na)
            {
              MDEBUG("Reusing stripe " << next_needed_pruning_stripe << " peer " << pe.adr.str());
              random_index = i;
              break;
            }
          }
        }
      }
      else
        random_index = crypto::rand_idx(filtered.size());

      CHECK_AND_ASSERT_MES(random_index < filtered.size(), false, "random_index < filtered.size() failed!!");
      random_index = filtered[random_index];
      CHECK_AND_ASSERT_MES(random_index < (use_white_list ? zone.m_peerlist.get_white_peers_count() : zone.m_peerlist.get_gray_peers_count()),
          false, "random_index < peers size failed!!");

      if(tried_peers.count(random_index))
        continue;

      tried_peers.insert(random_index);
      peerlist_entry pe = AUTO_VAL_INIT(pe);
      bool r = use_white_list ? zone.m_peerlist.get_white_peer_by_index(pe, random_index):zone.m_peerlist.get_gray_peer_by_index(pe, random_index);
      CHECK_AND_ASSERT_MES(r, false, "Failed to get random peer from peerlist(white:" << use_white_list << ")");

      ++try_count;

      _note("Considering connecting (out) to " << (use_white_list ? "white" : "gray") << " list peer: " <<
          peerid_to_string(pe.id) << " " << pe.adr.str() << ", pruning seed " << epee::string_tools::to_string_hex(pe.pruning_seed) <<
          " (stripe " << next_needed_pruning_stripe << " needed)");

      if(zone.m_our_address == pe.adr)
        continue;

      if(is_peer_used(pe)) {
        _note("Peer is used");
        continue;
      }

      if(!is_remote_host_allowed(pe.adr))
        continue;

      if(is_addr_recently_failed(pe.adr))
        continue;

      MDEBUG("Selected peer: " << peerid_to_string(pe.id) << " " << pe.adr.str()
                    << ", pruning seed " << epee::string_tools::to_string_hex(pe.pruning_seed) << " "
                    << "[peer_list=" << (use_white_list ? white : gray)
                    << "] last_seen: " << (pe.last_seen ? epee::misc_utils::get_time_interval_string(time(NULL) - pe.last_seen) : "never"));

      if(!try_to_connect_and_handshake_with_new_peer(pe.adr, false, pe.last_seen, use_white_list ? white : gray)) {
        _note("Handshake failed");
        continue;
      }

      return true;
    }
    return false;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::connect_to_seed(epee::net_utils::zone zone)
  {
      network_zone& server = m_network_zones.at(zone);
      boost::upgrade_lock<boost::shared_mutex> seed_nodes_upgrade_lock(server.m_seed_nodes_lock);

      if (!server.m_seed_nodes_initialized)
      {
        const std::uint16_t default_port = cryptonote::get_config(m_nettype).P2P_DEFAULT_PORT;
        boost::upgrade_to_unique_lock<boost::shared_mutex> seed_nodes_lock(seed_nodes_upgrade_lock);
        server.m_seed_nodes_initialized = true;
        for (const auto& full_addr : get_seed_nodes(zone))
        {
          // seeds should have hostname converted to IP already
          MDEBUG("Seed node: " << full_addr);
          server.m_seed_nodes.push_back(MONERO_UNWRAP(net::get_network_address(full_addr, default_port)));
        }
        MDEBUG("Number of seed nodes: " << server.m_seed_nodes.size());
      }

      if (server.m_seed_nodes.empty() || m_offline || !m_exclusive_peers.empty())
        return true;

      size_t try_count = 0;
      bool is_connected_to_at_least_one_seed_node = false;
      size_t current_index = crypto::rand_idx(server.m_seed_nodes.size());
      while(true)
      {
        if(server.m_net_server.is_stop_signal_sent())
          return false;

        peerlist_entry pe_seed{};
        pe_seed.adr = server.m_seed_nodes[current_index];
        if (is_peer_used(pe_seed))
          is_connected_to_at_least_one_seed_node = true;
        else if (try_to_connect_and_handshake_with_new_peer(server.m_seed_nodes[current_index], true))
          break;
        if(++try_count > server.m_seed_nodes.size())
        {
          // only IP zone has fallback (to direct IP) seeds
          if (zone == epee::net_utils::zone::public_ && !m_fallback_seed_nodes_added.test_and_set())
          {
            MWARNING("Failed to connect to any of seed peers, trying fallback seeds");
            current_index = server.m_seed_nodes.size() - 1;
            {
              boost::upgrade_to_unique_lock<boost::shared_mutex> seed_nodes_lock(seed_nodes_upgrade_lock);

              for (const auto &peer: get_ip_seed_nodes())
              {
                MDEBUG("Fallback seed node: " << peer);
                append_net_address(server.m_seed_nodes, peer, cryptonote::get_config(m_nettype).P2P_DEFAULT_PORT);
              }
            }
            if (current_index == server.m_seed_nodes.size() - 1)
            {
              MWARNING("No fallback seeds, continuing without seeds");
              break;
            }
            // continue for another few cycles
          }
          else
          {
            if (!is_connected_to_at_least_one_seed_node)
              MWARNING("Failed to connect to any of seed peers, continuing without seeds");
            break;
          }
        }
        if(++current_index >= server.m_seed_nodes.size())
          current_index = 0;
      }
      return true;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::connections_maker()
  {
    using zone_type = epee::net_utils::zone;

    if (m_offline) return true;
    if (!connect_to_peerlist(m_exclusive_peers)) return false;

    if (!m_exclusive_peers.empty()) return true;

    bool one_succeeded = false;
    for(auto& zone : m_network_zones)
    {
      size_t start_conn_count = get_outgoing_connections_count(zone.second);
      if(!zone.second.m_peerlist.get_white_peers_count() && !connect_to_seed(zone.first))
      {
        continue;
      }

      if (zone.first == zone_type::public_ && !connect_to_peerlist(m_priority_peers)) continue;

      size_t base_expected_white_connections = (zone.second.m_config.m_net_config.max_out_connection_count*P2P_DEFAULT_WHITELIST_CONNECTIONS_PERCENT)/100;

      // carefully avoid `continue` in nested loop
      
      size_t conn_count = get_outgoing_connections_count(zone.second);
      while(conn_count < zone.second.m_config.m_net_config.max_out_connection_count)
      {
        const size_t expected_white_connections = m_payload_handler.get_next_needed_pruning_stripe().second ? zone.second.m_config.m_net_config.max_out_connection_count : base_expected_white_connections;
        if(conn_count < expected_white_connections)
        {
          //start from anchor list
          while (get_outgoing_connections_count(zone.second) < P2P_DEFAULT_ANCHOR_CONNECTIONS_COUNT
            && make_expected_connections_count(zone.second, anchor, P2P_DEFAULT_ANCHOR_CONNECTIONS_COUNT));
          //then do white list
          while (get_outgoing_connections_count(zone.second) < expected_white_connections
            && make_expected_connections_count(zone.second, white, expected_white_connections));
          //then do grey list
          while (get_outgoing_connections_count(zone.second) < zone.second.m_config.m_net_config.max_out_connection_count
            && make_expected_connections_count(zone.second, gray, zone.second.m_config.m_net_config.max_out_connection_count));
        }else
        {
          //start from grey list
          while (get_outgoing_connections_count(zone.second) < zone.second.m_config.m_net_config.max_out_connection_count
            && make_expected_connections_count(zone.second, gray, zone.second.m_config.m_net_config.max_out_connection_count));
          //and then do white list
          while (get_outgoing_connections_count(zone.second) < zone.second.m_config.m_net_config.max_out_connection_count
            && make_expected_connections_count(zone.second, white, zone.second.m_config.m_net_config.max_out_connection_count));
        }
        if(zone.second.m_net_server.is_stop_signal_sent())
          return false;
        size_t new_conn_count = get_outgoing_connections_count(zone.second);
        if (new_conn_count <= conn_count)
        {
          // we did not make any connection, sleep a bit to avoid a busy loop in case we don't have
          // any peers to try, then break so we will try seeds to get more peers
          boost::this_thread::sleep_for(boost::chrono::seconds(1));
          break;
        }
        conn_count = new_conn_count;
      }

      if (start_conn_count == get_outgoing_connections_count(zone.second) && start_conn_count < zone.second.m_config.m_net_config.max_out_connection_count)
      {
        MINFO("Failed to connect to any, trying seeds");
        if (!connect_to_seed(zone.first))
          continue;
      }
      one_succeeded = true;
    }

    return one_succeeded;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::make_expected_connections_count(network_zone& zone, PeerType peer_type, size_t expected_connections)
  {
    if (m_offline)
      return false;

    std::vector<anchor_peerlist_entry> apl;

    if (peer_type == anchor) {
      zone.m_peerlist.get_and_empty_anchor_peerlist(apl);
    }

    size_t conn_count = get_outgoing_connections_count(zone);
    //add new connections from white peers
    if(conn_count < expected_connections)
    {
      if(zone.m_net_server.is_stop_signal_sent())
        return false;

      MDEBUG("Making expected connection, type " << peer_type << ", " << conn_count << "/" << expected_connections << " connections");

      if (peer_type == anchor && !make_new_connection_from_anchor_peerlist(apl)) {
        return false;
      }

      if (peer_type == white && !make_new_connection_from_peerlist(zone, true)) {
        return false;
      }

      if (peer_type == gray && !make_new_connection_from_peerlist(zone, false)) {
        return false;
      }
    }
    return true;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  size_t node_server<t_payload_net_handler>::get_public_outgoing_connections_count()
  {
    auto public_zone = m_network_zones.find(epee::net_utils::zone::public_);
    if (public_zone == m_network_zones.end())
      return 0;
    return get_outgoing_connections_count(public_zone->second);
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  size_t node_server<t_payload_net_handler>::get_incoming_connections_count(network_zone& zone)
  {
    size_t count = 0;
    zone.m_net_server.get_config_object().foreach_connection([&](const p2p_connection_context& cntxt)
    {
      if(cntxt.m_is_income)
        ++count;
      return true;
    });
    return count;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  size_t node_server<t_payload_net_handler>::get_outgoing_connections_count(network_zone& zone)
  {
    size_t count = 0;
    zone.m_net_server.get_config_object().foreach_connection([&](const p2p_connection_context& cntxt)
    {
      if(!cntxt.m_is_income)
        ++count;
      return true;
    });
    return count;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  size_t node_server<t_payload_net_handler>::get_outgoing_connections_count()
  {
    size_t count = 0;
    for(auto& zone : m_network_zones)
      count += get_outgoing_connections_count(zone.second);
    return count;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  size_t node_server<t_payload_net_handler>::get_incoming_connections_count()
  {
    size_t count = 0;
    for (auto& zone : m_network_zones)
    {
      zone.second.m_net_server.get_config_object().foreach_connection([&](const p2p_connection_context& cntxt)
      {
        if(cntxt.m_is_income)
          ++count;
        return true;
      });
    }
    return count;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  size_t node_server<t_payload_net_handler>::get_public_white_peers_count()
  {
    auto public_zone = m_network_zones.find(epee::net_utils::zone::public_);
    if (public_zone == m_network_zones.end())
      return 0;
    return public_zone->second.m_peerlist.get_white_peers_count();
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  size_t node_server<t_payload_net_handler>::get_public_gray_peers_count()
  {
    auto public_zone = m_network_zones.find(epee::net_utils::zone::public_);
    if (public_zone == m_network_zones.end())
      return 0;
    return public_zone->second.m_peerlist.get_gray_peers_count();
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  void node_server<t_payload_net_handler>::get_public_peerlist(std::vector<peerlist_entry>& gray, std::vector<peerlist_entry>& white)
  {
    auto public_zone = m_network_zones.find(epee::net_utils::zone::public_);
    if (public_zone != m_network_zones.end())
      public_zone->second.m_peerlist.get_peerlist(gray, white);
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  void node_server<t_payload_net_handler>::get_peerlist(std::vector<peerlist_entry>& gray, std::vector<peerlist_entry>& white)
  {
    for (auto &zone: m_network_zones)
    {
      zone.second.m_peerlist.get_peerlist(gray, white); // appends
    }
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::idle_worker()
  {
    m_peer_handshake_idle_maker_interval.do_call(boost::bind(&node_server<t_payload_net_handler>::peer_sync_idle_maker, this));
    m_connections_maker_interval.do_call(boost::bind(&node_server<t_payload_net_handler>::connections_maker, this));
    m_gray_peerlist_housekeeping_interval.do_call(boost::bind(&node_server<t_payload_net_handler>::gray_peerlist_housekeeping, this));
    m_peerlist_store_interval.do_call(boost::bind(&node_server<t_payload_net_handler>::store_config, this));
    m_incoming_connections_interval.do_call(boost::bind(&node_server<t_payload_net_handler>::check_incoming_connections, this));
    m_dns_blocklist_interval.do_call(boost::bind(&node_server<t_payload_net_handler>::update_dns_blocklist, this));
    return true;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::update_dns_blocklist()
  {
    if (!m_enable_dns_blocklist)
      return true;
    if (m_nettype != cryptonote::MAINNET)
      return true;

    static const std::vector<std::string> dns_urls = {
      "blocklist.moneropulse.se"
    , "blocklist.moneropulse.org"
    , "blocklist.moneropulse.net"
    , "blocklist.moneropulse.no"
    , "blocklist.moneropulse.fr"
    , "blocklist.moneropulse.de"
    , "blocklist.moneropulse.ch"
    };

    std::vector<std::string> records;
    if (!tools::dns_utils::load_txt_records_from_dns(records, dns_urls))
      return true;

    unsigned good = 0, bad = 0;
    for (const auto& record : records)
    {
      std::vector<std::string> ips;
      boost::split(ips, record, boost::is_any_of(";"));
      for (const auto &ip: ips)
      {
        if (ip.empty())
          continue;
        auto subnet = net::get_ipv4_subnet_address(ip);
        if (subnet)
        {
          block_subnet(*subnet, DNS_BLOCKLIST_LIFETIME);
          ++good;
          continue;
        }
        const expect<epee::net_utils::network_address> parsed_addr = net::get_network_address(ip, 0);
        if (parsed_addr)
        {
          block_host(*parsed_addr, DNS_BLOCKLIST_LIFETIME, true);
          ++good;
          continue;
        }
        MWARNING("Invalid IP address or subnet from DNS blocklist: " << ip << " - " << parsed_addr.error());
        ++bad;
      }
    }
    if (good > 0)
      MINFO(good << " addresses added to the blocklist");
    return true;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::check_incoming_connections()
  {
    if (m_offline)
      return true;

    const auto public_zone = m_network_zones.find(epee::net_utils::zone::public_);
    if (public_zone != m_network_zones.end() && get_incoming_connections_count(public_zone->second) == 0)
    {
      if (m_hide_my_port || public_zone->second.m_config.m_net_config.max_in_connection_count == 0)
      {
        MGINFO("Incoming connections disabled, enable them for full connectivity");
      }
      else
      {
        if (m_igd == delayed_igd)
        {
          MWARNING("No incoming connections, trying to setup IGD");
          add_upnp_port_mapping(m_listening_port);
          m_igd = igd;
        }
        else
        {
          const el::Level level = el::Level::Warning;
          MCLOG_RED(level, "global", "No incoming connections - check firewalls/routers allow port " << get_this_peer_port());
        }
      }
    }
    return true;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::peer_sync_idle_maker()
  {
    MDEBUG("STARTED PEERLIST IDLE HANDSHAKE");
    typedef std::list<std::pair<epee::net_utils::connection_context_base, peerid_type> > local_connects_type;
    local_connects_type cncts;
    for(auto& zone : m_network_zones)
    {
      zone.second.m_net_server.get_config_object().foreach_connection([&](p2p_connection_context& cntxt)
      {
        if(cntxt.peer_id && !cntxt.m_in_timedsync)
        {
          cntxt.m_in_timedsync = true;
          cncts.push_back(local_connects_type::value_type(cntxt, cntxt.peer_id));//do idle sync only with handshaked connections
        }
        return true;
      });
    }

    std::for_each(cncts.begin(), cncts.end(), [&](const typename local_connects_type::value_type& vl){do_peer_timed_sync(vl.first, vl.second);});

    MDEBUG("FINISHED PEERLIST IDLE HANDSHAKE");
    return true;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::sanitize_peerlist(std::vector<peerlist_entry>& local_peerlist)
  {
    for (size_t i = 0; i < local_peerlist.size(); ++i)
    {
      bool ignore = false;
      peerlist_entry &be = local_peerlist[i];
      epee::net_utils::network_address &na = be.adr;
      if (na.is_loopback() || na.is_local())
      {
        ignore = true;
      }
      else if (be.adr.get_type_id() == epee::net_utils::ipv4_network_address::get_type_id())
      {
        const epee::net_utils::ipv4_network_address &ipv4 = na.as<const epee::net_utils::ipv4_network_address>();
        if (ipv4.ip() == 0)
          ignore = true;
        else if (ipv4.port() == be.rpc_port)
          ignore = true;
      }
      if (be.pruning_seed && (be.pruning_seed < tools::make_pruning_seed(1, CRYPTONOTE_PRUNING_LOG_STRIPES) || be.pruning_seed > tools::make_pruning_seed(1ul << CRYPTONOTE_PRUNING_LOG_STRIPES, CRYPTONOTE_PRUNING_LOG_STRIPES)))
        ignore = true;
      if (ignore)
      {
        MDEBUG("Ignoring " << be.adr.str());
        std::swap(local_peerlist[i], local_peerlist[local_peerlist.size() - 1]);
        local_peerlist.resize(local_peerlist.size() - 1);
        --i;
        continue;
      }
      local_peerlist[i].last_seen = 0;
    }
    return true;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::handle_remote_peerlist(const std::vector<peerlist_entry>& peerlist, const epee::net_utils::connection_context_base& context)
  {
    if (peerlist.size() > P2P_MAX_PEERS_IN_HANDSHAKE)
    {
      MWARNING(context << "peer sent " << peerlist.size() << " peers, considered spamming");
      return false;
    }
    std::vector<peerlist_entry> peerlist_ = peerlist;
    if(!sanitize_peerlist(peerlist_))
      return false;

    const epee::net_utils::zone zone = context.m_remote_address.get_zone();
    for(const auto& peer : peerlist_)
    {
      if(peer.adr.get_zone() != zone)
      {
        MWARNING(context << " sent peerlist from another zone, dropping");
        return false;
      }
    }

    LOG_DEBUG_CC(context, "REMOTE PEERLIST: remote peerlist size=" << peerlist_.size());
    LOG_TRACE_CC(context, "REMOTE PEERLIST: " << ENDL << print_peerlist_to_string(peerlist_));
    CRITICAL_REGION_LOCAL(m_blocked_hosts_lock);
    return m_network_zones.at(context.m_remote_address.get_zone()).m_peerlist.merge_peerlist(peerlist_, [this](const peerlist_entry &pe) {
      return !is_addr_recently_failed(pe.adr) && is_remote_host_allowed(pe.adr);
    });
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::get_local_node_data(basic_node_data& node_data, const network_zone& zone)
  {
    node_data.peer_id = zone.m_config.m_peer_id;
    if(!m_hide_my_port && zone.m_can_pingback)
      node_data.my_port = m_external_port ? m_external_port : m_listening_port;
    else
      node_data.my_port = 0;
    node_data.rpc_port = zone.m_can_pingback ? m_rpc_port : 0;
    node_data.rpc_credits_per_hash = zone.m_can_pingback ? m_rpc_credits_per_hash : 0;
    node_data.network_id = m_network_id;
    node_data.support_flags = zone.m_config.m_support_flags;
    return true;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  int node_server<t_payload_net_handler>::handle_get_support_flags(int command, COMMAND_REQUEST_SUPPORT_FLAGS::request& arg, COMMAND_REQUEST_SUPPORT_FLAGS::response& rsp, p2p_connection_context& context)
  {
    rsp.support_flags = m_network_zones.at(context.m_remote_address.get_zone()).m_config.m_support_flags;
    return 1;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  void node_server<t_payload_net_handler>::request_callback(const epee::net_utils::connection_context_base& context)
  {
    m_network_zones.at(context.m_remote_address.get_zone()).m_net_server.get_config_object().request_callback(context.m_connection_id);
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::relay_notify_to_list(int command, epee::levin::message_writer data_buff, std::vector<std::pair<epee::net_utils::zone, boost::uuids::uuid>> connections)
  {
    epee::byte_slice message = data_buff.finalize_notify(command);
    std::sort(connections.begin(), connections.end());
    auto zone = m_network_zones.begin();
    for(const auto& c_id: connections)
    {
      for (;;)
      {
        if (zone == m_network_zones.end())
        {
           MWARNING("Unable to relay all messages, " << epee::net_utils::zone_to_string(c_id.first) << " not available");
           return false;
        }
        if (c_id.first <= zone->first)
          break;

        ++zone;
      }
      if (zone->first == c_id.first)
        zone->second.m_net_server.get_config_object().send(message.clone(), c_id.second);
    }
    return true;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  epee::net_utils::zone node_server<t_payload_net_handler>::send_txs(std::vector<cryptonote::blobdata> txs, const epee::net_utils::zone origin, const boost::uuids::uuid& source, const cryptonote::relay_method tx_relay)
  {
    namespace enet = epee::net_utils;

    const auto send = [&txs, &source, tx_relay] (std::pair<const enet::zone, network_zone>& network)
    {
      if (network.second.m_notifier.send_txs(std::move(txs), source, tx_relay))
        return network.first;
      return enet::zone::invalid;
    };

    if (m_network_zones.empty())
      return enet::zone::invalid;

    if (origin != enet::zone::invalid)
      return send(*m_network_zones.begin()); // send all txs received via p2p over public network

    if (m_network_zones.size() <= 2)
      return send(*m_network_zones.rbegin()); // see static asserts below; sends over anonymity network iff enabled

    /* These checks are to ensure that i2p is highest priority if multiple
       zones are selected. Make sure to update logic if the values cannot be
       in the same relative order. `m_network_zones` must be sorted map too. */
    static_assert(std::is_same<std::underlying_type<enet::zone>::type, std::uint8_t>{}, "expected uint8_t zone");
    static_assert(unsigned(enet::zone::invalid) == 0, "invalid expected to be 0");
    static_assert(unsigned(enet::zone::public_) == 1, "public_ expected to be 1");
    static_assert(unsigned(enet::zone::i2p) == 2, "i2p expected to be 2");
    static_assert(unsigned(enet::zone::tor) == 3, "tor expected to be 3");

    // check for anonymity networks with noise and connections
    for (auto network = ++m_network_zones.begin(); network != m_network_zones.end(); ++network)
    {
      if (enet::zone::tor < network->first)
        break; // unknown network

      const auto status = network->second.m_notifier.get_status();
      if (status.has_noise && status.connections_filled)
        return send(*network);
    }

    // use the anonymity network with outbound support
    for (auto network = ++m_network_zones.begin(); network != m_network_zones.end(); ++network)
    {
      if (enet::zone::tor < network->first)
        break; // unknown network

      if (network->second.m_connect)
        return send(*network);
    }

    // configuration should not allow this scenario
    return enet::zone::invalid;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  void node_server<t_payload_net_handler>::callback(p2p_connection_context& context)
  {
    m_payload_handler.on_callback(context);
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::invoke_notify_to_peer(const int command, epee::levin::message_writer message, const epee::net_utils::connection_context_base& context)
  {
    if(is_filtered_command(context.m_remote_address, command))
      return false;

    network_zone& zone = m_network_zones.at(context.m_remote_address.get_zone());
    int res = zone.m_net_server.get_config_object().send(message.finalize_notify(command), context.m_connection_id);
    return res > 0;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::drop_connection(const epee::net_utils::connection_context_base& context)
  {
    m_network_zones.at(context.m_remote_address.get_zone()).m_net_server.get_config_object().close(context.m_connection_id);
    return true;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler> template<class t_callback>
  bool node_server<t_payload_net_handler>::try_ping(basic_node_data& node_data, p2p_connection_context& context, const t_callback &cb)
  {
    if(!node_data.my_port)
      return false;

    bool address_ok = (context.m_remote_address.get_type_id() == epee::net_utils::ipv4_network_address::get_type_id() || context.m_remote_address.get_type_id() == epee::net_utils::ipv6_network_address::get_type_id());
    CHECK_AND_ASSERT_MES(address_ok, false,
        "Only IPv4 or IPv6 addresses are supported here");

    const epee::net_utils::network_address na = context.m_remote_address;
    std::string ip;
    uint32_t ipv4_addr = 0;
    boost::asio::ip::address_v6 ipv6_addr;
    bool is_ipv4;
    if (na.get_type_id() == epee::net_utils::ipv4_network_address::get_type_id())
    {
      ipv4_addr = na.as<const epee::net_utils::ipv4_network_address>().ip();
      ip = epee::string_tools::get_ip_string_from_int32(ipv4_addr);
      is_ipv4 = true;
    }
    else
    {
      ipv6_addr = na.as<const epee::net_utils::ipv6_network_address>().ip();
      ip = ipv6_addr.to_string();
      is_ipv4 = false;
    }
    network_zone& zone = m_network_zones.at(na.get_zone());

    if(!zone.m_peerlist.is_host_allowed(context.m_remote_address))
      return false;

    std::string port = epee::string_tools::num_to_string_fast(node_data.my_port);

    epee::net_utils::network_address address;
    if (is_ipv4)
    {
      address = epee::net_utils::network_address{epee::net_utils::ipv4_network_address(ipv4_addr, node_data.my_port)};
    }
    else
    {
      address = epee::net_utils::network_address{epee::net_utils::ipv6_network_address(ipv6_addr, node_data.my_port)};
    }
    peerid_type pr = node_data.peer_id;
    bool r = zone.m_net_server.connect_async(ip, port, zone.m_config.m_net_config.ping_connection_timeout, [cb, /*context,*/ address, pr, this](
      const typename net_server::t_connection_context& ping_context,
      const boost::system::error_code& ec)->bool
    {
      if(ec)
      {
        LOG_WARNING_CC(ping_context, "back ping connect failed to " << address.str());
        return false;
      }
      COMMAND_PING::request req;
      COMMAND_PING::response rsp;
      //vc2010 workaround
      /*std::string ip_ = ip;
      std::string port_=port;
      peerid_type pr_ = pr;
      auto cb_ = cb;*/

      // GCC 5.1.0 gives error with second use of uint64_t (peerid_type) variable.
      peerid_type pr_ = pr;

      network_zone& zone = m_network_zones.at(address.get_zone());

      bool inv_call_res = epee::net_utils::async_invoke_remote_command2<COMMAND_PING::response>(ping_context, COMMAND_PING::ID, req, zone.m_net_server.get_config_object(),
        [=](int code, const COMMAND_PING::response& rsp, p2p_connection_context& context)
      {
        if(code <= 0)
        {
          LOG_WARNING_CC(ping_context, "Failed to invoke COMMAND_PING to " << address.str() << "(" << code <<  ", " << epee::levin::get_err_descr(code) << ")");
          return;
        }

        network_zone& zone = m_network_zones.at(address.get_zone());
        if(rsp.status != PING_OK_RESPONSE_STATUS_TEXT || pr != rsp.peer_id)
        {
          LOG_WARNING_CC(ping_context, "back ping invoke wrong response \"" << rsp.status << "\" from" << address.str() << ", hsh_peer_id=" << pr_ << ", rsp.peer_id=" << peerid_to_string(rsp.peer_id));
          zone.m_net_server.get_config_object().close(ping_context.m_connection_id);
          return;
        }
        zone.m_net_server.get_config_object().close(ping_context.m_connection_id);
        cb();
      });

      if(!inv_call_res)
      {
        LOG_WARNING_CC(ping_context, "back ping invoke failed to " << address.str());
        zone.m_net_server.get_config_object().close(ping_context.m_connection_id);
        return false;
      }
      return true;
    }, "0.0.0.0", m_ssl_support);
    if(!r)
    {
      LOG_WARNING_CC(context, "Failed to call connect_async, network error.");
    }
    return r;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::try_get_support_flags(const p2p_connection_context& context, std::function<void(p2p_connection_context&, const uint32_t&)> f)
  {
    if(context.m_remote_address.get_zone() != epee::net_utils::zone::public_)
      return false;

    COMMAND_REQUEST_SUPPORT_FLAGS::request support_flags_request;
    bool r = epee::net_utils::async_invoke_remote_command2<typename COMMAND_REQUEST_SUPPORT_FLAGS::response>
    (
      context,
      COMMAND_REQUEST_SUPPORT_FLAGS::ID, 
      support_flags_request, 
      m_network_zones.at(epee::net_utils::zone::public_).m_net_server.get_config_object(),
      [=](int code, const typename COMMAND_REQUEST_SUPPORT_FLAGS::response& rsp, p2p_connection_context& context_)
      {  
        if(code < 0)
        {
          LOG_WARNING_CC(context_, "COMMAND_REQUEST_SUPPORT_FLAGS invoke failed. (" << code <<  ", " << epee::levin::get_err_descr(code) << ")");
          return;
        }
        
        f(context_, rsp.support_flags);
      },
      P2P_DEFAULT_HANDSHAKE_INVOKE_TIMEOUT
    );

    return r;
  }  
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  int node_server<t_payload_net_handler>::handle_timed_sync(int command, typename COMMAND_TIMED_SYNC::request& arg, typename COMMAND_TIMED_SYNC::response& rsp, p2p_connection_context& context)
  {
    if(!m_payload_handler.process_payload_sync_data(arg.payload_data, context, false))
    {
      LOG_WARNING_CC(context, "Failed to process_payload_sync_data(), dropping connection");
      drop_connection(context);
      return 1;
    }

    //fill response
    const epee::net_utils::zone zone_type = context.m_remote_address.get_zone();
    network_zone& zone = m_network_zones.at(zone_type);

    //will add self to peerlist if in same zone as outgoing later in this function
    const bool outgoing_to_same_zone = !context.m_is_income && zone.m_our_address.get_zone() == zone_type;
    const uint32_t max_peerlist_size = P2P_DEFAULT_PEERS_IN_HANDSHAKE - (outgoing_to_same_zone ? 1 : 0);

    std::vector<peerlist_entry> local_peerlist_new;
    zone.m_peerlist.get_peerlist_head(local_peerlist_new, true, max_peerlist_size);

    //only include out peers we did not already send
    rsp.local_peerlist_new.reserve(local_peerlist_new.size());
    for (auto &pe: local_peerlist_new)
    {
      if (!context.sent_addresses.insert(pe.adr).second)
        continue;
      rsp.local_peerlist_new.push_back(std::move(pe));
    }
    m_payload_handler.get_payload_sync_data(rsp.payload_data);

    /* Tor/I2P nodes receiving connections via forwarding (from tor/i2p daemon)
    do not know the address of the connecting peer. This is relayed to them,
    iff the node has setup an inbound hidden service. The other peer will have
    to use the random peer_id value to link the two. My initial thought is that
    the inbound peer should leave the other side marked as `<unknown tor host>`,
    etc., because someone could give faulty addresses over Tor/I2P to get the
    real peer with that identity banned/blacklisted. */

    if(outgoing_to_same_zone)
      rsp.local_peerlist_new.push_back(peerlist_entry{zone.m_our_address, zone.m_config.m_peer_id, std::time(nullptr)});

    LOG_DEBUG_CC(context, "COMMAND_TIMED_SYNC");
    return 1;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  int node_server<t_payload_net_handler>::handle_handshake(int command, typename COMMAND_HANDSHAKE::request& arg, typename COMMAND_HANDSHAKE::response& rsp, p2p_connection_context& context)
  {
    if(arg.node_data.network_id != m_network_id)
    {

      LOG_INFO_CC(context, "WRONG NETWORK AGENT CONNECTED! id=" << arg.node_data.network_id);
      drop_connection(context);
      add_host_fail(context.m_remote_address);
      return 1;
    }

    if(!context.m_is_income)
    {
      LOG_WARNING_CC(context, "COMMAND_HANDSHAKE came not from incoming connection");
      drop_connection(context);
      add_host_fail(context.m_remote_address);
      return 1;
    }

    if(context.peer_id)
    {
      LOG_WARNING_CC(context, "COMMAND_HANDSHAKE came, but seems that connection already have associated peer_id (double COMMAND_HANDSHAKE?)");
      drop_connection(context);
      return 1;
    }

    const auto azone = context.m_remote_address.get_zone();
    network_zone& zone = m_network_zones.at(azone);

    // test only the remote end's zone, otherwise an attacker could connect to you on clearnet
    // and pass in a tor connection's peer id, and deduce the two are the same if you reject it
    if(azone == epee::net_utils::zone::public_ && arg.node_data.peer_id == zone.m_config.m_peer_id)
    {
      LOG_DEBUG_CC(context, "Connection to self detected, dropping connection");
      drop_connection(context);
      return 1;
    }

    if (zone.m_current_number_of_in_peers >= zone.m_config.m_net_config.max_in_connection_count) // in peers limit
    {
      LOG_WARNING_CC(context, "COMMAND_HANDSHAKE came, but already have max incoming connections, so dropping this one.");
      drop_connection(context);
      return 1;
    }

    if(!m_payload_handler.process_payload_sync_data(arg.payload_data, context, true))
    {
      LOG_WARNING_CC(context, "COMMAND_HANDSHAKE came, but process_payload_sync_data returned false, dropping connection.");
      drop_connection(context);
      return 1;
    }

    zone.m_notifier.on_handshake_complete(context.m_connection_id, context.m_is_income);

    if(has_too_many_connections(context.m_remote_address))
    {
      LOG_PRINT_CCONTEXT_L1("CONNECTION FROM " << context.m_remote_address.host_str() << " REFUSED, too many connections from the same address");
      drop_connection(context);
      return 1;
    }

    //associate peer_id with this connection
    context.peer_id = arg.node_data.peer_id;
    context.m_in_timedsync = false;
    context.m_rpc_port = arg.node_data.rpc_port;
    context.m_rpc_credits_per_hash = arg.node_data.rpc_credits_per_hash;
    context.support_flags = arg.node_data.support_flags;

    if(arg.node_data.my_port && zone.m_can_pingback)
    {
      peerid_type peer_id_l = arg.node_data.peer_id;
      uint32_t port_l = arg.node_data.my_port;
      //try ping to be sure that we can add this peer to peer_list
      try_ping(arg.node_data, context, [peer_id_l, port_l, context, this]()
      {
        CHECK_AND_ASSERT_MES((context.m_remote_address.get_type_id() == epee::net_utils::ipv4_network_address::get_type_id() || context.m_remote_address.get_type_id() == epee::net_utils::ipv6_network_address::get_type_id()), void(),
            "Only IPv4 or IPv6 addresses are supported here");
        //called only(!) if success pinged, update local peerlist
        peerlist_entry pe;
        const epee::net_utils::network_address na = context.m_remote_address;
        if (context.m_remote_address.get_type_id() == epee::net_utils::ipv4_network_address::get_type_id())
        {
          pe.adr = epee::net_utils::ipv4_network_address(na.as<epee::net_utils::ipv4_network_address>().ip(), port_l);
        }
        else
        {
          pe.adr = epee::net_utils::ipv6_network_address(na.as<epee::net_utils::ipv6_network_address>().ip(), port_l);
        }
        time_t last_seen;
        time(&last_seen);
        pe.last_seen = static_cast<int64_t>(last_seen);
        pe.id = peer_id_l;
        pe.pruning_seed = context.m_pruning_seed;
        pe.rpc_port = context.m_rpc_port;
        pe.rpc_credits_per_hash = context.m_rpc_credits_per_hash;
        this->m_network_zones.at(context.m_remote_address.get_zone()).m_peerlist.append_with_peer_white(pe);
        LOG_DEBUG_CC(context, "PING SUCCESS " << context.m_remote_address.host_str() << ":" << port_l);
      });
    }
    
    if (context.support_flags == 0)
      try_get_support_flags(context, [](p2p_connection_context& flags_context, const uint32_t& support_flags) 
      {
        flags_context.support_flags = support_flags;
      });

    //fill response
    zone.m_peerlist.get_peerlist_head(rsp.local_peerlist_new, true);
    for (const auto &e: rsp.local_peerlist_new)
      context.sent_addresses.insert(e.adr);
    get_local_node_data(rsp.node_data, zone);
    m_payload_handler.get_payload_sync_data(rsp.payload_data);
    LOG_DEBUG_CC(context, "COMMAND_HANDSHAKE");
    return 1;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  int node_server<t_payload_net_handler>::handle_ping(int command, COMMAND_PING::request& arg, COMMAND_PING::response& rsp, p2p_connection_context& context)
  {
    LOG_DEBUG_CC(context, "COMMAND_PING");
    rsp.status = PING_OK_RESPONSE_STATUS_TEXT;
    rsp.peer_id = m_network_zones.at(context.m_remote_address.get_zone()).m_config.m_peer_id;
    return 1;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::log_peerlist()
  {
    std::vector<peerlist_entry> pl_white;
    std::vector<peerlist_entry> pl_gray;
    for (auto& zone : m_network_zones)
      zone.second.m_peerlist.get_peerlist(pl_gray, pl_white);
    MINFO(ENDL << "Peerlist white:" << ENDL << print_peerlist_to_string(pl_white) << ENDL << "Peerlist gray:" << ENDL << print_peerlist_to_string(pl_gray) );
    return true;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::log_connections()
  {
    MINFO("Connections: \r\n" << print_connections_container() );
    return true;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  std::string node_server<t_payload_net_handler>::print_connections_container()
  {

    std::stringstream ss;
    for (auto& zone : m_network_zones)
    {
      zone.second.m_net_server.get_config_object().foreach_connection([&](const p2p_connection_context& cntxt)
      {
        ss << cntxt.m_remote_address.str()
          << " \t\tpeer_id " << peerid_to_string(cntxt.peer_id)
          << " \t\tconn_id " << cntxt.m_connection_id << (cntxt.m_is_income ? " INC":" OUT")
          << std::endl;
        return true;
      });
    }
    std::string s = ss.str();
    return s;
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  void node_server<t_payload_net_handler>::on_connection_new(p2p_connection_context& context)
  {
    MINFO("["<< epee::net_utils::print_connection_context(context) << "] NEW CONNECTION");
  }
  //-----------------------------------------------------------------------------------
  template<class t_payload_net_handler>
  void node_server<t_payload_net_handler>::on_connection_close(p2p_connection_context& context)
  {
    network_zone& zone = m_network_zones.at(context.m_remote_address.get_zone());
    if (!zone.m_net_server.is_stop_signal_sent() && !context.m_is_income) {
      epee::net_utils::network_address na = AUTO_VAL_INIT(na);
      na = context.m_remote_address;

      zone.m_peerlist.remove_from_peer_anchor(na);
    }

    if (!zone.m_net_server.is_stop_signal_sent()) {
      zone.m_notifier.on_connection_close(context.m_connection_id);
    }
    m_payload_handler.on_connection_close(context);

    MINFO("["<< epee::net_utils::print_connection_context(context) << "] CLOSE CONNECTION");
  }

  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::is_priority_node(const epee::net_utils::network_address& na)
  {
    return (std::find(m_priority_peers.begin(), m_priority_peers.end(), na) != m_priority_peers.end()) || (std::find(m_exclusive_peers.begin(), m_exclusive_peers.end(), na) != m_exclusive_peers.end());
  }

  template<class t_payload_net_handler> template <class Container>
  bool node_server<t_payload_net_handler>::connect_to_peerlist(const Container& peers)
  {
    const network_zone& public_zone = m_network_zones.at(epee::net_utils::zone::public_);
    for(const epee::net_utils::network_address& na: peers)
    {
      if(public_zone.m_net_server.is_stop_signal_sent())
        return false;

      if(is_addr_connected(na))
        continue;

      try_to_connect_and_handshake_with_new_peer(na);
    }

    return true;
  }

  template<class t_payload_net_handler> template <class Container>
  bool node_server<t_payload_net_handler>::parse_peers_and_add_to_container(const boost::program_options::variables_map& vm, const command_line::arg_descriptor<std::vector<std::string> > & arg, Container& container)
  {
    std::vector<std::string> perrs = command_line::get_arg(vm, arg);

    for(const std::string& pr_str: perrs)
    {
      const uint16_t default_port = cryptonote::get_config(m_nettype).P2P_DEFAULT_PORT;
      expect<epee::net_utils::network_address> adr = net::get_network_address(pr_str, default_port);
      if (adr)
      {
        add_zone(adr->get_zone());
        container.push_back(std::move(*adr));
        continue;
      }
      std::vector<epee::net_utils::network_address> resolved_addrs;
      bool r = append_net_address(resolved_addrs, pr_str, default_port);
      CHECK_AND_ASSERT_MES(r, false, "Failed to parse or resolve address from string: " << pr_str);
      for (const epee::net_utils::network_address& addr : resolved_addrs)
      {
        container.push_back(addr);
      }
    }

    return true;
  }

  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::set_max_out_peers(network_zone& zone, int64_t max)
  {
    if(max == -1) {
      zone.m_config.m_net_config.max_out_connection_count = P2P_DEFAULT_CONNECTIONS_COUNT;
      return true;
    }
    zone.m_config.m_net_config.max_out_connection_count = max;
    return true;
  }

  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::set_max_in_peers(network_zone& zone, int64_t max)
  {
    zone.m_config.m_net_config.max_in_connection_count = max;
    return true;
  }

  template<class t_payload_net_handler>
  void node_server<t_payload_net_handler>::change_max_out_public_peers(size_t count)
  {
    auto public_zone = m_network_zones.find(epee::net_utils::zone::public_);
    if (public_zone != m_network_zones.end())
    {
      const auto current = public_zone->second.m_net_server.get_config_object().get_out_connections_count();
      public_zone->second.m_config.m_net_config.max_out_connection_count = count;
      if(current > count)
        public_zone->second.m_net_server.get_config_object().del_out_connections(current - count);
      m_payload_handler.set_max_out_peers(epee::net_utils::zone::public_, count);
    }
  }

  template<class t_payload_net_handler>
  uint32_t node_server<t_payload_net_handler>::get_max_out_public_peers() const
  {
    const auto public_zone = m_network_zones.find(epee::net_utils::zone::public_);
    if (public_zone == m_network_zones.end())
      return 0;
    return public_zone->second.m_config.m_net_config.max_out_connection_count;
  }

  template<class t_payload_net_handler>
  void node_server<t_payload_net_handler>::change_max_in_public_peers(size_t count)
  {
    auto public_zone = m_network_zones.find(epee::net_utils::zone::public_);
    if (public_zone != m_network_zones.end())
    {
      const auto current = public_zone->second.m_net_server.get_config_object().get_in_connections_count();
      public_zone->second.m_config.m_net_config.max_in_connection_count = count;
      if(current > count)
        public_zone->second.m_net_server.get_config_object().del_in_connections(current - count);
    }
  }

  template<class t_payload_net_handler>
  uint32_t node_server<t_payload_net_handler>::get_max_in_public_peers() const
  {
    const auto public_zone = m_network_zones.find(epee::net_utils::zone::public_);
    if (public_zone == m_network_zones.end())
      return 0;
    return public_zone->second.m_config.m_net_config.max_in_connection_count;
  }

  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::set_tos_flag(const boost::program_options::variables_map& vm, int flag)
  {
    if(flag==-1){
      return true;
    }
    epee::net_utils::connection<epee::levin::async_protocol_handler<p2p_connection_context> >::set_tos_flag(flag);
    _dbg1("Set ToS flag  " << flag);
    return true;
  }

  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::set_rate_up_limit(const boost::program_options::variables_map& vm, int64_t limit)
  {
    this->islimitup=(limit != -1) && (limit != default_limit_up);

    if (limit==-1) {
      limit=default_limit_up;
    }

    epee::net_utils::connection<epee::levin::async_protocol_handler<p2p_connection_context> >::set_rate_up_limit( limit );
    MINFO("Set limit-up to " << limit << " kB/s");
    return true;
  }

  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::set_rate_down_limit(const boost::program_options::variables_map& vm, int64_t limit)
  {
    this->islimitdown=(limit != -1) && (limit != default_limit_down);
    if(limit==-1) {
      limit=default_limit_down;
    }
    epee::net_utils::connection<epee::levin::async_protocol_handler<p2p_connection_context> >::set_rate_down_limit( limit );
    MINFO("Set limit-down to " << limit << " kB/s");
    return true;
  }

  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::set_rate_limit(const boost::program_options::variables_map& vm, int64_t limit)
  {
    int64_t limit_up = 0;
    int64_t limit_down = 0;

    if(limit == -1)
    {
      limit_up = default_limit_up;
      limit_down = default_limit_down;
    }
    else
    {
      limit_up = limit;
      limit_down = limit;
    }
    if(!this->islimitup) {
      epee::net_utils::connection<epee::levin::async_protocol_handler<p2p_connection_context> >::set_rate_up_limit(limit_up);
      MINFO("Set limit-up to " << limit_up << " kB/s");
    }
    if(!this->islimitdown) {
      epee::net_utils::connection<epee::levin::async_protocol_handler<p2p_connection_context> >::set_rate_down_limit(limit_down);
      MINFO("Set limit-down to " << limit_down << " kB/s");
    }

    return true;
  }

  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::has_too_many_connections(const epee::net_utils::network_address &address)
  {
    if (address.get_zone() != epee::net_utils::zone::public_)
      return false; // Unable to determine how many connections from host

    uint32_t count = 0;

    m_network_zones.at(epee::net_utils::zone::public_).m_net_server.get_config_object().foreach_connection([&](const p2p_connection_context& cntxt)
    {
      if (cntxt.m_is_income && cntxt.m_remote_address.is_same_host(address)) {
        count++;

        if (count > max_connections) {
          return false;
        }
      }

      return true;
    });

    return count > max_connections;
  }

  template<class t_payload_net_handler>
  bool node_server<t_payload_net_handler>::gray_peerlist_housekeeping()
  {
    if (m_offline) return true;
    if (!m_exclusive_peers.empty()) return true;

    for (auto& zone : m_network_zones)
    {
      if (m_payload_handler.needs_new_sync_connections(zone.first))
        continue;

      if (zone.second.m_net_server.is_stop_signal_sent())
        return false;

      if (zone.second.m_connect == nullptr)
        continue;

      peerlist_entry pe{};
      if (!zone.second.m_peerlist.get_random_gray_peer(pe))
        continue;

      if (!check_connection_and_handshake_with_peer(pe.adr, pe.last_seen))
      {
        zone.second.m_peerlist.remove_from_peer_gray(pe);
        LOG_PRINT_L2("PEER EVICTED FROM GRAY PEER LIST: address: " << pe.adr.host_str() << " Peer ID: " << peerid_to_string(pe.id));
      }
      else
      {
        zone.second.m_peerlist.set_peer_just_seen(pe.id, pe.adr, pe.pruning_seed, pe.rpc_port, pe.rpc_credits_per_hash);
        LOG_PRINT_L2("PEER PROMOTED TO WHITE PEER LIST IP address: " << pe.adr.host_str() << " Peer ID: " << peerid_to_string(pe.id));
      }
    }
    return true;
  }

  template<class t_payload_net_handler>
  void node_server<t_payload_net_handler>::add_used_stripe_peer(const typename t_payload_net_handler::connection_context &context)
  {
    const uint32_t stripe = tools::get_pruning_stripe(context.m_pruning_seed);
    if (stripe == 0 || stripe > (1ul << CRYPTONOTE_PRUNING_LOG_STRIPES))
      return;
    const uint32_t index = stripe - 1;
    CRITICAL_REGION_LOCAL(m_used_stripe_peers_mutex);
    MINFO("adding stripe " << stripe << " peer: " << context.m_remote_address.str());
    m_used_stripe_peers[index].erase(std::remove_if(m_used_stripe_peers[index].begin(), m_used_stripe_peers[index].end(),
        [&context](const epee::net_utils::network_address &na){ return context.m_remote_address == na; }), m_used_stripe_peers[index].end());
    m_used_stripe_peers[index].push_back(context.m_remote_address);
  }

  template<class t_payload_net_handler>
  void node_server<t_payload_net_handler>::remove_used_stripe_peer(const typename t_payload_net_handler::connection_context &context)
  {
    const uint32_t stripe = tools::get_pruning_stripe(context.m_pruning_seed);
    if (stripe == 0 || stripe > (1ul << CRYPTONOTE_PRUNING_LOG_STRIPES))
      return;
    const uint32_t index = stripe - 1;
    CRITICAL_REGION_LOCAL(m_used_stripe_peers_mutex);
    MINFO("removing stripe " << stripe << " peer: " << context.m_remote_address.str());
    m_used_stripe_peers[index].erase(std::remove_if(m_used_stripe_peers[index].begin(), m_used_stripe_peers[index].end(),
        [&context](const epee::net_utils::network_address &na){ return context.m_remote_address == na; }), m_used_stripe_peers[index].end());
  }

  template<class t_payload_net_handler>
  void node_server<t_payload_net_handler>::clear_used_stripe_peers()
  {
    CRITICAL_REGION_LOCAL(m_used_stripe_peers_mutex);
    MINFO("clearing used stripe peers");
    for (auto &e: m_used_stripe_peers)
      e.clear();
  }

  template<class t_payload_net_handler>
  void node_server<t_payload_net_handler>::add_upnp_port_mapping_impl(uint32_t port, bool ipv6) // if ipv6 false, do ipv4
  {
    std::string ipversion = ipv6 ? "(IPv6)" : "(IPv4)";
    MDEBUG("Attempting to add IGD port mapping " << ipversion << ".");
    int result;
    const int ipv6_arg = ipv6 ? 1 : 0;

#if MINIUPNPC_API_VERSION > 13
    // default according to miniupnpc.h
    unsigned char ttl = 2;
    UPNPDev* deviceList = upnpDiscover(1000, NULL, NULL, 0, ipv6_arg, ttl, &result);
#else
    UPNPDev* deviceList = upnpDiscover(1000, NULL, NULL, 0, ipv6_arg, &result);
#endif
    UPNPUrls urls;
    IGDdatas igdData;
    char lanAddress[64];
    result = UPNP_GetValidIGD(deviceList, &urls, &igdData, lanAddress, sizeof lanAddress);
    freeUPNPDevlist(deviceList);
    if (result > 0) {
      if (result == 1) {
        std::ostringstream portString;
        portString << port;

        // Delete the port mapping before we create it, just in case we have dangling port mapping from the daemon not being shut down correctly
        UPNP_DeletePortMapping(urls.controlURL, igdData.first.servicetype, portString.str().c_str(), "TCP", 0);

        int portMappingResult;
        portMappingResult = UPNP_AddPortMapping(urls.controlURL, igdData.first.servicetype, portString.str().c_str(), portString.str().c_str(), lanAddress, CRYPTONOTE_NAME, "TCP", 0, "0");
        if (portMappingResult != 0) {
          LOG_ERROR("UPNP_AddPortMapping failed, error: " << strupnperror(portMappingResult));
        } else {
          MLOG_GREEN(el::Level::Info, "Added IGD port mapping.");
        }
      } else if (result == 2) {
        MWARNING("IGD was found but reported as not connected.");
      } else if (result == 3) {
        MWARNING("UPnP device was found but not recognized as IGD.");
      } else {
        MWARNING("UPNP_GetValidIGD returned an unknown result code.");
      }

      FreeUPNPUrls(&urls);
    } else {
      MINFO("No IGD was found.");
    }
  }

  template<class t_payload_net_handler>
  void node_server<t_payload_net_handler>::add_upnp_port_mapping_v4(uint32_t port)
  {
    add_upnp_port_mapping_impl(port, false);
  }

  template<class t_payload_net_handler>
  void node_server<t_payload_net_handler>::add_upnp_port_mapping_v6(uint32_t port)
  {
    add_upnp_port_mapping_impl(port, true);
  }

  template<class t_payload_net_handler>
  void node_server<t_payload_net_handler>::add_upnp_port_mapping(uint32_t port, bool ipv4, bool ipv6)
  {
    if (ipv4) add_upnp_port_mapping_v4(port);
    if (ipv6) add_upnp_port_mapping_v6(port);
  }


  template<class t_payload_net_handler>
  void node_server<t_payload_net_handler>::delete_upnp_port_mapping_impl(uint32_t port, bool ipv6)
  {
    std::string ipversion = ipv6 ? "(IPv6)" : "(IPv4)";
    MDEBUG("Attempting to delete IGD port mapping " << ipversion << ".");
    int result;
    const int ipv6_arg = ipv6 ? 1 : 0;
#if MINIUPNPC_API_VERSION > 13
    // default according to miniupnpc.h
    unsigned char ttl = 2;
    UPNPDev* deviceList = upnpDiscover(1000, NULL, NULL, 0, ipv6_arg, ttl, &result);
#else
    UPNPDev* deviceList = upnpDiscover(1000, NULL, NULL, 0, ipv6_arg, &result);
#endif
    UPNPUrls urls;
    IGDdatas igdData;
    char lanAddress[64];
    result = UPNP_GetValidIGD(deviceList, &urls, &igdData, lanAddress, sizeof lanAddress);
    freeUPNPDevlist(deviceList);
    if (result > 0) {
      if (result == 1) {
        std::ostringstream portString;
        portString << port;

        int portMappingResult;
        portMappingResult = UPNP_DeletePortMapping(urls.controlURL, igdData.first.servicetype, portString.str().c_str(), "TCP", 0);
        if (portMappingResult != 0) {
          LOG_ERROR("UPNP_DeletePortMapping failed, error: " << strupnperror(portMappingResult));
        } else {
          MLOG_GREEN(el::Level::Info, "Deleted IGD port mapping.");
        }
      } else if (result == 2) {
        MWARNING("IGD was found but reported as not connected.");
      } else if (result == 3) {
        MWARNING("UPnP device was found but not recognized as IGD.");
      } else {
        MWARNING("UPNP_GetValidIGD returned an unknown result code.");
      }

      FreeUPNPUrls(&urls);
    } else {
      MINFO("No IGD was found.");
    }
  }

  template<class t_payload_net_handler>
  void node_server<t_payload_net_handler>::delete_upnp_port_mapping_v4(uint32_t port)
  {
    delete_upnp_port_mapping_impl(port, false);
  }

  template<class t_payload_net_handler>
  void node_server<t_payload_net_handler>::delete_upnp_port_mapping_v6(uint32_t port)
  {
    delete_upnp_port_mapping_impl(port, true);
  }

  template<class t_payload_net_handler>
  void node_server<t_payload_net_handler>::delete_upnp_port_mapping(uint32_t port)
  {
    delete_upnp_port_mapping_v4(port);
    delete_upnp_port_mapping_v6(port);
  }

  template<typename t_payload_net_handler>
  boost::optional<p2p_connection_context_t<typename t_payload_net_handler::connection_context>>
  node_server<t_payload_net_handler>::socks_connect(network_zone& zone, const epee::net_utils::network_address& remote, epee::net_utils::ssl_support_t ssl_support)
  {
    auto result = socks_connect_internal(zone.m_net_server.get_stop_signal(), zone.m_net_server.get_io_service(), zone.m_proxy_address, remote);
    if (result) // if no error
    {
      p2p_connection_context context{};
      if (zone.m_net_server.add_connection(context, std::move(*result), remote, ssl_support))
        return {std::move(context)};
    }
    return boost::none;
  }

  template<typename t_payload_net_handler>
  boost::optional<p2p_connection_context_t<typename t_payload_net_handler::connection_context>>
  node_server<t_payload_net_handler>::public_connect(network_zone& zone, epee::net_utils::network_address const& na, epee::net_utils::ssl_support_t ssl_support)
  {
    bool is_ipv4 = na.get_type_id() == epee::net_utils::ipv4_network_address::get_type_id();
    bool is_ipv6 = na.get_type_id() == epee::net_utils::ipv6_network_address::get_type_id();
    CHECK_AND_ASSERT_MES(is_ipv4 || is_ipv6, boost::none,
      "Only IPv4 or IPv6 addresses are supported here");

    std::string address;
    std::string port;

    if (is_ipv4)
    {
      const epee::net_utils::ipv4_network_address &ipv4 = na.as<const epee::net_utils::ipv4_network_address>();
      address = epee::string_tools::get_ip_string_from_int32(ipv4.ip());
      port = epee::string_tools::num_to_string_fast(ipv4.port());
    }
    else if (is_ipv6)
    {
      const epee::net_utils::ipv6_network_address &ipv6 = na.as<const epee::net_utils::ipv6_network_address>();
      address = ipv6.ip().to_string();
      port = epee::string_tools::num_to_string_fast(ipv6.port());
    }
    else
    {
      LOG_ERROR("Only IPv4 or IPv6 addresses are supported here");
      return boost::none;
    }

    typename net_server::t_connection_context con{};
    const bool res = zone.m_net_server.connect(address, port,
      zone.m_config.m_net_config.connection_timeout,
      con, "0.0.0.0", ssl_support);

    if (res)
      return {std::move(con)};
    return boost::none;
  }
}
