// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <boost/uuid/uuid.hpp>
#include "net/net_utils_base.h"
#include "p2p_protocol_defs.h"

namespace nodetool
{

  typedef boost::uuids::uuid uuid;
  typedef boost::uuids::uuid net_connection_id;

  template<class t_connection_context>
  struct i_p2p_endpoint
  {
    virtual bool relay_notify_to_all(int command, const std::string& data_buff, const epee::net_utils::connection_context_base& context)=0;
    virtual bool invoke_command_to_peer(int command, const std::string& req_buff, std::string& resp_buff, const epee::net_utils::connection_context_base& context)=0;
    virtual bool invoke_notify_to_peer(int command, const std::string& req_buff, const epee::net_utils::connection_context_base& context)=0;
    virtual bool drop_connection(const epee::net_utils::connection_context_base& context)=0;
    virtual void request_callback(const epee::net_utils::connection_context_base& context)=0;
    virtual uint64_t get_connections_count()=0;
    virtual void for_each_connection(std::function<bool(t_connection_context&, peerid_type)> f)=0;
  };

  template<class t_connection_context>
  struct p2p_endpoint_stub: public i_p2p_endpoint<t_connection_context>
  {
    virtual bool relay_notify_to_all(int command, const std::string& data_buff, const epee::net_utils::connection_context_base& context)
    {
      return false;
    }
    virtual bool invoke_command_to_peer(int command, const std::string& req_buff, std::string& resp_buff, const epee::net_utils::connection_context_base& context)
    {
      return false;
    }
    virtual bool invoke_notify_to_peer(int command, const std::string& req_buff, const epee::net_utils::connection_context_base& context)
    {
      return true;
    }
    virtual bool drop_connection(const epee::net_utils::connection_context_base& context)
    {
      return false;
    }
    virtual void request_callback(const epee::net_utils::connection_context_base& context)
    {

    }
    virtual void for_each_connection(std::function<bool(t_connection_context&,peerid_type)> f)
    {

    }

    virtual uint64_t get_connections_count()    
    {
      return false;
    }
  };
}
