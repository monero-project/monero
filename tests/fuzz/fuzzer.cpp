// Copyright (c) 2017, The Monero Project
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

#include <boost/program_options.hpp>
#include "include_base_utils.h"
#include "common/command_line.h"
#include "fuzzer.h"

#if (!defined(__clang__) || (__clang__ < 5))
static int __AFL_LOOP(int)
{
  static int once = 0;
  if (once)
    return 0;
  once = 1;
  return 1;
}
#endif

using namespace epee;
using namespace boost::program_options;

int run_fuzzer(int argc, const char **argv, Fuzzer &fuzzer)
{
  TRY_ENTRY();
  string_tools::set_module_name_and_folder(argv[0]);

  //set up logging options
  mlog_configure(mlog_get_default_log_path("fuzztests.log"), true);
  mlog_set_log("*:FATAL,logging:none");

  options_description desc_options("Allowed options");
  command_line::add_arg(desc_options, command_line::arg_help);

  variables_map vm;
  bool r = command_line::handle_error_helper(desc_options, [&]()
  {
    store(parse_command_line(argc, argv, desc_options), vm);
    notify(vm);
    return true;
  });
  if (!r)
    return 1;

  if (command_line::get_arg(vm, command_line::arg_help))
  {
    std::cout << desc_options << std::endl;
    return 0;
  }

  if (argc < 2)
  {
    std::cout << desc_options << std::endl;
    return 1;
  }

  int ret = fuzzer.init();
  if (ret)
    return ret;

  const std::string filename = argv[1];
  while (__AFL_LOOP(1000))
  {
    ret = fuzzer.run(filename);
    if (ret)
      return ret;
  }

  CATCH_ENTRY_L0("fuzzer_main", 1);
  return 0;
}
