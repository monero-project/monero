// Copyright (c) 2014, The Monero Project
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// Parts of this file are originally copyright (c) 2006-2013, Andrey N. Sabelnikov

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