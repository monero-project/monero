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

#pragma once

#include <stdexcept>
#include <system_error>
#include <string>
#include <vector>

#include "cryptonote_basic/cryptonote_format_utils.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "include_base_utils.h"


namespace tools
{
  namespace error
  {
    // std::exception
    //   std::runtime_error
    //     wallet_runtime_error *
    //       wallet_internal_error
    //         unexpected_txin_type
    //         wallet_not_initialized
    //   std::logic_error
    //     wallet_logic_error *
    //       file_exists
    //       file_not_found
    //       file_read_error
    //       file_save_error
    //       invalid_password
    //       invalid_priority
    //       refresh_error *
    //         acc_outs_lookup_error
    //         block_parse_error
    //         get_blocks_error
    //         get_hashes_error
    //         get_out_indexes_error
    //         tx_parse_error
    //         get_tx_pool_error
    //       transfer_error *
    //         get_random_outs_general_error
    //         not_enough_money
    //         tx_not_possible
    //         not_enough_outs_to_mix
    //         tx_not_constructed
    //         tx_rejected
    //         tx_sum_overflow
    //         tx_too_big
    //         zero_destination
    //       wallet_rpc_error *
    //         daemon_busy
    //         no_connection_to_daemon
    //         is_key_image_spent_error
    //         get_histogram_error
    //       wallet_files_doesnt_correspond
    //
    // * - class with protected ctor

    //----------------------------------------------------------------------------------------------------
    template<typename Base>
    struct wallet_error_base : public Base
    {
      const std::string& location() const { return m_loc; }

      std::string to_string() const
      {
        std::ostringstream ss;
        ss << m_loc << ':' << typeid(*this).name() << ": " << Base::what();
        return ss.str();
      }

    protected:
      wallet_error_base(std::string&& loc, const std::string& message)
        : Base(message)
        , m_loc(loc)
      {
      }

    private:
      std::string m_loc;
    };
    //----------------------------------------------------------------------------------------------------
    const char* const failed_rpc_request_messages[] = {
      "failed to get blocks",
      "failed to get hashes",
      "failed to get out indices",
      "failed to get random outs"
    };
    enum failed_rpc_request_message_indices
    {
      get_blocks_error_message_index,
      get_hashes_error_message_index,
      get_out_indices_error_message_index,
      get_random_outs_error_message_index
    };

    template<typename Base, int msg_index>
    struct failed_rpc_request : public Base
    {
      explicit failed_rpc_request(std::string&& loc, const std::string& status)
        : Base(std::move(loc), failed_rpc_request_messages[msg_index])
        , m_status(status)
      {
      }

      const std::string& status() const { return m_status; }

      std::string to_string() const
      {
        std::ostringstream ss;
        ss << Base::to_string() << ", status = " << status();
        return ss.str();
      }

    private:
      std::string m_status;
    };
    //----------------------------------------------------------------------------------------------------
    typedef wallet_error_base<std::logic_error> wallet_logic_error;
    typedef wallet_error_base<std::runtime_error> wallet_runtime_error;
    //----------------------------------------------------------------------------------------------------
    struct wallet_internal_error : public wallet_runtime_error
    {
      explicit wallet_internal_error(std::string&& loc, const std::string& message)
        : wallet_runtime_error(std::move(loc), message)
      {
      }
    };
    //----------------------------------------------------------------------------------------------------
    struct unexpected_txin_type : public wallet_internal_error
    {
      explicit unexpected_txin_type(std::string&& loc, const cryptonote::transaction& tx)
        : wallet_internal_error(std::move(loc), "one of tx inputs has unexpected type")
        , m_tx(tx)
      {
      }

      const cryptonote::transaction& tx() const { return m_tx; }

      std::string to_string() const
      {
        std::ostringstream ss;
        cryptonote::transaction tx = m_tx;
        ss << wallet_internal_error::to_string() << ", tx:\n" << cryptonote::obj_to_json_str(tx);
        return ss.str();
      }

    private:
      cryptonote::transaction m_tx;
    };
    //----------------------------------------------------------------------------------------------------
    struct wallet_not_initialized : public wallet_internal_error
    {
      explicit wallet_not_initialized(std::string&& loc)
        : wallet_internal_error(std::move(loc), "wallet is not initialized")
      {
      }
    };

    //----------------------------------------------------------------------------------------------------
    const char* const file_error_messages[] = {
      "file already exists",
      "file not found",
      "failed to read file",
      "failed to save file"
    };
    enum file_error_message_indices
    {
      file_exists_message_index,
      file_not_found_message_index,
      file_read_error_message_index,
      file_save_error_message_index
    };

    template<int msg_index>
    struct file_error_base : public wallet_logic_error
    {
      explicit file_error_base(std::string&& loc, const std::string& file)
        : wallet_logic_error(std::move(loc), std::string(file_error_messages[msg_index]) +  " \"" + file + '\"')
        , m_file(file)
      {
      }

      explicit file_error_base(std::string&& loc, const std::string& file, const std::error_code &e)
        : wallet_logic_error(std::move(loc), std::string(file_error_messages[msg_index]) +  " \"" + file + "\": " + e.message())
        , m_file(file)
      {
      }

      const std::string& file() const { return m_file; }

      std::string to_string() const { return wallet_logic_error::to_string(); }

    private:
      std::string m_file;
    };
    //----------------------------------------------------------------------------------------------------
    typedef file_error_base<file_exists_message_index> file_exists;
    typedef file_error_base<file_not_found_message_index>  file_not_found;
    typedef file_error_base<file_read_error_message_index> file_read_error;
    typedef file_error_base<file_save_error_message_index> file_save_error;
    //----------------------------------------------------------------------------------------------------
    struct invalid_password : public wallet_logic_error
    {
      explicit invalid_password(std::string&& loc)
        : wallet_logic_error(std::move(loc), "invalid password")
      {
      }

      std::string to_string() const { return wallet_logic_error::to_string(); }
    };
    struct invalid_priority : public wallet_logic_error
    {
      explicit invalid_priority(std::string&& loc)
        : wallet_logic_error(std::move(loc), "invalid priority")
      {
      }

      std::string to_string() const { return wallet_logic_error::to_string(); }
    };

    //----------------------------------------------------------------------------------------------------
    struct invalid_pregenerated_random : public wallet_logic_error
    {
      explicit invalid_pregenerated_random (std::string&& loc)
        : wallet_logic_error(std::move(loc), "invalid pregenerated random for wallet creation/recovery")
      {
      }

      std::string to_string() const { return wallet_logic_error::to_string(); }
    };
    //----------------------------------------------------------------------------------------------------
    struct refresh_error : public wallet_logic_error
    {
    protected:
      explicit refresh_error(std::string&& loc, const std::string& message)
        : wallet_logic_error(std::move(loc), message)
      {
      }
    };
    //----------------------------------------------------------------------------------------------------
    struct acc_outs_lookup_error : public refresh_error
    {
      explicit acc_outs_lookup_error(std::string&& loc, const cryptonote::transaction& tx,
        const crypto::public_key& tx_pub_key, const cryptonote::account_keys& acc_keys)
        : refresh_error(std::move(loc), "account outs lookup error")
        , m_tx(tx)
        , m_tx_pub_key(tx_pub_key)
        , m_acc_keys(acc_keys)
      {
      }

      const cryptonote::transaction& tx() const { return m_tx; }
      const crypto::public_key& tx_pub_key() const { return m_tx_pub_key; }
      const cryptonote::account_keys& acc_keys() const { return m_acc_keys; }

      std::string to_string() const
      {
        std::ostringstream ss;
        cryptonote::transaction tx = m_tx;
        ss << refresh_error::to_string() << ", tx: " << cryptonote::obj_to_json_str(tx);
        return ss.str();
      }

    private:
      const cryptonote::transaction m_tx;
      const crypto::public_key m_tx_pub_key;
      const cryptonote::account_keys m_acc_keys;
    };
    //----------------------------------------------------------------------------------------------------
    struct block_parse_error : public refresh_error
    {
      explicit block_parse_error(std::string&& loc, const cryptonote::blobdata& block_data)
        : refresh_error(std::move(loc), "block parse error")
        , m_block_blob(block_data)
      {
      }

      const cryptonote::blobdata& block_blob() const { return m_block_blob; }

      std::string to_string() const { return refresh_error::to_string(); }

    private:
      cryptonote::blobdata m_block_blob;
    };
    //----------------------------------------------------------------------------------------------------
    typedef failed_rpc_request<refresh_error, get_blocks_error_message_index> get_blocks_error;
    //----------------------------------------------------------------------------------------------------
    typedef failed_rpc_request<refresh_error, get_hashes_error_message_index> get_hashes_error;
    //----------------------------------------------------------------------------------------------------
    typedef failed_rpc_request<refresh_error, get_out_indices_error_message_index> get_out_indices_error;
    //----------------------------------------------------------------------------------------------------
    struct tx_parse_error : public refresh_error
    {
      explicit tx_parse_error(std::string&& loc, const cryptonote::blobdata& tx_blob)
        : refresh_error(std::move(loc), "transaction parse error")
        , m_tx_blob(tx_blob)
      {
      }

      const cryptonote::blobdata& tx_blob() const { return m_tx_blob; }

      std::string to_string() const { return refresh_error::to_string(); }

    private:
      cryptonote::blobdata m_tx_blob;
    };
    //----------------------------------------------------------------------------------------------------
    struct get_tx_pool_error : public refresh_error
    {
      explicit get_tx_pool_error(std::string&& loc)
        : refresh_error(std::move(loc), "error getting tranaction pool")
      {
      }

      std::string to_string() const { return refresh_error::to_string(); }
    };
    //----------------------------------------------------------------------------------------------------
    struct transfer_error : public wallet_logic_error
    {
    protected:
      explicit transfer_error(std::string&& loc, const std::string& message)
        : wallet_logic_error(std::move(loc), message)
      {
      }
    };
    //----------------------------------------------------------------------------------------------------
    typedef failed_rpc_request<transfer_error, get_random_outs_error_message_index> get_random_outs_error;
    //----------------------------------------------------------------------------------------------------
    struct not_enough_money : public transfer_error
    {
      explicit not_enough_money(std::string&& loc, uint64_t availbable, uint64_t tx_amount, uint64_t fee)
        : transfer_error(std::move(loc), "not enough money")
        , m_available(availbable)
        , m_tx_amount(tx_amount)
      {
      }

      uint64_t available() const { return m_available; }
      uint64_t tx_amount() const { return m_tx_amount; }

      std::string to_string() const
      {
        std::ostringstream ss;
        ss << transfer_error::to_string() <<
          ", available = " << cryptonote::print_money(m_available) <<
          ", tx_amount = " << cryptonote::print_money(m_tx_amount);
        return ss.str();
      }

    private:
      uint64_t m_available;
      uint64_t m_tx_amount;
    };
    //----------------------------------------------------------------------------------------------------
    struct tx_not_possible : public transfer_error
    {
      explicit tx_not_possible(std::string&& loc, uint64_t availbable, uint64_t tx_amount, uint64_t fee)
        : transfer_error(std::move(loc), "tx not possible")
        , m_available(availbable)
        , m_tx_amount(tx_amount)
        , m_fee(fee)
      {
      }

      uint64_t available() const { return m_available; }
      uint64_t tx_amount() const { return m_tx_amount; }
      uint64_t fee() const { return m_fee; }

      std::string to_string() const
      {
        std::ostringstream ss;
        ss << transfer_error::to_string() <<
          ", available = " << cryptonote::print_money(m_available) <<
          ", tx_amount = " << cryptonote::print_money(m_tx_amount) <<
          ", fee = " << cryptonote::print_money(m_fee);
        return ss.str();
      }

    private:
      uint64_t m_available;
      uint64_t m_tx_amount;
      uint64_t m_fee;
    };
    //----------------------------------------------------------------------------------------------------
    struct not_enough_outs_to_mix : public transfer_error
    {
      typedef std::unordered_map<uint64_t, uint64_t> scanty_outs_t;

      explicit not_enough_outs_to_mix(std::string&& loc, const scanty_outs_t& scanty_outs, size_t mixin_count)
        : transfer_error(std::move(loc), "not enough outputs to use")
        , m_scanty_outs(scanty_outs)
        , m_mixin_count(mixin_count)
      {
      }

      const scanty_outs_t& scanty_outs() const { return m_scanty_outs; }
      size_t mixin_count() const { return m_mixin_count; }

      std::string to_string() const
      {
        std::ostringstream ss;
        ss << transfer_error::to_string() << ", ring size = " << (m_mixin_count + 1) << ", scanty_outs:";
        for (const auto& out: m_scanty_outs)
        {
          ss << '\n' << cryptonote::print_money(out.first) << " - " << out.second;
        }
        return ss.str();
      }

    private:
      scanty_outs_t m_scanty_outs;
      size_t m_mixin_count;
    };
    //----------------------------------------------------------------------------------------------------
    struct tx_not_constructed : public transfer_error
    {
      typedef std::vector<cryptonote::tx_source_entry> sources_t;
      typedef std::vector<cryptonote::tx_destination_entry> destinations_t;

      explicit tx_not_constructed(
          std::string && loc
        , sources_t const & sources
        , destinations_t const & destinations
        , uint64_t unlock_time
        , bool testnet
        )
        : transfer_error(std::move(loc), "transaction was not constructed")
        , m_sources(sources)
        , m_destinations(destinations)
        , m_unlock_time(unlock_time)
        , m_testnet(testnet)
      {
      }

      const sources_t& sources() const { return m_sources; }
      const destinations_t& destinations() const { return m_destinations; }
      uint64_t unlock_time() const { return m_unlock_time; }

      std::string to_string() const
      {
        std::ostringstream ss;
        ss << transfer_error::to_string();
        ss << "\nSources:";
        for (size_t i = 0; i < m_sources.size(); ++i)
        {
          const cryptonote::tx_source_entry& src = m_sources[i];
          ss << "\n  source " << i << ":";
          ss << "\n    amount: " << cryptonote::print_money(src.amount);
          // It's not good, if logs will contain such much data
          //ss << "\n    real_output: " << src.real_output;
          //ss << "\n    real_output_in_tx_index: " << src.real_output_in_tx_index;
          //ss << "\n    real_out_tx_key: " << epee::string_tools::pod_to_hex(src.real_out_tx_key);
          //ss << "\n    outputs:";
          //for (size_t j = 0; j < src.outputs.size(); ++j)
          //{
          //  const cryptonote::tx_source_entry::output_entry& out = src.outputs[j];
          //  ss << "\n      " << j << ": " << out.first << ", " << epee::string_tools::pod_to_hex(out.second);
          //}
        }

        ss << "\nDestinations:";
        for (size_t i = 0; i < m_destinations.size(); ++i)
        {
          const cryptonote::tx_destination_entry& dst = m_destinations[i];
          ss << "\n  " << i << ": " << cryptonote::get_account_address_as_str(m_testnet, dst.addr) << " " <<
            cryptonote::print_money(dst.amount);
        }

        ss << "\nunlock_time: " << m_unlock_time;

        return ss.str();
      }

    private:
      sources_t m_sources;
      destinations_t m_destinations;
      uint64_t m_unlock_time;
      bool m_testnet;
    };
    //----------------------------------------------------------------------------------------------------
    struct tx_rejected : public transfer_error
    {
      explicit tx_rejected(std::string&& loc, const cryptonote::transaction& tx, const std::string& status, const std::string& reason)
        : transfer_error(std::move(loc), "transaction was rejected by daemon")
        , m_tx(tx)
        , m_status(status)
        , m_reason(reason)
      {
      }

      const cryptonote::transaction& tx() const { return m_tx; }
      const std::string& status() const { return m_status; }
      const std::string& reason() const { return m_reason; }

      std::string to_string() const
      {
        std::ostringstream ss;
        ss << transfer_error::to_string() << ", status = " << m_status << ", tx:\n";
        cryptonote::transaction tx = m_tx;
        ss << cryptonote::obj_to_json_str(tx);
        if (!m_reason.empty())
        {
          ss << " (" << m_reason << ")";
        }
        return ss.str();
      }

    private:
      cryptonote::transaction m_tx;
      std::string m_status;
      std::string m_reason;
    };
    //----------------------------------------------------------------------------------------------------
    struct tx_sum_overflow : public transfer_error
    {
      explicit tx_sum_overflow(
          std::string && loc
        , const std::vector<cryptonote::tx_destination_entry>& destinations
        , uint64_t fee
        , bool testnet
        )
        : transfer_error(std::move(loc), "transaction sum + fee exceeds " + cryptonote::print_money(std::numeric_limits<uint64_t>::max()))
        , m_destinations(destinations)
        , m_fee(fee)
        , m_testnet(testnet)
      {
      }

      const std::vector<cryptonote::tx_destination_entry>& destinations() const { return m_destinations; }
      uint64_t fee() const { return m_fee; }

      std::string to_string() const
      {
        std::ostringstream ss;
        ss << transfer_error::to_string() <<
          ", fee = " << cryptonote::print_money(m_fee) <<
          ", destinations:";
        for (const auto& dst : m_destinations)
        {
          ss << '\n' << cryptonote::print_money(dst.amount) << " -> " << cryptonote::get_account_address_as_str(m_testnet, dst.addr);
        }
        return ss.str();
      }

    private:
      std::vector<cryptonote::tx_destination_entry> m_destinations;
      uint64_t m_fee;
      bool m_testnet;
    };
    //----------------------------------------------------------------------------------------------------
    struct tx_too_big : public transfer_error
    {
      explicit tx_too_big(std::string&& loc, const cryptonote::transaction& tx, uint64_t tx_size_limit)
        : transfer_error(std::move(loc), "transaction is too big")
        , m_tx(tx)
        , m_tx_size_limit(tx_size_limit)
      {
      }

      const cryptonote::transaction& tx() const { return m_tx; }
      uint64_t tx_size_limit() const { return m_tx_size_limit; }

      std::string to_string() const
      {
        std::ostringstream ss;
        cryptonote::transaction tx = m_tx;
        ss << transfer_error::to_string() <<
          ", tx_size_limit = " << m_tx_size_limit <<
          ", tx size = " << get_object_blobsize(m_tx) <<
          ", tx:\n" << cryptonote::obj_to_json_str(tx);
        return ss.str();
      }

    private:
      cryptonote::transaction m_tx;
      uint64_t m_tx_size_limit;
    };
    //----------------------------------------------------------------------------------------------------
    struct zero_destination : public transfer_error
    {
      explicit zero_destination(std::string&& loc)
        : transfer_error(std::move(loc), "destination amount is zero")
      {
      }
    };
    //----------------------------------------------------------------------------------------------------
    struct wallet_rpc_error : public wallet_logic_error
    {
      const std::string& request() const { return m_request; }

      std::string to_string() const
      {
        std::ostringstream ss;
        ss << wallet_logic_error::to_string() << ", request = " << m_request;
        return ss.str();
      }

    protected:
      explicit wallet_rpc_error(std::string&& loc, const std::string& message, const std::string& request)
        : wallet_logic_error(std::move(loc), message)
        , m_request(request)
      {
      }

    private:
      std::string m_request;
    };
    //----------------------------------------------------------------------------------------------------
    struct wallet_generic_rpc_error : public wallet_rpc_error
    {
      explicit wallet_generic_rpc_error(std::string&& loc, const std::string& request, const std::string& status)
        : wallet_rpc_error(std::move(loc), std::string("error in ") + request + " RPC: " + status, request),
        m_status(status)
      {
      }
      const std::string& status() const { return m_status; }
    private:
      const std::string m_status;
    };
    //----------------------------------------------------------------------------------------------------
    struct daemon_busy : public wallet_rpc_error
    {
      explicit daemon_busy(std::string&& loc, const std::string& request)
        : wallet_rpc_error(std::move(loc), "daemon is busy", request)
      {
      }
    };
    //----------------------------------------------------------------------------------------------------
    struct no_connection_to_daemon : public wallet_rpc_error
    {
      explicit no_connection_to_daemon(std::string&& loc, const std::string& request)
        : wallet_rpc_error(std::move(loc), "no connection to daemon", request)
      {
      }
    };
    //----------------------------------------------------------------------------------------------------
    struct is_key_image_spent_error : public wallet_rpc_error
    {
      explicit is_key_image_spent_error(std::string&& loc, const std::string& request)
        : wallet_rpc_error(std::move(loc), "error from is_key_image_spent call", request)
      {
      }
    };
    //----------------------------------------------------------------------------------------------------
    struct get_histogram_error : public wallet_rpc_error
    {
      explicit get_histogram_error(std::string&& loc, const std::string& request)
        : wallet_rpc_error(std::move(loc), "failed to get output histogram", request)
      {
      }
    };
    //----------------------------------------------------------------------------------------------------
    struct wallet_files_doesnt_correspond : public wallet_logic_error
    {
      explicit wallet_files_doesnt_correspond(std::string&& loc, const std::string& keys_file, const std::string& wallet_file)
        : wallet_logic_error(std::move(loc), "file " + wallet_file + " does not correspond to " + keys_file)
      {
      }

      const std::string& keys_file() const { return m_keys_file; }
      const std::string& wallet_file() const { return m_wallet_file; }

      std::string to_string() const { return wallet_logic_error::to_string(); }

    private:
      std::string m_keys_file;
      std::string m_wallet_file;
    };
    //----------------------------------------------------------------------------------------------------

#if !defined(_MSC_VER)

    template<typename TException, typename... TArgs>
    void throw_wallet_ex(std::string&& loc, const TArgs&... args)
    {
      TException e(std::move(loc), args...);
      LOG_PRINT_L0(e.to_string());
      throw e;
    }

#else
    #include <boost/preprocessor/repetition/enum_binary_params.hpp>
    #include <boost/preprocessor/repetition/enum_params.hpp>
    #include <boost/preprocessor/repetition/repeat_from_to.hpp>

    template<typename TException>
    void throw_wallet_ex(std::string&& loc)
    {
      TException e(std::move(loc));
      LOG_PRINT_L0(e.to_string());
      throw e;
    }

#define GEN_throw_wallet_ex(z, n, data)                                                       \
    template<typename TException, BOOST_PP_ENUM_PARAMS(n, typename TArg)>                     \
    void throw_wallet_ex(std::string&& loc, BOOST_PP_ENUM_BINARY_PARAMS(n, const TArg, &arg)) \
    {                                                                                         \
      TException e(std::move(loc), BOOST_PP_ENUM_PARAMS(n, arg));                             \
      LOG_PRINT_L0(e.to_string());                                                            \
      throw e;                                                                                \
    }

    BOOST_PP_REPEAT_FROM_TO(1, 6, GEN_throw_wallet_ex, ~)
#endif
  }
}

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)

#define THROW_WALLET_EXCEPTION_IF(cond, err_type, ...)                                                      \
  if (cond)                                                                                                 \
  {                                                                                                         \
    LOG_ERROR(#cond << ". THROW EXCEPTION: " << #err_type);                                                 \
    tools::error::throw_wallet_ex<err_type>(std::string(__FILE__ ":" STRINGIZE(__LINE__)), ## __VA_ARGS__); \
  }
