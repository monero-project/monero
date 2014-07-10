#include "common/command_line.h"
#include "common/util.h"
#include "cryptonote_config.h"
#include "daemon/command_line_options.h"
#include "version.h"

namespace command_line_options
{
  namespace
  {
    const command_line::arg_descriptor<bool> arg_os_version = {
      "os-version"
    , "OS for which this executable was compiled"
    };
  }

  void init_help_option(
      boost::program_options::options_description & option_spec
    )
  {
    command_line::add_arg(option_spec, command_line::arg_help);
  }

  void init_system_query_options(
      boost::program_options::options_description & option_spec
    )
  {
    command_line::add_arg(option_spec, command_line::arg_version);
    command_line::add_arg(option_spec, arg_os_version);
  }

  bool parse_options(
      boost::program_options::variables_map & result
    , int argc, char const * argv[]
    , boost::program_options::options_description const & visible_options
    , boost::program_options::options_description const & all_options
    , boost::program_options::positional_options_description const & positional_options
    )
  {
    return command_line::handle_error_helper(visible_options, [&]()
    {
      boost::program_options::store(
        boost::program_options::command_line_parser(argc, argv)
          .options(all_options).positional(positional_options).run()
      , result
      );

      return true;
    });
  }

  bool print_help(
      std::string const & usage_note
    , boost::program_options::variables_map const & vm
    , boost::program_options::options_description const & visible_options
    )
  {
    if (command_line::get_arg(vm, command_line::arg_help))
    {
      std::cout << CRYPTONOTE_NAME << " v" << PROJECT_VERSION_LONG << ENDL << ENDL;
      std::cout << usage_note << std::endl << std::endl;
      std::cout << visible_options << std::endl;
      return true;
    }

    return false;
  }

  bool query_system_info(
      boost::program_options::variables_map const & vm
    )
  {
    // Monero Version
    if (command_line::get_arg(vm, command_line::arg_version))
    {
      std::cout << CRYPTONOTE_NAME  << " v" << PROJECT_VERSION_LONG << ENDL;
      return true;
    }

    // OS
    if (command_line::get_arg(vm, arg_os_version))
    {
      std::cout << "OS: " << tools::get_os_version_string() << ENDL;
      return true;
    }

    return false;
  }

  boost::filesystem::path init_data_directory(
      boost::program_options::variables_map const & vm
    )
  {
    boost::filesystem::path data_dir = boost::filesystem::absolute(
        command_line::get_arg(vm, command_line::arg_data_dir));
    tools::create_directories_if_necessary(data_dir.string());
    return data_dir;
  }
}
