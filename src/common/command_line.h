// Copyright (c) 2014-2020, The Monero Project
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

#pragma once

#include "include_base_utils.h"

#include "fwd/boost_monero_program_options_fwd.h"

#include <functional>
#include <array>

namespace command_line
{

  //! \return True if `str` is `is_iequal("y" || "yes" || `tr("yes"))`.
  bool is_yes(const std::string& str);
  //! \return True if `str` is `is_iequal("n" || "no" || `tr("no"))`.
  bool is_no(const std::string& str);

  template<typename T, bool required = false, bool dependent = false, int NUM_DEPS = 1>
  struct arg_descriptor;

  template<typename T>
  struct arg_descriptor<T, false>
  {
    typedef T value_type;

    const char* name;
    const char* description;
    T default_value;
    bool not_use_default;
  };

  template<typename T>
  struct arg_descriptor<std::vector<T>, false>
  {
    typedef std::vector<T> value_type;

    const char* name;
    const char* description;
  };

  template<typename T>
  struct arg_descriptor<T, true>
  {
    static_assert(!std::is_same<T, bool>::value, "Boolean switch can't be required");

    typedef T value_type;

    const char* name;
    const char* description;
  };

  template<typename T>
  struct arg_descriptor<T, false, true>
  {
    typedef T value_type;

    const char* name;
    const char* description;

    T default_value;

    const arg_descriptor<bool, false>& ref;
    std::function<T(bool, bool, T)> depf;

    bool not_use_default;
  };

  template<typename T, int NUM_DEPS>
  struct arg_descriptor<T, false, true, NUM_DEPS>
  {
    typedef T value_type;

    const char* name;
    const char* description;

    T default_value;

    std::array<const arg_descriptor<bool, false> *, NUM_DEPS> ref;
    std::function<T(std::array<bool, NUM_DEPS>, bool, T)> depf;

    bool not_use_default;
  };

  extern const arg_descriptor<bool> arg_help;
  extern const arg_descriptor<bool> arg_version;
}
