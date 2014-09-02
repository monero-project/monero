// Copyright (c) 2014, The Monero Project
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

#include <memory>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <atomic>

#include "include_base_utils.h"
#include "cryptonote_core/account.h"
#include "cryptonote_core/account_boost_serialization.h"
#include "cryptonote_core/cryptonote_basic_impl.h"
#include "net/http_client.h"
#include "storages/http_abstract_invoke.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "cryptonote_core/cryptonote_format_utils.h"
#include "common/unordered_containers_boost_serialization.h"
#include "crypto/chacha8.h"
#include "crypto/hash.h"

#include "wallet_errors.h"

#include <iostream>
#define DEFAULT_TX_SPENDABLE_AGE                               10
#define WALLET_RCP_CONNECTION_TIMEOUT                          200000

namespace tools
{
  class i_wallet2_callback
  {
  public:
    virtual void on_new_block(uint64_t height, const cryptonote::block& block) {}
    virtual void on_money_received(uint64_t height, const cryptonote::transaction& tx, size_t out_index) {}
    virtual void on_money_spent(uint64_t height, const cryptonote::transaction& in_tx, size_t out_index, const cryptonote::transaction& spend_tx) {}
    virtual void on_skip_transaction(uint64_t height, const cryptonote::transaction& tx) {}
  };

  struct tx_dust_policy
  {
    uint64_t dust_threshold;
    bool add_to_fee;
    cryptonote::account_public_address addr_for_dust;

    tx_dust_policy(uint64_t a_dust_threshold = 0, bool an_add_to_fee = true, cryptonote::account_public_address an_addr_for_dust = cryptonote::account_public_address())
      : dust_threshold(a_dust_threshold)
      , add_to_fee(an_add_to_fee)
      , addr_for_dust(an_addr_for_dust)
    {
    }
  };

  class wallet2
  {
    wallet2(const wallet2&) : m_run(true), m_callback(0), m_testnet(false) {};
  public:
    wallet2(bool testnet = false) : m_run(true), m_callback(0), m_testnet(testnet) {};
    struct transfer_details
    {
      uint64_t m_block_height;
      cryptonote::transaction m_tx;
      size_t m_internal_output_index;
      uint64_t m_global_output_index;
      bool m_spent;
      crypto::key_image m_key_image; //TODO: key_image stored twice :(

      uint64_t amount() const { return m_tx.vout[m_internal_output_index].amount; }
    };

    struct payment_details
    {
      crypto::hash m_tx_hash;
      uint64_t m_amount;
      uint64_t m_block_height;
      uint64_t m_unlock_time;
    };

    struct unconfirmed_transfer_details
    {
      cryptonote::transaction m_tx;
      uint64_t m_change;
      time_t m_sent_time;
    };

    typedef std::vector<transfer_details> transfer_container;
    typedef std::unordered_multimap<crypto::hash, payment_details> payment_container;

    struct pending_tx
    {
      cryptonote::transaction tx;
      uint64_t dust, fee;
      cryptonote::tx_destination_entry change_dts;
      std::list<transfer_container::iterator> selected_transfers;
      std::string key_images;
    };

    struct keys_file_data
    {
      crypto::chacha8_iv iv;
      std::string account_data;

      BEGIN_SERIALIZE_OBJECT()
        FIELD(iv)
        FIELD(account_data)
      END_SERIALIZE()
    };

    crypto::secret_key generate(const std::string& wallet, const std::string& password, const crypto::secret_key& recovery_param = crypto::secret_key(), bool recover = false, bool two_random = false);
    void load(const std::string& wallet, const std::string& password);
    void store();
    cryptonote::account_base& get_account(){return m_account;}

    // upper_transaction_size_limit as defined below is set to 
    // approximately 125% of the fixed minimum allowable penalty
    // free block size. TODO: fix this so that it actually takes
    // into account the current median block size rather than
    // the minimum block size.
    void init(const std::string& daemon_address = "http://localhost:8080", uint64_t upper_transaction_size_limit = ((CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * 125) / 100) - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE);
    bool deinit();

    void stop() { m_run.store(false, std::memory_order_relaxed); }

    i_wallet2_callback* callback() const { return m_callback; }
    void callback(i_wallet2_callback* callback) { m_callback = callback; }

    bool get_seed(std::string& electrum_words);
    
    void refresh();
    void refresh(uint64_t start_height, size_t & blocks_fetched);
    void refresh(uint64_t start_height, size_t & blocks_fetched, bool& received_money);
    bool refresh(size_t & blocks_fetched, bool& received_money, bool& ok);

    uint64_t balance();
    uint64_t unlocked_balance();
    template<typename T>
    void create_pending_transaction(
        const std::vector<cryptonote::tx_destination_entry>& dsts
      , size_t fake_outputs_count
      , uint64_t unlock_time
      , uint64_t fee
      , const std::vector<uint8_t>& extra
      , T destination_split_strategy
      , const tx_dust_policy& dust_policy
      , pending_tx& ptx
      );
    void create_pending_transaction(
        const std::vector<cryptonote::tx_destination_entry>& dsts
      , size_t fake_outputs_count
      , uint64_t unlock_time
      , uint64_t fee
      , const std::vector<uint8_t>& extra
      , pending_tx& ptx
      );
    void commit_tx(pending_tx& ptx_vector);
    void commit_tx(std::vector<pending_tx>& ptx_vector);
    std::vector<pending_tx> create_transactions(std::vector<cryptonote::tx_destination_entry> dsts, const size_t fake_outs_count, const uint64_t unlock_time, const uint64_t fee, const std::vector<uint8_t> extra);
    bool check_connection();
    void get_transfers(wallet2::transfer_container& incoming_transfers) const;
    void get_payments(const crypto::hash& payment_id, std::list<wallet2::payment_details>& payments, uint64_t min_height = 0) const;
    uint64_t get_blockchain_current_height() const { return m_local_bc_height; }
    template <class t_archive>
    inline void serialize(t_archive &a, const unsigned int ver)
    {
      if(ver < 5)
        return;
      a & m_blockchain;
      a & m_transfers;
      a & m_account_public_address;
      a & m_key_images;
      if(ver < 6)
        return;
      a & m_unconfirmed_txs;
      if(ver < 7)
        return;
      a & m_payments;
    }

    static void wallet_exists(const std::string& file_path, bool& keys_file_exists, bool& wallet_file_exists);

    static bool parse_payment_id(const std::string& payment_id_str, crypto::hash& payment_id);

  private:
    bool store_keys(const std::string& keys_file_name, const std::string& password);
    void load_keys(const std::string& keys_file_name, const std::string& password);
    void process_new_transaction(const cryptonote::transaction& tx, uint64_t height);
    void process_new_blockchain_entry(const cryptonote::block& b, cryptonote::block_complete_entry& bche, crypto::hash& bl_id, uint64_t height);
    void detach_blockchain(uint64_t height);
    void get_short_chain_history(std::list<crypto::hash>& ids);
    bool is_tx_spendtime_unlocked(uint64_t unlock_time) const;
    bool is_transfer_unlocked(const transfer_details& td) const;
    bool clear();
    void pull_blocks(uint64_t start_height, size_t& blocks_added);
    uint64_t select_transfers(uint64_t needed_money, bool add_dust, uint64_t dust, std::list<transfer_container::iterator>& selected_transfers);
    bool prepare_file_names(const std::string& file_path);
    void process_unconfirmed(const cryptonote::transaction& tx);
    void add_unconfirmed_tx(const cryptonote::transaction& tx, uint64_t change_amount);
    void generate_genesis(cryptonote::block& b);
    void check_genesis(const crypto::hash& genesis_hash); //throws

    cryptonote::account_base m_account;
    std::string m_daemon_address;
    std::string m_wallet_file;
    std::string m_keys_file;
    epee::net_utils::http::http_simple_client m_http_client;
    std::vector<crypto::hash> m_blockchain;
    std::atomic<uint64_t> m_local_bc_height; //temporary workaround
    std::unordered_map<crypto::hash, unconfirmed_transfer_details> m_unconfirmed_txs;

    transfer_container m_transfers;
    payment_container m_payments;
    std::unordered_map<crypto::key_image, size_t> m_key_images;
    cryptonote::account_public_address m_account_public_address;
    uint64_t m_upper_transaction_size_limit; //TODO: auto-calc this value or request from daemon, now use some fixed value

    std::atomic<bool> m_run;

    i_wallet2_callback* m_callback;
    bool m_testnet;
  };
}
BOOST_CLASS_VERSION(tools::wallet2, 7)

namespace boost
{
  namespace serialization
  {
    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::transfer_details &x, const boost::serialization::version_type ver)
    {
      a & x.m_block_height;
      a & x.m_global_output_index;
      a & x.m_internal_output_index;
      a & x.m_tx;
      a & x.m_spent;
      a & x.m_key_image;
    }

    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::unconfirmed_transfer_details &x, const boost::serialization::version_type ver)
    {
      a & x.m_change;
      a & x.m_sent_time;
      a & x.m_tx;
    }

    template <class Archive>
    inline void serialize(Archive& a, tools::wallet2::payment_details& x, const boost::serialization::version_type ver)
    {
      a & x.m_tx_hash;
      a & x.m_amount;
      a & x.m_block_height;
      a & x.m_unlock_time;
    }
  }
}

namespace tools
{

  namespace detail
  {
    //----------------------------------------------------------------------------------------------------
    inline void digit_split_strategy(
        const std::vector<cryptonote::tx_destination_entry>& addressed_payments
      , const cryptonote::tx_destination_entry& change_payment
      , uint64_t dust_threshold
      , std::vector<cryptonote::tx_destination_entry>& splitted_dsts
      , uint64_t& dust
      )
    {
      splitted_dsts.clear();
      dust = 0;

      // Split each outgoing payment up into one dust transaction and multiple
      // transactions with amount n * 10^x, where 1 <= n <= 9.  The dust
      // transaction is the only transaction with amount below the dust
      // threshold.
      for (auto& payment : addressed_payments)
      {
        cryptonote::decompose_amount_into_digits(
            payment.amount
          , dust_threshold
          , [&](uint64_t chunk)
            {
              splitted_dsts.push_back(cryptonote::tx_destination_entry(chunk, payment.addr));
            }
          , [&](uint64_t a_dust)
            {
              splitted_dsts.push_back(cryptonote::tx_destination_entry(a_dust, payment.addr));
            }
          );
      }

      // Split the change transaction into multiple transactions with amount
      // n * 10^x, where 1 <= n <= 9.  The unhandled remainder is returned by
      // reference as the variable dust.
      cryptonote::decompose_amount_into_digits(
          change_payment.amount
        , dust_threshold
        , [&](uint64_t chunk)
          {
            splitted_dsts.push_back(cryptonote::tx_destination_entry(chunk, change_payment.addr));
          }
        , [&](uint64_t a_dust) {
            dust = a_dust;
          }
        );
    }
    //----------------------------------------------------------------------------------------------------
    inline void null_split_strategy(const std::vector<cryptonote::tx_destination_entry>& dsts,
      const cryptonote::tx_destination_entry& change_dst, uint64_t dust_threshold,
      std::vector<cryptonote::tx_destination_entry>& splitted_dsts, uint64_t& dust)
    {
      splitted_dsts = dsts;

      dust = 0;
      uint64_t change = change_dst.amount;
      if (0 < dust_threshold)
      {
        for (uint64_t order = 10; order <= 10 * dust_threshold; order *= 10)
        {
          uint64_t dust_candidate = change_dst.amount % order;
          uint64_t change_candidate = (change_dst.amount / order) * order;
          if (dust_candidate <= dust_threshold)
          {
            dust = dust_candidate;
            change = change_candidate;
          }
          else
          {
            break;
          }
        }
      }

      if (0 != change)
      {
        splitted_dsts.push_back(cryptonote::tx_destination_entry(change, change_dst.addr));
      }
    }
    //----------------------------------------------------------------------------------------------------
    inline void print_source_entry(const cryptonote::tx_source_entry& src)
    {
      std::string indexes;
      std::for_each(src.outputs.begin(), src.outputs.end(), [&](const cryptonote::tx_source_entry::output_entry& s_e) { indexes += boost::to_string(s_e.first) + " "; });
      LOG_PRINT_L0("amount=" << cryptonote::print_money(src.amount) << ", real_output=" <<src.real_output << ", real_output_in_tx_index=" << src.real_output_in_tx_index << ", indexes: " << indexes);
    }
    //----------------------------------------------------------------------------------------------------
  }
  //----------------------------------------------------------------------------------------------------
  // Create a pending transaction that transforms unspent, unlocked input
  // transfers owned by this account into output transfers that will be owned
  // by the destination accounts.
  template<typename T>
  void wallet2::create_pending_transaction(
      const std::vector<cryptonote::tx_destination_entry>& output_transfers
    , size_t fake_inputs_per_real_input
    , uint64_t unlock_time
    , uint64_t fee
    , const std::vector<uint8_t>& extra
    , T destination_split_strategy
    , const tx_dust_policy& dust_policy
    , pending_tx &ptx
    )
  {
    using namespace cryptonote;

    // Throw an exception if no payments were requested
    if (output_transfers.empty())
    {
      THROW_WALLET_EXCEPTION(error::zero_destination);
    }

    uint64_t needed_money = fee;

    // Calculate total amount being sent to all destinations and throw if total
    // amount overflows uint64_t
    for (auto& dt : output_transfers)
    {
      if (0 == dt.amount)
      {
        THROW_WALLET_EXCEPTION(error::zero_destination);
      }

      needed_money += dt.amount;
      if (needed_money < dt.amount)
      {
        THROW_WALLET_EXCEPTION(error::tx_sum_overflow, output_transfers, fee);
      }
    }

    // Store unspent, unlocked transfers owned by this account in
    // real_input_transfers
    std::list<transfer_container::iterator> real_input_transfers;
    uint64_t found_money = select_transfers(
        needed_money
      , 0 == fake_inputs_per_real_input // ask for dust if there are no mixins
      , dust_policy.dust_threshold
      , real_input_transfers
      );

    // Throw exception if requested send amount is greater than amount
    // available to send
    if (found_money < needed_money)
    {
      THROW_WALLET_EXCEPTION(error::not_enough_money, found_money, needed_money - fee, fee);
    }

    // If mixins (fake input transfers) were requested, fetch them from the
    // daemon via RPC.  For each real input transfer, the daemon returns
    // fake_inputs_per_real_input mixin transfers.
    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response daemon_resp {};
    if (fake_inputs_per_real_input)
    {
      COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request req {};
      req.outs_count = fake_inputs_per_real_input + 1;// add one to make possible (if need) to skip real output key

      // Inform the daemon of the amount for each of the real inputs.
      for (transfer_container::iterator it : real_input_transfers)
      {
        if (it->m_tx.vout.size() <= it->m_internal_output_index)
        {
          THROW_WALLET_EXCEPTION(
              error::wallet_internal_error
            , "m_internal_output_index = " + std::to_string(it->m_internal_output_index) +
              " is greater or equal to outputs count = " + std::to_string(it->m_tx.vout.size())
            );
        }

        req.amounts.push_back(it->amount());
      }

      // Query the daemon via RPC
      {
        bool r = epee::net_utils::invoke_http_bin_remote_command2(
            m_daemon_address + "/getrandom_outs.bin"
          , req
          , daemon_resp
          , m_http_client
          , 200000
          );

        if (!r)
        {
          THROW_WALLET_EXCEPTION(error::no_connection_to_daemon, "getrandom_outs.bin");
        }

        if (daemon_resp.status == CORE_RPC_STATUS_BUSY)
        {
          THROW_WALLET_EXCEPTION(error::daemon_busy, "getrandom_outs.bin");
        }

        if (daemon_resp.status != CORE_RPC_STATUS_OK)
        {
          THROW_WALLET_EXCEPTION(error::get_random_outs_error, daemon_resp.status);
        }
      }

      // Ensure that there is a group of mixins for every real input transfer
      if (daemon_resp.outs.size() != real_input_transfers.size())
      {
        THROW_WALLET_EXCEPTION(
            error::wallet_internal_error
          , "daemon returned wrong response for getrandom_outs.bin, wrong amounts count = " +
            std::to_string(daemon_resp.outs.size()) + ", expected " + std::to_string(real_input_transfers.size())
          );
      }

      // Ensure that we received the right number of mixins for each real input
      // transaction.
      {
        std::vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount> scanty_outs;
        for (COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& amount_outs : daemon_resp.outs)
        {
          if (amount_outs.outs.size() < fake_inputs_per_real_input)
          {
            scanty_outs.push_back(amount_outs);
          }
        }

        if (!scanty_outs.empty())
        {
          THROW_WALLET_EXCEPTION(error::not_enough_outs_to_mix, scanty_outs, fake_inputs_per_real_input);
        }
      }
    }

    // Iterate through the selected real input transfers, and combine them with
    // any fake mixin transfers returned by the daemon to create the input_transfers
    // vector.
    size_t i = 0;
    std::vector<cryptonote::tx_source_entry> obfuscated_input_transfers;
    for (transfer_container::iterator it : real_input_transfers)
    {
      typedef cryptonote::tx_source_entry::output_entry tx_output_entry;

      obfuscated_input_transfers.resize(obfuscated_input_transfers.size()+1);
      cryptonote::tx_source_entry& src = obfuscated_input_transfers.back();
      transfer_details& td = *it;
      src.amount = td.amount();

      // Add the mixin input transfers associated with the current real input
      // transfer to the obfuscated_input_transfers vector
      if (daemon_resp.outs.size())
      {
        typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry out_entry;

        daemon_resp.outs[i].outs.sort(
            [](const out_entry& a, const out_entry& b)
            {
              return a.global_amount_index < b.global_amount_index;
            }
          );

        for (out_entry& daemon_oe : daemon_resp.outs[i].outs)
        {
          // Don't use this mixin transfer if it's the same transfer as the
          // real one
          if (td.m_global_output_index == daemon_oe.global_amount_index)
          {
            continue;
          }

          // Add the mixin to the obfuscated_input_transfers vector
          tx_output_entry oe;
          oe.first = daemon_oe.global_amount_index;
          oe.second = daemon_oe.out_key;
          src.outputs.push_back(oe);

          // We're done if we have the requested number of mixins
          if (src.outputs.size() >= fake_inputs_per_real_input)
          {
            break;
          }
        }
      }

      // Get an iterator referencing the position that the real input transfer
      // will be inserted to
      auto it_to_insert = std::find_if(
          src.outputs.begin()
        , src.outputs.end()
        , [&](const tx_output_entry& a)
          {
            return a.first >= td.m_global_output_index;
          }
        );

      // Add the real input transfer to the obfuscated_input_transfers vector
      tx_output_entry real_oe;
      real_oe.first = td.m_global_output_index;
      real_oe.second = boost::get<txout_to_key>(td.m_tx.vout[td.m_internal_output_index].target).key;
      auto interted_it = src.outputs.insert(it_to_insert, real_oe);
      src.real_out_tx_key = get_tx_pub_key_from_extra(td.m_tx);
      src.real_output = interted_it - src.outputs.begin();
      src.real_output_in_tx_index = td.m_internal_output_index;
      detail::print_source_entry(src);
      ++i;
    }

    // The input transfer amounts likely sum to more than the output amounts
    // (plus the fee). Set up a change output transfer for the remainder which
    // is addressed to this account.
    cryptonote::tx_destination_entry change_transfer {};
    if (needed_money < found_money)
    {
      change_transfer.addr = m_account.get_keys().m_account_address;
      change_transfer.amount = found_money - needed_money;
    }

    // Split up the output transfers to obfuscate their amounts.  The dust
    // variable will contain an amount below the dust threshold that is not
    // represented in any outgoing transfers.
    uint64_t dust = 0;
    std::vector<cryptonote::tx_destination_entry> split_output_transfers;
    destination_split_strategy(
        output_transfers
      , change_transfer
      , dust_policy.dust_threshold
      , split_output_transfers
      , dust
      );

    // Throw an exception if the dust amount is more than the dust threshold
    if (dust_policy.dust_threshold < dust)
    {
      THROW_WALLET_EXCEPTION(
          error::wallet_internal_error
        , "invalid dust value: dust = " + std::to_string(dust) + ", dust_threshold = "
          + std::to_string(dust_policy.dust_threshold)
        );
    }

    // The dust policy can specify an address to which dust should be sent.  If
    // this is the case, we create the appropriate output transfer here.
    if (0 != dust && !dust_policy.add_to_fee)
    {
      split_output_transfers.push_back(cryptonote::tx_destination_entry(dust, dust_policy.addr_for_dust));
    }

    // Construct the actual transaction.
    cryptonote::transaction tx;
    bool r = cryptonote::construct_tx(
        m_account.get_keys()
      , obfuscated_input_transfers
      , split_output_transfers
      , extra
      , tx
      , unlock_time
      );

    if (!r)
    {
      THROW_WALLET_EXCEPTION(error::tx_not_constructed
        , obfuscated_input_transfers, split_output_transfers, unlock_time);
    }

    // Throw an exception if the transaction is too large
    if (m_upper_transaction_size_limit <= get_object_blobsize(tx))
    {
      THROW_WALLET_EXCEPTION(error::tx_too_big, tx, m_upper_transaction_size_limit);
    }

    // Create a key image string from the transaction inputs
    std::string key_images;
    bool all_are_txin_to_key = std::all_of(
        tx.vin.begin()
      , tx.vin.end()
      , [&](const txin_v& s_e) -> bool
        {
          if(s_e.type() != typeid(txin_to_key))
          {
            LOG_PRINT_L0("wrong variant type: " << s_e.type().name() << ", expected " << typeid(txin_to_key).name());
            return false;
          }
          txin_to_key in = boost::get<txin_to_key>(s_e);

          key_images += boost::to_string(in.k_image) + " ";
          return true;
        }
      );

    if (!all_are_txin_to_key)
    {
      THROW_WALLET_EXCEPTION(error::unexpected_txin_type, tx);
    }

    // Inintialize the pending transaction
    ptx.key_images = key_images;
    ptx.fee = fee;
    ptx.dust = dust;
    ptx.tx = tx;
    ptx.change_dts = change_transfer;
    ptx.selected_transfers = real_input_transfers;
  }
}
