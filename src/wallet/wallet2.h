// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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

#define DEFAULT_TX_SPENDABLE_AGE                               10
#define WALLET_RCP_CONNECTION_TIMEOUT                          200000

namespace tools
{
  inline std::string interpret_rpc_response(bool ok, const std::string& status)
  {
    std::string err;
    if (ok)
    {
      if (status == CORE_RPC_STATUS_BUSY)
      {
        err = "daemon is busy. Please try later";
      }
      else if (status != CORE_RPC_STATUS_OK)
      {
        err = status;
      }
    }
    else
    {
      err = "possible lost connection to daemon";
    }
    return err;
  }


  class wallet2
  {
    wallet2(const wallet2&) : m_run(true) {};
  public:
    wallet2() : m_run(true) {};
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
    typedef std::vector<transfer_details> transfer_container;

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

    struct keys_file_data
    {
      crypto::chacha8_iv iv;
      std::string account_data;

      BEGIN_SERIALIZE_OBJECT()
        FIELD(iv)
        FIELD(account_data)
      END_SERIALIZE()
    };

    struct fail_details
    {
      enum fail_reason
      {
        error_ok = 0,
        error_not_connected,
        error_daemon_is_busy,
        error_rejected_by_daemon,
        error_too_big_transaction,
        error_not_enough_money,
        error_too_big_mixin,
        error_to_parse_block,
        error_to_parse_tx,
        error_to_parse_tx_extra,
        error_invalid_tx,
        error_internal_error
      };
      fail_reason reason;
      uint64_t tx_blob_size;
      uint64_t max_expected_tx_blob_size;

      std::string what() const
      {
        switch (reason)
        {
        case error_ok:                  return "OK";
        case error_not_connected:       return "not connected";
        case error_daemon_is_busy:      return "daemon is busy. Please try later";
        case error_rejected_by_daemon:  return "rejected by daemon";
        case error_too_big_transaction: return "transaction size is too big";
        case error_not_enough_money:    return "not enough money";
        case error_too_big_mixin:       return "not enough outputs for specified mixin_count";
        case error_to_parse_block:      return "failed to parse/validate block";
        case error_to_parse_tx:         return "failed to parse/validate tx";
        case error_to_parse_tx_extra:   return "failed to parse/validate tx extra";
        case error_invalid_tx:          return "wrong tx";
        case error_internal_error:      return "internal error";
        default:                        return "unknown error";
        }
      }
    };

    bool generate(const std::string& wallet, const std::string& password);
    bool load(const std::string& wallet, const std::string& password);
    bool store();
    cryptonote::account_base& get_account(){return m_account;}

    bool init(const std::string& daemon_address = "http://localhost:8080", uint64_t upper_transaction_size_limit = CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE*2 - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE);

    bool refresh(fail_details& fd);
    bool refresh(size_t & blocks_fetched, fail_details& fd);
    bool refresh(size_t & blocks_fetched, bool& received_money, fail_details& fd);
    bool deinit();

    void stop() { m_run.store(false, std::memory_order_relaxed); }

    uint64_t balance();
    uint64_t unlocked_balance();
    template<typename T>
    bool enum_incoming_transfers(const T& handler) const;
    template<typename T>
    bool transfer(const std::vector<cryptonote::tx_destination_entry>& dsts, size_t fake_outputs_count, uint64_t unlock_time, uint64_t fee, T destination_split_strategy, const tx_dust_policy& dust_policy);
    template<typename T>
    bool transfer(const std::vector<cryptonote::tx_destination_entry>& dsts, size_t fake_outputs_count, uint64_t unlock_time, uint64_t fee, T destination_split_strategy, const tx_dust_policy& dust_policy, cryptonote::transaction &tx, fail_details& tfd);
    template<typename T>
    bool transfer(const std::vector<cryptonote::tx_destination_entry>& dsts, size_t fake_outputs_count, uint64_t unlock_time, uint64_t fee, T destination_split_strategy, const tx_dust_policy& dust_policy, cryptonote::transaction &tx);
    bool transfer(const std::vector<cryptonote::tx_destination_entry>& dsts, size_t fake_outputs_count, uint64_t unlock_time, uint64_t fee);
    bool transfer(const std::vector<cryptonote::tx_destination_entry>& dsts, size_t fake_outputs_count, uint64_t unlock_time, uint64_t fee, cryptonote::transaction& tx);
    bool transfer(const std::vector<cryptonote::tx_destination_entry>& dsts, size_t fake_outputs_count, uint64_t unlock_time, uint64_t fee, cryptonote::transaction& tx, fail_details& tfd);
    bool check_connection();
    bool get_transfers(wallet2::transfer_container& incoming_transfers);
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
    }

  private:
    bool store_keys(const std::string& keys_file_name, const std::string& password);
    bool load_keys(const std::string& keys_file_name, const std::string& password);
    bool process_new_transaction(cryptonote::transaction& tx, uint64_t height, fail_details& fd);
    bool process_new_blockchain_entry(cryptonote::block& b, cryptonote::block_complete_entry& bche, crypto::hash& bl_id, uint64_t height, fail_details& fd);
    bool detach_blockchain(uint64_t height);
    bool get_short_chain_history(std::list<crypto::hash>& ids);
    bool is_tx_spendtime_unlocked(uint64_t unlock_time) const;
    bool is_transfer_unlocked(const transfer_details& td) const;
    bool clear();
    bool pull_blocks(size_t& blocks_added, fail_details& fd);
    uint64_t select_transfers(uint64_t needed_money, bool add_dust, uint64_t dust, std::list<transfer_container::iterator>& selected_transfers);
    bool prepare_file_names(const std::string& file_path);

    cryptonote::account_base m_account;
    std::string m_daemon_address;
    std::string m_wallet_file;
    std::string m_keys_file;
    epee::net_utils::http::http_simple_client m_http_client;
    std::vector<crypto::hash> m_blockchain;
    std::atomic<uint64_t> m_local_bc_height; //temporary workaround 

    transfer_container m_transfers;
    std::unordered_map<crypto::key_image, size_t> m_key_images;
    cryptonote::account_public_address m_account_public_address;
    uint64_t m_upper_transaction_size_limit; //TODO: auto-calc this value or request from daemon, now use some fixed value

    std::atomic<bool> m_run;
  };
}
BOOST_CLASS_VERSION(tools::wallet2, 5)

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
  }
}

namespace tools
{
  namespace detail
  {
    //----------------------------------------------------------------------------------------------------
    inline void digit_split_strategy(const std::vector<cryptonote::tx_destination_entry>& dsts,
      const cryptonote::tx_destination_entry& change_dst, uint64_t dust_threshold,
      std::vector<cryptonote::tx_destination_entry>& splitted_dsts, uint64_t& dust)
    {
      splitted_dsts.clear();
      dust = 0;

      BOOST_FOREACH(auto& de, dsts)
      {
        cryptonote::decompose_amount_into_digits(de.amount, dust_threshold,
          [&](uint64_t chunk) { splitted_dsts.push_back(cryptonote::tx_destination_entry(chunk, de.addr)); },
          [&](uint64_t a_dust) { splitted_dsts.push_back(cryptonote::tx_destination_entry(a_dust, de.addr)); } );
      }

      cryptonote::decompose_amount_into_digits(change_dst.amount, dust_threshold,
        [&](uint64_t chunk) { splitted_dsts.push_back(cryptonote::tx_destination_entry(chunk, change_dst.addr)); },
        [&](uint64_t a_dust) { dust = a_dust; } );
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
  template<typename T>
  bool wallet2::enum_incoming_transfers(const T& handler) const
  {
    if(!m_transfers.empty())
    {
      BOOST_FOREACH(const transfer_details& td, m_transfers)
      {
        handler(td.m_tx, td.m_global_output_index, td.amount(), td.m_spent);
      }
      return true;
    }
    else
    {
      return false;
    }
  }
  //----------------------------------------------------------------------------------------------------
  template<typename T>
  bool wallet2::transfer(const std::vector<cryptonote::tx_destination_entry>& dsts, size_t fake_outputs_count,
    uint64_t unlock_time, uint64_t fee, T destination_split_strategy, const tx_dust_policy& dust_policy)
  {
    cryptonote::transaction tx;
    return transfer(dsts, fake_outputs_count, unlock_time, fee, destination_split_strategy, dust_policy, tx);
  }

  template<typename T>
  bool wallet2::transfer(const std::vector<cryptonote::tx_destination_entry>& dsts, size_t fake_outputs_count,
    uint64_t unlock_time, uint64_t fee, T destination_split_strategy, const tx_dust_policy& dust_policy, cryptonote::transaction &tx)
  {
    fail_details stub = AUTO_VAL_INIT(stub);
    return transfer(dsts, fake_outputs_count, unlock_time, fee, destination_split_strategy, dust_policy, tx, stub);
  }

  template<typename T>
  bool wallet2::transfer(const std::vector<cryptonote::tx_destination_entry>& dsts, size_t fake_outputs_count,
    uint64_t unlock_time, uint64_t fee, T destination_split_strategy, const tx_dust_policy& dust_policy, cryptonote::transaction &tx, fail_details& tfd)
  {
    using namespace cryptonote;

    uint64_t needed_money = fee;
    BOOST_FOREACH(auto& dt, dsts)
    {
      CHECK_AND_ASSERT_MES(dt.amount > 0, false, "Wrong destination amount value: " << dt.amount);
      needed_money += dt.amount;
    }

    std::list<transfer_container::iterator> selected_transfers;
    uint64_t found_money = select_transfers(needed_money, 0 == fake_outputs_count, dust_policy.dust_threshold, selected_transfers);

    if(found_money < needed_money)
    {
      LOG_ERROR("not enough money, available only " << print_money(found_money) << ", transaction amount " <<
        print_money(needed_money) << " = " << print_money(needed_money - fee) << " + " << print_money(fee) << " (fee)");
      tfd.reason = fail_details::error_not_enough_money;
      return false;
    }
    //typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount outs_for_amount;
    typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry out_entry;
    typedef cryptonote::tx_source_entry::output_entry tx_output_entry;

    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response daemon_resp = AUTO_VAL_INIT(daemon_resp);
    if(fake_outputs_count)
    {
      COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request req = AUTO_VAL_INIT(req);
      req.outs_count = fake_outputs_count + 1;// add one to make possible (if need) to skip real output key
      BOOST_FOREACH(transfer_container::iterator it, selected_transfers)
      {
        CHECK_AND_ASSERT_MES(it->m_tx.vout.size() > it->m_internal_output_index, false, "internal error: m_internal_output_index = "
          << it->m_internal_output_index << " more than " << it->m_tx.vout.size());
        req.amounts.push_back(it->amount());
      }

      bool r = net_utils::invoke_http_bin_remote_command2(m_daemon_address + "/getrandom_outs.bin", req, daemon_resp, m_http_client, 200000);
      if (!r)                                              tfd.reason = fail_details::error_not_connected;
      else if (CORE_RPC_STATUS_BUSY == daemon_resp.status) tfd.reason = fail_details::error_daemon_is_busy;
      else if (CORE_RPC_STATUS_OK != daemon_resp.status)   tfd.reason = fail_details::error_internal_error;
      else                                                 tfd.reason = fail_details::error_ok;
      if (fail_details::error_ok != tfd.reason)
      {
        LOG_PRINT_L0("failed to invoke getrandom_outs.bin: " << interpret_rpc_response(r, daemon_resp.status));
        return false;
      }

      tfd.reason = fail_details::error_internal_error;
      CHECK_AND_ASSERT_MES(daemon_resp.outs.size() == selected_transfers.size(), false,
        "internal error: daemon returned wrong response for getrandom_outs.bin, wrong amounts count = "
        << daemon_resp.outs.size() << ", expected " << selected_transfers.size());

      tfd.reason = fail_details::error_ok;
      BOOST_FOREACH(COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& amount_outs, daemon_resp.outs)
      {
        if (amount_outs.outs.size() != fake_outputs_count)
        {
          tfd.reason = fail_details::error_too_big_mixin;
          LOG_PRINT_L0("not enough outputs to mix output " << print_money(amount_outs.amount) << ", requested " <<
            fake_outputs_count << ", found " << amount_outs.outs.size());
        }
      }
      if (fail_details::error_ok != tfd.reason)
        return false;
    }
    tfd.reason = fail_details::error_ok;
 
    //prepare inputs
    size_t i = 0;
    std::vector<cryptonote::tx_source_entry> sources;
    BOOST_FOREACH(transfer_container::iterator it, selected_transfers)
    {
      sources.resize(sources.size()+1);
      cryptonote::tx_source_entry& src = sources.back();
      transfer_details& td = *it;
      src.amount = td.amount();
      //paste mixin transaction
      if(daemon_resp.outs.size())
      {
        daemon_resp.outs[i].outs.sort([](const out_entry& a, const out_entry& b){return a.global_amount_index < b.global_amount_index;});
        BOOST_FOREACH(out_entry& daemon_oe, daemon_resp.outs[i].outs)
        {
          if(td.m_global_output_index == daemon_oe.global_amount_index)
            continue;
          tx_output_entry oe;
          oe.first = daemon_oe.global_amount_index;
          oe.second = daemon_oe.out_key;
          src.outputs.push_back(oe);
          if(src.outputs.size() >= fake_outputs_count)
            break;
        }
      }

      //paste real transaction to the random index
      auto it_to_insert = std::find_if(src.outputs.begin(), src.outputs.end(), [&](const tx_output_entry& a)
      {
        return a.first >= td.m_global_output_index;
      });
      //size_t real_index = src.outputs.size() ? (rand() % src.outputs.size() ):0;
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

    cryptonote::tx_destination_entry change_dts = AUTO_VAL_INIT(change_dts);
    if (needed_money < found_money)
    {
      change_dts.addr = m_account.get_keys().m_account_address;
      change_dts.amount = found_money - needed_money;
    }

    uint64_t dust = 0;
    std::vector<cryptonote::tx_destination_entry> splitted_dsts;
    destination_split_strategy(dsts, change_dts, dust_policy.dust_threshold, splitted_dsts, dust);
    CHECK_AND_ASSERT_MES(dust <= dust_policy.dust_threshold, false, "internal error: invalid dust value");
    if (0 != dust && !dust_policy.add_to_fee)
    {
      splitted_dsts.push_back(cryptonote::tx_destination_entry(dust, dust_policy.addr_for_dust));
    }

    tfd.reason = fail_details::error_internal_error;
    bool r = cryptonote::construct_tx(m_account.get_keys(), sources, splitted_dsts, tx, unlock_time);
    CHECK_AND_ASSERT_MES(r, false, "Transaction construction failed");

    //check transaction size
    if(get_object_blobsize(tx) >= m_upper_transaction_size_limit)
    {
      LOG_PRINT_L0("Transaction size is too big: " << get_object_blobsize(tx)  << ", expected size < " << m_upper_transaction_size_limit);
      tfd.reason = fail_details::error_too_big_transaction;
      tfd.tx_blob_size = get_object_blobsize(tx);
      tfd.max_expected_tx_blob_size = m_upper_transaction_size_limit;
      return false;
    }

    COMMAND_RPC_SEND_RAW_TX::request req;
    req.tx_as_hex = epee::string_tools::buff_to_hex_nodelimer(tx_to_blob(tx));
    COMMAND_RPC_SEND_RAW_TX::response daemon_send_resp;
    r = net_utils::invoke_http_json_remote_command2(m_daemon_address + "/sendrawtransaction", req, daemon_send_resp, m_http_client, 200000);
    if (!r)
    {
      tfd.reason = fail_details::error_not_connected;
      LOG_PRINT_L0("failed to send transaction: " << interpret_rpc_response(r, daemon_send_resp.status));
      return false;
    }
    else if (CORE_RPC_STATUS_BUSY == daemon_send_resp.status)
    {
      tfd.reason = fail_details::error_daemon_is_busy;
      LOG_PRINT_L0("failed to send transaction: " << interpret_rpc_response(r, daemon_send_resp.status));
      return false;
    }
    else if (CORE_RPC_STATUS_OK != daemon_send_resp.status)
    {
      tfd.reason = fail_details::error_rejected_by_daemon;
      LOG_ERROR("daemon failed to accept generated transaction, id: " << get_transaction_hash(tx));
      return false;
    }
    else
    {
      tfd.reason = fail_details::error_ok;
    }

    std::string key_images;
    std::for_each(tx.vin.begin(), tx.vin.end(), [&](const txin_v& s_e) -> bool
    {
      CHECKED_GET_SPECIFIC_VARIANT(s_e, const txin_to_key, in, false);
      key_images += boost::to_string(in.k_image) + " ";
      return true;
    });
    LOG_PRINT_L2("transaction " << get_transaction_hash(tx) << " generated ok and sent to daemon, key_images: [" << key_images << "]");

    BOOST_FOREACH(transfer_container::iterator it, selected_transfers)
      it->m_spent = true;

    LOG_PRINT_L0("Transaction successfully sent. <" << get_transaction_hash(tx) << ">" << ENDL 
                  << "Commission: " << print_money(fee+dust) << " (dust: " << print_money(dust) << ")" << ENDL
                  << "Balance: " << print_money(balance()) << ENDL
                  << "Unlocked: " << print_money(unlocked_balance()) << ENDL
                  << "Please, wait for confirmation for your balance to be unlocked.");

    tfd.reason = fail_details::error_ok;
    return true;
  }
}
