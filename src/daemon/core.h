#pragma once

#include "cryptonote_core/checkpoints_create.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "misc_log_ex.h"
#include <stdexcept>
#include <boost/program_options.hpp>

namespace daemonize
{

class t_core final
{
public:
  static void init_options(boost::program_options::options_description & option_spec)
  {
    cryptonote::core::init_options(option_spec);
    cryptonote::miner::init_options(option_spec);
  }
private:
  typedef cryptonote::t_cryptonote_protocol_handler<cryptonote::core> t_protocol_raw;
  cryptonote::core m_core;
public:
  t_core(
      boost::program_options::variables_map const & vm
    )
    : m_core{nullptr}
  {
    cryptonote::checkpoints checkpoints;
    if (!cryptonote::create_checkpoints(checkpoints))
    {
      throw std::runtime_error("Failed to initialize checkpoints");
    }
    m_core.set_checkpoints(std::move(checkpoints));

    //initialize core here
    LOG_PRINT_L0("Initializing core...");
    if (!m_core.init(vm))
    {
      throw std::runtime_error("Failed to initialize core");
    }
    LOG_PRINT_L0("Core initialized OK");
  }

  // TODO - get rid of circular dependencies in internals
  void set_protocol(t_protocol_raw & protocol)
  {
    m_core.set_cryptonote_protocol(&protocol);
  }

  cryptonote::core & get()
  {
    return m_core;
  }

  ~t_core()
  {
    LOG_PRINT_L0("Deinitializing core...");
    try {
      m_core.deinit();
      m_core.set_cryptonote_protocol(nullptr);
    } catch (...) {
      LOG_PRINT_L0("Failed to deinitialize core...");
    }
  }
};

}
