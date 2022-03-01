// Copyright (c) 2014-2022, The Monero Project
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

#include "gtest/gtest.h"

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/program_options.hpp>

#include "p2p/net_node.h"
#include "p2p/net_node.inl"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.inl"
#include "include_base_utils.h"
#include "string_tools.h"
#include "common/command_line.h"
#include "common/util.h"
#include "unit_tests_utils.h"

namespace po = boost::program_options;

boost::filesystem::path unit_test::data_dir;

namespace nodetool { template class node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core>>; }
namespace cryptonote { template class t_cryptonote_protocol_handler<cryptonote::core>; }

int main(int argc, char** argv)
{
  TRY_ENTRY();

  tools::on_startup();
  epee::string_tools::set_module_name_and_folder(argv[0]);
  mlog_configure(mlog_get_default_log_path("unit_tests.log"), true);
  epee::debug::get_set_enable_assert(true, false);

  ::testing::InitGoogleTest(&argc, argv);

  po::options_description desc_options("Command line options");
  const command_line::arg_descriptor<std::string> arg_data_dir = { "data-dir", "Data files directory", DEFAULT_DATA_DIR };
  command_line::add_arg(desc_options, arg_data_dir);

  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc_options, [&]()
  {
    po::store(po::parse_command_line(argc, argv, desc_options), vm);
    po::notify(vm);
    return true;
  });
  if (! r)
    return 1;

  unit_test::data_dir = command_line::get_arg(vm, arg_data_dir);

  CATCH_ENTRY_L0("main", 1);

  return RUN_ALL_TESTS();
}
