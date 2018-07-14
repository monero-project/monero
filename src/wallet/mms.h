/* 
 * File:   mms.h
 * Author: root
 *
 * Created on May 11, 2018, 2:40 PM
 */

#pragma once

#include <cstdlib>
#include <string>
#include <vector>
#include "crypto/hash.h"
#include <boost/serialization/vector.hpp>
#include "serialization/serialization.h"
#include "cryptonote_basic/cryptonote_boost_serialization.h"
#include "cryptonote_basic/account_boost_serialization.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "common/i18n.h"
#include "net/http_server_impl_base.h"
#include "net/http_client.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet.mms"

namespace tools
{
  enum message_type
  {
    message_type_key_set,
    message_type_finalizing_key_set,
    message_type_multisig_sync_data,
    message_type_partially_signed_tx,
    message_type_fully_signed_tx
  };
  
  enum message_direction
  {
    message_direction_in,
    message_direction_out
  };
  
  enum message_state
  {
    message_state_ready_to_send,
    message_state_sent,
    
    message_state_waiting,
    message_state_processed,
    
    message_state_cancelled
  };
  
  enum message_processing
  {
    message_processing_prepare_multisig,
    message_processing_make_multisig,
    message_processing_finalize_multisig,
    message_processing_create_sync_data,
    message_processing_process_sync_data,
    message_processing_sign_tx,
    message_processing_send_tx,
    message_processing_submit_tx
  };
  
  struct message
  {
    uint32_t id;
    message_type type;
    message_direction direction;
    std::string content;
    uint64_t timestamp;
    uint32_t member_index;
    crypto::hash hash;
    message_state state;
    uint32_t wallet_height;
    std::string transport_id;
  };
  // "wallet_height" (for lack of a short name that would describe what it is about)
  // is the number of transfers present in the wallet at the time of message
  // construction; used to coordinate generation of sync info (which depends
  // on the content of the wallet at time of generation)
  
  struct coalition_member
  {
    cryptonote::account_public_address monero_address;
    std::string transport_address;
    std::string label;
    bool me;
    uint32_t index;
  };
  
  struct processing_data
  {
    message_processing processing;
    std::vector<uint32_t> message_ids;
    uint32_t receiving_member_index = 0;
  };
  
  struct transport_message
  {
    cryptonote::account_public_address sender_address;
    crypto::chacha_iv iv;
    crypto::public_key encryption_public_key;
    message internal_message;
  };
  
  struct wallet_state
  {
    cryptonote::account_public_address address;
    cryptonote::network_type nettype;
    crypto::secret_key view_secret_key;
    bool multisig;
    bool multisig_is_ready;
    bool has_multisig_partial_key_images;
    size_t num_transfer_details;
    
    bool original_keys_available;
    cryptonote::account_public_address original_address;
    crypto::secret_key original_view_secret_key;
  };

  // The construct with "i_message_state_provider solves the problem that the message store
  // needs access to some state info from a "wallet2" object, but you can't just hand over
  // such an object directly to a "message_store" because that would result in circular
  // references. Solution: "wallet2" implements the "i_message_state_provider" interface.
  class i_wallet_state_provider
  {
  public:
    virtual void get_wallet_state(wallet_state &state) {}
    virtual void get_num_transfer_details(size_t &num) {}
    virtual ~i_wallet_state_provider() {}
  };
  
  class message_store
  {
  public:
    message_store();
    message_store(i_wallet_state_provider *wallet_state_provider);
    // Initialize and start to use the MMS, set the first member, this wallet itself
    // reset it if already used, with deletion of all members and messages
    void init(i_wallet_state_provider *wallet_state_provider, 
              const std::string &own_transport_address, uint32_t coalition_size, uint32_t threshold);
    void set_active(bool active) { m_active = active; };
    bool is_active() const { return m_active; };
    uint32_t get_threshold() const { return m_threshold; };
    uint32_t get_coalition_size() const { return m_coalition_size; };
    
    uint32_t add_member(const std::string &label, const cryptonote::account_public_address &monero_address,
            const std::string &transport_address);
    const coalition_member &get_member(uint32_t index) const;
    uint32_t member_index_by_monero_address(const cryptonote::account_public_address &monero_address) const;
    uint32_t member_index_by_transport_address(const std::string &transport_address) const;
    const std::vector<coalition_member> &get_all_members() const { return m_members; };
    
    // Process data just created by "me" i.e. the own local wallet, e.g. as the result of a "prepare_multisig" command
    // Creates the resulting messages to the right members
    void process_wallet_created_data(message_type type, const std::string &content);
    
    // Go through all the messages, look at the "ready to process" ones, and check whether any single one
    // or any group of them can be processed, because they are processable as single messages (like a tx
    // that is fully signed and thus ready for submit to the net) or because they form a complete group
    // (e.g. key sets from all coalition members to make the wallet multisig). If there are multiple
    // candidates, e.g. in 2/3 multisig sending to one OR the other member to sign, there will be more
    // than 1 element in 'data' for the user to choose. If nothing is ready "false" is returned.
    // The method mostly ignores the order in which the messages were received because messages may be delayed
    // (e.g. sync data from a member arrives AFTER a transaction to submit) or because message time stamps
    // may be wrong so it's not possible to order them reliably.
    // Messages also may be ready by themselves but the wallet not yet ready for them (e.g. sync data already
    // arriving when the wallet is not yet multisig because key sets were delayed or were lost altogether.)
    // If nothing is ready 'wait_reason' may contain further info about the reason why.
    bool get_processable_messages(bool force_sync,
                                  std::vector<processing_data> &data_list,
                                  std::string &wait_reason);
    void set_messages_processed(const processing_data &data);
    
    uint32_t add_message(uint32_t member_index, message_type type, message_direction direction, 
                         const std::string &content);
    const std::vector<message> &get_all_messages() const { return m_messages; };
    message get_message_by_id(uint32_t id);
    void set_message_processed_or_sent(uint32_t id);
    void delete_message(uint32_t id);
    void delete_all_messages();
    
    // Debugging support: Read and write the contents of single messages into files for "sending" and "receiving" them
    void send_message(uint32_t id);
    bool receive_message();
    
    void write_to_file(const std::string &filename);
    void read_from_file(const std::string &filename);
    
    template <class t_archive>
    inline void serialize(t_archive &a, const unsigned int ver)
    {
      a & m_active;
      a & m_coalition_size;
      if (ver > 0)
      {
        a & m_nettype;
      }
      a & m_threshold;
      a & m_members;
      a & m_messages;
      a & m_next_message_id;
    }

    const char *message_type_to_string(message_type type);
    const char *message_direction_to_string(message_direction direction);
    const char *message_state_to_string(message_state state);
    std::string member_to_string(const coalition_member &member, uint32_t max_width);
    
    static const char *tr(const char *str) { return i18n_translate(str, "tools::mms"); }

  private:
    bool m_active;
    uint32_t m_coalition_size;
    uint32_t m_threshold;
    cryptonote::network_type m_nettype;
    std::vector<coalition_member> m_members;
    std::vector<message> m_messages;
    uint32_t m_next_message_id;
    std::string m_filename;
    i_wallet_state_provider *m_wallet_state_provider;
    epee::net_utils::http::http_simple_client m_messaging_daemon;
    
    uint32_t get_message_index_by_id(uint32_t id);
    bool any_message_of_type(message_type type, message_direction direction) const;
    bool any_message_with_hash(const crypto::hash &hash) const;
    bool message_ids_complete(const std::vector<uint32_t> ids) const;
    void encrypt(uint32_t member_index, const std::string &plaintext, 
                 std::string &ciphertext, crypto::public_key &encryption_public_key, crypto::chacha_iv &iv);
    void decrypt(const std::string &ciphertext, const crypto::public_key &encryption_public_key, const crypto::chacha_iv &iv,
                 std::string &plaintext);
    void delete_transport_message(uint32_t id);
  };
  
}

BOOST_CLASS_VERSION(tools::message_store, 1)
BOOST_CLASS_VERSION(tools::message, 1)
BOOST_CLASS_VERSION(tools::transport_message, 0)
BOOST_CLASS_VERSION(tools::coalition_member, 0)

namespace boost
{
  namespace serialization
  {
    template <class Archive>
    inline void serialize(Archive &a, tools::message &x, const boost::serialization::version_type ver)
    {
      a & x.id;
      a & x.type;
      a & x.direction;
      a & x.content;
      a & x.timestamp;
      a & x.member_index;
      a & x.hash;
      a & x.state;
      a & x.wallet_height;
      if (ver > 0)
      {
        a & x.transport_id;
      }
    }
    
    template <class Archive>
    inline void serialize(Archive &a, tools::coalition_member &x, const boost::serialization::version_type ver)
    {
      a & x.monero_address;
      a & x.transport_address;
      a & x.label;
      a & x.me;
      a & x.index;
    }

    template <class Archive>
    inline void serialize(Archive &a, tools::transport_message &x, const boost::serialization::version_type ver)
    {
      a & x.sender_address;
      a & x.iv;
      a & x.encryption_public_key;
      a & x.internal_message;
    }
    
    template <class Archive>
    inline void serialize(Archive &a, crypto::chacha_iv &x, const boost::serialization::version_type ver)
    {
      a & x.data;
    }

  }
}