// Copyright (c) 2017-2022, The Monero Project
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

#ifndef MONERO_DEVICE_TREZOR_BASE_H
#define MONERO_DEVICE_TREZOR_BASE_H


#include <cstddef>
#include <string>
#include "device/device.hpp"
#include "device/device_default.hpp"
#include "device/device_cold.hpp"
#include <boost/scope_exit.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include "cryptonote_config.h"
#include "trezor.hpp"

#ifdef WITH_TREZOR_DEBUGGING
#include "trezor/debug_link.hpp"
#endif

//automatic lock one more level on device ensuring the current thread is allowed to use it
#define TREZOR_AUTO_LOCK_CMD() \
  /* lock both mutexes without deadlock*/ \
  boost::lock(device_locker, command_locker); \
  /* make sure both already-locked mutexes are unlocked at the end of scope */ \
  boost::lock_guard<boost::recursive_mutex> lock1(device_locker, boost::adopt_lock); \
  boost::lock_guard<boost::mutex> lock2(command_locker, boost::adopt_lock)

#define TREZOR_AUTO_LOCK_DEVICE() boost::lock_guard<boost::recursive_mutex> lock1_device(device_locker)

namespace hw {
namespace trezor {

#ifdef WITH_DEVICE_TREZOR
  class device_trezor_base;

#ifdef WITH_TREZOR_DEBUGGING
    class trezor_debug_callback : public hw::i_device_callback {
    public:
      trezor_debug_callback()=default;
      explicit trezor_debug_callback(std::shared_ptr<Transport> & debug_transport);

      void on_button_request(uint64_t code=0) override;
      boost::optional<epee::wipeable_string> on_pin_request() override;
      boost::optional<epee::wipeable_string> on_passphrase_request(bool & on_device) override;
      void on_passphrase_state_request(const std::string &state);
      void on_disconnect();
    protected:
      std::shared_ptr<DebugLink> m_debug_link;
    };

#endif

  /**
   * TREZOR device template with basic functions
   */
  class device_trezor_base : public hw::core::device_default {
    protected:

      // Locker for concurrent access
      mutable boost::recursive_mutex  device_locker;
      mutable boost::mutex  command_locker;

      std::shared_ptr<Transport> m_transport;
      i_device_callback * m_callback;

      std::string m_full_name;
      std::vector<unsigned int> m_wallet_deriv_path;
      epee::wipeable_string m_device_session_id;  // returned after passphrase entry, session
      std::shared_ptr<messages::management::Features> m_features;  // features from the last device reset
      boost::optional<epee::wipeable_string> m_pin;
      boost::optional<epee::wipeable_string> m_passphrase;
      messages::MessageType m_last_msg_type;

      cryptonote::network_type network_type;
      bool m_reply_with_empty_passphrase;
      bool m_always_use_empty_passphrase;
      bool m_seen_passphrase_entry_message;

#ifdef WITH_TREZOR_DEBUGGING
      std::shared_ptr<trezor_debug_callback> m_debug_callback;
      bool m_debug;

      void setup_debug();
#endif

      //
      // Internal methods
      //

      void require_connected() const;
      void require_initialized() const;
      void call_ping_unsafe();
      void test_ping();
      virtual void device_state_initialize_unsafe();
      void ensure_derivation_path() noexcept;

      // Communication methods

      void write_raw(const google::protobuf::Message * msg);
      GenericMessage read_raw();
      GenericMessage call_raw(const google::protobuf::Message * msg);

      // Trezor message protocol handler. Handles specific signalling messages.
      bool message_handler(GenericMessage & input);

      /**
       * Client communication wrapper, handles specific Trezor protocol.
       *
       * @throws UnexpectedMessageException if the response message type is different than expected.
       * Exception contains message type and the message itself.
       */
      template<class t_message=google::protobuf::Message>
      std::shared_ptr<t_message>
      client_exchange(const std::shared_ptr<const google::protobuf::Message> &req,
                      const boost::optional<messages::MessageType> & resp_type = boost::none,
                      const boost::optional<std::vector<messages::MessageType>> & resp_types = boost::none,
                      const boost::optional<messages::MessageType*> & resp_type_ptr = boost::none,
                      bool open_session = false)
      {
        // Require strictly protocol buffers response in the template.
        BOOST_STATIC_ASSERT(boost::is_base_of<google::protobuf::Message, t_message>::value);
        const bool accepting_base = boost::is_same<google::protobuf::Message, t_message>::value;
        if (resp_types && !accepting_base){
          throw std::invalid_argument("Cannot specify list of accepted types and not using generic response");
        }

        // Determine type of expected message response
        const messages::MessageType required_type = accepting_base ? messages::MessageType_Success :
                  (resp_type ? resp_type.get() : MessageMapper::get_message_wire_number<t_message>());

        // Open session if required
        if (open_session){
          try {
            m_transport->open();
          } catch (const std::exception& e) {
            std::throw_with_nested(exc::SessionException("Could not open session"));
          }
        }

        // Scoped session closer
        BOOST_SCOPE_EXIT_ALL(&, this) {
          if (open_session && this->get_transport()){
            this->get_transport()->close();
          }
        };

        // Write/read the request
        CHECK_AND_ASSERT_THROW_MES(req, "Request is null");
        auto msg_resp = call_raw(req.get());

        bool processed = false;
        do {
          processed = message_handler(msg_resp);
        } while(processed);

        // Response section
        if (resp_type_ptr){
          *(resp_type_ptr.get()) = msg_resp.m_type;
        }

        if (msg_resp.m_type == messages::MessageType_Failure) {
          throw_failure_exception(dynamic_cast<messages::common::Failure *>(msg_resp.m_msg.get()));

        } else if (!accepting_base && msg_resp.m_type == required_type) {
          return message_ptr_retype<t_message>(msg_resp.m_msg);

        } else if (accepting_base && (!resp_types ||
                     std::find(resp_types.get().begin(), resp_types.get().end(), msg_resp.m_type) != resp_types.get().end())) {
            return message_ptr_retype<t_message>(msg_resp.m_msg);

        } else {
          throw exc::UnexpectedMessageException(msg_resp.m_type, msg_resp.m_msg);
        }
      }

      /**
       * Utility method to set address_n and network type to the message requets.
       */
      template<class t_message>
      void set_msg_addr(t_message * msg,
                        const boost::optional<std::vector<uint32_t>> & path = boost::none,
                        const boost::optional<cryptonote::network_type> & network_type = boost::none)
      {
        CHECK_AND_ASSERT_THROW_MES(msg, "Message is null");
        msg->clear_address_n();
        if (path){
          for(auto x : path.get()){
            msg->add_address_n(x);
          }
        } else {
          ensure_derivation_path();
          for (unsigned int i : DEFAULT_BIP44_PATH) {
            msg->add_address_n(i);
          }
          for (unsigned int i : m_wallet_deriv_path) {
            msg->add_address_n(i);
          }
        }

        if (network_type){
          msg->set_network_type(static_cast<uint32_t>(network_type.get()));
        } else {
          msg->set_network_type(static_cast<uint32_t>(this->network_type));
        }
      }

    public:
    device_trezor_base();
    ~device_trezor_base() override;

    device_trezor_base(const device_trezor_base &device) = delete ;
    device_trezor_base& operator=(const device_trezor_base &device) = delete;

    explicit operator bool() const override {return true;}
    device_type get_type() const override {return device_type::TREZOR;};

    bool reset();

    // Default derivation path for Monero
    static const uint32_t DEFAULT_BIP44_PATH[2];

    std::shared_ptr<Transport> get_transport(){
      return m_transport;
    }

    void set_callback(i_device_callback * callback) override {
      m_callback = callback;
    }

    i_device_callback * get_callback(){
      return m_callback;
    }

    std::shared_ptr<messages::management::Features> & get_features() {
      return m_features;
    }

    uint64_t get_version() const {
      CHECK_AND_ASSERT_THROW_MES(m_features, "Features not loaded");
      CHECK_AND_ASSERT_THROW_MES(m_features->has_major_version() && m_features->has_minor_version() && m_features->has_patch_version(), "Invalid Trezor firmware version information");
      return pack_version(m_features->major_version(), m_features->minor_version(), m_features->patch_version());
    }

    void set_derivation_path(const std::string &deriv_path) override;

    virtual bool has_ki_live_refresh(void) const override { return false; }

    virtual void set_pin(const epee::wipeable_string & pin) override {
      m_pin = pin;
    }
    virtual void set_passphrase(const epee::wipeable_string & passphrase) override {
      m_passphrase = passphrase;
    }

    /* ======================================================================= */
    /*                              SETUP/TEARDOWN                             */
    /* ======================================================================= */
    bool set_name(const std::string &name) override;

    const std::string get_name() const override;
    bool init() override;
    bool release() override;
    bool connect() override;
    bool disconnect() override;

    /* ======================================================================= */
    /*  LOCKER                                                                 */
    /* ======================================================================= */
    void lock()  override;
    void unlock() override;
    bool try_lock() override;

    /* ======================================================================= */
    /*                              TREZOR PROTOCOL                            */
    /* ======================================================================= */

    /**
     * Device ping, no-throw
     */
    bool ping();

    /**
     * Performs Initialize call to the Trezor, resets to known state.
     */
    void device_state_reset();

    // Protocol callbacks
    void on_button_request(GenericMessage & resp, const messages::common::ButtonRequest * msg);
    void on_button_pressed();
    void on_pin_request(GenericMessage & resp, const messages::common::PinMatrixRequest * msg);
    void on_passphrase_request(GenericMessage & resp, const messages::common::PassphraseRequest * msg);
    void on_passphrase_state_request(GenericMessage & resp, const messages::common::Deprecated_PassphraseStateRequest * msg);

#ifdef WITH_TREZOR_DEBUGGING
    void set_debug(bool debug){
      m_debug = debug;
    }

    void set_debug_callback(std::shared_ptr<trezor_debug_callback> & debug_callback){
      m_debug_callback = debug_callback;
    }

    void wipe_device();
    void init_device();
    void load_device(const std::string & mnemonic, const std::string & pin="", bool passphrase_protection=false,
        const std::string & label="test", const std::string & language="english",
        bool skip_checksum=false, bool expand=false);

#endif
  };

#endif

}
}
#endif //MONERO_DEVICE_TREZOR_BASE_H
