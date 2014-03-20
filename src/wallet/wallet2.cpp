// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <boost/utility/value_init.hpp>
#include "include_base_utils.h"
using namespace epee;

#include "wallet2.h"
#include "cryptonote_core/cryptonote_format_utils.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "misc_language.h"
#include "cryptonote_core/cryptonote_basic_impl.h"
#include "common/boost_serialization_helper.h"
#include "profile_tools.h"
#include "crypto/crypto.h"
#include "serialization/binary_utils.h"

using namespace cryptonote;

namespace tools
{
//----------------------------------------------------------------------------------------------------
bool wallet2::init(const std::string& daemon_address, uint64_t upper_transaction_size_limit)
{
  m_upper_transaction_size_limit = upper_transaction_size_limit;
  m_daemon_address = daemon_address;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::process_new_transaction(cryptonote::transaction& tx, uint64_t height, fail_details& fd)
{
  std::vector<size_t> outs;
  uint64_t tx_money_got_in_outs = 0;
  crypto::public_key tx_pub_key = null_pkey;
  bool r = parse_and_validate_tx_extra(tx, tx_pub_key);
  fd.reason = fail_details::error_to_parse_tx_extra;
  CHECK_AND_ASSERT_MES(r && tx_pub_key != null_pkey, false, "process_new_transaction failed.");
  r = lookup_acc_outs(m_account.get_keys(), tx, tx_pub_key, outs, tx_money_got_in_outs);
  fd.reason = fail_details::error_invalid_tx;
  CHECK_AND_ASSERT_MES(r, false, "call lookup_acc_outs failed");
  if(outs.size() && tx_money_got_in_outs)
  {
    //good news - got money! take care about it
    //usually we have only one transfer for user in transaction
    cryptonote::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request req = AUTO_VAL_INIT(req);
    cryptonote::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response res = AUTO_VAL_INIT(res);
    req.txid = get_transaction_hash(tx);
    bool r = net_utils::invoke_http_bin_remote_command2(m_daemon_address + "/get_o_indexes.bin", req, res, m_http_client, WALLET_RCP_CONNECTION_TIMEOUT);
    if (!r)                                      fd.reason = fail_details::error_not_connected;
    else if (CORE_RPC_STATUS_BUSY == res.status) fd.reason = fail_details::error_daemon_is_busy;
    else if (CORE_RPC_STATUS_OK != res.status)   fd.reason = fail_details::error_internal_error;
    else                                         fd.reason = fail_details::error_ok;
    if (fail_details::error_ok != fd.reason)
    {
      // in case of split while lookup_acc_outs, transaction could be lost (especially if it is coinbase tx)
      LOG_PRINT_L0("failed to invoke get_o_indexes.bin: " << interpret_rpc_response(r, res.status));
      return false;
    }

    fd.reason = fail_details::error_internal_error;
    CHECK_AND_ASSERT_MES(res.o_indexes.size() == tx.vout.size(), false, "internal error: transactions outputs size=" << tx.vout.size()
      << " not match with COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES response size=" << res.o_indexes.size());

    BOOST_FOREACH(size_t o, outs)
    {
      fd.reason = fail_details::error_invalid_tx;
      CHECK_AND_ASSERT_MES(o < tx.vout.size(), false, "wrong out in transaction: internal index=" << o << ", total_outs" << tx.vout.size());
      m_transfers.push_back(boost::value_initialized<transfer_details>());
      transfer_details& td = m_transfers.back();
      td.m_block_height = height;
      td.m_internal_output_index = o;
      td.m_global_output_index = res.o_indexes[o];
      td.m_tx = tx;
      td.m_spent = false;
      cryptonote::keypair in_ephemeral;
      cryptonote::generate_key_image_helper(m_account.get_keys(), tx_pub_key, o, in_ephemeral, td.m_key_image);
      fd.reason = fail_details::error_internal_error;
      CHECK_AND_ASSERT_MES(in_ephemeral.pub == boost::get<cryptonote::txout_to_key>(tx.vout[o].target).key,
        false, "internal error: at key_image generating ephemeral public key not matched with output_key");
      m_key_images[td.m_key_image] = m_transfers.size()-1;
      LOG_PRINT_COLOR("Received money: " << print_money(td.amount()) << ", with tx: " << get_transaction_hash(tx),
        LOG_LEVEL_0, epee::log_space::console_color_green);
    }
  }
  // check all outputs for spending (compare key images)
  BOOST_FOREACH(auto& in, tx.vin)
  {
    if(in.type() != typeid(cryptonote::txin_to_key))
      continue;
    auto it = m_key_images.find(boost::get<cryptonote::txin_to_key>(in).k_image);
    if(it != m_key_images.end())
    {
      LOG_PRINT_COLOR("Spent money: " << print_money(boost::get<cryptonote::txin_to_key>(in).amount) << ", with tx: " << get_transaction_hash(tx),
        LOG_LEVEL_0, epee::log_space::console_color_magenta);
      m_transfers[it->second].m_spent = true;
    }
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::process_new_blockchain_entry(cryptonote::block& b, cryptonote::block_complete_entry& bche, crypto::hash& bl_id, uint64_t height, fail_details& fd)
{
  //handle transactions from new block
  fd.reason = fail_details::error_internal_error;
  CHECK_AND_ASSERT_MES(height == m_blockchain.size(), false, "internal error: current_index=" << height << ", m_blockchain.size()=" << m_blockchain.size());
  //optimization: seeking only for blocks that are not older then the wallet creation time plus 1 day. 1 day is for possible user incorrect time setup
  if(b.timestamp + 60*60*24 > m_account.get_createtime())
  {
    TIME_MEASURE_START(miner_tx_handle_time);
    bool r = process_new_transaction(b.miner_tx, height, fd);
    TIME_MEASURE_FINISH(miner_tx_handle_time);
    CHECK_AND_NO_ASSERT_MES(r, false, "failed to process transaction");

    TIME_MEASURE_START(txs_handle_time);
    BOOST_FOREACH(auto& txblob, bche.txs)
    {
      cryptonote::transaction tx;
      r = parse_and_validate_tx_from_blob(txblob, tx);
      fd.reason = fail_details::error_to_parse_tx;
      CHECK_AND_ASSERT_MES(r, false, "failed to parse and validate transaction from blob");
      r = process_new_transaction(tx, height, fd);
      CHECK_AND_ASSERT_MES(r, false, "failed to process transaction");
    }
    TIME_MEASURE_FINISH(txs_handle_time);
    LOG_PRINT_L2("Processed block: " << bl_id << ", height " << height << ", " <<  miner_tx_handle_time + txs_handle_time << "(" << miner_tx_handle_time << "/" << txs_handle_time <<")ms");
  }else
  {
    LOG_PRINT_L2( "Skipped block by timestamp, height: " << height << ", block time " << b.timestamp << ", account time " << m_account.get_createtime());
  }
  m_blockchain.push_back(bl_id);
  ++m_local_bc_height;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::get_short_chain_history(std::list<crypto::hash>& ids)
{
  size_t i = 0;
  size_t current_multiplier = 1;
  size_t sz = m_blockchain.size();
  if(!sz)
    return true;
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

  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::pull_blocks(size_t& blocks_added, fail_details& fd)
{
  blocks_added = 0;
  cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request req = AUTO_VAL_INIT(req);
  cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response res = AUTO_VAL_INIT(res);
  get_short_chain_history(req.block_ids);
  bool r = net_utils::invoke_http_bin_remote_command2(m_daemon_address + "/getblocks.bin", req, res, m_http_client, WALLET_RCP_CONNECTION_TIMEOUT);
  if (!r)                                      fd.reason = fail_details::error_not_connected;
  else if (CORE_RPC_STATUS_BUSY == res.status) fd.reason = fail_details::error_daemon_is_busy;
  else if (CORE_RPC_STATUS_OK != res.status)   fd.reason = fail_details::error_internal_error;
  else                                         fd.reason = fail_details::error_ok;
  if (fail_details::error_ok != fd.reason)
  {
    LOG_PRINT_L0("failed to get blocks: " << interpret_rpc_response(r, res.status));
    return false;
  }

  //find split position, if split happened

  fd.reason = fail_details::error_internal_error;
  CHECK_AND_ASSERT_MES(res.start_height < m_blockchain.size(), false, "wrong daemon response: m_start_height="
    << res.start_height << " not less than local blockchain size=" << m_blockchain.size());

  size_t current_index = res.start_height;
  BOOST_FOREACH(auto& bl_entry, res.blocks)
  {
    cryptonote::block bl;
    r = cryptonote::parse_and_validate_block_from_blob(bl_entry.block, bl);
    fd.reason = fail_details::error_to_parse_block;
    CHECK_AND_ASSERT_MES(r, false, "failed to parse/validate block");
    crypto::hash bl_id = get_block_hash(bl);
    if(current_index >= m_blockchain.size())
    {
      r = process_new_blockchain_entry(bl, bl_entry, bl_id, current_index, fd);
      if(!r) return false;
      ++blocks_added;
    }else
    {
      if(bl_id != m_blockchain[current_index])
      {
        //split detected here !!!
        fd.reason = fail_details::error_internal_error;
        CHECK_AND_ASSERT_MES(current_index != res.start_height, false, "wrong daemon response: first block in response " << string_tools::pod_to_hex(bl_id)
          << "\nnot match with local block id " << string_tools::pod_to_hex(m_blockchain[current_index]));
        detach_blockchain(current_index);
        r = process_new_blockchain_entry(bl, bl_entry, bl_id, current_index, fd);
        if(!r) return false;
      }
    }
    ++current_index;
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::refresh(fail_details& fd)
{
  size_t blocks_fetched = 0;
  return refresh(blocks_fetched, fd);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::refresh(size_t & blocks_fetched, fail_details& fd)
{
  bool received_money = false;
  return refresh(blocks_fetched, received_money, fd);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::refresh(size_t & blocks_fetched, bool& received_money, fail_details& fd)
{
  received_money = false;
  blocks_fetched = 0;
  size_t added_blocks = 0;
  size_t try_count = 0;
  crypto::hash last_tx_hash_id = m_transfers.size() ? get_transaction_hash(m_transfers.back().m_tx) : null_hash;

  while(m_run.load(std::memory_order_relaxed))
  {
    bool res = pull_blocks(added_blocks, fd);
    if(!res)
    {
      if(try_count < 3)
      {
        LOG_PRINT_L1("Another try pull_blocks (try_count=" << try_count << ")...");
        ++try_count;
        continue;
      }else
      {
        LOG_PRINT_L1("pull_blocks failed, try_count=" << try_count);
        return false;
      }
    }
    blocks_fetched+=added_blocks;

    if(!added_blocks)
      break;
  }
  if(last_tx_hash_id != (m_transfers.size() ? get_transaction_hash(m_transfers.back().m_tx) : null_hash))
    received_money = true;

  LOG_PRINT_L1("Refresh done, blocks received: " << blocks_fetched << ", balance: " << print_money(balance()) << ", unlocked: " << print_money(unlocked_balance()));
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::detach_blockchain(uint64_t height)
{
  LOG_PRINT_L0("Detaching blockchain on height " << height);
  size_t transfers_detached = 0;

  auto it = std::find_if(m_transfers.begin(), m_transfers.end(), [&](const transfer_details& td){return td.m_block_height >= height;});
  size_t i_start = it - m_transfers.begin();

  for(size_t i = i_start; i!= m_transfers.size();i++)
  {
    auto it_ki = m_key_images.find(m_transfers[i].m_key_image);
    CHECK_AND_ASSERT_MES(it_ki != m_key_images.end(), false, "key image not found");
    m_key_images.erase(it_ki);
    ++transfers_detached;
  }
  m_transfers.erase(it, m_transfers.end());

  size_t blocks_detached = m_blockchain.end() - (m_blockchain.begin()+height);
  m_blockchain.erase(m_blockchain.begin()+height, m_blockchain.end());
  m_local_bc_height -= blocks_detached;

  LOG_PRINT_L0("Detached blockchain on height " << height << ", transfers detached " << transfers_detached << ", blocks detached " << blocks_detached);

  return true;
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
  cryptonote::block b;
  cryptonote::generate_genesis_block(b);
  m_blockchain.push_back(get_block_hash(b));
  m_local_bc_height = 1;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::store_keys(const std::string& keys_file_name, const std::string& password)
{
  std::string account_data;
  bool r = epee::serialization::store_t_to_binary(m_account, account_data);
  CHECK_AND_ASSERT_MES(r, false, "failed to serialize wallet keys");
  wallet2::keys_file_data keys_file_data = boost::value_initialized<wallet2::keys_file_data>();

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
//----------------------------------------------------------------------------------------------------
bool wallet2::load_keys(const std::string& keys_file_name, const std::string& password)
{
  wallet2::keys_file_data keys_file_data;
  std::string buf;
  bool r = epee::file_io_utils::load_file_to_string(keys_file_name, buf);
  r &= ::serialization::parse_binary(buf, keys_file_data);
  CHECK_AND_ASSERT_MES(r, false, "failed to load wallet keys file: " << keys_file_name);

  crypto::chacha8_key key;
  crypto::generate_chacha8_key(password, key);
  std::string account_data;
  account_data.resize(keys_file_data.account_data.size());
  crypto::chacha8(keys_file_data.account_data.data(), keys_file_data.account_data.size(), key, keys_file_data.iv, &account_data[0]);

  const cryptonote::account_keys& keys = m_account.get_keys();
  r = epee::serialization::load_t_from_binary(m_account, account_data);
  r = r && verify_keys(keys.m_view_secret_key,  keys.m_account_address.m_view_public_key);
  r = r && verify_keys(keys.m_spend_secret_key, keys.m_account_address.m_spend_public_key);
  if (!r)
  {
    LOG_ERROR("invalid password");
    return false;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::generate(const std::string& wallet_, const std::string& password)
{
  clear();
  prepare_file_names(wallet_);
  boost::system::error_code e;
  if(boost::filesystem::exists(m_wallet_file, e) || boost::filesystem::exists(m_keys_file, e))
  {
    LOG_PRINT_RED_L0("failed to generate wallet, file already exist or wrong path: " << wallet_);
    return false;
  }

  m_account.generate();
  m_account_public_address = m_account.get_keys().m_account_address;

  bool r = store_keys(m_keys_file, password);
  CHECK_AND_ASSERT_MES(r, false, "Failed to store wallet key files!");
  r = file_io_utils::save_string_to_file(m_wallet_file + ".address.txt", m_account.get_public_address_str());
  if(!r) LOG_PRINT_RED_L0("String with address text not saved");
  return store();
}
//----------------------------------------------------------------------------------------------------
bool wallet2::prepare_file_names(const std::string& file_path)
{
  m_keys_file = file_path;
  m_wallet_file = file_path;
  boost::system::error_code e;
  if(string_tools::get_extension(m_keys_file) == "keys")
  {//provided keys file name
    m_wallet_file = string_tools::cut_off_extension(m_wallet_file);
  }else
  {//provided wallet file name
    m_keys_file += ".keys";
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::check_connection()
{
  if(m_http_client.is_connected())
    return true;

  net_utils::http::url_content u;
  net_utils::parse_url(m_daemon_address, u);
  if(!u.port)
    u.port = 8081;  
  return m_http_client.connect(u.host, std::to_string(u.port), WALLET_RCP_CONNECTION_TIMEOUT);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::load(const std::string& wallet_, const std::string& password)
{
  clear();
  prepare_file_names(wallet_);
  boost::system::error_code e;
  if(!boost::filesystem::exists(m_keys_file, e) || e)
  {
    LOG_PRINT_L0("file not found: " << m_keys_file);
    return false;
  }

  bool r = load_keys(m_keys_file, password);
  if (!r)
    return false;

  LOG_PRINT_L0("Loaded wallet keys file, with public address: " << m_account.get_public_address_str());
  //keys loaded ok!
  //try to load wallet file. but even if we failed, it is not big problem
  if(!boost::filesystem::exists(m_wallet_file, e) || e)
  {
    LOG_PRINT_L0("file not found: " << m_wallet_file << ", starting with empty blockchain");
    m_account_public_address = m_account.get_keys().m_account_address;
    return true;
  }
  r = tools::unserialize_obj_from_file(*this, m_wallet_file);
  CHECK_AND_ASSERT_MES(r, false, "failed to load wallet from file " << m_wallet_file);
  CHECK_AND_ASSERT_MES(m_account_public_address.m_spend_public_key == m_account.get_keys().m_account_address.m_spend_public_key &&
                       m_account_public_address.m_view_public_key == m_account.get_keys().m_account_address.m_view_public_key,
                       false, "addresses of wallet keys file and wallet data file are mismatched");
  if(!m_blockchain.size())
  {
    cryptonote::block b;
    cryptonote::generate_genesis_block(b);
    m_blockchain.push_back(get_block_hash(b));
  }
  m_local_bc_height = m_blockchain.size();

  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::store()
{
  bool r = tools::serialize_obj_to_file(*this, m_wallet_file);
  CHECK_AND_ASSERT_MES(r, false, "failed to save wallet to file " << m_wallet_file);
  return r;
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::unlocked_balance()
{
  uint64_t amount = 0;
  BOOST_FOREACH(transfer_details& td, m_transfers)
    if(!td.m_spent && is_transfer_unlocked(td))
      amount += td.amount();

  return amount;
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::balance()
{
  uint64_t amount = 0;
  BOOST_FOREACH(auto& td, m_transfers)
    if(!td.m_spent)
      amount += td.amount();

  return amount;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::get_transfers(wallet2::transfer_container& incoming_transfers)
{
  incoming_transfers = m_transfers;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::is_transfer_unlocked(const transfer_details& td) const
{
  if(!is_tx_spendtime_unlocked(td.m_tx.unlock_time))
    return false;

  if(td.m_block_height + DEFAULT_TX_SPENDABLE_AGE > m_blockchain.size())
    return false;

  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::is_tx_spendtime_unlocked(uint64_t unlock_time) const
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
    if(current_time + CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS >= unlock_time)
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
  T pop_random_value(std::vector<T>& vec)
  {
    CHECK_AND_ASSERT_MES(!vec.empty(), T(), "Vector must be non-empty");

    size_t idx = crypto::rand<size_t>() % vec.size();
    T res = vec[idx];
    if (idx + 1 != vec.size())
    {
      vec[idx] = vec.back();
    }
    vec.resize(vec.size() - 1);

    return res;
  }
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::select_transfers(uint64_t needed_money, bool add_dust, uint64_t dust, std::list<transfer_container::iterator>& selected_transfers)
{
  std::vector<size_t> unused_transfers_indices;
  std::vector<size_t> unused_dust_indices;
  for (size_t i = 0; i < m_transfers.size(); ++i)
  {
    const transfer_details& td = m_transfers[i];
    if (!td.m_spent && is_transfer_unlocked(td))
    {
      if (dust < td.amount())
        unused_transfers_indices.push_back(i);
      else
        unused_dust_indices.push_back(i);
    }
  }

  bool select_one_dust = add_dust && !unused_dust_indices.empty();
  uint64_t found_money = 0;
  while (found_money < needed_money && (!unused_transfers_indices.empty() || !unused_dust_indices.empty()))
  {
    size_t idx;
    if (select_one_dust)
    {
      idx = pop_random_value(unused_dust_indices);
      select_one_dust = false;
    }
    else
    {
      idx = !unused_transfers_indices.empty() ? pop_random_value(unused_transfers_indices) : pop_random_value(unused_dust_indices);
    }

    transfer_container::iterator it = m_transfers.begin() + idx;
    selected_transfers.push_back(it);
    found_money += it->amount();
  }

  return found_money;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::transfer(const std::vector<cryptonote::tx_destination_entry>& dsts, size_t fake_outputs_count,
  uint64_t unlock_time, uint64_t fee, cryptonote::transaction& tx)
{
  return transfer(dsts, fake_outputs_count, unlock_time, fee, detail::digit_split_strategy, tx_dust_policy(fee), tx);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::transfer(const std::vector<cryptonote::tx_destination_entry>& dsts, size_t fake_outputs_count, uint64_t unlock_time, uint64_t fee, cryptonote::transaction& tx, fail_details& tfd)
{
  return transfer(dsts, fake_outputs_count, unlock_time, fee, detail::digit_split_strategy, tx_dust_policy(fee), tx, tfd);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::transfer(const std::vector<cryptonote::tx_destination_entry>& dsts, size_t fake_outputs_count,
                       uint64_t unlock_time, uint64_t fee)
{
  cryptonote::transaction tx;
  return transfer(dsts, fake_outputs_count, unlock_time, fee, tx);
}
//----------------------------------------------------------------------------------------------------
}
