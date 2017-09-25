// Copyright (c) 2014-2017, The Monero Project
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

#include <boost/filesystem.hpp>

#include "include_base_utils.h"
#include "unit_tests_utils.h"

boost::filesystem::path unit_test::data_dir;

int main(int argc, char** argv)
{
  mlog_configure(mlog_get_default_log_path("unit_tests.log"), true);
  epee::debug::get_set_enable_assert(true, false);

  ::testing::InitGoogleTest(&argc, argv);

  // Process remaining arguments
  if (argc == 2 && argv[1] != NULL) { // one arg: path to dir with test data
    unit_test::data_dir = argv[1];
  } else if (argc == 1) { // legacy: assume test binaries in 'build/release'
    epee::string_tools::set_module_name_and_folder(argv[0]);
    unit_test::data_dir = boost::filesystem::path(epee::string_tools::get_current_module_folder())
                          .parent_path().parent_path().parent_path().parent_path()
                          .append("tests").append("data");
  } else {
    std::cerr << "Usage: " << argv[0] << " [<path-to-test-data-dir>]" << std::endl;
    return 1;
  }

  return RUN_ALL_TESTS();
}
