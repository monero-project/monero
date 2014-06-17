// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "console_handler.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "daemon/command_server.h"
#include "p2p/net_node.h"
#include <functional>

namespace daemonize {

class t_console_command_thread
{
public:
  typedef nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> > t_server;
private:
  t_command_server m_server;
  t_server & m_srv;
  async_console_handler m_console_handler;
public:
  t_console_command_thread(nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> >& srv) :
      m_server(srv)
    , m_srv(srv)
    , m_console_handler()
  {}

  void start()
  {
    using namespace std::placeholders;

    auto process_command_callback = [this](t_server* /*psrv*/, const std::string& cmd) {
      return m_server.process_command(cmd);
    };

    auto loop = [this, process_command_callback]() {
      m_console_handler.run(&m_srv, process_command_callback, "", "");
    };
    std::thread(loop).detach();
  }

  void stop()
  {
    m_console_handler.stop();
  }
};

} // namespace daemonize
