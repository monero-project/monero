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

#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/tx_extra.h"
#include "cryptonote_core/blockchain.h"
#include "common/command_line.h"
#include "version.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "debugtools.deserialize"

namespace po = boost::program_options;
using namespace epee;

using namespace cryptonote;

int main(int argc, char* argv[])
{
  uint32_t log_level = 0;
  std::string input;

  tools::sanitize_locale();

  boost::filesystem::path output_file_path;

  po::options_description desc_cmd_only("Command line options");
  po::options_description desc_cmd_sett("Command line options and settings options");
  const command_line::arg_descriptor<std::string> arg_output_file = {"output-file", "Specify output file", "", true};
  const command_line::arg_descriptor<uint32_t> arg_log_level  = {"log-level",  "", log_level};
  const command_line::arg_descriptor<std::string> arg_input = {"input", "Specify input has a hexadecimal string", ""};

  command_line::add_arg(desc_cmd_sett, arg_output_file);
  command_line::add_arg(desc_cmd_sett, arg_log_level);
  command_line::add_arg(desc_cmd_sett, arg_input);

  command_line::add_arg(desc_cmd_only, command_line::arg_help);

  po::options_description desc_options("Allowed options");
  desc_options.add(desc_cmd_only).add(desc_cmd_sett);

  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc_options, [&]()
  {
    po::store(po::parse_command_line(argc, argv, desc_options), vm);
    po::notify(vm);
    return true;
  });
  if (! r)
    return 1;

  if (command_line::get_arg(vm, command_line::arg_help))
  {
    std::cout << "Monero '" << MONERO_RELEASE_NAME << "' (v" << MONERO_VERSION_FULL << ")" << ENDL << ENDL;
    std::cout << desc_options << std::endl;
    return 1;
  }

  log_level    = command_line::get_arg(vm, arg_log_level);
  input        = command_line::get_arg(vm, arg_input);
  if (input.empty())
  {
    std::cerr << "--input is mandatory" << std::endl;
    return 1;
  }

  mlog_configure("", true);

  std::string m_config_folder;

  std::ostream *output;
  std::ofstream *raw_data_file = NULL;
  if (command_line::has_arg(vm, arg_output_file))
  {
    output_file_path = boost::filesystem::path(command_line::get_arg(vm, arg_output_file));

    const boost::filesystem::path dir_path = output_file_path.parent_path();
    if (!dir_path.empty())
    {
      if (boost::filesystem::exists(dir_path))
      {
        if (!boost::filesystem::is_directory(dir_path))
        {
          std::cerr << "output directory path is a file: " << dir_path << std::endl;
          return 1;
        }
      }
      else
      {
        if (!boost::filesystem::create_directory(dir_path))
        {
          std::cerr << "Failed to create directory " << dir_path << std::endl;
          return 1;
        }
      }
    }

    raw_data_file = new std::ofstream();
    raw_data_file->open(output_file_path.string(), std::ios_base::out | std::ios::trunc);
    if (raw_data_file->fail())
      return 1;
    output = raw_data_file;
  }
  else
  {
    output_file_path = "";
    output = &std::cout;
  }

  cryptonote::blobdata blob;
  if (!epee::string_tools::parse_hexstr_to_binbuff(input, blob))
  {
    std::cerr << "Invalid hex input" << std::endl;
    std::cerr << "Invalid hex input: " << input << std::endl;
    return 1;
  }

  cryptonote::block block;
  cryptonote::transaction tx;
  std::vector<cryptonote::tx_extra_field> fields;
  if (cryptonote::parse_and_validate_block_from_blob(blob, block))
  {
    std::cout << "Parsed block:" << std::endl;
    std::cout << cryptonote::obj_to_json_str(block) << std::endl;
  }
  else if (cryptonote::parse_and_validate_tx_from_blob(blob, tx))
  {
    std::cout << "Parsed transaction:" << std::endl;
    std::cout << cryptonote::obj_to_json_str(tx) << std::endl;

    if (cryptonote::parse_tx_extra(tx.extra, fields))
    {
      std::cout << "tx_extra has " << fields.size() << " field(s)" << std::endl;
      for (size_t n = 0; n < fields.size(); ++n)
      {
        std::cout << "field " << n << ": ";
        if (typeid(cryptonote::tx_extra_padding) == fields[n].type()) std::cout << "extra padding: " << boost::get<cryptonote::tx_extra_padding>(fields[n]).size << " bytes";
        else if (typeid(cryptonote::tx_extra_pub_key) == fields[n].type()) std::cout << "extra pub key: " << boost::get<cryptonote::tx_extra_pub_key>(fields[n]).pub_key;
        else if (typeid(cryptonote::tx_extra_nonce) == fields[n].type()) std::cout << "extra nonce: " << epee::string_tools::buff_to_hex_nodelimer(boost::get<cryptonote::tx_extra_nonce>(fields[n]).nonce);
        else if (typeid(cryptonote::tx_extra_merge_mining_tag) == fields[n].type()) std::cout << "extra merge mining tag: depth " << boost::get<cryptonote::tx_extra_merge_mining_tag>(fields[n]).depth << ", merkle root " << boost::get<cryptonote::tx_extra_merge_mining_tag>(fields[n]).merkle_root;
        else if (typeid(cryptonote::tx_extra_mysterious_minergate) == fields[n].type()) std::cout << "extra minergate custom: " << epee::string_tools::buff_to_hex_nodelimer(boost::get<cryptonote::tx_extra_mysterious_minergate>(fields[n]).data);
        else std::cout << "unknown";
        std::cout << std::endl;
      }
    }
    else
    {
      std::cout << "Failed to parse tx_extra" << std::endl;
    }
  }
  else
  {
    std::cerr << "Not a recognized CN type" << std::endl;
    return 1;
  }



  if (output->fail())
    return 1;
  output->flush();
  if (raw_data_file)
    delete raw_data_file;

  return 0;
}
