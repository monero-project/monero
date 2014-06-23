// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "console_handler.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "daemon/command_server.h"
#include "p2p/net_node.h"
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>

namespace daemonize {

class t_console_command_thread
{
public:
  typedef nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> > t_server;
private:
  t_command_server<t_interactive_command_executor> m_server;
  t_server & m_srv;
  epee::async_console_handler m_console_handler;
  std::mutex mtx;
  std::condition_variable cv;
  bool m_finished = false;
public:
  t_console_command_thread(t_server & srv) :
      m_server(t_interactive_command_executor(srv))
    , m_srv(srv)
  {}

  void start()
  {
    using namespace std::placeholders;

    auto process_command_callback = [this](t_server* /*psrv*/, const std::string& cmd) {
      return m_server.process_command_str(cmd);
    };

    auto loop = [this, process_command_callback]() {
      m_console_handler.run(&m_srv, process_command_callback, "", "");

      // notify the control thread that this thread has finished
      std::unique_lock<std::mutex> lck(mtx);
      m_finished = true;
      cv.notify_all();
    };
    std::thread(loop).detach();
  }

  void stop()
  {
    m_console_handler.stop();

    // wait until we're sure the thread is finished since it uses a shared
    // node_server
    std::unique_lock<std::mutex> lck(mtx);
    while (!m_finished) cv.wait(lck);
  }
};

} // namespace daemonize
