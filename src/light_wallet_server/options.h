#pragma once

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <stdexcept>
#include <string>

#include "common/command_line.h"
#include "common/util.h"

namespace lws
{
    constexpr const char default_db_subdir[] = "/light_wallet_server";

    struct options
    {
        const command_line::arg_descriptor<std::string> db_path;
        const command_line::arg_descriptor<std::string> network;

        options()
          : db_path{"db-path", "Folder for LMDB files", tools::get_default_data_dir() + default_db_subdir}
          , network{"network", "<\"main\"|\"stage\"|\"test\"> - Blockchain net type", "main"}
        {}

        void prepare(boost::program_options::options_description& description) const
        {
            command_line::add_arg(description, db_path);
            command_line::add_arg(description, network);
            command_line::add_arg(description, command_line::arg_help);
        }

        void set_network(boost::program_options::variables_map const& args) const
        {
            const std::string net = command_line::get_arg(args, network);
            if (net == "main")
                lws::config::network = cryptonote::MAINNET;
            else if (net == "stage")
                lws::config::network = cryptonote::STAGENET;
            else if (net == "test")
                lws::config::network = cryptonote::TESTNET;
            else
                throw std::runtime_error{"Bad --network value"};
        }
    };
}

