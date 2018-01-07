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

#include "net/net_utils_base.h"
#include "p2p/p2p_protocol_defs.h"

namespace boost
{
  namespace serialization
  {
    template <class T, class Archive>
    inline void do_serialize(Archive &a, epee::net_utils::network_address& na, T local)
    {
      if (typename Archive::is_saving()) local = na.as<T>();
      a & local;
      if (!typename Archive::is_saving()) na = local;
    }
    template <class Archive, class ver_type>
    inline void serialize(Archive &a, epee::net_utils::network_address& na, const ver_type ver)
    {
      uint8_t type;
      if (typename Archive::is_saving())
        type = na.get_type_id();
      a & type;
      switch (type)
      {
        case epee::net_utils::ipv4_network_address::ID:
	  do_serialize(a, na, epee::net_utils::ipv4_network_address{0, 0});
	  break; 
        default:
          throw std::runtime_error("Unsupported network address type");
      }
    }
    template <class Archive, class ver_type>
    inline void serialize(Archive &a, epee::net_utils::ipv4_network_address& na, const ver_type ver)
    {
      uint32_t ip{na.ip()};
      uint16_t port{na.port()};
      a & ip;
      a & port;
      if (!typename Archive::is_saving())
        na = epee::net_utils::ipv4_network_address{ip, port};
    }

    template <class Archive, class ver_type>
    inline void serialize(Archive &a,  nodetool::peerlist_entry& pl, const ver_type ver)
    {
      a & pl.adr;
      a & pl.id;
      a & pl.last_seen;
    }

    template <class Archive, class ver_type>
    inline void serialize(Archive &a, nodetool::anchor_peerlist_entry& pl, const ver_type ver)
    {
      a & pl.adr;
      a & pl.id;
      a & pl.first_seen;
    }
  }
}
