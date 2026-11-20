// Copyright (c) 2014-2024, The Monero Project
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

#include <iomanip>
#include <boost/uuid/uuid.hpp>
#include <boost/serialization/version.hpp>
#include <boost/variant/variant.hpp>
#include <string>
#include <sstream>
#include <utility>

#include "serialization/keyvalue_serialization.h"
#include "net/net_utils_base.h"
#include "net/tor_address.h" // needed for serialization
#include "net/i2p_address.h" // needed for serialization
#include "net/net_ssl.h"
#include "misc_language.h"
#include "string_tools.h"
#include "time_helper.h"
#include "serialization/serialization.h"
#include "cryptonote_config.h"

namespace nodetool
{
  typedef boost::uuids::uuid uuid;
  typedef uint64_t peerid_type;

  // The current modes/versions for p2p encryption
  constexpr uint8_t emode_ssl_autodetect = 0;
  constexpr uint8_t emode_disabled = 1;
  constexpr uint8_t emode_ssl_enabled = 2;

  static inline std::string peerid_to_string(peerid_type peer_id)
  {
    std::ostringstream s;
    s << std::hex << peer_id;
    return epee::string_tools::pad_string(s.str(), 16, '0', true);
  }

  
  inline constexpr uint8_t get_encryption_mode(const epee::net_utils::ssl_support_t support) noexcept
  {
    switch (support)
    {
      case epee::net_utils::ssl_support_t::e_ssl_support_enabled:
        return emode_ssl_enabled;
      case epee::net_utils::ssl_support_t::e_ssl_support_disabled:
        return emode_disabled;
      default:
      case epee::net_utils::ssl_support_t::e_ssl_support_autodetect:
        break;
    }
    return emode_ssl_autodetect;
  }

  struct ssl_parameters
  {
    using ssl_support_t = epee::net_utils::ssl_support_t;
    using ssl_options_t = epee::net_utils::ssl_options_t;

    std::string cert_finger;
    uint8_t mode;

    ssl_parameters(std::string cert_finger, ssl_support_t mode);
    explicit ssl_parameters(std::string cert_finger, const uint8_t mode = emode_ssl_enabled);

    ssl_parameters() noexcept
      : cert_finger(), mode(emode_ssl_autodetect)
    {}

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(mode)
      KV_SERIALIZE(cert_finger)
      if (!is_store)
        sanitize();
    END_KV_SERIALIZE_MAP()

    bool operator==(const ssl_parameters& rhs) const noexcept
    { return mode == rhs.mode && cert_finger == rhs.cert_finger; }

    void sanitize();
    ssl_options_t get_p2p_encryption() const;
  };

  template<typename T>
  inline epee::net_utils::ssl_options_t get_p2p_encryption(const T& peer) 
  {
    if (peer.local_ssl.mode != emode_ssl_autodetect)
      return peer.local_ssl.get_p2p_encryption(); // force user CLI setting
    else if (peer.peer_ssl.mode == emode_disabled)
      return epee::net_utils::ssl_support_t::e_ssl_support_disabled;
    return epee::net_utils::ssl_support_t::e_ssl_support_autodetect;
    /* We ignore fingerprints from the peer for now (and maybe permanently).
      The issue is when a certificate needs to be rotated - we either block
      that ip+port permanently (due to fingerprint failures), or open ourselves
      to a MitM. */
  }

  
  template<typename AddressType>
  struct peerlist_entry_base
  {
    peerlist_entry_base()
      : adr(),
        local_ssl(),
        peer_ssl(),
        id(0),
        last_seen(0),
        pruning_seed(0),
        rpc_port(0),
        rpc_credits_per_hash(0)
    {}

    peerlist_entry_base(AddressType addr_in, peerid_type id_in, int64_t last_seen_in = 0)
      : peerlist_entry_base()
    {
      adr = std::move(addr_in);
      id = id_in;
      last_seen = last_seen_in;
    }

    template<typename T>
    void copy_ssl_to(T& dest) const
    {
      dest.local_ssl = local_ssl;
      dest.peer_ssl = peer_ssl;
    }

    AddressType adr;
    ssl_parameters local_ssl; //!< user cli settings
    ssl_parameters peer_ssl;  //!< tofu settings
    peerid_type id;
    int64_t last_seen;
    uint32_t pruning_seed;
    uint16_t rpc_port;
    uint32_t rpc_credits_per_hash;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(adr)
      KV_SERIALIZE(id)
      KV_SERIALIZE_OPT(last_seen, (int64_t)0)
      KV_SERIALIZE_OPT(pruning_seed, (uint32_t)0)
      KV_SERIALIZE_OPT(rpc_port, (uint16_t)0)
      KV_SERIALIZE_OPT(rpc_credits_per_hash, (uint32_t)0)
      /* Do not serialize `local_ssl` (privacy leak), and do not serialize
        `peer_ssl` (observed ssl parameters directly from outgoing connection).
        Eventually it may be worth adding `third_ssl` for relaying ssl
        parameters of peers, which has lowest precedence. */
    END_KV_SERIALIZE_MAP()
  };
  typedef peerlist_entry_base<epee::net_utils::network_address> peerlist_entry;

  template<typename AddressType>
  struct anchor_peerlist_entry_base
  {
    anchor_peerlist_entry_base()
      : adr(),
        local_ssl(),
        peer_ssl(),
        id(0),
        first_seen(0)
    {}

    anchor_peerlist_entry_base(AddressType addr_in, peerid_type id_in, int64_t first_seen_in = 0)
      : anchor_peerlist_entry_base()
    {
      adr = std::move(addr_in);
      id = id_in;
      first_seen = first_seen_in;
    }

    AddressType adr;
    ssl_parameters local_ssl; //!< user cli settings
    ssl_parameters peer_ssl;  //!< tofu settings
    peerid_type id;
    int64_t first_seen;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(adr)
      KV_SERIALIZE(id)
      KV_SERIALIZE(first_seen)
      // see `peerlist_entry_base` comments about ssl serialization
    END_KV_SERIALIZE_MAP()
  };
  typedef anchor_peerlist_entry_base<epee::net_utils::network_address> anchor_peerlist_entry;

  inline 
  std::string print_peerlist_to_string(const std::vector<peerlist_entry>& pl)
  {
    time_t now_time = 0;
    time(&now_time);
    std::stringstream ss;
    ss << std::setfill ('0') << std::setw (8) << std::hex << std::noshowbase;
    for(const peerlist_entry& pe: pl)
    {
      ss << peerid_to_string(pe.id) << "\t" << pe.adr.str()
        << " \trpc port " << (pe.rpc_port > 0 ? std::to_string(pe.rpc_port) : "-")
        << " \trpc credits per hash " << (pe.rpc_credits_per_hash > 0 ? std::to_string(pe.rpc_credits_per_hash) : "-")
        << " \tpruning seed " << pe.pruning_seed 
        << " \tlast_seen: " << (pe.last_seen == 0 ? std::string("never") : epee::misc_utils::get_time_interval_string(now_time - pe.last_seen))
        << std::endl;
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

    std::chrono::milliseconds ping_connection_timeout;
    std::chrono::milliseconds connection_timeout;
    uint32_t max_out_connection_count;
    uint32_t max_in_connection_count;
    uint32_t handshake_interval;
    uint32_t packet_max_size;
    uint32_t config_id;
    uint32_t send_peerlist_sz;
  };

  struct basic_node_data
  {
    ssl_parameters ssl;
    uuid network_id;
    uint32_t my_port;
    uint16_t rpc_port;
    uint32_t rpc_credits_per_hash;
    peerid_type peer_id;
    uint32_t support_flags;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_VAL_POD_AS_BLOB(network_id)
      KV_SERIALIZE(peer_id)
      KV_SERIALIZE(my_port)
      KV_SERIALIZE_OPT(rpc_port, (uint16_t)(0))
      KV_SERIALIZE_OPT(rpc_credits_per_hash, (uint32_t)0)
      KV_SERIALIZE_OPT(support_flags, (uint32_t)0)
      KV_SERIALIZE_OPT(ssl, ssl_parameters{})
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

    struct request_t
    {
      basic_node_data node_data;
      t_playload_type payload_data;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(node_data)
        KV_SERIALIZE(payload_data)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      basic_node_data node_data;
      t_playload_type payload_data;
      std::vector<peerlist_entry> local_peerlist_new;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(node_data)
        KV_SERIALIZE(payload_data)
        KV_SERIALIZE(local_peerlist_new)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };


  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  template<class t_playload_type>
  struct COMMAND_TIMED_SYNC_T
  {
    const static int ID = P2P_COMMANDS_POOL_BASE + 2;

    struct request_t
    {
      t_playload_type payload_data;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payload_data)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      t_playload_type payload_data;
      std::vector<peerlist_entry> local_peerlist_new;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payload_data)
        KV_SERIALIZE(local_peerlist_new)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
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

    struct request_t
    {
      /*actually we don't need to send any real data*/

      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string status;
      peerid_type peer_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(peer_id)
      END_KV_SERIALIZE_MAP()    
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct COMMAND_REQUEST_SUPPORT_FLAGS
  {
    const static int ID = P2P_COMMANDS_POOL_BASE + 7;

    struct request_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()    
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      uint32_t support_flags;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(support_flags)
      END_KV_SERIALIZE_MAP()    
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };
}
