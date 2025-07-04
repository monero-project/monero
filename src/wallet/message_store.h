// Copyright (c) 2018-2024, The Monero Project

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

#pragma once

#include <cstdlib>
#include <string>
#include <vector>
#include "crypto/hash.h"
#include <boost/serialization/vector.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/optional/optional.hpp>
#include "serialization/serialization.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "common/i18n.h"
#include "common/command_line.h"
#include "wipeable_string.h"
#include "net/abstract_http_client.h"
#include "serialization/crypto.h"
#include "serialization/string.h"
#include "serialization/containers.h"
#include "message_transporter.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet.mms"
#define AUTO_CONFIG_TOKEN_BYTES 4
#define AUTO_CONFIG_TOKEN_PREFIX "mms"

namespace mms
{
  enum class message_type
  {
    key_set,
    additional_key_set,
    multisig_sync_data,
    partially_signed_tx,
    fully_signed_tx,
    note,
    signer_config,
    auto_config_data
  };

  enum class message_direction
  {
    in,
    out
  };

  enum class message_state
  {
    ready_to_send,
    sent,

    waiting,
    processed,

    cancelled
  };

  enum class message_processing
  {
    prepare_multisig,
    make_multisig,
    exchange_multisig_keys,
    create_sync_data,
    process_sync_data,
    sign_tx,
    send_tx,
    submit_tx,
    process_signer_config,
    process_auto_config_data
  };

  struct message
  {
    uint32_t id;
    message_type type;
    message_direction direction;
    std::string content;
    uint64_t created;
    uint64_t modified;
    uint64_t sent;
    uint32_t signer_index;
    crypto::hash hash;
    message_state state;
    uint32_t wallet_height;
    uint32_t round;
    uint32_t signature_count;
    std::string transport_id;

    BEGIN_SERIALIZE_OBJECT()
      VERSION_FIELD(0)
      VARINT_FIELD(id)
      VARINT_FIELD(type)
      VARINT_FIELD(direction)
      FIELD(content)
      VARINT_FIELD(created)
      VARINT_FIELD(modified)
      VARINT_FIELD(sent)
      VARINT_FIELD(signer_index)
      FIELD(hash)
      VARINT_FIELD(state)
      VARINT_FIELD(wallet_height)
      VARINT_FIELD(round)
      VARINT_FIELD(signature_count)
      FIELD(transport_id)
    END_SERIALIZE()
  };
  // "wallet_height" (for lack of a short name that would describe what it is about)
  // is the number of transfers present in the wallet at the time of message
  // construction; used to coordinate generation of sync info (which depends
  // on the content of the wallet at time of generation)

  struct authorized_signer
  {
    std::string label;
    std::string transport_address;
    bool monero_address_known;
    cryptonote::account_public_address monero_address;
    bool me;
    uint32_t index;
    std::string auto_config_token;
    crypto::public_key auto_config_public_key;
    crypto::secret_key auto_config_secret_key;
    std::string auto_config_transport_address;
    bool auto_config_running;

    BEGIN_SERIALIZE_OBJECT()
      VERSION_FIELD(0)
      FIELD(label)
      FIELD(transport_address)
      FIELD(monero_address_known)
      FIELD(monero_address)
      FIELD(me)
      VARINT_FIELD(index)
      FIELD(auto_config_token)
      FIELD(auto_config_public_key)
      FIELD(auto_config_secret_key)
      FIELD(auto_config_transport_address)
      FIELD(auto_config_running)
    END_SERIALIZE()

    authorized_signer()
    {
      monero_address_known = false;
      memset(&monero_address, 0, sizeof(cryptonote::account_public_address));
      me = false;
      index = 0;
      auto_config_public_key = crypto::null_pkey;
      auto_config_secret_key = crypto::null_skey;
      auto_config_running = false;
    };
  };

  struct processing_data
  {
    message_processing processing;
    std::vector<uint32_t> message_ids;
    uint32_t receiving_signer_index = 0;
  };

  struct file_transport_message
  {
    cryptonote::account_public_address sender_address;
    crypto::chacha_iv iv;
    crypto::public_key encryption_public_key;
    message internal_message;
  };
  
  struct auto_config_data
  {
    std::string label;
    std::string transport_address;
    cryptonote::account_public_address monero_address;

    BEGIN_SERIALIZE_OBJECT()
      VERSION_FIELD(0)
      FIELD(label)
      FIELD(transport_address)
      FIELD(monero_address)
    END_SERIALIZE()
  };

  // Overal .mms file structure, with the "message_store" object serialized to and
  // encrypted in "encrypted_data"
  struct file_data
  {
    std::string magic_string;
    uint32_t file_version;
    crypto::chacha_iv iv;
    std::string encrypted_data;

    BEGIN_SERIALIZE_OBJECT()
      FIELD(magic_string)
      FIELD(file_version)
      FIELD(iv)
      FIELD(encrypted_data)
    END_SERIALIZE()
  };

    // The following struct provides info about the current state of a "wallet2" object
  // at the time of a "message_store" method call that those methods need. See on the
  // one hand a first parameter of this type for several of those methods, and on the
  // other hand the method "wallet2::get_multisig_wallet_state" which clients like the
  // CLI wallet can use to get that info.
  //
  // Note that in the case of a wallet that is already multisig "address" is NOT the
  // multisig address, but the "original" wallet address at creation time. Likewise
  // "view_secret_key" is the original view secret key then.
  //
  // This struct definition is here and not in "wallet2.h" to avoid circular imports.
  struct multisig_wallet_state
  {
    cryptonote::account_public_address address;
    cryptonote::network_type nettype;
    crypto::secret_key view_secret_key;
    bool multisig;
    bool multisig_is_ready;
    bool multisig_kex_is_done;
    bool has_multisig_partial_key_images;
    uint32_t multisig_rounds_passed;
    size_t num_transfer_details;
    std::string mms_file;

    BEGIN_SERIALIZE_OBJECT()
      VERSION_FIELD(1)
      FIELD(address)
      VARINT_FIELD(nettype)
      FIELD(view_secret_key)
      FIELD(multisig)
      FIELD(multisig_is_ready)
      if (version > 0)
        FIELD(multisig_kex_is_done)
      else
        multisig_kex_is_done = multisig_is_ready;
      FIELD(has_multisig_partial_key_images)
      VARINT_FIELD(multisig_rounds_passed)
      VARINT_FIELD(num_transfer_details)
      FIELD(mms_file)
    END_SERIALIZE()
  };

  class message_store
  {
  public:
    message_store(std::unique_ptr<epee::net_utils::http::abstract_http_client> http_client);

    // Initialize and start to use the MMS, set the first signer, this wallet itself
    // Filename, if not null and not empty, is used to create the ".mms" file
    // reset it if already used, with deletion of all signers and messages
    void init(const multisig_wallet_state &state, const std::string &own_label,
              const std::string &own_transport_address, uint32_t num_authorized_signers, uint32_t num_required_signers);
    void set_active(bool active) { m_active = active; };
    void set_auto_send(bool auto_send) { m_auto_send = auto_send; };
    void set_options(const boost::program_options::variables_map& vm);
    void set_options(const std::string &bitmessage_address, const epee::wipeable_string &bitmessage_login);
    bool get_active() const { return m_active; };
    bool get_auto_send() const { return m_auto_send; };
    uint32_t get_num_required_signers() const { return m_num_required_signers; };
    uint32_t get_num_authorized_signers() const { return m_num_authorized_signers; };

    void set_signer(const multisig_wallet_state &state,
                    uint32_t index,
                    const boost::optional<std::string> &label,
                    const boost::optional<std::string> &transport_address,
                    const boost::optional<cryptonote::account_public_address> monero_address);

    const authorized_signer &get_signer(uint32_t index) const;
    bool get_signer_index_by_monero_address(const cryptonote::account_public_address &monero_address, uint32_t &index) const;
    bool get_signer_index_by_label(const std::string label, uint32_t &index) const;
    const std::vector<authorized_signer> &get_all_signers() const { return m_signers; };
    bool signer_config_complete() const;
    bool signer_labels_complete() const;
    void get_signer_config(std::string &signer_config);
    void unpack_signer_config(const multisig_wallet_state &state, const std::string &signer_config,
                              std::vector<authorized_signer> &signers);
    void process_signer_config(const multisig_wallet_state &state, const std::string &signer_config);

    void start_auto_config(const multisig_wallet_state &state);
    bool check_auto_config_token(const std::string &raw_token,
                                 std::string &adjusted_token) const;
    size_t add_auto_config_data_message(const multisig_wallet_state &state,
                                        const std::string &auto_config_token);
    void process_auto_config_data_message(uint32_t id);
    std::string get_config_checksum() const;
    void stop_auto_config();

    // Process data just created by "me" i.e. the own local wallet, e.g. as the result of a "prepare_multisig" command
    // Creates the resulting messages to the right signers
    void process_wallet_created_data(const multisig_wallet_state &state, message_type type, const std::string &content);

    // Go through all the messages, look at the "ready to process" ones, and check whether any single one
    // or any group of them can be processed, because they are processable as single messages (like a tx
    // that is fully signed and thus ready for submit to the net) or because they form a complete group
    // (e.g. key sets from all authorized signers to make the wallet multisig). If there are multiple
    // candidates, e.g. in 2/3 multisig sending to one OR the other signer to sign, there will be more
    // than 1 element in 'data' for the user to choose. If nothing is ready "false" is returned.
    // The method mostly ignores the order in which the messages were received because messages may be delayed
    // (e.g. sync data from a signer arrives AFTER a transaction to submit) or because message time stamps
    // may be wrong so it's not possible to order them reliably.
    // Messages also may be ready by themselves but the wallet not yet ready for them (e.g. sync data already
    // arriving when the wallet is not yet multisig because key sets were delayed or were lost altogether.)
    // If nothing is ready 'wait_reason' may contain further info about the reason why.
    bool get_processable_messages(const multisig_wallet_state &state,
                                  bool force_sync,
                                  std::vector<processing_data> &data_list,
                                  std::string &wait_reason);
    void set_messages_processed(const processing_data &data);

    size_t add_message(const multisig_wallet_state &state,
                       uint32_t signer_index, message_type type, message_direction direction,
                       const std::string &content);
    const std::vector<message> &get_all_messages() const { return m_messages; };
    bool get_message_by_id(uint32_t id, message &m) const;
    message get_message_by_id(uint32_t id) const;
    void set_message_processed_or_sent(uint32_t id);
    void delete_message(uint32_t id);
    void delete_all_messages();
    static std::string get_sanitized_text(const std::string &text, size_t max_length);

    void send_message(const multisig_wallet_state &state, uint32_t id);
    bool check_for_messages(const multisig_wallet_state &state, std::vector<message> &messages);
    void stop() { m_run.store(false, std::memory_order_relaxed); m_transporter.stop(); }

    void write_to_file(const multisig_wallet_state &state, const std::string &filename);
    void read_from_file(const multisig_wallet_state &state, const std::string &filename);

    template <class t_archive>
    inline void serialize(t_archive &a, const unsigned int ver)
    {
      a & m_active;
      a & m_num_authorized_signers;
      a & m_nettype;
      a & m_num_required_signers;
      a & m_signers;
      a & m_messages;
      a & m_next_message_id;
      a & m_auto_send;
    }

    BEGIN_SERIALIZE_OBJECT()
      VERSION_FIELD(0)
      FIELD(m_active)
      VARINT_FIELD(m_num_authorized_signers)
      VARINT_FIELD(m_nettype)
      VARINT_FIELD(m_num_required_signers)
      FIELD(m_signers)
      FIELD(m_messages)
      VARINT_FIELD(m_next_message_id)
      FIELD(m_auto_send)
    END_SERIALIZE()

    static const char* message_type_to_string(message_type type);
    static const char* message_direction_to_string(message_direction direction);
    static const char* message_state_to_string(message_state state);
    std::string signer_to_string(const authorized_signer &signer, uint32_t max_width);
    
    static const char *tr(const char *str) { return i18n_translate(str, "tools::mms"); }
    static void init_options(boost::program_options::options_description& desc_params);

  private:
    bool m_active;
    uint32_t m_num_authorized_signers;
    uint32_t m_num_required_signers;
    bool m_auto_send;
    cryptonote::network_type m_nettype;
    std::vector<authorized_signer> m_signers;
    std::vector<message> m_messages;
    uint32_t m_next_message_id;
    std::string m_filename;
    message_transporter m_transporter;
    std::atomic<bool> m_run;

    bool get_message_index_by_id(uint32_t id, size_t &index) const;
    size_t get_message_index_by_id(uint32_t id) const;
    message& get_message_ref_by_id(uint32_t id);
    bool any_message_of_type(message_type type, message_direction direction) const;
    bool any_message_with_hash(const crypto::hash &hash) const;
    size_t get_other_signers_id_count(const std::vector<uint32_t> &ids) const;
    bool message_ids_complete(const std::vector<uint32_t> &ids) const;
    void encrypt(crypto::public_key public_key, const std::string &plaintext,
                 std::string &ciphertext, crypto::public_key &encryption_public_key, crypto::chacha_iv &iv);
    void decrypt(const std::string &ciphertext, const crypto::public_key &encryption_public_key, const crypto::chacha_iv &iv,
                 const crypto::secret_key &view_secret_key, std::string &plaintext);
    std::string create_auto_config_token();
    void setup_signer_for_auto_config(uint32_t index, const std::string token, bool receiving);
    void delete_transport_message(uint32_t id);
    std::string account_address_to_string(const cryptonote::account_public_address &account_address) const;
    void save(const multisig_wallet_state &state);
  };
}
