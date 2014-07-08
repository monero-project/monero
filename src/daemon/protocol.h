#pragma once

#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "misc_log_ex.h"
#include "p2p/net_node.h"
#include <stdexcept>
#include <boost/program_options.hpp>

namespace daemonize
{

class t_protocol final
{
private:
  typedef cryptonote::t_cryptonote_protocol_handler<cryptonote::core> t_protocol_raw;
  typedef nodetool::node_server<t_protocol_raw> t_node_server;

  t_protocol_raw m_protocol;
public:
  t_protocol(
      boost::program_options::variables_map const & vm
    , t_core & core
    )
    : m_protocol{core.get(), nullptr}
  {
    LOG_PRINT_L0("Initializing cryptonote protocol...");
    if (!m_protocol.init(vm))
    {
      throw std::runtime_error("Failed to initialize cryptonote protocol.");
    }
    LOG_PRINT_L0("Cryptonote protocol initialized OK");
  }

  t_protocol_raw & get()
  {
    return m_protocol;
  }

  void set_p2p_endpoint(
      t_node_server & server
    )
  {
    m_protocol.set_p2p_endpoint(&server);
  }

  ~t_protocol()
  {
    LOG_PRINT_L0("Deinitializing cryptonote_protocol...");
    try {
      m_protocol.deinit();
      m_protocol.set_p2p_endpoint(nullptr);
    } catch (...) {
      LOG_PRINT_L0("Failed to deinitialize protocol...");
    }
  }
};

}
