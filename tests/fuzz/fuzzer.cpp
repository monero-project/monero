// Copyright (c) 2017-2024, The Monero Project
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
#include "string_tools.h"
#include "common/command_line.h"
#include "common/util.h"
#include "fuzzer.h"

#ifndef OSSFUZZ

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

int run_fuzzer(int argc, const char **argv, Fuzzer &fuzzer)
{
  TRY_ENTRY();

  if (argc < 2)
  {
    std::cout << "usage: " << argv[0] << " " << "<filename>" << std::endl;
    return 1;
  }

#ifdef __AFL_HAVE_MANUAL_CONTROL
  __AFL_INIT();
#endif

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

  return 0;

  CATCH_ENTRY_L0("run_fuzzer", 1);
}

#endif
