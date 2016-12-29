// Copyright (c) 2014-2016, The Monero Project
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

#pragma once
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/bimap.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/global_fun.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/serialization/version.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/serialization/map.hpp>

#include "cryptonote_config.h"
#include "warnings.h"
#include "net/levin_server_cp2.h"
#include "p2p_protocol_defs.h"
#include "storages/levin_abstract_invoke2.h"
#include "net_peerlist.h"
#include "math_helper.h"
#include "net_node_common.h"
#include "common/command_line.h"

PUSH_WARNINGS
DISABLE_VS_WARNINGS(4355)

namespace nodetool
{
  template<class base_type>
  struct p2p_connection_context_t: base_type //t_payload_net_handler::connection_context //public net_utils::connection_context_base
  {
    peerid_type peer_id;
    uint32_t support_flags;
  };

  template<class t_payload_net_handler>
  class node_server: public epee::levin::levin_commands_handler<p2p_connection_context_t<typename t_payload_net_handler::connection_context> >,
                     public i_p2p_endpoint<typename t_payload_net_handler::connection_context>,
                     public epee::net_utils::i_connection_filter
  {
    struct by_conn_id{};
    struct by_peer_id{};
    struct by_addr{};

    typedef p2p_connection_context_t<typename t_payload_net_handler::connection_context> p2p_connection_context;

    typedef COMMAND_HANDSHAKE_T<typename t_payload_net_handler::payload_type> COMMAND_HANDSHAKE;
    typedef COMMAND_TIMED_SYNC_T<typename t_payload_net_handler::payload_type> COMMAND_TIMED_SYNC;

  public:
    typedef t_payload_net_handler payload_net_handler;

    node_server(t_payload_net_handler& payload_handler)
      :m_payload_handler(payload_handler),
    m_current_number_of_out_peers(0),
    m_allow_local_ip(false),
    m_hide_my_port(false),
    m_no_igd(false),
    m_offline(false),
    m_save_graph(false),
    is_closing(false),
    m_net_server( epee::net_utils::e_connection_type_P2P ) // this is a P2P connection of the main p2p node server, because this is class node_server<>
    {}
    virtual ~node_server()
    {}

    static void init_options(boost::program_options::options_description& desc);

    bool run();
    bool init(const boost::program_options::variables_map& vm);
    bool deinit();
    bool send_stop_signal();
    uint32_t get_this_peer_port(){return m_listenning_port;}
    t_payload_net_handler& get_payload_object();

    template <class Archive, class t_version_type>
    void serialize(Archive &a,  const t_version_type ver)
    {
      a & m_peerlist;
      a & m_config.m_peer_id;
    }
    // debug functions
    bool log_peerlist();
    bool log_connections();
    virtual uint64_t get_connections_count();
    size_t get_outgoing_connections_count();
    peerlist_manager& get_peerlist_manager(){return m_peerlist;}
    void delete_connections(size_t count);
    virtual bool block_ip(uint32_t adress, time_t seconds = P2P_IP_BLOCKTIME);
    virtual bool unblock_ip(uint32_t address);
    virtual std::map<uint32_t, time_t> get_blocked_ips() { CRITICAL_REGION_LOCAL(m_blocked_ips_lock); return m_blocked_ips; }
  private:
    const std::vector<std::string> m_seed_nodes_list =
    { "seeds.sumoseeds.bid"
    , "seeds.sumoseeds.win"
    , "seeds.sumoseeds2.bid"
    , "seeds.sumoseeds2.win"
    };

    bool islimitup=false;
    bool islimitdown=false;

    typedef COMMAND_REQUEST_STAT_INFO_T<typename t_payload_net_handler::stat_info> COMMAND_REQUEST_STAT_INFO;

    CHAIN_LEVIN_INVOKE_MAP2(p2p_connection_context); //move levin_commands_handler interface invoke(...) callbacks into invoke map
    CHAIN_LEVIN_NOTIFY_MAP2(p2p_connection_context); //move levin_commands_handler interface notify(...) callbacks into nothing

    BEGIN_INVOKE_MAP2(node_server)
      HANDLE_INVOKE_T2(COMMAND_HANDSHAKE, &node_server::handle_handshake)
      HANDLE_INVOKE_T2(COMMAND_TIMED_SYNC, &node_server::handle_timed_sync)
      HANDLE_INVOKE_T2(COMMAND_PING, &node_server::handle_ping)
#ifdef ALLOW_DEBUG_COMMANDS
      HANDLE_INVOKE_T2(COMMAND_REQUEST_STAT_INFO, &node_server::handle_get_stat_info)
      HANDLE_INVOKE_T2(COMMAND_REQUEST_NETWORK_STATE, &node_server::handle_get_network_state)
      HANDLE_INVOKE_T2(COMMAND_REQUEST_PEER_ID, &node_server::handle_get_peer_id)
#endif
      HANDLE_INVOKE_T2(COMMAND_REQUEST_SUPPORT_FLAGS, &node_server::handle_get_support_flags)
      CHAIN_INVOKE_MAP_TO_OBJ_FORCE_CONTEXT(m_payload_handler, typename t_payload_net_handler::connection_context&)
    END_INVOKE_MAP2()

    //----------------- commands handlers ----------------------------------------------
    int handle_handshake(int command, typename COMMAND_HANDSHAKE::request& arg, typename COMMAND_HANDSHAKE::response& rsp, p2p_connection_context& context);
    int handle_timed_sync(int command, typename COMMAND_TIMED_SYNC::request& arg, typename COMMAND_TIMED_SYNC::response& rsp, p2p_connection_context& context);
    int handle_ping(int command, COMMAND_PING::request& arg, COMMAND_PING::response& rsp, p2p_connection_context& context);
#ifdef ALLOW_DEBUG_COMMANDS
    int handle_get_stat_info(int command, typename COMMAND_REQUEST_STAT_INFO::request& arg, typename COMMAND_REQUEST_STAT_INFO::response& rsp, p2p_connection_context& context);
    int handle_get_network_state(int command, COMMAND_REQUEST_NETWORK_STATE::request& arg, COMMAND_REQUEST_NETWORK_STATE::response& rsp, p2p_connection_context& context);
    int handle_get_peer_id(int command, COMMAND_REQUEST_PEER_ID::request& arg, COMMAND_REQUEST_PEER_ID::response& rsp, p2p_connection_context& context);
#endif
    int handle_get_support_flags(int command, COMMAND_REQUEST_SUPPORT_FLAGS::request& arg, COMMAND_REQUEST_SUPPORT_FLAGS::response& rsp, p2p_connection_context& context);
    bool init_config();
    bool make_default_config();
    bool store_config();
    bool check_trust(const proof_of_trust& tr);


    //----------------- levin_commands_handler -------------------------------------------------------------
    virtual void on_connection_new(p2p_connection_context& context);
    virtual void on_connection_close(p2p_connection_context& context);
    virtual void callback(p2p_connection_context& context);
    //----------------- i_p2p_endpoint -------------------------------------------------------------
    virtual bool relay_notify_to_list(int command, const std::string& data_buff, const std::list<boost::uuids::uuid> &connections);
    virtual bool relay_notify_to_all(int command, const std::string& data_buff, const epee::net_utils::connection_context_base& context);
    virtual bool invoke_command_to_peer(int command, const std::string& req_buff, std::string& resp_buff, const epee::net_utils::connection_context_base& context);
    virtual bool invoke_notify_to_peer(int command, const std::string& req_buff, const epee::net_utils::connection_context_base& context);
    virtual bool drop_connection(const epee::net_utils::connection_context_base& context);
    virtual void request_callback(const epee::net_utils::connection_context_base& context);
    virtual void for_each_connection(std::function<bool(typename t_payload_net_handler::connection_context&, peerid_type, uint32_t)> f);
    virtual bool add_ip_fail(uint32_t address);
    //----------------- i_connection_filter  --------------------------------------------------------
    virtual bool is_remote_ip_allowed(uint32_t adress);
    //-----------------------------------------------------------------------------------------------
    bool parse_peer_from_string(nodetool::net_address& pe, const std::string& node_addr);
    bool handle_command_line(
        const boost::program_options::variables_map& vm
      , bool testnet
      );
    bool idle_worker();
    bool handle_remote_peerlist(const std::list<peerlist_entry>& peerlist, time_t local_time, const epee::net_utils::connection_context_base& context);
    bool get_local_node_data(basic_node_data& node_data);
    //bool get_local_handshake_data(handshake_data& hshd);

    bool merge_peerlist_with_local(const std::list<peerlist_entry>& bs);
    bool fix_time_delta(std::list<peerlist_entry>& local_peerlist, time_t local_time, int64_t& delta);

    bool connections_maker();
    bool peer_sync_idle_maker();
    bool do_handshake_with_peer(peerid_type& pi, p2p_connection_context& context, bool just_take_peerlist = false);
    bool do_peer_timed_sync(const epee::net_utils::connection_context_base& context, peerid_type peer_id);

    bool make_new_connection_from_peerlist(bool use_white_list);
    bool try_to_connect_and_handshake_with_new_peer(const net_address& na, bool just_take_peerlist = false, uint64_t last_seen_stamp = 0, bool white = true);
    size_t get_random_index_with_fixed_probability(size_t max_index);
    bool is_peer_used(const peerlist_entry& peer);
    bool is_addr_connected(const net_address& peer);
    template<class t_callback>
    bool try_ping(basic_node_data& node_data, p2p_connection_context& context, t_callback cb);
    bool try_get_support_flags(const p2p_connection_context& context, std::function<void(p2p_connection_context&, const uint32_t&)> f);
    bool make_expected_connections_count(bool white_list, size_t expected_connections);
    void cache_connect_fail_info(const net_address& addr);
    bool is_addr_recently_failed(const net_address& addr);
    bool is_priority_node(const net_address& na);

    template <class Container>
    bool connect_to_peerlist(const Container& peers);

    template <class Container>
    bool parse_peers_and_add_to_container(const boost::program_options::variables_map& vm, const command_line::arg_descriptor<std::vector<std::string> > & arg, Container& container);

    bool set_max_out_peers(const boost::program_options::variables_map& vm, int64_t max);
    bool set_tos_flag(const boost::program_options::variables_map& vm, int limit);

    bool set_rate_up_limit(const boost::program_options::variables_map& vm, int64_t limit);
    bool set_rate_down_limit(const boost::program_options::variables_map& vm, int64_t limit);
    bool set_rate_limit(const boost::program_options::variables_map& vm, int64_t limit);

    void kill() { ///< will be called e.g. from deinit()
      _info("Killing the net_node");
      is_closing = true;
      if(mPeersLoggerThread != nullptr)
        mPeersLoggerThread->join(); // make sure the thread finishes
      _info("Joined extra background net_node threads");
    }

    //debug functions
    std::string print_connections_container();


    typedef epee::net_utils::boosted_tcp_server<epee::levin::async_protocol_handler<p2p_connection_context> > net_server;

    struct config
    {
      network_config m_net_config;
      uint64_t m_peer_id;
      uint32_t m_support_flags;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(m_net_config)
        KV_SERIALIZE(m_peer_id)
        KV_SERIALIZE(m_support_flags)
      END_KV_SERIALIZE_MAP()
    };

  public:
    config m_config; // TODO was private, add getters?
    std::atomic<unsigned int> m_current_number_of_out_peers;

    void set_save_graph(bool save_graph)
    {
      m_save_graph = save_graph;
      epee::net_utils::connection_basic::set_save_graph(save_graph);
    }
  private:
    std::string m_config_folder;

    bool m_have_address;
    bool m_first_connection_maker_call;
    uint32_t m_listenning_port;
    uint32_t m_external_port;
    uint32_t m_ip_address;
    bool m_allow_local_ip;
    bool m_hide_my_port;
    bool m_no_igd;
    bool m_offline;
    std::atomic<bool> m_save_graph;
    std::atomic<bool> is_closing;
    std::unique_ptr<boost::thread> mPeersLoggerThread;
    //critical_section m_connections_lock;
    //connections_indexed_container m_connections;

    t_payload_net_handler& m_payload_handler;
    peerlist_manager m_peerlist;

    epee::math_helper::once_a_time_seconds<P2P_DEFAULT_HANDSHAKE_INTERVAL> m_peer_handshake_idle_maker_interval;
    epee::math_helper::once_a_time_seconds<1> m_connections_maker_interval;
    epee::math_helper::once_a_time_seconds<60*30, false> m_peerlist_store_interval;

    std::string m_bind_ip;
    std::string m_port;
#ifdef ALLOW_DEBUG_COMMANDS
    uint64_t m_last_stat_request_time;
#endif
    std::list<net_address>   m_priority_peers;
    std::vector<net_address> m_exclusive_peers;
    std::vector<net_address> m_seed_nodes;
    std::list<nodetool::peerlist_entry> m_command_line_peers;
    uint64_t m_peer_livetime;
    //keep connections to initiate some interactions
    net_server m_net_server;
    boost::uuids::uuid m_network_id;

    std::map<net_address, time_t> m_conn_fails_cache;
    epee::critical_section m_conn_fails_cache_lock;

    epee::critical_section m_blocked_ips_lock;
    std::map<uint32_t, time_t> m_blocked_ips;

    epee::critical_section m_ip_fails_score_lock;
    std::map<uint32_t, uint64_t> m_ip_fails_score;
  };
}

#include "net_node.inl"

POP_WARNINGS
