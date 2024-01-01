// Copyright (c) 2017-2024, The Monero Project
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

#include "device_trezor_base.hpp"
#include "memwipe.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/regex.hpp>

namespace hw {
namespace trezor {

#ifdef WITH_DEVICE_TREZOR

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "device.trezor"
#define TREZOR_BIP44_HARDENED_ZERO 0x80000000

    const uint32_t device_trezor_base::DEFAULT_BIP44_PATH[] = {0x8000002c, 0x80000080};

    device_trezor_base::device_trezor_base(): m_callback(nullptr), m_last_msg_type(messages::MessageType_Success),
                                              m_reply_with_empty_passphrase(false),
                                              m_always_use_empty_passphrase(false),
                                              m_seen_passphrase_entry_message(false) {
#ifdef WITH_TREZOR_DEBUGGING
      m_debug = false;
#endif
    }

    device_trezor_base::~device_trezor_base() {
      try {
        disconnect();
        release();
      } catch(std::exception const& e){
        MERROR("Could not disconnect and release: " << e.what());
      }
    }

    /* ======================================================================= */
    /*                              SETUP/TEARDOWN                             */
    /* ======================================================================= */

    bool device_trezor_base::reset() {
      return false;
    }

    bool device_trezor_base::set_name(const std::string & name) {
      this->m_full_name = name;
      this->name = "";

      auto delim = name.find(':');
      if (delim != std::string::npos && delim + 1 < name.length()) {
        this->name = name.substr(delim + 1);
      }

      return true;
    }

    const std::string device_trezor_base::get_name() const {
      if (this->m_full_name.empty()) {
        return std::string("<disconnected:").append(this->name).append(">");
      }
      return this->m_full_name;
    }

    bool device_trezor_base::init() {
      if (!release()){
        MERROR("Release failed");
        return false;
      }

      return true;
    }

    bool device_trezor_base::release() {
      try {
        disconnect();
        return true;

      } catch(std::exception const& e){
        MERROR("Release exception: " << e.what());
        return false;
      }
    }

    bool device_trezor_base::connect() {
      disconnect();

      // Enumerate all available devices
      TREZOR_AUTO_LOCK_DEVICE();
      try {
        hw::trezor::t_transport_vect trans;

        MDEBUG("Enumerating Trezor devices...");
        enumerate(trans);
        sort_transports_by_env(trans);

        MDEBUG("Enumeration yielded " << trans.size() << " Trezor devices");
        for (auto &cur : trans) {
          MDEBUG("  device: " << *(cur.get()));
        }

        for (auto &cur : trans) {
          std::string cur_path = cur->get_path();
          if (boost::starts_with(cur_path, this->name)) {
            MDEBUG("Device Match: " << cur_path);
            m_transport = cur;
            break;
          }
        }

        if (!m_transport) {
          MERROR("No matching Trezor device found. Device specifier: \"" + this->name + "\"");
          return false;
        }

        m_transport->open();

#ifdef WITH_TREZOR_DEBUGGING
        setup_debug();
#endif
        return true;

      } catch(std::exception const& e){
        MERROR("Open exception: " << e.what());
        return false;
      }
    }

    bool device_trezor_base::disconnect() {
      TREZOR_AUTO_LOCK_DEVICE();
      m_device_session_id.clear();
      m_features.reset();
      m_seen_passphrase_entry_message = false;
      m_reply_with_empty_passphrase = false;
      m_always_use_empty_passphrase = false;

      if (m_transport){
        try {
          m_transport->close();
          m_transport = nullptr;

        } catch(std::exception const& e){
          MERROR("Disconnect exception: " << e.what());
          m_transport = nullptr;
          return false;
        }
      }

#ifdef WITH_TREZOR_DEBUGGING
      if (m_debug_callback) {
        m_debug_callback->on_disconnect();
        m_debug_callback = nullptr;
      }
#endif
      return true;
    }

    /* ======================================================================= */
    /*  LOCKER                                                                 */
    /* ======================================================================= */

    //lock the device for a long sequence
    void device_trezor_base::lock() {
      MTRACE("Ask for LOCKING for device " << this->name << " in thread ");
      device_locker.lock();
      MTRACE("Device " << this->name << " LOCKed");
    }

    //lock the device for a long sequence
    bool device_trezor_base::try_lock() {
      MTRACE("Ask for LOCKING(try) for device " << this->name << " in thread ");
      bool r = device_locker.try_lock();
      if (r) {
        MTRACE("Device " << this->name << " LOCKed(try)");
      } else {
        MDEBUG("Device " << this->name << " not LOCKed(try)");
      }
      return r;
    }

    //unlock the device
    void device_trezor_base::unlock() {
      MTRACE("Ask for UNLOCKING for device " << this->name << " in thread ");
      device_locker.unlock();
      MTRACE("Device " << this->name << " UNLOCKed");
    }

    /* ======================================================================= */
    /*  Helpers                                                                */
    /* ======================================================================= */

    void device_trezor_base::require_connected() const {
      if (!m_transport){
        throw exc::NotConnectedException();
      }
    }

    void device_trezor_base::require_initialized() const {
      if (!m_features){
        throw exc::TrezorException("Device state not initialized");
      }

      if (m_features->has_bootloader_mode() && m_features->bootloader_mode()){
        throw exc::TrezorException("Device is in the bootloader mode");
      }

      if (m_features->has_firmware_present() && !m_features->firmware_present()){
        throw exc::TrezorException("Device has no firmware loaded");
      }

      // Hard requirement on initialized field, has to be there.
      if (!m_features->has_initialized() || !m_features->initialized()){
        throw exc::TrezorException("Device is not initialized");
      }
    }

    void device_trezor_base::call_ping_unsafe(){
      auto pingMsg = std::make_shared<messages::management::Ping>();
      pingMsg->set_message("PING");

      auto success = this->client_exchange<messages::common::Success>(pingMsg);  // messages::MessageType_Success
      MDEBUG("Ping response " << success->message());
      (void)success;
    }

    void device_trezor_base::test_ping(){
      require_connected();

      try {
        this->call_ping_unsafe();

      } catch(exc::TrezorException const& e){
        MINFO("Trezor does not respond: " << e.what());
        throw exc::DeviceNotResponsiveException(std::string("Trezor not responding: ") + e.what());
      }
    }

    void device_trezor_base::write_raw(const google::protobuf::Message * msg){
      require_connected();
      CHECK_AND_ASSERT_THROW_MES(msg, "Empty message");
      this->get_transport()->write(*msg);
    }

    GenericMessage device_trezor_base::read_raw(){
      require_connected();
      std::shared_ptr<google::protobuf::Message> msg_resp;
      hw::trezor::messages::MessageType msg_resp_type;

      this->get_transport()->read(msg_resp, &msg_resp_type);
      return GenericMessage(msg_resp_type, msg_resp);
    }

    GenericMessage device_trezor_base::call_raw(const google::protobuf::Message * msg) {
      write_raw(msg);
      return read_raw();
    }

    bool device_trezor_base::message_handler(GenericMessage & input){
      // Later if needed this generic message handler can be replaced by a pointer to
      // a protocol message handler which by default points to the device class which implements
      // the default handler.

      if (m_last_msg_type == messages::MessageType_ButtonRequest){
        on_button_pressed();
      }
      m_last_msg_type = input.m_type;

      switch(input.m_type){
        case messages::MessageType_ButtonRequest:
          on_button_request(input, dynamic_cast<const messages::common::ButtonRequest*>(input.m_msg.get()));
          return true;
        case messages::MessageType_PassphraseRequest:
          on_passphrase_request(input, dynamic_cast<const messages::common::PassphraseRequest*>(input.m_msg.get()));
          return true;
        case messages::MessageType_PinMatrixRequest:
          on_pin_request(input, dynamic_cast<const messages::common::PinMatrixRequest*>(input.m_msg.get()));
          return true;
        default:
          return false;
      }
    }

    void device_trezor_base::ensure_derivation_path() noexcept {
      if (m_wallet_deriv_path.empty()){
        m_wallet_deriv_path.push_back(TREZOR_BIP44_HARDENED_ZERO);  // default 0'
      }
    }

    void device_trezor_base::set_derivation_path(const std::string &deriv_path){
      this->m_wallet_deriv_path.clear();

      if (deriv_path.empty() || deriv_path == "-"){
        ensure_derivation_path();
        return;
      }

      CHECK_AND_ASSERT_THROW_MES(deriv_path.size() <= 255, "Derivation path is too long");

      std::vector<std::string> fields;
      boost::split(fields, deriv_path, boost::is_any_of("/"));
      CHECK_AND_ASSERT_THROW_MES(fields.size() <= 10, "Derivation path is too long");

      boost::regex rgx("^([0-9]+)'?$");
      boost::cmatch match;

      this->m_wallet_deriv_path.reserve(fields.size());
      for(const std::string & cur : fields){
        const bool ok = boost::regex_match(cur.c_str(), match, rgx);
        CHECK_AND_ASSERT_THROW_MES(ok, "Invalid wallet code: " << deriv_path << ". Invalid path element: " << cur);
        CHECK_AND_ASSERT_THROW_MES(match[0].length() > 0, "Invalid wallet code: " << deriv_path << ". Invalid path element: " << cur);

        const unsigned long cidx = std::stoul(match[0].str()) | TREZOR_BIP44_HARDENED_ZERO;
        this->m_wallet_deriv_path.push_back((unsigned int)cidx);
      }
    }

    /* ======================================================================= */
    /*                              TREZOR PROTOCOL                            */
    /* ======================================================================= */

    bool device_trezor_base::ping() {
      TREZOR_AUTO_LOCK_CMD();
      if (!m_transport){
        MINFO("Ping failed, device not connected");
        return false;
      }

      try {
        this->call_ping_unsafe();
        return true;

      } catch(std::exception const& e) {
        MERROR("Ping failed, exception thrown " << e.what());
      } catch(...){
        MERROR("Ping failed, general exception thrown" << boost::current_exception_diagnostic_information());
      }

      return false;
    }

    void device_trezor_base::device_state_initialize_unsafe()
    {
      require_connected();
      auto initMsg = std::make_shared<messages::management::Initialize>();
      const auto data_cleaner = epee::misc_utils::create_scope_leave_handler([&]() {
        if (initMsg->has_session_id())
          memwipe(&(*initMsg->mutable_session_id())[0], initMsg->mutable_session_id()->size());
      });

      if(!m_device_session_id.empty()) {
        initMsg->set_allocated_session_id(new std::string(m_device_session_id.data(), m_device_session_id.size()));
      }

      m_features = this->client_exchange<messages::management::Features>(initMsg);
      if (m_features->has_session_id()){
        m_device_session_id = m_features->session_id();
      } else {
        m_device_session_id.clear();
      }
    }

    void device_trezor_base::device_state_reset()
    {
      TREZOR_AUTO_LOCK_CMD();
      device_state_initialize_unsafe();
    }

#ifdef WITH_TREZOR_DEBUGGING
#define TREZOR_CALLBACK(method, ...) do { \
  if (m_debug_callback) m_debug_callback->method(__VA_ARGS__); \
  if (m_callback) m_callback->method(__VA_ARGS__);             \
}while(0)
#define TREZOR_CALLBACK_GET(VAR, method, ...) do { \
  if (m_debug_callback) VAR = m_debug_callback->method(__VA_ARGS__); \
  if (m_callback) VAR = m_callback->method(__VA_ARGS__);             \
}while(0)

    void device_trezor_base::setup_debug(){
      if (!m_debug){
        return;
      }

      if (!m_debug_callback){
        CHECK_AND_ASSERT_THROW_MES(m_transport, "Transport does not exist");
        auto debug_transport = m_transport->find_debug();
        if (debug_transport) {
          m_debug_callback = std::make_shared<trezor_debug_callback>(debug_transport);
        } else {
          MDEBUG("Transport does not have debug link option");
        }
      }
    }

#else
#define TREZOR_CALLBACK(method, ...) do { if (m_callback) m_callback->method(__VA_ARGS__); } while(0)
#define TREZOR_CALLBACK_GET(VAR, method, ...) VAR = (m_callback ? m_callback->method(__VA_ARGS__) : boost::none)
#endif

    void device_trezor_base::on_button_request(GenericMessage & resp, const messages::common::ButtonRequest * msg)
    {
      CHECK_AND_ASSERT_THROW_MES(msg, "Empty message");
      MDEBUG("on_button_request, code: " << msg->code());

      TREZOR_CALLBACK(on_button_request, msg->code());

      messages::common::ButtonAck ack;
      write_raw(&ack);

      resp = read_raw();
    }

    void device_trezor_base::on_button_pressed()
    {
      TREZOR_CALLBACK(on_button_pressed);
    }

    void device_trezor_base::on_pin_request(GenericMessage & resp, const messages::common::PinMatrixRequest * msg)
    {
      MDEBUG("on_pin_request");
      CHECK_AND_ASSERT_THROW_MES(msg, "Empty message");

      boost::optional<epee::wipeable_string> pin;
      TREZOR_CALLBACK_GET(pin, on_pin_request);

      if (!pin && m_pin){
        pin = m_pin;
      }

      messages::common::PinMatrixAck m;
      if (pin) {
        m.set_allocated_pin(new std::string(pin->data(), pin->size()));
      }

      const auto data_cleaner = epee::misc_utils::create_scope_leave_handler([&]() {
        if (m.has_pin())
          memwipe(&(*m.mutable_pin())[0], m.mutable_pin()->size());
      });

      resp = call_raw(&m);
    }

    void device_trezor_base::on_passphrase_request(GenericMessage & resp, const messages::common::PassphraseRequest * msg)
    {
      CHECK_AND_ASSERT_THROW_MES(msg, "Empty message");
      MDEBUG("on_passhprase_request");

      m_seen_passphrase_entry_message = true;
      bool on_device = false;
      if (m_features){
        for (auto it = m_features->capabilities().begin(); it != m_features->capabilities().end(); it++) {
          if (*it == messages::management::Features::Capability_PassphraseEntry){
            on_device = true;
          }
        }
      }

      boost::optional<epee::wipeable_string> passphrase;
      if (m_reply_with_empty_passphrase || m_always_use_empty_passphrase) {
        MDEBUG("Answering passphrase prompt with an empty passphrase, always use empty: " << m_always_use_empty_passphrase);
        on_device = false;
        passphrase = epee::wipeable_string("");
      } else if (m_passphrase){
        MWARNING("Answering passphrase prompt with a stored passphrase (do not use; passphrase can be seen by a potential malware / attacker)");
        on_device = false;
        passphrase = epee::wipeable_string(m_passphrase.get());
      } else {
        TREZOR_CALLBACK_GET(passphrase, on_passphrase_request, on_device);
      }

      messages::common::PassphraseAck m;
      m.set_on_device(on_device);
      if (!on_device) {
        if (passphrase) {
          m.set_allocated_passphrase(new std::string(passphrase->data(), passphrase->size()));
        }
      }

      const auto data_cleaner = epee::misc_utils::create_scope_leave_handler([&]() {
        if (m.has_passphrase())
          memwipe(&(*m.mutable_passphrase())[0], m.mutable_passphrase()->size());
      });

      resp = call_raw(&m);
    }

#ifdef WITH_TREZOR_DEBUGGING
    void device_trezor_base::wipe_device()
    {
      auto msg = std::make_shared<messages::management::WipeDevice>();
      auto ret = client_exchange<messages::common::Success>(msg);
      (void)ret;
      init_device();
    }

    void device_trezor_base::init_device()
    {
      auto msg = std::make_shared<messages::management::Initialize>();
      m_features = client_exchange<messages::management::Features>(msg);
    }

    void device_trezor_base::load_device(const std::string & mnemonic, const std::string & pin,
        bool passphrase_protection, const std::string & label, const std::string & language,
        bool skip_checksum, bool expand)
    {
      if (m_features && m_features->initialized()){
        throw std::runtime_error("Device is initialized already. Call device.wipe() and try again.");
      }

      auto msg = std::make_shared<messages::management::LoadDevice>();
      msg->add_mnemonics(mnemonic);
      msg->set_pin(pin);
      msg->set_passphrase_protection(passphrase_protection);
      msg->set_label(label);
      msg->set_language(language);
      msg->set_skip_checksum(skip_checksum);
      auto ret = client_exchange<messages::common::Success>(msg);
      (void)ret;

      init_device();
    }

    trezor_debug_callback::trezor_debug_callback(std::shared_ptr<Transport> & debug_transport){
      m_debug_link = std::make_shared<DebugLink>();
      m_debug_link->init(debug_transport);
    }

    void trezor_debug_callback::on_button_request(uint64_t code) {
      if (m_debug_link) m_debug_link->press_yes();
    }

    boost::optional<epee::wipeable_string> trezor_debug_callback::on_pin_request() {
      return boost::none;
    }

    boost::optional<epee::wipeable_string> trezor_debug_callback::on_passphrase_request(bool & on_device) {
      on_device = true;
      return boost::none;
    }

    void trezor_debug_callback::on_passphrase_state_request(const std::string &state) {

    }

    void trezor_debug_callback::on_disconnect(){
      if (m_debug_link) m_debug_link->close();
    }
#endif

#endif //WITH_DEVICE_TREZOR
}}
