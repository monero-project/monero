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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include <random>
#include <tuple>
#include <boost/format.hpp>
#include <boost/optional/optional.hpp>
#include <boost/utility/value_init.hpp>
#include "include_base_utils.h"
using namespace epee;

#include "cryptonote_config.h"
#include "wallet2.h"
#include "wallet2_api.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "misc_language.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "common/boost_serialization_helper.h"
#include "common/command_line.h"
#include "profile_tools.h"
#include "crypto/crypto.h"
#include "serialization/binary_utils.h"
#include "cryptonote_protocol/blobdatatype.h"
#include "mnemonics/electrum-words.h"
#include "common/i18n.h"
#include "common/util.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "common/json_util.h"
#include "common/base58.h"
#include "common/scoped_message_writer.h"
#include "ringct/rctSigs.h"

extern "C"
{
#include "crypto/keccak.h"
#include "crypto/crypto-ops.h"
}
using namespace cryptonote;

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet.wallet2"

// used to choose when to stop adding outputs to a tx
#define APPROXIMATE_INPUT_BYTES 80

// used to target a given block size (additional outputs may be added on top to build fee)
#define TX_SIZE_TARGET(bytes) (bytes*2/3)

// arbitrary, used to generate different hashes from the same input
#define CHACHA8_KEY_TAIL 0x8c

#define UNSIGNED_TX_PREFIX "Monero unsigned tx set\003"
#define SIGNED_TX_PREFIX "Monero signed tx set\003"

#define RECENT_OUTPUT_RATIO (0.5) // 50% of outputs are from the recent zone
#define RECENT_OUTPUT_ZONE ((time_t)(1.8 * 86400)) // last 1.8 day makes up the recent zone (taken from monerolink.pdf, Miller et al)

#define FEE_ESTIMATE_GRACE_BLOCKS 10 // estimate fee valid for that many blocks

#define SECOND_OUTPUT_RELATEDNESS_THRESHOLD 0.0f

#define KILL_IOSERVICE()  \
    do { \
      work.reset(); \
      while (!ioservice.stopped()) ioservice.poll(); \
      threadpool.join_all(); \
      ioservice.stop(); \
    } while(0)

#define KEY_IMAGE_EXPORT_FILE_MAGIC "Monero key image export\002"

namespace
{
// Create on-demand to prevent static initialization order fiasco issues.
struct options {
  const command_line::arg_descriptor<std::string> daemon_address = {"daemon-address", tools::wallet2::tr("Use daemon instance at <host>:<port>"), ""};
  const command_line::arg_descriptor<std::string> daemon_host = {"daemon-host", tools::wallet2::tr("Use daemon instance at host <arg> instead of localhost"), ""};
  const command_line::arg_descriptor<std::string> password = {"password", tools::wallet2::tr("Wallet password (escape/quote as needed)"), "", true};
  const command_line::arg_descriptor<std::string> password_file = {"password-file", tools::wallet2::tr("Wallet password file"), "", true};
  const command_line::arg_descriptor<int> daemon_port = {"daemon-port", tools::wallet2::tr("Use daemon instance at port <arg> instead of 18081"), 0};
  const command_line::arg_descriptor<std::string> daemon_login = {"daemon-login", tools::wallet2::tr("Specify username[:password] for daemon RPC client"), "", true};
  const command_line::arg_descriptor<bool> testnet = {"testnet", tools::wallet2::tr("For testnet. Daemon must also be launched with --testnet flag"), false};
  const command_line::arg_descriptor<bool> restricted = {"restricted-rpc", tools::wallet2::tr("Restricts to view-only commands"), false};
};

void do_prepare_file_names(const std::string& file_path, std::string& keys_file, std::string& wallet_file)
{
  keys_file = file_path;
  wallet_file = file_path;
  boost::system::error_code e;
  if(string_tools::get_extension(keys_file) == "keys")
  {//provided keys file name
    wallet_file = string_tools::cut_off_extension(wallet_file);
  }else
  {//provided wallet file name
    keys_file += ".keys";
  }
}

uint64_t calculate_fee(uint64_t fee_per_kb, size_t bytes, uint64_t fee_multiplier)
{
  uint64_t kB = (bytes + 1023) / 1024;
  return kB * fee_per_kb * fee_multiplier;
}

uint64_t calculate_fee(uint64_t fee_per_kb, const cryptonote::blobdata &blob, uint64_t fee_multiplier)
{
  return calculate_fee(fee_per_kb, blob.size(), fee_multiplier);
}

std::unique_ptr<tools::wallet2> make_basic(const boost::program_options::variables_map& vm, const options& opts)
{
  const bool testnet = command_line::get_arg(vm, opts.testnet);
  const bool restricted = command_line::get_arg(vm, opts.restricted);

  auto daemon_address = command_line::get_arg(vm, opts.daemon_address);
  auto daemon_host = command_line::get_arg(vm, opts.daemon_host);
  auto daemon_port = command_line::get_arg(vm, opts.daemon_port);

  if (!daemon_address.empty() && !daemon_host.empty() && 0 != daemon_port)
  {
    tools::fail_msg_writer() << tools::wallet2::tr("can't specify daemon host or port more than once");
    return nullptr;
  }

  boost::optional<epee::net_utils::http::login> login{};
  if (command_line::has_arg(vm, opts.daemon_login))
  {
    auto parsed = tools::login::parse(
      command_line::get_arg(vm, opts.daemon_login), false, "Daemon client password"
    );
    if (!parsed)
      return nullptr;

    login.emplace(std::move(parsed->username), std::move(parsed->password).password());
  }

  if (daemon_host.empty())
    daemon_host = "localhost";

  if (!daemon_port)
  {
    daemon_port = testnet ? config::testnet::RPC_DEFAULT_PORT : config::RPC_DEFAULT_PORT;
  }

  if (daemon_address.empty())
    daemon_address = std::string("http://") + daemon_host + ":" + std::to_string(daemon_port);

  std::unique_ptr<tools::wallet2> wallet(new tools::wallet2(testnet, restricted));
  wallet->init(std::move(daemon_address), std::move(login));
  return wallet;
}

boost::optional<tools::password_container> get_password(const boost::program_options::variables_map& vm, const options& opts, const bool verify)
{
  if (command_line::has_arg(vm, opts.password) && command_line::has_arg(vm, opts.password_file))
  {
    tools::fail_msg_writer() << tools::wallet2::tr("can't specify more than one of --password and --password-file");
    return boost::none;
  }

  if (command_line::has_arg(vm, opts.password))
  {
    return tools::password_container{command_line::get_arg(vm, opts.password)};
  }

  if (command_line::has_arg(vm, opts.password_file))
  {
    std::string password;
    bool r = epee::file_io_utils::load_file_to_string(command_line::get_arg(vm, opts.password_file),
                                                      password);
    if (!r)
    {
      tools::fail_msg_writer() << tools::wallet2::tr("the password file specified could not be read");
      return boost::none;
    }

    // Remove line breaks the user might have inserted
    boost::trim_right_if(password, boost::is_any_of("\r\n"));
    return {tools::password_container{std::move(password)}};
  }

  return tools::wallet2::password_prompt(verify);
}

std::unique_ptr<tools::wallet2> generate_from_json(const std::string& json_file, const boost::program_options::variables_map& vm, const options& opts)
{
  const bool testnet = command_line::get_arg(vm, opts.testnet);

  /* GET_FIELD_FROM_JSON_RETURN_ON_ERROR Is a generic macro that can return
  false. Gcc will coerce this into unique_ptr(nullptr), but clang correctly
  fails. This large wrapper is for the use of that macro */
  std::unique_ptr<tools::wallet2> wallet;
  const auto do_generate = [&]() -> bool {
    std::string buf;
    if (!epee::file_io_utils::load_file_to_string(json_file, buf)) {
      tools::fail_msg_writer() << tools::wallet2::tr("Failed to load file ") << json_file;
      return false;
    }

    rapidjson::Document json;
    if (json.Parse(buf.c_str()).HasParseError()) {
      tools::fail_msg_writer() << tools::wallet2::tr("Failed to parse JSON");
      return false;
    }

    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, version, unsigned, Uint, true, 0);
    const int current_version = 1;
    if (field_version > current_version) {
      tools::fail_msg_writer() << boost::format(tools::wallet2::tr("Version %u too new, we can only grok up to %u")) % field_version % current_version;
      return false;
    }

    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, filename, std::string, String, true, std::string());

    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, scan_from_height, uint64_t, Uint64, false, 0);
    const bool recover = field_scan_from_height_found;

    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, password, std::string, String, false, std::string());

    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, viewkey, std::string, String, false, std::string());
    crypto::secret_key viewkey;
    if (field_viewkey_found)
    {
      cryptonote::blobdata viewkey_data;
      if(!epee::string_tools::parse_hexstr_to_binbuff(field_viewkey, viewkey_data) || viewkey_data.size() != sizeof(crypto::secret_key))
      {
        tools::fail_msg_writer() << tools::wallet2::tr("failed to parse view key secret key");
        return false;
      }
      viewkey = *reinterpret_cast<const crypto::secret_key*>(viewkey_data.data());
      crypto::public_key pkey;
      if (!crypto::secret_key_to_public_key(viewkey, pkey)) {
        tools::fail_msg_writer() << tools::wallet2::tr("failed to verify view key secret key");
        return false;
      }
    }

    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, spendkey, std::string, String, false, std::string());
    crypto::secret_key spendkey;
    if (field_spendkey_found)
    {
      cryptonote::blobdata spendkey_data;
      if(!epee::string_tools::parse_hexstr_to_binbuff(field_spendkey, spendkey_data) || spendkey_data.size() != sizeof(crypto::secret_key))
      {
        tools::fail_msg_writer() << tools::wallet2::tr("failed to parse spend key secret key");
        return false;
      }
      spendkey = *reinterpret_cast<const crypto::secret_key*>(spendkey_data.data());
      crypto::public_key pkey;
      if (!crypto::secret_key_to_public_key(spendkey, pkey)) {
        tools::fail_msg_writer() << tools::wallet2::tr("failed to verify spend key secret key");
        return false;
      }
    }

    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, seed, std::string, String, false, std::string());
    std::string old_language;
    crypto::secret_key recovery_key;
    bool restore_deterministic_wallet = false;
    if (field_seed_found)
    {
      if (!crypto::ElectrumWords::words_to_bytes(field_seed, recovery_key, old_language))
      {
        tools::fail_msg_writer() << tools::wallet2::tr("Electrum-style word list failed verification");
        return false;
      }
      restore_deterministic_wallet = true;
    }

    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, address, std::string, String, false, std::string());

    // compatibility checks
    if (!field_seed_found && !field_viewkey_found)
    {
      tools::fail_msg_writer() << tools::wallet2::tr("At least one of Electrum-style word list and private view key must be specified");
      return false;
    }
    if (field_seed_found && (field_viewkey_found || field_spendkey_found))
    {
      tools::fail_msg_writer() << tools::wallet2::tr("Both Electrum-style word list and private key(s) specified");
      return false;
    }

    // if an address was given, we check keys against it, and deduce the spend
    // public key if it was not given
    if (field_address_found)
    {
      cryptonote::account_public_address address;
      bool has_payment_id;
      crypto::hash8 new_payment_id;
      if(!get_account_integrated_address_from_str(address, has_payment_id, new_payment_id, testnet, field_address))
      {
        tools::fail_msg_writer() << tools::wallet2::tr("invalid address");
        return false;
      }
      if (field_viewkey_found)
      {
        crypto::public_key pkey;
        if (!crypto::secret_key_to_public_key(viewkey, pkey)) {
          tools::fail_msg_writer() << tools::wallet2::tr("failed to verify view key secret key");
          return false;
        }
        if (address.m_view_public_key != pkey) {
          tools::fail_msg_writer() << tools::wallet2::tr("view key does not match standard address");
          return false;
        }
      }
      if (field_spendkey_found)
      {
        crypto::public_key pkey;
        if (!crypto::secret_key_to_public_key(spendkey, pkey)) {
          tools::fail_msg_writer() << tools::wallet2::tr("failed to verify spend key secret key");
          return false;
        }
        if (address.m_spend_public_key != pkey) {
          tools::fail_msg_writer() << tools::wallet2::tr("spend key does not match standard address");
          return false;
        }
      }
    }

    const bool deprecated_wallet = restore_deterministic_wallet && ((old_language == crypto::ElectrumWords::old_language_name) ||
      crypto::ElectrumWords::get_is_old_style_seed(field_seed));
    if (deprecated_wallet) {
      tools::fail_msg_writer() << tools::wallet2::tr("Cannot create deprecated wallets from JSON");
      return false;
    }

    wallet.reset(make_basic(vm, opts).release());
    wallet->set_refresh_from_block_height(field_scan_from_height);

    try
    {
      if (!field_seed.empty())
      {
        wallet->generate(field_filename, field_password, recovery_key, recover, false);
      }
      else
      {
        cryptonote::account_public_address address;
        if (!crypto::secret_key_to_public_key(viewkey, address.m_view_public_key)) {
          tools::fail_msg_writer() << tools::wallet2::tr("failed to verify view key secret key");
          return false;
        }

        if (field_spendkey.empty())
        {
          // if we have an addres but no spend key, we can deduce the spend public key
          // from the address
          if (field_address_found)
          {
            cryptonote::account_public_address address2;
            bool has_payment_id;
            crypto::hash8 new_payment_id;
            get_account_integrated_address_from_str(address2, has_payment_id, new_payment_id, testnet, field_address);
            address.m_spend_public_key = address2.m_spend_public_key;
          }
          wallet->generate(field_filename, field_password, address, viewkey);
        }
        else
        {
          if (!crypto::secret_key_to_public_key(spendkey, address.m_spend_public_key)) {
            tools::fail_msg_writer() << tools::wallet2::tr("failed to verify spend key secret key");
            return false;
          }
          wallet->generate(field_filename, field_password, address, spendkey, viewkey);
        }
      }
    }
    catch (const std::exception& e)
    {
      tools::fail_msg_writer() << tools::wallet2::tr("failed to generate new wallet: ") << e.what();
      return false;
    }
    return true;
  };

  if (do_generate())
  {
    return wallet;
  }
  return nullptr;
}

static void throw_on_rpc_response_error(const boost::optional<std::string> &status, const char *method)
{
  // no error
  if (!status)
    return;

  // empty string -> not connection
  THROW_WALLET_EXCEPTION_IF(status->empty(), tools::error::no_connection_to_daemon, method);

  THROW_WALLET_EXCEPTION_IF(*status == CORE_RPC_STATUS_BUSY, tools::error::daemon_busy, method);
  THROW_WALLET_EXCEPTION_IF(*status != CORE_RPC_STATUS_OK, tools::error::wallet_generic_rpc_error, method, *status);
}

} //namespace

namespace tools
{
// for now, limit to 30 attempts.  TODO: discuss a good number to limit to.
const size_t MAX_SPLIT_ATTEMPTS = 30;

constexpr const std::chrono::seconds wallet2::rpc_timeout;
const char* wallet2::tr(const char* str) { return i18n_translate(str, "tools::wallet2"); }

bool wallet2::has_testnet_option(const boost::program_options::variables_map& vm)
{
  return command_line::get_arg(vm, options().testnet);
}

void wallet2::init_options(boost::program_options::options_description& desc_params)
{
  const options opts{};
  command_line::add_arg(desc_params, opts.daemon_address);
  command_line::add_arg(desc_params, opts.daemon_host);
  command_line::add_arg(desc_params, opts.password);
  command_line::add_arg(desc_params, opts.password_file);
  command_line::add_arg(desc_params, opts.daemon_port);
  command_line::add_arg(desc_params, opts.daemon_login);
  command_line::add_arg(desc_params, opts.testnet);
  command_line::add_arg(desc_params, opts.restricted);
}

boost::optional<password_container> wallet2::password_prompt(const bool new_password)
{
  auto pwd_container = tools::password_container::prompt(
    new_password, (new_password ? tr("Enter new wallet password") : tr("Wallet password"))
  );
  if (!pwd_container)
  {
    tools::fail_msg_writer() << tr("failed to read wallet password");
  }
  return pwd_container;
}

std::unique_ptr<wallet2> wallet2::make_from_json(const boost::program_options::variables_map& vm, const std::string& json_file)
{
  const options opts{};
  return generate_from_json(json_file, vm, opts);
}

std::pair<std::unique_ptr<wallet2>, password_container> wallet2::make_from_file(
  const boost::program_options::variables_map& vm, const std::string& wallet_file)
{
  const options opts{};
  auto pwd = get_password(vm, opts, false);
  if (!pwd)
  {
    return {nullptr, password_container{}};
  }
  auto wallet = make_basic(vm, opts);
  if (wallet)
  {
    wallet->load(wallet_file, pwd->password());
  }
  return {std::move(wallet), std::move(*pwd)};
}

std::pair<std::unique_ptr<wallet2>, password_container> wallet2::make_new(const boost::program_options::variables_map& vm)
{
  const options opts{};
  auto pwd = get_password(vm, opts, true);
  if (!pwd)
  {
    return {nullptr, password_container{}};
  }
  return {make_basic(vm, opts), std::move(*pwd)};
}

std::unique_ptr<wallet2> wallet2::make_dummy(const boost::program_options::variables_map& vm)
{
  const options opts{};
  return make_basic(vm, opts);
}

//----------------------------------------------------------------------------------------------------
bool wallet2::init(std::string daemon_address, boost::optional<epee::net_utils::http::login> daemon_login, uint64_t upper_transaction_size_limit)
{
  if(m_http_client.is_connected())
    m_http_client.disconnect();
  m_is_initialized = true;
  m_upper_transaction_size_limit = upper_transaction_size_limit;
  m_daemon_address = std::move(daemon_address);
  m_daemon_login = std::move(daemon_login);
  return m_http_client.set_server(get_daemon_address(), get_daemon_login());
}
//----------------------------------------------------------------------------------------------------
bool wallet2::is_deterministic() const
{
  crypto::secret_key second;
  keccak((uint8_t *)&get_account().get_keys().m_spend_secret_key, sizeof(crypto::secret_key), (uint8_t *)&second, sizeof(crypto::secret_key));
  sc_reduce32((uint8_t *)&second);
  bool keys_deterministic = memcmp(second.data,get_account().get_keys().m_view_secret_key.data, sizeof(crypto::secret_key)) == 0;
  return keys_deterministic;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::get_seed(std::string& electrum_words) const
{
  bool keys_deterministic = is_deterministic();
  if (!keys_deterministic)
  {
    std::cout << "This is not a deterministic wallet" << std::endl;
    return false;
  }
  if (seed_language.empty())
  {
    std::cout << "seed_language not set" << std::endl;
    return false;
  }

  crypto::ElectrumWords::bytes_to_words(get_account().get_keys().m_spend_secret_key, electrum_words, seed_language);

  return true;
}
/*!
 * \brief Gets the seed language
 */
const std::string &wallet2::get_seed_language() const
{
  return seed_language;
}
/*!
 * \brief Sets the seed language
 * \param language  Seed language to set to
 */
void wallet2::set_seed_language(const std::string &language)
{
  seed_language = language;
}
/*!
 * \brief Tells if the wallet file is deprecated.
 */
bool wallet2::is_deprecated() const
{
  return is_old_file_format;
}
//----------------------------------------------------------------------------------------------------
void wallet2::set_spent(size_t idx, uint64_t height)
{
  transfer_details &td = m_transfers[idx];
  LOG_PRINT_L2("Setting SPENT at " << height << ": ki " << td.m_key_image << ", amount " << print_money(td.m_amount));
  td.m_spent = true;
  td.m_spent_height = height;
}
//----------------------------------------------------------------------------------------------------
void wallet2::set_unspent(size_t idx)
{
  transfer_details &td = m_transfers[idx];
  LOG_PRINT_L2("Setting UNSPENT: ki " << td.m_key_image << ", amount " << print_money(td.m_amount));
  td.m_spent = false;
  td.m_spent_height = 0;
}
//----------------------------------------------------------------------------------------------------
void wallet2::check_acc_out_precomp(const crypto::public_key &spend_public_key, const tx_out &o, const crypto::key_derivation &derivation, size_t i, bool &received, uint64_t &money_transfered, bool &error) const
{
  if (o.target.type() !=  typeid(txout_to_key))
  {
     error = true;
     LOG_ERROR("wrong type id in transaction out");
     return;
  }
  received = is_out_to_acc_precomp(spend_public_key, boost::get<txout_to_key>(o.target), derivation, i);
  if(received)
  {
    money_transfered = o.amount; // may be 0 for ringct outputs
  }
  else
  {
    money_transfered = 0;
  }
  error = false;
}
//----------------------------------------------------------------------------------------------------
static uint64_t decodeRct(const rct::rctSig & rv, const crypto::public_key &pub, const crypto::secret_key &sec, unsigned int i, rct::key & mask)
{
  crypto::key_derivation derivation;
  bool r = crypto::generate_key_derivation(pub, sec, derivation);
  if (!r)
  {
    LOG_ERROR("Failed to generate key derivation to decode rct output " << i);
    return 0;
  }
  crypto::secret_key scalar1;
  crypto::derivation_to_scalar(derivation, i, scalar1);
  try
  {
    switch (rv.type)
    {
    case rct::RCTTypeSimple:
      return rct::decodeRctSimple(rv, rct::sk2rct(scalar1), i, mask);
    case rct::RCTTypeFull:
      return rct::decodeRct(rv, rct::sk2rct(scalar1), i, mask);
    default:
      LOG_ERROR("Unsupported rct type: " << rv.type);
      return 0;
    }
  }
  catch (const std::exception &e)
  {
    LOG_ERROR("Failed to decode input " << i);
    return 0;
  }
}
//----------------------------------------------------------------------------------------------------
bool wallet2::wallet_generate_key_image_helper(const cryptonote::account_keys& ack, const crypto::public_key& tx_public_key, size_t real_output_index, cryptonote::keypair& in_ephemeral, crypto::key_image& ki)
{
  if (!cryptonote::generate_key_image_helper(ack, tx_public_key, real_output_index, in_ephemeral, ki))
    return false;
  return true;
}
//----------------------------------------------------------------------------------------------------
void wallet2::process_new_transaction(const crypto::hash &txid, const cryptonote::transaction& tx, const std::vector<uint64_t> &o_indices, uint64_t height, uint64_t ts, bool miner_tx, bool pool)
{
  // In this function, tx (probably) only contains the base information
  // (that is, the prunable stuff may or may not be included)

  if (!miner_tx)
    process_unconfirmed(txid, tx, height);
  std::vector<size_t> outs;
  uint64_t tx_money_got_in_outs = 0;
  crypto::public_key tx_pub_key = null_pkey;

  std::vector<tx_extra_field> tx_extra_fields;
  if(!parse_tx_extra(tx.extra, tx_extra_fields))
  {
    // Extra may only be partially parsed, it's OK if tx_extra_fields contains public key
    LOG_PRINT_L0("Transaction extra has unsupported format: " << txid);
  }

  // Don't try to extract tx public key if tx has no ouputs
  size_t pk_index = 0;
  while (!tx.vout.empty())
  {
    // if tx.vout is not empty, we loop through all tx pubkeys

    tx_extra_pub_key pub_key_field;
    if(!find_tx_extra_field_by_type(tx_extra_fields, pub_key_field, pk_index++))
    {
      if (pk_index > 1)
        break;
      LOG_PRINT_L0("Public key wasn't found in the transaction extra. Skipping transaction " << txid);
      if(0 != m_callback)
	m_callback->on_skip_transaction(height, txid, tx);
      return;
    }

    int num_vouts_received = 0;
    tx_pub_key = pub_key_field.pub_key;
    bool r = true;
    std::deque<cryptonote::keypair> in_ephemeral(tx.vout.size());
    std::deque<crypto::key_image> ki(tx.vout.size());
    std::deque<uint64_t> amount(tx.vout.size());
    std::deque<rct::key> mask(tx.vout.size());
    int threads = tools::get_max_concurrency();
    const cryptonote::account_keys& keys = m_account.get_keys();
    crypto::key_derivation derivation;
    generate_key_derivation(tx_pub_key, keys.m_view_secret_key, derivation);
    if (miner_tx && m_refresh_type == RefreshNoCoinbase)
    {
      // assume coinbase isn't for us
    }
    else if (miner_tx && m_refresh_type == RefreshOptimizeCoinbase)
    {
      uint64_t money_transfered = 0;
      bool error = false, received = false;
      check_acc_out_precomp(keys.m_account_address.m_spend_public_key, tx.vout[0], derivation, 0, received, money_transfered, error);
      if (error)
      {
        r = false;
      }
      else
      {
        // this assumes that the miner tx pays a single address
        if (received)
        {
          wallet_generate_key_image_helper(keys, tx_pub_key, 0, in_ephemeral[0], ki[0]);
          THROW_WALLET_EXCEPTION_IF(in_ephemeral[0].pub != boost::get<cryptonote::txout_to_key>(tx.vout[0].target).key,
              error::wallet_internal_error, "key_image generated ephemeral public key not matched with output_key");

          outs.push_back(0);
          if (money_transfered == 0)
          {
            money_transfered = tools::decodeRct(tx.rct_signatures, pub_key_field.pub_key, keys.m_view_secret_key, 0, mask[0]);
          }
          amount[0] = money_transfered;
          tx_money_got_in_outs = money_transfered;
          ++num_vouts_received;

          // process the other outs from that tx
          boost::asio::io_service ioservice;
          boost::thread_group threadpool;
          std::unique_ptr < boost::asio::io_service::work > work(new boost::asio::io_service::work(ioservice));
          for (int i = 0; i < threads; i++)
          {
            threadpool.create_thread(boost::bind(&boost::asio::io_service::run, &ioservice));
          }

          std::vector<uint64_t> money_transfered(tx.vout.size());
          std::deque<bool> error(tx.vout.size());
          std::deque<bool> received(tx.vout.size());
          // the first one was already checked
          for (size_t i = 1; i < tx.vout.size(); ++i)
          {
            ioservice.dispatch(boost::bind(&wallet2::check_acc_out_precomp, this, std::cref(keys.m_account_address.m_spend_public_key), std::cref(tx.vout[i]), std::cref(derivation), i,
              std::ref(received[i]), std::ref(money_transfered[i]), std::ref(error[i])));
          }
          KILL_IOSERVICE();
          for (size_t i = 1; i < tx.vout.size(); ++i)
          {
            if (error[i])
            {
              r = false;
              break;
            }
            if (received[i])
            {
              wallet_generate_key_image_helper(keys, tx_pub_key, i, in_ephemeral[i], ki[i]);
              THROW_WALLET_EXCEPTION_IF(in_ephemeral[i].pub != boost::get<cryptonote::txout_to_key>(tx.vout[i].target).key,
                  error::wallet_internal_error, "key_image generated ephemeral public key not matched with output_key");

              outs.push_back(i);
              if (money_transfered[i] == 0)
              {
                money_transfered[i] = tools::decodeRct(tx.rct_signatures, pub_key_field.pub_key, keys.m_view_secret_key, i, mask[i]);
              }
              tx_money_got_in_outs += money_transfered[i];
              amount[i] = money_transfered[i];
              ++num_vouts_received;
            }
          }
        }
      }
    }
    else if (tx.vout.size() > 1 && threads > 1)
    {
      boost::asio::io_service ioservice;
      boost::thread_group threadpool;
      std::unique_ptr < boost::asio::io_service::work > work(new boost::asio::io_service::work(ioservice));
      for (int i = 0; i < threads; i++)
      {
        threadpool.create_thread(boost::bind(&boost::asio::io_service::run, &ioservice));
      }

      std::vector<uint64_t> money_transfered(tx.vout.size());
      std::deque<bool> error(tx.vout.size());
      std::deque<bool> received(tx.vout.size());
      for (size_t i = 0; i < tx.vout.size(); ++i)
      {
        ioservice.dispatch(boost::bind(&wallet2::check_acc_out_precomp, this, std::cref(keys.m_account_address.m_spend_public_key), std::cref(tx.vout[i]), std::cref(derivation), i,
          std::ref(received[i]), std::ref(money_transfered[i]), std::ref(error[i])));
      }
      KILL_IOSERVICE();
      tx_money_got_in_outs = 0;
      for (size_t i = 0; i < tx.vout.size(); ++i)
      {
        if (error[i])
        {
          r = false;
          break;
        }
        if (received[i])
        {
          wallet_generate_key_image_helper(keys, tx_pub_key, i, in_ephemeral[i], ki[i]);
          THROW_WALLET_EXCEPTION_IF(in_ephemeral[i].pub != boost::get<cryptonote::txout_to_key>(tx.vout[i].target).key,
              error::wallet_internal_error, "key_image generated ephemeral public key not matched with output_key");

          outs.push_back(i);
          if (money_transfered[i] == 0)
          {
            money_transfered[i] = tools::decodeRct(tx.rct_signatures, pub_key_field.pub_key, keys.m_view_secret_key, i, mask[i]);
          }
          tx_money_got_in_outs += money_transfered[i];
          amount[i] = money_transfered[i];
          ++num_vouts_received;
        }
      }
    }
    else
    {
      for (size_t i = 0; i < tx.vout.size(); ++i)
      {
        uint64_t money_transfered = 0;
        bool error = false, received = false;
        check_acc_out_precomp(keys.m_account_address.m_spend_public_key, tx.vout[i], derivation, i, received, money_transfered, error);
        if (error)
        {
          r = false;
          break;
        }
        else
        {
          if (received)
          {
            wallet_generate_key_image_helper(keys, tx_pub_key, i, in_ephemeral[i], ki[i]);
            THROW_WALLET_EXCEPTION_IF(in_ephemeral[i].pub != boost::get<cryptonote::txout_to_key>(tx.vout[i].target).key,
                error::wallet_internal_error, "key_image generated ephemeral public key not matched with output_key");

            outs.push_back(i);
            if (money_transfered == 0)
            {
              money_transfered = tools::decodeRct(tx.rct_signatures, pub_key_field.pub_key, keys.m_view_secret_key, i, mask[i]);
            }
            amount[i] = money_transfered;
            tx_money_got_in_outs += money_transfered;
            ++num_vouts_received;
          }
        }
      }
    }
    THROW_WALLET_EXCEPTION_IF(!r, error::acc_outs_lookup_error, tx, tx_pub_key, m_account.get_keys());

    if(!outs.empty() && num_vouts_received > 0)
    {
      //good news - got money! take care about it
      //usually we have only one transfer for user in transaction
      if (!pool)
      {
        THROW_WALLET_EXCEPTION_IF(tx.vout.size() != o_indices.size(), error::wallet_internal_error,
            "transactions outputs size=" + std::to_string(tx.vout.size()) +
            " not match with daemon response size=" + std::to_string(o_indices.size()));
      }

      for(size_t o: outs)
      {
	THROW_WALLET_EXCEPTION_IF(tx.vout.size() <= o, error::wallet_internal_error, "wrong out in transaction: internal index=" +
				  std::to_string(o) + ", total_outs=" + std::to_string(tx.vout.size()));

        auto kit = m_pub_keys.find(in_ephemeral[o].pub);
	THROW_WALLET_EXCEPTION_IF(kit != m_pub_keys.end() && kit->second >= m_transfers.size(),
            error::wallet_internal_error, std::string("Unexpected transfer index from public key: ")
            + "got " + (kit == m_pub_keys.end() ? "<none>" : boost::lexical_cast<std::string>(kit->second))
            + ", m_transfers.size() is " + boost::lexical_cast<std::string>(m_transfers.size()));
        if (kit == m_pub_keys.end())
        {
          if (!pool)
          {
	    m_transfers.push_back(boost::value_initialized<transfer_details>());
	    transfer_details& td = m_transfers.back();
	    td.m_block_height = height;
	    td.m_internal_output_index = o;
	    td.m_global_output_index = o_indices[o];
	    td.m_tx = (const cryptonote::transaction_prefix&)tx;
	    td.m_txid = txid;
            td.m_key_image = ki[o];
            td.m_key_image_known = !m_watch_only;
            td.m_amount = tx.vout[o].amount;
            td.m_pk_index = pk_index - 1;
            if (td.m_amount == 0)
            {
              td.m_mask = mask[o];
              td.m_amount = amount[o];
              td.m_rct = true;
            }
            else if (miner_tx && tx.version == 2)
            {
              td.m_mask = rct::identity();
              td.m_rct = true;
            }
            else
            {
              td.m_mask = rct::identity();
              td.m_rct = false;
            }
	    set_unspent(m_transfers.size()-1);
	    m_key_images[td.m_key_image] = m_transfers.size()-1;
	    m_pub_keys[in_ephemeral[o].pub] = m_transfers.size()-1;
	    LOG_PRINT_L0("Received money: " << print_money(td.amount()) << ", with tx: " << txid);
	    if (0 != m_callback)
	      m_callback->on_money_received(height, txid, tx, td.m_amount);
          }
        }
	else if (m_transfers[kit->second].m_spent || m_transfers[kit->second].amount() >= tx.vout[o].amount)
        {
	  LOG_ERROR("Public key " << epee::string_tools::pod_to_hex(kit->first)
              << " from received " << print_money(tx.vout[o].amount) << " output already exists with "
              << (m_transfers[kit->second].m_spent ? "spent" : "unspent") << " "
              << print_money(m_transfers[kit->second].amount()) << ", received output ignored");
        }
        else
        {
	  LOG_ERROR("Public key " << epee::string_tools::pod_to_hex(kit->first)
              << " from received " << print_money(tx.vout[o].amount) << " output already exists with "
              << print_money(m_transfers[kit->second].amount()) << ", replacing with new output");
          // The new larger output replaced a previous smaller one
          tx_money_got_in_outs -= tx.vout[o].amount;

          if (!pool)
          {
            transfer_details &td = m_transfers[kit->second];
	    td.m_block_height = height;
	    td.m_internal_output_index = o;
	    td.m_global_output_index = o_indices[o];
	    td.m_tx = (const cryptonote::transaction_prefix&)tx;
	    td.m_txid = txid;
            td.m_amount = tx.vout[o].amount;
            td.m_pk_index = pk_index - 1;
            if (td.m_amount == 0)
            {
              td.m_mask = mask[o];
              td.m_amount = amount[o];
              td.m_rct = true;
            }
            else if (miner_tx && tx.version == 2)
            {
              td.m_mask = rct::identity();
              td.m_rct = true;
            }
            else
            {
              td.m_mask = rct::identity();
              td.m_rct = false;
            }
            THROW_WALLET_EXCEPTION_IF(td.get_public_key() != in_ephemeral[o].pub, error::wallet_internal_error, "Inconsistent public keys");
	    THROW_WALLET_EXCEPTION_IF(td.m_spent, error::wallet_internal_error, "Inconsistent spent status");

	    LOG_PRINT_L0("Received money: " << print_money(td.amount()) << ", with tx: " << txid);
	    if (0 != m_callback)
	      m_callback->on_money_received(height, txid, tx, td.m_amount);
          }
        }
      }
    }
  }

  uint64_t tx_money_spent_in_ins = 0;
  // check all outputs for spending (compare key images)
  for(auto& in: tx.vin)
  {
    if(in.type() != typeid(cryptonote::txin_to_key))
      continue;
    auto it = m_key_images.find(boost::get<cryptonote::txin_to_key>(in).k_image);
    if(it != m_key_images.end())
    {
      transfer_details& td = m_transfers[it->second];
      uint64_t amount = boost::get<cryptonote::txin_to_key>(in).amount;
      if (amount > 0)
      {
        THROW_WALLET_EXCEPTION_IF(amount != td.amount(), error::wallet_internal_error,
            std::string("Inconsistent amount in tx input: got ") + print_money(amount) +
            std::string(", expected ") + print_money(td.amount()));
      }
      amount = td.amount();
      tx_money_spent_in_ins += amount;
      if (!pool)
      {
        LOG_PRINT_L0("Spent money: " << print_money(amount) << ", with tx: " << txid);
        set_spent(it->second, height);
        if (0 != m_callback)
          m_callback->on_money_spent(height, txid, tx, amount, tx);
      }
    }
  }

  if (tx_money_spent_in_ins > 0)
  {
    process_outgoing(txid, tx, height, ts, tx_money_spent_in_ins, tx_money_got_in_outs);
  }

  uint64_t received = (tx_money_spent_in_ins < tx_money_got_in_outs) ? tx_money_got_in_outs - tx_money_spent_in_ins : 0;
  if (0 < received)
  {
    tx_extra_nonce extra_nonce;
    crypto::hash payment_id = null_hash;
    if (find_tx_extra_field_by_type(tx_extra_fields, extra_nonce))
    {
      crypto::hash8 payment_id8 = null_hash8;
      if(get_encrypted_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id8))
      {
        // We got a payment ID to go with this tx
        LOG_PRINT_L2("Found encrypted payment ID: " << payment_id8);
        if (tx_pub_key != null_pkey)
        {
          if (!decrypt_payment_id(payment_id8, tx_pub_key, m_account.get_keys().m_view_secret_key))
          {
            LOG_PRINT_L0("Failed to decrypt payment ID: " << payment_id8);
          }
          else
          {
            LOG_PRINT_L2("Decrypted payment ID: " << payment_id8);
            // put the 64 bit decrypted payment id in the first 8 bytes
            memcpy(payment_id.data, payment_id8.data, 8);
            // rest is already 0, but guard against code changes above
            memset(payment_id.data + 8, 0, 24);
          }
        }
        else
        {
          LOG_PRINT_L1("No public key found in tx, unable to decrypt payment id");
        }
      }
      else if (get_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id))
      {
        LOG_PRINT_L2("Found unencrypted payment ID: " << payment_id);
      }
    }
    else if (get_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id))
    {
      LOG_PRINT_L2("Found unencrypted payment ID: " << payment_id);
    }

    payment_details payment;
    payment.m_tx_hash      = txid;
    payment.m_amount       = received;
    payment.m_block_height = height;
    payment.m_unlock_time  = tx.unlock_time;
    payment.m_timestamp    = ts;
    if (pool) {
      m_unconfirmed_payments.emplace(payment_id, payment);
      if (0 != m_callback)
        m_callback->on_unconfirmed_money_received(height, txid, tx, payment.m_amount);
    }
    else
      m_payments.emplace(payment_id, payment);
    LOG_PRINT_L2("Payment found in " << (pool ? "pool" : "block") << ": " << payment_id << " / " << payment.m_tx_hash << " / " << payment.m_amount);
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::process_unconfirmed(const crypto::hash &txid, const cryptonote::transaction& tx, uint64_t height)
{
  if (m_unconfirmed_txs.empty())
    return;

  auto unconf_it = m_unconfirmed_txs.find(txid);
  if(unconf_it != m_unconfirmed_txs.end()) {
    if (store_tx_info()) {
      try {
        m_confirmed_txs.insert(std::make_pair(txid, confirmed_transfer_details(unconf_it->second, height)));
      }
      catch (...) {
        // can fail if the tx has unexpected input types
        LOG_PRINT_L0("Failed to add outgoing transaction to confirmed transaction map");
      }
    }
    m_unconfirmed_txs.erase(unconf_it);
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::process_outgoing(const crypto::hash &txid, const cryptonote::transaction &tx, uint64_t height, uint64_t ts, uint64_t spent, uint64_t received)
{
  std::pair<std::unordered_map<crypto::hash, confirmed_transfer_details>::iterator, bool> entry = m_confirmed_txs.insert(std::make_pair(txid, confirmed_transfer_details()));
  // fill with the info we know, some info might already be there
  if (entry.second)
  {
    // this case will happen if the tx is from our outputs, but was sent by another
    // wallet (eg, we're a cold wallet and the hot wallet sent it). For RCT transactions,
    // we only see 0 input amounts, so have to deduce amount out from other parameters.
    entry.first->second.m_amount_in = spent;
    if (tx.version == 1)
      entry.first->second.m_amount_out = get_outs_money_amount(tx);
    else
      entry.first->second.m_amount_out = spent - tx.rct_signatures.txnFee;
    entry.first->second.m_change = received;

    std::vector<tx_extra_field> tx_extra_fields;
    if(parse_tx_extra(tx.extra, tx_extra_fields))
    {
      tx_extra_nonce extra_nonce;
      if (find_tx_extra_field_by_type(tx_extra_fields, extra_nonce))
      {
        // we do not care about failure here
        get_payment_id_from_tx_extra_nonce(extra_nonce.nonce, entry.first->second.m_payment_id);
      }
    }
  }
  entry.first->second.m_block_height = height;
  entry.first->second.m_timestamp = ts;
  entry.first->second.m_unlock_time = tx.unlock_time;
}
//----------------------------------------------------------------------------------------------------
void wallet2::process_new_blockchain_entry(const cryptonote::block& b, const cryptonote::block_complete_entry& bche, const crypto::hash& bl_id, uint64_t height, const cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::block_output_indices &o_indices)
{
  size_t txidx = 0;
  THROW_WALLET_EXCEPTION_IF(bche.txs.size() + 1 != o_indices.indices.size(), error::wallet_internal_error,
      "block transactions=" + std::to_string(bche.txs.size()) +
      " not match with daemon response size=" + std::to_string(o_indices.indices.size()));

  //handle transactions from new block
    
  //optimization: seeking only for blocks that are not older then the wallet creation time plus 1 day. 1 day is for possible user incorrect time setup
  if(b.timestamp + 60*60*24 > m_account.get_createtime() && height >= m_refresh_from_block_height)
  {
    TIME_MEASURE_START(miner_tx_handle_time);
    process_new_transaction(get_transaction_hash(b.miner_tx), b.miner_tx, o_indices.indices[txidx++].indices, height, b.timestamp, true, false);
    TIME_MEASURE_FINISH(miner_tx_handle_time);

    TIME_MEASURE_START(txs_handle_time);
    THROW_WALLET_EXCEPTION_IF(bche.txs.size() != b.tx_hashes.size(), error::wallet_internal_error, "Wrong amount of transactions for block");
    size_t idx = 0;
    for (const auto& txblob: bche.txs)
    {
      cryptonote::transaction tx;
      bool r = parse_and_validate_tx_base_from_blob(txblob, tx);
      THROW_WALLET_EXCEPTION_IF(!r, error::tx_parse_error, txblob);
      process_new_transaction(b.tx_hashes[idx], tx, o_indices.indices[txidx++].indices, height, b.timestamp, false, false);
      ++idx;
    }
    TIME_MEASURE_FINISH(txs_handle_time);
    LOG_PRINT_L2("Processed block: " << bl_id << ", height " << height << ", " <<  miner_tx_handle_time + txs_handle_time << "(" << miner_tx_handle_time << "/" << txs_handle_time <<")ms");
  }else
  {
    if (!(height % 100))
      LOG_PRINT_L2( "Skipped block by timestamp, height: " << height << ", block time " << b.timestamp << ", account time " << m_account.get_createtime());
  }
  m_blockchain.push_back(bl_id);
  ++m_local_bc_height;

  if (0 != m_callback)
    m_callback->on_new_block(height, b);
}
//----------------------------------------------------------------------------------------------------
void wallet2::get_short_chain_history(std::list<crypto::hash>& ids) const
{
  size_t i = 0;
  size_t current_multiplier = 1;
  size_t sz = m_blockchain.size();
  if(!sz)
    return;
  size_t current_back_offset = 1;
  bool genesis_included = false;
  while(current_back_offset < sz)
  {
    ids.push_back(m_blockchain[sz-current_back_offset]);
    if(sz-current_back_offset == 0)
      genesis_included = true;
    if(i < 10)
    {
      ++current_back_offset;
    }else
    {
      current_back_offset += current_multiplier *= 2;
    }
    ++i;
  }
  if(!genesis_included)
    ids.push_back(m_blockchain[0]);
}
//----------------------------------------------------------------------------------------------------
void wallet2::parse_block_round(const cryptonote::blobdata &blob, cryptonote::block &bl, crypto::hash &bl_id, bool &error) const
{
  error = !cryptonote::parse_and_validate_block_from_blob(blob, bl);
  if (!error)
    bl_id = get_block_hash(bl);
}
//----------------------------------------------------------------------------------------------------
void wallet2::pull_blocks(uint64_t start_height, uint64_t &blocks_start_height, const std::list<crypto::hash> &short_chain_history, std::list<cryptonote::block_complete_entry> &blocks, std::vector<cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::block_output_indices> &o_indices)
{
  cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request req = AUTO_VAL_INIT(req);
  cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response res = AUTO_VAL_INIT(res);
  req.block_ids = short_chain_history;

  uint32_t rpc_version;
  boost::optional<std::string> result = m_node_rpc_proxy.get_rpc_version(rpc_version);
  // no error
  if (!!result)
  {
    // empty string -> not connection
    THROW_WALLET_EXCEPTION_IF(result->empty(), tools::error::no_connection_to_daemon, "getversion");
    THROW_WALLET_EXCEPTION_IF(*result == CORE_RPC_STATUS_BUSY, tools::error::daemon_busy, "getversion");
    if (*result != CORE_RPC_STATUS_OK)
    {
      MDEBUG("Cannot determined daemon RPC version, not asking for pruned blocks");
      req.prune = false; // old daemon
    }
  }
  else
  {
    if (rpc_version >= MAKE_CORE_RPC_VERSION(1, 7))
    {
      MDEBUG("Daemon is recent enough, asking for pruned blocks");
      req.prune = true;
    }
    else
    {
      MDEBUG("Daemon is too old, not asking for pruned blocks");
      req.prune = false;
    }
  }

  req.start_height = start_height;
  m_daemon_rpc_mutex.lock();
  bool r = net_utils::invoke_http_bin("/getblocks.bin", req, res, m_http_client, rpc_timeout);
  m_daemon_rpc_mutex.unlock();
  THROW_WALLET_EXCEPTION_IF(!r, error::no_connection_to_daemon, "getblocks.bin");
  THROW_WALLET_EXCEPTION_IF(res.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "getblocks.bin");
  THROW_WALLET_EXCEPTION_IF(res.status != CORE_RPC_STATUS_OK, error::get_blocks_error, res.status);
  THROW_WALLET_EXCEPTION_IF(res.blocks.size() != res.output_indices.size(), error::wallet_internal_error,
      "mismatched blocks (" + boost::lexical_cast<std::string>(res.blocks.size()) + ") and output_indices (" +
      boost::lexical_cast<std::string>(res.output_indices.size()) + ") sizes from daemon");

  blocks_start_height = res.start_height;
  blocks = res.blocks;
  o_indices = res.output_indices;
}
//----------------------------------------------------------------------------------------------------
void wallet2::pull_hashes(uint64_t start_height, uint64_t &blocks_start_height, const std::list<crypto::hash> &short_chain_history, std::list<crypto::hash> &hashes)
{
  cryptonote::COMMAND_RPC_GET_HASHES_FAST::request req = AUTO_VAL_INIT(req);
  cryptonote::COMMAND_RPC_GET_HASHES_FAST::response res = AUTO_VAL_INIT(res);
  req.block_ids = short_chain_history;

  req.start_height = start_height;
  m_daemon_rpc_mutex.lock();
  bool r = net_utils::invoke_http_bin("/gethashes.bin", req, res, m_http_client, rpc_timeout);
  m_daemon_rpc_mutex.unlock();
  THROW_WALLET_EXCEPTION_IF(!r, error::no_connection_to_daemon, "gethashes.bin");
  THROW_WALLET_EXCEPTION_IF(res.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "gethashes.bin");
  THROW_WALLET_EXCEPTION_IF(res.status != CORE_RPC_STATUS_OK, error::get_hashes_error, res.status);

  blocks_start_height = res.start_height;
  hashes = res.m_block_ids;
}
//----------------------------------------------------------------------------------------------------
void wallet2::process_blocks(uint64_t start_height, const std::list<cryptonote::block_complete_entry> &blocks, const std::vector<cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::block_output_indices> &o_indices, uint64_t& blocks_added)
{
  size_t current_index = start_height;
  blocks_added = 0;
  size_t tx_o_indices_idx = 0;

  THROW_WALLET_EXCEPTION_IF(blocks.size() != o_indices.size(), error::wallet_internal_error, "size mismatch");

  int threads = tools::get_max_concurrency();
  if (threads > 1)
  {
    std::vector<crypto::hash> round_block_hashes(threads);
    std::vector<cryptonote::block> round_blocks(threads);
    std::deque<bool> error(threads);
    size_t blocks_size = blocks.size();
    std::list<block_complete_entry>::const_iterator blocki = blocks.begin();
    for (size_t b = 0; b < blocks_size; b += threads)
    {
      size_t round_size = std::min((size_t)threads, blocks_size - b);

      boost::asio::io_service ioservice;
      boost::thread_group threadpool;
      std::unique_ptr < boost::asio::io_service::work > work(new boost::asio::io_service::work(ioservice));
      for (size_t i = 0; i < round_size; i++)
      {
        threadpool.create_thread(boost::bind(&boost::asio::io_service::run, &ioservice));
      }

      std::list<block_complete_entry>::const_iterator tmpblocki = blocki;
      for (size_t i = 0; i < round_size; ++i)
      {
        ioservice.dispatch(boost::bind(&wallet2::parse_block_round, this, std::cref(tmpblocki->block),
          std::ref(round_blocks[i]), std::ref(round_block_hashes[i]), std::ref(error[i])));
        ++tmpblocki;
      }
      KILL_IOSERVICE();
      tmpblocki = blocki;
      for (size_t i = 0; i < round_size; ++i)
      {
        THROW_WALLET_EXCEPTION_IF(error[i], error::block_parse_error, tmpblocki->block);
        ++tmpblocki;
      }
      for (size_t i = 0; i < round_size; ++i)
      {
        const crypto::hash &bl_id = round_block_hashes[i];
        cryptonote::block &bl = round_blocks[i];

        if(current_index >= m_blockchain.size())
        {
          process_new_blockchain_entry(bl, *blocki, bl_id, current_index, o_indices[b+i]);
          ++blocks_added;
        }
        else if(bl_id != m_blockchain[current_index])
        {
          //split detected here !!!
          THROW_WALLET_EXCEPTION_IF(current_index == start_height, error::wallet_internal_error,
            "wrong daemon response: split starts from the first block in response " + string_tools::pod_to_hex(bl_id) +
            " (height " + std::to_string(start_height) + "), local block id at this height: " +
            string_tools::pod_to_hex(m_blockchain[current_index]));

          detach_blockchain(current_index);
          process_new_blockchain_entry(bl, *blocki, bl_id, current_index, o_indices[b+i]);
        }
        else
        {
          LOG_PRINT_L2("Block is already in blockchain: " << string_tools::pod_to_hex(bl_id));
        }
        ++current_index;
        ++blocki;
      }
    }
  }
  else
  {
  for(auto& bl_entry: blocks)
  {
    cryptonote::block bl;
    bool r = cryptonote::parse_and_validate_block_from_blob(bl_entry.block, bl);
    THROW_WALLET_EXCEPTION_IF(!r, error::block_parse_error, bl_entry.block);

    crypto::hash bl_id = get_block_hash(bl);
    if(current_index >= m_blockchain.size())
    {
      process_new_blockchain_entry(bl, bl_entry, bl_id, current_index, o_indices[tx_o_indices_idx]);
      ++blocks_added;
    }
    else if(bl_id != m_blockchain[current_index])
    {
      //split detected here !!!
      THROW_WALLET_EXCEPTION_IF(current_index == start_height, error::wallet_internal_error,
        "wrong daemon response: split starts from the first block in response " + string_tools::pod_to_hex(bl_id) +
        " (height " + std::to_string(start_height) + "), local block id at this height: " +
        string_tools::pod_to_hex(m_blockchain[current_index]));

      detach_blockchain(current_index);
      process_new_blockchain_entry(bl, bl_entry, bl_id, current_index, o_indices[tx_o_indices_idx]);
    }
    else
    {
      LOG_PRINT_L2("Block is already in blockchain: " << string_tools::pod_to_hex(bl_id));
    }

    ++current_index;
    ++tx_o_indices_idx;
  }
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::refresh()
{
  uint64_t blocks_fetched = 0;
  refresh(0, blocks_fetched);
}
//----------------------------------------------------------------------------------------------------
void wallet2::refresh(uint64_t start_height, uint64_t & blocks_fetched)
{
  bool received_money = false;
  refresh(start_height, blocks_fetched, received_money);
}
//----------------------------------------------------------------------------------------------------
void wallet2::pull_next_blocks(uint64_t start_height, uint64_t &blocks_start_height, std::list<crypto::hash> &short_chain_history, const std::list<cryptonote::block_complete_entry> &prev_blocks, std::list<cryptonote::block_complete_entry> &blocks, std::vector<cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::block_output_indices> &o_indices, bool &error)
{
  error = false;

  try
  {
    // prepend the last 3 blocks, should be enough to guard against a block or two's reorg
    cryptonote::block bl;
    std::list<cryptonote::block_complete_entry>::const_reverse_iterator i = prev_blocks.rbegin();
    for (size_t n = 0; n < std::min((size_t)3, prev_blocks.size()); ++n)
    {
      bool ok = cryptonote::parse_and_validate_block_from_blob(i->block, bl);
      THROW_WALLET_EXCEPTION_IF(!ok, error::block_parse_error, i->block);
      short_chain_history.push_front(cryptonote::get_block_hash(bl));
      ++i;
    }

    // pull the new blocks
    pull_blocks(start_height, blocks_start_height, short_chain_history, blocks, o_indices);
  }
  catch(...)
  {
    error = true;
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::update_pool_state(bool refreshed)
{
  MDEBUG("update_pool_state start");

  // get the pool state
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL_HASHES::request req;
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL_HASHES::response res;
  m_daemon_rpc_mutex.lock();
  bool r = epee::net_utils::invoke_http_json("/get_transaction_pool_hashes.bin", req, res, m_http_client, rpc_timeout);
  m_daemon_rpc_mutex.unlock();
  THROW_WALLET_EXCEPTION_IF(!r, error::no_connection_to_daemon, "get_transaction_pool_hashes.bin");
  THROW_WALLET_EXCEPTION_IF(res.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "get_transaction_pool_hashes.bin");
  THROW_WALLET_EXCEPTION_IF(res.status != CORE_RPC_STATUS_OK, error::get_tx_pool_error);
  MDEBUG("update_pool_state got pool");

  // remove any pending tx that's not in the pool
  std::unordered_map<crypto::hash, wallet2::unconfirmed_transfer_details>::iterator it = m_unconfirmed_txs.begin();
  while (it != m_unconfirmed_txs.end())
  {
    const crypto::hash &txid = it->first;
    bool found = false;
    for (const auto &it2: res.tx_hashes)
    {
      if (it2 == txid)
      {
        found = true;
        break;
      }
    }
    auto pit = it++;
    if (!found)
    {
      // we want to avoid a false positive when we ask for the pool just after
      // a tx is removed from the pool due to being found in a new block, but
      // just before the block is visible by refresh. So we keep a boolean, so
      // that the first time we don't see the tx, we set that boolean, and only
      // delete it the second time it is checked (but only when refreshed, so
      // we're sure we've seen the blockchain state first)
      if (pit->second.m_state == wallet2::unconfirmed_transfer_details::pending)
      {
        LOG_PRINT_L1("Pending txid " << txid << " not in pool, marking as not in pool");
        pit->second.m_state = wallet2::unconfirmed_transfer_details::pending_not_in_pool;
      }
      else if (pit->second.m_state == wallet2::unconfirmed_transfer_details::pending_not_in_pool && refreshed)
      {
        LOG_PRINT_L1("Pending txid " << txid << " not in pool, marking as failed");
        pit->second.m_state = wallet2::unconfirmed_transfer_details::failed;

        // the inputs aren't spent anymore, since the tx failed
        for (size_t vini = 0; vini < pit->second.m_tx.vin.size(); ++vini)
        {
          if (pit->second.m_tx.vin[vini].type() == typeid(txin_to_key))
          {
            txin_to_key &tx_in_to_key = boost::get<txin_to_key>(pit->second.m_tx.vin[vini]);
            for (size_t i = 0; i < m_transfers.size(); ++i)
            {
              const transfer_details &td = m_transfers[i];
              if (td.m_key_image == tx_in_to_key.k_image)
              {
                 LOG_PRINT_L1("Resetting spent status for output " << vini << ": " << td.m_key_image);
                 set_unspent(i);
                 break;
              }
            }
          }
        }
      }
    }
  }
  MDEBUG("update_pool_state done first loop");

  // remove pool txes to us that aren't in the pool anymore
  // but only if we just refreshed, so that the tx can go in
  // the in transfers list instead (or nowhere if it just
  // disappeared without being mined)
  if (refreshed)
  {
    std::unordered_map<crypto::hash, wallet2::payment_details>::iterator uit = m_unconfirmed_payments.begin();
    while (uit != m_unconfirmed_payments.end())
    {
      const crypto::hash &txid = uit->second.m_tx_hash;
      bool found = false;
      for (const auto &it2: res.tx_hashes)
      {
        if (it2 == txid)
        {
          found = true;
          break;
        }
      }
      auto pit = uit++;
      if (!found)
      {
        MDEBUG("Removing " << txid << " from unconfirmed payments, not found in pool");
        m_unconfirmed_payments.erase(pit);
      }
    }
  }
  MDEBUG("update_pool_state done second loop");

  // gather txids of new pool txes to us
  std::vector<crypto::hash> txids;
  for (const auto &txid: res.tx_hashes)
  {
    if (m_scanned_pool_txs[0].find(txid) != m_scanned_pool_txs[0].end() || m_scanned_pool_txs[1].find(txid) != m_scanned_pool_txs[1].end())
    {
      LOG_PRINT_L2("Already seen " << txid << ", skipped");
      continue;
    }
    bool txid_found_in_up = false;
    for (const auto &up: m_unconfirmed_payments)
    {
      if (up.second.m_tx_hash == txid)
      {
        txid_found_in_up = true;
        break;
      }
    }
    if (!txid_found_in_up)
    {
      LOG_PRINT_L1("Found new pool tx: " << txid);
      bool found = false;
      for (const auto &i: m_unconfirmed_txs)
      {
        if (i.first == txid)
        {
          found = true;
          break;
        }
      }
      if (!found)
      {
        // not one of those we sent ourselves
        txids.push_back(txid);
      }
      else
      {
        LOG_PRINT_L1("We sent that one");
      }
    }
    else
    {
      LOG_PRINT_L1("Already saw that one, it's for us");
    }
  }

  // get those txes
  if (!txids.empty())
  {
    cryptonote::COMMAND_RPC_GET_TRANSACTIONS::request req;
    cryptonote::COMMAND_RPC_GET_TRANSACTIONS::response res;
    for (const auto &txid: txids)
      req.txs_hashes.push_back(epee::string_tools::pod_to_hex(txid));
    MDEBUG("asking for " << txids.size() << " transactions");
    req.decode_as_json = false;
    m_daemon_rpc_mutex.lock();
    bool r = epee::net_utils::invoke_http_json("/gettransactions", req, res, m_http_client, rpc_timeout);
    m_daemon_rpc_mutex.unlock();
    MDEBUG("Got " << r << " and " << res.status);
    if (r && res.status == CORE_RPC_STATUS_OK)
    {
      if (res.txs.size() == txids.size())
      {
        size_t n = 0;
        for (const auto &txid: txids)
        {
          // might have just been put in a block
          if (res.txs[n].in_pool)
          {
            cryptonote::transaction tx;
            cryptonote::blobdata bd;
            crypto::hash tx_hash, tx_prefix_hash;
            if (epee::string_tools::parse_hexstr_to_binbuff(res.txs[n].as_hex, bd))
            {
              if (cryptonote::parse_and_validate_tx_from_blob(bd, tx, tx_hash, tx_prefix_hash))
              {
                if (tx_hash == txid)
                {
                  process_new_transaction(txid, tx, std::vector<uint64_t>(), 0, time(NULL), false, true);
                  m_scanned_pool_txs[0].insert(txid);
                  if (m_scanned_pool_txs[0].size() > 5000)
                  {
                    std::swap(m_scanned_pool_txs[0], m_scanned_pool_txs[1]);
                    m_scanned_pool_txs[0].clear();
                  }
                }
                else
                {
                  LOG_PRINT_L0("Mismatched txids when processing unconfimed txes from pool");
                }
              }
              else
              {
                LOG_PRINT_L0("failed to validate transaction from daemon");
              }
            }
            else
            {
              LOG_PRINT_L0("Failed to parse tx " << txid);
            }
          }
          else
          {
            LOG_PRINT_L1("Tx " << txid << " was in pool, but is no more");
          }
          ++n;
        }
      }
      else
      {
        LOG_PRINT_L0("Expected " << txids.size() << " tx(es), got " << res.txs.size());
      }
    }
    else
    {
      LOG_PRINT_L0("Error calling gettransactions daemon RPC: r " << r << ", status " << res.status);
    }
  }
  MDEBUG("update_pool_state end");
}
//----------------------------------------------------------------------------------------------------
void wallet2::fast_refresh(uint64_t stop_height, uint64_t &blocks_start_height, std::list<crypto::hash> &short_chain_history)
{
  std::list<crypto::hash> hashes;
  size_t current_index = m_blockchain.size();

  while(m_run.load(std::memory_order_relaxed) && current_index < stop_height)
  {
    pull_hashes(0, blocks_start_height, short_chain_history, hashes);
    if (hashes.size() <= 3)
      return;
    if (hashes.size() + current_index < stop_height) {
      std::list<crypto::hash>::iterator right;
      // drop early 3 off, skipping the genesis block
      if (short_chain_history.size() > 3) {
        right = short_chain_history.end();
        std::advance(right,-1);
        std::list<crypto::hash>::iterator left = right;
        std::advance(left, -3);
        short_chain_history.erase(left, right);
      }
      right = hashes.end();
      // prepend 3 more
      for (int i = 0; i<3; i++) {
        right--;
        short_chain_history.push_front(*right);
      }
    }
    current_index = blocks_start_height;
    for(auto& bl_id: hashes)
    {
      if(current_index >= m_blockchain.size())
      {
        if (!(current_index % 1000))
          LOG_PRINT_L2( "Skipped block by height: " << current_index);
        m_blockchain.push_back(bl_id);
        ++m_local_bc_height;

        if (0 != m_callback)
        { // FIXME: this isn't right, but simplewallet just logs that we got a block.
          cryptonote::block dummy;
          m_callback->on_new_block(current_index, dummy);
        }
      }
      else if(bl_id != m_blockchain[current_index])
      {
        //split detected here !!!
        return;
      }
      ++current_index;
      if (current_index >= stop_height)
        return;
    }
  }
}


bool wallet2::add_address_book_row(const cryptonote::account_public_address &address, const crypto::hash &payment_id, const std::string &description)
{
  wallet2::address_book_row a;
  a.m_address = address;
  a.m_payment_id = payment_id;
  a.m_description = description;
  
  auto old_size = m_address_book.size();
  m_address_book.push_back(a);
  if(m_address_book.size() == old_size+1)
    return true;
  return false;
}

bool wallet2::delete_address_book_row(std::size_t row_id) {
  if(m_address_book.size() <= row_id)
    return false;
  
  m_address_book.erase(m_address_book.begin()+row_id);

  return true;
}

//----------------------------------------------------------------------------------------------------
void wallet2::refresh(uint64_t start_height, uint64_t & blocks_fetched, bool& received_money)
{
  received_money = false;
  blocks_fetched = 0;
  uint64_t added_blocks = 0;
  size_t try_count = 0;
  crypto::hash last_tx_hash_id = m_transfers.size() ? m_transfers.back().m_txid : null_hash;
  std::list<crypto::hash> short_chain_history;
  boost::thread pull_thread;
  uint64_t blocks_start_height;
  std::list<cryptonote::block_complete_entry> blocks;
  std::vector<COMMAND_RPC_GET_BLOCKS_FAST::block_output_indices> o_indices;
  bool refreshed = false;

  // pull the first set of blocks
  get_short_chain_history(short_chain_history);
  m_run.store(true, std::memory_order_relaxed);
  if (start_height > m_blockchain.size() || m_refresh_from_block_height > m_blockchain.size()) {
    if (!start_height)
      start_height = m_refresh_from_block_height;
    // we can shortcut by only pulling hashes up to the start_height
    fast_refresh(start_height, blocks_start_height, short_chain_history);
    // regenerate the history now that we've got a full set of hashes
    short_chain_history.clear();
    get_short_chain_history(short_chain_history);
    start_height = 0;
    // and then fall through to regular refresh processing
  }

  // If stop() is called during fast refresh we don't need to continue
  if(!m_run.load(std::memory_order_relaxed))
    return;
  pull_blocks(start_height, blocks_start_height, short_chain_history, blocks, o_indices);
  // always reset start_height to 0 to force short_chain_ history to be used on
  // subsequent pulls in this refresh.
  start_height = 0;

  while(m_run.load(std::memory_order_relaxed))
  {
    try
    {
      // pull the next set of blocks while we're processing the current one
      uint64_t next_blocks_start_height;
      std::list<cryptonote::block_complete_entry> next_blocks;
      std::vector<cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::block_output_indices> next_o_indices;
      bool error = false;
      pull_thread = boost::thread([&]{pull_next_blocks(start_height, next_blocks_start_height, short_chain_history, blocks, next_blocks, next_o_indices, error);});

      process_blocks(blocks_start_height, blocks, o_indices, added_blocks);
      blocks_fetched += added_blocks;
      pull_thread.join();
      if(blocks_start_height == next_blocks_start_height)
      {
        m_node_rpc_proxy.set_height(m_blockchain.size());
        refreshed = true;
        break;
      }

      // switch to the new blocks from the daemon
      blocks_start_height = next_blocks_start_height;
      blocks = next_blocks;
      o_indices = next_o_indices;

      // handle error from async fetching thread
      if (error)
      {
        throw std::runtime_error("proxy exception in refresh thread");
      }
    }
    catch (const std::exception&)
    {
      blocks_fetched += added_blocks;
      if (pull_thread.joinable())
        pull_thread.join();
      if(try_count < 3)
      {
        LOG_PRINT_L1("Another try pull_blocks (try_count=" << try_count << ")...");
        ++try_count;
      }
      else
      {
        LOG_ERROR("pull_blocks failed, try_count=" << try_count);
        throw;
      }
    }
  }
  if(last_tx_hash_id != (m_transfers.size() ? m_transfers.back().m_txid : null_hash))
    received_money = true;

  try
  {
    // If stop() is called we don't need to check pending transactions
    if(m_run.load(std::memory_order_relaxed))
      update_pool_state(refreshed);
  }
  catch (...)
  {
    LOG_PRINT_L1("Failed to check pending transactions");
  }

  LOG_PRINT_L1("Refresh done, blocks received: " << blocks_fetched << ", balance: " << print_money(balance()) << ", unlocked: " << print_money(unlocked_balance()));
}
//----------------------------------------------------------------------------------------------------
bool wallet2::refresh(uint64_t & blocks_fetched, bool& received_money, bool& ok)
{
  try
  {
    refresh(0, blocks_fetched, received_money);
    ok = true;
  }
  catch (...)
  {
    ok = false;
  }
  return ok;
}
//----------------------------------------------------------------------------------------------------
void wallet2::detach_blockchain(uint64_t height)
{
  LOG_PRINT_L0("Detaching blockchain on height " << height);
  size_t transfers_detached = 0;

  for (size_t i = 0; i < m_transfers.size(); ++i)
  {
    wallet2::transfer_details &td = m_transfers[i];
    if (td.m_spent && td.m_spent_height >= height)
    {
      LOG_PRINT_L1("Resetting spent status for output " << i << ": " << td.m_key_image);
      set_unspent(i);
    }
  }

  auto it = std::find_if(m_transfers.begin(), m_transfers.end(), [&](const transfer_details& td){return td.m_block_height >= height;});
  size_t i_start = it - m_transfers.begin();

  for(size_t i = i_start; i!= m_transfers.size();i++)
  {
    auto it_ki = m_key_images.find(m_transfers[i].m_key_image);
    THROW_WALLET_EXCEPTION_IF(it_ki == m_key_images.end(), error::wallet_internal_error, "key image not found");
    m_key_images.erase(it_ki);
  }

  for(size_t i = i_start; i!= m_transfers.size();i++)
  {
    auto it_pk = m_pub_keys.find(m_transfers[i].get_public_key());
    THROW_WALLET_EXCEPTION_IF(it_pk == m_pub_keys.end(), error::wallet_internal_error, "public key not found");
    m_pub_keys.erase(it_pk);
  }
  m_transfers.erase(it, m_transfers.end());

  size_t blocks_detached = m_blockchain.end() - (m_blockchain.begin()+height);
  m_blockchain.erase(m_blockchain.begin()+height, m_blockchain.end());
  m_local_bc_height -= blocks_detached;

  for (auto it = m_payments.begin(); it != m_payments.end(); )
  {
    if(height <= it->second.m_block_height)
      it = m_payments.erase(it);
    else
      ++it;
  }

  for (auto it = m_confirmed_txs.begin(); it != m_confirmed_txs.end(); )
  {
    if(height <= it->second.m_block_height)
      it = m_confirmed_txs.erase(it);
    else
      ++it;
  }

  LOG_PRINT_L0("Detached blockchain on height " << height << ", transfers detached " << transfers_detached << ", blocks detached " << blocks_detached);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::deinit()
{
  m_is_initialized=false;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::clear()
{
  m_blockchain.clear();
  m_transfers.clear();
  m_key_images.clear();
  m_pub_keys.clear();
  m_unconfirmed_txs.clear();
  m_payments.clear();
  m_tx_keys.clear();
  m_confirmed_txs.clear();
  m_unconfirmed_payments.clear();
  m_scanned_pool_txs[0].clear();
  m_scanned_pool_txs[1].clear();
  m_address_book.clear();
  m_local_bc_height = 1;
  return true;
}

/*!
 * \brief Stores wallet information to wallet file.
 * \param  keys_file_name Name of wallet file
 * \param  password       Password of wallet file
 * \param  watch_only     true to save only view key, false to save both spend and view keys
 * \return                Whether it was successful.
 */
bool wallet2::store_keys(const std::string& keys_file_name, const std::string& password, bool watch_only)
{
  std::string account_data;
  cryptonote::account_base account = m_account;

  if (watch_only)
    account.forget_spend_key();
  bool r = epee::serialization::store_t_to_binary(account, account_data);
  CHECK_AND_ASSERT_MES(r, false, "failed to serialize wallet keys");
  wallet2::keys_file_data keys_file_data = boost::value_initialized<wallet2::keys_file_data>();

  // Create a JSON object with "key_data" and "seed_language" as keys.
  rapidjson::Document json;
  json.SetObject();
  rapidjson::Value value(rapidjson::kStringType);
  value.SetString(account_data.c_str(), account_data.length());
  json.AddMember("key_data", value, json.GetAllocator());
  if (!seed_language.empty())
  {
    value.SetString(seed_language.c_str(), seed_language.length());
    json.AddMember("seed_language", value, json.GetAllocator());
  }

  rapidjson::Value value2(rapidjson::kNumberType);
  value2.SetInt(watch_only ? 1 :0); // WTF ? JSON has different true and false types, and not boolean ??
  json.AddMember("watch_only", value2, json.GetAllocator());

  value2.SetInt(m_always_confirm_transfers ? 1 :0);
  json.AddMember("always_confirm_transfers", value2, json.GetAllocator());

  value2.SetInt(m_print_ring_members ? 1 :0);
  json.AddMember("print_ring_members", value2, json.GetAllocator());

  value2.SetInt(m_store_tx_info ? 1 :0);
  json.AddMember("store_tx_info", value2, json.GetAllocator());

  value2.SetUint(m_default_mixin);
  json.AddMember("default_mixin", value2, json.GetAllocator());

  value2.SetUint(m_default_priority);
  json.AddMember("default_priority", value2, json.GetAllocator());

  value2.SetInt(m_auto_refresh ? 1 :0);
  json.AddMember("auto_refresh", value2, json.GetAllocator());

  value2.SetInt(m_refresh_type);
  json.AddMember("refresh_type", value2, json.GetAllocator());

  value2.SetUint64(m_refresh_from_block_height);
  json.AddMember("refresh_height", value2, json.GetAllocator());

  value2.SetInt(m_confirm_missing_payment_id ? 1 :0);
  json.AddMember("confirm_missing_payment_id", value2, json.GetAllocator());

  value2.SetInt(m_ask_password ? 1 :0);
  json.AddMember("ask_password", value2, json.GetAllocator());

  value2.SetUint(m_min_output_count);
  json.AddMember("min_output_count", value2, json.GetAllocator());

  value2.SetUint64(m_min_output_value);
  json.AddMember("min_output_value", value2, json.GetAllocator());

  value2.SetInt(cryptonote::get_default_decimal_point());
  json.AddMember("default_decimal_point", value2, json.GetAllocator());

  value2.SetInt(m_merge_destinations ? 1 :0);
  json.AddMember("merge_destinations", value2, json.GetAllocator());

  value2.SetInt(m_confirm_backlog ? 1 :0);
  json.AddMember("confirm_backlog", value2, json.GetAllocator());

  value2.SetInt(m_testnet ? 1 :0);
  json.AddMember("testnet", value2, json.GetAllocator());

  // Serialize the JSON object
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  json.Accept(writer);
  account_data = buffer.GetString();

  // Encrypt the entire JSON object.
  crypto::chacha8_key key;
  crypto::generate_chacha8_key(password, key);
  std::string cipher;
  cipher.resize(account_data.size());
  keys_file_data.iv = crypto::rand<crypto::chacha8_iv>();
  crypto::chacha8(account_data.data(), account_data.size(), key, keys_file_data.iv, &cipher[0]);
  keys_file_data.account_data = cipher;

  std::string buf;
  r = ::serialization::dump_binary(keys_file_data, buf);
  r = r && epee::file_io_utils::save_string_to_file(keys_file_name, buf); //and never touch wallet_keys_file again, only read
  CHECK_AND_ASSERT_MES(r, false, "failed to generate wallet keys file " << keys_file_name);

  return true;
}
//----------------------------------------------------------------------------------------------------
namespace
{
  bool verify_keys(const crypto::secret_key& sec, const crypto::public_key& expected_pub)
  {
    crypto::public_key pub;
    bool r = crypto::secret_key_to_public_key(sec, pub);
    return r && expected_pub == pub;
  }
}

/*!
 * \brief Load wallet information from wallet file.
 * \param keys_file_name Name of wallet file
 * \param password       Password of wallet file
 */
bool wallet2::load_keys(const std::string& keys_file_name, const std::string& password)
{
  wallet2::keys_file_data keys_file_data;
  std::string buf;
  bool r = epee::file_io_utils::load_file_to_string(keys_file_name, buf);
  THROW_WALLET_EXCEPTION_IF(!r, error::file_read_error, keys_file_name);

  // Decrypt the contents
  r = ::serialization::parse_binary(buf, keys_file_data);
  THROW_WALLET_EXCEPTION_IF(!r, error::wallet_internal_error, "internal error: failed to deserialize \"" + keys_file_name + '\"');
  crypto::chacha8_key key;
  crypto::generate_chacha8_key(password, key);
  std::string account_data;
  account_data.resize(keys_file_data.account_data.size());
  crypto::chacha8(keys_file_data.account_data.data(), keys_file_data.account_data.size(), key, keys_file_data.iv, &account_data[0]);

  // The contents should be JSON if the wallet follows the new format.
  rapidjson::Document json;
  if (json.Parse(account_data.c_str()).HasParseError())
  {
    is_old_file_format = true;
    m_watch_only = false;
    m_always_confirm_transfers = false;
    m_print_ring_members = false;
    m_default_mixin = 0;
    m_default_priority = 0;
    m_auto_refresh = true;
    m_refresh_type = RefreshType::RefreshDefault;
    m_confirm_missing_payment_id = true;
    m_ask_password = true;
    m_min_output_count = 0;
    m_min_output_value = 0;
    m_merge_destinations = false;
    m_confirm_backlog = true;
  }
  else
  {
    if (!json.HasMember("key_data"))
    {
      LOG_ERROR("Field key_data not found in JSON");
      return false;
    }
    if (!json["key_data"].IsString())
    {
      LOG_ERROR("Field key_data found in JSON, but not String");
      return false;
    }
    const char *field_key_data = json["key_data"].GetString();
    account_data = std::string(field_key_data, field_key_data + json["key_data"].GetStringLength());

    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, seed_language, std::string, String, false, std::string());
    if (field_seed_language_found)
    {
      set_seed_language(field_seed_language);
    }
    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, watch_only, int, Int, false, false);
    m_watch_only = field_watch_only;
    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, always_confirm_transfers, int, Int, false, true);
    m_always_confirm_transfers = field_always_confirm_transfers;
    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, print_ring_members, int, Int, false, true);
    m_print_ring_members = field_print_ring_members;
    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, store_tx_keys, int, Int, false, true);
    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, store_tx_info, int, Int, false, true);
    m_store_tx_info = ((field_store_tx_keys != 0) || (field_store_tx_info != 0));
    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, default_mixin, unsigned int, Uint, false, 0);
    m_default_mixin = field_default_mixin;
    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, default_priority, unsigned int, Uint, false, 0);
    if (field_default_priority_found)
    {
      m_default_priority = field_default_priority;
    }
    else
    {
      GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, default_fee_multiplier, unsigned int, Uint, false, 0);
      if (field_default_fee_multiplier_found)
        m_default_priority = field_default_fee_multiplier;
      else
        m_default_priority = 0;
    }
    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, auto_refresh, int, Int, false, true);
    m_auto_refresh = field_auto_refresh;
    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, refresh_type, int, Int, false, RefreshType::RefreshDefault);
    m_refresh_type = RefreshType::RefreshDefault;
    if (field_refresh_type_found)
    {
      if (field_refresh_type == RefreshFull || field_refresh_type == RefreshOptimizeCoinbase || field_refresh_type == RefreshNoCoinbase)
        m_refresh_type = (RefreshType)field_refresh_type;
      else
        LOG_PRINT_L0("Unknown refresh-type value (" << field_refresh_type << "), using default");
    }
    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, refresh_height, uint64_t, Uint64, false, 0);
    m_refresh_from_block_height = field_refresh_height;
    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, confirm_missing_payment_id, int, Int, false, true);
    m_confirm_missing_payment_id = field_confirm_missing_payment_id;
    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, ask_password, int, Int, false, true);
    m_ask_password = field_ask_password;
    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, default_decimal_point, int, Int, false, CRYPTONOTE_DISPLAY_DECIMAL_POINT);
    cryptonote::set_default_decimal_point(field_default_decimal_point);
    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, min_output_count, uint32_t, Uint, false, 0);
    m_min_output_count = field_min_output_count;
    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, min_output_value, uint64_t, Uint64, false, 0);
    m_min_output_value = field_min_output_value;
    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, merge_destinations, int, Int, false, false);
    m_merge_destinations = field_merge_destinations;
    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, confirm_backlog, int, Int, false, true);
    m_confirm_backlog = field_confirm_backlog;
    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, testnet, int, Int, false, m_testnet);
    // Wallet is being opened with testnet flag, but is saved as a mainnet wallet
    THROW_WALLET_EXCEPTION_IF(m_testnet && !field_testnet, error::wallet_internal_error, "Mainnet wallet can not be opened as testnet wallet");
    // Wallet is being opened without testnet flag but is saved as a testnet wallet.
    THROW_WALLET_EXCEPTION_IF(!m_testnet && field_testnet, error::wallet_internal_error, "Testnet wallet can not be opened as mainnet wallet");
  }

  const cryptonote::account_keys& keys = m_account.get_keys();
  r = epee::serialization::load_t_from_binary(m_account, account_data);
  r = r && verify_keys(keys.m_view_secret_key,  keys.m_account_address.m_view_public_key);
  if(!m_watch_only)
    r = r && verify_keys(keys.m_spend_secret_key, keys.m_account_address.m_spend_public_key);
  THROW_WALLET_EXCEPTION_IF(!r, error::invalid_password);
  return true;
}

/*!
 * \brief verify password for default wallet keys file.
 * \param password       Password to verify
 * \return               true if password is correct
 *
 * for verification only
 * should not mutate state, unlike load_keys()
 * can be used prior to rewriting wallet keys file, to ensure user has entered the correct password
 *
 */
bool wallet2::verify_password(const std::string& password) const
{
  return verify_password(m_keys_file, password, m_watch_only);
}

/*!
 * \brief verify password for specified wallet keys file.
 * \param keys_file_name  Keys file to verify password for
 * \param password        Password to verify
 * \param watch_only      If set = only verify view keys, otherwise also spend keys
 * \return                true if password is correct
 *
 * for verification only
 * should not mutate state, unlike load_keys()
 * can be used prior to rewriting wallet keys file, to ensure user has entered the correct password
 *
 */
bool wallet2::verify_password(const std::string& keys_file_name, const std::string& password, bool watch_only)
{
  wallet2::keys_file_data keys_file_data;
  std::string buf;
  bool r = epee::file_io_utils::load_file_to_string(keys_file_name, buf);
  THROW_WALLET_EXCEPTION_IF(!r, error::file_read_error, keys_file_name);

  // Decrypt the contents
  r = ::serialization::parse_binary(buf, keys_file_data);
  THROW_WALLET_EXCEPTION_IF(!r, error::wallet_internal_error, "internal error: failed to deserialize \"" + keys_file_name + '\"');
  crypto::chacha8_key key;
  crypto::generate_chacha8_key(password, key);
  std::string account_data;
  account_data.resize(keys_file_data.account_data.size());
  crypto::chacha8(keys_file_data.account_data.data(), keys_file_data.account_data.size(), key, keys_file_data.iv, &account_data[0]);

  // The contents should be JSON if the wallet follows the new format.
  rapidjson::Document json;
  if (json.Parse(account_data.c_str()).HasParseError())
  {
    // old format before JSON wallet key file format
  }
  else
  {
    account_data = std::string(json["key_data"].GetString(), json["key_data"].GetString() +
      json["key_data"].GetStringLength());
  }

  cryptonote::account_base account_data_check;

  r = epee::serialization::load_t_from_binary(account_data_check, account_data);
  const cryptonote::account_keys& keys = account_data_check.get_keys();

  r = r && verify_keys(keys.m_view_secret_key,  keys.m_account_address.m_view_public_key);
  if(!watch_only)
    r = r && verify_keys(keys.m_spend_secret_key, keys.m_account_address.m_spend_public_key);
  return r;
}

/*!
 * \brief  Generates a wallet or restores one.
 * \param  wallet_        Name of wallet file
 * \param  password       Password of wallet file
 * \param  recovery_param If it is a restore, the recovery key
 * \param  recover        Whether it is a restore
 * \param  two_random     Whether it is a non-deterministic wallet
 * \return                The secret key of the generated wallet
 */
crypto::secret_key wallet2::generate(const std::string& wallet_, const std::string& password,
  const crypto::secret_key& recovery_param, bool recover, bool two_random)
{
  clear();
  prepare_file_names(wallet_);

  boost::system::error_code ignored_ec;
  THROW_WALLET_EXCEPTION_IF(boost::filesystem::exists(m_wallet_file, ignored_ec), error::file_exists, m_wallet_file);
  THROW_WALLET_EXCEPTION_IF(boost::filesystem::exists(m_keys_file,   ignored_ec), error::file_exists, m_keys_file);

  crypto::secret_key retval = m_account.generate(recovery_param, recover, two_random);

  m_account_public_address = m_account.get_keys().m_account_address;
  m_watch_only = false;

  // -1 month for fluctuations in block time and machine date/time setup.
  // avg seconds per block
  const int seconds_per_block = DIFFICULTY_TARGET_V2;
  // ~num blocks per month
  const uint64_t blocks_per_month = 60*60*24*30/seconds_per_block;

  // try asking the daemon first
  if(m_refresh_from_block_height == 0 && !recover){
    std::string err;
    uint64_t height = 0;

    // we get the max of approximated height and known height
    // approximated height is the least of daemon target height
    // (the max of what the other daemons are claiming is their
    // height) and the theoretical height based on the local
    // clock. This will be wrong only if both the local clock
    // is bad *and* a peer daemon claims a highest height than
    // the real chain.
    // known height is the height the local daemon is currently
    // synced to, it will be lower than the real chain height if
    // the daemon is currently syncing.
    height = get_approximate_blockchain_height();
    uint64_t target_height = get_daemon_blockchain_target_height(err);
    if (err.empty() && target_height < height)
      height = target_height;
    uint64_t local_height = get_daemon_blockchain_height(err);
    if (err.empty() && local_height > height)
      height = local_height;
    m_refresh_from_block_height = height >= blocks_per_month ? height - blocks_per_month : 0;
  }

  if(m_refresh_from_block_height == 0 && !recover){
    // Wallets created offline don't know blockchain height.
    // Set blockchain height calculated from current date/time
    uint64_t approx_blockchain_height = get_approximate_blockchain_height();
    if(approx_blockchain_height > 0) {
      m_refresh_from_block_height = approx_blockchain_height >= blocks_per_month ? approx_blockchain_height - blocks_per_month : 0;
    }
  }
  bool r = store_keys(m_keys_file, password, false);
  THROW_WALLET_EXCEPTION_IF(!r, error::file_save_error, m_keys_file);

  r = file_io_utils::save_string_to_file(m_wallet_file + ".address.txt", m_account.get_public_address_str(m_testnet));
  if(!r) MERROR("String with address text not saved");

  cryptonote::block b;
  generate_genesis(b);
  m_blockchain.push_back(get_block_hash(b));

  store();
  return retval;
}

/*!
* \brief Creates a watch only wallet from a public address and a view secret key.
* \param  wallet_        Name of wallet file
* \param  password       Password of wallet file
* \param  viewkey        view secret key
*/
void wallet2::generate(const std::string& wallet_, const std::string& password,
  const cryptonote::account_public_address &account_public_address,
  const crypto::secret_key& viewkey)
{
  clear();
  prepare_file_names(wallet_);

  boost::system::error_code ignored_ec;
  THROW_WALLET_EXCEPTION_IF(boost::filesystem::exists(m_wallet_file, ignored_ec), error::file_exists, m_wallet_file);
  THROW_WALLET_EXCEPTION_IF(boost::filesystem::exists(m_keys_file,   ignored_ec), error::file_exists, m_keys_file);

  m_account.create_from_viewkey(account_public_address, viewkey);
  m_account_public_address = account_public_address;
  m_watch_only = true;

  bool r = store_keys(m_keys_file, password, true);
  THROW_WALLET_EXCEPTION_IF(!r, error::file_save_error, m_keys_file);

  r = file_io_utils::save_string_to_file(m_wallet_file + ".address.txt", m_account.get_public_address_str(m_testnet));
  if(!r) MERROR("String with address text not saved");

  cryptonote::block b;
  generate_genesis(b);
  m_blockchain.push_back(get_block_hash(b));

  store();
}

/*!
* \brief Creates a wallet from a public address and a spend/view secret key pair.
* \param  wallet_        Name of wallet file
* \param  password       Password of wallet file
* \param  spendkey       spend secret key
* \param  viewkey        view secret key
*/
void wallet2::generate(const std::string& wallet_, const std::string& password,
  const cryptonote::account_public_address &account_public_address,
  const crypto::secret_key& spendkey, const crypto::secret_key& viewkey)
{
  clear();
  prepare_file_names(wallet_);

  boost::system::error_code ignored_ec;
  THROW_WALLET_EXCEPTION_IF(boost::filesystem::exists(m_wallet_file, ignored_ec), error::file_exists, m_wallet_file);
  THROW_WALLET_EXCEPTION_IF(boost::filesystem::exists(m_keys_file,   ignored_ec), error::file_exists, m_keys_file);

  m_account.create_from_keys(account_public_address, spendkey, viewkey);
  m_account_public_address = account_public_address;
  m_watch_only = false;

  bool r = store_keys(m_keys_file, password, false);
  THROW_WALLET_EXCEPTION_IF(!r, error::file_save_error, m_keys_file);

  r = file_io_utils::save_string_to_file(m_wallet_file + ".address.txt", m_account.get_public_address_str(m_testnet));
  if(!r) MERROR("String with address text not saved");

  cryptonote::block b;
  generate_genesis(b);
  m_blockchain.push_back(get_block_hash(b));

  store();
}

/*!
 * \brief Rewrites to the wallet file for wallet upgrade (doesn't generate key, assumes it's already there)
 * \param wallet_name Name of wallet file (should exist)
 * \param password    Password for wallet file
 */
void wallet2::rewrite(const std::string& wallet_name, const std::string& password)
{
  prepare_file_names(wallet_name);
  boost::system::error_code ignored_ec;
  THROW_WALLET_EXCEPTION_IF(!boost::filesystem::exists(m_keys_file, ignored_ec), error::file_not_found, m_keys_file);
  bool r = store_keys(m_keys_file, password, m_watch_only);
  THROW_WALLET_EXCEPTION_IF(!r, error::file_save_error, m_keys_file);
}
/*!
 * \brief Writes to a file named based on the normal wallet (doesn't generate key, assumes it's already there)
 * \param wallet_name Base name of wallet file
 * \param password    Password for wallet file
 */
void wallet2::write_watch_only_wallet(const std::string& wallet_name, const std::string& password)
{
  prepare_file_names(wallet_name);
  boost::system::error_code ignored_ec;
  std::string filename = m_keys_file + "-watchonly";
  bool watch_only_keys_file_exists = boost::filesystem::exists(filename, ignored_ec);
  THROW_WALLET_EXCEPTION_IF(watch_only_keys_file_exists, error::file_save_error, filename);
  bool r = store_keys(filename, password, true);
  THROW_WALLET_EXCEPTION_IF(!r, error::file_save_error, filename);
}
//----------------------------------------------------------------------------------------------------
void wallet2::wallet_exists(const std::string& file_path, bool& keys_file_exists, bool& wallet_file_exists)
{
  std::string keys_file, wallet_file;
  do_prepare_file_names(file_path, keys_file, wallet_file);

  boost::system::error_code ignore;
  keys_file_exists = boost::filesystem::exists(keys_file, ignore);
  wallet_file_exists = boost::filesystem::exists(wallet_file, ignore);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::wallet_valid_path_format(const std::string& file_path)
{
  return !file_path.empty();
}
//----------------------------------------------------------------------------------------------------
bool wallet2::parse_long_payment_id(const std::string& payment_id_str, crypto::hash& payment_id)
{
  cryptonote::blobdata payment_id_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(payment_id_str, payment_id_data))
    return false;

  if(sizeof(crypto::hash) != payment_id_data.size())
    return false;

  payment_id = *reinterpret_cast<const crypto::hash*>(payment_id_data.data());
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::parse_short_payment_id(const std::string& payment_id_str, crypto::hash8& payment_id)
{
  cryptonote::blobdata payment_id_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(payment_id_str, payment_id_data))
    return false;

  if(sizeof(crypto::hash8) != payment_id_data.size())
    return false;

  payment_id = *reinterpret_cast<const crypto::hash8*>(payment_id_data.data());
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::parse_payment_id(const std::string& payment_id_str, crypto::hash& payment_id)
{
  if (parse_long_payment_id(payment_id_str, payment_id))
    return true;
  crypto::hash8 payment_id8;
  if (parse_short_payment_id(payment_id_str, payment_id8))
  {
    memcpy(payment_id.data, payment_id8.data, 8);
    memset(payment_id.data + 8, 0, 24);
    return true;
  }
  return false;
}
//----------------------------------------------------------------------------------------------------
void wallet2::set_default_decimal_point(unsigned int decimal_point)
{
  cryptonote::set_default_decimal_point(decimal_point);
}
//----------------------------------------------------------------------------------------------------
unsigned int wallet2::get_default_decimal_point() const
{
  return cryptonote::get_default_decimal_point();
}
//----------------------------------------------------------------------------------------------------
bool wallet2::prepare_file_names(const std::string& file_path)
{
  do_prepare_file_names(file_path, m_keys_file, m_wallet_file);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::check_connection(uint32_t *version, uint32_t timeout)
{
  THROW_WALLET_EXCEPTION_IF(!m_is_initialized, error::wallet_not_initialized);

  boost::lock_guard<boost::mutex> lock(m_daemon_rpc_mutex);

  if(!m_http_client.is_connected())
  {
    m_node_rpc_proxy.invalidate();
    if (!m_http_client.connect(std::chrono::milliseconds(timeout)))
      return false;
  }

  if (version)
  {
    epee::json_rpc::request<cryptonote::COMMAND_RPC_GET_VERSION::request> req_t = AUTO_VAL_INIT(req_t);
    epee::json_rpc::response<cryptonote::COMMAND_RPC_GET_VERSION::response, std::string> resp_t = AUTO_VAL_INIT(resp_t);
    req_t.jsonrpc = "2.0";
    req_t.id = epee::serialization::storage_entry(0);
    req_t.method = "get_version";
    bool r = net_utils::invoke_http_json("/json_rpc", req_t, resp_t, m_http_client);
    if(!r) {
      *version = 0;
      return false;
    }
    if (resp_t.result.status != CORE_RPC_STATUS_OK)
      *version = 0;
    else
      *version = resp_t.result.version;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::generate_chacha8_key_from_secret_keys(crypto::chacha8_key &key) const
{
  const account_keys &keys = m_account.get_keys();
  const crypto::secret_key &view_key = keys.m_view_secret_key;
  const crypto::secret_key &spend_key = keys.m_spend_secret_key;
  char data[sizeof(view_key) + sizeof(spend_key) + 1];
  memcpy(data, &view_key, sizeof(view_key));
  memcpy(data + sizeof(view_key), &spend_key, sizeof(spend_key));
  data[sizeof(data) - 1] = CHACHA8_KEY_TAIL;
  crypto::generate_chacha8_key(data, sizeof(data), key);
  memset(data, 0, sizeof(data));
  return true;
}
//----------------------------------------------------------------------------------------------------
void wallet2::load(const std::string& wallet_, const std::string& password)
{
  clear();
  prepare_file_names(wallet_);

  boost::system::error_code e;
  bool exists = boost::filesystem::exists(m_keys_file, e);
  THROW_WALLET_EXCEPTION_IF(e || !exists, error::file_not_found, m_keys_file);

  if (!load_keys(m_keys_file, password))
  {
    THROW_WALLET_EXCEPTION_IF(true, error::file_read_error, m_keys_file);
  }
  LOG_PRINT_L0("Loaded wallet keys file, with public address: " << m_account.get_public_address_str(m_testnet));

  //keys loaded ok!
  //try to load wallet file. but even if we failed, it is not big problem
  if(!boost::filesystem::exists(m_wallet_file, e) || e)
  {
    LOG_PRINT_L0("file not found: " << m_wallet_file << ", starting with empty blockchain");
    m_account_public_address = m_account.get_keys().m_account_address;
  }
  else
  {
    wallet2::cache_file_data cache_file_data;
    std::string buf;
    bool r = epee::file_io_utils::load_file_to_string(m_wallet_file, buf);
    THROW_WALLET_EXCEPTION_IF(!r, error::file_read_error, m_wallet_file);

    // try to read it as an encrypted cache
    try
    {
      LOG_PRINT_L1("Trying to decrypt cache data");

      r = ::serialization::parse_binary(buf, cache_file_data);
      THROW_WALLET_EXCEPTION_IF(!r, error::wallet_internal_error, "internal error: failed to deserialize \"" + m_wallet_file + '\"');
      crypto::chacha8_key key;
      generate_chacha8_key_from_secret_keys(key);
      std::string cache_data;
      cache_data.resize(cache_file_data.cache_data.size());
      crypto::chacha8(cache_file_data.cache_data.data(), cache_file_data.cache_data.size(), key, cache_file_data.iv, &cache_data[0]);

      std::stringstream iss;
      iss << cache_data;
      try {
        boost::archive::portable_binary_iarchive ar(iss);
        ar >> *this;
      }
      catch (...)
      {
        LOG_PRINT_L0("Failed to open portable binary, trying unportable");
        boost::filesystem::copy_file(m_wallet_file, m_wallet_file + ".unportable", boost::filesystem::copy_option::overwrite_if_exists);
        iss.str("");
        iss << cache_data;
        boost::archive::binary_iarchive ar(iss);
        ar >> *this;
      }
    }
    catch (...)
    {
      LOG_PRINT_L1("Failed to load encrypted cache, trying unencrypted");
      std::stringstream iss;
      iss << buf;
      try {
        boost::archive::portable_binary_iarchive ar(iss);
        ar >> *this;
      }
      catch (...)
      {
        LOG_PRINT_L0("Failed to open portable binary, trying unportable");
        boost::filesystem::copy_file(m_wallet_file, m_wallet_file + ".unportable", boost::filesystem::copy_option::overwrite_if_exists);
        iss.str("");
        iss << buf;
        boost::archive::binary_iarchive ar(iss);
        ar >> *this;
      }
    }
    THROW_WALLET_EXCEPTION_IF(
      m_account_public_address.m_spend_public_key != m_account.get_keys().m_account_address.m_spend_public_key ||
      m_account_public_address.m_view_public_key  != m_account.get_keys().m_account_address.m_view_public_key,
      error::wallet_files_doesnt_correspond, m_keys_file, m_wallet_file);
  }

  cryptonote::block genesis;
  generate_genesis(genesis);
  crypto::hash genesis_hash = get_block_hash(genesis);

  if (m_blockchain.empty())
  {
    m_blockchain.push_back(genesis_hash);
  }
  else
  {
    check_genesis(genesis_hash);
  }

  m_local_bc_height = m_blockchain.size();
}
//----------------------------------------------------------------------------------------------------
void wallet2::check_genesis(const crypto::hash& genesis_hash) const {
  std::string what("Genesis block mismatch. You probably use wallet without testnet flag with blockchain from test network or vice versa");

  THROW_WALLET_EXCEPTION_IF(genesis_hash != m_blockchain[0], error::wallet_internal_error, what);
}
//----------------------------------------------------------------------------------------------------
std::string wallet2::path() const
{
  return m_wallet_file;
}
//----------------------------------------------------------------------------------------------------
void wallet2::store()
{
  store_to("", "");
}
//----------------------------------------------------------------------------------------------------
void wallet2::store_to(const std::string &path, const std::string &password)
{
  // if file is the same, we do:
  // 1. save wallet to the *.new file
  // 2. remove old wallet file
  // 3. rename *.new to wallet_name

  // handle if we want just store wallet state to current files (ex store() replacement);
  bool same_file = true;
  if (!path.empty())
  {
    std::string canonical_path = boost::filesystem::canonical(m_wallet_file).string();
    size_t pos = canonical_path.find(path);
    same_file = pos != std::string::npos;
  }


  if (!same_file)
  {
    // check if we want to store to directory which doesn't exists yet
    boost::filesystem::path parent_path = boost::filesystem::path(path).parent_path();

    // if path is not exists, try to create it
    if (!parent_path.empty() &&  !boost::filesystem::exists(parent_path))
    {
      boost::system::error_code ec;
      if (!boost::filesystem::create_directories(parent_path, ec))
      {
        throw std::logic_error(ec.message());
      }
    }
  }
  // preparing wallet data
  std::stringstream oss;
  boost::archive::portable_binary_oarchive ar(oss);
  ar << *this;

  wallet2::cache_file_data cache_file_data = boost::value_initialized<wallet2::cache_file_data>();
  cache_file_data.cache_data = oss.str();
  crypto::chacha8_key key;
  generate_chacha8_key_from_secret_keys(key);
  std::string cipher;
  cipher.resize(cache_file_data.cache_data.size());
  cache_file_data.iv = crypto::rand<crypto::chacha8_iv>();
  crypto::chacha8(cache_file_data.cache_data.data(), cache_file_data.cache_data.size(), key, cache_file_data.iv, &cipher[0]);
  cache_file_data.cache_data = cipher;

  const std::string new_file = same_file ? m_wallet_file + ".new" : path;
  const std::string old_file = m_wallet_file;
  const std::string old_keys_file = m_keys_file;
  const std::string old_address_file = m_wallet_file + ".address.txt";

  // save to new file
  std::ofstream ostr;
  ostr.open(new_file, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
  binary_archive<true> oar(ostr);
  bool success = ::serialization::serialize(oar, cache_file_data);
  ostr.close();
  THROW_WALLET_EXCEPTION_IF(!success || !ostr.good(), error::file_save_error, new_file);

  // save keys to the new file
  // if we here, main wallet file is saved and we only need to save keys and address files
  if (!same_file) {
    prepare_file_names(path);
    store_keys(m_keys_file, password, false);
    // save address to the new file
    const std::string address_file = m_wallet_file + ".address.txt";
    bool r = file_io_utils::save_string_to_file(address_file, m_account.get_public_address_str(m_testnet));
    THROW_WALLET_EXCEPTION_IF(!r, error::file_save_error, m_wallet_file);
    // remove old wallet file
    r = boost::filesystem::remove(old_file);
    if (!r) {
      LOG_ERROR("error removing file: " << old_file);
    }
    // remove old keys file
    r = boost::filesystem::remove(old_keys_file);
    if (!r) {
      LOG_ERROR("error removing file: " << old_keys_file);
    }
    // remove old address file
    r = boost::filesystem::remove(old_address_file);
    if (!r) {
      LOG_ERROR("error removing file: " << old_address_file);
    }
  } else {
    // here we have "*.new" file, we need to rename it to be without ".new"
    std::error_code e = tools::replace_file(new_file, m_wallet_file);
    THROW_WALLET_EXCEPTION_IF(e, error::file_save_error, m_wallet_file, e);
  }
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::unlocked_balance() const
{
  uint64_t amount = 0;
  for(const transfer_details& td: m_transfers)
    if(!td.m_spent && is_transfer_unlocked(td))
      amount += td.amount();

  return amount;
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::balance() const
{
  uint64_t amount = 0;
  for(auto& td: m_transfers)
    if(!td.m_spent)
      amount += td.amount();


  for(auto& utx: m_unconfirmed_txs)
    if (utx.second.m_state != wallet2::unconfirmed_transfer_details::failed)
      amount+= utx.second.m_change;

  return amount;
}
//----------------------------------------------------------------------------------------------------
void wallet2::get_transfers(wallet2::transfer_container& incoming_transfers) const
{
  incoming_transfers = m_transfers;
}
//----------------------------------------------------------------------------------------------------
void wallet2::get_payments(const crypto::hash& payment_id, std::list<wallet2::payment_details>& payments, uint64_t min_height) const
{
  auto range = m_payments.equal_range(payment_id);
  std::for_each(range.first, range.second, [&payments, &min_height](const payment_container::value_type& x) {
    if (min_height < x.second.m_block_height)
    {
      payments.push_back(x.second);
    }
  });
}
//----------------------------------------------------------------------------------------------------
void wallet2::get_payments(std::list<std::pair<crypto::hash,wallet2::payment_details>>& payments, uint64_t min_height, uint64_t max_height) const
{
  auto range = std::make_pair(m_payments.begin(), m_payments.end());
  std::for_each(range.first, range.second, [&payments, &min_height, &max_height](const payment_container::value_type& x) {
    if (min_height < x.second.m_block_height && max_height >= x.second.m_block_height)
    {
      payments.push_back(x);
    }
  });
}
//----------------------------------------------------------------------------------------------------
void wallet2::get_payments_out(std::list<std::pair<crypto::hash,wallet2::confirmed_transfer_details>>& confirmed_payments,
    uint64_t min_height, uint64_t max_height) const
{
  for (auto i = m_confirmed_txs.begin(); i != m_confirmed_txs.end(); ++i) {
    if (i->second.m_block_height > min_height && i->second.m_block_height <= max_height) {
      confirmed_payments.push_back(*i);
    }
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::get_unconfirmed_payments_out(std::list<std::pair<crypto::hash,wallet2::unconfirmed_transfer_details>>& unconfirmed_payments) const
{
  for (auto i = m_unconfirmed_txs.begin(); i != m_unconfirmed_txs.end(); ++i) {
    unconfirmed_payments.push_back(*i);
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::get_unconfirmed_payments(std::list<std::pair<crypto::hash,wallet2::payment_details>>& unconfirmed_payments) const
{
  for (auto i = m_unconfirmed_payments.begin(); i != m_unconfirmed_payments.end(); ++i) {
    unconfirmed_payments.push_back(*i);
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::rescan_spent()
{
  // This is RPC call that can take a long time if there are many outputs,
  // so we call it several times, in stripes, so we don't time out spuriously
  std::vector<int> spent_status;
  spent_status.reserve(m_transfers.size());
  const size_t chunk_size = 1000;
  for (size_t start_offset = 0; start_offset < m_transfers.size(); start_offset += chunk_size)
  {
    const size_t n_outputs = std::min<size_t>(chunk_size, m_transfers.size() - start_offset);
    MDEBUG("Calling is_key_image_spent on " << start_offset << " - " << (start_offset + n_outputs - 1) << ", out of " << m_transfers.size());
    COMMAND_RPC_IS_KEY_IMAGE_SPENT::request req = AUTO_VAL_INIT(req);
    COMMAND_RPC_IS_KEY_IMAGE_SPENT::response daemon_resp = AUTO_VAL_INIT(daemon_resp);
    for (size_t n = start_offset; n < start_offset + n_outputs; ++n)
      req.key_images.push_back(string_tools::pod_to_hex(m_transfers[n].m_key_image));
    m_daemon_rpc_mutex.lock();
    bool r = epee::net_utils::invoke_http_json("/is_key_image_spent", req, daemon_resp, m_http_client, rpc_timeout);
    m_daemon_rpc_mutex.unlock();
    THROW_WALLET_EXCEPTION_IF(!r, error::no_connection_to_daemon, "is_key_image_spent");
    THROW_WALLET_EXCEPTION_IF(daemon_resp.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "is_key_image_spent");
    THROW_WALLET_EXCEPTION_IF(daemon_resp.status != CORE_RPC_STATUS_OK, error::is_key_image_spent_error, daemon_resp.status);
    THROW_WALLET_EXCEPTION_IF(daemon_resp.spent_status.size() != n_outputs, error::wallet_internal_error,
      "daemon returned wrong response for is_key_image_spent, wrong amounts count = " +
      std::to_string(daemon_resp.spent_status.size()) + ", expected " +  std::to_string(n_outputs));
    std::copy(daemon_resp.spent_status.begin(), daemon_resp.spent_status.end(), std::back_inserter(spent_status));
  }

  // update spent status
  for (size_t i = 0; i < m_transfers.size(); ++i)
  {
    transfer_details& td = m_transfers[i];
    // a view wallet may not know about key images
    if (!td.m_key_image_known)
      continue;
    if (td.m_spent != (spent_status[i] != COMMAND_RPC_IS_KEY_IMAGE_SPENT::UNSPENT))
    {
      if (td.m_spent)
      {
        LOG_PRINT_L0("Marking output " << i << "(" << td.m_key_image << ") as unspent, it was marked as spent");
        set_unspent(i);
        td.m_spent_height = 0;
      }
      else
      {
        LOG_PRINT_L0("Marking output " << i << "(" << td.m_key_image << ") as spent, it was marked as unspent");
        set_spent(i, td.m_spent_height);
        // unknown height, if this gets reorged, it might still be missed
      }
    }
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::rescan_blockchain(bool refresh)
{
  clear();

  cryptonote::block genesis;
  generate_genesis(genesis);
  crypto::hash genesis_hash = get_block_hash(genesis);
  m_blockchain.push_back(genesis_hash);
  m_local_bc_height = 1;

  if (refresh)
    this->refresh();
}
//----------------------------------------------------------------------------------------------------
bool wallet2::is_transfer_unlocked(const transfer_details& td) const
{
  if(!is_tx_spendtime_unlocked(td.m_tx.unlock_time, td.m_block_height))
    return false;

  if(td.m_block_height + CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE > m_blockchain.size())
    return false;

  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::is_tx_spendtime_unlocked(uint64_t unlock_time, uint64_t block_height) const
{
  if(unlock_time < CRYPTONOTE_MAX_BLOCK_NUMBER)
  {
    //interpret as block index
    if(m_blockchain.size()-1 + CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS >= unlock_time)
      return true;
    else
      return false;
  }else
  {
    //interpret as time
    uint64_t current_time = static_cast<uint64_t>(time(NULL));
    // XXX: this needs to be fast, so we'd need to get the starting heights
    // from the daemon to be correct once voting kicks in
    uint64_t v2height = m_testnet ? 624634 : 1009827;
    uint64_t leeway = block_height < v2height ? CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS_V1 : CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS_V2;
    if(current_time + leeway >= unlock_time)
      return true;
    else
      return false;
  }
  return false;
}
//----------------------------------------------------------------------------------------------------
namespace
{
  template<typename T>
  T pop_index(std::vector<T>& vec, size_t idx)
  {
    CHECK_AND_ASSERT_MES(!vec.empty(), T(), "Vector must be non-empty");
    CHECK_AND_ASSERT_MES(idx < vec.size(), T(), "idx out of bounds");

    T res = vec[idx];
    if (idx + 1 != vec.size())
    {
      vec[idx] = vec.back();
    }
    vec.resize(vec.size() - 1);

    return res;
  }

  template<typename T>
  T pop_random_value(std::vector<T>& vec)
  {
    CHECK_AND_ASSERT_MES(!vec.empty(), T(), "Vector must be non-empty");

    size_t idx = crypto::rand<size_t>() % vec.size();
    return pop_index (vec, idx);
  }

  template<typename T>
  T pop_back(std::vector<T>& vec)
  {
    CHECK_AND_ASSERT_MES(!vec.empty(), T(), "Vector must be non-empty");

    T res = vec.back();
    vec.pop_back();
    return res;
  }

  template<typename T>
  void pop_if_present(std::vector<T>& vec, T e)
  {
    for (size_t i = 0; i < vec.size(); ++i)
    {
      if (e == vec[i])
      {
        pop_index (vec, i);
        return;
      }
    }
  }
}
//----------------------------------------------------------------------------------------------------
// This returns a handwavy estimation of how much two outputs are related
// If they're from the same tx, then they're fully related. From close block
// heights, they're kinda related. The actual values don't matter, just
// their ordering, but it could become more murky if we add scores later.
float wallet2::get_output_relatedness(const transfer_details &td0, const transfer_details &td1) const
{
  int dh;

  // expensive test, and same tx will fall onto the same block height below
  if (td0.m_txid == td1.m_txid)
    return 1.0f;

  // same block height -> possibly tx burst, or same tx (since above is disabled)
  dh = td0.m_block_height > td1.m_block_height ? td0.m_block_height - td1.m_block_height : td1.m_block_height - td0.m_block_height;
  if (dh == 0)
    return 0.9f;

  // adjacent blocks -> possibly tx burst
  if (dh == 1)
    return 0.8f;

  // could extract the payment id, and compare them, but this is a bit expensive too

  // similar block heights
  if (dh < 10)
    return 0.2f;

  // don't think these are particularly related
  return 0.0f;
}
//----------------------------------------------------------------------------------------------------
size_t wallet2::pop_best_value_from(const transfer_container &transfers, std::vector<size_t> &unused_indices, const std::list<size_t>& selected_transfers, bool smallest) const
{
  std::vector<size_t> candidates;
  float best_relatedness = 1.0f;
  for (size_t n = 0; n < unused_indices.size(); ++n)
  {
    const transfer_details &candidate = transfers[unused_indices[n]];
    float relatedness = 0.0f;
    for (std::list<size_t>::const_iterator i = selected_transfers.begin(); i != selected_transfers.end(); ++i)
    {
      float r = get_output_relatedness(candidate, transfers[*i]);
      if (r > relatedness)
      {
        relatedness = r;
        if (relatedness == 1.0f)
          break;
      }
    }

    if (relatedness < best_relatedness)
    {
      best_relatedness = relatedness;
      candidates.clear();
    }

    if (relatedness == best_relatedness)
      candidates.push_back(n);
  }

  // we have all the least related outputs in candidates, so we can pick either
  // the smallest, or a random one, depending on request
  size_t idx;
  if (smallest)
  {
    idx = 0;
    for (size_t n = 0; n < candidates.size(); ++n)
    {
      const transfer_details &td = transfers[unused_indices[candidates[n]]];
      if (td.amount() < transfers[unused_indices[candidates[idx]]].amount())
        idx = n;
    }
  }
  else
  {
    idx = crypto::rand<size_t>() % candidates.size();
  }
  return pop_index (unused_indices, candidates[idx]);
}
//----------------------------------------------------------------------------------------------------
size_t wallet2::pop_best_value(std::vector<size_t> &unused_indices, const std::list<size_t>& selected_transfers, bool smallest) const
{
  return pop_best_value_from(m_transfers, unused_indices, selected_transfers, smallest);
}
//----------------------------------------------------------------------------------------------------
// Select random input sources for transaction.
// returns:
//    direct return: amount of money found
//    modified reference: selected_transfers, a list of iterators/indices of input sources
uint64_t wallet2::select_transfers(uint64_t needed_money, std::vector<size_t> unused_transfers_indices, std::list<size_t>& selected_transfers, bool trusted_daemon)
{
  uint64_t found_money = 0;
  while (found_money < needed_money && !unused_transfers_indices.empty())
  {
    size_t idx = pop_best_value(unused_transfers_indices, selected_transfers);

    transfer_container::iterator it = m_transfers.begin() + idx;
    selected_transfers.push_back(idx);
    found_money += it->amount();
  }

  return found_money;
}
//----------------------------------------------------------------------------------------------------
void wallet2::add_unconfirmed_tx(const cryptonote::transaction& tx, uint64_t amount_in, const std::vector<cryptonote::tx_destination_entry> &dests, const crypto::hash &payment_id, uint64_t change_amount)
{
  unconfirmed_transfer_details& utd = m_unconfirmed_txs[cryptonote::get_transaction_hash(tx)];
  utd.m_amount_in = amount_in;
  utd.m_amount_out = 0;
  for (const auto &d: dests)
    utd.m_amount_out += d.amount;
  utd.m_amount_out += change_amount; // dests does not contain change
  utd.m_change = change_amount;
  utd.m_sent_time = time(NULL);
  utd.m_tx = (const cryptonote::transaction_prefix&)tx;
  utd.m_dests = dests;
  utd.m_payment_id = payment_id;
  utd.m_state = wallet2::unconfirmed_transfer_details::pending;
  utd.m_timestamp = time(NULL);
}

//----------------------------------------------------------------------------------------------------
void wallet2::transfer(const std::vector<cryptonote::tx_destination_entry>& dsts, const size_t fake_outs_count, const std::vector<size_t> &unused_transfers_indices,
                       uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra, cryptonote::transaction& tx, pending_tx& ptx, bool trusted_daemon)
{
  transfer(dsts, fake_outs_count, unused_transfers_indices, unlock_time, fee, extra, detail::digit_split_strategy, tx_dust_policy(::config::DEFAULT_DUST_THRESHOLD), tx, ptx, trusted_daemon);
}
//----------------------------------------------------------------------------------------------------
void wallet2::transfer(const std::vector<cryptonote::tx_destination_entry>& dsts, const size_t fake_outs_count, const std::vector<size_t> &unused_transfers_indices,
                       uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra, bool trusted_daemon)
{
  cryptonote::transaction tx;
  pending_tx ptx;
  transfer(dsts, fake_outs_count, unused_transfers_indices, unlock_time, fee, extra, tx, ptx, trusted_daemon);
}

namespace {
// split_amounts(vector<cryptonote::tx_destination_entry> dsts, size_t num_splits)
//
// split amount for each dst in dsts into num_splits parts
// and make num_splits new vector<crypt...> instances to hold these new amounts
std::vector<std::vector<cryptonote::tx_destination_entry>> split_amounts(
    std::vector<cryptonote::tx_destination_entry> dsts, size_t num_splits)
{
  std::vector<std::vector<cryptonote::tx_destination_entry>> retVal;

  if (num_splits <= 1)
  {
    retVal.push_back(dsts);
    return retVal;
  }

  // for each split required
  for (size_t i=0; i < num_splits; i++)
  {
    std::vector<cryptonote::tx_destination_entry> new_dsts;

    // for each destination
    for (size_t j=0; j < dsts.size(); j++)
    {
      cryptonote::tx_destination_entry de;
      uint64_t amount;

      amount = dsts[j].amount;
      amount = amount / num_splits;

      // if last split, add remainder
      if (i + 1 == num_splits)
      {
        amount += dsts[j].amount % num_splits;
      }
      
      de.addr = dsts[j].addr;
      de.amount = amount;

      new_dsts.push_back(de);
    }

    retVal.push_back(new_dsts);
  }

  return retVal;
}
} // anonymous namespace
//----------------------------------------------------------------------------------------------------
crypto::hash wallet2::get_payment_id(const pending_tx &ptx) const
{
  std::vector<tx_extra_field> tx_extra_fields;
  if(!parse_tx_extra(ptx.tx.extra, tx_extra_fields))
    return cryptonote::null_hash;
  tx_extra_nonce extra_nonce;
  crypto::hash payment_id = null_hash;
  if (find_tx_extra_field_by_type(tx_extra_fields, extra_nonce))
  {
    crypto::hash8 payment_id8 = null_hash8;
    if(get_encrypted_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id8))
    {
      if (decrypt_payment_id(payment_id8, ptx.dests[0].addr.m_view_public_key, ptx.tx_key))
      {
        memcpy(payment_id.data, payment_id8.data, 8);
      }
    }
    else if (!get_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id))
    {
      payment_id = cryptonote::null_hash;
    }
  }
  return payment_id;
}

crypto::hash8 wallet2::get_short_payment_id(const pending_tx &ptx) const
{
  crypto::hash8 payment_id8 = null_hash8;
  std::vector<tx_extra_field> tx_extra_fields;
  if(!parse_tx_extra(ptx.tx.extra, tx_extra_fields))
    return payment_id8;
  cryptonote::tx_extra_nonce extra_nonce;
  if (find_tx_extra_field_by_type(tx_extra_fields, extra_nonce))
  {
    if(get_encrypted_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id8))
    {
      decrypt_payment_id(payment_id8, ptx.dests[0].addr.m_view_public_key, ptx.tx_key); 
    }
  }
  return payment_id8;
}

//----------------------------------------------------------------------------------------------------
// take a pending tx and actually send it to the daemon
void wallet2::commit_tx(pending_tx& ptx)
{
  using namespace cryptonote;
  crypto::hash txid;

  COMMAND_RPC_SEND_RAW_TX::request req;
  req.tx_as_hex = epee::string_tools::buff_to_hex_nodelimer(tx_to_blob(ptx.tx));
  req.do_not_relay = false;
  COMMAND_RPC_SEND_RAW_TX::response daemon_send_resp;
  m_daemon_rpc_mutex.lock();
  bool r = epee::net_utils::invoke_http_json("/sendrawtransaction", req, daemon_send_resp, m_http_client, rpc_timeout);
  m_daemon_rpc_mutex.unlock();
  THROW_WALLET_EXCEPTION_IF(!r, error::no_connection_to_daemon, "sendrawtransaction");
  THROW_WALLET_EXCEPTION_IF(daemon_send_resp.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "sendrawtransaction");
  THROW_WALLET_EXCEPTION_IF(daemon_send_resp.status != CORE_RPC_STATUS_OK, error::tx_rejected, ptx.tx, daemon_send_resp.status, daemon_send_resp.reason);

  // sanity checks
  for (size_t idx: ptx.selected_transfers)
  {
    THROW_WALLET_EXCEPTION_IF(idx >= m_transfers.size(), error::wallet_internal_error,
        "Bad output index in selected transfers: " + boost::lexical_cast<std::string>(idx));
  }

  txid = get_transaction_hash(ptx.tx);
  crypto::hash payment_id = cryptonote::null_hash;
  std::vector<cryptonote::tx_destination_entry> dests;
  uint64_t amount_in = 0;
  if (store_tx_info())
  {
    payment_id = get_payment_id(ptx);
    dests = ptx.dests;
    for(size_t idx: ptx.selected_transfers)
      amount_in += m_transfers[idx].amount();
  }
  add_unconfirmed_tx(ptx.tx, amount_in, dests, payment_id, ptx.change_dts.amount);
  if (store_tx_info())
  {
    m_tx_keys.insert(std::make_pair(txid, ptx.tx_key));
  }

  LOG_PRINT_L2("transaction " << txid << " generated ok and sent to daemon, key_images: [" << ptx.key_images << "]");

  for(size_t idx: ptx.selected_transfers)
  {
    set_spent(idx, 0);
  }

  //fee includes dust if dust policy specified it.
  LOG_PRINT_L1("Transaction successfully sent. <" << txid << ">" << ENDL
            << "Commission: " << print_money(ptx.fee) << " (dust sent to dust addr: " << print_money((ptx.dust_added_to_fee ? 0 : ptx.dust)) << ")" << ENDL
            << "Balance: " << print_money(balance()) << ENDL
            << "Unlocked: " << print_money(unlocked_balance()) << ENDL
            << "Please, wait for confirmation for your balance to be unlocked.");
}

void wallet2::commit_tx(std::vector<pending_tx>& ptx_vector)
{
  for (auto & ptx : ptx_vector)
  {
    commit_tx(ptx);
  }
}
//----------------------------------------------------------------------------------------------------
bool wallet2::save_tx(const std::vector<pending_tx>& ptx_vector, const std::string &filename)
{
  LOG_PRINT_L0("saving " << ptx_vector.size() << " transactions");
  unsigned_tx_set txs;
  for (auto &tx: ptx_vector)
  {
    tx_construction_data construction_data = tx.construction_data;
    // Short payment id is encrypted with tx_key. 
    // Since sign_tx() generates new tx_keys and encrypts the payment id, we need to save the decrypted payment ID
    // Get decrypted payment id from pending_tx
    crypto::hash8 payment_id = get_short_payment_id(tx);
    if (payment_id != null_hash8)
    {
      // Remove encrypted
      remove_field_from_tx_extra(construction_data.extra, typeid(cryptonote::tx_extra_nonce));
      // Add decrypted
      std::string extra_nonce;
      set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, payment_id);
      if (!add_extra_nonce_to_tx_extra(construction_data.extra, extra_nonce))
      {
        LOG_ERROR("Failed to add decrypted payment id to tx extra");
        return false;
      }
      LOG_PRINT_L1("Decrypted payment ID: " << payment_id);       
    }
    // Save tx construction_data to unsigned_tx_set
    txs.txes.push_back(construction_data);      
  }
  
  txs.transfers = m_transfers;
  // save as binary
  std::ostringstream oss;
  boost::archive::portable_binary_oarchive ar(oss);
  try
  {
    ar << txs;
  }
  catch (...)
  {
    return false;
  }
  LOG_PRINT_L2("Saving unsigned tx data: " << oss.str());
  return epee::file_io_utils::save_string_to_file(filename, std::string(UNSIGNED_TX_PREFIX) + oss.str());  
}
//----------------------------------------------------------------------------------------------------
bool wallet2::load_unsigned_tx(const std::string &unsigned_filename, unsigned_tx_set &exported_txs)
{
  std::string s;
  boost::system::error_code errcode;

  if (!boost::filesystem::exists(unsigned_filename, errcode))
  {
    LOG_PRINT_L0("File " << unsigned_filename << " does not exist: " << errcode);
    return false;
  }
  if (!epee::file_io_utils::load_file_to_string(unsigned_filename.c_str(), s))
  {
    LOG_PRINT_L0("Failed to load from " << unsigned_filename);
    return false;
  }
  const size_t magiclen = strlen(UNSIGNED_TX_PREFIX);
  if (strncmp(s.c_str(), UNSIGNED_TX_PREFIX, magiclen))
  {
    LOG_PRINT_L0("Bad magic from " << unsigned_filename);
    return false;
  }
  s = s.substr(magiclen);
  try
  {
    std::istringstream iss(s);
    boost::archive::portable_binary_iarchive ar(iss);
    ar >> exported_txs;
  }
  catch (...)  
  {
    LOG_PRINT_L0("Failed to parse data from " << unsigned_filename);
    return false;
  }
  LOG_PRINT_L1("Loaded tx unsigned data from binary: " << exported_txs.txes.size() << " transactions");

  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::sign_tx(const std::string &unsigned_filename, const std::string &signed_filename, std::vector<wallet2::pending_tx> &txs, std::function<bool(const unsigned_tx_set&)> accept_func)
{
  unsigned_tx_set exported_txs;
  if(!load_unsigned_tx(unsigned_filename, exported_txs))
    return false;
  
  if (accept_func && !accept_func(exported_txs))
  {
    LOG_PRINT_L1("Transactions rejected by callback");
    return false;
  }
  return sign_tx(exported_txs, signed_filename, txs);
}

//----------------------------------------------------------------------------------------------------
bool wallet2::sign_tx(unsigned_tx_set &exported_txs, const std::string &signed_filename, std::vector<wallet2::pending_tx> &txs)
{
  import_outputs(exported_txs.transfers);

  // sign the transactions
  signed_tx_set signed_txes;
  for (size_t n = 0; n < exported_txs.txes.size(); ++n)
  {
    const tools::wallet2::tx_construction_data &sd = exported_txs.txes[n];
    LOG_PRINT_L1(" " << (n+1) << ": " << sd.sources.size() << " inputs, ring size " << sd.sources[0].outputs.size());
    signed_txes.ptx.push_back(pending_tx());
    tools::wallet2::pending_tx &ptx = signed_txes.ptx.back();
    crypto::secret_key tx_key;
    bool r = cryptonote::construct_tx_and_get_tx_key(m_account.get_keys(), sd.sources, sd.splitted_dsts, sd.extra, ptx.tx, sd.unlock_time, tx_key, sd.use_rct);
    THROW_WALLET_EXCEPTION_IF(!r, error::tx_not_constructed, sd.sources, sd.splitted_dsts, sd.unlock_time, m_testnet);
    // we don't test tx size, because we don't know the current limit, due to not having a blockchain,
    // and it's a bit pointless to fail there anyway, since it'd be a (good) guess only. We sign anyway,
    // and if we really go over limit, the daemon will reject when it gets submitted. Chances are it's
    // OK anyway since it was generated in the first place, and rerolling should be within a few bytes.

    // normally, the tx keys are saved in commit_tx, when the tx is actually sent to the daemon.
    // we can't do that here since the tx will be sent from the compromised wallet, which we don't want
    // to see that info, so we save it here
    if (store_tx_info())
    {
      const crypto::hash txid = get_transaction_hash(ptx.tx);
      m_tx_keys.insert(std::make_pair(txid, tx_key));
    }

    std::string key_images;
    bool all_are_txin_to_key = std::all_of(ptx.tx.vin.begin(), ptx.tx.vin.end(), [&](const txin_v& s_e) -> bool
    {
      CHECKED_GET_SPECIFIC_VARIANT(s_e, const txin_to_key, in, false);
      key_images += boost::to_string(in.k_image) + " ";
      return true;
    });
    THROW_WALLET_EXCEPTION_IF(!all_are_txin_to_key, error::unexpected_txin_type, ptx.tx);

    ptx.key_images = key_images;
    ptx.fee = 0;
    for (const auto &i: sd.sources) ptx.fee += i.amount;
    for (const auto &i: sd.splitted_dsts) ptx.fee -= i.amount;
    ptx.dust = 0;
    ptx.dust_added_to_fee = false;
    ptx.change_dts = sd.change_dts;
    ptx.selected_transfers = sd.selected_transfers;
    ptx.tx_key = rct::rct2sk(rct::identity()); // don't send it back to the untrusted view wallet
    ptx.dests = sd.dests;
    ptx.construction_data = sd;

    txs.push_back(ptx);
  }

  // add key images
  signed_txes.key_images.resize(m_transfers.size());
  for (size_t i = 0; i < m_transfers.size(); ++i)
  {
    if (!m_transfers[i].m_key_image_known)
      LOG_PRINT_L0("WARNING: key image not known in signing wallet at index " << i);
    signed_txes.key_images[i] = m_transfers[i].m_key_image;
  }

  // save as binary
  std::ostringstream oss;
  boost::archive::portable_binary_oarchive ar(oss);
  try
  {
    ar << signed_txes;
  }
  catch(...)
  {
    return false;
  }
  LOG_PRINT_L3("Saving signed tx data: " << oss.str());
  return epee::file_io_utils::save_string_to_file(signed_filename, std::string(SIGNED_TX_PREFIX) + oss.str());  
}
//----------------------------------------------------------------------------------------------------
bool wallet2::load_tx(const std::string &signed_filename, std::vector<tools::wallet2::pending_tx> &ptx, std::function<bool(const signed_tx_set&)> accept_func)
{
  std::string s;
  boost::system::error_code errcode;
  signed_tx_set signed_txs;

  if (!boost::filesystem::exists(signed_filename, errcode))
  {
    LOG_PRINT_L0("File " << signed_filename << " does not exist: " << errcode);
    return false;
  }

  if (!epee::file_io_utils::load_file_to_string(signed_filename.c_str(), s))
  {
    LOG_PRINT_L0("Failed to load from " << signed_filename);
    return false;
  }
  const size_t magiclen = strlen(SIGNED_TX_PREFIX);
  if (strncmp(s.c_str(), SIGNED_TX_PREFIX, magiclen))
  {
    LOG_PRINT_L0("Bad magic from " << signed_filename);
    return false;
  }
  s = s.substr(magiclen);
  try
  {
    std::istringstream iss(s);
    boost::archive::portable_binary_iarchive ar(iss);
    ar >> signed_txs;
  }
  catch (...)
  {
    LOG_PRINT_L0("Failed to parse data from " << signed_filename);
    return false;
  }
  LOG_PRINT_L0("Loaded signed tx data from binary: " << signed_txs.ptx.size() << " transactions");
  for (auto &ptx: signed_txs.ptx) LOG_PRINT_L0(cryptonote::obj_to_json_str(ptx.tx));

  if (accept_func && !accept_func(signed_txs))
  {
    LOG_PRINT_L1("Transactions rejected by callback");
    return false;
  }

  // import key images
  if (signed_txs.key_images.size() > m_transfers.size())
  {
    LOG_PRINT_L1("More key images returned that we know outputs for");
    return false;
  }
  for (size_t i = 0; i < signed_txs.key_images.size(); ++i)
  {
    transfer_details &td = m_transfers[i];
    if (td.m_key_image_known && td.m_key_image != signed_txs.key_images[i])
      LOG_PRINT_L0("WARNING: imported key image differs from previously known key image at index " << i << ": trusting imported one");
    td.m_key_image = signed_txs.key_images[i];
    m_key_images[m_transfers[i].m_key_image] = i;
    td.m_key_image_known = true;
    m_pub_keys[m_transfers[i].get_public_key()] = i;
  }

  ptx = signed_txs.ptx;

  return true;
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::get_fee_multiplier(uint32_t priority, int fee_algorithm)
{
  static const uint64_t old_multipliers[3] = {1, 2, 3};
  static const uint64_t new_multipliers[3] = {1, 20, 166};
  static const uint64_t newer_multipliers[4] = {1, 4, 20, 166};

  if (fee_algorithm == -1)
    fee_algorithm = get_fee_algorithm();

  // 0 -> default (here, x1 till fee algorithm 2, x4 from it)
  if (priority == 0)
    priority = m_default_priority;
  if (priority == 0)
  {
    if (fee_algorithm >= 2)
      priority = 2;
    else
      priority = 1;
  }

  // 1 to 3/4 are allowed as priorities
  uint32_t max_priority = (fee_algorithm >= 2) ? 4 : 3;
  if (priority >= 1 && priority <= max_priority)
  {
    switch (fee_algorithm)
    {
      case 0: return old_multipliers[priority-1];
      case 1: return new_multipliers[priority-1];
      case 2: return newer_multipliers[priority-1];
      default: THROW_WALLET_EXCEPTION_IF (true, error::invalid_priority);
    }
  }

  THROW_WALLET_EXCEPTION_IF (false, error::invalid_priority);
  return 1;
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::get_dynamic_per_kb_fee_estimate()
{
  uint64_t fee;
  boost::optional<std::string> result = m_node_rpc_proxy.get_dynamic_per_kb_fee_estimate(FEE_ESTIMATE_GRACE_BLOCKS, fee);
  if (!result)
    return fee;
  LOG_PRINT_L1("Failed to query per kB fee, using " << print_money(FEE_PER_KB));
  return FEE_PER_KB;
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::get_per_kb_fee()
{
  bool use_dyn_fee = use_fork_rules(HF_VERSION_DYNAMIC_FEE, -720 * 1);
  if (!use_dyn_fee)
    return FEE_PER_KB;

  return get_dynamic_per_kb_fee_estimate();
}
//----------------------------------------------------------------------------------------------------
int wallet2::get_fee_algorithm()
{
  // changes at v3 and v5
  if (use_fork_rules(5, 0))
    return 2;
  if (use_fork_rules(3, -720 * 14))
   return 1;
  return 0;
}
//----------------------------------------------------------------------------------------------------
// separated the call(s) to wallet2::transfer into their own function
//
// this function will make multiple calls to wallet2::transfer if multiple
// transactions will be required
std::vector<wallet2::pending_tx> wallet2::create_transactions(std::vector<cryptonote::tx_destination_entry> dsts, const size_t fake_outs_count, const uint64_t unlock_time, uint32_t priority, const std::vector<uint8_t> extra, bool trusted_daemon)
{
  const std::vector<size_t> unused_transfers_indices = select_available_outputs_from_histogram(fake_outs_count + 1, true, true, true, trusted_daemon);

  const uint64_t fee_per_kb  = get_per_kb_fee();
  const uint64_t fee_multiplier = get_fee_multiplier(priority, get_fee_algorithm());

  // failsafe split attempt counter
  size_t attempt_count = 0;

  for(attempt_count = 1; ;attempt_count++)
  {
    size_t num_tx = 0.5 + pow(1.7,attempt_count-1);

    auto split_values = split_amounts(dsts, num_tx);

    // Throw if split_amounts comes back with a vector of size different than it should
    if (split_values.size() != num_tx)
    {
      throw std::runtime_error("Splitting transactions returned a number of potential tx not equal to what was requested");
    }

    std::vector<pending_tx> ptx_vector;
    try
    {
      // for each new destination vector (i.e. for each new tx)
      for (auto & dst_vector : split_values)
      {
        cryptonote::transaction tx;
        pending_tx ptx;

	// loop until fee is met without increasing tx size to next KB boundary.
	uint64_t needed_fee = 0;
	do
	{
	  transfer(dst_vector, fake_outs_count, unused_transfers_indices, unlock_time, needed_fee, extra, tx, ptx, trusted_daemon);
	  auto txBlob = t_serializable_object_to_blob(ptx.tx);
          needed_fee = calculate_fee(fee_per_kb, txBlob, fee_multiplier);
	} while (ptx.fee < needed_fee);

        ptx_vector.push_back(ptx);

        // mark transfers to be used as "spent"
        for(size_t idx: ptx.selected_transfers)
        {
          set_spent(idx, 0);
        }
      }

      // if we made it this far, we've selected our transactions.  committing them will mark them spent,
      // so this is a failsafe in case they don't go through
      // unmark pending tx transfers as spent
      for (auto & ptx : ptx_vector)
      {
        // mark transfers to be used as not spent
        for(size_t idx2: ptx.selected_transfers)
        {
          set_unspent(idx2);
        }

      }

      // if we made it this far, we're OK to actually send the transactions
      return ptx_vector;

    }
    // only catch this here, other exceptions need to pass through to the calling function
    catch (const tools::error::tx_too_big& e)
    {

      // unmark pending tx transfers as spent
      for (auto & ptx : ptx_vector)
      {
        // mark transfers to be used as not spent
        for(size_t idx2: ptx.selected_transfers)
        {
          set_unspent(idx2);
        }
      }

      if (attempt_count >= MAX_SPLIT_ATTEMPTS)
      {
        throw;
      }
    }
    catch (...)
    {
      // in case of some other exception, make sure any tx in queue are marked unspent again

      // unmark pending tx transfers as spent
      for (auto & ptx : ptx_vector)
      {
        // mark transfers to be used as not spent
        for(size_t idx2: ptx.selected_transfers)
        {
          set_unspent(idx2);
        }
      }

      throw;
    }
  }
}

void wallet2::get_outs(std::vector<std::vector<tools::wallet2::get_outs_entry>> &outs, const std::list<size_t> &selected_transfers, size_t fake_outputs_count)
{
  LOG_PRINT_L2("fake_outputs_count: " << fake_outputs_count);
  outs.clear();
  if (fake_outputs_count > 0)
  {
    // get histogram for the amounts we need
    epee::json_rpc::request<cryptonote::COMMAND_RPC_GET_OUTPUT_HISTOGRAM::request> req_t = AUTO_VAL_INIT(req_t);
    epee::json_rpc::response<cryptonote::COMMAND_RPC_GET_OUTPUT_HISTOGRAM::response, std::string> resp_t = AUTO_VAL_INIT(resp_t);
    m_daemon_rpc_mutex.lock();
    req_t.jsonrpc = "2.0";
    req_t.id = epee::serialization::storage_entry(0);
    req_t.method = "get_output_histogram";
    for(size_t idx: selected_transfers)
      req_t.params.amounts.push_back(m_transfers[idx].is_rct() ? 0 : m_transfers[idx].amount());
    std::sort(req_t.params.amounts.begin(), req_t.params.amounts.end());
    auto end = std::unique(req_t.params.amounts.begin(), req_t.params.amounts.end());
    req_t.params.amounts.resize(std::distance(req_t.params.amounts.begin(), end));
    req_t.params.unlocked = true;
    req_t.params.recent_cutoff = time(NULL) - RECENT_OUTPUT_ZONE;
    bool r = net_utils::invoke_http_json("/json_rpc", req_t, resp_t, m_http_client, rpc_timeout);
    m_daemon_rpc_mutex.unlock();
    THROW_WALLET_EXCEPTION_IF(!r, error::no_connection_to_daemon, "transfer_selected");
    THROW_WALLET_EXCEPTION_IF(resp_t.result.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "get_output_histogram");
    THROW_WALLET_EXCEPTION_IF(resp_t.result.status != CORE_RPC_STATUS_OK, error::get_histogram_error, resp_t.result.status);

    // we ask for more, to have spares if some outputs are still locked
    size_t base_requested_outputs_count = (size_t)((fake_outputs_count + 1) * 1.5 + 1);
    LOG_PRINT_L2("base_requested_outputs_count: " << base_requested_outputs_count);

    // generate output indices to request
    COMMAND_RPC_GET_OUTPUTS_BIN::request req = AUTO_VAL_INIT(req);
    COMMAND_RPC_GET_OUTPUTS_BIN::response daemon_resp = AUTO_VAL_INIT(daemon_resp);

    size_t num_selected_transfers = 0;
    for(size_t idx: selected_transfers)
    {
      ++num_selected_transfers;
      const transfer_details &td = m_transfers[idx];
      const uint64_t amount = td.is_rct() ? 0 : td.amount();
      std::unordered_set<uint64_t> seen_indices;
      // request more for rct in base recent (locked) coinbases are picked, since they're locked for longer
      size_t requested_outputs_count = base_requested_outputs_count + (td.is_rct() ? CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW - CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE : 0);
      size_t start = req.outputs.size();

      // if there are just enough outputs to mix with, use all of them.
      // Eventually this should become impossible.
      uint64_t num_outs = 0, num_recent_outs = 0;
      for (auto he: resp_t.result.histogram)
      {
        if (he.amount == amount)
        {
          LOG_PRINT_L2("Found " << print_money(amount) << ": " << he.total_instances << " total, "
              << he.unlocked_instances << " unlocked, " << he.recent_instances << " recent");
          num_outs = he.unlocked_instances;
          num_recent_outs = he.recent_instances;
          break;
        }
      }
      LOG_PRINT_L1("" << num_outs << " unlocked outputs of size " << print_money(amount));
      THROW_WALLET_EXCEPTION_IF(num_outs == 0, error::wallet_internal_error,
          "histogram reports no unlocked outputs for " + boost::lexical_cast<std::string>(amount) + ", not even ours");
      THROW_WALLET_EXCEPTION_IF(num_recent_outs > num_outs, error::wallet_internal_error,
          "histogram reports more recent outs than outs for " + boost::lexical_cast<std::string>(amount));

      // X% of those outs are to be taken from recent outputs
      size_t recent_outputs_count = requested_outputs_count * RECENT_OUTPUT_RATIO;
      if (recent_outputs_count == 0)
        recent_outputs_count = 1; // ensure we have at least one, if possible
      if (recent_outputs_count > num_recent_outs)
        recent_outputs_count = num_recent_outs;
      if (td.m_global_output_index >= num_outs - num_recent_outs && recent_outputs_count > 0)
        --recent_outputs_count; // if the real out is recent, pick one less recent fake out
      LOG_PRINT_L1("Using " << recent_outputs_count << " recent outputs");

      if (num_outs <= requested_outputs_count)
      {
        for (uint64_t i = 0; i < num_outs; i++)
          req.outputs.push_back({amount, i});
        // duplicate to make up shortfall: this will be caught after the RPC call,
        // so we can also output the amounts for which we can't reach the required
        // mixin after checking the actual unlockedness
        for (uint64_t i = num_outs; i < requested_outputs_count; ++i)
          req.outputs.push_back({amount, num_outs - 1});
      }
      else
      {
        // start with real one
        uint64_t num_found = 1;
        seen_indices.emplace(td.m_global_output_index);
        req.outputs.push_back({amount, td.m_global_output_index});
        LOG_PRINT_L1("Selecting real output: " << td.m_global_output_index << " for " << print_money(amount));

        // while we still need more mixins
        while (num_found < requested_outputs_count)
        {
          // if we've gone through every possible output, we've gotten all we can
          if (seen_indices.size() == num_outs)
            break;

          // get a random output index from the DB.  If we've already seen it,
          // return to the top of the loop and try again, otherwise add it to the
          // list of output indices we've seen.

          uint64_t i;
          if (num_found - 1 < recent_outputs_count) // -1 to account for the real one we seeded with
          {
            // triangular distribution over [a,b) with a=0, mode c=b=up_index_limit
            uint64_t r = crypto::rand<uint64_t>() % ((uint64_t)1 << 53);
            double frac = std::sqrt((double)r / ((uint64_t)1 << 53));
            i = (uint64_t)(frac*num_recent_outs) + num_outs - num_recent_outs;
            // just in case rounding up to 1 occurs after calc
            if (i == num_outs)
              --i;
            LOG_PRINT_L2("picking " << i << " as recent");
          }
          else
          {
            // triangular distribution over [a,b) with a=0, mode c=b=up_index_limit
            uint64_t r = crypto::rand<uint64_t>() % ((uint64_t)1 << 53);
            double frac = std::sqrt((double)r / ((uint64_t)1 << 53));
            i = (uint64_t)(frac*num_outs);
            // just in case rounding up to 1 occurs after calc
            if (i == num_outs)
              --i;
            LOG_PRINT_L2("picking " << i << " as triangular");
          }

          if (seen_indices.count(i))
            continue;
          seen_indices.emplace(i);

          req.outputs.push_back({amount, i});
          ++num_found;
        }
      }

      // sort the subsection, to ensure the daemon doesn't know wich output is ours
      std::sort(req.outputs.begin() + start, req.outputs.end(),
          [](const get_outputs_out &a, const get_outputs_out &b) { return a.index < b.index; });
    }

    for (auto i: req.outputs)
      LOG_PRINT_L1("asking for output " << i.index << " for " << print_money(i.amount));

    // get the keys for those
    m_daemon_rpc_mutex.lock();
    r = epee::net_utils::invoke_http_bin("/get_outs.bin", req, daemon_resp, m_http_client, rpc_timeout);
    m_daemon_rpc_mutex.unlock();
    THROW_WALLET_EXCEPTION_IF(!r, error::no_connection_to_daemon, "get_outs.bin");
    THROW_WALLET_EXCEPTION_IF(daemon_resp.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "get_outs.bin");
    THROW_WALLET_EXCEPTION_IF(daemon_resp.status != CORE_RPC_STATUS_OK, error::get_random_outs_error, daemon_resp.status);
    THROW_WALLET_EXCEPTION_IF(daemon_resp.outs.size() != req.outputs.size(), error::wallet_internal_error,
      "daemon returned wrong response for get_outs.bin, wrong amounts count = " +
      std::to_string(daemon_resp.outs.size()) + ", expected " +  std::to_string(req.outputs.size()));

    std::unordered_map<uint64_t, uint64_t> scanty_outs;
    size_t base = 0;
    outs.reserve(num_selected_transfers);
    for(size_t idx: selected_transfers)
    {
      const transfer_details &td = m_transfers[idx];
      size_t requested_outputs_count = base_requested_outputs_count + (td.is_rct() ? CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW - CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE : 0);
      outs.push_back(std::vector<get_outs_entry>());
      outs.back().reserve(fake_outputs_count + 1);
      const rct::key mask = td.is_rct() ? rct::commit(td.amount(), td.m_mask) : rct::zeroCommit(td.amount());

      // make sure the real outputs we asked for are really included, along
      // with the correct key and mask: this guards against an active attack
      // where the node sends dummy data for all outputs, and we then send
      // the real one, which the node can then tell from the fake outputs,
      // as it has different data than the dummy data it had sent earlier
      bool real_out_found = false;
      for (size_t n = 0; n < requested_outputs_count; ++n)
      {
        size_t i = base + n;
        if (req.outputs[i].index == td.m_global_output_index)
          if (daemon_resp.outs[i].key == boost::get<txout_to_key>(td.m_tx.vout[td.m_internal_output_index].target).key)
            if (daemon_resp.outs[i].mask == mask)
              real_out_found = true;
      }
      THROW_WALLET_EXCEPTION_IF(!real_out_found, error::wallet_internal_error,
          "Daemon response did not include the requested real output");

      // pick real out first (it will be sorted when done)
      outs.back().push_back(std::make_tuple(td.m_global_output_index, boost::get<txout_to_key>(td.m_tx.vout[td.m_internal_output_index].target).key, mask));

      // then pick others in random order till we reach the required number
      // since we use an equiprobable pick here, we don't upset the triangular distribution
      std::vector<size_t> order;
      order.resize(requested_outputs_count);
      for (size_t n = 0; n < order.size(); ++n)
        order[n] = n;
      std::shuffle(order.begin(), order.end(), std::default_random_engine(crypto::rand<unsigned>()));

      LOG_PRINT_L2("Looking for " << (fake_outputs_count+1) << " outputs of size " << print_money(td.is_rct() ? 0 : td.amount()));
      for (size_t o = 0; o < requested_outputs_count && outs.back().size() < fake_outputs_count + 1; ++o)
      {
        size_t i = base + order[o];
        LOG_PRINT_L2("Index " << i << "/" << requested_outputs_count << ": idx " << req.outputs[i].index << " (real " << td.m_global_output_index << "), unlocked " << daemon_resp.outs[i].unlocked << ", key " << daemon_resp.outs[i].key);
        if (req.outputs[i].index == td.m_global_output_index) // don't re-add real one
          continue;
        if (!daemon_resp.outs[i].unlocked) // don't add locked outs
          continue;
        auto item = std::make_tuple(req.outputs[i].index, daemon_resp.outs[i].key, daemon_resp.outs[i].mask);
        if (std::find(outs.back().begin(), outs.back().end(), item) != outs.back().end()) // don't add duplicates
          continue;
        outs.back().push_back(item);
      }
      if (outs.back().size() < fake_outputs_count + 1)
      {
        scanty_outs[td.is_rct() ? 0 : td.amount()] = outs.back().size();
      }
      else
      {
        // sort the subsection, so any spares are reset in order
        std::sort(outs.back().begin(), outs.back().end(), [](const get_outs_entry &a, const get_outs_entry &b) { return std::get<0>(a) < std::get<0>(b); });
      }
      base += requested_outputs_count;
    }
    THROW_WALLET_EXCEPTION_IF(!scanty_outs.empty(), error::not_enough_outs_to_mix, scanty_outs, fake_outputs_count);
  }
  else
  {
    for (size_t idx: selected_transfers)
    {
      const transfer_details &td = m_transfers[idx];
      std::vector<get_outs_entry> v;
      const rct::key mask = td.is_rct() ? rct::commit(td.amount(), td.m_mask) : rct::zeroCommit(td.amount());
      v.push_back(std::make_tuple(td.m_global_output_index, boost::get<txout_to_key>(td.m_tx.vout[td.m_internal_output_index].target).key, mask));
      outs.push_back(v);
    }
  }
}

template<typename T>
void wallet2::transfer_selected(const std::vector<cryptonote::tx_destination_entry>& dsts, const std::list<size_t> selected_transfers, size_t fake_outputs_count, 
  std::vector<std::vector<tools::wallet2::get_outs_entry>> &outs,
  uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra, T destination_split_strategy, const tx_dust_policy& dust_policy, cryptonote::transaction& tx, pending_tx &ptx)
{
  using namespace cryptonote;
  // throw if attempting a transaction with no destinations
  THROW_WALLET_EXCEPTION_IF(dsts.empty(), error::zero_destination);

  uint64_t upper_transaction_size_limit = get_upper_transaction_size_limit();
  uint64_t needed_money = fee;
  LOG_PRINT_L2("transfer: starting with fee " << print_money (needed_money));

  // calculate total amount being sent to all destinations
  // throw if total amount overflows uint64_t
  for(auto& dt: dsts)
  {
    THROW_WALLET_EXCEPTION_IF(0 == dt.amount, error::zero_destination);
    needed_money += dt.amount;
    LOG_PRINT_L2("transfer: adding " << print_money(dt.amount) << ", for a total of " << print_money (needed_money));
    THROW_WALLET_EXCEPTION_IF(needed_money < dt.amount, error::tx_sum_overflow, dsts, fee, m_testnet);
  }

  uint64_t found_money = 0;
  for(size_t idx: selected_transfers)
  {
    found_money += m_transfers[idx].amount();
  }

  LOG_PRINT_L2("wanted " << print_money(needed_money) << ", found " << print_money(found_money) << ", fee " << print_money(fee));
  THROW_WALLET_EXCEPTION_IF(found_money < needed_money, error::not_enough_money, found_money, needed_money - fee, fee);

  if (outs.empty())
    get_outs(outs, selected_transfers, fake_outputs_count); // may throw

  //prepare inputs
  LOG_PRINT_L2("preparing outputs");
  typedef cryptonote::tx_source_entry::output_entry tx_output_entry;
  size_t i = 0, out_index = 0;
  std::vector<cryptonote::tx_source_entry> sources;
  for(size_t idx: selected_transfers)
  {
    sources.resize(sources.size()+1);
    cryptonote::tx_source_entry& src = sources.back();
    const transfer_details& td = m_transfers[idx];
    src.amount = td.amount();
    src.rct = td.is_rct();
    //paste keys (fake and real)

    for (size_t n = 0; n < fake_outputs_count + 1; ++n)
    {
      tx_output_entry oe;
      oe.first = std::get<0>(outs[out_index][n]);
      oe.second.dest = rct::pk2rct(std::get<1>(outs[out_index][n]));
      oe.second.mask = std::get<2>(outs[out_index][n]);

      src.outputs.push_back(oe);
      ++i;
    }

    //paste real transaction to the random index
    auto it_to_replace = std::find_if(src.outputs.begin(), src.outputs.end(), [&](const tx_output_entry& a)
    {
      return a.first == td.m_global_output_index;
    });
    THROW_WALLET_EXCEPTION_IF(it_to_replace == src.outputs.end(), error::wallet_internal_error,
        "real output not found");

    tx_output_entry real_oe;
    real_oe.first = td.m_global_output_index;
    real_oe.second.dest = rct::pk2rct(boost::get<txout_to_key>(td.m_tx.vout[td.m_internal_output_index].target).key);
    real_oe.second.mask = rct::commit(td.amount(), td.m_mask);
    *it_to_replace = real_oe;
    src.real_out_tx_key = get_tx_pub_key_from_extra(td.m_tx, td.m_pk_index);
    src.real_output = it_to_replace - src.outputs.begin();
    src.real_output_in_tx_index = td.m_internal_output_index;
    detail::print_source_entry(src);
    ++out_index;
  }
  LOG_PRINT_L2("outputs prepared");

  cryptonote::tx_destination_entry change_dts = AUTO_VAL_INIT(change_dts);
  if (needed_money < found_money)
  {
    change_dts.addr = m_account.get_keys().m_account_address;
    change_dts.amount = found_money - needed_money;
  }

  std::vector<cryptonote::tx_destination_entry> splitted_dsts, dust_dsts;
  uint64_t dust = 0;
  destination_split_strategy(dsts, change_dts, dust_policy.dust_threshold, splitted_dsts, dust_dsts);
  for(auto& d: dust_dsts) {
    THROW_WALLET_EXCEPTION_IF(dust_policy.dust_threshold < d.amount, error::wallet_internal_error, "invalid dust value: dust = " +
      std::to_string(d.amount) + ", dust_threshold = " + std::to_string(dust_policy.dust_threshold));
  }
  for(auto& d: dust_dsts) {
    if (!dust_policy.add_to_fee)
      splitted_dsts.push_back(cryptonote::tx_destination_entry(d.amount, dust_policy.addr_for_dust));
    dust += d.amount;
  }

  crypto::secret_key tx_key;
  LOG_PRINT_L2("constructing tx");
  bool r = cryptonote::construct_tx_and_get_tx_key(m_account.get_keys(), sources, splitted_dsts, extra, tx, unlock_time, tx_key);
  LOG_PRINT_L2("constructed tx, r="<<r);
  THROW_WALLET_EXCEPTION_IF(!r, error::tx_not_constructed, sources, splitted_dsts, unlock_time, m_testnet);
  THROW_WALLET_EXCEPTION_IF(upper_transaction_size_limit <= get_object_blobsize(tx), error::tx_too_big, tx, upper_transaction_size_limit);

  std::string key_images;
  bool all_are_txin_to_key = std::all_of(tx.vin.begin(), tx.vin.end(), [&](const txin_v& s_e) -> bool
  {
    CHECKED_GET_SPECIFIC_VARIANT(s_e, const txin_to_key, in, false);
    key_images += boost::to_string(in.k_image) + " ";
    return true;
  });
  THROW_WALLET_EXCEPTION_IF(!all_are_txin_to_key, error::unexpected_txin_type, tx);
  
  
  bool dust_sent_elsewhere = (dust_policy.addr_for_dust.m_view_public_key != change_dts.addr.m_view_public_key
                                || dust_policy.addr_for_dust.m_spend_public_key != change_dts.addr.m_spend_public_key);
  
  if (dust_policy.add_to_fee || dust_sent_elsewhere) change_dts.amount -= dust;

  ptx.key_images = key_images;
  ptx.fee = (dust_policy.add_to_fee ? fee+dust : fee);
  ptx.dust = ((dust_policy.add_to_fee || dust_sent_elsewhere) ? dust : 0);
  ptx.dust_added_to_fee = dust_policy.add_to_fee;
  ptx.tx = tx;
  ptx.change_dts = change_dts;
  ptx.selected_transfers = selected_transfers;
  ptx.tx_key = tx_key;
  ptx.dests = dsts;
  ptx.construction_data.sources = sources;
  ptx.construction_data.change_dts = change_dts;
  ptx.construction_data.splitted_dsts = splitted_dsts;
  ptx.construction_data.selected_transfers = selected_transfers;
  ptx.construction_data.extra = tx.extra;
  ptx.construction_data.unlock_time = unlock_time;
  ptx.construction_data.use_rct = false;
  ptx.construction_data.dests = dsts;
  LOG_PRINT_L2("transfer_selected done");
}

void wallet2::transfer_selected_rct(std::vector<cryptonote::tx_destination_entry> dsts, const std::list<size_t> selected_transfers, size_t fake_outputs_count,
  std::vector<std::vector<tools::wallet2::get_outs_entry>> &outs,
  uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra, cryptonote::transaction& tx, pending_tx &ptx)
{
  using namespace cryptonote;
  // throw if attempting a transaction with no destinations
  THROW_WALLET_EXCEPTION_IF(dsts.empty(), error::zero_destination);

  uint64_t upper_transaction_size_limit = get_upper_transaction_size_limit();
  uint64_t needed_money = fee;
  LOG_PRINT_L2("transfer_selected_rct: starting with fee " << print_money (needed_money));
  LOG_PRINT_L0("selected transfers: ");
  for (auto t: selected_transfers)
    LOG_PRINT_L2("  " << t);

  // calculate total amount being sent to all destinations
  // throw if total amount overflows uint64_t
  for(auto& dt: dsts)
  {
    THROW_WALLET_EXCEPTION_IF(0 == dt.amount, error::zero_destination);
    needed_money += dt.amount;
    LOG_PRINT_L2("transfer: adding " << print_money(dt.amount) << ", for a total of " << print_money (needed_money));
    THROW_WALLET_EXCEPTION_IF(needed_money < dt.amount, error::tx_sum_overflow, dsts, fee, m_testnet);
  }

  uint64_t found_money = 0;
  for(size_t idx: selected_transfers)
  {
    found_money += m_transfers[idx].amount();
  }

  LOG_PRINT_L2("wanted " << print_money(needed_money) << ", found " << print_money(found_money) << ", fee " << print_money(fee));
  THROW_WALLET_EXCEPTION_IF(found_money < needed_money, error::not_enough_money, found_money, needed_money - fee, fee);

  if (outs.empty())
    get_outs(outs, selected_transfers, fake_outputs_count); // may throw

  //prepare inputs
  LOG_PRINT_L2("preparing outputs");
  size_t i = 0, out_index = 0;
  std::vector<cryptonote::tx_source_entry> sources;
  for(size_t idx: selected_transfers)
  {
    sources.resize(sources.size()+1);
    cryptonote::tx_source_entry& src = sources.back();
    const transfer_details& td = m_transfers[idx];
    src.amount = td.amount();
    src.rct = td.is_rct();
    //paste mixin transaction

    typedef cryptonote::tx_source_entry::output_entry tx_output_entry;
    for (size_t n = 0; n < fake_outputs_count + 1; ++n)
    {
      tx_output_entry oe;
      oe.first = std::get<0>(outs[out_index][n]);
      oe.second.dest = rct::pk2rct(std::get<1>(outs[out_index][n]));
      oe.second.mask = std::get<2>(outs[out_index][n]);
      src.outputs.push_back(oe);
    }
    ++i;

    //paste real transaction to the random index
    auto it_to_replace = std::find_if(src.outputs.begin(), src.outputs.end(), [&](const tx_output_entry& a)
    {
      return a.first == td.m_global_output_index;
    });
    THROW_WALLET_EXCEPTION_IF(it_to_replace == src.outputs.end(), error::wallet_internal_error,
        "real output not found");

    tx_output_entry real_oe;
    real_oe.first = td.m_global_output_index;
    real_oe.second.dest = rct::pk2rct(boost::get<txout_to_key>(td.m_tx.vout[td.m_internal_output_index].target).key);
    real_oe.second.mask = rct::commit(td.amount(), td.m_mask);
    *it_to_replace = real_oe;
    src.real_out_tx_key = get_tx_pub_key_from_extra(td.m_tx, td.m_pk_index);
    src.real_output = it_to_replace - src.outputs.begin();
    src.real_output_in_tx_index = td.m_internal_output_index;
    src.mask = td.m_mask;
    detail::print_source_entry(src);
    ++out_index;
  }
  LOG_PRINT_L2("outputs prepared");

  // we still keep a copy, since we want to keep dsts free of change for user feedback purposes
  std::vector<cryptonote::tx_destination_entry> splitted_dsts = dsts;
  cryptonote::tx_destination_entry change_dts = AUTO_VAL_INIT(change_dts);
  change_dts.amount = found_money - needed_money;
  if (change_dts.amount == 0)
  {
    // If the change is 0, send it to a random address, to avoid confusing
    // the sender with a 0 amount output. We send a 0 amount in order to avoid
    // letting the destination be able to work out which of the inputs is the
    // real one in our rings
    LOG_PRINT_L2("generating dummy address for 0 change");
    cryptonote::account_base dummy;
    dummy.generate();
    change_dts.addr = dummy.get_keys().m_account_address;
    LOG_PRINT_L2("generated dummy address for 0 change");
  }
  else
  {
    change_dts.addr = m_account.get_keys().m_account_address;
  }
  splitted_dsts.push_back(change_dts);

  crypto::secret_key tx_key;
  LOG_PRINT_L2("constructing tx");
  bool r = cryptonote::construct_tx_and_get_tx_key(m_account.get_keys(), sources, splitted_dsts, extra, tx, unlock_time, tx_key, true);
  LOG_PRINT_L2("constructed tx, r="<<r);
  THROW_WALLET_EXCEPTION_IF(!r, error::tx_not_constructed, sources, dsts, unlock_time, m_testnet);
  THROW_WALLET_EXCEPTION_IF(upper_transaction_size_limit <= get_object_blobsize(tx), error::tx_too_big, tx, upper_transaction_size_limit);

  LOG_PRINT_L2("gathering key images");
  std::string key_images;
  bool all_are_txin_to_key = std::all_of(tx.vin.begin(), tx.vin.end(), [&](const txin_v& s_e) -> bool
  {
    CHECKED_GET_SPECIFIC_VARIANT(s_e, const txin_to_key, in, false);
    key_images += boost::to_string(in.k_image) + " ";
    return true;
  });
  THROW_WALLET_EXCEPTION_IF(!all_are_txin_to_key, error::unexpected_txin_type, tx);
  LOG_PRINT_L2("gathered key images");

  ptx.key_images = key_images;
  ptx.fee = fee;
  ptx.dust = 0;
  ptx.dust_added_to_fee = false;
  ptx.tx = tx;
  ptx.change_dts = change_dts;
  ptx.selected_transfers = selected_transfers;
  ptx.tx_key = tx_key;
  ptx.dests = dsts;
  ptx.construction_data.sources = sources;
  ptx.construction_data.change_dts = change_dts;
  ptx.construction_data.splitted_dsts = splitted_dsts;
  ptx.construction_data.selected_transfers = selected_transfers;
  ptx.construction_data.extra = tx.extra;
  ptx.construction_data.unlock_time = unlock_time;
  ptx.construction_data.use_rct = true;
  ptx.construction_data.dests = dsts;
  LOG_PRINT_L2("transfer_selected_rct done");
}

static size_t estimate_rct_tx_size(int n_inputs, int mixin, int n_outputs)
{
  size_t size = 0;

  // tx prefix

  // first few bytes
  size += 1 + 6;

  // vin
  size += n_inputs * (1+6+(mixin+1)*2+32);

  // vout
  size += n_outputs * (6+32);

  // extra
  size += 40;

  // rct signatures

  // type
  size += 1;

  // rangeSigs
  size += (2*64*32+32+64*32) * n_outputs;

  // MGs
  size += n_inputs * (64 * (mixin+1) + 32);

  // mixRing - not serialized, can be reconstructed
  /* size += 2 * 32 * (mixin+1) * n_inputs; */

  // pseudoOuts
  size += 32 * n_inputs;
  // ecdhInfo
  size += 2 * 32 * n_outputs;
  // outPk - only commitment is saved
  size += 32 * n_outputs;
  // txnFee
  size += 4;

  LOG_PRINT_L2("estimated rct tx size for " << n_inputs << " with ring size " << (mixin+1) << " and " << n_outputs << ": " << size << " (" << ((32 * n_inputs/*+1*/) + 2 * 32 * (mixin+1) * n_inputs + 32 * n_outputs) << " saved)");
  return size;
}

static size_t estimate_tx_size(bool use_rct, int n_inputs, int mixin, int n_outputs)
{
  if (use_rct)
    return estimate_rct_tx_size(n_inputs, mixin, n_outputs + 1);
  else
    return n_inputs * (mixin+1) * APPROXIMATE_INPUT_BYTES;
}

std::vector<size_t> wallet2::pick_preferred_rct_inputs(uint64_t needed_money) const
{
  std::vector<size_t> picks;
  float current_output_relatdness = 1.0f;

  LOG_PRINT_L2("pick_preferred_rct_inputs: needed_money " << print_money(needed_money));

  // try to find a rct input of enough size
  for (size_t i = 0; i < m_transfers.size(); ++i)
  {
    const transfer_details& td = m_transfers[i];
    if (!td.m_spent && td.is_rct() && td.amount() >= needed_money && is_transfer_unlocked(td))
    {
      LOG_PRINT_L2("We can use " << i << " alone: " << print_money(td.amount()));
      picks.push_back(i);
      return picks;
    }
  }

  // then try to find two outputs
  // this could be made better by picking one of the outputs to be a small one, since those
  // are less useful since often below the needed money, so if one can be used in a pair,
  // it gets rid of it for the future
  for (size_t i = 0; i < m_transfers.size(); ++i)
  {
    const transfer_details& td = m_transfers[i];
    if (!td.m_spent && td.is_rct() && is_transfer_unlocked(td))
    {
      LOG_PRINT_L2("Considering input " << i << ", " << print_money(td.amount()));
      for (size_t j = i + 1; j < m_transfers.size(); ++j)
      {
        const transfer_details& td2 = m_transfers[j];
        if (!td2.m_spent && td2.is_rct() && td.amount() + td2.amount() >= needed_money && is_transfer_unlocked(td2))
        {
          // update our picks if those outputs are less related than any we
          // already found. If the same, don't update, and oldest suitable outputs
          // will be used in preference.
          float relatedness = get_output_relatedness(td, td2);
          LOG_PRINT_L2("  with input " << j << ", " << print_money(td2.amount()) << ", relatedness " << relatedness);
          if (relatedness < current_output_relatdness)
          {
            // reset the current picks with those, and return them directly
            // if they're unrelated. If they are related, we'll end up returning
            // them if we find nothing better
            picks.clear();
            picks.push_back(i);
            picks.push_back(j);
            LOG_PRINT_L0("we could use " << i << " and " << j);
            if (relatedness == 0.0f)
              return picks;
            current_output_relatdness = relatedness;
          }
        }
      }
    }
  }

  return picks;
}

bool wallet2::should_pick_a_second_output(bool use_rct, size_t n_transfers, const std::vector<size_t> &unused_transfers_indices, const std::vector<size_t> &unused_dust_indices) const
{
  if (!use_rct)
    return false;
  if (n_transfers > 1)
    return false;
  if (unused_dust_indices.empty() && unused_transfers_indices.empty())
    return false;
  // we want at least one free rct output to avoid a corner case where
  // we'd choose a non rct output which doesn't have enough "siblings"
  // value-wise on the chain, and thus can't be mixed
  bool found = false;
  for (auto i: unused_dust_indices)
  {
    if (m_transfers[i].is_rct())
    {
      found = true;
      break;
    }
  }
  if (!found) for (auto i: unused_transfers_indices)
  {
    if (m_transfers[i].is_rct())
    {
      found = true;
      break;
    }
  }
  if (!found)
    return false;
  return true;
}

std::vector<size_t> wallet2::get_only_rct(const std::vector<size_t> &unused_dust_indices, const std::vector<size_t> &unused_transfers_indices) const
{
  std::vector<size_t> indices;
  for (size_t n: unused_dust_indices)
    if (m_transfers[n].is_rct())
      indices.push_back(n);
  for (size_t n: unused_transfers_indices)
    if (m_transfers[n].is_rct())
      indices.push_back(n);
  return indices;
}

static uint32_t get_count_above(const std::vector<wallet2::transfer_details> &transfers, const std::vector<size_t> &indices, uint64_t threshold)
{
  uint32_t count = 0;
  for (size_t idx: indices)
    if (transfers[idx].amount() >= threshold)
      ++count;
  return count;
}

// Another implementation of transaction creation that is hopefully better
// While there is anything left to pay, it goes through random outputs and tries
// to fill the next destination/amount. If it fully fills it, it will use the
// remainder to try to fill the next one as well.
// The tx size if roughly estimated as a linear function of only inputs, and a
// new tx will be created when that size goes above a given fraction of the
// max tx size. At that point, more outputs may be added if the fee cannot be
// satisfied.
// If the next output in the next tx would go to the same destination (ie, we
// cut off at a tx boundary in the middle of paying a given destination), the
// fee will be carved out of the current input if possible, to avoid having to
// add another output just for the fee and getting change.
// This system allows for sending (almost) the entire balance, since it does
// not generate spurious change in all txes, thus decreasing the instantaneous
// usable balance.
std::vector<wallet2::pending_tx> wallet2::create_transactions_2(std::vector<cryptonote::tx_destination_entry> dsts, const size_t fake_outs_count, const uint64_t unlock_time, uint32_t priority, const std::vector<uint8_t> extra, bool trusted_daemon)
{
  std::vector<size_t> unused_transfers_indices;
  std::vector<size_t> unused_dust_indices;
  uint64_t needed_money;
  uint64_t accumulated_fee, accumulated_outputs, accumulated_change;
  struct TX {
    std::list<size_t> selected_transfers;
    std::vector<cryptonote::tx_destination_entry> dsts;
    cryptonote::transaction tx;
    pending_tx ptx;
    size_t bytes;

    void add(const account_public_address &addr, uint64_t amount, unsigned int original_output_index, bool merge_destinations) {
      if (merge_destinations)
      {
        std::vector<cryptonote::tx_destination_entry>::iterator i;
        i = std::find_if(dsts.begin(), dsts.end(), [&](const cryptonote::tx_destination_entry &d) { return !memcmp (&d.addr, &addr, sizeof(addr)); });
        if (i == dsts.end())
        {
          dsts.push_back(tx_destination_entry(0,addr));
          i = dsts.end() - 1;
        }
        i->amount += amount;
      }
      else
      {
        THROW_WALLET_EXCEPTION_IF(original_output_index > dsts.size(), error::wallet_internal_error, "original_output_index too large");
        if (original_output_index == dsts.size())
          dsts.push_back(tx_destination_entry(0,addr));
        THROW_WALLET_EXCEPTION_IF(memcmp(&dsts[original_output_index].addr, &addr, sizeof(addr)), error::wallet_internal_error, "Mismatched destination address");
        dsts[original_output_index].amount += amount;
      }
    }
  };
  std::vector<TX> txes;
  bool adding_fee; // true if new outputs go towards fee, rather than destinations
  uint64_t needed_fee, available_for_fee = 0;
  uint64_t upper_transaction_size_limit = get_upper_transaction_size_limit();
  const bool use_rct = use_fork_rules(4, 0);

  const uint64_t fee_per_kb  = get_per_kb_fee();
  const uint64_t fee_multiplier = get_fee_multiplier(priority, get_fee_algorithm());

  // throw if attempting a transaction with no destinations
  THROW_WALLET_EXCEPTION_IF(dsts.empty(), error::zero_destination);

  // calculate total amount being sent to all destinations
  // throw if total amount overflows uint64_t
  needed_money = 0;
  for(auto& dt: dsts)
  {
    THROW_WALLET_EXCEPTION_IF(0 == dt.amount, error::zero_destination);
    needed_money += dt.amount;
    LOG_PRINT_L2("transfer: adding " << print_money(dt.amount) << ", for a total of " << print_money (needed_money));
    THROW_WALLET_EXCEPTION_IF(needed_money < dt.amount, error::tx_sum_overflow, dsts, 0, m_testnet);
  }

  // throw if attempting a transaction with no money
  THROW_WALLET_EXCEPTION_IF(needed_money == 0, error::zero_destination);

  // gather all our dust and non dust outputs
  for (size_t i = 0; i < m_transfers.size(); ++i)
  {
    const transfer_details& td = m_transfers[i];
    if (!td.m_spent && (use_rct ? true : !td.is_rct()) && is_transfer_unlocked(td))
    {
      if ((td.is_rct()) || is_valid_decomposed_amount(td.amount()))
        unused_transfers_indices.push_back(i);
      else
        unused_dust_indices.push_back(i);
    }
  }
  LOG_PRINT_L2("Starting with " << unused_transfers_indices.size() << " non-dust outputs and " << unused_dust_indices.size() << " dust outputs");

  // early out if we know we can't make it anyway
  // we could also check for being within FEE_PER_KB, but if the fee calculation
  // ever changes, this might be missed, so let this go through
  THROW_WALLET_EXCEPTION_IF(needed_money > unlocked_balance(), error::not_enough_money,
      unlocked_balance(), needed_money, 0);

  if (unused_dust_indices.empty() && unused_transfers_indices.empty())
    return std::vector<wallet2::pending_tx>();

  // start with an empty tx
  txes.push_back(TX());
  accumulated_fee = 0;
  accumulated_outputs = 0;
  accumulated_change = 0;
  adding_fee = false;
  needed_fee = 0;
  std::vector<std::vector<tools::wallet2::get_outs_entry>> outs;

  // for rct, since we don't see the amounts, we will try to make all transactions
  // look the same, with 1 or 2 inputs, and 2 outputs. One input is preferable, as
  // this prevents linking to another by provenance analysis, but two is ok if we
  // try to pick outputs not from the same block. We will get two outputs, one for
  // the destination, and one for change.
  LOG_PRINT_L2("checking preferred");
  std::vector<size_t> preferred_inputs;
  uint64_t rct_outs_needed = 2 * (fake_outs_count + 1);
  rct_outs_needed += 100; // some fudge factor since we don't know how many are locked
  if (use_rct && get_num_rct_outputs() >= rct_outs_needed)
  {
    // this is used to build a tx that's 1 or 2 inputs, and 2 outputs, which
    // will get us a known fee.
    uint64_t estimated_fee = calculate_fee(fee_per_kb, estimate_rct_tx_size(2, fake_outs_count + 1, 2), fee_multiplier);
    preferred_inputs = pick_preferred_rct_inputs(needed_money + estimated_fee);
    if (!preferred_inputs.empty())
    {
      string s;
      for (auto i: preferred_inputs) s += boost::lexical_cast<std::string>(i) + "(" + print_money(m_transfers[i].amount()) + ") ";
      LOG_PRINT_L1("Found preferred rct inputs for rct tx: " << s);
    }
  }
  LOG_PRINT_L2("done checking preferred");

  // while:
  // - we have something to send
  // - or we need to gather more fee
  // - or we have just one input in that tx, which is rct (to try and make all/most rct txes 2/2)
  unsigned int original_output_index = 0;
  while ((!dsts.empty() && dsts[0].amount > 0) || adding_fee || should_pick_a_second_output(use_rct, txes.back().selected_transfers.size(), unused_transfers_indices, unused_dust_indices)) {
    TX &tx = txes.back();

    LOG_PRINT_L2("Start of loop with " << unused_transfers_indices.size() << " " << unused_dust_indices.size());
    LOG_PRINT_L2("unused_transfers_indices:");
    for (auto t: unused_transfers_indices)
      LOG_PRINT_L2("  " << t);
    LOG_PRINT_L2("unused_dust_indices:");
    for (auto t: unused_dust_indices)
      LOG_PRINT_L2("  " << t);
    LOG_PRINT_L2("dsts size " << dsts.size() << ", first " << (dsts.empty() ? -1 : dsts[0].amount));
    LOG_PRINT_L2("adding_fee " << adding_fee << ", use_rct " << use_rct);

    // if we need to spend money and don't have any left, we fail
    if (unused_dust_indices.empty() && unused_transfers_indices.empty()) {
      LOG_PRINT_L2("No more outputs to choose from");
      THROW_WALLET_EXCEPTION_IF(1, error::tx_not_possible, unlocked_balance(), needed_money, accumulated_fee + needed_fee);
    }

    // get a random unspent output and use it to pay part (or all) of the current destination (and maybe next one, etc)
    // This could be more clever, but maybe at the cost of making probabilistic inferences easier
    size_t idx;
    if ((dsts.empty() || dsts[0].amount == 0) && !adding_fee) {
      // the "make rct txes 2/2" case - we pick a small value output to "clean up" the wallet too
      std::vector<size_t> indices = get_only_rct(unused_dust_indices, unused_transfers_indices);
      idx = pop_best_value(indices, tx.selected_transfers, true);

      // we might not want to add it if it's a large output and we don't have many left
      if (m_transfers[idx].amount() >= m_min_output_value) {
        if (get_count_above(m_transfers, unused_transfers_indices, m_min_output_value) < m_min_output_count) {
          LOG_PRINT_L2("Second output was not strictly needed, and we're running out of outputs above " << print_money(m_min_output_value) << ", not adding");
          break;
        }
      }

      // since we're trying to add a second output which is not strictly needed,
      // we only add it if it's unrelated enough to the first one
      float relatedness = get_output_relatedness(m_transfers[idx], m_transfers[tx.selected_transfers.front()]);
      if (relatedness > SECOND_OUTPUT_RELATEDNESS_THRESHOLD)
      {
        LOG_PRINT_L2("Second output was not strictly needed, and relatedness " << relatedness << ", not adding");
        break;
      }
      pop_if_present(unused_transfers_indices, idx);
      pop_if_present(unused_dust_indices, idx);
    } else if (!preferred_inputs.empty()) {
      idx = pop_back(preferred_inputs);
      pop_if_present(unused_transfers_indices, idx);
      pop_if_present(unused_dust_indices, idx);
    } else
      idx = pop_best_value(unused_transfers_indices.empty() ? unused_dust_indices : unused_transfers_indices, tx.selected_transfers);

    const transfer_details &td = m_transfers[idx];
    LOG_PRINT_L2("Picking output " << idx << ", amount " << print_money(td.amount()) << ", ki " << td.m_key_image);

    // add this output to the list to spend
    tx.selected_transfers.push_back(idx);
    uint64_t available_amount = td.amount();
    accumulated_outputs += available_amount;

    // clear any fake outs we'd already gathered, since we'll need a new set
    outs.clear();

    if (adding_fee)
    {
      LOG_PRINT_L2("We need more fee, adding it to fee");
      available_for_fee += available_amount;
    }
    else
    {
      while (!dsts.empty() && dsts[0].amount <= available_amount && estimate_tx_size(use_rct, tx.selected_transfers.size(), fake_outs_count, tx.dsts.size()) < TX_SIZE_TARGET(upper_transaction_size_limit))
      {
        // we can fully pay that destination
        LOG_PRINT_L2("We can fully pay " << get_account_address_as_str(m_testnet, dsts[0].addr) <<
          " for " << print_money(dsts[0].amount));
        tx.add(dsts[0].addr, dsts[0].amount, original_output_index, m_merge_destinations);
        available_amount -= dsts[0].amount;
        dsts[0].amount = 0;
        pop_index(dsts, 0);
        ++original_output_index;
      }

      if (available_amount > 0 && !dsts.empty() && estimate_tx_size(use_rct, tx.selected_transfers.size(), fake_outs_count, tx.dsts.size()) < TX_SIZE_TARGET(upper_transaction_size_limit)) {
        // we can partially fill that destination
        LOG_PRINT_L2("We can partially pay " << get_account_address_as_str(m_testnet, dsts[0].addr) <<
          " for " << print_money(available_amount) << "/" << print_money(dsts[0].amount));
        tx.add(dsts[0].addr, available_amount, original_output_index, m_merge_destinations);
        dsts[0].amount -= available_amount;
        available_amount = 0;
      }
    }

    // here, check if we need to sent tx and start a new one
    LOG_PRINT_L2("Considering whether to create a tx now, " << tx.selected_transfers.size() << " inputs, tx limit "
      << upper_transaction_size_limit);
    bool try_tx;
    if (adding_fee)
    {
      /* might not actually be enough if adding this output bumps size to next kB, but we need to try */
      try_tx = available_for_fee >= needed_fee;
    }
    else
    {
      const size_t estimated_rct_tx_size = estimate_tx_size(use_rct, tx.selected_transfers.size(), fake_outs_count, tx.dsts.size());
      try_tx = dsts.empty() || (estimated_rct_tx_size >= TX_SIZE_TARGET(upper_transaction_size_limit));
    }

    if (try_tx) {
      cryptonote::transaction test_tx;
      pending_tx test_ptx;

      needed_fee = 0;

      LOG_PRINT_L2("Trying to create a tx now, with " << tx.dsts.size() << " destinations and " <<
        tx.selected_transfers.size() << " outputs");
      if (use_rct)
        transfer_selected_rct(tx.dsts, tx.selected_transfers, fake_outs_count, outs, unlock_time, needed_fee, extra,
          test_tx, test_ptx);
      else
        transfer_selected(tx.dsts, tx.selected_transfers, fake_outs_count, outs, unlock_time, needed_fee, extra,
          detail::digit_split_strategy, tx_dust_policy(::config::DEFAULT_DUST_THRESHOLD), test_tx, test_ptx);
      auto txBlob = t_serializable_object_to_blob(test_ptx.tx);
      needed_fee = calculate_fee(fee_per_kb, txBlob, fee_multiplier);
      available_for_fee = test_ptx.fee + test_ptx.change_dts.amount + (!test_ptx.dust_added_to_fee ? test_ptx.dust : 0);
      LOG_PRINT_L2("Made a " << ((txBlob.size() + 1023) / 1024) << " kB tx, with " << print_money(available_for_fee) << " available for fee (" <<
        print_money(needed_fee) << " needed)");

      if (needed_fee > available_for_fee && dsts[0].amount > 0)
      {
        // we don't have enough for the fee, but we've only partially paid the current address,
        // so we can take the fee from the paid amount, since we'll have to make another tx anyway
        std::vector<cryptonote::tx_destination_entry>::iterator i;
        i = std::find_if(tx.dsts.begin(), tx.dsts.end(),
          [&](const cryptonote::tx_destination_entry &d) { return !memcmp (&d.addr, &dsts[0].addr, sizeof(dsts[0].addr)); });
        THROW_WALLET_EXCEPTION_IF(i == tx.dsts.end(), error::wallet_internal_error, "paid address not found in outputs");
        if (i->amount > needed_fee)
        {
          uint64_t new_paid_amount = i->amount /*+ test_ptx.fee*/ - needed_fee;
          LOG_PRINT_L2("Adjusting amount paid to " << get_account_address_as_str(m_testnet, i->addr) << " from " <<
            print_money(i->amount) << " to " << print_money(new_paid_amount) << " to accommodate " <<
            print_money(needed_fee) << " fee");
          dsts[0].amount += i->amount - new_paid_amount;
          i->amount = new_paid_amount;
          test_ptx.fee = needed_fee;
          available_for_fee = needed_fee;
        }
      }

      if (needed_fee > available_for_fee)
      {
        LOG_PRINT_L2("We could not make a tx, switching to fee accumulation");

        adding_fee = true;
      }
      else
      {
        LOG_PRINT_L2("We made a tx, adjusting fee and saving it");
        do {
          if (use_rct)
            transfer_selected_rct(tx.dsts, tx.selected_transfers, fake_outs_count, outs, unlock_time, needed_fee, extra,
              test_tx, test_ptx);
          else
            transfer_selected(tx.dsts, tx.selected_transfers, fake_outs_count, outs, unlock_time, needed_fee, extra,
              detail::digit_split_strategy, tx_dust_policy(::config::DEFAULT_DUST_THRESHOLD), test_tx, test_ptx);
          txBlob = t_serializable_object_to_blob(test_ptx.tx);
          needed_fee = calculate_fee(fee_per_kb, txBlob, fee_multiplier);
          LOG_PRINT_L2("Made an attempt at a  final " << ((txBlob.size() + 1023)/1024) << " kB tx, with " << print_money(test_ptx.fee) <<
            " fee  and " << print_money(test_ptx.change_dts.amount) << " change");
        } while (needed_fee > test_ptx.fee);

        LOG_PRINT_L2("Made a final " << ((txBlob.size() + 1023)/1024) << " kB tx, with " << print_money(test_ptx.fee) <<
          " fee  and " << print_money(test_ptx.change_dts.amount) << " change");

        tx.tx = test_tx;
        tx.ptx = test_ptx;
        tx.bytes = txBlob.size();
        accumulated_fee += test_ptx.fee;
        accumulated_change += test_ptx.change_dts.amount;
        adding_fee = false;
        if (!dsts.empty())
        {
          LOG_PRINT_L2("We have more to pay, starting another tx");
          txes.push_back(TX());
        }
      }
    }
  }

  if (adding_fee)
  {
    LOG_PRINT_L1("We ran out of outputs while trying to gather final fee");
    THROW_WALLET_EXCEPTION_IF(1, error::tx_not_possible, unlocked_balance(), needed_money, accumulated_fee + needed_fee);
  }

  LOG_PRINT_L1("Done creating " << txes.size() << " transactions, " << print_money(accumulated_fee) <<
    " total fee, " << print_money(accumulated_change) << " total change");

  std::vector<wallet2::pending_tx> ptx_vector;
  for (std::vector<TX>::iterator i = txes.begin(); i != txes.end(); ++i)
  {
    TX &tx = *i;
    uint64_t tx_money = 0;
    for (size_t idx: tx.selected_transfers)
      tx_money += m_transfers[idx].amount();
    LOG_PRINT_L1("  Transaction " << (1+std::distance(txes.begin(), i)) << "/" << txes.size() <<
      ": " << (tx.bytes+1023)/1024 << " kB, sending " << print_money(tx_money) << " in " << tx.selected_transfers.size() <<
      " outputs to " << tx.dsts.size() << " destination(s), including " <<
      print_money(tx.ptx.fee) << " fee, " << print_money(tx.ptx.change_dts.amount) << " change");
    ptx_vector.push_back(tx.ptx);
  }

  // if we made it this far, we're OK to actually send the transactions
  return ptx_vector;
}

std::vector<wallet2::pending_tx> wallet2::create_transactions_all(uint64_t below, const cryptonote::account_public_address &address, const size_t fake_outs_count, const uint64_t unlock_time, uint32_t priority, const std::vector<uint8_t> extra, bool trusted_daemon)
{
  std::vector<size_t> unused_transfers_indices;
  std::vector<size_t> unused_dust_indices;
  const bool use_rct = use_fork_rules(4, 0);

  // gather all our dust and non dust outputs
  for (size_t i = 0; i < m_transfers.size(); ++i)
  {
    const transfer_details& td = m_transfers[i];
    if (!td.m_spent && (use_rct ? true : !td.is_rct()) && is_transfer_unlocked(td))
    {
      if (below == 0 || td.amount() < below)
      {
        if (td.is_rct() || is_valid_decomposed_amount(td.amount()))
          unused_transfers_indices.push_back(i);
        else
          unused_dust_indices.push_back(i);
      }
    }
  }

  return create_transactions_from(address, unused_transfers_indices, unused_dust_indices, fake_outs_count, unlock_time, priority, extra, trusted_daemon);
}

std::vector<wallet2::pending_tx> wallet2::create_transactions_from(const cryptonote::account_public_address &address, std::vector<size_t> unused_transfers_indices, std::vector<size_t> unused_dust_indices, const size_t fake_outs_count, const uint64_t unlock_time, uint32_t priority, const std::vector<uint8_t> extra, bool trusted_daemon)
{
  uint64_t accumulated_fee, accumulated_outputs, accumulated_change;
  struct TX {
    std::list<size_t> selected_transfers;
    std::vector<cryptonote::tx_destination_entry> dsts;
    cryptonote::transaction tx;
    pending_tx ptx;
    size_t bytes;
  };
  std::vector<TX> txes;
  uint64_t needed_fee, available_for_fee = 0;
  uint64_t upper_transaction_size_limit = get_upper_transaction_size_limit();
  std::vector<std::vector<get_outs_entry>> outs;

  const bool use_rct = fake_outs_count > 0 && use_fork_rules(4, 0);
  const uint64_t fee_per_kb  = get_per_kb_fee();
  const uint64_t fee_multiplier = get_fee_multiplier(priority, get_fee_algorithm());

  LOG_PRINT_L2("Starting with " << unused_transfers_indices.size() << " non-dust outputs and " << unused_dust_indices.size() << " dust outputs");

  if (unused_dust_indices.empty() && unused_transfers_indices.empty())
    return std::vector<wallet2::pending_tx>();

  // start with an empty tx
  txes.push_back(TX());
  accumulated_fee = 0;
  accumulated_outputs = 0;
  accumulated_change = 0;
  needed_fee = 0;

  // while we have something to send
  while (!unused_dust_indices.empty() || !unused_transfers_indices.empty()) {
    TX &tx = txes.back();

    // get a random unspent output and use it to pay next chunk. We try to alternate
    // dust and non dust to ensure we never get with only dust, from which we might
    // get a tx that can't pay for itself
    size_t idx = unused_transfers_indices.empty() ? pop_best_value(unused_dust_indices, tx.selected_transfers) : unused_dust_indices.empty() ? pop_best_value(unused_transfers_indices, tx.selected_transfers) : ((tx.selected_transfers.size() & 1) || accumulated_outputs > fee_per_kb * fee_multiplier * (upper_transaction_size_limit + 1023) / 1024) ? pop_best_value(unused_dust_indices, tx.selected_transfers) : pop_best_value(unused_transfers_indices, tx.selected_transfers);

    const transfer_details &td = m_transfers[idx];
    LOG_PRINT_L2("Picking output " << idx << ", amount " << print_money(td.amount()));

    // add this output to the list to spend
    tx.selected_transfers.push_back(idx);
    uint64_t available_amount = td.amount();
    accumulated_outputs += available_amount;

    // clear any fake outs we'd already gathered, since we'll need a new set
    outs.clear();

    // here, check if we need to sent tx and start a new one
    LOG_PRINT_L2("Considering whether to create a tx now, " << tx.selected_transfers.size() << " inputs, tx limit "
      << upper_transaction_size_limit);
    const size_t estimated_rct_tx_size = estimate_tx_size(use_rct, tx.selected_transfers.size(), fake_outs_count, tx.dsts.size() + 1);
    bool try_tx = (unused_dust_indices.empty() && unused_transfers_indices.empty()) || ( estimated_rct_tx_size >= TX_SIZE_TARGET(upper_transaction_size_limit));

    if (try_tx) {
      cryptonote::transaction test_tx;
      pending_tx test_ptx;

      needed_fee = 0;

      tx.dsts.push_back(tx_destination_entry(1, address));

      LOG_PRINT_L2("Trying to create a tx now, with " << tx.dsts.size() << " destinations and " <<
        tx.selected_transfers.size() << " outputs");
      if (use_rct)
        transfer_selected_rct(tx.dsts, tx.selected_transfers, fake_outs_count, outs, unlock_time, needed_fee, extra,
          test_tx, test_ptx);
      else
        transfer_selected(tx.dsts, tx.selected_transfers, fake_outs_count, outs, unlock_time, needed_fee, extra,
          detail::digit_split_strategy, tx_dust_policy(::config::DEFAULT_DUST_THRESHOLD), test_tx, test_ptx);
      auto txBlob = t_serializable_object_to_blob(test_ptx.tx);
      needed_fee = calculate_fee(fee_per_kb, txBlob, fee_multiplier);
      available_for_fee = test_ptx.fee + test_ptx.dests[0].amount + test_ptx.change_dts.amount;
      LOG_PRINT_L2("Made a " << ((txBlob.size() + 1023) / 1024) << " kB tx, with " << print_money(available_for_fee) << " available for fee (" <<
        print_money(needed_fee) << " needed)");

      THROW_WALLET_EXCEPTION_IF(needed_fee > available_for_fee, error::wallet_internal_error, "Transaction cannot pay for itself");

      do {
        LOG_PRINT_L2("We made a tx, adjusting fee and saving it");
        tx.dsts[0].amount = available_for_fee - needed_fee;
        if (use_rct)
          transfer_selected_rct(tx.dsts, tx.selected_transfers, fake_outs_count, outs, unlock_time, needed_fee, extra,
            test_tx, test_ptx);
        else
          transfer_selected(tx.dsts, tx.selected_transfers, fake_outs_count, outs, unlock_time, needed_fee, extra,
            detail::digit_split_strategy, tx_dust_policy(::config::DEFAULT_DUST_THRESHOLD), test_tx, test_ptx);
        txBlob = t_serializable_object_to_blob(test_ptx.tx);
        needed_fee = calculate_fee(fee_per_kb, txBlob, fee_multiplier);
        LOG_PRINT_L2("Made an attempt at a final " << ((txBlob.size() + 1023)/1024) << " kB tx, with " << print_money(test_ptx.fee) <<
          " fee  and " << print_money(test_ptx.change_dts.amount) << " change");
      } while (needed_fee > test_ptx.fee);

      LOG_PRINT_L2("Made a final " << ((txBlob.size() + 1023)/1024) << " kB tx, with " << print_money(test_ptx.fee) <<
        " fee  and " << print_money(test_ptx.change_dts.amount) << " change");

      tx.tx = test_tx;
      tx.ptx = test_ptx;
      tx.bytes = txBlob.size();
      accumulated_fee += test_ptx.fee;
      accumulated_change += test_ptx.change_dts.amount;
      if (!unused_transfers_indices.empty() || !unused_dust_indices.empty())
      {
        LOG_PRINT_L2("We have more to pay, starting another tx");
        txes.push_back(TX());
      }
    }
  }

  LOG_PRINT_L1("Done creating " << txes.size() << " transactions, " << print_money(accumulated_fee) <<
    " total fee, " << print_money(accumulated_change) << " total change");

  std::vector<wallet2::pending_tx> ptx_vector;
  for (std::vector<TX>::iterator i = txes.begin(); i != txes.end(); ++i)
  {
    TX &tx = *i;
    uint64_t tx_money = 0;
    for (size_t idx: tx.selected_transfers)
      tx_money += m_transfers[idx].amount();
    LOG_PRINT_L1("  Transaction " << (1+std::distance(txes.begin(), i)) << "/" << txes.size() <<
      ": " << (tx.bytes+1023)/1024 << " kB, sending " << print_money(tx_money) << " in " << tx.selected_transfers.size() <<
      " outputs to " << tx.dsts.size() << " destination(s), including " <<
      print_money(tx.ptx.fee) << " fee, " << print_money(tx.ptx.change_dts.amount) << " change");
    ptx_vector.push_back(tx.ptx);
  }

  // if we made it this far, we're OK to actually send the transactions
  return ptx_vector;
}

uint64_t wallet2::unlocked_dust_balance(const tx_dust_policy &dust_policy) const
{
  uint64_t money = 0;
  std::list<transfer_container::iterator> selected_transfers;
  for (transfer_container::const_iterator i = m_transfers.begin(); i != m_transfers.end(); ++i)
  {
    const transfer_details& td = *i;
    if (!td.m_spent && td.amount() < dust_policy.dust_threshold && is_transfer_unlocked(td))
    {
      money += td.amount();
    }
  }
  return money;
}
//----------------------------------------------------------------------------------------------------
void wallet2::get_hard_fork_info(uint8_t version, uint64_t &earliest_height)
{
  boost::optional<std::string> result = m_node_rpc_proxy.get_earliest_height(version, earliest_height);
  throw_on_rpc_response_error(result, "get_hard_fork_info");
}
//----------------------------------------------------------------------------------------------------
bool wallet2::use_fork_rules(uint8_t version, int64_t early_blocks)
{
  uint64_t height, earliest_height;
  boost::optional<std::string> result = m_node_rpc_proxy.get_height(height);
  throw_on_rpc_response_error(result, "get_info");
  result = m_node_rpc_proxy.get_earliest_height(version, earliest_height);
  throw_on_rpc_response_error(result, "get_hard_fork_info");

  bool close_enough = height >=  earliest_height - early_blocks; // start using the rules that many blocks beforehand
  if (close_enough)
    LOG_PRINT_L2("Using v" << (unsigned)version << " rules");
  else
    LOG_PRINT_L2("Not using v" << (unsigned)version << " rules");
  return close_enough;
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::get_upper_transaction_size_limit()
{
  if (m_upper_transaction_size_limit > 0)
    return m_upper_transaction_size_limit;
  uint64_t full_reward_zone = use_fork_rules(5, 10) ? CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V5 : use_fork_rules(2, 10) ? CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 : CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1;
  return full_reward_zone - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE;
}
//----------------------------------------------------------------------------------------------------
std::vector<size_t> wallet2::select_available_outputs(const std::function<bool(const transfer_details &td)> &f)
{
  std::vector<size_t> outputs;
  size_t n = 0;
  for (transfer_container::const_iterator i = m_transfers.begin(); i != m_transfers.end(); ++i, ++n)
  {
    if (i->m_spent)
      continue;
    if (!is_transfer_unlocked(*i))
      continue;
    if (f(*i))
      outputs.push_back(n);
  }
  return outputs;
}
//----------------------------------------------------------------------------------------------------
std::vector<uint64_t> wallet2::get_unspent_amounts_vector()
{
  std::set<uint64_t> set;
  for (const auto &td: m_transfers)
  {
    if (!td.m_spent)
      set.insert(td.is_rct() ? 0 : td.amount());
  }
  std::vector<uint64_t> vector;
  vector.reserve(set.size());
  for (const auto &i: set)
  {
    vector.push_back(i);
  }
  return vector;
}
//----------------------------------------------------------------------------------------------------
std::vector<size_t> wallet2::select_available_outputs_from_histogram(uint64_t count, bool atleast, bool unlocked, bool allow_rct, bool trusted_daemon)
{
  epee::json_rpc::request<cryptonote::COMMAND_RPC_GET_OUTPUT_HISTOGRAM::request> req_t = AUTO_VAL_INIT(req_t);
  epee::json_rpc::response<cryptonote::COMMAND_RPC_GET_OUTPUT_HISTOGRAM::response, std::string> resp_t = AUTO_VAL_INIT(resp_t);
  m_daemon_rpc_mutex.lock();
  req_t.jsonrpc = "2.0";
  req_t.id = epee::serialization::storage_entry(0);
  req_t.method = "get_output_histogram";
  if (trusted_daemon)
    req_t.params.amounts = get_unspent_amounts_vector();
  req_t.params.min_count = count;
  req_t.params.max_count = 0;
  req_t.params.unlocked = unlocked;
  bool r = net_utils::invoke_http_json("/json_rpc", req_t, resp_t, m_http_client, rpc_timeout);
  m_daemon_rpc_mutex.unlock();
  THROW_WALLET_EXCEPTION_IF(!r, error::no_connection_to_daemon, "select_available_outputs_from_histogram");
  THROW_WALLET_EXCEPTION_IF(resp_t.result.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "get_output_histogram");
  THROW_WALLET_EXCEPTION_IF(resp_t.result.status != CORE_RPC_STATUS_OK, error::get_histogram_error, resp_t.result.status);

  std::set<uint64_t> mixable;
  for (const auto &i: resp_t.result.histogram)
  {
    mixable.insert(i.amount);
  }

  return select_available_outputs([mixable, atleast, allow_rct](const transfer_details &td) {
    if (!allow_rct && td.is_rct())
      return false;
    const uint64_t amount = td.is_rct() ? 0 : td.amount();
    if (atleast) {
      if (mixable.find(amount) != mixable.end())
        return true;
    }
    else {
      if (mixable.find(amount) == mixable.end())
        return true;
    }
    return false;
  });
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::get_num_rct_outputs()
{
  epee::json_rpc::request<cryptonote::COMMAND_RPC_GET_OUTPUT_HISTOGRAM::request> req_t = AUTO_VAL_INIT(req_t);
  epee::json_rpc::response<cryptonote::COMMAND_RPC_GET_OUTPUT_HISTOGRAM::response, std::string> resp_t = AUTO_VAL_INIT(resp_t);
  m_daemon_rpc_mutex.lock();
  req_t.jsonrpc = "2.0";
  req_t.id = epee::serialization::storage_entry(0);
  req_t.method = "get_output_histogram";
  req_t.params.amounts.push_back(0);
  req_t.params.min_count = 0;
  req_t.params.max_count = 0;
  bool r = net_utils::invoke_http_json("/json_rpc", req_t, resp_t, m_http_client, rpc_timeout);
  m_daemon_rpc_mutex.unlock();
  THROW_WALLET_EXCEPTION_IF(!r, error::no_connection_to_daemon, "get_num_rct_outputs");
  THROW_WALLET_EXCEPTION_IF(resp_t.result.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "get_output_histogram");
  THROW_WALLET_EXCEPTION_IF(resp_t.result.status != CORE_RPC_STATUS_OK, error::get_histogram_error, resp_t.result.status);
  THROW_WALLET_EXCEPTION_IF(resp_t.result.histogram.size() != 1, error::get_histogram_error, "Expected exactly one response");
  THROW_WALLET_EXCEPTION_IF(resp_t.result.histogram[0].amount != 0, error::get_histogram_error, "Expected 0 amount");

  return resp_t.result.histogram[0].total_instances;
}
//----------------------------------------------------------------------------------------------------
const wallet2::transfer_details &wallet2::get_transfer_details(size_t idx) const
{
  THROW_WALLET_EXCEPTION_IF(idx >= m_transfers.size(), error::wallet_internal_error, "Bad transfer index");
  return m_transfers[idx];
}
//----------------------------------------------------------------------------------------------------
std::vector<size_t> wallet2::select_available_unmixable_outputs(bool trusted_daemon)
{
  // request all outputs with less than 3 instances
  const size_t min_mixin = use_fork_rules(6, 10) ? 4 : 2; // v6 increases min mixin from 2 to 4
  return select_available_outputs_from_histogram(min_mixin + 1, false, true, false, trusted_daemon);
}
//----------------------------------------------------------------------------------------------------
std::vector<size_t> wallet2::select_available_mixable_outputs(bool trusted_daemon)
{
  // request all outputs with at least 3 instances, so we can use mixin 2 with
  const size_t min_mixin = use_fork_rules(6, 10) ? 4 : 2; // v6 increases min mixin from 2 to 4
  return select_available_outputs_from_histogram(min_mixin + 1, true, true, true, trusted_daemon);
}
//----------------------------------------------------------------------------------------------------
std::vector<wallet2::pending_tx> wallet2::create_unmixable_sweep_transactions(bool trusted_daemon)
{
  // From hard fork 1, we don't consider small amounts to be dust anymore
  const bool hf1_rules = use_fork_rules(2, 10); // first hard fork has version 2
  tx_dust_policy dust_policy(hf1_rules ? 0 : ::config::DEFAULT_DUST_THRESHOLD);

  const uint64_t fee_per_kb  = get_per_kb_fee();

  // may throw
  std::vector<size_t> unmixable_outputs = select_available_unmixable_outputs(trusted_daemon);
  size_t num_dust_outputs = unmixable_outputs.size();

  if (num_dust_outputs == 0)
  {
    return std::vector<wallet2::pending_tx>();
  }

  // split in "dust" and "non dust" to make it easier to select outputs
  std::vector<size_t> unmixable_transfer_outputs, unmixable_dust_outputs;
  for (auto n: unmixable_outputs)
  {
    if (m_transfers[n].amount() < fee_per_kb)
      unmixable_dust_outputs.push_back(n);
    else
      unmixable_transfer_outputs.push_back(n);
  }

  return create_transactions_from(m_account_public_address, unmixable_transfer_outputs, unmixable_dust_outputs, 0 /*fake_outs_count */, 0 /* unlock_time */, 1 /*priority */, std::vector<uint8_t>(), trusted_daemon);
}

bool wallet2::get_tx_key(const crypto::hash &txid, crypto::secret_key &tx_key) const
{
  const std::unordered_map<crypto::hash, crypto::secret_key>::const_iterator i = m_tx_keys.find(txid);
  if (i == m_tx_keys.end())
    return false;
  tx_key = i->second;
  return true;
}

std::string wallet2::get_wallet_file() const
{
  return m_wallet_file;
}

std::string wallet2::get_keys_file() const
{
  return m_keys_file;
}

std::string wallet2::get_daemon_address() const
{
  return m_daemon_address;
}

uint64_t wallet2::get_daemon_blockchain_height(string &err)
{
  uint64_t height;

  boost::optional<std::string> result = m_node_rpc_proxy.get_height(height);
  if (result)
  {
    err = *result;
    return 0;
  }

  err = "";
  return height;
}

uint64_t wallet2::get_daemon_blockchain_target_height(string &err)
{
  epee::json_rpc::request<cryptonote::COMMAND_RPC_GET_INFO::request> req_t = AUTO_VAL_INIT(req_t);
  epee::json_rpc::response<cryptonote::COMMAND_RPC_GET_INFO::response, std::string> resp_t = AUTO_VAL_INIT(resp_t);
  m_daemon_rpc_mutex.lock();
  req_t.jsonrpc = "2.0";
  req_t.id = epee::serialization::storage_entry(0);
  req_t.method = "get_info";
  bool ok = net_utils::invoke_http_json("/json_rpc", req_t, resp_t, m_http_client);
  m_daemon_rpc_mutex.unlock();
  if (ok)
  {
    if (resp_t.result.status == CORE_RPC_STATUS_BUSY)
    {
      err = "daemon is busy. Please try again later.";
    }
    else if (resp_t.result.status != CORE_RPC_STATUS_OK)
    {
      err = resp_t.result.status;
    }
    else // success, cleaning up error message
    {
      err = "";
    }
  }
  else
  {
    err = "possibly lost connection to daemon";
  }
  return resp_t.result.target_height;
}

uint64_t wallet2::get_approximate_blockchain_height() const
{
  // time of v2 fork
  const time_t fork_time = m_testnet ? 1448285909 : 1458748658;
  // v2 fork block
  const uint64_t fork_block = m_testnet ? 624634 : 1009827;
  // avg seconds per block
  const int seconds_per_block = DIFFICULTY_TARGET_V2;
  // Calculated blockchain height
  uint64_t approx_blockchain_height = fork_block + (time(NULL) - fork_time)/seconds_per_block;
  LOG_PRINT_L2("Calculated blockchain height: " << approx_blockchain_height);
  return approx_blockchain_height;
}

void wallet2::set_tx_note(const crypto::hash &txid, const std::string &note)
{
  m_tx_notes[txid] = note;
}

std::string wallet2::get_tx_note(const crypto::hash &txid) const
{
  std::unordered_map<crypto::hash, std::string>::const_iterator i = m_tx_notes.find(txid);
  if (i == m_tx_notes.end())
    return std::string();
  return i->second;
}

std::string wallet2::sign(const std::string &data) const
{
  crypto::hash hash;
  crypto::cn_fast_hash(data.data(), data.size(), hash);
  const cryptonote::account_keys &keys = m_account.get_keys();
  crypto::signature signature;
  crypto::generate_signature(hash, keys.m_account_address.m_spend_public_key, keys.m_spend_secret_key, signature);
  return std::string("SigV1") + tools::base58::encode(std::string((const char *)&signature, sizeof(signature)));
}

bool wallet2::verify(const std::string &data, const cryptonote::account_public_address &address, const std::string &signature) const
{
  const size_t header_len = strlen("SigV1");
  if (signature.size() < header_len || signature.substr(0, header_len) != "SigV1") {
    LOG_PRINT_L0("Signature header check error");
    return false;
  }
  crypto::hash hash;
  crypto::cn_fast_hash(data.data(), data.size(), hash);
  std::string decoded;
  if (!tools::base58::decode(signature.substr(header_len), decoded)) {
    LOG_PRINT_L0("Signature decoding error");
    return false;
  }
  crypto::signature s;
  if (sizeof(s) != decoded.size()) {
    LOG_PRINT_L0("Signature decoding error");
    return false;
  }
  memcpy(&s, decoded.data(), sizeof(s));
  return crypto::check_signature(hash, address.m_spend_public_key, s);
}
//----------------------------------------------------------------------------------------------------
crypto::public_key wallet2::get_tx_pub_key_from_received_outs(const tools::wallet2::transfer_details &td) const
{
  std::vector<tx_extra_field> tx_extra_fields;
  if(!parse_tx_extra(td.m_tx.extra, tx_extra_fields))
  {
    // Extra may only be partially parsed, it's OK if tx_extra_fields contains public key
  }

  // Due to a previous bug, there might be more than one tx pubkey in extra, one being
  // the result of a previously discarded signature.
  // For speed, since scanning for outputs is a slow process, we check whether extra
  // contains more than one pubkey. If not, the first one is returned. If yes, they're
  // checked for whether they yield at least one output
  tx_extra_pub_key pub_key_field;
  THROW_WALLET_EXCEPTION_IF(!find_tx_extra_field_by_type(tx_extra_fields, pub_key_field, 0), error::wallet_internal_error,
      "Public key wasn't found in the transaction extra");
  const crypto::public_key tx_pub_key = pub_key_field.pub_key;
  bool two_found = find_tx_extra_field_by_type(tx_extra_fields, pub_key_field, 1);
  if (!two_found) {
    // easy case, just one found
    return tx_pub_key;
  }

  // more than one, loop and search
  const cryptonote::account_keys& keys = m_account.get_keys();
  size_t pk_index = 0;
  while (find_tx_extra_field_by_type(tx_extra_fields, pub_key_field, pk_index++)) {
    const crypto::public_key tx_pub_key = pub_key_field.pub_key;
    crypto::key_derivation derivation;
    generate_key_derivation(tx_pub_key, keys.m_view_secret_key, derivation);

    for (size_t i = 0; i < td.m_tx.vout.size(); ++i)
    {
      uint64_t money_transfered = 0;
      bool error = false, received = false;
      check_acc_out_precomp(keys.m_account_address.m_spend_public_key, td.m_tx.vout[i], derivation, i, received, money_transfered, error);
      if (!error && received)
        return tx_pub_key;
    }
  }

  // we found no key yielding an output
  THROW_WALLET_EXCEPTION_IF(true, error::wallet_internal_error,
      "Public key yielding at least one output wasn't found in the transaction extra");
  return cryptonote::null_pkey;
}

bool wallet2::export_key_images(const std::string filename)
{
  std::vector<std::pair<crypto::key_image, crypto::signature>> ski = export_key_images();
  std::string magic(KEY_IMAGE_EXPORT_FILE_MAGIC, strlen(KEY_IMAGE_EXPORT_FILE_MAGIC));
  const cryptonote::account_public_address &keys = get_account().get_keys().m_account_address;

  std::string data;
  data += std::string((const char *)&keys.m_spend_public_key, sizeof(crypto::public_key));
  data += std::string((const char *)&keys.m_view_public_key, sizeof(crypto::public_key));
  for (const auto &i: ski)
  {
    data += std::string((const char *)&i.first, sizeof(crypto::key_image));
    data += std::string((const char *)&i.second, sizeof(crypto::signature));
  }

  // encrypt data, keep magic plaintext
  std::string ciphertext = encrypt_with_view_secret_key(data);
  return epee::file_io_utils::save_string_to_file(filename, magic + ciphertext);    
}

//----------------------------------------------------------------------------------------------------
std::vector<std::pair<crypto::key_image, crypto::signature>> wallet2::export_key_images() const
{
  std::vector<std::pair<crypto::key_image, crypto::signature>> ski;

  ski.reserve(m_transfers.size());
  for (size_t n = 0; n < m_transfers.size(); ++n)
  {
    const transfer_details &td = m_transfers[n];

    crypto::hash hash;
    crypto::cn_fast_hash(&td.m_key_image, sizeof(td.m_key_image), hash);

    // get ephemeral public key
    const cryptonote::tx_out &out = td.m_tx.vout[td.m_internal_output_index];
    THROW_WALLET_EXCEPTION_IF(out.target.type() != typeid(txout_to_key), error::wallet_internal_error,
        "Output is not txout_to_key");
    const cryptonote::txout_to_key &o = boost::get<const cryptonote::txout_to_key>(out.target);
    const crypto::public_key pkey = o.key;

    // get tx pub key
    std::vector<tx_extra_field> tx_extra_fields;
    if(!parse_tx_extra(td.m_tx.extra, tx_extra_fields))
    {
      // Extra may only be partially parsed, it's OK if tx_extra_fields contains public key
    }

    crypto::public_key tx_pub_key = get_tx_pub_key_from_received_outs(td);

    // generate ephemeral secret key
    crypto::key_image ki;
    cryptonote::keypair in_ephemeral;
    cryptonote::generate_key_image_helper(m_account.get_keys(), tx_pub_key, td.m_internal_output_index, in_ephemeral, ki);

    THROW_WALLET_EXCEPTION_IF(td.m_key_image_known && ki != td.m_key_image,
        error::wallet_internal_error, "key_image generated not matched with cached key image");
    THROW_WALLET_EXCEPTION_IF(in_ephemeral.pub != pkey,
        error::wallet_internal_error, "key_image generated ephemeral public key not matched with output_key");

    // sign the key image with the output secret key
    crypto::signature signature;
    std::vector<const crypto::public_key*> key_ptrs;
    key_ptrs.push_back(&pkey);

    crypto::generate_ring_signature((const crypto::hash&)td.m_key_image, td.m_key_image, key_ptrs, in_ephemeral.sec, 0, &signature);

    ski.push_back(std::make_pair(td.m_key_image, signature));
  }
  return ski;
}

uint64_t wallet2::import_key_images(const std::string &filename, uint64_t &spent, uint64_t &unspent)
{
  std::string data;
  bool r = epee::file_io_utils::load_file_to_string(filename, data);

  if (!r)
  {
    fail_msg_writer() << tr("failed to read file ") << filename;
    return 0;
  }
  const size_t magiclen = strlen(KEY_IMAGE_EXPORT_FILE_MAGIC);
  if (data.size() < magiclen || memcmp(data.data(), KEY_IMAGE_EXPORT_FILE_MAGIC, magiclen))
  {
    fail_msg_writer() << "Bad key image export file magic in " << filename;
    return 0;
  }

  try
  {
    data = decrypt_with_view_secret_key(std::string(data, magiclen));
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << "Failed to decrypt " << filename << ": " << e.what();
    return 0;
  }

  const size_t headerlen = 2 * sizeof(crypto::public_key);
  if (data.size() < headerlen)
  {
    fail_msg_writer() << "Bad data size from file " << filename;
    return 0;
  }
  const crypto::public_key &public_spend_key = *(const crypto::public_key*)&data[0];
  const crypto::public_key &public_view_key = *(const crypto::public_key*)&data[sizeof(crypto::public_key)];
  const cryptonote::account_public_address &keys = get_account().get_keys().m_account_address;
  if (public_spend_key != keys.m_spend_public_key || public_view_key != keys.m_view_public_key)
  {
    fail_msg_writer() << "Key images from " << filename << " are for a different account";
    return 0;
  }

  const size_t record_size = sizeof(crypto::key_image) + sizeof(crypto::signature);
  if ((data.size() - headerlen) % record_size)
  {
    fail_msg_writer() << "Bad data size from file " << filename;
    return 0;
  }
  size_t nki = (data.size() - headerlen) / record_size;

  std::vector<std::pair<crypto::key_image, crypto::signature>> ski;
  ski.reserve(nki);
  for (size_t n = 0; n < nki; ++n)
  {
    crypto::key_image key_image = *reinterpret_cast<const crypto::key_image*>(&data[headerlen + n * record_size]);
    crypto::signature signature = *reinterpret_cast<const crypto::signature*>(&data[headerlen + n * record_size + sizeof(crypto::key_image)]);

    ski.push_back(std::make_pair(key_image, signature));
  }
  
  return import_key_images(ski, spent, unspent);    
}

//----------------------------------------------------------------------------------------------------
uint64_t wallet2::import_key_images(const std::vector<std::pair<crypto::key_image, crypto::signature>> &signed_key_images, uint64_t &spent, uint64_t &unspent)
{
  COMMAND_RPC_IS_KEY_IMAGE_SPENT::request req = AUTO_VAL_INIT(req);
  COMMAND_RPC_IS_KEY_IMAGE_SPENT::response daemon_resp = AUTO_VAL_INIT(daemon_resp);

  THROW_WALLET_EXCEPTION_IF(signed_key_images.size() > m_transfers.size(), error::wallet_internal_error,
      "The blockchain is out of date compared to the signed key images");

  if (signed_key_images.empty())
  {
    spent = 0;
    unspent = 0;
    return 0;
  }

  for (size_t n = 0; n < signed_key_images.size(); ++n)
  {
    const transfer_details &td = m_transfers[n];
    const crypto::key_image &key_image = signed_key_images[n].first;
    const crypto::signature &signature = signed_key_images[n].second;

    // get ephemeral public key
    const cryptonote::tx_out &out = td.m_tx.vout[td.m_internal_output_index];
    THROW_WALLET_EXCEPTION_IF(out.target.type() != typeid(txout_to_key), error::wallet_internal_error,
      "Non txout_to_key output found");
    const cryptonote::txout_to_key &o = boost::get<cryptonote::txout_to_key>(out.target);
    const crypto::public_key pkey = o.key;

    std::vector<const crypto::public_key*> pkeys;
    pkeys.push_back(&pkey);
    THROW_WALLET_EXCEPTION_IF(!(rct::scalarmultKey(rct::ki2rct(key_image), rct::curveOrder()) == rct::identity()),
        error::wallet_internal_error, "Key image out of validity domain: input " + boost::lexical_cast<std::string>(n) + "/"
        + boost::lexical_cast<std::string>(signed_key_images.size()) + ", key image " + epee::string_tools::pod_to_hex(key_image));

    THROW_WALLET_EXCEPTION_IF(!crypto::check_ring_signature((const crypto::hash&)key_image, key_image, pkeys, &signature),
        error::wallet_internal_error, "Signature check failed: input " + boost::lexical_cast<std::string>(n) + "/"
        + boost::lexical_cast<std::string>(signed_key_images.size()) + ", key image " + epee::string_tools::pod_to_hex(key_image)
        + ", signature " + epee::string_tools::pod_to_hex(signature) + ", pubkey " + epee::string_tools::pod_to_hex(*pkeys[0]));

    req.key_images.push_back(epee::string_tools::pod_to_hex(key_image));
  }

  for (size_t n = 0; n < signed_key_images.size(); ++n)
  {
    m_transfers[n].m_key_image = signed_key_images[n].first;
    m_key_images[m_transfers[n].m_key_image] = n;
    m_transfers[n].m_key_image_known = true;
  }

  m_daemon_rpc_mutex.lock();
  bool r = epee::net_utils::invoke_http_json("/is_key_image_spent", req, daemon_resp, m_http_client, rpc_timeout);
  m_daemon_rpc_mutex.unlock();
  THROW_WALLET_EXCEPTION_IF(!r, error::no_connection_to_daemon, "is_key_image_spent");
  THROW_WALLET_EXCEPTION_IF(daemon_resp.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "is_key_image_spent");
  THROW_WALLET_EXCEPTION_IF(daemon_resp.status != CORE_RPC_STATUS_OK, error::is_key_image_spent_error, daemon_resp.status);
  THROW_WALLET_EXCEPTION_IF(daemon_resp.spent_status.size() != signed_key_images.size(), error::wallet_internal_error,
    "daemon returned wrong response for is_key_image_spent, wrong amounts count = " +
    std::to_string(daemon_resp.spent_status.size()) + ", expected " +  std::to_string(signed_key_images.size()));

  spent = 0;
  unspent = 0;
  for (size_t n = 0; n < daemon_resp.spent_status.size(); ++n)
  {
    transfer_details &td = m_transfers[n];
    uint64_t amount = td.amount();
    td.m_spent = daemon_resp.spent_status[n] != COMMAND_RPC_IS_KEY_IMAGE_SPENT::UNSPENT;
    if (td.m_spent)
      spent += amount;
    else
      unspent += amount;
    LOG_PRINT_L2("Transfer " << n << ": " << print_money(amount) << " (" << td.m_global_output_index << "): "
        << (td.m_spent ? "spent" : "unspent") << " (key image " << req.key_images[n] << ")");
  }
  LOG_PRINT_L1("Total: " << print_money(spent) << " spent, " << print_money(unspent) << " unspent");

  return m_transfers[signed_key_images.size() - 1].m_block_height;
}
//----------------------------------------------------------------------------------------------------
std::vector<tools::wallet2::transfer_details> wallet2::export_outputs() const
{
  std::vector<tools::wallet2::transfer_details> outs;

  outs.reserve(m_transfers.size());
  for (size_t n = 0; n < m_transfers.size(); ++n)
  {
    const transfer_details &td = m_transfers[n];

    outs.push_back(td);
  }

  return outs;
}
//----------------------------------------------------------------------------------------------------
size_t wallet2::import_outputs(const std::vector<tools::wallet2::transfer_details> &outputs)
{
  m_transfers.clear();
  m_transfers.reserve(outputs.size());
  for (size_t i = 0; i < outputs.size(); ++i)
  {
    transfer_details td = outputs[i];

    // the hot wallet wouldn't have known about key images (except if we already exported them)
    cryptonote::keypair in_ephemeral;
    std::vector<tx_extra_field> tx_extra_fields;
    tx_extra_pub_key pub_key_field;

    THROW_WALLET_EXCEPTION_IF(td.m_tx.vout.empty(), error::wallet_internal_error, "tx with no outputs at index " + boost::lexical_cast<std::string>(i));
    THROW_WALLET_EXCEPTION_IF(!parse_tx_extra(td.m_tx.extra, tx_extra_fields), error::wallet_internal_error,
        "Transaction extra has unsupported format at index " + boost::lexical_cast<std::string>(i));
    crypto::public_key tx_pub_key = get_tx_pub_key_from_received_outs(td);

    cryptonote::generate_key_image_helper(m_account.get_keys(), tx_pub_key, td.m_internal_output_index, in_ephemeral, td.m_key_image);
    td.m_key_image_known = true;
    THROW_WALLET_EXCEPTION_IF(in_ephemeral.pub != boost::get<cryptonote::txout_to_key>(td.m_tx.vout[td.m_internal_output_index].target).key,
        error::wallet_internal_error, "key_image generated ephemeral public key not matched with output_key at index " + boost::lexical_cast<std::string>(i));

    m_key_images[td.m_key_image] = m_transfers.size();
    m_pub_keys[td.get_public_key()] = m_transfers.size();
    m_transfers.push_back(td);
  }

  return m_transfers.size();
}
//----------------------------------------------------------------------------------------------------
std::string wallet2::encrypt(const std::string &plaintext, const crypto::secret_key &skey, bool authenticated) const
{
  crypto::chacha8_key key;
  crypto::generate_chacha8_key(&skey, sizeof(skey), key);
  std::string ciphertext;
  crypto::chacha8_iv iv = crypto::rand<crypto::chacha8_iv>();
  ciphertext.resize(plaintext.size() + sizeof(iv) + (authenticated ? sizeof(crypto::signature) : 0));
  crypto::chacha8(plaintext.data(), plaintext.size(), key, iv, &ciphertext[sizeof(iv)]);
  memcpy(&ciphertext[0], &iv, sizeof(iv));
  if (authenticated)
  {
    crypto::hash hash;
    crypto::cn_fast_hash(ciphertext.data(), ciphertext.size() - sizeof(signature), hash);
    crypto::public_key pkey;
    crypto::secret_key_to_public_key(skey, pkey);
    crypto::signature &signature = *(crypto::signature*)&ciphertext[ciphertext.size() - sizeof(crypto::signature)];
    crypto::generate_signature(hash, pkey, skey, signature);
  }
  return ciphertext;
}
//----------------------------------------------------------------------------------------------------
std::string wallet2::encrypt_with_view_secret_key(const std::string &plaintext, bool authenticated) const
{
  return encrypt(plaintext, get_account().get_keys().m_view_secret_key, authenticated);
}
//----------------------------------------------------------------------------------------------------
std::string wallet2::decrypt(const std::string &ciphertext, const crypto::secret_key &skey, bool authenticated) const
{
  const size_t prefix_size = sizeof(chacha8_iv) + (authenticated ? sizeof(crypto::signature) : 0);
  THROW_WALLET_EXCEPTION_IF(ciphertext.size() < prefix_size,
    error::wallet_internal_error, "Unexpected ciphertext size");

  crypto::chacha8_key key;
  crypto::generate_chacha8_key(&skey, sizeof(skey), key);
  const crypto::chacha8_iv &iv = *(const crypto::chacha8_iv*)&ciphertext[0];
  std::string plaintext;
  plaintext.resize(ciphertext.size() - prefix_size);
  if (authenticated)
  {
    crypto::hash hash;
    crypto::cn_fast_hash(ciphertext.data(), ciphertext.size() - sizeof(signature), hash);
    crypto::public_key pkey;
    crypto::secret_key_to_public_key(skey, pkey);
    const crypto::signature &signature = *(const crypto::signature*)&ciphertext[ciphertext.size() - sizeof(crypto::signature)];
    THROW_WALLET_EXCEPTION_IF(!crypto::check_signature(hash, pkey, signature),
      error::wallet_internal_error, "Failed to authenticate criphertext");
  }
  crypto::chacha8(ciphertext.data() + sizeof(iv), ciphertext.size() - prefix_size, key, iv, &plaintext[0]);
  return plaintext;
}
//----------------------------------------------------------------------------------------------------
std::string wallet2::decrypt_with_view_secret_key(const std::string &ciphertext, bool authenticated) const
{
  return decrypt(ciphertext, get_account().get_keys().m_view_secret_key, authenticated);
}
//----------------------------------------------------------------------------------------------------
std::string wallet2::make_uri(const std::string &address, const std::string &payment_id, uint64_t amount, const std::string &tx_description, const std::string &recipient_name, std::string &error)
{
  cryptonote::account_public_address tmp_address;
  bool has_payment_id;
  crypto::hash8 new_payment_id;
  if(!get_account_integrated_address_from_str(tmp_address, has_payment_id, new_payment_id, testnet(), address))
  {
    error = std::string("wrong address: ") + address;
    return std::string();
  }

  // we want only one payment id
  if (has_payment_id && !payment_id.empty())
  {
    error = "A single payment id is allowed";
    return std::string();
  }

  if (!payment_id.empty())
  {
    crypto::hash pid32;
    crypto::hash8 pid8;
    if (!wallet2::parse_long_payment_id(payment_id, pid32) && !wallet2::parse_short_payment_id(payment_id, pid8))
    {
      error = "Invalid payment id";
      return std::string();
    }
  }

  std::string uri = "monero:" + address;
  unsigned int n_fields = 0;

  if (!payment_id.empty())
  {
    uri += (n_fields++ ? "&" : "?") + std::string("tx_payment_id=") + payment_id;
  }

  if (amount > 0)
  {
    // URI encoded amount is in decimal units, not atomic units
    uri += (n_fields++ ? "&" : "?") + std::string("tx_amount=") + cryptonote::print_money(amount);
  }

  if (!recipient_name.empty())
  {
    uri += (n_fields++ ? "&" : "?") + std::string("recipient_name=") + epee::net_utils::conver_to_url_format(recipient_name);
  }

  if (!tx_description.empty())
  {
    uri += (n_fields++ ? "&" : "?") + std::string("tx_description=") + epee::net_utils::conver_to_url_format(tx_description);
  }

  return uri;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::parse_uri(const std::string &uri, std::string &address, std::string &payment_id, uint64_t &amount, std::string &tx_description, std::string &recipient_name, std::vector<std::string> &unknown_parameters, std::string &error)
{
  if (uri.substr(0, 7) != "monero:")
  {
    error = std::string("URI has wrong scheme (expected \"monero:\"): ") + uri;
    return false;
  }

  std::string remainder = uri.substr(7);
  const char *ptr = strchr(remainder.c_str(), '?');
  address = ptr ? remainder.substr(0, ptr-remainder.c_str()) : remainder;

  cryptonote::account_public_address addr;
  bool has_payment_id;
  crypto::hash8 new_payment_id;
  if(!get_account_integrated_address_from_str(addr, has_payment_id, new_payment_id, testnet(), address))
  {
    error = std::string("URI has wrong address: ") + address;
    return false;
  }
  if (!strchr(remainder.c_str(), '?'))
    return true;

  std::vector<std::string> arguments;
  std::string body = remainder.substr(address.size() + 1);
  if (body.empty())
    return true;
  boost::split(arguments, body, boost::is_any_of("&"));
  std::set<std::string> have_arg;
  for (const auto &arg: arguments)
  {
    std::vector<std::string> kv;
    boost::split(kv, arg, boost::is_any_of("="));
    if (kv.size() != 2)
    {
      error = std::string("URI has wrong parameter: ") + arg;
      return false;
    }
    if (have_arg.find(kv[0]) != have_arg.end())
    {
      error = std::string("URI has more than one instance of " + kv[0]);
      return false;
    }
    have_arg.insert(kv[0]);

    if (kv[0] == "tx_amount")
    {
      amount = 0;
      if (!cryptonote::parse_amount(amount, kv[1]))
      {
        error = std::string("URI has invalid amount: ") + kv[1];
        return false;
      }
    }
    else if (kv[0] == "tx_payment_id")
    {
      if (has_payment_id)
      {
        error = "Separate payment id given with an integrated address";
        return false;
      }
      crypto::hash hash;
      crypto::hash8 hash8;
      if (!wallet2::parse_long_payment_id(kv[1], hash) && !wallet2::parse_short_payment_id(kv[1], hash8))
      {
        error = "Invalid payment id: " + kv[1];
        return false;
      }
      payment_id = kv[1];
    }
    else if (kv[0] == "recipient_name")
    {
      recipient_name = epee::net_utils::convert_from_url_format(kv[1]);
    }
    else if (kv[0] == "tx_description")
    {
      tx_description = epee::net_utils::convert_from_url_format(kv[1]);
    }
    else
    {
      unknown_parameters.push_back(arg);
    }
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::get_blockchain_height_by_date(uint16_t year, uint8_t month, uint8_t day)
{
  uint32_t version;
  if (!check_connection(&version))
  {
    throw std::runtime_error("failed to connect to daemon: " + get_daemon_address());
  }
  if (version < MAKE_CORE_RPC_VERSION(1, 6))
  {
    throw std::runtime_error("this function requires RPC version 1.6 or higher");
  }
  std::tm date = { 0, 0, 0, 0, 0, 0, 0, 0 };
  date.tm_year = year - 1900;
  date.tm_mon  = month - 1;
  date.tm_mday = day;
  if (date.tm_mon < 0 || 11 < date.tm_mon || date.tm_mday < 1 || 31 < date.tm_mday)
  {
    throw std::runtime_error("month or day out of range");
  }
  uint64_t timestamp_target = std::mktime(&date);
  std::string err;
  uint64_t height_min = 0;
  uint64_t height_max = get_daemon_blockchain_height(err) - 1;
  if (!err.empty())
  {
    throw std::runtime_error("failed to get blockchain height");
  }
  while (true)
  {
    COMMAND_RPC_GET_BLOCKS_BY_HEIGHT::request req;
    COMMAND_RPC_GET_BLOCKS_BY_HEIGHT::response res;
    uint64_t height_mid = (height_min + height_max) / 2;
    req.heights =
    {
      height_min,
      height_mid,
      height_max
    };
    bool r = net_utils::invoke_http_bin("/getblocks_by_height.bin", req, res, m_http_client, rpc_timeout);
    if (!r || res.status != CORE_RPC_STATUS_OK)
    {
      std::ostringstream oss;
      oss << "failed to get blocks by heights: ";
      for (auto height : req.heights)
        oss << height << ' ';
      oss << endl << "reason: ";
      if (!r)
        oss << "possibly lost connection to daemon";
      else if (res.status == CORE_RPC_STATUS_BUSY)
        oss << "daemon is busy";
      else
        oss << res.status;
      throw std::runtime_error(oss.str());
    }
    cryptonote::block blk_min, blk_mid, blk_max;
    if (!parse_and_validate_block_from_blob(res.blocks[0].block, blk_min)) throw std::runtime_error("failed to parse blob at height " + std::to_string(height_min));
    if (!parse_and_validate_block_from_blob(res.blocks[1].block, blk_mid)) throw std::runtime_error("failed to parse blob at height " + std::to_string(height_mid));
    if (!parse_and_validate_block_from_blob(res.blocks[2].block, blk_max)) throw std::runtime_error("failed to parse blob at height " + std::to_string(height_max));
    uint64_t timestamp_min = blk_min.timestamp;
    uint64_t timestamp_mid = blk_mid.timestamp;
    uint64_t timestamp_max = blk_max.timestamp;
    if (!(timestamp_min <= timestamp_mid && timestamp_mid <= timestamp_max))
    {
      // the timestamps are not in the chronological order. 
      // assuming they're sufficiently close to each other, simply return the smallest height
      return std::min({height_min, height_mid, height_max});
    }
    if (timestamp_target > timestamp_max)
    {
      throw std::runtime_error("specified date is in the future");
    }
    if (timestamp_target <= timestamp_min + 2 * 24 * 60 * 60)   // two days of "buffer" period
    {
      return height_min;
    }
    if (timestamp_target <= timestamp_mid)
      height_max = height_mid;
    else
      height_min = height_mid;
    if (height_max - height_min <= 2 * 24 * 30)        // don't divide the height range finer than two days
    {
      return height_min;
    }
  }
}
//----------------------------------------------------------------------------------------------------
bool wallet2::is_synced() const
{
  uint64_t height;
  boost::optional<std::string> result = m_node_rpc_proxy.get_target_height(height);
  if (result && *result != CORE_RPC_STATUS_OK)
    return false;
  return get_blockchain_current_height() >= height;
}
//----------------------------------------------------------------------------------------------------
std::vector<std::pair<uint64_t, uint64_t>> wallet2::estimate_backlog(uint64_t min_blob_size, uint64_t max_blob_size, const std::vector<uint64_t> &fees)
{
  THROW_WALLET_EXCEPTION_IF(min_blob_size == 0, error::wallet_internal_error, "Invalid 0 fee");
  THROW_WALLET_EXCEPTION_IF(max_blob_size == 0, error::wallet_internal_error, "Invalid 0 fee");
  for (uint64_t fee: fees)
  {
    THROW_WALLET_EXCEPTION_IF(fee == 0, error::wallet_internal_error, "Invalid 0 fee");
  }

  // get txpool backlog
  epee::json_rpc::request<cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL_BACKLOG::request> req = AUTO_VAL_INIT(req);
  epee::json_rpc::response<cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL_BACKLOG::response, std::string> res = AUTO_VAL_INIT(res);
  m_daemon_rpc_mutex.lock();
  req.jsonrpc = "2.0";
  req.id = epee::serialization::storage_entry(0);
  req.method = "get_txpool_backlog";
  bool r = net_utils::invoke_http_json("/json_rpc", req, res, m_http_client, rpc_timeout);
  m_daemon_rpc_mutex.unlock();
  THROW_WALLET_EXCEPTION_IF(!r, error::no_connection_to_daemon, "Failed to connect to daemon");
  THROW_WALLET_EXCEPTION_IF(res.result.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "get_txpool_backlog");
  THROW_WALLET_EXCEPTION_IF(res.result.status != CORE_RPC_STATUS_OK, error::get_tx_pool_error);

  epee::json_rpc::request<cryptonote::COMMAND_RPC_GET_INFO::request> req_t = AUTO_VAL_INIT(req_t);
  epee::json_rpc::response<cryptonote::COMMAND_RPC_GET_INFO::response, std::string> resp_t = AUTO_VAL_INIT(resp_t);
  m_daemon_rpc_mutex.lock();
  req_t.jsonrpc = "2.0";
  req_t.id = epee::serialization::storage_entry(0);
  req_t.method = "get_info";
  r = net_utils::invoke_http_json("/json_rpc", req_t, resp_t, m_http_client);
  m_daemon_rpc_mutex.unlock();
  THROW_WALLET_EXCEPTION_IF(!r, error::no_connection_to_daemon, "get_info");
  THROW_WALLET_EXCEPTION_IF(resp_t.result.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "get_info");
  THROW_WALLET_EXCEPTION_IF(resp_t.result.status != CORE_RPC_STATUS_OK, error::get_tx_pool_error);
  uint64_t full_reward_zone = resp_t.result.block_size_limit / 2;

  std::vector<std::pair<uint64_t, uint64_t>> blocks;
  for (uint64_t fee: fees)
  {
    double our_fee_byte_min = fee / (double)min_blob_size, our_fee_byte_max = fee / (double)max_blob_size;
    uint64_t priority_size_min = 0, priority_size_max = 0;
    for (const auto &i: res.result.backlog)
    {
      if (i.blob_size == 0)
      {
        MWARNING("Got 0 sized blob from txpool, ignored");
        continue;
      }
      double this_fee_byte = i.fee / (double)i.blob_size;
      if (this_fee_byte >= our_fee_byte_min)
        priority_size_min += i.blob_size;
      if (this_fee_byte >= our_fee_byte_max)
        priority_size_max += i.blob_size;
    }

    uint64_t nblocks_min = (priority_size_min + full_reward_zone - 1) / full_reward_zone;
    uint64_t nblocks_max = (priority_size_max + full_reward_zone - 1) / full_reward_zone;
    MDEBUG("estimate_backlog: priority_size " << priority_size_min << " - " << priority_size_max << " for " << fee
        << " (" << our_fee_byte_min << " - " << our_fee_byte_max << " piconero byte fee), "
        << nblocks_min << " - " << nblocks_max << " blocks at block size " << full_reward_zone);
    blocks.push_back(std::make_pair(nblocks_min, nblocks_max));
  }
  return blocks;
}
//----------------------------------------------------------------------------------------------------
void wallet2::generate_genesis(cryptonote::block& b) {
  if (m_testnet)
  {
    cryptonote::generate_genesis_block(b, config::testnet::GENESIS_TX, config::testnet::GENESIS_NONCE);
  }
  else
  {
    cryptonote::generate_genesis_block(b, config::GENESIS_TX, config::GENESIS_NONCE);
  }
}
}
