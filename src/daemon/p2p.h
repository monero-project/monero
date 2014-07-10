#pragma once

#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "daemon/protocol.h"
#include "misc_log_ex.h"
#include "p2p/net_node.h"
#include <stdexcept>
#include <boost/program_options.hpp>

namespace daemonize
{

class t_p2p final
{
private:
  typedef cryptonote::t_cryptonote_protocol_handler<cryptonote::core> t_protocol_raw;
  typedef nodetool::node_server<t_protocol_raw> t_node_server;
public:
  static void init_options(boost::program_options::options_description & option_spec)
  {
    t_node_server::init_options(option_spec);
  }
private:
  t_node_server m_server;
public:
  t_p2p(
      boost::program_options::variables_map const & vm
    , t_protocol & protocol
    )
    : m_server{protocol.get()}
  {
    //initialize objects
    LOG_PRINT_L0("Initializing p2p server...");
    if (!m_server.init(vm))
    {
      throw std::runtime_error("Failed to initialize p2p server.");
    }
    LOG_PRINT_L0("P2p server initialized OK");
  }

  t_node_server & get()
  {
    return m_server;
  }

  void run()
  {
    LOG_PRINT_L0("Starting p2p net loop...");
    m_server.run();
    LOG_PRINT_L0("p2p net loop stopped");
  }

  void stop()
  {
    m_server.send_stop_signal();
  }

  ~t_p2p()
  {
    LOG_PRINT_L0("Deinitializing p2p...");
    try {
      m_server.deinit();
    } catch (...) {
      LOG_PRINT_L0("Failed to deinitialize p2p...");
    }
  }
};

}
