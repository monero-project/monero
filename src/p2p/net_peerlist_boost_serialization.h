// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

namespace boost
{
  namespace serialization
  {
    //BOOST_CLASS_VERSION(odetool::net_adress, 1)
    template <class Archive, class ver_type>
    inline void serialize(Archive &a,  nodetool::net_address& na, const ver_type ver)
    {
      a & na.ip;
      a & na.port;
    }


    template <class Archive, class ver_type>
    inline void serialize(Archive &a,  nodetool::peerlist_entry& pl, const ver_type ver)
    {
      a & pl.adr;
      a & pl.id;
      a & pl.last_seen;
    }    
  }
}
