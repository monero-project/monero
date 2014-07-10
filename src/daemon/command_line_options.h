#pragma once

#include <boost/program_options.hpp>

namespace command_line_options
{
  void init_help_option(
      boost::program_options::options_description & option_spec
    );
  void init_system_query_options(
      boost::program_options::options_description & option_spec
    );
  bool print_help(
      std::string const & usage_note
    , boost::program_options::variables_map const & vm
    , boost::program_options::options_description const & visible_options
    );
  bool query_system_info(
      boost::program_options::variables_map const & vm
    );
}
