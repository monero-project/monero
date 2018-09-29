// Copyright (c) 2018, The Monero Project
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

#include <boost/algorithm/string.hpp>
#include "misc_log_ex.h"
#include "file_io_utils.h"
#include "exec.h"
#include "notify.h"

namespace tools
{

Notify::Notify(const char *spec)
{
  CHECK_AND_ASSERT_THROW_MES(spec, "Null spec");

  boost::split(args, spec, boost::is_any_of(" "));
  CHECK_AND_ASSERT_THROW_MES(args.size() > 0, "Failed to parse spec");
  filename = args[0];
  CHECK_AND_ASSERT_THROW_MES(epee::file_io_utils::is_file_exist(filename), "File not found: " << filename);
}

int Notify::notify(const char *parameter)
{
  std::vector<std::string> margs = args;
  for (std::string &s: margs)
    boost::replace_all(s, "%s", parameter);

  char **cargs = (char**)alloca(sizeof(char*) * (margs.size() + 1));
  for (size_t n = 0; n < margs.size(); ++n)
    cargs[n] = (char*)margs[n].c_str();
  cargs[margs.size()] = NULL;
  return tools::exec(filename.c_str(), cargs, false);
}

}
