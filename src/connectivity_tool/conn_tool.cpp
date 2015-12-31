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

#include "include_base_utils.h"
#include "version.h"

using namespace epee;
#include <boost/program_options.hpp>
#include "p2p/p2p_protocol_defs.h"
#include "common/command_line.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "net/levin_client.h"
#include "storages/levin_abstract_invoke2.h"
#include "cryptonote_core/cryptonote_core.h"
#include "storages/portable_storage_template_helper.h"
#include "crypto/crypto.h"
#include "storages/http_abstract_invoke.h"
#include "net/http_client.h"

namespace po = boost::program_options;
using namespace cryptonote;
using namespace nodetool;

unsigned int epee::g_test_dbg_lock_sleep = 0;

namespace
{
  const command_line::arg_descriptor<std::string, true> arg_ip           = {"ip", "set ip"};
  const command_line::arg_descriptor<size_t>      arg_port               = {"port", "set port"};
  const command_line::arg_descriptor<size_t>      arg_rpc_port           = {"rpc_port", "set rpc port"};
  const command_line::arg_descriptor<uint32_t, true> arg_timeout         = {"timeout", "set timeout"};
  const command_line::arg_descriptor<std::string> arg_priv_key           = {"private_key", "private key to subscribe debug command", "", true};
  const command_line::arg_descriptor<uint64_t>    arg_peer_id            = {"peer_id", "peer_id if known(if not - will be requested)", 0};
  const command_line::arg_descriptor<bool>        arg_generate_keys      = {"generate_keys_pair", "generate private and public keys pair"};
  const command_line::arg_descriptor<bool>        arg_request_stat_info  = {"request_stat_info", "request statistics information"};
  const command_line::arg_descriptor<bool>        arg_request_net_state  = {"request_net_state", "request network state information (peer list, connections count)"};
  const command_line::arg_descriptor<bool>        arg_get_daemon_info    = {"rpc_get_daemon_info", "request daemon state info vie rpc (--rpc_port option should be set ).", "", true};
}

typedef COMMAND_REQUEST_STAT_INFO_T<t_cryptonote_protocol_handler<core>::stat_info> COMMAND_REQUEST_STAT_INFO;

struct response_schema
{
  std::string status;
  std::string COMMAND_REQUEST_STAT_INFO_status;
  std::string COMMAND_REQUEST_NETWORK_STATE_status;
  enableable<COMMAND_REQUEST_STAT_INFO::response> si_rsp;
  enableable<COMMAND_REQUEST_NETWORK_STATE::response> ns_rsp;

  BEGIN_KV_SERIALIZE_MAP()
    KV_SERIALIZE(status)
    KV_SERIALIZE(COMMAND_REQUEST_STAT_INFO_status)
    KV_SERIALIZE(COMMAND_REQUEST_NETWORK_STATE_status)
    KV_SERIALIZE(si_rsp)
    KV_SERIALIZE(ns_rsp)
  END_KV_SERIALIZE_MAP() 
};

  std::string get_response_schema_as_json(response_schema& rs)
  {
    std::stringstream ss;
    ss << "{" << ENDL 
       << "  \"status\": \"" << rs.status << "\"," << ENDL
       << "  \"COMMAND_REQUEST_NETWORK_STATE_status\": \"" << rs.COMMAND_REQUEST_NETWORK_STATE_status << "\"," << ENDL
       << "  \"COMMAND_REQUEST_STAT_INFO_status\": \"" << rs.COMMAND_REQUEST_STAT_INFO_status <<  "\"";
    if(rs.si_rsp.enabled)
    {
      ss << "," << ENDL << "  \"si_rsp\": " <<  epee::serialization::store_t_to_json(rs.si_rsp.v, 1);
    }
    if(rs.ns_rsp.enabled)
    {
      ss << "," << ENDL << "  \"ns_rsp\": {" << ENDL 
        << "    \"local_time\": " <<  rs.ns_rsp.v.local_time << "," << ENDL 
        << "    \"my_id\": \"" <<  rs.ns_rsp.v.my_id << "\"," << ENDL
        << "    \"connections_list\": [" << ENDL;

      size_t i = 0;
      BOOST_FOREACH(const connection_entry& ce, rs.ns_rsp.v.connections_list)
      {
        ss <<  "      {\"peer_id\": \"" << ce.id << "\", \"ip\": \"" << string_tools::get_ip_string_from_int32(ce.adr.ip) << "\", \"port\": " << ce.adr.port << ", \"is_income\": "<< ce.is_income << "}";
        if(rs.ns_rsp.v.connections_list.size()-1 != i)
          ss << ",";
        ss << ENDL; 
        i++;
      }
      ss << "    ]," << ENDL;
      ss << "    \"local_peerlist_white\": [" << ENDL;      
      i = 0;
      BOOST_FOREACH(const peerlist_entry& pe, rs.ns_rsp.v.local_peerlist_white)
      {
        ss <<  "      {\"peer_id\": \"" << pe.id << "\", \"ip\": \"" << string_tools::get_ip_string_from_int32(pe.adr.ip) << "\", \"port\": " << pe.adr.port << ", \"last_seen\": "<< rs.ns_rsp.v.local_time - pe.last_seen << "}";
        if(rs.ns_rsp.v.local_peerlist_white.size()-1 != i)
          ss << ",";
        ss << ENDL; 
        i++;
      }
      ss << "    ]," << ENDL;

      ss << "    \"local_peerlist_gray\": [" << ENDL;      
      i = 0;
      BOOST_FOREACH(const peerlist_entry& pe, rs.ns_rsp.v.local_peerlist_gray)
      {
        ss <<  "      {\"peer_id\": \"" << pe.id << "\", \"ip\": \"" << string_tools::get_ip_string_from_int32(pe.adr.ip) << "\", \"port\": " << pe.adr.port << ", \"last_seen\": "<< rs.ns_rsp.v.local_time - pe.last_seen << "}";
        if(rs.ns_rsp.v.local_peerlist_gray.size()-1 != i)
          ss << ",";
        ss << ENDL; 
        i++;
      }
      ss << "    ]" << ENDL << "  }" << ENDL;
    }
    ss << "}";
    return std::move(ss.str());
  }
//---------------------------------------------------------------------------------------------------------------
bool print_COMMAND_REQUEST_STAT_INFO(const COMMAND_REQUEST_STAT_INFO::response& si)
{
  std::cout << " ------ COMMAND_REQUEST_STAT_INFO ------ " << ENDL;
  std::cout << "Version:             " << si.version << ENDL;
  std::cout << "OS Version:          " << si.os_version << ENDL;
  std::cout << "Connections:          " << si.connections_count << ENDL;
  std::cout << "INC Connections:     " << si.incoming_connections_count << ENDL;


  std::cout << "Tx pool size:        " << si.payload_info.tx_pool_size << ENDL;
  std::cout << "BC height:           " << si.payload_info.blockchain_height << ENDL;
  std::cout << "Mining speed:          " << si.payload_info.mining_speed << ENDL;
  std::cout << "Alternative blocks:  " << si.payload_info.alternative_blocks << ENDL;
  std::cout << "Top block id:        " << si.payload_info.top_block_id_str << ENDL;
  return true;
}
//---------------------------------------------------------------------------------------------------------------
bool print_COMMAND_REQUEST_NETWORK_STATE(const COMMAND_REQUEST_NETWORK_STATE::response& ns)
{
  std::cout << " ------ COMMAND_REQUEST_NETWORK_STATE ------ " << ENDL;
  std::cout << "Peer id: " << ns.my_id << ENDL;
  std::cout << "Active connections:"  << ENDL;
  BOOST_FOREACH(const connection_entry& ce, ns.connections_list)
  {
    std::cout <<  ce.id << "\t" << string_tools::get_ip_string_from_int32(ce.adr.ip) << ":" << ce.adr.port << (ce.is_income ? "(INC)":"(OUT)") << ENDL; 
  }
  
  std::cout << "Peer list white:" << ns.my_id << ENDL;
  BOOST_FOREACH(const peerlist_entry& pe, ns.local_peerlist_white)
  {
    std::cout <<  pe.id << "\t" << string_tools::get_ip_string_from_int32(pe.adr.ip) << ":" << pe.adr.port <<  "\t" << misc_utils::get_time_interval_string(ns.local_time - pe.last_seen) << ENDL; 
  }

  std::cout << "Peer list gray:" << ns.my_id << ENDL;
  BOOST_FOREACH(const peerlist_entry& pe, ns.local_peerlist_gray)
  {
    std::cout <<  pe.id << "\t" << string_tools::get_ip_string_from_int32(pe.adr.ip) << ":" << pe.adr.port <<  "\t" << misc_utils::get_time_interval_string(ns.local_time - pe.last_seen) << ENDL; 
  }


  return true;
}
//---------------------------------------------------------------------------------------------------------------
bool handle_get_daemon_info(po::variables_map& vm)
{
  if(!command_line::has_arg(vm, arg_rpc_port))
  {
    std::cout << "ERROR: rpc port not set" << ENDL;
    return false;
  }

  epee::net_utils::http::http_simple_client http_client;

  cryptonote::COMMAND_RPC_GET_INFO::request req = AUTO_VAL_INIT(req);
  cryptonote::COMMAND_RPC_GET_INFO::response res = AUTO_VAL_INIT(res);
  std::string daemon_addr = command_line::get_arg(vm, arg_ip) + ":" + std::to_string(command_line::get_arg(vm, arg_rpc_port));
  bool r = net_utils::invoke_http_json_remote_command2(daemon_addr + "/getinfo", req, res, http_client, command_line::get_arg(vm, arg_timeout));
  if(!r)
  {
    std::cout << "ERROR: failed to invoke request" << ENDL;
    return false;
  }
  std::cout << "OK" << ENDL
    << "height: " << res.height << ENDL
  << "difficulty: " << res.difficulty << ENDL
  << "tx_count: " << res.tx_count << ENDL
  << "tx_pool_size: " << res.tx_pool_size << ENDL
  << "alt_blocks_count: " << res.alt_blocks_count << ENDL
  << "outgoing_connections_count: " << res.outgoing_connections_count << ENDL
  << "incoming_connections_count: " << res.incoming_connections_count << ENDL
  << "white_peerlist_size: " << res.white_peerlist_size << ENDL
  << "grey_peerlist_size: " << res.grey_peerlist_size << ENDL;

  return true;
}
//---------------------------------------------------------------------------------------------------------------
bool handle_request_stat(po::variables_map& vm, peerid_type peer_id)
{

  if(!command_line::has_arg(vm, arg_priv_key))
  {
    std::cout << "{" << ENDL << "  \"status\": \"ERROR: " << "secret key not set \"" << ENDL << "}";
    return false;
  }
  crypto::secret_key prvk = AUTO_VAL_INIT(prvk);
  if(!string_tools::hex_to_pod(command_line::get_arg(vm, arg_priv_key) , prvk))
  {
    std::cout << "{" << ENDL << "  \"status\": \"ERROR: " << "wrong secret key set \"" << ENDL << "}";
    return false;
  }


  response_schema rs = AUTO_VAL_INIT(rs);

  levin::levin_client_impl2 transport;
  if(!transport.connect(command_line::get_arg(vm, arg_ip), static_cast<int>(command_line::get_arg(vm, arg_port)), static_cast<int>(command_line::get_arg(vm, arg_timeout))))
  {
    std::cout << "{" << ENDL << "  \"status\": \"ERROR: " << "Failed to connect to " << command_line::get_arg(vm, arg_ip) << ":" << command_line::get_arg(vm, arg_port) << "\"" << ENDL << "}";
    return false;
  }else
    rs.status = "OK";

  if(!peer_id)
  {
    COMMAND_REQUEST_PEER_ID::request req = AUTO_VAL_INIT(req);
    COMMAND_REQUEST_PEER_ID::response rsp = AUTO_VAL_INIT(rsp);
    if(!net_utils::invoke_remote_command2(COMMAND_REQUEST_PEER_ID::ID, req, rsp, transport))
    {
      std::cout << "{" << ENDL << "  \"status\": \"ERROR: " << "Failed to connect to " << command_line::get_arg(vm, arg_ip) << ":" << command_line::get_arg(vm, arg_port) << "\"" << ENDL << "}";
      return false;
    }else
    {
      peer_id = rsp.my_id;
    }
  }


  nodetool::proof_of_trust pot = AUTO_VAL_INIT(pot);
  pot.peer_id = peer_id;
  pot.time = time(NULL);
  crypto::public_key pubk = AUTO_VAL_INIT(pubk);
  string_tools::hex_to_pod(::config::P2P_REMOTE_DEBUG_TRUSTED_PUB_KEY, pubk);
  crypto::hash h = tools::get_proof_of_trust_hash(pot);
  crypto::generate_signature(h, pubk, prvk, pot.sign);

  if(command_line::get_arg(vm, arg_request_stat_info))
  {
    COMMAND_REQUEST_STAT_INFO::request req = AUTO_VAL_INIT(req);
    req.tr = pot;
    if(!net_utils::invoke_remote_command2(COMMAND_REQUEST_STAT_INFO::ID, req, rs.si_rsp.v, transport))
    {
      std::stringstream ss;
      ss << "ERROR: " << "Failed to invoke remote command COMMAND_REQUEST_STAT_INFO to " << command_line::get_arg(vm, arg_ip) << ":" << command_line::get_arg(vm, arg_port);
      rs.COMMAND_REQUEST_STAT_INFO_status = ss.str();
    }else
    {
      rs.si_rsp.enabled = true;
      rs.COMMAND_REQUEST_STAT_INFO_status = "OK";
    }
  }


  if(command_line::get_arg(vm, arg_request_net_state))
  {
    ++pot.time;
    h = tools::get_proof_of_trust_hash(pot);
    crypto::generate_signature(h, pubk, prvk, pot.sign);
    COMMAND_REQUEST_NETWORK_STATE::request req = AUTO_VAL_INIT(req);    
    req.tr = pot;
    if(!net_utils::invoke_remote_command2(COMMAND_REQUEST_NETWORK_STATE::ID, req, rs.ns_rsp.v, transport))
    {
      std::stringstream ss;
      ss << "ERROR: " << "Failed to invoke remote command COMMAND_REQUEST_NETWORK_STATE to " << command_line::get_arg(vm, arg_ip) << ":" << command_line::get_arg(vm, arg_port);
      rs.COMMAND_REQUEST_NETWORK_STATE_status = ss.str();
    }else
    {
      rs.ns_rsp.enabled = true;
      rs.COMMAND_REQUEST_NETWORK_STATE_status = "OK";
    }
  }
  std::cout << get_response_schema_as_json(rs);
  return true;
}
//---------------------------------------------------------------------------------------------------------------
bool generate_and_print_keys()
{
  crypto::public_key pk = AUTO_VAL_INIT(pk);
  crypto::secret_key sk = AUTO_VAL_INIT(sk);
  generate_keys(pk, sk);
  std::cout << "PUBLIC KEY: " << epee::string_tools::pod_to_hex(pk) << ENDL 
    << "PRIVATE KEY: " << epee::string_tools::pod_to_hex(sk);
  return true;
}
int main(int argc, char* argv[])
{
  string_tools::set_module_name_and_folder(argv[0]);
  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_0);

  // Declare the supported options.
  po::options_description desc_general("General options");
  command_line::add_arg(desc_general, command_line::arg_help);

  po::options_description desc_params("Connectivity options");
  command_line::add_arg(desc_params, arg_ip);
  command_line::add_arg(desc_params, arg_port);
  command_line::add_arg(desc_params, arg_rpc_port);
  command_line::add_arg(desc_params, arg_timeout);
  command_line::add_arg(desc_params, arg_request_stat_info);
  command_line::add_arg(desc_params, arg_request_net_state);
  command_line::add_arg(desc_params, arg_generate_keys);
  command_line::add_arg(desc_params, arg_peer_id);
  command_line::add_arg(desc_params, arg_priv_key);
  command_line::add_arg(desc_params, arg_get_daemon_info);


  po::options_description desc_all;
  desc_all.add(desc_general).add(desc_params);

  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc_all, [&]()
  {
    po::store(command_line::parse_command_line(argc, argv, desc_general, true), vm);
    if (command_line::get_arg(vm, command_line::arg_help))
    {
      std::cout << desc_all << ENDL;
      return false;
    }

    po::store(command_line::parse_command_line(argc, argv, desc_params, false), vm);
    po::notify(vm);

    return true;
  });
  if (!r)
    return 1;

  if(command_line::has_arg(vm, arg_request_stat_info) || command_line::has_arg(vm, arg_request_net_state))
  {
    return handle_request_stat(vm, command_line::get_arg(vm, arg_peer_id)) ? 0:1;
  }
  if(command_line::has_arg(vm, arg_get_daemon_info))
  {
    return handle_get_daemon_info(vm) ? 0:1;
  }
  else if(command_line::has_arg(vm, arg_generate_keys))
  {
    return  generate_and_print_keys() ? 0:1;
  }
  else
  {
    std::cerr << "Not enough arguments." << ENDL;
    std::cerr << desc_all << ENDL;
  }

  return 1;
}

