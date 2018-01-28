// Copyright (c) 2014-2018, The Monero Project
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

#pragma once

#include <iostream>
#include <type_traits>

#include <boost/program_options/parsers.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include "include_base_utils.h"

namespace command_line
{

  //! \return True if `str` is `is_iequal("y" || "yes" || `tr("yes"))`.
  bool is_yes(const std::string& str);
  //! \return True if `str` is `is_iequal("n" || "no" || `tr("no"))`.
  bool is_no(const std::string& str);

  template<typename T, bool required = false>
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
  boost::program_options::typed_value<T, char>* make_semantic(const arg_descriptor<T, true>& /*arg*/)
  {
    return boost::program_options::value<T>()->required();
  }

  template<typename T>
  boost::program_options::typed_value<T, char>* make_semantic(const arg_descriptor<T, false>& arg)
  {
    auto semantic = boost::program_options::value<T>();
    if (!arg.not_use_default)
      semantic->default_value(arg.default_value);
    return semantic;
  }

  template<typename T>
  boost::program_options::typed_value<T, char>* make_semantic(const arg_descriptor<T, false>& arg, const T& def)
  {
    auto semantic = boost::program_options::value<T>();
    if (!arg.not_use_default)
      semantic->default_value(def);
    return semantic;
  }

  template<typename T>
  boost::program_options::typed_value<std::vector<T>, char>* make_semantic(const arg_descriptor<std::vector<T>, false>& /*arg*/)
  {
    auto semantic = boost::program_options::value< std::vector<T> >();
    semantic->default_value(std::vector<T>(), "");
    return semantic;
  }

  template<typename T, bool required>
  void add_arg(boost::program_options::options_description& description, const arg_descriptor<T, required>& arg, bool unique = true)
  {
    if (0 != description.find_nothrow(arg.name, false))
    {
      CHECK_AND_ASSERT_MES(!unique, void(), "Argument already exists: " << arg.name);
      return;
    }

    description.add_options()(arg.name, make_semantic(arg), arg.description);
  }

  template<typename T>
  void add_arg(boost::program_options::options_description& description, const arg_descriptor<T, false>& arg, const T& def, bool unique = true)
  {
    if (0 != description.find_nothrow(arg.name, false))
    {
      CHECK_AND_ASSERT_MES(!unique, void(), "Argument already exists: " << arg.name);
      return;
    }

    description.add_options()(arg.name, make_semantic(arg, def), arg.description);
  }

  template<>
  inline void add_arg(boost::program_options::options_description& description, const arg_descriptor<bool, false>& arg, bool unique)
  {
    if (0 != description.find_nothrow(arg.name, false))
    {
      CHECK_AND_ASSERT_MES(!unique, void(), "Argument already exists: " << arg.name);
      return;
    }

    description.add_options()(arg.name, boost::program_options::bool_switch(), arg.description);
  }

  template<typename charT>
  boost::program_options::basic_parsed_options<charT> parse_command_line(int argc, const charT* const argv[],
    const boost::program_options::options_description& desc, bool allow_unregistered = false)
  {
    auto parser = boost::program_options::command_line_parser(argc, argv);
    parser.options(desc);
    if (allow_unregistered)
    {
      parser.allow_unregistered();
    }
    return parser.run();
  }

  template<typename F>
  bool handle_error_helper(const boost::program_options::options_description& desc, F parser)
  {
    try
    {
      return parser();
    }
    catch (const std::exception& e)
    {
      std::cerr << "Failed to parse arguments: " << e.what() << std::endl;
      std::cerr << desc << std::endl;
      return false;
    }
    catch (...)
    {
      std::cerr << "Failed to parse arguments: unknown exception" << std::endl;
      std::cerr << desc << std::endl;
      return false;
    }
  }

  template<typename T, bool required>
  bool has_arg(const boost::program_options::variables_map& vm, const arg_descriptor<T, required>& arg)
  {
    auto value = vm[arg.name];
    return !value.empty();
  }

  template<typename T, bool required>
  bool is_arg_defaulted(const boost::program_options::variables_map& vm, const arg_descriptor<T, required>& arg)
  {
    return vm[arg.name].defaulted();
  }


  template<typename T, bool required>
  T get_arg(const boost::program_options::variables_map& vm, const arg_descriptor<T, required>& arg)
  {
    return vm[arg.name].template as<T>();
  }
 
  template<>
  inline bool has_arg<bool, false>(const boost::program_options::variables_map& vm, const arg_descriptor<bool, false>& arg)
  {
    return get_arg<bool, false>(vm, arg);
  }


  extern const arg_descriptor<bool> arg_help;
  extern const arg_descriptor<bool> arg_version;
}
