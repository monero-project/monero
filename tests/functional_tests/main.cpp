// Copyright (c) 2014-2024, The Monero Project
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
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include <boost/program_options.hpp>

#include "include_base_utils.h"
#include "string_tools.h"
using namespace epee;

#include "common/command_line.h"
#include "common/util.h"
#include "transactions_flow_test.h"

namespace po = boost::program_options;

namespace
{
  const command_line::arg_descriptor<bool> arg_test_transactions_flow = {"test_transactions_flow", ""};

  const command_line::arg_descriptor<std::string> arg_working_folder  = {"working-folder", "", "."};
  const command_line::arg_descriptor<std::string> arg_source_wallet   = {"source-wallet",  "", "", true};
  const command_line::arg_descriptor<std::string> arg_dest_wallet     = {"dest-wallet",    "", "", true};
  const command_line::arg_descriptor<std::string> arg_daemon_addr_a   = {"daemon-addr-a",  "", "127.0.0.1:8080"};
  const command_line::arg_descriptor<std::string> arg_daemon_addr_b   = {"daemon-addr-b",  "", "127.0.0.1:8082"};

  const command_line::arg_descriptor<uint64_t> arg_transfer_amount = {"transfer_amount",   "", 60000000000000};
  const command_line::arg_descriptor<size_t> arg_mix_in_factor     = {"mix-in-factor",     "", 15};
  const command_line::arg_descriptor<size_t> arg_tx_count          = {"tx-count",          "", 100};
  const command_line::arg_descriptor<size_t> arg_tx_per_second     = {"tx-per-second",     "", 20};
  const command_line::arg_descriptor<size_t> arg_test_repeat_count = {"test_repeat_count", "", 1};
}

int main(int argc, char* argv[])
{
  TRY_ENTRY();
  tools::on_startup();
  string_tools::set_module_name_and_folder(argv[0]);

  //set up logging options
  mlog_configure(mlog_get_default_log_path("functional_tests.log"), true);
  mlog_set_log_level(3);

  po::options_description desc_options("Allowed options");
  command_line::add_arg(desc_options, command_line::arg_help);

  command_line::add_arg(desc_options, arg_test_transactions_flow);

  command_line::add_arg(desc_options, arg_working_folder);
  command_line::add_arg(desc_options, arg_source_wallet);
  command_line::add_arg(desc_options, arg_dest_wallet);
  command_line::add_arg(desc_options, arg_daemon_addr_a);
  command_line::add_arg(desc_options, arg_daemon_addr_b);

  command_line::add_arg(desc_options, arg_transfer_amount);
  command_line::add_arg(desc_options, arg_mix_in_factor);
  command_line::add_arg(desc_options, arg_tx_count);
  command_line::add_arg(desc_options, arg_tx_per_second);
  command_line::add_arg(desc_options, arg_test_repeat_count);

  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc_options, [&]()
  {
    po::store(po::parse_command_line(argc, argv, desc_options), vm);
    po::notify(vm);
    return true;
  });
  if (!r)
    return 1;

  if (command_line::get_arg(vm, command_line::arg_help))
  {
    std::cout << desc_options << std::endl;
    return 0;
  }

  if (command_line::get_arg(vm, arg_test_transactions_flow))
  {
    std::string working_folder     = command_line::get_arg(vm, arg_working_folder);
    std::string path_source_wallet, path_target_wallet;
    if(command_line::has_arg(vm, arg_source_wallet))
      path_source_wallet = command_line::get_arg(vm, arg_source_wallet);
    if(command_line::has_arg(vm, arg_dest_wallet))
      path_target_wallet = command_line::get_arg(vm, arg_dest_wallet);

    std::string daemon_addr_a = command_line::get_arg(vm, arg_daemon_addr_a);
    std::string daemon_addr_b = command_line::get_arg(vm, arg_daemon_addr_b);
    uint64_t amount_to_transfer = command_line::get_arg(vm, arg_transfer_amount);
    size_t mix_in_factor = command_line::get_arg(vm, arg_mix_in_factor);
    size_t transactions_count = command_line::get_arg(vm, arg_tx_count);
    size_t transactions_per_second = command_line::get_arg(vm, arg_tx_per_second);
    size_t repeat_count = command_line::get_arg(vm, arg_test_repeat_count);

    for(size_t i = 0; i != repeat_count; i++)
      if(!transactions_flow_test(working_folder, path_source_wallet, path_target_wallet, daemon_addr_a, daemon_addr_b, amount_to_transfer, mix_in_factor, transactions_count, transactions_per_second))
        break;

    std::string s;
    std::cin >> s;
    
    return 1;
  }
  else
  {
    std::cout << desc_options << std::endl;
    return 1;
  }

  CATCH_ENTRY_L0("main", 1);

  return 0;
}
