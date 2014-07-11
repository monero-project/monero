#pragma once

#include "daemon/daemon.h"
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <string>
#include <vector>

namespace daemonize
{
  class t_executor final
  {
  public:
    static std::string const NAME;

    static void init_options(
        boost::program_options::options_description & hidden_options
      , boost::program_options::options_description & normal_options
      , boost::program_options::options_description & configurable_options
      );

    t_daemon create_daemon(
        boost::program_options::variables_map const & vm
      );

    bool run_interactive(
        boost::program_options::variables_map const & vm
      );
  };
}
