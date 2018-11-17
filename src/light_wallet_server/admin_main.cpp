// Copyright (c) 2018, The Monero Project
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

#include <algorithm>
#include <boost/optional/optional.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <cassert>
#include <cstring>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "common/command_line.h"
#include "common/expect.h"
#include "light_wallet_server/config.h"
#include "light_wallet_server/error.h"
#include "light_wallet_server/db/storage.h"
#include "light_wallet_server/db/string.h"
#include "light_wallet_server/options.h"
#include "misc_log_ex.h"
#include "serialization/new/json_output.h"
#include "span.h"
#include "string_tools.h"

namespace 
{
    struct address_json_
    {
        expect<void> operator()(std::ostream& dest, lws::db::account_address const& src) const
        {
            const expect<std::string> user = lws::db::address_string(src);
            if (!user)
                return user.error();

            return ::json::string(dest, *user);
        }
    };
    constexpr const address_json_ address_json{};

    struct truncated_account_json_
    {
        expect<void> operator()(std::ostream& dest, lws::db::account const& src) const
        {
            static constexpr const auto fmt = json::object(
                json::field("address", address_json),
                json::field("scan_height", json::uint64),
                json::field("access_time", json::uint32)
            );
            return fmt(dest, src.address, src.scan_height, src.access);
        }
    };
    constexpr const truncated_account_json_ truncated_account_json{};

    constexpr const auto updated_list_json = ::json::object(
        ::json::field("updated", ::json::array(address_json))
    );

    struct options : lws::options
    {
        const command_line::arg_descriptor<bool> show_sensitive;
        const command_line::arg_descriptor<std::string> command;
        const command_line::arg_descriptor<std::vector<std::string>> arguments;

        options()
          : lws::options()
          , show_sensitive{"show-sensitive", "Show view keys", false}
          , command{"command", "Admin command to execute", ""}
          , arguments{"arguments", "Arguments to command"}
        {}

        void prepare(boost::program_options::options_description& description) const
        {
            lws::options::prepare(description);
            command_line::add_arg(description, show_sensitive);
            command_line::add_arg(description, command);
            command_line::add_arg(description, arguments);
        }
    };

    struct program
    {
        lws::db::storage disk;
        std::vector<std::string> arguments;
        bool show_sensitive;
    };

    crypto::secret_key get_key(std::string const& hex)
    {
        crypto::secret_key out{};
        if (!epee::string_tools::hex_to_pod(hex, out))
            MONERO_THROW(lws::error::bad_view_key, "View key has invalid hex");
        return out;
    }

    std::vector<lws::db::account_address> get_addresses(epee::span<const std::string> arguments)
    {
        // first entry is currently always some other option
        assert(!arguments.empty());
        arguments.remove_prefix(1);

        std::vector<lws::db::account_address> addresses{};
        for (std::string const& address : arguments)
            addresses.push_back(lws::db::address_string(address).value());
        return addresses;
    }

    void accept_requests(program prog, std::ostream& out)
    {
        if (prog.arguments.size() < 2)
            throw std::runtime_error{"accept_requests requires 2 or more arguments"};

        const lws::db::request req =
            MONERO_UNWRAP(lws::db::request_string(prog.arguments[0]));
        std::vector<lws::db::account_address> addresses =
            get_addresses(epee::to_span(prog.arguments));

        const std::vector<lws::db::account_address> updated =
            prog.disk.accept_requests(req, epee::to_span(addresses)).value();
        MONERO_UNWRAP(updated_list_json(out, epee::to_span(updated)));
    }

    void add_account(program prog, std::ostream& out)
    {
        if (prog.arguments.size() != 2)
            throw std::runtime_error{"add_account needs exactly two arguments"};

        const lws::db::account_address address[1] = {
            lws::db::address_string(prog.arguments[0]).value()
        };
        const crypto::secret_key key{get_key(prog.arguments[1])};

        MONERO_UNWRAP(prog.disk.add_account(address[0], key));
        MONERO_UNWRAP(updated_list_json(out, address));
    }

    void debug_database(program prog, std::ostream& out)
    {
        if (!prog.arguments.empty())
            throw std::runtime_error{"debug_database takes zero arguments"};

        auto reader = prog.disk.start_read().value();
        reader.json_debug(out, prog.show_sensitive);
    }

    void list_accounts(program prog, std::ostream& out)
    {
        static constexpr const auto fmt = json::map(
            json::from_enum(lws::db::status_string), json::array(truncated_account_json)
        );

        if (!prog.arguments.empty())
            throw std::runtime_error{"list_accounts takes zero arguments"};

        auto reader = prog.disk.start_read().value();
        auto stream = reader.get_accounts().value();
        MONERO_UNWRAP(fmt(out, stream.make_range()));
    }

    void list_requests(program prog, std::ostream& out)
    {
        struct truncated_request_json
        {
            expect<void> operator()(std::ostream& dest, lws::db::request_info const& request) const
            {
                static constexpr const auto fmt = json::object(
                    json::field("address", address_json),
                    json::field("start_height", json::uint64)
                );
                return fmt(dest, request.address, request.start_height);
            }
        };

        static constexpr const auto fmt = json::map(
            json::from_enum(lws::db::request_string), json::array(truncated_request_json{})
        );

        if (!prog.arguments.empty())
            throw std::runtime_error{"list_requests takes zero arguments"};

        auto reader = prog.disk.start_read().value();
        auto stream = reader.get_requests().value();
        MONERO_UNWRAP(fmt(out, stream.make_range()));
    }

    void modify_account(program prog, std::ostream& out)
    {
        if (prog.arguments.size() < 2)
            throw std::runtime_error{"modify_account_status requires 2 or more arguments"};

        const lws::db::account_status status =
            lws::db::status_string(prog.arguments[0]).value();
        std::vector<lws::db::account_address> addresses =
            get_addresses(epee::to_span(prog.arguments));

        const std::vector<lws::db::account_address> updated =
            prog.disk.change_status(status, epee::to_span(addresses)).value();

        MONERO_UNWRAP(updated_list_json(out, epee::to_span(updated)));
    }

    void reject_requests(program prog, std::ostream& out)
    {
        if (prog.arguments.size() < 2)
            MONERO_THROW(common_error::kInvalidArgument, "reject_requests requires 2 or more arguments");

        const lws::db::request req =
            lws::db::request_string(prog.arguments[0]).value();
        std::vector<lws::db::account_address> addresses =
            get_addresses(epee::to_span(prog.arguments));

        MONERO_UNWRAP(prog.disk.reject_requests(req, epee::to_span(addresses)));
    }

    void rescan(program prog, std::ostream& out)
    {
        if (prog.arguments.size() < 2)
            throw std::runtime_error{"rescan requires 2 or more arguments"};

        const auto height = lws::db::block_id(std::stoull(prog.arguments[0]));
        const std::vector<lws::db::account_address> addresses =
            get_addresses(epee::to_span(prog.arguments));

        const std::vector<lws::db::account_address> updated =
            prog.disk.rescan(height, epee::to_span(addresses)).value();
        MONERO_UNWRAP(updated_list_json(out, epee::to_span(updated)));
    }

    void rollback(program prog, std::ostream& out)
    {
        static constexpr const auto new_height_json = json::object(
            json::field("new_height", json::uint64)
        );

        if (prog.arguments.size() != 1)
            throw std::runtime_error{"rollback requires 1 argument"};

        const auto height = lws::db::block_id(std::stoull(prog.arguments[0]));
        MONERO_UNWRAP(prog.disk.rollback(height));
        MONERO_UNWRAP(new_height_json(out, height));
    }

    struct command
    {
        char const* const name;
        void (*const handler)(program, std::ostream&);
        char const* const parameters;
    };

    static constexpr const command commands[] = {
        {"accept_requests",       &accept_requests, "<\"create\"|\"import\"> <base58 address> [base 58 address]..."},
        {"add_account",           &add_account,     "<base58 address> <view key hex>"},
        {"debug_database",        &debug_database,  ""},
        {"list_accounts",         &list_accounts,   ""},
        {"list_requests",         &list_requests,   ""},
        {"modify_account_status", &modify_account,  "<\"active\"|\"inactive\"|\"hidden\"> <base58 address> [base 58 address]..."},
        {"reject_requests",       &reject_requests, "<\"create\"|\"import\"> <base58 address> [base 58 address]..."},
        {"rescan",                &rescan,          "<height> <base58 address> [base 58 address]..."},
        {"rollback",              &rollback,        "<height>"}
    };

    void print_help(std::ostream& out)
    {
        boost::program_options::options_description description{"Options"};
        options{}.prepare(description);

        out << "Usage: [options] [command] [arguments]" << std::endl;
        out << description << std::endl;
        out << "Commands:" << std::endl;
        for (command cmd : commands)
        {
            out << "  " << cmd.name << "\t\t" << cmd.parameters << std::endl;
        }
    }

    boost::optional<std::pair<std::string, program>> get_program(int argc, char** argv)
    {
        namespace po = boost::program_options;

        const options opts{};
        po::variables_map args{};
        {
            po::options_description description{"Options"};
            opts.prepare(description);

            po::positional_options_description positional{};
            positional.add(opts.command.name, 1);
            positional.add(opts.arguments.name, -1);

            po::store(
                po::command_line_parser(argc, argv)
                    .options(description).positional(positional).run()
              , args
            );
            po::notify(args);
        }

        if (command_line::get_arg(args, command_line::arg_help))
        {
            print_help(std::cout);
            return boost::none;
        }

        opts.set_network(args); // do this first, sets global variable :/

        program prog{
            lws::db::storage::open(command_line::get_arg(args, opts.db_path).c_str(), 0)
        };

        prog.show_sensitive = command_line::get_arg(args, opts.show_sensitive);
        auto cmd = args[opts.command.name];
        if (cmd.empty())
            throw std::runtime_error{"No command given"};

        prog.arguments = command_line::get_arg(args, opts.arguments);
        return {{cmd.as<std::string>(), std::move(prog)}};
    }

    void run(boost::string_ref name, program prog, std::ostream& out)
    {
        struct by_name
        {
            bool operator()(command const& left, command const& right) const noexcept
            {
                assert(left.name && right.name);
                return std::strcmp(left.name, right.name) < 0;
            }
            bool operator()(boost::string_ref left, command const& right) const noexcept
            {
                assert(right.name);
                return left < right.name;
            }
            bool operator()(command const& left, boost::string_ref right) const noexcept
            {
                assert(left.name);
                return left.name < right;
            }
        };

        assert(std::is_sorted(std::begin(commands), std::end(commands), by_name{}));
        const auto found = std::lower_bound(
            std::begin(commands), std::end(commands), name, by_name{}
        );
        if (found == std::end(commands) || found->name != name)
            throw std::runtime_error{"No such command"};

        assert(found->handler != nullptr);
        found->handler(std::move(prog), out);

        if (out.bad())
            MONERO_THROW(std::io_errc::stream, "Writing to stdout failed");

        out << std::endl;
    }
}

int main (int argc, char** argv)
{
    try
    {
        mlog_configure("", false, 0, 0); // disable logging

        boost::optional<std::pair<std::string, program>> prog;

        try
        { 
            prog = get_program(argc, argv);
        }
        catch (std::exception const& e)
        {
            std::cerr << e.what() << std::endl << std::endl;
            print_help(std::cerr);
            return EXIT_FAILURE;
        }

        if (prog)
            run(prog->first, std::move(prog->second), std::cout);
    }
    catch (std::exception const& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cerr << "Unknown exception" << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
