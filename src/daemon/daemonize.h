#pragma once

#include <boost/program_options.hpp>

namespace daemonize {
  void init_options(
      boost::program_options::options_description & visible
    , boost::program_options::options_description & all
    );
  void daemonize(boost::program_options::variables_map const & vm);
  bool unregister();  // doesn't do anything on posix
}
