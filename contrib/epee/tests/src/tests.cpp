
#include "include_base_utils.h"
#include "storages/storage_tests.h"
#include "misc/test_math.h"
#include "storages/portable_storages_test.h"
#include "net/test_net.h"

using namespace epee;

int main(int argc, char* argv[])
{

  string_tools::set_module_name_and_folder(argv[0]);

  //set up logging options
  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_2);
  log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
  log_space::log_singletone::add_logger(LOGGER_FILE, 
    log_space::log_singletone::get_default_log_file().c_str(), 
    log_space::log_singletone::get_default_log_folder().c_str());


  string_tools::command_line_params_a start_params;
  string_tools::parse_commandline(start_params, argc, argv);
  std::string tests_data_path;
  string_tools::get_xparam_from_command_line(start_params, std::string("/tests_folder"), tests_data_path);

  if(string_tools::have_in_command_line(start_params, std::string("/run_net_tests")))
  {
    if(!tests::do_run_test_server())
    {
      LOG_ERROR("net tests failed");
      return 1;
    }
    if(!tests::do_run_test_server_async_connect() )
    {
      LOG_ERROR("net tests failed");
      return 1;
    }
  }else if(string_tools::have_in_command_line(start_params, std::string("/run_unit_tests")))
  {
    if(!tests::test_median())
    {
      LOG_ERROR("median test failed");
      return 1;
    }


    if(!tests::test_storages(tests_data_path))
    {
      LOG_ERROR("storage test failed");
      return 1;
    }
  }else  if(string_tools::have_in_command_line(start_params, std::string("/run_portable_storage_test")))
  {
    tests::test_portable_storages(tests_data_path);
  }
  return 1;
}
