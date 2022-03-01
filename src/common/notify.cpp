// Copyright (c) 2018-2022, The Monero Project

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
#include <stdarg.h>
#include "misc_log_ex.h"
#include "file_io_utils.h"
#include "spawn.h"
#include "notify.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "notify"

namespace tools
{

/*
  TODO: 
  - Improve tokenization to handle paths containing whitespaces, quotes, etc.
  - Windows unicode support (implies implementing unicode command line parsing code)
*/
Notify::Notify(const char *spec)
{
  CHECK_AND_ASSERT_THROW_MES(spec, "Null spec");

  boost::split(args, spec, boost::is_any_of(" \t"), boost::token_compress_on);
  CHECK_AND_ASSERT_THROW_MES(args.size() > 0, "Failed to parse spec");
  if (strchr(spec, '\'') || strchr(spec, '\"') || strchr(spec, '\\'))
    MWARNING("A notification spec contains a quote or backslash: note that these are handled verbatim, which may not be the intent");
  filename = args[0];
  CHECK_AND_ASSERT_THROW_MES(epee::file_io_utils::is_file_exist(filename), "File not found: " << filename);
}

static void replace(std::vector<std::string> &v, const char *tag, const char *s)
{
  for (std::string &str: v)
    boost::replace_all(str, tag, s);
}

int Notify::notify(const char *tag, const char *s, ...) const
{
  std::vector<std::string> margs = args;

  replace(margs, tag, s);

  va_list ap;
  va_start(ap, s);
  while ((tag = va_arg(ap, const char*)))
  {
    s = va_arg(ap, const char*);
    replace(margs, tag, s);
  }
  va_end(ap);

  return tools::spawn(filename.c_str(), margs, false);
}

}
