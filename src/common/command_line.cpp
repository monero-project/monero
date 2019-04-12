// Copyright (c) 2014-2019, The Monero Project
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

#include "command_line.h"
#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "common/i18n.h"

namespace command_line
{
  namespace
  {
    const char* tr(const char* str)
    {
      return i18n_translate(str, "command_line");
    }
  }

  static bool str_compare_with_boost(const std::string &str, char const *check_str)
  {
    boost::algorithm::is_iequal ignore_case{};
    if (boost::algorithm::equals(check_str, str, ignore_case))
      return true;
    if (boost::algorithm::equals(command_line::tr(check_str), str, ignore_case))
      return true;

    return false;
  }

  bool is_yes(const std::string& str)
  {
    bool result = (str == "y" || str == "Y") || str_compare_with_boost(str, "yes");
    return result;
  }

  bool is_no(const std::string& str)
  {
    bool result = (str == "n" || str == "N") || str_compare_with_boost(str, "no");
    return result;
  }

  bool is_cancel(const std::string& str)
  {
    bool result = (str == "c" || str == "C") || str_compare_with_boost(str, "cancel");
    return result;
  }

  bool is_back(const std::string& str)
  {
    bool result = (str == "b" || str == "B") || str_compare_with_boost(str, "back");
    return result;
  }

  const arg_descriptor<bool> arg_help = {"help", "Produce help message"};
  const arg_descriptor<bool> arg_version = {"version", "Output version information"};
}
