#pragma once

#include "daemon/daemon.h"
#include "rpcwallet/wallet_daemon.h"
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <string>
#include <vector>

namespace tools
{
  class t_wallet_executor final
  {
  public:
    typedef t_wallet_daemon t_daemon;

    static void init_options(
        boost::program_options::options_description & hidden_options
      , boost::program_options::options_description & normal_options
      , boost::program_options::options_description & configurable_options
      );
  private:
    std::string m_name;
  public:
    t_wallet_executor(
        boost::program_options::variables_map const & vm
      );

    std::string const & name();

    t_wallet_daemon create_daemon(
        boost::program_options::variables_map const & vm
      );

    bool run_interactive(
        boost::program_options::variables_map const & vm
      );
  };
}
