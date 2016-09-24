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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include <random>
#include <tuple>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <boost/utility/value_init.hpp>
#include "include_base_utils.h"
using namespace epee;

#include "cryptonote_config.h"
#include "wallet2.h"
#include "cryptonote_core/cryptonote_format_utils.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "misc_language.h"
#include "cryptonote_core/cryptonote_basic_impl.h"
#include "common/boost_serialization_helper.h"
#include "profile_tools.h"
#include "crypto/crypto.h"
#include "serialization/binary_utils.h"
#include "cryptonote_protocol/blobdatatype.h"
#include "mnemonics/electrum-words.h"
#include "common/dns_utils.h"
#include "common/util.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "common/json_util.h"
#include "common/base58.h"
#include "ringct/rctSigs.h"

extern "C"
{
#include "crypto/keccak.h"
#include "crypto/crypto-ops.h"
}
using namespace cryptonote;

// used to choose when to stop adding outputs to a tx
#define APPROXIMATE_INPUT_BYTES 80

// used to target a given block size (additional outputs may be added on top to build fee)
#define TX_SIZE_TARGET(bytes) (bytes*2/3)

// arbitrary, used to generate different hashes from the same input
#define CHACHA8_KEY_TAIL 0x8c

#define KILL_IOSERVICE()  \
    do { \
      work.reset(); \
      while (!ioservice.stopped()) ioservice.poll(); \
      threadpool.join_all(); \
      ioservice.stop(); \
    } while(0)

namespace
{
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

} //namespace

namespace tools
{
// for now, limit to 30 attempts.  TODO: discuss a good number to limit to.
const size_t MAX_SPLIT_ATTEMPTS = 30;

//----------------------------------------------------------------------------------------------------
void wallet2::init(const std::string& daemon_address, uint64_t upper_transaction_size_limit)
{
  m_upper_transaction_size_limit = upper_transaction_size_limit;
  m_daemon_address = daemon_address;
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
void wallet2::set_spent(transfer_details &td, uint64_t height)
{
  LOG_PRINT_L2("Setting SPENT at " << height << ": ki " << td.m_key_image << ", amount " << print_money(td.m_amount));
  td.m_spent = true;
  td.m_spent_height = height;
}
//----------------------------------------------------------------------------------------------------
void wallet2::set_unspent(transfer_details &td)
{
  LOG_PRINT_L2("Setting UNSPENT: ki " << td.m_key_image << ", amount " << print_money(td.m_amount));
  td.m_spent = false;
  td.m_spent_height = 0;
}
//----------------------------------------------------------------------------------------------------
void wallet2::check_acc_out(const account_keys &acc, const tx_out &o, const crypto::public_key &tx_pub_key, size_t i, bool &received, uint64_t &money_transfered, bool &error) const
{
  if (o.target.type() !=  typeid(txout_to_key))
  {
     error = true;
     LOG_ERROR("wrong type id in transaction out");
     return;
  }
  received = is_out_to_acc(acc, boost::get<txout_to_key>(o.target), tx_pub_key, i);
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
static uint64_t decodeRct(const rct::rctSig & rv, const crypto::public_key pub, const crypto::secret_key &sec, unsigned int i, rct::key & mask)
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
void wallet2::process_new_transaction(const cryptonote::transaction& tx, const std::vector<uint64_t> &o_indices, uint64_t height, uint64_t ts, bool miner_tx, bool pool)
{
  class lazy_txid_getter
  {
    const cryptonote::transaction &tx;
    crypto::hash lazy_txid;
    bool computed;
    public:
    lazy_txid_getter(const transaction &tx): tx(tx), computed(false) {}
    const crypto::hash &operator()()
    {
      if (!computed)
      {
        lazy_txid = cryptonote::get_transaction_hash(tx);
        computed = true;
      }
      return lazy_txid;
    }
  } txid(tx);

  if (!miner_tx)
    process_unconfirmed(tx, height);
  std::vector<size_t> outs;
  uint64_t tx_money_got_in_outs = 0;
  crypto::public_key tx_pub_key = null_pkey;

  std::vector<tx_extra_field> tx_extra_fields;
  if(!parse_tx_extra(tx.extra, tx_extra_fields))
  {
    // Extra may only be partially parsed, it's OK if tx_extra_fields contains public key
    LOG_PRINT_L0("Transaction extra has unsupported format: " << txid());
  }

  // Don't try to extract tx public key if tx has no ouputs
  if (!tx.vout.empty()) 
  {
    tx_extra_pub_key pub_key_field;
    if(!find_tx_extra_field_by_type(tx_extra_fields, pub_key_field))
    {
      LOG_PRINT_L0("Public key wasn't found in the transaction extra. Skipping transaction " << txid());
      if(0 != m_callback)
	m_callback->on_skip_transaction(height, tx);
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
    if (miner_tx && m_refresh_type == RefreshNoCoinbase)
    {
      // assume coinbase isn't for us
    }
    else if (miner_tx && m_refresh_type == RefreshOptimizeCoinbase)
    {
      uint64_t money_transfered = 0;
      bool error = false, received = false;
      check_acc_out(m_account.get_keys(), tx.vout[0], tx_pub_key, 0, received, money_transfered, error);
      if (error)
      {
        r = false;
      }
      else
      {
        // this assumes that the miner tx pays a single address
        if (received)
        {
          cryptonote::generate_key_image_helper(m_account.get_keys(), tx_pub_key, 0, in_ephemeral[0], ki[0]);
          THROW_WALLET_EXCEPTION_IF(in_ephemeral[0].pub != boost::get<cryptonote::txout_to_key>(tx.vout[0].target).key,
              error::wallet_internal_error, "key_image generated ephemeral public key not matched with output_key");

          outs.push_back(0);
          if (money_transfered == 0)
          {
            const cryptonote::account_keys& keys = m_account.get_keys();
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

          const account_keys &keys = m_account.get_keys();
          std::vector<uint64_t> money_transfered(tx.vout.size());
          std::deque<bool> error(tx.vout.size());
          std::deque<bool> received(tx.vout.size());
          // the first one was already checked
          for (size_t i = 1; i < tx.vout.size(); ++i)
          {
            ioservice.dispatch(boost::bind(&wallet2::check_acc_out, this, std::cref(keys), std::cref(tx.vout[i]), std::cref(tx_pub_key), i,
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
              cryptonote::generate_key_image_helper(m_account.get_keys(), tx_pub_key, i, in_ephemeral[i], ki[i]);
              THROW_WALLET_EXCEPTION_IF(in_ephemeral[i].pub != boost::get<cryptonote::txout_to_key>(tx.vout[i].target).key,
                  error::wallet_internal_error, "key_image generated ephemeral public key not matched with output_key");

              outs.push_back(i);
              if (money_transfered[i] == 0)
              {
                const cryptonote::account_keys& keys = m_account.get_keys();
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

      const account_keys &keys = m_account.get_keys();
      std::vector<uint64_t> money_transfered(tx.vout.size());
      std::deque<bool> error(tx.vout.size());
      std::deque<bool> received(tx.vout.size());
      for (size_t i = 0; i < tx.vout.size(); ++i)
      {
        ioservice.dispatch(boost::bind(&wallet2::check_acc_out, this, std::cref(keys), std::cref(tx.vout[i]), std::cref(tx_pub_key), i,
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
          cryptonote::generate_key_image_helper(m_account.get_keys(), tx_pub_key, i, in_ephemeral[i], ki[i]);
          THROW_WALLET_EXCEPTION_IF(in_ephemeral[i].pub != boost::get<cryptonote::txout_to_key>(tx.vout[i].target).key,
              error::wallet_internal_error, "key_image generated ephemeral public key not matched with output_key");

          outs.push_back(i);
          if (money_transfered[i] == 0)
          {
            const cryptonote::account_keys& keys = m_account.get_keys();
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
        check_acc_out(m_account.get_keys(), tx.vout[i], tx_pub_key, i, received, money_transfered, error);
        if (error)
        {
          r = false;
          break;
        }
        else
        {
          if (received)
          {
            cryptonote::generate_key_image_helper(m_account.get_keys(), tx_pub_key, i, in_ephemeral[i], ki[i]);
            THROW_WALLET_EXCEPTION_IF(in_ephemeral[i].pub != boost::get<cryptonote::txout_to_key>(tx.vout[i].target).key,
                error::wallet_internal_error, "key_image generated ephemeral public key not matched with output_key");

            outs.push_back(i);
            if (money_transfered == 0)
            {
              const cryptonote::account_keys& keys = m_account.get_keys();
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

      BOOST_FOREACH(size_t o, outs)
      {
	THROW_WALLET_EXCEPTION_IF(tx.vout.size() <= o, error::wallet_internal_error, "wrong out in transaction: internal index=" +
				  std::to_string(o) + ", total_outs=" + std::to_string(tx.vout.size()));

        auto kit = m_key_images.find(ki[o]);
	THROW_WALLET_EXCEPTION_IF(kit != m_key_images.end() && kit->second >= m_transfers.size(),
            error::wallet_internal_error, std::string("Unexpected transfer index from key image: ")
            + "got " + (kit == m_key_images.end() ? "<none>" : boost::lexical_cast<std::string>(kit->second))
            + ", m_transfers.size() is " + boost::lexical_cast<std::string>(m_transfers.size()));
        if (kit == m_key_images.end())
        {
          if (!pool)
          {
	    m_transfers.push_back(boost::value_initialized<transfer_details>());
	    transfer_details& td = m_transfers.back();
	    td.m_block_height = height;
	    td.m_internal_output_index = o;
	    td.m_global_output_index = o_indices[o];
	    td.m_tx = (const cryptonote::transaction_prefix&)tx;
	    td.m_txid = txid();
            td.m_key_image = ki[o];
            td.m_amount = tx.vout[o].amount;
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
	    set_unspent(td);
	    m_key_images[td.m_key_image] = m_transfers.size()-1;
	    LOG_PRINT_L0("Received money: " << print_money(td.amount()) << ", with tx: " << txid());
	    if (0 != m_callback)
	      m_callback->on_money_received(height, tx, td.m_amount);
          }
        }
	else if (m_transfers[kit->second].m_spent || m_transfers[kit->second].amount() >= tx.vout[o].amount)
        {
	  LOG_ERROR("key image " << epee::string_tools::pod_to_hex(ki)
              << " from received " << print_money(tx.vout[o].amount) << " output already exists with "
              << (m_transfers[kit->second].m_spent ? "spent" : "unspent") << " "
              << print_money(m_transfers[kit->second].amount()) << ", received output ignored");
        }
        else
        {
	  LOG_ERROR("key image " << epee::string_tools::pod_to_hex(ki)
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
	    td.m_txid = txid();
            td.m_amount = tx.vout[o].amount;
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
            THROW_WALLET_EXCEPTION_IF(td.m_key_image != ki[o], error::wallet_internal_error, "Inconsistent key images");
	    THROW_WALLET_EXCEPTION_IF(td.m_spent, error::wallet_internal_error, "Inconsistent spent status");

	    LOG_PRINT_L0("Received money: " << print_money(td.amount()) << ", with tx: " << txid());
	    if (0 != m_callback)
	      m_callback->on_money_received(height, tx, td.m_amount);
          }
        }
      }
    }
  }

  uint64_t tx_money_spent_in_ins = 0;
  // check all outputs for spending (compare key images)
  BOOST_FOREACH(auto& in, tx.vin)
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
      LOG_PRINT_L0("Spent money: " << print_money(amount) << ", with tx: " << txid());
      tx_money_spent_in_ins += amount;
      set_spent(td, height);
      if (0 != m_callback)
        m_callback->on_money_spent(height, tx, amount, tx);
    }
  }

  if (tx_money_spent_in_ins > 0)
  {
    process_outgoing(tx, height, ts, tx_money_spent_in_ins, tx_money_got_in_outs);
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
    payment.m_tx_hash      = txid();
    payment.m_amount       = received;
    payment.m_block_height = height;
    payment.m_unlock_time  = tx.unlock_time;
    payment.m_timestamp    = ts;
    if (pool)
      m_unconfirmed_payments.emplace(payment_id, payment);
    else
      m_payments.emplace(payment_id, payment);
    LOG_PRINT_L2("Payment found in " << (pool ? "pool" : "block") << ": " << payment_id << " / " << payment.m_tx_hash << " / " << payment.m_amount);
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::process_unconfirmed(const cryptonote::transaction& tx, uint64_t height)
{
  if (m_unconfirmed_txs.empty())
    return;

  crypto::hash txid = get_transaction_hash(tx);
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
void wallet2::process_outgoing(const cryptonote::transaction &tx, uint64_t height, uint64_t ts, uint64_t spent, uint64_t received)
{
  crypto::hash txid = get_transaction_hash(tx);
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
  }
  entry.first->second.m_block_height = height;
  entry.first->second.m_timestamp = ts;
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
    process_new_transaction(b.miner_tx, o_indices.indices[txidx++].indices, height, b.timestamp, true, false);
    TIME_MEASURE_FINISH(miner_tx_handle_time);

    TIME_MEASURE_START(txs_handle_time);
    BOOST_FOREACH(auto& txblob, bche.txs)
    {
      cryptonote::transaction tx;
      bool r = parse_and_validate_tx_from_blob(txblob, tx);
      THROW_WALLET_EXCEPTION_IF(!r, error::tx_parse_error, txblob);
      process_new_transaction(tx, o_indices.indices[txidx++].indices, height, b.timestamp, false, false);
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

  req.start_height = start_height;
  m_daemon_rpc_mutex.lock();
  bool r = net_utils::invoke_http_bin_remote_command2(m_daemon_address + "/getblocks.bin", req, res, m_http_client, WALLET_RCP_CONNECTION_TIMEOUT);
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
  bool r = net_utils::invoke_http_bin_remote_command2(m_daemon_address + "/gethashes.bin", req, res, m_http_client, WALLET_RCP_CONNECTION_TIMEOUT);
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
  BOOST_FOREACH(auto& bl_entry, blocks)
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
void wallet2::update_pool_state()
{
  // get the pool state
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::request req;
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::response res;
  m_daemon_rpc_mutex.lock();
  bool r = epee::net_utils::invoke_http_json_remote_command2(m_daemon_address + "/get_transaction_pool", req, res, m_http_client, 200000);
  m_daemon_rpc_mutex.unlock();
  THROW_WALLET_EXCEPTION_IF(!r, error::no_connection_to_daemon, "get_transaction_pool");
  THROW_WALLET_EXCEPTION_IF(res.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "get_transaction_pool");
  THROW_WALLET_EXCEPTION_IF(res.status != CORE_RPC_STATUS_OK, error::get_tx_pool_error);

  // remove any pending tx that's not in the pool
  std::unordered_map<crypto::hash, wallet2::unconfirmed_transfer_details>::iterator it = m_unconfirmed_txs.begin();
  while (it != m_unconfirmed_txs.end())
  {
    const std::string txid = epee::string_tools::pod_to_hex(it->first);
    bool found = false;
    for (auto it2: res.transactions)
    {
      if (it2.id_hash == txid)
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
      // delete it the second time it is checked
      if (pit->second.m_state == wallet2::unconfirmed_transfer_details::pending)
      {
        LOG_PRINT_L1("Pending txid " << txid << " not in pool, marking as not in pool");
        pit->second.m_state = wallet2::unconfirmed_transfer_details::pending_not_in_pool;
      }
      else if (pit->second.m_state == wallet2::unconfirmed_transfer_details::pending_not_in_pool)
      {
        LOG_PRINT_L1("Pending txid " << txid << " not in pool, marking as failed");
        pit->second.m_state = wallet2::unconfirmed_transfer_details::failed;

        // the inputs aren't spent anymore, since the tx failed
        for (size_t vini = 0; vini < pit->second.m_tx.vin.size(); ++vini)
        {
          if (pit->second.m_tx.vin[vini].type() == typeid(txin_to_key))
          {
            txin_to_key &tx_in_to_key = boost::get<txin_to_key>(pit->second.m_tx.vin[vini]);
            for (auto &td: m_transfers)
            {
              if (td.m_key_image == tx_in_to_key.k_image)
              {
                 LOG_PRINT_L1("Resetting spent status for output " << vini << ": " << td.m_key_image);
                 set_unspent(td);
                 break;
              }
            }
          }
        }
      }
    }
  }

  // remove pool txes to us that aren't in the pool anymore
  std::unordered_map<crypto::hash, wallet2::payment_details>::iterator uit = m_unconfirmed_payments.begin();
  while (uit != m_unconfirmed_payments.end())
  {
    const std::string txid = string_tools::pod_to_hex(uit->first);
    bool found = false;
    for (auto it2: res.transactions)
    {
      if (it2.id_hash == txid)
      {
        found = true;
        break;
      }
    }
    auto pit = uit++;
    if (!found)
    {
      m_unconfirmed_payments.erase(pit);
    }
  }

  // add new pool txes to us
  for (auto it: res.transactions)
  {
    cryptonote::blobdata txid_data;
    if(epee::string_tools::parse_hexstr_to_binbuff(it.id_hash, txid_data))
    {
      const crypto::hash txid = *reinterpret_cast<const crypto::hash*>(txid_data.data());
      if (m_unconfirmed_payments.find(txid) == m_unconfirmed_payments.end())
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
          cryptonote::COMMAND_RPC_GET_TRANSACTIONS::request req;
          cryptonote::COMMAND_RPC_GET_TRANSACTIONS::response res;
          req.txs_hashes.push_back(it.id_hash);
          req.decode_as_json = false;
          m_daemon_rpc_mutex.lock();
          bool r = epee::net_utils::invoke_http_json_remote_command2(m_daemon_address + "/gettransactions", req, res, m_http_client, 200000);
          m_daemon_rpc_mutex.unlock();
          if (r && res.status == CORE_RPC_STATUS_OK)
          {
            if (res.txs.size() == 1)
            {
              // might have just been put in a block
              if (res.txs[0].in_pool)
              {
                cryptonote::transaction tx;
                cryptonote::blobdata bd;
                crypto::hash tx_hash, tx_prefix_hash;
                if (epee::string_tools::parse_hexstr_to_binbuff(res.txs[0].as_hex, bd))
                {
                  if (cryptonote::parse_and_validate_tx_from_blob(bd, tx, tx_hash, tx_prefix_hash))
                  {
                    if (tx_hash == txid)
                    {
                      process_new_transaction(tx, std::vector<uint64_t>(), 0, time(NULL), false, true);
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
            }
            else
            {
              LOG_PRINT_L0("Expected 1 tx, got " << res.txs.size());
            }
          }
          else
          {
            LOG_PRINT_L0("Error calling gettransactions daemon RPC: r " << r << ", status " << res.status);
          }
        }
        else
        {
          LOG_PRINT_L1("We sent that one");
        }
      }
      else
      {
        LOG_PRINT_L1("Already saw that one");
      }
    }
    else
    {
      LOG_PRINT_L0("Failed to parse txid");
    }
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::fast_refresh(uint64_t stop_height, uint64_t &blocks_start_height, std::list<crypto::hash> &short_chain_history)
{
  std::list<crypto::hash> hashes;
  size_t current_index = m_blockchain.size();

  while(m_run.load(std::memory_order_relaxed) && current_index < stop_height)
  {
    pull_hashes(0, blocks_start_height, short_chain_history, hashes);
    if (hashes.size() < 3)
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
    BOOST_FOREACH(auto& bl_id, hashes)
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
      if(!added_blocks)
        break;

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
    update_pool_state();
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
      set_unspent(td);
    }
  }

  auto it = std::find_if(m_transfers.begin(), m_transfers.end(), [&](const transfer_details& td){return td.m_block_height >= height;});
  size_t i_start = it - m_transfers.begin();

  for(size_t i = i_start; i!= m_transfers.size();i++)
  {
    auto it_ki = m_key_images.find(m_transfers[i].m_key_image);
    THROW_WALLET_EXCEPTION_IF(it_ki == m_key_images.end(), error::wallet_internal_error, "key image not found");
    m_key_images.erase(it_ki);
    ++transfers_detached;
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
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::clear()
{
  m_blockchain.clear();
  m_transfers.clear();
  m_key_images.clear();
  m_unconfirmed_txs.clear();
  m_payments.clear();
  m_tx_keys.clear();
  m_confirmed_txs.clear();
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
    m_default_mixin = 0;
    m_default_priority = 0;
    m_auto_refresh = true;
    m_refresh_type = RefreshType::RefreshDefault;
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
    GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, always_confirm_transfers, int, Int, false, false);
    m_always_confirm_transfers = field_always_confirm_transfers_found && field_always_confirm_transfers;
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
    if (field_refresh_height_found)
      m_refresh_from_block_height = field_refresh_height;
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
 *
 * for verification only
 * should not mutate state, unlike load_keys()
 * can be used prior to rewriting wallet keys file, to ensure user has entered the correct password
 *
 */
bool wallet2::verify_password(const std::string& password) const
{
  const std::string keys_file_name = m_keys_file;
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

  bool r = store_keys(m_keys_file, password, false);
  THROW_WALLET_EXCEPTION_IF(!r, error::file_save_error, m_keys_file);

  r = file_io_utils::save_string_to_file(m_wallet_file + ".address.txt", m_account.get_public_address_str(m_testnet));
  if(!r) LOG_PRINT_RED_L0("String with address text not saved");

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
  if(!r) LOG_PRINT_RED_L0("String with address text not saved");

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
  if(!r) LOG_PRINT_RED_L0("String with address text not saved");

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
  bool r = store_keys(m_keys_file, password, false);
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
bool wallet2::prepare_file_names(const std::string& file_path)
{
  do_prepare_file_names(file_path, m_keys_file, m_wallet_file);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::check_connection(bool *same_version)
{
  boost::lock_guard<boost::mutex> lock(m_daemon_rpc_mutex);

  if(!m_http_client.is_connected())
  {
    net_utils::http::url_content u;
    net_utils::parse_url(m_daemon_address, u);

    if(!u.port)
    {
      u.port = m_testnet ? config::testnet::RPC_DEFAULT_PORT : config::RPC_DEFAULT_PORT;
    }

    if (!m_http_client.connect(u.host, std::to_string(u.port), WALLET_RCP_CONNECTION_TIMEOUT))
      return false;
  }

  if (same_version)
  {
    epee::json_rpc::request<cryptonote::COMMAND_RPC_GET_VERSION::request> req_t = AUTO_VAL_INIT(req_t);
    epee::json_rpc::response<cryptonote::COMMAND_RPC_GET_VERSION::response, std::string> resp_t = AUTO_VAL_INIT(resp_t);
    req_t.jsonrpc = "2.0";
    req_t.id = epee::serialization::storage_entry(0);
    req_t.method = "get_version";
    bool r = net_utils::invoke_http_json_remote_command2(m_daemon_address + "/json_rpc", req_t, resp_t, m_http_client);
    if (!r || resp_t.result.status != CORE_RPC_STATUS_OK)
      *same_version = false;
    else
      *same_version = resp_t.result.version == CORE_RPC_VERSION;
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
      boost::archive::binary_iarchive ar(iss);
      ar >> *this;
    }
    catch (...)
    {
      LOG_PRINT_L1("Failed to load encrypted cache, trying unencrypted");
      std::stringstream iss;
      iss << buf;
      boost::archive::binary_iarchive ar(iss);
      ar >> *this;
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
  std::string what("Genesis block missmatch. You probably use wallet without testnet flag with blockchain from test network or vice versa");

  THROW_WALLET_EXCEPTION_IF(genesis_hash != m_blockchain[0], error::wallet_internal_error, what);
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
  boost::archive::binary_oarchive ar(oss);
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
  BOOST_FOREACH(const transfer_details& td, m_transfers)
    if(!td.m_spent && is_transfer_unlocked(td))
      amount += td.amount();

  return amount;
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::balance() const
{
  uint64_t amount = 0;
  BOOST_FOREACH(auto& td, m_transfers)
    if(!td.m_spent)
      amount += td.amount();


  BOOST_FOREACH(auto& utx, m_unconfirmed_txs)
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
  std::vector<std::string> key_images;

  // make a list of key images for all our outputs
  for (size_t i = 0; i < m_transfers.size(); ++i)
  {
    const transfer_details& td = m_transfers[i];
    key_images.push_back(string_tools::pod_to_hex(td.m_key_image));
  }

  COMMAND_RPC_IS_KEY_IMAGE_SPENT::request req = AUTO_VAL_INIT(req);
  COMMAND_RPC_IS_KEY_IMAGE_SPENT::response daemon_resp = AUTO_VAL_INIT(daemon_resp);
  req.key_images = key_images;
  m_daemon_rpc_mutex.lock();
  bool r = epee::net_utils::invoke_http_json_remote_command2(m_daemon_address + "/is_key_image_spent", req, daemon_resp, m_http_client, 200000);
  m_daemon_rpc_mutex.unlock();
  THROW_WALLET_EXCEPTION_IF(!r, error::no_connection_to_daemon, "is_key_image_spent");
  THROW_WALLET_EXCEPTION_IF(daemon_resp.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "is_key_image_spent");
  THROW_WALLET_EXCEPTION_IF(daemon_resp.status != CORE_RPC_STATUS_OK, error::is_key_image_spent_error, daemon_resp.status);
  THROW_WALLET_EXCEPTION_IF(daemon_resp.spent_status.size() != key_images.size(), error::wallet_internal_error,
    "daemon returned wrong response for is_key_image_spent, wrong amounts count = " +
    std::to_string(daemon_resp.spent_status.size()) + ", expected " +  std::to_string(key_images.size()));

  // update spent status
  for (size_t i = 0; i < m_transfers.size(); ++i)
  {
    transfer_details& td = m_transfers[i];
    if (td.m_spent != (daemon_resp.spent_status[i] != COMMAND_RPC_IS_KEY_IMAGE_SPENT::UNSPENT))
    {
      if (td.m_spent)
      {
        LOG_PRINT_L0("Marking output " << i << "(" << td.m_key_image << ") as unspent, it was marked as spent");
        set_unspent(td);
        td.m_spent_height = 0;
      }
      else
      {
        LOG_PRINT_L0("Marking output " << i << "(" << td.m_key_image << ") as spent, it was marked as unspent");
        set_spent(td, td.m_spent_height);
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
size_t wallet2::pop_best_value_from(const transfer_container &transfers, std::vector<size_t> &unused_indices, const std::list<transfer_container::iterator>& selected_transfers) const
{
  std::vector<size_t> candidates;
  float best_relatedness = 1.0f;
  for (size_t n = 0; n < unused_indices.size(); ++n)
  {
    const transfer_details &candidate = transfers[unused_indices[n]];
    float relatedness = 0.0f;
    for (const auto &i: selected_transfers)
    {
      float r = get_output_relatedness(candidate, *i);
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
  size_t idx = crypto::rand<size_t>() % candidates.size();
  return pop_index (unused_indices, candidates[idx]);
}
//----------------------------------------------------------------------------------------------------
size_t wallet2::pop_best_value(std::vector<size_t> &unused_indices, const std::list<transfer_container::iterator>& selected_transfers) const
{
  return pop_best_value_from(m_transfers, unused_indices, selected_transfers);
}
//----------------------------------------------------------------------------------------------------
// Select random input sources for transaction.
// returns:
//    direct return: amount of money found
//    modified reference: selected_transfers, a list of iterators/indices of input sources
uint64_t wallet2::select_transfers(uint64_t needed_money, std::vector<size_t> unused_transfers_indices, std::list<transfer_container::iterator>& selected_transfers, bool trusted_daemon)
{
  uint64_t found_money = 0;
  while (found_money < needed_money && !unused_transfers_indices.empty())
  {
    size_t idx = pop_best_value(unused_transfers_indices, selected_transfers);

    transfer_container::iterator it = m_transfers.begin() + idx;
    selected_transfers.push_back(it);
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

/**
 * @brief gets a monero address from the TXT record of a DNS entry
 *
 * gets the monero address from the TXT record of the DNS entry associated
 * with <url>.  If this lookup fails, or the TXT record does not contain an
 * XMR address in the correct format, returns an empty string.  <dnssec_valid>
 * will be set true or false according to whether or not the DNS query passes
 * DNSSEC validation.
 *
 * @param url the url to look up
 * @param dnssec_valid return-by-reference for DNSSEC status of query
 *
 * @return a monero address (as a string) or an empty string
 */
std::vector<std::string> wallet2::addresses_from_url(const std::string& url, bool& dnssec_valid)
{
  std::vector<std::string> addresses;
  // get txt records
  bool dnssec_available, dnssec_isvalid;
  std::string oa_addr = tools::DNSResolver::instance().get_dns_format_from_oa_address(url);
  auto records = tools::DNSResolver::instance().get_txt_record(oa_addr, dnssec_available, dnssec_isvalid);

  // TODO: update this to allow for conveying that dnssec was not available
  if (dnssec_available && dnssec_isvalid)
  {
    dnssec_valid = true;
  }
  else dnssec_valid = false;

  // for each txt record, try to find a monero address in it.
  for (auto& rec : records)
  {
    std::string addr = address_from_txt_record(rec);
    if (addr.size())
    {
      addresses.push_back(addr);
    }
  }

  return addresses;
}

//----------------------------------------------------------------------------------------------------
// TODO: parse the string in a less stupid way, probably with regex
std::string wallet2::address_from_txt_record(const std::string& s)
{
  // make sure the txt record has "oa1:xmr" and find it
  auto pos = s.find("oa1:xmr");

  // search from there to find "recipient_address="
  pos = s.find("recipient_address=", pos);

  pos += 18; // move past "recipient_address="

  // find the next semicolon
  auto pos2 = s.find(";", pos);
  if (pos2 != std::string::npos)
  {
    // length of address == 95, we can at least validate that much here
    if (pos2 - pos == 95)
    {
      return s.substr(pos, 95);
    }
  }
  return std::string();
}

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
  bool r = epee::net_utils::invoke_http_json_remote_command2(m_daemon_address + "/sendrawtransaction", req, daemon_send_resp, m_http_client, 200000);
  m_daemon_rpc_mutex.unlock();
  THROW_WALLET_EXCEPTION_IF(!r, error::no_connection_to_daemon, "sendrawtransaction");
  THROW_WALLET_EXCEPTION_IF(daemon_send_resp.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "sendrawtransaction");
  THROW_WALLET_EXCEPTION_IF(daemon_send_resp.status != CORE_RPC_STATUS_OK, error::tx_rejected, ptx.tx, daemon_send_resp.status, daemon_send_resp.reason);

  txid = get_transaction_hash(ptx.tx);
  crypto::hash payment_id = cryptonote::null_hash;
  std::vector<cryptonote::tx_destination_entry> dests;
  uint64_t amount_in = 0;
  if (store_tx_info())
  {
    payment_id = get_payment_id(ptx);
    dests = ptx.dests;
    BOOST_FOREACH(transfer_container::iterator it, ptx.selected_transfers)
      amount_in += it->amount();
  }
  add_unconfirmed_tx(ptx.tx, amount_in, dests, payment_id, ptx.change_dts.amount);
  if (store_tx_info())
  {
    m_tx_keys.insert(std::make_pair(txid, ptx.tx_key));
  }

  LOG_PRINT_L2("transaction " << txid << " generated ok and sent to daemon, key_images: [" << ptx.key_images << "]");

  BOOST_FOREACH(transfer_container::iterator it, ptx.selected_transfers)
  {
    set_spent(*it, 0);
  }

  //fee includes dust if dust policy specified it.
  LOG_PRINT_L0("Transaction successfully sent. <" << txid << ">" << ENDL
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

uint64_t wallet2::get_fee_multiplier(uint32_t priority, bool use_new_fee) const
{
  static const uint64_t old_multipliers[3] = {1, 2, 3};
  static const uint64_t new_multipliers[3] = {1, 20, 166};

  // 0 -> default (here, x1)
  if (priority == 0)
    priority = m_default_priority;
  if (priority == 0)
    priority = 1;

  // 1 to 3 are allowed as priorities
  if (priority >= 1 && priority <= 3)
    return (use_new_fee ? new_multipliers : old_multipliers)[priority-1];

  THROW_WALLET_EXCEPTION_IF (false, error::invalid_priority);
  return 1;
}

//----------------------------------------------------------------------------------------------------
// separated the call(s) to wallet2::transfer into their own function
//
// this function will make multiple calls to wallet2::transfer if multiple
// transactions will be required
std::vector<wallet2::pending_tx> wallet2::create_transactions(std::vector<cryptonote::tx_destination_entry> dsts, const size_t fake_outs_count, const uint64_t unlock_time, uint32_t priority, const std::vector<uint8_t> extra, bool trusted_daemon)
{
  const std::vector<size_t> unused_transfers_indices = select_available_outputs_from_histogram(fake_outs_count + 1, true, true, trusted_daemon);

  const bool use_new_fee  = use_fork_rules(3, -720 * 14);
  const uint64_t fee_per_kb  = use_new_fee ? FEE_PER_KB : FEE_PER_KB_OLD;
  const uint64_t fee_multiplier = get_fee_multiplier(priority, use_new_fee);

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
        BOOST_FOREACH(transfer_container::iterator it, ptx.selected_transfers)
        {
          set_spent(*it, 0);
        }
      }

      // if we made it this far, we've selected our transactions.  committing them will mark them spent,
      // so this is a failsafe in case they don't go through
      // unmark pending tx transfers as spent
      for (auto & ptx : ptx_vector)
      {
        // mark transfers to be used as not spent
        BOOST_FOREACH(transfer_container::iterator it2, ptx.selected_transfers)
        {
          set_unspent(*it2);
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
        BOOST_FOREACH(transfer_container::iterator it2, ptx.selected_transfers)
        {
          set_unspent(*it2);
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
        BOOST_FOREACH(transfer_container::iterator it2, ptx.selected_transfers)
        {
          set_unspent(*it2);
        }
      }

      throw;
    }
  }
}

template<typename entry>
void wallet2::get_outs(std::vector<std::vector<entry>> &outs, const std::list<transfer_container::iterator> &selected_transfers, size_t fake_outputs_count)
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
    for(auto it: selected_transfers)
      req_t.params.amounts.push_back(it->is_rct() ? 0 : it->amount());
    std::sort(req_t.params.amounts.begin(), req_t.params.amounts.end());
    auto end = std::unique(req_t.params.amounts.begin(), req_t.params.amounts.end());
    req_t.params.amounts.resize(std::distance(req_t.params.amounts.begin(), end));
    req_t.params.unlocked = true;
    bool r = net_utils::invoke_http_json_remote_command2(m_daemon_address + "/json_rpc", req_t, resp_t, m_http_client);
    m_daemon_rpc_mutex.unlock();
    THROW_WALLET_EXCEPTION_IF(!r, error::no_connection_to_daemon, "transfer_selected");
    THROW_WALLET_EXCEPTION_IF(resp_t.result.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "get_output_histogram");
    THROW_WALLET_EXCEPTION_IF(resp_t.result.status != CORE_RPC_STATUS_OK, error::get_histogram_error, resp_t.result.status);

    // we ask for more, to have spares if some outputs are still locked
    size_t base_requested_outputs_count = (size_t)((fake_outputs_count + 1) * 1.5 + 1);
    LOG_PRINT_L2("base_requested_outputs_count: " << base_requested_outputs_count);

    // generate output indices to request
    COMMAND_RPC_GET_OUTPUTS::request req = AUTO_VAL_INIT(req);
    COMMAND_RPC_GET_OUTPUTS::response daemon_resp = AUTO_VAL_INIT(daemon_resp);

    for(transfer_container::iterator it: selected_transfers)
    {
      const uint64_t amount = it->is_rct() ? 0 : it->amount();
      std::unordered_set<uint64_t> seen_indices;
      // request more for rct in base recent (locked) coinbases are picked, since they're locked for longer
      size_t requested_outputs_count = base_requested_outputs_count + (it->is_rct() ? CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW - CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE : 0);
      size_t start = req.outputs.size();

      // if there are just enough outputs to mix with, use all of them.
      // Eventually this should become impossible.
      uint64_t num_outs = 0;
      for (auto he: resp_t.result.histogram)
      {
        if (he.amount == amount)
        {
          num_outs = he.instances;
          break;
        }
      }
      LOG_PRINT_L1("" << num_outs << " outputs of size " << print_money(amount));
      THROW_WALLET_EXCEPTION_IF(num_outs == 0, error::wallet_internal_error,
          "histogram reports no outputs for " + boost::lexical_cast<std::string>(amount) + ", not even ours");

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
        seen_indices.emplace(it->m_global_output_index);
        req.outputs.push_back({amount, it->m_global_output_index});

        // while we still need more mixins
        while (num_found < requested_outputs_count)
        {
          // if we've gone through every possible output, we've gotten all we can
          if (seen_indices.size() == num_outs)
            break;

          // get a random output index from the DB.  If we've already seen it,
          // return to the top of the loop and try again, otherwise add it to the
          // list of output indices we've seen.

          // triangular distribution over [a,b) with a=0, mode c=b=up_index_limit
          uint64_t r = crypto::rand<uint64_t>() % ((uint64_t)1 << 53);
          double frac = std::sqrt((double)r / ((uint64_t)1 << 53));
          uint64_t i = (uint64_t)(frac*num_outs);
          // just in case rounding up to 1 occurs after sqrt
          if (i == num_outs)
            --i;

          if (seen_indices.count(i))
            continue;
          seen_indices.emplace(i);

          req.outputs.push_back({amount, i});
          ++num_found;
        }
      }

      // sort the subsection, to ensure the daemon doesn't know wich output is ours
      std::sort(req.outputs.begin() + start, req.outputs.end(),
          [](const COMMAND_RPC_GET_OUTPUTS::out &a, const COMMAND_RPC_GET_OUTPUTS::out &b) { return a.index < b.index; });
    }

    for (auto i: req.outputs)
      LOG_PRINT_L1("asking for output " << i.index << " for " << print_money(i.amount));

    // get the keys for those
    m_daemon_rpc_mutex.lock();
    r = epee::net_utils::invoke_http_bin_remote_command2(m_daemon_address + "/get_outs.bin", req, daemon_resp, m_http_client, 200000);
    m_daemon_rpc_mutex.unlock();
    THROW_WALLET_EXCEPTION_IF(!r, error::no_connection_to_daemon, "get_outs.bin");
    THROW_WALLET_EXCEPTION_IF(daemon_resp.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "get_outs.bin");
    THROW_WALLET_EXCEPTION_IF(daemon_resp.status != CORE_RPC_STATUS_OK, error::get_random_outs_error, daemon_resp.status);
    THROW_WALLET_EXCEPTION_IF(daemon_resp.outs.size() != req.outputs.size(), error::wallet_internal_error,
      "daemon returned wrong response for get_outs.bin, wrong amounts count = " +
      std::to_string(daemon_resp.outs.size()) + ", expected " +  std::to_string(req.outputs.size()));

    std::unordered_map<uint64_t, uint64_t> scanty_outs;
    size_t base = 0;
    outs.reserve(selected_transfers.size());
    for(transfer_container::iterator it: selected_transfers)
    {
      size_t requested_outputs_count = base_requested_outputs_count + (it->is_rct() ? CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW - CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE : 0);
      outs.push_back(std::vector<entry>());
      outs.back().reserve(fake_outputs_count + 1);
      const rct::key mask = it->is_rct() ? rct::commit(it->amount(), it->m_mask) : rct::zeroCommit(it->amount());

      // pick real out first (it will be sorted when done)
      outs.back().push_back(std::make_tuple(it->m_global_output_index, boost::get<txout_to_key>(it->m_tx.vout[it->m_internal_output_index].target).key, mask));

      // then pick others in random order till we reach the required number
      // since we use an equiprobable pick here, we don't upset the triangular distribution
      std::vector<size_t> order;
      order.resize(requested_outputs_count);
      for (size_t n = 0; n < order.size(); ++n)
        order[n] = n;
      std::shuffle(order.begin(), order.end(), std::default_random_engine(crypto::rand<unsigned>()));

      LOG_PRINT_L2("Looking for " << (fake_outputs_count+1) << " outputs of size " << print_money(it->is_rct() ? 0 : it->amount()));
      for (size_t o = 0; o < requested_outputs_count && outs.back().size() < fake_outputs_count + 1; ++o)
      {
        size_t i = base + order[o];
        LOG_PRINT_L2("Index " << i << "/" << requested_outputs_count << ": idx " << req.outputs[i].index << " (real " << it->m_global_output_index << "), unlocked " << daemon_resp.outs[i].unlocked << ", key " << daemon_resp.outs[i].key);
        if (req.outputs[i].index == it->m_global_output_index) // don't re-add real one
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
        scanty_outs[it->is_rct() ? 0 : it->amount()] = outs.back().size();
      }
      else
      {
        // sort the subsection, so any spares are reset in order
        std::sort(outs.back().begin(), outs.back().end(), [](const entry &a, const entry &b) { return std::get<0>(a) < std::get<0>(b); });
      }
      base += requested_outputs_count;
    }
    THROW_WALLET_EXCEPTION_IF(!scanty_outs.empty(), error::not_enough_outs_to_mix, scanty_outs, fake_outputs_count);
  }
  else
  {
    for (transfer_container::iterator it: selected_transfers)
    {
      std::vector<entry> v;
      const rct::key mask = it->is_rct() ? rct::commit(it->amount(), it->m_mask) : rct::zeroCommit(it->amount());
      v.push_back(std::make_tuple(it->m_global_output_index, boost::get<txout_to_key>(it->m_tx.vout[it->m_internal_output_index].target).key, mask));
      outs.push_back(v);
    }
  }
}

template<typename T>
void wallet2::transfer_selected(const std::vector<cryptonote::tx_destination_entry>& dsts, const std::list<transfer_container::iterator> selected_transfers, size_t fake_outputs_count,
  uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra, T destination_split_strategy, const tx_dust_policy& dust_policy, cryptonote::transaction& tx, pending_tx &ptx)
{
  using namespace cryptonote;
  // throw if attempting a transaction with no destinations
  THROW_WALLET_EXCEPTION_IF(dsts.empty(), error::zero_destination);

  uint64_t upper_transaction_size_limit = get_upper_tranaction_size_limit();
  uint64_t needed_money = fee;
  LOG_PRINT_L2("transfer: starting with fee " << print_money (needed_money));

  // calculate total amount being sent to all destinations
  // throw if total amount overflows uint64_t
  BOOST_FOREACH(auto& dt, dsts)
  {
    THROW_WALLET_EXCEPTION_IF(0 == dt.amount, error::zero_destination);
    needed_money += dt.amount;
    LOG_PRINT_L2("transfer: adding " << print_money(dt.amount) << ", for a total of " << print_money (needed_money));
    THROW_WALLET_EXCEPTION_IF(needed_money < dt.amount, error::tx_sum_overflow, dsts, fee, m_testnet);
  }

  uint64_t found_money = 0;
  BOOST_FOREACH(auto it, selected_transfers)
  {
    found_money += it->amount();
  }

  LOG_PRINT_L2("wanted " << print_money(needed_money) << ", found " << print_money(found_money) << ", fee " << print_money(fee));
  THROW_WALLET_EXCEPTION_IF(found_money < needed_money, error::not_enough_money, found_money, needed_money - fee, fee);

  typedef std::tuple<uint64_t, crypto::public_key, rct::key> entry;
  std::vector<std::vector<entry>> outs;
  get_outs(outs, selected_transfers, fake_outputs_count); // may throw

  //prepare inputs
  typedef cryptonote::tx_source_entry::output_entry tx_output_entry;
  size_t i = 0, out_index = 0;
  std::vector<cryptonote::tx_source_entry> sources;
  BOOST_FOREACH(transfer_container::iterator it, selected_transfers)
  {
    sources.resize(sources.size()+1);
    cryptonote::tx_source_entry& src = sources.back();
    transfer_details& td = *it;
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
    src.real_out_tx_key = get_tx_pub_key_from_extra(td.m_tx);
    src.real_output = it_to_replace - src.outputs.begin();
    src.real_output_in_tx_index = td.m_internal_output_index;
    detail::print_source_entry(src);
    ++out_index;
  }

  cryptonote::tx_destination_entry change_dts = AUTO_VAL_INIT(change_dts);
  if (needed_money < found_money)
  {
    change_dts.addr = m_account.get_keys().m_account_address;
    change_dts.amount = found_money - needed_money;
  }

  std::vector<cryptonote::tx_destination_entry> splitted_dsts, dust_dsts;
  uint64_t dust = 0;
  destination_split_strategy(dsts, change_dts, dust_policy.dust_threshold, splitted_dsts, dust_dsts);
  BOOST_FOREACH(auto& d, dust_dsts) {
    THROW_WALLET_EXCEPTION_IF(dust_policy.dust_threshold < d.amount, error::wallet_internal_error, "invalid dust value: dust = " +
      std::to_string(d.amount) + ", dust_threshold = " + std::to_string(dust_policy.dust_threshold));
  }
  BOOST_FOREACH(auto& d, dust_dsts) {
    if (!dust_policy.add_to_fee)
      splitted_dsts.push_back(cryptonote::tx_destination_entry(d.amount, dust_policy.addr_for_dust));
    dust += d.amount;
  }

  crypto::secret_key tx_key;
  bool r = cryptonote::construct_tx_and_get_tx_key(m_account.get_keys(), sources, splitted_dsts, extra, tx, unlock_time, tx_key);
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
}

void wallet2::transfer_selected_rct(std::vector<cryptonote::tx_destination_entry> dsts, const std::list<transfer_container::iterator> selected_transfers, size_t fake_outputs_count,
  uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra, cryptonote::transaction& tx, pending_tx &ptx)
{
  using namespace cryptonote;
  // throw if attempting a transaction with no destinations
  THROW_WALLET_EXCEPTION_IF(dsts.empty(), error::zero_destination);

  uint64_t upper_transaction_size_limit = get_upper_tranaction_size_limit();
  uint64_t needed_money = fee;
  LOG_PRINT_L2("transfer: starting with fee " << print_money (needed_money));

  // calculate total amount being sent to all destinations
  // throw if total amount overflows uint64_t
  BOOST_FOREACH(auto& dt, dsts)
  {
    THROW_WALLET_EXCEPTION_IF(0 == dt.amount, error::zero_destination);
    needed_money += dt.amount;
    LOG_PRINT_L2("transfer: adding " << print_money(dt.amount) << ", for a total of " << print_money (needed_money));
    THROW_WALLET_EXCEPTION_IF(needed_money < dt.amount, error::tx_sum_overflow, dsts, fee, m_testnet);
  }

  uint64_t found_money = 0;
  BOOST_FOREACH(auto it, selected_transfers)
  {
    found_money += it->amount();
  }

  LOG_PRINT_L2("wanted " << print_money(needed_money) << ", found " << print_money(found_money) << ", fee " << print_money(fee));
  THROW_WALLET_EXCEPTION_IF(found_money < needed_money, error::not_enough_money, found_money, needed_money - fee, fee);

  typedef std::tuple<uint64_t, crypto::public_key, rct::key> entry;
  std::vector<std::vector<entry>> outs;
  get_outs(outs, selected_transfers, fake_outputs_count); // may throw

  //prepare inputs
  size_t i = 0, out_index = 0;
  std::vector<cryptonote::tx_source_entry> sources;
  BOOST_FOREACH(transfer_container::iterator it, selected_transfers)
  {
    sources.resize(sources.size()+1);
    cryptonote::tx_source_entry& src = sources.back();
    transfer_details& td = *it;
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
    src.real_out_tx_key = get_tx_pub_key_from_extra(td.m_tx);
    src.real_output = it_to_replace - src.outputs.begin();
    src.real_output_in_tx_index = td.m_internal_output_index;
    src.mask = td.m_mask;
    detail::print_source_entry(src);
    ++out_index;
  }

  // we still keep a copy, since we want to keep dsts free of change for user feedback purposes
  std::vector<cryptonote::tx_destination_entry> splitted_dsts = dsts;
  cryptonote::tx_destination_entry change_dts = AUTO_VAL_INIT(change_dts);
  if (needed_money < found_money)
  {
    change_dts.addr = m_account.get_keys().m_account_address;
    change_dts.amount = found_money - needed_money;
    splitted_dsts.push_back(change_dts);
  }

  crypto::secret_key tx_key;
  bool r = cryptonote::construct_tx_and_get_tx_key(m_account.get_keys(), sources, splitted_dsts, extra, tx, unlock_time, tx_key, true);
  THROW_WALLET_EXCEPTION_IF(!r, error::tx_not_constructed, sources, dsts, unlock_time, m_testnet);
  THROW_WALLET_EXCEPTION_IF(upper_transaction_size_limit <= get_object_blobsize(tx), error::tx_too_big, tx, upper_transaction_size_limit);

  std::string key_images;
  bool all_are_txin_to_key = std::all_of(tx.vin.begin(), tx.vin.end(), [&](const txin_v& s_e) -> bool
  {
    CHECKED_GET_SPECIFIC_VARIANT(s_e, const txin_to_key, in, false);
    key_images += boost::to_string(in.k_image) + " ";
    return true;
  });
  THROW_WALLET_EXCEPTION_IF(!all_are_txin_to_key, error::unexpected_txin_type, tx);

  ptx.key_images = key_images;
  ptx.fee = fee;
  ptx.dust = 0;
  ptx.dust_added_to_fee = false;
  ptx.tx = tx;
  ptx.change_dts = change_dts;
  ptx.selected_transfers = selected_transfers;
  ptx.tx_key = tx_key;
  ptx.dests = dsts;
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
  size += n_inputs * (32 * (mixin+1) + 32);

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

  LOG_PRINT_L2("estimated rct tx size for " << n_inputs << " at mixin " << mixin << " and " << n_outputs << ": " << size << " (" << ((32 * n_inputs/*+1*/) + 2 * 32 * (mixin+1) * n_inputs + 32 * n_outputs) << " saved)");
  return size;
}

std::vector<size_t> wallet2::pick_prefered_rct_inputs(uint64_t needed_money) const
{
  std::vector<size_t> picks;
  float current_output_relatdness = 1.0f;

  LOG_PRINT_L2("pick_prefered_rct_inputs: needed_money " << print_money(needed_money));

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
    std::list<transfer_container::iterator> selected_transfers;
    std::vector<cryptonote::tx_destination_entry> dsts;
    cryptonote::transaction tx;
    pending_tx ptx;
    size_t bytes;

    void add(const account_public_address &addr, uint64_t amount) {
      std::vector<cryptonote::tx_destination_entry>::iterator i;
      i = std::find_if(dsts.begin(), dsts.end(), [&](const cryptonote::tx_destination_entry &d) { return !memcmp (&d.addr, &addr, sizeof(addr)); });
      if (i == dsts.end())
        dsts.push_back(tx_destination_entry(amount,addr));
      else
        i->amount += amount;
    }
  };
  std::vector<TX> txes;
  bool adding_fee; // true if new outputs go towards fee, rather than destinations
  uint64_t needed_fee, available_for_fee = 0;
  uint64_t upper_transaction_size_limit = get_upper_tranaction_size_limit();
  const bool use_rct = use_fork_rules(4, 0);

  const bool use_new_fee  = use_fork_rules(3, -720 * 14);
  const uint64_t fee_per_kb  = use_new_fee ? FEE_PER_KB : FEE_PER_KB_OLD;
  const uint64_t fee_multiplier = get_fee_multiplier(priority, use_new_fee);

  // throw if attempting a transaction with no destinations
  THROW_WALLET_EXCEPTION_IF(dsts.empty(), error::zero_destination);

  // calculate total amount being sent to all destinations
  // throw if total amount overflows uint64_t
  needed_money = 0;
  BOOST_FOREACH(auto& dt, dsts)
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

  if (unused_dust_indices.empty() && unused_transfers_indices.empty())
    return std::vector<wallet2::pending_tx>();

  // start with an empty tx
  txes.push_back(TX());
  accumulated_fee = 0;
  accumulated_outputs = 0;
  accumulated_change = 0;
  adding_fee = false;
  needed_fee = 0;

  // for rct, since we don't see the amounts, we will try to make all transactions
  // look the same, with 1 or 2 inputs, and 2 outputs. One input is preferable, as
  // this prevents linking to another by provenance analysis, but two is ok if we
  // try to pick outputs not from the same block. We will get two outputs, one for
  // the destination, and one for change.
  std::vector<size_t> prefered_inputs;
  uint64_t rct_outs_needed = 2 * (fake_outs_count + 1);
  rct_outs_needed += 100; // some fudge factor since we don't know how many are locked
  if (use_rct && get_num_rct_outputs() >= rct_outs_needed)
  {
    // this is used to build a tx that's 1 or 2 inputs, and 2 outputs, which
    // will get us a known fee.
    uint64_t estimated_fee = calculate_fee(fee_per_kb, estimate_rct_tx_size(2, fake_outs_count + 1, 2), fee_multiplier);
    prefered_inputs = pick_prefered_rct_inputs(needed_money + estimated_fee);
    if (!prefered_inputs.empty())
    {
      string s;
      for (auto i: prefered_inputs) s += print_money(m_transfers[i].amount()) + " ";
      LOG_PRINT_L1("Found prefered rct inputs for rct tx: " << s);
    }
  }

  // while we have something to send
  while ((!dsts.empty() && dsts[0].amount > 0) || adding_fee) {
    TX &tx = txes.back();

    // if we need to spend money and don't have any left, we fail
    if (unused_dust_indices.empty() && unused_transfers_indices.empty()) {
      LOG_PRINT_L2("No more outputs to choose from");
      THROW_WALLET_EXCEPTION_IF(1, error::not_enough_money, unlocked_balance(), needed_money, accumulated_fee + needed_fee);
    }

    // get a random unspent output and use it to pay part (or all) of the current destination (and maybe next one, etc)
    // This could be more clever, but maybe at the cost of making probabilistic inferences easier
    size_t idx = !prefered_inputs.empty() ? pop_back(prefered_inputs) : !unused_transfers_indices.empty() ? pop_best_value(unused_transfers_indices, tx.selected_transfers) : pop_best_value(unused_dust_indices, tx.selected_transfers);

    const transfer_details &td = m_transfers[idx];
    LOG_PRINT_L2("Picking output " << idx << ", amount " << print_money(td.amount()));

    // add this output to the list to spend
    tx.selected_transfers.push_back(m_transfers.begin() + idx);
    uint64_t available_amount = td.amount();
    accumulated_outputs += available_amount;

    if (adding_fee)
    {
      LOG_PRINT_L2("We need more fee, adding it to fee");
      available_for_fee += available_amount;
    }
    else
    {
      while (!dsts.empty() && dsts[0].amount <= available_amount)
      {
        // we can fully pay that destination
        LOG_PRINT_L2("We can fully pay " << get_account_address_as_str(m_testnet, dsts[0].addr) <<
          " for " << print_money(dsts[0].amount));
        tx.add(dsts[0].addr, dsts[0].amount);
        available_amount -= dsts[0].amount;
        dsts[0].amount = 0;
        pop_index(dsts, 0);
      }

      if (available_amount > 0 && !dsts.empty()) {
        // we can partially fill that destination
        LOG_PRINT_L2("We can partially pay " << get_account_address_as_str(m_testnet, dsts[0].addr) <<
          " for " << print_money(available_amount) << "/" << print_money(dsts[0].amount));
        tx.add(dsts[0].addr, available_amount);
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
      size_t estimated_rct_tx_size;
      if (use_rct)
        estimated_rct_tx_size = estimate_rct_tx_size(tx.selected_transfers.size(), fake_outs_count, tx.dsts.size() + 1);
      else
        estimated_rct_tx_size = tx.selected_transfers.size() * (fake_outs_count+1) * APPROXIMATE_INPUT_BYTES;
      try_tx = dsts.empty() || (estimated_rct_tx_size >= TX_SIZE_TARGET(upper_transaction_size_limit));
    }

    if (try_tx) {
      cryptonote::transaction test_tx;
      pending_tx test_ptx;

      needed_fee = 0;

      LOG_PRINT_L2("Trying to create a tx now, with " << tx.dsts.size() << " destinations and " <<
        tx.selected_transfers.size() << " outputs");
      if (use_rct)
        transfer_selected_rct(tx.dsts, tx.selected_transfers, fake_outs_count, unlock_time, needed_fee, extra,
          test_tx, test_ptx);
      else
        transfer_selected(tx.dsts, tx.selected_transfers, fake_outs_count, unlock_time, needed_fee, extra,
          detail::digit_split_strategy, tx_dust_policy(::config::DEFAULT_DUST_THRESHOLD), test_tx, test_ptx);
      auto txBlob = t_serializable_object_to_blob(test_ptx.tx);
      needed_fee = calculate_fee(fee_per_kb, txBlob, fee_multiplier);
      available_for_fee = test_ptx.fee + test_ptx.change_dts.amount + (!test_ptx.dust_added_to_fee ? test_ptx.dust : 0);
      LOG_PRINT_L2("Made a " << txBlob.size() << " kB tx, with " << print_money(available_for_fee) << " available for fee (" <<
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
            print_money(i->amount) << " to " << print_money(new_paid_amount) << " to accomodate " <<
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
        if (use_rct)
          transfer_selected_rct(tx.dsts, tx.selected_transfers, fake_outs_count, unlock_time, needed_fee, extra,
            test_tx, test_ptx);
        else
          transfer_selected(tx.dsts, tx.selected_transfers, fake_outs_count, unlock_time, needed_fee, extra,
            detail::digit_split_strategy, tx_dust_policy(::config::DEFAULT_DUST_THRESHOLD), test_tx, test_ptx);
        txBlob = t_serializable_object_to_blob(test_ptx.tx);
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
    THROW_WALLET_EXCEPTION_IF(1, error::not_enough_money, unlocked_balance(), needed_money, accumulated_fee + needed_fee);
  }

  LOG_PRINT_L1("Done creating " << txes.size() << " transactions, " << print_money(accumulated_fee) <<
    " total fee, " << print_money(accumulated_change) << " total change");

  std::vector<wallet2::pending_tx> ptx_vector;
  for (std::vector<TX>::iterator i = txes.begin(); i != txes.end(); ++i)
  {
    TX &tx = *i;
    uint64_t tx_money = 0;
    for (std::list<transfer_container::iterator>::const_iterator mi = tx.selected_transfers.begin(); mi != tx.selected_transfers.end(); ++mi)
      tx_money += (*mi)->amount();
    LOG_PRINT_L1("  Transaction " << (1+std::distance(txes.begin(), i)) << "/" << txes.size() <<
      ": " << (tx.bytes+1023)/1024 << " kB, sending " << print_money(tx_money) << " in " << tx.selected_transfers.size() <<
      " outputs to " << tx.dsts.size() << " destination(s), including " <<
      print_money(tx.ptx.fee) << " fee, " << print_money(tx.ptx.change_dts.amount) << " change");
    ptx_vector.push_back(tx.ptx);
  }

  // if we made it this far, we're OK to actually send the transactions
  return ptx_vector;
}

std::vector<wallet2::pending_tx> wallet2::create_transactions_all(const cryptonote::account_public_address &address, const size_t fake_outs_count, const uint64_t unlock_time, uint32_t priority, const std::vector<uint8_t> extra, bool trusted_daemon)
{
  std::vector<size_t> unused_transfers_indices;
  std::vector<size_t> unused_dust_indices;
  uint64_t accumulated_fee, accumulated_outputs, accumulated_change;
  struct TX {
    std::list<transfer_container::iterator> selected_transfers;
    std::vector<cryptonote::tx_destination_entry> dsts;
    cryptonote::transaction tx;
    pending_tx ptx;
    size_t bytes;
  };
  std::vector<TX> txes;
  uint64_t needed_fee, available_for_fee = 0;
  uint64_t upper_transaction_size_limit = get_upper_tranaction_size_limit();
  const bool use_rct = use_fork_rules(4, 0);

  const bool use_new_fee  = use_fork_rules(3, -720 * 14);
  const uint64_t fee_per_kb  = use_new_fee ? FEE_PER_KB : FEE_PER_KB_OLD;
  const uint64_t fee_multiplier = get_fee_multiplier(priority, use_new_fee);

  // gather all our dust and non dust outputs
  for (size_t i = 0; i < m_transfers.size(); ++i)
  {
    const transfer_details& td = m_transfers[i];
    if (!td.m_spent && (use_rct ? true : !td.is_rct()) && is_transfer_unlocked(td))
    {
      if (td.is_rct() || is_valid_decomposed_amount(td.amount()))
        unused_transfers_indices.push_back(i);
      else
        unused_dust_indices.push_back(i);
    }
  }
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
    tx.selected_transfers.push_back(m_transfers.begin() + idx);
    uint64_t available_amount = td.amount();
    accumulated_outputs += available_amount;

    // here, check if we need to sent tx and start a new one
    LOG_PRINT_L2("Considering whether to create a tx now, " << tx.selected_transfers.size() << " inputs, tx limit "
      << upper_transaction_size_limit);
    size_t estimated_rct_tx_size;
    if (use_rct)
      estimated_rct_tx_size = estimate_rct_tx_size(tx.selected_transfers.size(), fake_outs_count, tx.dsts.size() + 1);
    else
      estimated_rct_tx_size = tx.selected_transfers.size() * (fake_outs_count+1) * APPROXIMATE_INPUT_BYTES;
    bool try_tx = (unused_dust_indices.empty() && unused_transfers_indices.empty()) || ( estimated_rct_tx_size >= TX_SIZE_TARGET(upper_transaction_size_limit));

    if (try_tx) {
      cryptonote::transaction test_tx;
      pending_tx test_ptx;

      needed_fee = 0;

      tx.dsts.push_back(tx_destination_entry(1, address));

      LOG_PRINT_L2("Trying to create a tx now, with " << tx.dsts.size() << " destinations and " <<
        tx.selected_transfers.size() << " outputs");
      if (use_rct)
        transfer_selected_rct(tx.dsts, tx.selected_transfers, fake_outs_count, unlock_time, needed_fee, extra,
          test_tx, test_ptx);
      else
        transfer_selected(tx.dsts, tx.selected_transfers, fake_outs_count, unlock_time, needed_fee, extra,
          detail::digit_split_strategy, tx_dust_policy(::config::DEFAULT_DUST_THRESHOLD), test_tx, test_ptx);
      auto txBlob = t_serializable_object_to_blob(test_ptx.tx);
      needed_fee = calculate_fee(fee_per_kb, txBlob, fee_multiplier);
      available_for_fee = test_ptx.fee + test_ptx.dests[0].amount + test_ptx.change_dts.amount;
      LOG_PRINT_L2("Made a " << txBlob.size() << " kB tx, with " << print_money(available_for_fee) << " available for fee (" <<
        print_money(needed_fee) << " needed)");

      THROW_WALLET_EXCEPTION_IF(needed_fee > available_for_fee, error::wallet_internal_error, "Transaction cannot pay for itself");

      do {
        LOG_PRINT_L2("We made a tx, adjusting fee and saving it");
        tx.dsts[0].amount = available_for_fee - needed_fee;
        if (use_rct)
          transfer_selected_rct(tx.dsts, tx.selected_transfers, fake_outs_count, unlock_time, needed_fee, extra,
            test_tx, test_ptx);
        else
          transfer_selected(tx.dsts, tx.selected_transfers, fake_outs_count, unlock_time, needed_fee, extra,
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
    for (std::list<transfer_container::iterator>::const_iterator mi = tx.selected_transfers.begin(); mi != tx.selected_transfers.end(); ++mi)
      tx_money += (*mi)->amount();
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

template<typename T>
void wallet2::transfer_from(const std::vector<size_t> &outs, size_t num_outputs, uint64_t unlock_time, uint64_t needed_fee, T destination_split_strategy, const tx_dust_policy& dust_policy, const std::vector<uint8_t> &extra, cryptonote::transaction& tx, pending_tx &ptx)
{
  using namespace cryptonote;

  uint64_t upper_transaction_size_limit = get_upper_tranaction_size_limit();

  // select all dust inputs for transaction
  // throw if there are none
  uint64_t money = 0;
  std::list<transfer_container::iterator> selected_transfers;
#if 1
  for (size_t n = 0; n < outs.size(); ++n)
  {
    const transfer_details& td = m_transfers[outs[n]];
    if (!td.m_spent)
    {
      selected_transfers.push_back (m_transfers.begin() + outs[n]);
      money += td.amount();
      if (selected_transfers.size() >= num_outputs)
        break;
    }
  }
#else
  for (transfer_container::iterator i = m_transfers.begin(); i != m_transfers.end(); ++i)
  {
    const transfer_details& td = *i;
    if (!td.m_spent && (td.amount() < dust_policy.dust_threshold || !is_valid_decomposed_amount(td.amount())) && is_transfer_unlocked(td))
    {
      selected_transfers.push_back (i);
      money += td.amount();
      if (selected_transfers.size() >= num_outputs)
        break;
    }
  }
#endif

  // we don't allow no output to self, easier, but one may want to burn the dust if = fee
  THROW_WALLET_EXCEPTION_IF(money <= needed_fee, error::not_enough_money, money, needed_fee, needed_fee);

  typedef cryptonote::tx_source_entry::output_entry tx_output_entry;

  //prepare inputs
  size_t i = 0;
  std::vector<cryptonote::tx_source_entry> sources;
  BOOST_FOREACH(transfer_container::iterator it, selected_transfers)
  {
    sources.resize(sources.size()+1);
    cryptonote::tx_source_entry& src = sources.back();
    transfer_details& td = *it;
    src.amount = td.amount();
    src.rct = td.is_rct();

    //paste real transaction to the random index
    auto it_to_insert = std::find_if(src.outputs.begin(), src.outputs.end(), [&](const tx_output_entry& a)
    {
      return a.first >= td.m_global_output_index;
    });
    tx_output_entry real_oe;
    real_oe.first = td.m_global_output_index;
    real_oe.second.dest = rct::pk2rct(boost::get<txout_to_key>(td.m_tx.vout[td.m_internal_output_index].target).key);
    real_oe.second.mask = rct::commit(td.amount(), td.m_mask);
    auto interted_it = src.outputs.insert(it_to_insert, real_oe);
    src.real_out_tx_key = get_tx_pub_key_from_extra(td.m_tx);
    src.real_output = interted_it - src.outputs.begin();
    src.real_output_in_tx_index = td.m_internal_output_index;
    detail::print_source_entry(src);
    ++i;
  }

  cryptonote::tx_destination_entry change_dts = AUTO_VAL_INIT(change_dts);

  std::vector<cryptonote::tx_destination_entry> dsts;
  uint64_t money_back = money - needed_fee;
  if (dust_policy.dust_threshold > 0)
    money_back = money_back - money_back % dust_policy.dust_threshold;
  dsts.push_back(cryptonote::tx_destination_entry(money_back, m_account_public_address));
  std::vector<cryptonote::tx_destination_entry> splitted_dsts, dust;
  destination_split_strategy(dsts, change_dts, dust_policy.dust_threshold, splitted_dsts, dust);
  BOOST_FOREACH(auto& d, dust) {
    THROW_WALLET_EXCEPTION_IF(dust_policy.dust_threshold < d.amount, error::wallet_internal_error, "invalid dust value: dust = " +
      std::to_string(d.amount) + ", dust_threshold = " + std::to_string(dust_policy.dust_threshold));
  }

  crypto::secret_key tx_key;
  bool r = cryptonote::construct_tx_and_get_tx_key(m_account.get_keys(), sources, splitted_dsts, extra, tx, unlock_time, tx_key);
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

  ptx.key_images = key_images;
  ptx.fee = money - money_back;
  ptx.dust = 0;
  ptx.tx = tx;
  ptx.change_dts = change_dts;
  ptx.selected_transfers = selected_transfers;
  ptx.tx_key = tx_key;
  ptx.dests = dsts;
}

//----------------------------------------------------------------------------------------------------
void wallet2::get_hard_fork_info(uint8_t version, uint64_t &earliest_height)
{
  epee::json_rpc::request<cryptonote::COMMAND_RPC_HARD_FORK_INFO::request> req_t = AUTO_VAL_INIT(req_t);
  epee::json_rpc::response<cryptonote::COMMAND_RPC_HARD_FORK_INFO::response, std::string> resp_t = AUTO_VAL_INIT(resp_t);

  m_daemon_rpc_mutex.lock();
  req_t.jsonrpc = "2.0";
  req_t.id = epee::serialization::storage_entry(0);
  req_t.method = "hard_fork_info";
  req_t.params.version = version;
  bool r = net_utils::invoke_http_json_remote_command2(m_daemon_address + "/json_rpc", req_t, resp_t, m_http_client);
  m_daemon_rpc_mutex.unlock();
  CHECK_AND_ASSERT_THROW_MES(r, "Failed to connect to daemon");
  CHECK_AND_ASSERT_THROW_MES(resp_t.result.status != CORE_RPC_STATUS_BUSY, "Failed to connect to daemon");
  CHECK_AND_ASSERT_THROW_MES(resp_t.result.status == CORE_RPC_STATUS_OK, "Failed to get hard fork status");

  earliest_height = resp_t.result.earliest_height;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::use_fork_rules(uint8_t version, int64_t early_blocks)
{
  cryptonote::COMMAND_RPC_GET_HEIGHT::request req = AUTO_VAL_INIT(req);
  cryptonote::COMMAND_RPC_GET_HEIGHT::response res = AUTO_VAL_INIT(res);

  m_daemon_rpc_mutex.lock();
  bool r = net_utils::invoke_http_json_remote_command2(m_daemon_address + "/getheight", req, res, m_http_client);
  m_daemon_rpc_mutex.unlock();
  CHECK_AND_ASSERT_MES(r, false, "Failed to connect to daemon");
  CHECK_AND_ASSERT_MES(res.status != CORE_RPC_STATUS_BUSY, false, "Failed to connect to daemon");
  CHECK_AND_ASSERT_MES(res.status == CORE_RPC_STATUS_OK, false, "Failed to get current blockchain height");

  uint64_t earliest_height;
  get_hard_fork_info(version, earliest_height); // can throw

  bool close_enough = res.height >=  earliest_height - early_blocks; // start using the rules that many blocks beforehand
  if (close_enough)
    LOG_PRINT_L2("Using v" << (unsigned)version << " rules");
  else
    LOG_PRINT_L2("Not using v" << (unsigned)version << " rules");
  return close_enough;
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::get_upper_tranaction_size_limit()
{
  if (m_upper_transaction_size_limit > 0)
    return m_upper_transaction_size_limit;
  uint64_t full_reward_zone = use_fork_rules(2, 10) ? CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 : CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1;
  return ((full_reward_zone * 125) / 100) - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE;
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
      set.insert(td.amount());
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
std::vector<size_t> wallet2::select_available_outputs_from_histogram(uint64_t count, bool atleast, bool unlocked, bool trusted_daemon)
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
  bool r = net_utils::invoke_http_json_remote_command2(m_daemon_address + "/json_rpc", req_t, resp_t, m_http_client);
  m_daemon_rpc_mutex.unlock();
  THROW_WALLET_EXCEPTION_IF(!r, error::no_connection_to_daemon, "select_available_unmixable_outputs");
  THROW_WALLET_EXCEPTION_IF(resp_t.result.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "get_output_histogram");
  THROW_WALLET_EXCEPTION_IF(resp_t.result.status != CORE_RPC_STATUS_OK, error::get_histogram_error, resp_t.result.status);

  std::set<uint64_t> mixable;
  for (const auto &i: resp_t.result.histogram)
  {
    mixable.insert(i.amount);
  }

  return select_available_outputs([mixable, atleast](const transfer_details &td) {
    if (td.is_rct())
      return false;
    const uint64_t amount = td.amount();
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
  bool r = net_utils::invoke_http_json_remote_command2(m_daemon_address + "/json_rpc", req_t, resp_t, m_http_client);
  m_daemon_rpc_mutex.unlock();
  THROW_WALLET_EXCEPTION_IF(!r, error::no_connection_to_daemon, "get_num_rct_outputs");
  THROW_WALLET_EXCEPTION_IF(resp_t.result.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "get_output_histogram");
  THROW_WALLET_EXCEPTION_IF(resp_t.result.status != CORE_RPC_STATUS_OK, error::get_histogram_error, resp_t.result.status);
  THROW_WALLET_EXCEPTION_IF(resp_t.result.histogram.size() != 1, error::get_histogram_error, "Expected exactly one response");
  THROW_WALLET_EXCEPTION_IF(resp_t.result.histogram[0].amount != 0, error::get_histogram_error, "Expected 0 amount");

  return resp_t.result.histogram[0].instances;
}
//----------------------------------------------------------------------------------------------------
std::vector<size_t> wallet2::select_available_unmixable_outputs(bool trusted_daemon)
{
  // request all outputs with less than 3 instances
  return select_available_outputs_from_histogram(3, false, true, trusted_daemon);
}
//----------------------------------------------------------------------------------------------------
std::vector<size_t> wallet2::select_available_mixable_outputs(bool trusted_daemon)
{
  // request all outputs with at least 3 instances, so we can use mixin 2 with
  return select_available_outputs_from_histogram(3, true, true, trusted_daemon);
}
//----------------------------------------------------------------------------------------------------
std::vector<wallet2::pending_tx> wallet2::create_unmixable_sweep_transactions(bool trusted_daemon)
{
  // From hard fork 1, we don't consider small amounts to be dust anymore
  const bool hf1_rules = use_fork_rules(2, 10); // first hard fork has version 2
  tx_dust_policy dust_policy(hf1_rules ? 0 : ::config::DEFAULT_DUST_THRESHOLD);

  const bool use_new_fee  = use_fork_rules(3, -720 * 14);
  const uint64_t fee_per_kb  = use_new_fee ? FEE_PER_KB : FEE_PER_KB_OLD;

  // may throw
  std::vector<size_t> unmixable_outputs = select_available_unmixable_outputs(trusted_daemon);
  size_t num_dust_outputs = unmixable_outputs.size();

  if (num_dust_outputs == 0)
  {
    return std::vector<wallet2::pending_tx>();
  }

  // failsafe split attempt counter
  size_t attempt_count = 0;

  for(attempt_count = 1; ;attempt_count++)
  {
    size_t num_tx = 0.5 + pow(1.7,attempt_count-1);
    size_t num_outputs_per_tx = (num_dust_outputs + num_tx - 1) / num_tx;

    std::vector<pending_tx> ptx_vector;
    try
    {
      // for each new tx
      for (size_t i=0; i<num_tx;++i)
      {
        cryptonote::transaction tx;
        pending_tx ptx;
        std::vector<uint8_t> extra;

	// loop until fee is met without increasing tx size to next KB boundary.
	uint64_t needed_fee = 0;
	do
	{
	  transfer_from(unmixable_outputs, num_outputs_per_tx, (uint64_t)0 /* unlock_time */, 0, detail::digit_split_strategy, dust_policy, extra, tx, ptx);
	  auto txBlob = t_serializable_object_to_blob(ptx.tx);
          needed_fee = calculate_fee(fee_per_kb, txBlob, 1);

          // reroll the tx with the actual amount minus the fee
          // if there's not enough for the fee, it'll throw
	  transfer_from(unmixable_outputs, num_outputs_per_tx, (uint64_t)0 /* unlock_time */, needed_fee, detail::digit_split_strategy, dust_policy, extra, tx, ptx);
	  txBlob = t_serializable_object_to_blob(ptx.tx);
          needed_fee = calculate_fee(fee_per_kb, txBlob, 1);
	} while (ptx.fee < needed_fee);

        ptx_vector.push_back(ptx);

        // mark transfers to be used as "spent"
        BOOST_FOREACH(transfer_container::iterator it, ptx.selected_transfers)
        {
          set_spent(*it, 0);
        }
      }

      // if we made it this far, we've selected our transactions.  committing them will mark them spent,
      // so this is a failsafe in case they don't go through
      // unmark pending tx transfers as spent
      for (auto & ptx : ptx_vector)
      {
        // mark transfers to be used as not spent
        BOOST_FOREACH(transfer_container::iterator it2, ptx.selected_transfers)
        {
          set_unspent(*it2);
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
        BOOST_FOREACH(transfer_container::iterator it2, ptx.selected_transfers)
        {
          set_unspent(*it2);
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
        BOOST_FOREACH(transfer_container::iterator it2, ptx.selected_transfers)
        {
          set_unspent(*it2);
        }
      }

      throw;
    }
  }
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
    tx_extra_pub_key pub_key_field;
    THROW_WALLET_EXCEPTION_IF(!find_tx_extra_field_by_type(tx_extra_fields, pub_key_field), error::wallet_internal_error,
        "Public key wasn't found in the transaction extra");
    crypto::public_key tx_pub_key = pub_key_field.pub_key;

    // generate ephemeral secret key
    crypto::key_image ki;
    cryptonote::keypair in_ephemeral;
    cryptonote::generate_key_image_helper(m_account.get_keys(), tx_pub_key, td.m_internal_output_index, in_ephemeral, ki);
    THROW_WALLET_EXCEPTION_IF(ki != td.m_key_image,
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
    THROW_WALLET_EXCEPTION_IF(!crypto::check_ring_signature((const crypto::hash&)key_image, key_image, pkeys, &signature),
        error::wallet_internal_error, "Signature check failed: input " + boost::lexical_cast<std::string>(n) + "/"
        + boost::lexical_cast<std::string>(signed_key_images.size()) + ", key image " + epee::string_tools::pod_to_hex(key_image)
        + ", signature " + epee::string_tools::pod_to_hex(signature) + ", pubkey " + epee::string_tools::pod_to_hex(*pkeys[0]));

    req.key_images.push_back(epee::string_tools::pod_to_hex(key_image));
  }

  for (size_t n = 0; n < signed_key_images.size(); ++n)
    m_transfers[n].m_key_image = signed_key_images[n].first;

  m_daemon_rpc_mutex.lock();
  bool r = epee::net_utils::invoke_http_json_remote_command2(m_daemon_address + "/is_key_image_spent", req, daemon_resp, m_http_client, 200000);
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
