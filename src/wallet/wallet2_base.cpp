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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "wallet2_base.h"
#include "wallet2.h"
#include "net/abstract_http_client.h"

namespace tools
{
std::unique_ptr<wallet2_base> wallet2_base::create(cryptonote::network_type nettype, uint64_t kdf_rounds, bool unattended, std::unique_ptr<epee::net_utils::http::http_client_factory> http_client_factory)
{
    return std::unique_ptr<wallet2_base>(new wallet2(nettype, kdf_rounds, unattended, std::unique_ptr<epee::net_utils::http::http_client_factory>(http_client_factory.release())));
}
wallet2_base::~wallet2_base()
{
}

// Delegating all static calls to wallet2
const char* wallet2_base::tr(const char* str)
{
    return wallet2::tr(str);
}
bool wallet2_base::has_testnet_option(const boost::program_options::variables_map& vm)
{
    return wallet2::has_testnet_option(vm);
}
bool wallet2_base::has_stagenet_option(const boost::program_options::variables_map& vm)
{
    return wallet2::has_stagenet_option(vm);
}
std::string wallet2_base::device_name_option(const boost::program_options::variables_map& vm)
{
    return wallet2::device_name_option(vm);
}
std::string wallet2_base::device_derivation_path_option(const boost::program_options::variables_map &vm)
{
    return wallet2::device_derivation_path_option(vm);
}
void wallet2_base::init_options(boost::program_options::options_description& desc_params)
{
    return wallet2::init_options(desc_params);
}
std::pair<std::unique_ptr<wallet2_base>, password_container> wallet2_base::make_from_json(const boost::program_options::variables_map& vm, bool unattended, const std::string& json_file, const std::function<boost::optional<password_container>(const char *, bool)> &password_prompter)
{
    return wallet2::make_from_json(vm, unattended, json_file, password_prompter);
}
std::pair<std::unique_ptr<wallet2_base>, password_container>
  wallet2_base::make_from_file(const boost::program_options::variables_map& vm, bool unattended, const std::string& wallet_file, const std::function<boost::optional<password_container>(const char *, bool)> &password_prompter)
{
    return wallet2::make_from_file(vm, unattended, wallet_file, password_prompter);
}
std::pair<std::unique_ptr<wallet2_base>, password_container> wallet2_base::make_new(const boost::program_options::variables_map& vm, bool unattended, const std::function<boost::optional<password_container>(const char *, bool)> &password_prompter)
{
    return wallet2::make_new(vm, unattended, password_prompter);
}
std::unique_ptr<wallet2_base> wallet2_base::make_dummy(const boost::program_options::variables_map& vm, bool unattended, const std::function<boost::optional<password_container>(const char *, bool)> &password_prompter)
{
    return wallet2::make_dummy(vm, unattended, password_prompter);
}
bool wallet2_base::verify_password(const std::string& keys_file_name, const epee::wipeable_string& password, bool no_spend_key, hw::device &hwdev, uint64_t kdf_rounds)
{
    return wallet2::verify_password(keys_file_name, password, no_spend_key, hwdev, kdf_rounds);
}
bool wallet2_base::query_device(hw::device::device_type& device_type, const std::string& keys_file_name, const epee::wipeable_string& password, uint64_t kdf_rounds)
{
    return wallet2::query_device(device_type, keys_file_name, password, kdf_rounds);
}
bool wallet2_base::verify_multisig_info(const std::string &data, crypto::secret_key &skey, crypto::public_key &pkey)
{
    return wallet2::verify_multisig_info(data, skey, pkey);
}
bool wallet2_base::verify_extra_multisig_info(const std::string &data, std::unordered_set<crypto::public_key> &pkeys, crypto::public_key &signer)
{
    return wallet2::verify_extra_multisig_info(data, pkeys, signer);
}
void wallet2_base::wallet_exists(const std::string& file_path, bool& keys_file_exists, bool& wallet_file_exists)
{
    return wallet2::wallet_exists(file_path, keys_file_exists, wallet_file_exists);
}
bool wallet2_base::wallet_valid_path_format(const std::string& file_path)
{
    return wallet2::wallet_valid_path_format(file_path);
}
bool wallet2_base::parse_long_payment_id(const std::string& payment_id_str, crypto::hash& payment_id)
{
    return wallet2::parse_long_payment_id(payment_id_str, payment_id);
}
bool wallet2_base::parse_short_payment_id(const std::string& payment_id_str, crypto::hash8& payment_id)
{
    return wallet2::parse_short_payment_id(payment_id_str, payment_id);
}
bool wallet2_base::parse_payment_id(const std::string& payment_id_str, crypto::hash& payment_id)
{
    return wallet2::parse_payment_id(payment_id_str, payment_id);
}
bool wallet2_base::load_from_file(const std::string& path_to_file, std::string& target_str, size_t max_size)
{
    return wallet2::load_from_file(path_to_file, target_str, max_size);
}
std::string wallet2_base::get_default_daemon_address()
{
    return wallet2::get_default_daemon_address();
}

}
