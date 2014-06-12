// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "command_line.h"
#include "string_tools.h"

namespace command_line
{
  std::string input_line(const std::string& prompt)
  {
    std::cout << prompt;

    std::string buf;
    std::getline(std::cin, buf);

    return epee::string_tools::trim(buf);

  }

  const arg_descriptor<bool> arg_help = {"help", "Produce help message"};
  const arg_descriptor<bool> arg_version = {"version", "Output version information"};
  const arg_descriptor<std::string> arg_data_dir = {"data-dir", "Specify data directory"};
}
