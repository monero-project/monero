// Copyright (c) 2017-2018, The Monero Project
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

namespace hw {
namespace trezor {

#if WITH_DEVICE_TREZOR

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "device.trezor"

    std::shared_ptr<google::protobuf::Message> trezor_protocol_callback::on_button_request(const messages::common::ButtonRequest * msg){
      MDEBUG("on_button_request");
      device.on_button_request();
      return std::make_shared<messages::common::ButtonAck>();
    }

    std::shared_ptr<google::protobuf::Message> trezor_protocol_callback::on_pin_matrix_request(const messages::common::PinMatrixRequest * msg){
      MDEBUG("on_pin_request");
      epee::wipeable_string pin;
      device.on_pin_request(pin);
      auto resp = std::make_shared<messages::common::PinMatrixAck>();
      resp->set_pin(pin.data(), pin.size());
      return resp;
    }

    std::shared_ptr<google::protobuf::Message> trezor_protocol_callback::on_passphrase_request(const messages::common::PassphraseRequest * msg){
      MDEBUG("on_passhprase_request");
      epee::wipeable_string passphrase;
      device.on_passphrase_request(msg->on_device(), passphrase);
      auto resp = std::make_shared<messages::common::PassphraseAck>();
      if (!msg->on_device()){
        resp->set_passphrase(passphrase.data(), passphrase.size());
      }
      return resp;
    }

    std::shared_ptr<google::protobuf::Message> trezor_protocol_callback::on_passphrase_state_request(const messages::common::PassphraseStateRequest * msg){
      MDEBUG("on_passhprase_state_request");
      device.on_passphrase_state_request(msg->state());
      return std::make_shared<messages::common::PassphraseStateAck>();
    }

    const uint32_t device_trezor_base::DEFAULT_BIP44_PATH[] = {0x8000002c, 0x80000080, 0x80000000};

    device_trezor_base::device_trezor_base() {

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
      this->full_name = name;
      this->name = "";

      auto delim = name.find(':');
      if (delim != std::string::npos && delim + 1 < name.length()) {
        this->name = name.substr(delim + 1);
      }

      return true;
    }

    const std::string device_trezor_base::get_name() const {
      if (this->full_name.empty()) {
        return std::string("<disconnected:").append(this->name).append(">");
      }
      return this->full_name;
    }

    bool device_trezor_base::init() {
      if (!release()){
        MERROR("Release failed");
        return false;
      }

      if (!m_protocol_callback){
        m_protocol_callback = std::make_shared<trezor_protocol_callback>(*this);
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
      try {
        hw::trezor::t_transport_vect trans;

        MDEBUG("Enumerating Trezor devices...");
        enumerate(trans);

        MDEBUG("Enumeration yielded " << trans.size() << " devices");
        for (auto &cur : trans) {
          MDEBUG("  device: " << *(cur.get()));
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
        return true;

      } catch(std::exception const& e){
        MERROR("Open exception: " << e.what());
        return false;
      }
    }

    bool device_trezor_base::disconnect() {
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

    void device_trezor_base::require_connected(){
      if (!m_transport){
        throw exc::NotConnectedException();
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

    /* ======================================================================= */
    /*                              TREZOR PROTOCOL                            */
    /* ======================================================================= */

    bool device_trezor_base::ping() {
      AUTO_LOCK_CMD();
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

    void device_trezor_base::on_button_request()
    {
      if (m_callback){
        m_callback->on_button_request();
      }
    }

    void device_trezor_base::on_pin_request(epee::wipeable_string & pin)
    {
      if (m_callback){
        m_callback->on_pin_request(pin);
      }
    }

    void device_trezor_base::on_passphrase_request(bool on_device, epee::wipeable_string & passphrase)
    {
      if (m_callback){
        m_callback->on_passphrase_request(on_device, passphrase);
      }
    }

    void device_trezor_base::on_passphrase_state_request(const std::string & state)
    {
      if (m_callback){
        m_callback->on_passphrase_state_request(state);
      }
    }

#endif //WITH_DEVICE_TREZOR
}}
