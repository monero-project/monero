// Copyright (c) 2014-2018, The Monero Project
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

#include <boost/uuid/uuid.hpp>
#include "serialization/keyvalue_serialization.h"
#include "net/net_utils_base.h"
#include "misc_language.h"
#include "string_tools.h"
#include "time_helper.h"
#include "cryptonote_config.h"
#ifdef ALLOW_DEBUG_COMMANDS
#include "crypto/crypto.h"
#endif

namespace nodetool
{
  typedef boost::uuids::uuid uuid;
  typedef uint64_t peerid_type;

  static inline std::string peerid_to_string(peerid_type peer_id)
  {
    std::ostringstream s;
    s << std::hex << peer_id;
    return epee::string_tools::pad_string(s.str(), 16, '0', true);
  }

#pragma pack (push, 1)
  
  struct network_address_old
  {
    uint32_t ip;
    uint32_t port;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(ip)
      KV_SERIALIZE(port)
    END_KV_SERIALIZE_MAP()
  };

  template<typename AddressType>
  struct peerlist_entry_base
  {
    AddressType adr;
    peerid_type id;
    int64_t last_seen;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(adr)
      KV_SERIALIZE(id)
      KV_SERIALIZE(last_seen)
    END_KV_SERIALIZE_MAP()
  };
  typedef peerlist_entry_base<epee::net_utils::network_address> peerlist_entry;

  template<typename AddressType>
  struct anchor_peerlist_entry_base
  {
    AddressType adr;
    peerid_type id;
    int64_t first_seen;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(adr)
      KV_SERIALIZE(id)
      KV_SERIALIZE(first_seen)
    END_KV_SERIALIZE_MAP()
  };
  typedef anchor_peerlist_entry_base<epee::net_utils::network_address> anchor_peerlist_entry;

  template<typename AddressType>
  struct connection_entry_base
  {
    AddressType adr;
    peerid_type id;
    bool is_income;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(adr)
      KV_SERIALIZE(id)
      KV_SERIALIZE(is_income)
    END_KV_SERIALIZE_MAP()
  };
  typedef connection_entry_base<epee::net_utils::network_address> connection_entry;

#pragma pack(pop)

  inline 
  std::string print_peerlist_to_string(const std::list<peerlist_entry>& pl)
  {
    time_t now_time = 0;
    time(&now_time);
    std::stringstream ss;
    ss << std::setfill ('0') << std::setw (8) << std::hex << std::noshowbase;
    for(const peerlist_entry& pe: pl)
    {
      ss << pe.id << "\t" << pe.adr.str() << " \tlast_seen: " << epee::misc_utils::get_time_interval_string(now_time - pe.last_seen) << std::endl;
    }
    return ss.str();
  }


  struct network_config
  {
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(max_out_connection_count)
      KV_SERIALIZE(max_in_connection_count)
      KV_SERIALIZE(handshake_interval)
      KV_SERIALIZE(packet_max_size)
      KV_SERIALIZE(config_id)
    END_KV_SERIALIZE_MAP()

    uint32_t max_out_connection_count;
    uint32_t max_in_connection_count;
    uint32_t connection_timeout;
    uint32_t ping_connection_timeout;
    uint32_t handshake_interval;
    uint32_t packet_max_size;
    uint32_t config_id;
    uint32_t send_peerlist_sz;
  };

  struct basic_node_data
  {
    uuid network_id;                   
    uint64_t local_time;
    uint32_t my_port;
    peerid_type peer_id;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_VAL_POD_AS_BLOB(network_id)
      KV_SERIALIZE(peer_id)
      KV_SERIALIZE(local_time)
      KV_SERIALIZE(my_port)
    END_KV_SERIALIZE_MAP()
  };
  

#define P2P_COMMANDS_POOL_BASE 1000

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  template<class t_playload_type>
	struct COMMAND_HANDSHAKE_T
	{
		const static int ID = P2P_COMMANDS_POOL_BASE + 1;

    struct request
    {
      basic_node_data node_data;
      t_playload_type payload_data;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(node_data)
        KV_SERIALIZE(payload_data)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      basic_node_data node_data;
      t_playload_type payload_data;
      std::list<peerlist_entry> local_peerlist_new;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(node_data)
        KV_SERIALIZE(payload_data)
        if (is_store)
        {
          // saving: save both, so old and new peers can understand it
          KV_SERIALIZE(local_peerlist_new)
        }
        else
        {
          // loading: load old list only if there is no new one
          if (!epee::serialization::selector<is_store>::serialize(this_ref.local_peerlist_new, stg, hparent_section, "local_peerlist_new"))
          {
	    MDEBUG("No peers to load");
          }
        }
      END_KV_SERIALIZE_MAP()
    };
	};


  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  template<class t_playload_type>
  struct COMMAND_TIMED_SYNC_T
  {
    const static int ID = P2P_COMMANDS_POOL_BASE + 2;

    struct request
    {
      t_playload_type payload_data;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payload_data)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t local_time;
      t_playload_type payload_data;
      std::list<peerlist_entry> local_peerlist_new;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(local_time)
        KV_SERIALIZE(payload_data)
        if (is_store)
        {
          // saving: save both, so old and new peers can understand it
          KV_SERIALIZE(local_peerlist_new)
          std::list<peerlist_entry_base<network_address_old>> local_peerlist;
          for (const auto &p: this_ref.local_peerlist_new)
          {
            if (p.adr.get_type_id() == epee::net_utils::ipv4_network_address::ID)
            {
              const epee::net_utils::network_address  &na = p.adr;
              const epee::net_utils::ipv4_network_address &ipv4 = na.as<const epee::net_utils::ipv4_network_address>();
              local_peerlist.push_back(peerlist_entry_base<network_address_old>({{ipv4.ip(), ipv4.port()}, p.id, p.last_seen}));
            }
            else
              MDEBUG("Not including in legacy peer list: " << p.adr.str());
          }
          epee::serialization::selector<is_store>::serialize_stl_container_pod_val_as_blob(local_peerlist, stg, hparent_section, "local_peerlist");
        }
        else
        {
          // loading: load old list only if there is no new one
          if (!epee::serialization::selector<is_store>::serialize(this_ref.local_peerlist_new, stg, hparent_section, "local_peerlist_new"))
          {
            std::list<peerlist_entry_base<network_address_old>> local_peerlist;
            epee::serialization::selector<is_store>::serialize_stl_container_pod_val_as_blob(local_peerlist, stg, hparent_section, "local_peerlist");
            for (const auto &p: local_peerlist)
              ((response&)this_ref).local_peerlist_new.push_back(peerlist_entry({epee::net_utils::ipv4_network_address(p.adr.ip, p.adr.port), p.id, p.last_seen}));
          }
        }
      END_KV_SERIALIZE_MAP()
    };
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/

  struct COMMAND_PING
  {
    /*
      Used to make "callback" connection, to be sure that opponent node 
      have accessible connection point. Only other nodes can add peer to peerlist,
      and ONLY in case when peer has accepted connection and answered to ping.
    */
    const static int ID = P2P_COMMANDS_POOL_BASE + 3;

#define PING_OK_RESPONSE_STATUS_TEXT "OK"

    struct request
    {
      /*actually we don't need to send any real data*/

      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      peerid_type peer_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(peer_id)
      END_KV_SERIALIZE_MAP()    
    };
  };

  
#ifdef ALLOW_DEBUG_COMMANDS
  //These commands are considered as insecure, and made in debug purposes for a limited lifetime. 
  //Anyone who feel unsafe with this commands can disable the ALLOW_GET_STAT_COMMAND macro.

  struct proof_of_trust
  {
    peerid_type peer_id;
    uint64_t    time;
    crypto::signature sign;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(peer_id)
      KV_SERIALIZE(time)        
      KV_SERIALIZE_VAL_POD_AS_BLOB(sign)  
    END_KV_SERIALIZE_MAP()    
  };


  template<class payload_stat_info>
  struct COMMAND_REQUEST_STAT_INFO_T
  {
    const static int ID = P2P_COMMANDS_POOL_BASE + 4;

    struct request
    {
      proof_of_trust tr;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tr)
      END_KV_SERIALIZE_MAP()    
    };
    
    struct response
    {
      std::string version;
      std::string os_version;
      uint64_t connections_count;
      uint64_t incoming_connections_count;
      payload_stat_info payload_info;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(version)
        KV_SERIALIZE(os_version)
        KV_SERIALIZE(connections_count)
        KV_SERIALIZE(incoming_connections_count)
        KV_SERIALIZE(payload_info)
      END_KV_SERIALIZE_MAP()    
    };
  };


  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct COMMAND_REQUEST_NETWORK_STATE
  {
    const static int ID = P2P_COMMANDS_POOL_BASE + 5;

    struct request
    {
      proof_of_trust tr;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tr)
      END_KV_SERIALIZE_MAP()    
    };

    struct response
    {
      std::list<peerlist_entry> local_peerlist_white; 
      std::list<peerlist_entry> local_peerlist_gray; 
      std::list<connection_entry> connections_list; 
      peerid_type my_id;
      uint64_t    local_time;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(local_peerlist_white)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(local_peerlist_gray)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(connections_list)
        KV_SERIALIZE(my_id)
        KV_SERIALIZE(local_time)
      END_KV_SERIALIZE_MAP()    
    };
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct COMMAND_REQUEST_PEER_ID
  {
    const static int ID = P2P_COMMANDS_POOL_BASE + 6;

    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()    
    };

    struct response
    {
      peerid_type my_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(my_id)
      END_KV_SERIALIZE_MAP()    
    };
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct COMMAND_REQUEST_SUPPORT_FLAGS
  {
    const static int ID = P2P_COMMANDS_POOL_BASE + 7;

    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()    
    };

    struct response
    {
      uint32_t support_flags;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(support_flags)
      END_KV_SERIALIZE_MAP()    
    };
  };
  
#endif /* ALLOW_DEBUG_COMMANDS */


  inline crypto::hash get_proof_of_trust_hash(const nodetool::proof_of_trust& pot)
  {
    std::string s;
    s.append(reinterpret_cast<const char*>(&pot.peer_id), sizeof(pot.peer_id));
    s.append(reinterpret_cast<const char*>(&pot.time), sizeof(pot.time));
    return crypto::cn_fast_hash(s.data(), s.size());
  }

}



