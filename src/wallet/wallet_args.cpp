// Copyright (c) 2014-2016, The Monero Project
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
#include "wallet/wallet_args.h"

#include <boost/filesystem/path.hpp>
#include <boost/format.hpp>
#include "common/i18n.h"
#include "common/scoped_message_writer.h"
#include "common/util.h"
#include "misc_log_ex.h"
#include "string_tools.h"
#include "version.h"

#if defined(WIN32)
#include <crtdbg.h>
#endif

// workaround for a suspected bug in pthread/kernel on MacOS X
#ifdef __APPLE__
#define DEFAULT_MAX_CONCURRENCY 1
#else
#define DEFAULT_MAX_CONCURRENCY 0
#endif


namespace wallet_args
{
  // Create on-demand to prevent static initialization order fiasco issues.
  command_line::arg_descriptor<std::string> arg_generate_from_json()
  {
    return {"generate-from-json", wallet_args::tr("Generate wallet from JSON format file"), ""};
  }
  command_line::arg_descriptor<std::string> arg_wallet_file()
  {
    return {"wallet-file", wallet_args::tr("Use wallet <arg>"), ""};
  }

  const char* tr(const char* str)
  {
    return i18n_translate(str, "wallet_args");
  }

  boost::optional<boost::program_options::variables_map> main(
    int argc, char** argv,
    const char* const usage,
    boost::program_options::options_description desc_params,
    const boost::program_options::positional_options_description& positional_options)
  
  {
    namespace bf = boost::filesystem;
    namespace po = boost::program_options;
#ifdef WIN32
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    const command_line::arg_descriptor<uint32_t> arg_log_level = {"log-level", "", LOG_LEVEL_0};
    const command_line::arg_descriptor<uint32_t> arg_max_concurrency = {"max-concurrency", wallet_args::tr("Max number of threads to use for a parallel job"), DEFAULT_MAX_CONCURRENCY};
    const command_line::arg_descriptor<std::string> arg_log_file = {"log-file", wallet_args::tr("Specify log file"), ""};


    std::string lang = i18n_get_language();
    tools::sanitize_locale();
    tools::set_strict_default_file_permissions(true);

    epee::string_tools::set_module_name_and_folder(argv[0]);

    po::options_description desc_general(wallet_args::tr("General options"));
    command_line::add_arg(desc_general, command_line::arg_help);
    command_line::add_arg(desc_general, command_line::arg_version);


    bf::path default_log {epee::log_space::log_singletone::get_default_log_folder()};
    std::string log_file_name = epee::log_space::log_singletone::get_default_log_file();
    if (log_file_name.empty())
    {
      // Sanity check: File path should also be empty if file name is. If not,
      // this would be a problem in epee's discovery of current process's file
      // path.
      if (! default_log.empty())
      {
        tools::fail_msg_writer() << wallet_args::tr("unexpected empty log file name in presence of non-empty file path");
        return boost::none;
      }
      // epee didn't find path to executable from argv[0], so use this default file name.
      log_file_name = "monero-wallet-cli.log";
      // The full path will use cwd because epee also returned an empty default log folder.
    }
    default_log /= log_file_name;

    command_line::add_arg(desc_params, arg_log_file, default_log.string());
    command_line::add_arg(desc_params, arg_log_level);
    command_line::add_arg(desc_params, arg_max_concurrency);

    i18n_set_language("translations", "monero", lang);

    po::options_description desc_all;
    desc_all.add(desc_general).add(desc_params);
    po::variables_map vm;
    bool r = command_line::handle_error_helper(desc_all, [&]()
    {
      po::store(command_line::parse_command_line(argc, argv, desc_general, true), vm);

      if (command_line::get_arg(vm, command_line::arg_help))
      {
        tools::msg_writer() << "Monero '" << MONERO_RELEASE_NAME << "' (v" << MONERO_VERSION_FULL << ")";
        tools::msg_writer() << wallet_args::tr("Usage:") << ' ' << usage;
        tools::msg_writer() << desc_all;
        return false;
      }
      else if (command_line::get_arg(vm, command_line::arg_version))
      {
        tools::msg_writer() << "Monero '" << MONERO_RELEASE_NAME << "' (v" << MONERO_VERSION_FULL << ")";
        return false;
      }

      auto parser = po::command_line_parser(argc, argv).options(desc_params).positional(positional_options);
      po::store(parser.run(), vm);
      po::notify(vm);
      return true;
    });
    if (!r)
      return boost::none;

    // log_file_path
    //   default: < argv[0] directory >/monero-wallet-cli.log
    //     so if ran as "monero-wallet-cli" (no path), log file will be in cwd
    //
    //   if log-file argument given:
    //     absolute path
    //     relative path: relative to cwd

    // Set log file
    bf::path log_file_path {bf::absolute(command_line::get_arg(vm, arg_log_file))};

    // Set up logging options
    int log_level = LOG_LEVEL_2;
    epee::log_space::get_set_log_detalisation_level(true, log_level);
    //epee::log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL, LOG_LEVEL_0);
    epee::log_space::log_singletone::add_logger(LOGGER_FILE,
      log_file_path.filename().string().c_str(),
      log_file_path.parent_path().string().c_str(),
      LOG_LEVEL_4
    );

    if(command_line::has_arg(vm, arg_max_concurrency))
      tools::set_max_concurrency(command_line::get_arg(vm, arg_max_concurrency));

    tools::scoped_message_writer(epee::log_space::console_color_white, true) << "Monero '" << MONERO_RELEASE_NAME << "' (v" << MONERO_VERSION_FULL << ")";

    if(command_line::has_arg(vm, arg_log_level))
      log_level = command_line::get_arg(vm, arg_log_level);
    LOG_PRINT_L0("Setting log level = " << log_level);
    LOG_PRINT_L0(wallet_args::tr("default_log: ") << default_log.string());
    tools::scoped_message_writer(epee::log_space::console_color_white, true) << boost::format(wallet_args::tr("Logging at log level %d to %s")) %
      log_level % log_file_path.string();
    epee::log_space::get_set_log_detalisation_level(true, log_level);

    return {std::move(vm)};
  }
}
