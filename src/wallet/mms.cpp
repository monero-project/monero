#include "mms.h"
#include <boost/archive/portable_binary_oarchive.hpp>
#include <boost/archive/portable_binary_iarchive.hpp>
#include <fstream>
#include <sstream>
#include "file_io_utils.h"
#include "messaging_daemon_commands_defs.h"
#include "storages/http_abstract_invoke.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet.mms"

namespace tools
{
message_store::message_store() {
  m_active = false;
  m_next_message_id = 1;
  m_coalition_size = 0;
  m_threshold = 0;
  m_nettype = cryptonote::network_type::TESTNET;
  m_wallet_state_provider = NULL;
}

message_store::message_store(i_wallet_state_provider *wallet_state_provider)
{
  m_active = false;
  m_next_message_id = 1;
  m_coalition_size = 0;
  m_threshold = 0;
  m_wallet_state_provider = wallet_state_provider;
}

void message_store::init(i_wallet_state_provider *wallet_state_provider,
			 const std::string &own_transport_address, uint32_t coalition_size, uint32_t threshold)
{
  set_active(true);
  m_coalition_size = coalition_size;
  m_threshold = threshold;
  m_members.clear();
  m_messages.clear();
  m_next_message_id = 1;

  m_wallet_state_provider = wallet_state_provider;
  wallet_state state;
  m_wallet_state_provider->get_wallet_state(state);
  m_nettype = state.nettype;
  cryptonote::account_public_address &own_address = state.address;
  if (state.multisig)
  {
    if (state.original_keys_available)
    {
      // wallet is already multisig, the address is now the multisig address,
      // get the address from the "original" info
      own_address = state.original_address;
    }
    else
    {
      throw std::invalid_argument("Own address not available");
    }
  }
  add_member(tr("me"), own_address, own_transport_address);
}

uint32_t message_store::add_member(const std::string &label, const cryptonote::account_public_address &monero_address,
				   const std::string &transport_address)
{
  coalition_member m;
  m.monero_address = monero_address;
  m.transport_address = transport_address;
  m.label = label;
  m.index = m_members.size();
  // Simple convention/automatism for now: The very first member is fixed as / must be "me"
  m.me = m.index == 0;
  m_members.push_back(m);
  return m.index;
}

const coalition_member &message_store::get_member(uint32_t index) const
{
  return m_members[index];
}

uint32_t message_store::member_index_by_monero_address(const cryptonote::account_public_address &monero_address) const
{
  for (size_t i = 1; i < m_members.size(); ++i) {
    const coalition_member &m = m_members[i];
    if (m.monero_address == monero_address) {
      return m.index;
    }
  }
  MERROR("No coalition member with Monero address " << get_account_address_as_str(m_nettype, false, monero_address));
  return 0;
}

uint32_t message_store::member_index_by_transport_address(const std::string &transport_address) const
{
  for (size_t i = 1; i < m_members.size(); ++i) {
    const coalition_member &m = m_members[i];
    if (m.transport_address == transport_address) {
      return m.index;
    }
  }
  MERROR("No coalition member with transport address " << transport_address);
  return 0;
}

void message_store::process_wallet_created_data(message_type type, const std::string &content) {
  switch(type)
  {
  case message_type_key_set:
    // Result of a "prepare_multisig" command in the wallet
    // Send the key set to all other members
    for (size_t i = 1; i < m_members.size(); ++i) {
      add_message(i, type, message_direction_out, content);
    }
    break;

  case message_type_finalizing_key_set:
    // Result of a "make_multisig" command in the wallet in case of N-1/N multisig
    // Send the finalizing key set to all other members
    for (size_t i = 1; i < m_members.size(); ++i) {
      add_message(i, type, message_direction_out, content);
    }
    break;

  case message_type_multisig_sync_data:
    // Result of a "export_multisig_info" command in the wallet
    // Send the sync data to all other members
    for (size_t i = 1; i < m_members.size(); ++i) {
      add_message(i, type, message_direction_out, content);
    }
    break;

  case message_type_partially_signed_tx:
    // Result of a "transfer" command in the wallet, or a "sign_multisig" command
    // that did not yet result in the minimum number of signatures required
    // Create a message "from me to me" as a container for the tx data
    if (m_threshold == 1)
    {
      // Probably rare, but possible: The 1 signature is already enough, correct the type
      // Easier to correct here than asking all callers to detect this rare special case
      type = message_type_fully_signed_tx;
    }
    add_message(0, type, message_direction_in, content);
    break;

  case message_type_fully_signed_tx:
    add_message(0, type, message_direction_in, content);
    break;

  default:
    MERROR("Unknown message type " << (uint32_t)type);
    break;
  }
}

uint32_t message_store::add_message(uint32_t member_index, message_type type, message_direction direction,
				    const std::string &content)
{
  size_t wallet_height;
  m_wallet_state_provider->get_num_transfer_details(wallet_height);

  message m;
  m.id = m_next_message_id++;
  m.type = type;
  m.direction = direction;
  m.content = content;
  m.timestamp = time(NULL);
  m.member_index = member_index;
  if (direction == message_direction_out)
  {
    m.state = message_state_ready_to_send;
  }
  else {
    m.state = message_state_waiting;
  };
  m.wallet_height = wallet_height;
  m.hash = crypto::null_hash;
  m_messages.push_back(m);
  return m_messages.size() - 1;
}

uint32_t message_store::get_message_index_by_id(uint32_t id)
{
  for (size_t i = 0; i < m_messages.size(); ++i)
  {
    if (m_messages[i].id == id)
    {
      return i;
    }
  }
  throw std::invalid_argument("Message id not found");
  return 0;
}

message message_store::get_message_by_id(uint32_t id)
{
  return m_messages[get_message_index_by_id(id)];
}

bool message_store::any_message_of_type(message_type type, message_direction direction) const
{
  for (size_t i = 0; i < m_messages.size(); ++i)
  {
    if ((m_messages[i].type == type) && (m_messages[i].direction == direction))
    {
      return true;
    }
  }
  return false;
}

bool message_store::any_message_with_hash(const crypto::hash &hash) const
{
  for (size_t i = 0; i < m_messages.size(); ++i)
  {
    if (m_messages[i].hash == hash)
    {
      return true;
    }
  }
  return false;
}

// Is in every element of vector 'ids' (except at index 0) a message id i.e. not 0?
// Mostly used to check whether we have a message for each coalition member except me,
// with the member index used as index into 'ids'; the element at index 0, for me,
// is ignored, to make constant subtractions of 1 for indices when filling the
// vector unnecessary
bool message_store::message_ids_complete(const std::vector<uint32_t> ids) const
{
  for (size_t i = 1 /* and not 0 */; i < ids.size(); ++i)
  {
    if (ids[i] == 0)
    {
      return false;
    }
  }
  return true;
}

void message_store::delete_message(uint32_t id)
{
  delete_transport_message(id);
  uint32_t index = get_message_index_by_id(id);
  m_messages.erase(m_messages.begin() + index);
}

void message_store::delete_all_messages()
{
  for (size_t i = 0; i < m_messages.size(); ++i)
  {
    delete_transport_message(m_messages[i].id);
  }
  m_messages.clear();
}

void message_store::write_to_file(const std::string &filename) {
  std::stringstream oss;
  boost::archive::portable_binary_oarchive ar(oss);
  ar << *this;
  std::string buf = oss.str();
  bool r = epee::file_io_utils::save_string_to_file(filename, buf);
}

void message_store::read_from_file(const std::string &filename) {
  boost::system::error_code ignored_ec;
  bool file_exists = boost::filesystem::exists(filename, ignored_ec);
  if (!file_exists) {
    // Simply do nothing if the file is not there; allows to easily upgrade to new
    // file structures by simply deleting files of "old" structure (instead of dealing
    // with versions, which in this early development state does not make much sense)
    return;
  }

  std::string buf;
  bool r = epee::file_io_utils::load_file_to_string(filename, buf);
  std::stringstream iss;
  iss << buf;
  boost::archive::portable_binary_iarchive ar(iss);
  ar >> *this;

  m_filename = filename;
}

bool message_store::get_processable_messages(bool force_sync, std::vector<processing_data> &data_list, std::string &wait_reason)
{
  wallet_state state;
  m_wallet_state_provider->get_wallet_state(state);
  uint32_t wallet_height = state.num_transfer_details;
  data_list.clear();
  wait_reason.clear();
  if (m_members.size() < m_coalition_size)
  {
    // Unless ALL members are known we can't do anything
    wait_reason = tr("The list of coalition members is not complete.");
    return false;
  }

  if (!state.multisig)
  {
    if (!any_message_of_type(message_type_key_set, message_direction_out))
    {
      // With the own key set not yet ready we must do "prepare_multisig" first;
      // Key sets from other members may be here already, but if we process them now
      // the wallet will go multisig too early: we can't produce our own key set any more!
      processing_data data;
      data.processing = message_processing_prepare_multisig;
      data_list.push_back(data);
      return true;
    }

    // Ids of key set messages per member index, to check completeness
    // Naturally, does not care about the order of the messages and is trivial to secure against
    // key sets that were received more than once
    std::vector<uint32_t> key_set_messages(m_coalition_size, 0);

    for (size_t i = 0; i < m_messages.size(); ++i)
    {
      message &m = m_messages[i];
      if ((m.type == message_type_key_set) && (m.state == message_state_waiting))
      {
	if (key_set_messages[m.member_index] == 0)
	{
	  key_set_messages[m.member_index] = m.id;
	}
	// else duplicate key set, ignore
      }
    }

    bool key_sets_complete = message_ids_complete(key_set_messages);
    if (key_sets_complete)
    {
      // Nothing else can be ready to process earlier than this, ignore everything else and give back
      processing_data data;
      data.processing = message_processing_make_multisig;
      data.message_ids = key_set_messages;
      data.message_ids.erase(data.message_ids.begin());
      data_list.push_back(data);
      return true;
    }
    else
    {
      wait_reason = tr("Wallet can't go multisig because key sets from other members missing or not complete.");
      return false;
    }
  }

  if (state.multisig && !state.multisig_is_ready)
  {
    // In the case of N-1/N multisig the call 'wallet2::multisig' returns already true
    // after "make_multisig" but before "finalize_multisig", but returns the parameter
    // 'ready' as false to document this particular state

    // Same story for finalizing key sets: If all are here we process them
    // It looks like the "finalize_multisig" command would also process less than all key sets,
    // and maybe also correctly so, but the MMS does not support that case and insists on completeness
    std::vector<uint32_t> finalizing_key_set_messages(m_coalition_size, 0);

    for (size_t i = 0; i < m_messages.size(); ++i)
    {
      message &m = m_messages[i];
      if ((m.type == message_type_finalizing_key_set) && (m.state == message_state_waiting))
      {
	if (finalizing_key_set_messages[m.member_index] == 0)
	{
	  finalizing_key_set_messages[m.member_index] = m.id;
	}
	// else duplicate key set, ignore
      }
    }

    bool key_sets_complete = message_ids_complete(finalizing_key_set_messages);
    if (key_sets_complete)
    {
      processing_data data;
      data.processing = message_processing_finalize_multisig;
      data.message_ids = finalizing_key_set_messages;
      data.message_ids.erase(data.message_ids.begin());
      data_list.push_back(data);
      return true;
    }
    else
    {
      wait_reason = tr("Wallet can't finalize multisig because key sets from other members missing or not complete.");
      return false;
    }
  }

  // Properly exchanging multisig sync data is easiest and most transparent
  // for the user if a wallet sends its own data first and processes any received
  // sync data afterwards so that's the order that the MMS enforces here.
  // (Technically, it seems to work also the other way round.)
  //
  // To check whether a NEW round of syncing is necessary the MMS works with a
  // "wallet state": new state means new syncing needed.
  //
  // The MMS monitors the "wallet state" by recording "wallet heights" as
  // numbers of transfers present in a wallet at the time of message creation. While
  // not watertight, this quite simple scheme should already suffice to trigger
  // and orchestrate a sensible exchange of sync data.
  if (state.has_multisig_partial_key_images || force_sync)
  {
    // Sync is necessary and not yet completed: Processing of transactions 
    // will only be possible again once properly synced
    // Check first whether we generated already OUR sync info; take note of
    // any processable sync info from other members on the way in case we need it
    bool own_sync_data_created = false;
    std::vector<uint32_t> sync_messages(m_coalition_size, 0);
    for (size_t i = 0; i < m_messages.size(); ++i)
    {
      message &m = m_messages[i];
      if ((m.type == message_type_multisig_sync_data) && (force_sync || (m.wallet_height == wallet_height)))
      // It's data for the same "round" of syncing, on the same "wallet height", therefore relevant
      {
	if (m.direction == message_direction_out)
	{
	  own_sync_data_created = true;
	  // Ignore whether sent already or not, and assume as complete if several other members there
	}
	else if ((m.direction == message_direction_in) && (m.state == message_state_waiting))
	{
	  if (sync_messages[m.member_index] == 0)
	  {
	    sync_messages[m.member_index] = m.id;
	  }
	  // else duplicate sync message, ignore
	}
      }
    }
    if (!own_sync_data_created) {
      // As explained above, creating sync data BEFORE processing such data from
      // other members reliably works, so insist on that here
      processing_data data;
      data.processing = message_processing_create_sync_data;
      data_list.push_back(data);
      return true;
    }
    else if (message_ids_complete(sync_messages))
    {
      processing_data data;
      data.processing = message_processing_process_sync_data;
      data.message_ids = sync_messages;
      data.message_ids.erase(data.message_ids.begin());
      data_list.push_back(data);
      return true;
    }
    else
    {
      // We can't proceed to any transactions until we have synced
      wait_reason = tr("Syncing not possible because multisig sync data from other members missing or not complete.");
      return false;
    }
  }

  bool waiting_found = false;
  for (size_t i = 0; i < m_messages.size(); ++i)
  {
    message &m = m_messages[i];
    if (m.state == message_state_waiting)
    {
      waiting_found = true;
      if (m.type == message_type_fully_signed_tx)
      {
	// We can either submit it ourselves, or send it to any other member for submission
	processing_data data;
	data.processing = message_processing_submit_tx;
	data.message_ids.push_back(m.id);
	data_list.push_back(data);

	data.processing = message_processing_send_tx;
	for (size_t j = 1; j < m_members.size(); ++j)
	{
	  data.receiving_member_index = j;
	  data_list.push_back(data);
	}
	return true;
      }
      else if (m.type == message_type_partially_signed_tx)
      {
	if (m.member_index == 0)
	{
	  // We started this ourselves, or signed it but with still signatures missing:
	  // We can send it to any other member for signing / further signing
	  // In principle it does not make sense to send it back to somebody who
	  // already signed, but the MMS does not / not yet keep track of that,
	  // because that would be somewhat complicated.
	  processing_data data;
	  data.processing = message_processing_send_tx;
	  data.message_ids.push_back(m.id);
	  for (size_t j = 1; j < m_members.size(); ++j)
	  {
	    data.receiving_member_index = j;
	    data_list.push_back(data);
	  }
	  return true;
	}
	else
	{
	  // Somebody else sent this to us: We can sign it
	  // It would be possible to just pass it on, but that's not directly supported here
	  processing_data data;
	  data.processing = message_processing_sign_tx;
	  data.message_ids.push_back(m.id);
	  data_list.push_back(data);
	  return true;
	}
      }
    }
  }
  if (waiting_found)
  {
    wait_reason = tr("Waiting message is not a tx and thus not processable now.");
  }
  else
  {
    wait_reason = tr("There is no message waiting to be processed.");
  }

  return false;
}

void message_store::set_messages_processed(const processing_data &data)
{
  for (size_t i = 0; i < data.message_ids.size(); ++i)
  {
    set_message_processed_or_sent(data.message_ids[i]);
  }
}

void message_store::set_message_processed_or_sent(uint32_t id)
{
  uint32_t index = get_message_index_by_id(id);
  message &m = m_messages[index];
  if (m.state == message_state_waiting)
  {
    m.state = message_state_processed;
    // So far a fairly cautious and conservative strategy: Only delete from Bitmessage
    // when fully processed (and e.g. not already after reception and writing into
    // the message store file)
    delete_transport_message(id);
  }
  else if (m.state == message_state_ready_to_send)
  {
    m.state = message_state_sent;
  }
}

void message_store::encrypt(uint32_t member_index, const std::string &plaintext, 
			    std::string &ciphertext, crypto::public_key &encryption_public_key, crypto::chacha_iv &iv)
{
  crypto::secret_key encryption_secret_key;
  crypto::generate_keys(encryption_public_key, encryption_secret_key);
  
  crypto::key_derivation derivation;
  crypto::public_key dest_view_public_key = m_members[member_index].monero_address.m_view_public_key;
  crypto::generate_key_derivation(dest_view_public_key, encryption_secret_key, derivation);

  crypto::chacha_key chacha_key;
  crypto::generate_chacha_key(&derivation, sizeof(derivation), chacha_key);
  iv = crypto::rand<crypto::chacha_iv>();
  ciphertext.resize(plaintext.size());
  crypto::chacha20(plaintext.data(), plaintext.size(), chacha_key, iv, &ciphertext[0]);
}

void message_store::decrypt(const std::string &ciphertext, const crypto::public_key &encryption_public_key, const crypto::chacha_iv &iv,
			    std::string &plaintext)
{
  wallet_state state;
  m_wallet_state_provider->get_wallet_state(state);
  crypto::secret_key& view_secret_key = state.view_secret_key;
  if (state.multisig)
  {
    if (state.original_keys_available)
    {
      view_secret_key = state.original_view_secret_key;
    }
    else
    {
      throw std::invalid_argument("Own view secret key not available");
    }
  }
  crypto::key_derivation derivation;
  crypto::generate_key_derivation(encryption_public_key, view_secret_key, derivation);
  crypto::chacha_key chacha_key;
  crypto::generate_chacha_key(&derivation, sizeof(derivation), chacha_key);
  plaintext.resize(ciphertext.size());
  crypto::chacha20(ciphertext.data(), ciphertext.size(), chacha_key, iv, &plaintext[0]);
}

void message_store::send_message(uint32_t id)
{
  uint32_t index = get_message_index_by_id(id);
  message m = m_messages[index];
  transport_message dm;
  dm.sender_address = m_members[0].monero_address;
  dm.internal_message = m;
  encrypt(m.member_index, m.content, dm.internal_message.content, dm.encryption_public_key, dm.iv);
  std::string transport_address = m_members[m.member_index].transport_address;
  if (transport_address.find("BM-") == 0)
  {
    // Take the transport address of the member as Bitmessage address and use the messaging daemon
    boost::optional<epee::net_utils::http::login> messaging_daemon_login{};
    m_messaging_daemon.set_server("http://localhost:18083/", messaging_daemon_login, false);
    bool connected = m_messaging_daemon.connect(std::chrono::milliseconds(1000));
/*
 *  cryptonote::account_public_address source_monero_address;
    std::string source_transport_address;
    cryptonote::account_public_address destination_monero_address;
    std::string destination_transport_address;
    crypto::chacha_iv iv;
    crypto::public_key encryption_public_key;
    uint64_t timestamp;
    std::string content;
    crypto::hash hash;
    std::string signature;
    std::string transport_id;
*/
    messaging_daemon_rpc::message rm;
    rm.source_monero_address = dm.sender_address;
    rm.source_transport_address = m_members[0].transport_address;
    rm.destination_monero_address = m_members[m.member_index].monero_address;
    rm.destination_transport_address = transport_address;
    rm.iv = dm.iv;
    rm.encryption_public_key = dm.encryption_public_key;
    rm.timestamp = time(NULL);
    rm.type = (uint32_t)dm.internal_message.type;
    rm.content = dm.internal_message.content;
    rm.hash = crypto::cn_fast_hash(rm.content.data(), rm.content.size());
    
    messaging_daemon_rpc::COMMAND_RPC_SEND_MESSAGE::request req;
    messaging_daemon_rpc::COMMAND_RPC_SEND_MESSAGE::response res;
    req.msg = rm;
    bool r = epee::net_utils::invoke_http_json_rpc("/json_rpc", "send_message", req, res, m_messaging_daemon, std::chrono::milliseconds(1000));
  }
  else
  {
    // Take the transport address of the member as a subdirectory and write the message as
    // file "debug_message" there
    std::string filename = transport_address + "/debug_message";

    std::stringstream oss;
    boost::archive::portable_binary_oarchive ar(oss);
    ar << dm;
    std::string buf = oss.str();
    bool r = epee::file_io_utils::save_string_to_file(filename, buf);
  }

  m_messages[index].state=message_state_sent;
}

bool message_store::receive_message()
{
  std::string transport_address = m_members[0].transport_address;
  if (transport_address.find("BM-") == 0)
  {
      // Take the transport address of "me" as Bitmessage address and use the messaging daemon
    boost::optional<epee::net_utils::http::login> messaging_daemon_login{};
    m_messaging_daemon.set_server("http://localhost:18083/", messaging_daemon_login, false);
    bool connected = m_messaging_daemon.connect(std::chrono::milliseconds(1000));
    
    messaging_daemon_rpc::COMMAND_RPC_RECEIVE_MESSAGES::request req;
    messaging_daemon_rpc::COMMAND_RPC_RECEIVE_MESSAGES::response res;
    req.destination_monero_address = m_members[0].monero_address;
    req.destination_transport_address = transport_address;
    bool r = epee::net_utils::invoke_http_json_rpc("/json_rpc", "receive_messages", req, res, m_messaging_daemon, std::chrono::milliseconds(10000));
    
    bool new_messages = false;
    for (size_t i = 0; i < res.msg.size(); ++i)
    {
      messaging_daemon_rpc::message rm = res.msg[i];
      if (any_message_with_hash(rm.hash))
      {
	// Already seen, do not take again
      }
      else
      {
        uint32_t sender_index = member_index_by_monero_address(rm.source_monero_address);
	if (sender_index == 0)
	{
	  // From an address that is not a member here: Ignore
	}
	else
	{
	  std::string plaintext;
	  decrypt(rm.content, rm.encryption_public_key, rm.iv, plaintext);
	  uint32_t index = add_message(sender_index, (message_type)rm.type, message_direction_in, plaintext);
	  m_messages[index].hash = rm.hash;
	  m_messages[index].transport_id = rm.transport_id;
	  new_messages = true;
	}
      }
    }
    return new_messages;
  }
  else
  {
    // Take the transport address of "me" as a subdirectory and read a single new
    // message from a file "debug_message" there, assumed to come from member 1
    std::string filename = m_members[0].transport_address + "/debug_message";
    boost::system::error_code ignored_ec;
    bool file_exists = boost::filesystem::exists(filename, ignored_ec);
    if (!file_exists) {
      return false;
    }

    transport_message dm;
    std::string buf;
    bool r = epee::file_io_utils::load_file_to_string(filename, buf);
    std::stringstream iss;
    iss << buf;
    boost::archive::portable_binary_iarchive ar(iss);
    ar >> dm;

    uint32_t sender_index = member_index_by_monero_address(dm.sender_address);
    std::string plaintext;
    decrypt(dm.internal_message.content, dm.encryption_public_key, dm.iv, plaintext);
    add_message(sender_index, dm.internal_message.type, message_direction_in, plaintext);
    boost::filesystem::remove(filename);
    return true;
  }
  return false;
}

void message_store::delete_transport_message(uint32_t id)
{
  uint32_t index = get_message_index_by_id(id);
  message m = m_messages[index];
  if (!m.transport_id.empty())
  {
    boost::optional<epee::net_utils::http::login> messaging_daemon_login{};
    m_messaging_daemon.set_server("http://localhost:18083/", messaging_daemon_login, false);
    bool connected = m_messaging_daemon.connect(std::chrono::milliseconds(1000));
    
    messaging_daemon_rpc::COMMAND_RPC_DELETE_MESSAGE::request req;
    messaging_daemon_rpc::COMMAND_RPC_DELETE_MESSAGE::response res;
    req.transport_id = m.transport_id;
    bool r = epee::net_utils::invoke_http_json_rpc("/json_rpc", "delete_message", req, res, m_messaging_daemon, std::chrono::milliseconds(1000));
  }
}

const char *message_store::message_type_to_string(message_type type) {
  switch (type)
  {
  case message_type_key_set:
    return tr("key set");
  case message_type_finalizing_key_set:
    return tr("finalizing key set");
  case message_type_multisig_sync_data:
    return tr("multisig sync data");
  case message_type_partially_signed_tx:
    return tr("partially signed tx");
  case message_type_fully_signed_tx:
    return tr("fully signed tx");
  default:
    return tr("unknown message type");
  }
}

const char *message_store::message_direction_to_string(message_direction direction) {
  switch (direction)
  {
  case message_direction_in:
    return tr("in");
  case message_direction_out:
    return tr("out");
  default:
    return tr("unknown message direction");
  }
}

const char *message_store::message_state_to_string(message_state state) {
  switch (state)
  {
  case message_state_ready_to_send:
    return tr("ready to send");
  case message_state_sent:
    return tr("sent");
  case message_state_waiting:
    return tr("waiting");
  case message_state_processed:
    return tr("processed");
  case message_state_cancelled:
    return tr("cancelled");
  default:
    return tr("unknown message state");
  }
}

// Convert a member to string suitable for a column in a list, with 'max_width'
// Format: label (transport_address)
std::string message_store::member_to_string(const coalition_member &member, uint32_t max_width) {
  std::string s = "";
  s.reserve(max_width);
  uint32_t avail = max_width;
  uint32_t label_len = member.label.length();
  if (label_len > avail)
  {
    s.append(member.label.substr(0, avail - 3));
    s.append("...");
    return s;
  }
  s.append(member.label);
  avail -= label_len;
  uint32_t transport_addr_len = member.transport_address.length();
  if ((transport_addr_len > 0) && (avail > 10))
  {
    s.append(" (");
    avail -= 3;  // including closing ")"
    if (transport_addr_len <= avail)
    {
      s.append(member.transport_address);
    }
    else
    {
      s.append(member.transport_address.substr(0, avail-3));
      s.append("...");
    }
    s.append(")");
  }
  return s;
}


}
