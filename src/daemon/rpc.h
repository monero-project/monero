#pragma once

#include "daemon/core.h"
#include "daemon/p2p.h"
#include "misc_log_ex.h"
#include "rpc/core_rpc_server.h"
#include <boost/program_options.hpp>
#include <stdexcept>

namespace daemonize
{

class t_rpc final
{
public:
  static void init_options(boost::program_options::options_description & option_spec)
  {
    cryptonote::core_rpc_server::init_options(option_spec);
  }
private:
  cryptonote::core_rpc_server m_server;
public:
  t_rpc(
      boost::program_options::variables_map const & vm
    , t_core & core
    , t_p2p & p2p
    )
    : m_server{core.get(), p2p.get()}
  {
    LOG_PRINT_L0("Initializing core rpc server...");
    if (!m_server.init(vm))
    {
      throw std::runtime_error("Failed to initialize core rpc server.");
    }
    LOG_PRINT_GREEN("Core rpc server initialized OK on port: " << m_server.get_binded_port(), LOG_LEVEL_0);
  }

  void run()
  {
    LOG_PRINT_L0("Starting core rpc server...");
    if (!m_server.run(2, false))
    {
      throw std::runtime_error("Failed to start core rpc server.");
    }
    LOG_PRINT_L0("Core rpc server started ok");
  }

  void stop()
  {
    LOG_PRINT_L0("Stopping core rpc server...");
    m_server.send_stop_signal();
    m_server.timed_wait_server_stop(5000);
  }

  ~t_rpc()
  {
    LOG_PRINT_L0("Deinitializing rpc server...");
    try {
      m_server.deinit();
    } catch (...) {
      LOG_PRINT_L0("Failed to deinitialize rpc server...");
    }
  }
};

}
