// Copyright (c) 2017-2019, The Monero Project
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

#include "version.h"
#include "device_ledger.hpp"
#include "ringct/rctOps.h"
#include "cryptonote_basic/account.h"
#include "cryptonote_basic/subaddress_index.h"
#include "cryptonote_core/cryptonote_tx_utils.h"

#include <boost/thread/locks.hpp> 
#include <boost/thread/lock_guard.hpp>

namespace hw {

  namespace ledger {

  #ifdef WITH_DEVICE_LEDGER

    #undef MONERO_DEFAULT_LOG_CATEGORY
    #define MONERO_DEFAULT_LOG_CATEGORY "device.ledger"

    /* ===================================================================== */
    /* ===                           Debug                              ==== */
    /* ===================================================================== */

    void set_apdu_verbose(bool verbose) {
      apdu_verbose = verbose;
    }

    #define TRACKD MTRACE("hw")
    #define ASSERT_SW(sw,ok,msk) CHECK_AND_ASSERT_THROW_MES(((sw)&(mask))==(ok), "Wrong Device Status : SW=" << std::hex << (sw) << " (EXPECT=" << std::hex << (ok) << ", MASK=" << std::hex << (mask) << ")") ;
    #define ASSERT_T0(exp)       CHECK_AND_ASSERT_THROW_MES(exp, "Protocol assert failure: "#exp ) ;
    #define ASSERT_X(exp,msg)    CHECK_AND_ASSERT_THROW_MES(exp, msg); 

    #ifdef DEBUG_HWDEVICE
      crypto::secret_key dbg_viewkey;
      crypto::secret_key dbg_spendkey;
    #endif

    /* ===================================================================== */
    /* ===                        hmacmap                               ==== */
    /* ===================================================================== */


    SecHMAC::SecHMAC(const uint8_t s[32], const uint8_t h[32]) {
        memcpy(this->sec, s, 32);
        memcpy(this->hmac, h, 32);
    }

    void  HMACmap::find_mac(const uint8_t sec[32], uint8_t hmac[32]) {
      size_t sz = hmacs.size();
      log_hexbuffer("find_mac: lookup for ", (char*)sec,32);
      for (size_t i=0; i<sz; i++) {
       log_hexbuffer("find_mac:   - try ",(char*)hmacs[i].sec,32);
        if (memcmp(sec, hmacs[i].sec, 32) == 0) {
          memcpy(hmac, hmacs[i].hmac, 32);
          log_hexbuffer("find_mac:   - found ",(char*)hmacs[i].hmac,32);
          return;
        }

      }
      throw std::runtime_error("Protocol error: try to send untrusted secret");
    }

    void  HMACmap::add_mac(const uint8_t sec[32], const uint8_t hmac[32]) {
      log_hexbuffer("add_mac: sec  ", (char*)sec,32);
      log_hexbuffer("add_mac: hmac ", (char*)hmac,32);
      hmacs.push_back(SecHMAC(sec,hmac));
    }

    void HMACmap::clear() {
      hmacs.clear();
    }

    /* ===================================================================== */
    /* ===                        Keymap                                ==== */
    /* ===================================================================== */

    ABPkeys::ABPkeys(const rct::key& A, const rct::key& B, const bool is_subaddr,  const bool is_change, const bool need_additional_txkeys,  const size_t real_output_index, const rct::key& P, const rct::key& AK) {
      Aout = A;
      Bout = B;
      is_subaddress = is_subaddr;
      is_change_address = is_change;
      additional_key = need_additional_txkeys;
      index = real_output_index;
      Pout = P;
      AKout = AK;
    }

    ABPkeys::ABPkeys(const ABPkeys& keys) {
      Aout = keys.Aout;
      Bout = keys.Bout;
      is_subaddress = keys.is_subaddress;
      is_change_address = keys.is_change_address;
      additional_key = keys.additional_key;
      index = keys.index;
      Pout = keys.Pout;
      AKout = keys.AKout;
    }

    ABPkeys &ABPkeys::operator=(const ABPkeys& keys) {
      if (&keys == this)
        return *this;
      Aout = keys.Aout;
      Bout = keys.Bout;
      is_subaddress = keys.is_subaddress;
      is_change_address = keys.is_change_address;
      additional_key = keys.additional_key;
      index = keys.index;
      Pout = keys.Pout;
      AKout = keys.AKout;
      return *this;
    }

    bool Keymap::find(const rct::key& P, ABPkeys& keys) const {
      size_t sz = ABP.size();
      for (size_t i=0; i<sz; i++) {
        if (ABP[i].Pout == P) {
          keys = ABP[i];
          return true;
        }
      }
      return false;
    }

    void Keymap::add(const ABPkeys& keys) {
      ABP.push_back(keys);
    }

    void Keymap::clear() {
      ABP.clear();
    }

    #ifdef DEBUG_HWDEVICE
    void Keymap::log() {
      log_message("keymap", "content");
      size_t sz = ABP.size();
      for (size_t i=0; i<sz; i++) {
        log_message("  keymap", std::to_string(i));
        log_hexbuffer("    Aout", (char*)ABP[i].Aout.bytes, 32);
        log_hexbuffer("    Bout", (char*)ABP[i].Bout.bytes, 32);
        log_message  ("  is_sub", std::to_string(ABP[i].is_subaddress));
        log_message  ("   index", std::to_string(ABP[i].index));
        log_hexbuffer("    Pout", (char*)ABP[i].Pout.bytes, 32);
      }
    }
    #endif

    /* ===================================================================== */
    /* ===                       Internal Helpers                       ==== */
    /* ===================================================================== */
    static bool is_fake_view_key(const crypto::secret_key &sec) {
      return sec == crypto::null_skey;
    }

    bool operator==(const crypto::key_derivation &d0, const crypto::key_derivation &d1) {
      static_assert(sizeof(crypto::key_derivation) == 32, "key_derivation must be 32 bytes");
      return !crypto_verify_32((const unsigned char*)&d0, (const unsigned char*)&d1);
    }

    /* ===================================================================== */
    /* ===                             Device                           ==== */
    /* ===================================================================== */

    static int device_id = 0;

    #define PROTOCOL_VERSION                    2

    #define INS_NONE                            0x00
    #define INS_RESET                           0x02

    #define INS_GET_KEY                         0x20
    #define INS_DISPLAY_ADDRESS                 0x21
    #define INS_PUT_KEY                         0x22
    #define INS_GET_CHACHA8_PREKEY              0x24
    #define INS_VERIFY_KEY                      0x26
    #define INS_MANAGE_SEEDWORDS                0x28

    #define INS_SECRET_KEY_TO_PUBLIC_KEY        0x30
    #define INS_GEN_KEY_DERIVATION              0x32
    #define INS_DERIVATION_TO_SCALAR            0x34
    #define INS_DERIVE_PUBLIC_KEY               0x36
    #define INS_DERIVE_SECRET_KEY               0x38
    #define INS_GEN_KEY_IMAGE                   0x3A
    #define INS_SECRET_KEY_ADD                  0x3C
    #define INS_SECRET_KEY_SUB                  0x3E
    #define INS_GENERATE_KEYPAIR                0x40
    #define INS_SECRET_SCAL_MUL_KEY             0x42
    #define INS_SECRET_SCAL_MUL_BASE            0x44

    #define INS_DERIVE_SUBADDRESS_PUBLIC_KEY    0x46
    #define INS_GET_SUBADDRESS                  0x48
    #define INS_GET_SUBADDRESS_SPEND_PUBLIC_KEY 0x4A
    #define INS_GET_SUBADDRESS_SECRET_KEY       0x4C

    #define INS_OPEN_TX                         0x70
    #define INS_SET_SIGNATURE_MODE              0x72
    #define INS_GET_ADDITIONAL_KEY              0x74
    #define INS_STEALTH                         0x76
    #define INS_GEN_COMMITMENT_MASK             0x77
    #define INS_BLIND                           0x78
    #define INS_UNBLIND                         0x7A
    #define INS_GEN_TXOUT_KEYS                  0x7B
    #define INS_VALIDATE                        0x7C
    #define INS_MLSAG                           0x7E
    #define INS_CLOSE_TX                        0x80

    #define INS_GET_TX_PROOF                    0xA0

    #define INS_GET_RESPONSE                    0xc0


    device_ledger::device_ledger(): hw_device(0x0101, 0x05, 64, 2000) {
      this->id = device_id++;
      this->reset_buffer();      
      this->mode = NONE;
      this->has_view_key = false;
      this->tx_in_progress = false;
      MDEBUG( "Device "<<this->id <<" Created");
    }

    device_ledger::~device_ledger() {
      this->release();
      MDEBUG( "Device "<<this->id <<" Destroyed");
    }

    /* ======================================================================= */
    /*  LOCKER                                                                 */
    /* ======================================================================= */ 
    
    //automatic lock one more level on device ensuring the current thread is allowed to use it
    #define AUTO_LOCK_CMD() \
      /* lock both mutexes without deadlock*/ \
      boost::lock(device_locker, command_locker); \
      /* make sure both already-locked mutexes are unlocked at the end of scope */ \
      boost::lock_guard<boost::recursive_mutex> lock1(device_locker, boost::adopt_lock); \
      boost::lock_guard<boost::mutex> lock2(command_locker, boost::adopt_lock)

    //lock the device for a long sequence
    void device_ledger::lock(void) {
      MDEBUG( "Ask for LOCKING for device "<<this->name << " in thread ");
      device_locker.lock();
      MDEBUG( "Device "<<this->name << " LOCKed");
    }

    //lock the device for a long sequence
    bool device_ledger::try_lock(void) {
      MDEBUG( "Ask for LOCKING(try) for device "<<this->name << " in thread ");
      bool r = device_locker.try_lock();
      if (r) {
        MDEBUG( "Device "<<this->name << " LOCKed(try)");
      } else {
        MDEBUG( "Device "<<this->name << " not LOCKed(try)");
      }
      return r;
    }

    //lock the device for a long sequence
    void device_ledger::unlock(void) {
      try {
        MDEBUG( "Ask for UNLOCKING for device "<<this->name << " in thread ");
      } catch (...) {
      }
      device_locker.unlock();
      MDEBUG( "Device "<<this->name << " UNLOCKed");
    }

  
    /* ======================================================================= */
    /*                                     IO                                  */
    /* ======================================================================= */

    #define IO_SW_DENY    0x6982
    #define IO_SECRET_KEY 0x02

      void device_ledger::logCMD() {
      if (apdu_verbose) {
        char  strbuffer[1024];
        snprintf(strbuffer, sizeof(strbuffer), "%.02x %.02x %.02x %.02x %.02x ",
          this->buffer_send[0],
          this->buffer_send[1],
          this->buffer_send[2],
          this->buffer_send[3],
          this->buffer_send[4]
          );
        const size_t len = strlen(strbuffer);
        buffer_to_str(strbuffer+len, sizeof(strbuffer)-len, (char*)(this->buffer_send+5), this->length_send-5);
        MDEBUG( "CMD  : " << strbuffer);
      }
    }

    void device_ledger::logRESP() {
      if (apdu_verbose) {
        char  strbuffer[1024];
        snprintf(strbuffer, sizeof(strbuffer), "%.04x ", this->sw);
        const size_t len = strlen(strbuffer);
        buffer_to_str(strbuffer+len, sizeof(strbuffer)-len, (char*)(this->buffer_recv), this->length_recv);
        MDEBUG( "RESP : " << strbuffer);

      }
    }

    int device_ledger::set_command_header(unsigned char ins, unsigned char p1, unsigned char p2) {
      reset_buffer();
      this->buffer_send[0] = PROTOCOL_VERSION;
      this->buffer_send[1] = ins;
      this->buffer_send[2] = p1;
      this->buffer_send[3] = p2;
      this->buffer_send[4] = 0x00;
      return 5;
    }

    int device_ledger::set_command_header_noopt(unsigned char ins, unsigned char p1, unsigned char p2) {
      int offset = set_command_header(ins, p1, p2);
      //options
      this->buffer_send[offset++] = 0;
      this->buffer_send[4] = offset - 5;
      return offset;
    }

    void device_ledger::send_simple(unsigned char ins, unsigned char p1) {
      this->length_send = set_command_header_noopt(ins, p1);
      if (ins == INS_GET_KEY && p1 == IO_SECRET_KEY) {
        // export view key user input
        this->exchange_wait_on_input();
      } else {
        this->exchange();
      }
    }

    void device_ledger::send_secret(const unsigned char sec[32], int &offset) {
      MDEBUG("send_secret: " << this->tx_in_progress);
      ASSERT_X(offset + 32 <= BUFFER_SEND_SIZE, "send_secret: out of bounds write (secret)");
      memmove(this->buffer_send+offset, sec, 32);
      offset +=32;
      if (this->tx_in_progress) {
        ASSERT_X(offset + 32 <= BUFFER_SEND_SIZE, "send_secret: out of bounds write (mac)");
        this->hmac_map.find_mac((uint8_t*)sec, this->buffer_send+offset);
        offset += 32;
      }
    }

    void device_ledger::receive_secret(unsigned char sec[32], int &offset) {
      MDEBUG("receive_secret: " << this->tx_in_progress);
      ASSERT_X(offset + 32 <= BUFFER_RECV_SIZE, "receive_secret: out of bounds read (secret)");
      memmove(sec, this->buffer_recv+offset, 32);
      offset += 32;
      if (this->tx_in_progress) {
        ASSERT_X(offset + 32 <= BUFFER_RECV_SIZE, "receive_secret: out of bounds read (mac)");
        this->hmac_map.add_mac((uint8_t*)sec, this->buffer_recv+offset);
        offset += 32;
      }
    }

    bool device_ledger::reset() {
      reset_buffer();
      int offset = set_command_header_noopt(INS_RESET);
      const size_t verlen = strlen(MONERO_VERSION);
      ASSERT_X(offset + verlen <= BUFFER_SEND_SIZE, "MONERO_VERSION is too long")
      memmove(this->buffer_send+offset, MONERO_VERSION, verlen);
      offset += strlen(MONERO_VERSION);
      this->buffer_send[4] = offset-5;
      this->length_send = offset;
      this->exchange();

      ASSERT_X(this->length_recv>=3, "Communication error, less than three bytes received. Check your application version.");

      unsigned int device_version = 0;
      device_version = VERSION(this->buffer_recv[0], this->buffer_recv[1], this->buffer_recv[2]);
  
      ASSERT_X (device_version >= MINIMAL_APP_VERSION,  
                "Unsupported device application version: " << VERSION_MAJOR(device_version)<<"."<<VERSION_MINOR(device_version)<<"."<<VERSION_MICRO(device_version) << 
                " At least " << MINIMAL_APP_VERSION_MAJOR<<"."<<MINIMAL_APP_VERSION_MINOR<<"."<<MINIMAL_APP_VERSION_MICRO<<" is required.");
     
      return true;
    }
     
    unsigned int device_ledger::exchange(unsigned int ok, unsigned int mask) {
      logCMD();

      this->length_recv =  hw_device.exchange(this->buffer_send, this->length_send, this->buffer_recv, BUFFER_SEND_SIZE, false);
      ASSERT_X(this->length_recv>=2, "Communication error, less than tow bytes received");

      this->length_recv -= 2;
      this->sw = (this->buffer_recv[length_recv]<<8) | this->buffer_recv[length_recv+1];
      logRESP();
      ASSERT_SW(this->sw,ok,msk);

      return this->sw;
    }

    unsigned int device_ledger::exchange_wait_on_input(unsigned int ok, unsigned int mask) {
      logCMD();
      unsigned int deny = 0;
      this->length_recv =  hw_device.exchange(this->buffer_send, this->length_send, this->buffer_recv, BUFFER_SEND_SIZE, true);
      ASSERT_X(this->length_recv>=2, "Communication error, less than two bytes received");

      this->length_recv -= 2;
      this->sw = (this->buffer_recv[length_recv]<<8) | this->buffer_recv[length_recv+1];
      if (this->sw == IO_SW_DENY) {
        // cancel on device
        deny = 1;
      } else {
        ASSERT_SW(this->sw,ok,msk);
      }

      logRESP();
      return deny;
    }

    void device_ledger::reset_buffer() {
      this->length_send = 0;
      memset(this->buffer_send, 0, BUFFER_SEND_SIZE);
      this->length_recv = 0;
      memset(this->buffer_recv, 0, BUFFER_RECV_SIZE);
    }

    /* ======================================================================= */
    /*                              SETUP/TEARDOWN                             */
    /* ======================================================================= */

    bool device_ledger::set_name(const std::string & name) {
      this->name = name;
      return true;
    }

    const std::string device_ledger::get_name() const {
      if (!this->connected()) {
        return std::string("<disconnected:").append(this->name).append(">");
      }
      return this->name;
    }

    bool device_ledger::init(void) {
      #ifdef DEBUG_HWDEVICE
      this->controle_device = &hw::get_device("default");
      #endif
      this->release();
      hw_device.init();      
      MDEBUG( "Device "<<this->id <<" HIDUSB inited");
      return true;
    }
    
    static const std::vector<hw::io::hid_conn_params> known_devices {
        {0x2c97, 0x0001, 0, 0xffa0}, 
        {0x2c97, 0x0004, 0, 0xffa0},       
    };

    bool device_ledger::connect(void) {
      this->disconnect();
      hw_device.connect(known_devices);
      this->reset();
      #ifdef DEBUG_HWDEVICE
      cryptonote::account_public_address pubkey;
      this->get_public_address(pubkey);
      #endif
      crypto::secret_key vkey;
      crypto::secret_key skey;
      this->get_secret_keys(vkey,skey);

      return true;
    }

    bool device_ledger::connected(void) const {
      return hw_device.connected();
    }

    bool device_ledger::disconnect() {
      hw_device.disconnect();
      return true;
    }

    bool device_ledger::release() {
      this->disconnect();
      hw_device.release();
      return true;
    }

    bool  device_ledger::set_mode(device_mode mode) {
        AUTO_LOCK_CMD();

        int offset;

        switch(mode) {
        case TRANSACTION_CREATE_REAL:
        case TRANSACTION_CREATE_FAKE:
          offset = set_command_header_noopt(INS_SET_SIGNATURE_MODE, 1);
          //account
          this->buffer_send[offset] = mode;
          offset += 1;

          this->buffer_send[4] = offset-5;
          this->length_send = offset;
          this->exchange();

          this->mode = mode;
          break;

        case TRANSACTION_PARSE: 
        case NONE:
          this->mode = mode;
          break;
        default:
           CHECK_AND_ASSERT_THROW_MES(false, " device_ledger::set_mode(unsigned int mode): invalid mode: "<<mode);
        }
        MDEBUG("Switch to mode: " <<mode);
        return device::set_mode(mode);
    }


    /* ======================================================================= */
    /*                             WALLET & ADDRESS                            */
    /* ======================================================================= */

     bool device_ledger::get_public_address(cryptonote::account_public_address &pubkey){
        AUTO_LOCK_CMD();

        send_simple(INS_GET_KEY, 1);

        memmove(pubkey.m_view_public_key.data, this->buffer_recv, 32);
        memmove(pubkey.m_spend_public_key.data, this->buffer_recv+32, 32);
 
        return true;
    }

    bool  device_ledger::get_secret_keys(crypto::secret_key &vkey , crypto::secret_key &skey) {
        AUTO_LOCK_CMD();

        //secret key are represented as fake key on the wallet side
        memset(vkey.data, 0x00, 32);
        memset(skey.data, 0xFF, 32);

        //spcialkey, normal conf handled in decrypt
        send_simple(INS_GET_KEY, 0x02);

        //View key is retrievied, if allowed, to speed up blockchain parsing
        memmove(this->viewkey.data,  this->buffer_recv+0,  32);
        if (is_fake_view_key(this->viewkey)) {
          MDEBUG("Have Not view key");
          this->has_view_key = false;
        } else {
          MDEBUG("Have view key");
          this->has_view_key = true;
        }
      
        #ifdef DEBUG_HWDEVICE
        send_simple(INS_GET_KEY, 0x04);
        memmove(dbg_viewkey.data, this->buffer_recv+0, 32);
        memmove(dbg_spendkey.data, this->buffer_recv+32, 32);
        #endif

        return true;
    }

    bool  device_ledger::generate_chacha_key(const cryptonote::account_keys &keys, crypto::chacha_key &key, uint64_t kdf_rounds) {
        AUTO_LOCK_CMD();

        #ifdef DEBUG_HWDEVICE
        crypto::chacha_key key_x;
        cryptonote::account_keys keys_x = hw::ledger::decrypt(keys); 
        this->controle_device->generate_chacha_key(keys_x, key_x, kdf_rounds);
        #endif

        send_simple(INS_GET_CHACHA8_PREKEY);

        char prekey[200];
        memmove(prekey, &this->buffer_recv[0], 200);
        crypto::generate_chacha_key_prehashed(&prekey[0], sizeof(prekey), key, kdf_rounds);

        #ifdef DEBUG_HWDEVICE
        hw::ledger::check32("generate_chacha_key_prehashed", "key", (char*)key_x.data(), (char*)key.data());
        #endif

      return true;
    }

    void  device_ledger::display_address(const cryptonote::subaddress_index& index, const boost::optional<crypto::hash8> &payment_id) {
        AUTO_LOCK_CMD();

        int offset = set_command_header_noopt(INS_DISPLAY_ADDRESS, payment_id?1:0);
        //index
        memmove(this->buffer_send+offset, &index, sizeof(cryptonote::subaddress_index));
        offset +=8 ;

        //payment ID
        if (payment_id) {
          memmove(this->buffer_send+offset, (*payment_id).data, 8);
        } else {
          memset(this->buffer_send+offset, 0, 8);
        }
        offset +=8;

        this->buffer_send[4] = offset-5;
        this->length_send = offset;
        CHECK_AND_ASSERT_THROW_MES(this->exchange_wait_on_input() == 0, "Timeout/Error on display address.");
    }

    /* ======================================================================= */
    /*                               SUB ADDRESS                               */
    /* ======================================================================= */

    bool device_ledger::derive_subaddress_public_key(const crypto::public_key &pub, const crypto::key_derivation &derivation, const std::size_t output_index, crypto::public_key &derived_pub){
        AUTO_LOCK_CMD();
        #ifdef DEBUG_HWDEVICE
        const crypto::public_key pub_x = pub;
        crypto::key_derivation derivation_x;
         if ((this->mode == TRANSACTION_PARSE) && has_view_key) {    
          derivation_x = derivation;
        } else {
          derivation_x = hw::ledger::decrypt(derivation);
        }
        const std::size_t output_index_x = output_index;
        crypto::public_key derived_pub_x;
        log_hexbuffer("derive_subaddress_public_key: [[IN]]  pub       ", pub_x.data, 32);
        log_hexbuffer("derive_subaddress_public_key: [[IN]]  derivation", derivation_x.data, 32);
        log_message  ("derive_subaddress_public_key: [[IN]]  index     ", std::to_string((int)output_index_x));
        this->controle_device->derive_subaddress_public_key(pub_x, derivation_x,output_index_x,derived_pub_x);
        log_hexbuffer("derive_subaddress_public_key: [[OUT]] derived_pub", derived_pub_x.data, 32);
        #endif

      if ((this->mode == TRANSACTION_PARSE) && has_view_key) {     
        //If we are in TRANSACTION_PARSE, the given derivation has been retrieved uncrypted (wihtout the help
        //of the device), so continue that way.
        MDEBUG( "derive_subaddress_public_key  : PARSE mode with known viewkey");     
        crypto::derive_subaddress_public_key(pub, derivation, output_index,derived_pub);
      } else {
       
        int offset = set_command_header_noopt(INS_DERIVE_SUBADDRESS_PUBLIC_KEY);
        //pub
        memmove(this->buffer_send+offset, pub.data, 32);
        offset += 32;
        //derivation
        this->send_secret((unsigned char*)derivation.data, offset);
        //index
        this->buffer_send[offset+0] = output_index>>24;
        this->buffer_send[offset+1] = output_index>>16;
        this->buffer_send[offset+2] = output_index>>8;
        this->buffer_send[offset+3] = output_index>>0;
        offset += 4;

        this->buffer_send[4] = offset-5;
        this->length_send = offset;
        this->exchange();

        //pub key
        memmove(derived_pub.data, &this->buffer_recv[0], 32);
      }
      #ifdef DEBUG_HWDEVICE
      hw::ledger::check32("derive_subaddress_public_key", "derived_pub", derived_pub_x.data, derived_pub.data);
      #endif

      return true;
    }

    crypto::public_key device_ledger::get_subaddress_spend_public_key(const cryptonote::account_keys& keys, const cryptonote::subaddress_index &index) {
        AUTO_LOCK_CMD();
        crypto::public_key D;

        #ifdef DEBUG_HWDEVICE
        const cryptonote::account_keys     keys_x =  hw::ledger::decrypt(keys);
        const cryptonote::subaddress_index index_x = index;
        crypto::public_key                 D_x;
        log_hexbuffer("get_subaddress_spend_public_key: [[IN]]  keys.m_view_secret_key ", keys_x.m_view_secret_key.data,32);
        log_hexbuffer("get_subaddress_spend_public_key: [[IN]]  keys.m_spend_secret_key", keys_x.m_spend_secret_key.data,32);
        log_message  ("get_subaddress_spend_public_key: [[IN]]  index               ", std::to_string(index_x.major)+"."+std::to_string(index_x.minor));
        D_x = this->controle_device->get_subaddress_spend_public_key(keys_x, index_x);
        log_hexbuffer("get_subaddress_spend_public_key: [[OUT]] derivation          ", D_x.data, 32);
        #endif

        if (index.is_zero()) {
           D = keys.m_account_address.m_spend_public_key;
        } else {

          int offset = set_command_header_noopt(INS_GET_SUBADDRESS_SPEND_PUBLIC_KEY);
          //index
          static_assert(sizeof(cryptonote::subaddress_index) == 8, "cryptonote::subaddress_index shall be 8 bytes length");
          memmove(this->buffer_send+offset, &index, sizeof(cryptonote::subaddress_index));
          offset +=8 ;

          this->buffer_send[4] = offset-5;
          this->length_send = offset;
          this->exchange();

          memmove(D.data, &this->buffer_recv[0], 32);
        }

        #ifdef DEBUG_HWDEVICE
        hw::ledger::check32("get_subaddress_spend_public_key", "D", D_x.data, D.data);
        #endif

        return D;
    }

    std::vector<crypto::public_key>  device_ledger::get_subaddress_spend_public_keys(const cryptonote::account_keys &keys, uint32_t account, uint32_t begin, uint32_t end) {
     std::vector<crypto::public_key> pkeys;
     cryptonote::subaddress_index index = {account, begin};
     crypto::public_key D;
     for (uint32_t idx = begin; idx < end; ++idx) {
        index.minor = idx;
        D = this->get_subaddress_spend_public_key(keys, index);
        pkeys.push_back(D);
      }
      return pkeys;
    }

    cryptonote::account_public_address device_ledger::get_subaddress(const cryptonote::account_keys& keys, const cryptonote::subaddress_index &index) {
        AUTO_LOCK_CMD();
        cryptonote::account_public_address address;

        #ifdef DEBUG_HWDEVICE
        const cryptonote::account_keys            keys_x =  hw::ledger::decrypt(keys);
        const cryptonote::subaddress_index        index_x = index;
        cryptonote::account_public_address  address_x;
        log_hexbuffer("get_subaddress: [[IN]]  keys.m_view_secret_key ", keys_x.m_view_secret_key.data, 32);
        log_hexbuffer("get_subaddress: [[IN]]  keys.m_view_public_key",  keys_x.m_account_address.m_view_public_key.data, 32);
        log_hexbuffer("get_subaddress: [[IN]]  keys.m_view_secret_key ", keys_x.m_view_secret_key.data, 32);
        log_hexbuffer("get_subaddress: [[IN]]  keys.m_spend_public_key", keys_x.m_account_address.m_spend_public_key.data, 32);
        log_message  ("get_subaddress: [[IN]]  index                                ", std::to_string(index_x.major)+"."+std::to_string(index_x.minor));
        address_x = this->controle_device->get_subaddress(keys_x, index_x);
        log_hexbuffer("get_subaddress: [[OUT]]  keys.m_view_public_key ", address_x.m_view_public_key.data, 32);
        log_hexbuffer("get_subaddress: [[OUT]]  keys.m_spend_public_key", address_x.m_spend_public_key.data, 32);
        #endif

        if (index.is_zero()) {
          address = keys.m_account_address;
        } else {
          int offset = set_command_header_noopt(INS_GET_SUBADDRESS);
          //index
          static_assert(sizeof(cryptonote::subaddress_index) == 8, "cryptonote::subaddress_index shall be 8 bytes length");
          memmove(this->buffer_send+offset, &index, sizeof(cryptonote::subaddress_index));
          offset +=8 ;

          this->buffer_send[4] = offset-5;
          this->length_send = offset;
          this->exchange();

          memmove(address.m_view_public_key.data,  &this->buffer_recv[0],  32);
          memmove(address.m_spend_public_key.data, &this->buffer_recv[32], 32);
        }

        #ifdef DEBUG_HWDEVICE
        hw::ledger::check32("get_subaddress", "address.m_view_public_key.data", address_x.m_view_public_key.data, address.m_view_public_key.data);
        hw::ledger::check32("get_subaddress", "address.m_spend_public_key.data", address_x.m_spend_public_key.data, address.m_spend_public_key.data);
        #endif

        return address;
    }

    crypto::secret_key  device_ledger::get_subaddress_secret_key(const crypto::secret_key &sec, const cryptonote::subaddress_index &index) {
        AUTO_LOCK_CMD();
        crypto::secret_key sub_sec;

        #ifdef DEBUG_HWDEVICE
        const crypto::secret_key            sec_x =  hw::ledger::decrypt(sec);
        const cryptonote::subaddress_index  index_x = index;
        crypto::secret_key            sub_sec_x;
        log_message  ("get_subaddress_secret_key: [[IN]]  index  ", std::to_string(index.major)+"."+std::to_string(index.minor));
        log_hexbuffer("get_subaddress_secret_key: [[IN]]  sec    ", sec_x.data, 32);
        sub_sec_x = this->controle_device->get_subaddress_secret_key(sec_x, index_x);
        log_hexbuffer("get_subaddress_secret_key: [[OUT]] sub_sec", sub_sec_x.data, 32);
        #endif

        int offset = set_command_header_noopt(INS_GET_SUBADDRESS_SECRET_KEY);
        //sec
        this->send_secret((unsigned char*)sec.data, offset);
        //index
        static_assert(sizeof(cryptonote::subaddress_index) == 8, "cryptonote::subaddress_index shall be 8 bytes length");
        memmove(this->buffer_send+offset, &index, sizeof(cryptonote::subaddress_index));
        offset +=8 ;

        this->buffer_send[4] = offset-5;
        this->length_send = offset;
        this->exchange();

        offset = 0;
        this->receive_secret((unsigned char*)sub_sec.data,  offset);

        #ifdef DEBUG_HWDEVICE
        crypto::secret_key            sub_sec_clear =   hw::ledger::decrypt(sub_sec);
        hw::ledger::check32("get_subaddress_secret_key", "sub_sec", sub_sec_x.data, sub_sec_clear.data);
        #endif

        return sub_sec;
    }

    /* ======================================================================= */
    /*                            DERIVATION & KEY                             */
    /* ======================================================================= */

    bool  device_ledger::verify_keys(const crypto::secret_key &secret_key, const crypto::public_key &public_key) {
        AUTO_LOCK_CMD();
        int offset;

        offset = set_command_header_noopt(INS_VERIFY_KEY);
        //sec
        this->send_secret((unsigned char*)secret_key.data, offset);
        //pub
        memmove(this->buffer_send+offset, public_key.data, 32);
        offset += 32;

        this->buffer_send[4] = offset-5;
        this->length_send = offset;
        this->exchange();

        uint32_t verified =
          this->buffer_recv[0] << 24 |
          this->buffer_recv[1] << 16 |
          this->buffer_recv[2] << 8  |
          this->buffer_recv[3] << 0  ;

        return verified == 1;
    }

    bool device_ledger::scalarmultKey(rct::key & aP, const rct::key &P, const rct::key &a) {
        AUTO_LOCK_CMD();

        #ifdef DEBUG_HWDEVICE
        const rct::key P_x    =  P;
        const rct::key a_x    =  hw::ledger::decrypt(a);
        rct::key aP_x;
        log_hexbuffer("scalarmultKey: [[IN]]  P ", (char*)P_x.bytes, 32);       
        log_hexbuffer("scalarmultKey: [[IN]]  a ", (char*)a_x.bytes, 32);       
        this->controle_device->scalarmultKey(aP_x, P_x, a_x);
        log_hexbuffer("scalarmultKey: [[OUT]] aP", (char*)aP_x.bytes, 32);       
        #endif

        int offset = set_command_header_noopt(INS_SECRET_SCAL_MUL_KEY);
        //pub
        memmove(this->buffer_send+offset, P.bytes, 32);
        offset += 32;
        //sec
        this->send_secret(a.bytes, offset);

        this->buffer_send[4] = offset-5;
        this->length_send = offset;
        this->exchange();

        //pub key
        memmove(aP.bytes, &this->buffer_recv[0], 32);

        #ifdef DEBUG_HWDEVICE
        hw::ledger::check32("scalarmultKey", "mulkey", (char*)aP_x.bytes, (char*)aP.bytes);
        #endif

        return true;
    }

    bool device_ledger::scalarmultBase(rct::key &aG, const rct::key &a) {
        AUTO_LOCK_CMD();

        #ifdef DEBUG_HWDEVICE
        const rct::key a_x    =  hw::ledger::decrypt(a);
        rct::key aG_x;
        log_hexbuffer("scalarmultKey: [[IN]]  a ", (char*)a_x.bytes, 32);       
        this->controle_device->scalarmultBase(aG_x, a_x);
        log_hexbuffer("scalarmultKey: [[OUT]] aG", (char*)aG_x.bytes, 32);       
        #endif

        int offset = set_command_header_noopt(INS_SECRET_SCAL_MUL_BASE);
        //sec
        this->send_secret(a.bytes, offset);

        this->buffer_send[4] = offset-5;
        this->length_send = offset;
        this->exchange();

        //pub key
        memmove(aG.bytes, &this->buffer_recv[0], 32);

        #ifdef DEBUG_HWDEVICE
        hw::ledger::check32("scalarmultBase", "mulkey", (char*)aG_x.bytes, (char*)aG.bytes);
        #endif

        return true;
    }

    bool device_ledger::sc_secret_add( crypto::secret_key &r, const crypto::secret_key &a, const crypto::secret_key &b) {
        AUTO_LOCK_CMD();
        int offset;

        #ifdef DEBUG_HWDEVICE
        const crypto::secret_key a_x = hw::ledger::decrypt(a);
        const crypto::secret_key b_x = hw::ledger::decrypt(b);
        crypto::secret_key r_x;
        rct::key aG_x;
        log_hexbuffer("sc_secret_add: [[IN]]  a ", (char*)a_x.data, 32);       
        log_hexbuffer("sc_secret_add: [[IN]]  b ", (char*)b_x.data, 32);       
        this->controle_device->sc_secret_add(r_x, a_x, b_x);
        log_hexbuffer("sc_secret_add: [[OUT]] aG", (char*)r_x.data, 32);       
        #endif

        offset = set_command_header_noopt(INS_SECRET_KEY_ADD);
        //sec key
        this->send_secret((unsigned char*)a.data, offset);
        //sec key
        this->send_secret((unsigned char*)b.data, offset);

        this->buffer_send[4] = offset-5;
        this->length_send = offset;
        this->exchange();

        //sec key
        offset = 0;
        this->receive_secret((unsigned char*)r.data, offset);

        #ifdef DEBUG_HWDEVICE
        crypto::secret_key r_clear = hw::ledger::decrypt(r);
        hw::ledger::check32("sc_secret_add", "r", r_x.data, r_clear.data);
        #endif

        return true;
    }

    crypto::secret_key  device_ledger::generate_keys(crypto::public_key &pub, crypto::secret_key &sec, const crypto::secret_key& recovery_key, bool recover) {
        AUTO_LOCK_CMD();
        int offset;

        if (recover) {
           throw std::runtime_error("device generate key does not support recover");
        }

        #ifdef DEBUG_HWDEVICE
        crypto::public_key pub_x;
        crypto::secret_key sec_x;
        crypto::secret_key recovery_key_x;
        if (recover) {
         recovery_key_x = hw::ledger::decrypt(recovery_key);
         log_hexbuffer("generate_keys: [[IN]] pub", (char*)recovery_key_x.data, 32);       
        }
        #endif

        send_simple(INS_GENERATE_KEYPAIR);

        offset = 0;
        //pub key
        memmove(pub.data, &this->buffer_recv[0], 32);
        offset += 32;
        this->receive_secret((unsigned char*)sec.data, offset);

        #ifdef DEBUG_HWDEVICE
        crypto::secret_key sec_clear = hw::ledger::decrypt(sec);
        sec_x = sec_clear;
        log_hexbuffer("generate_keys: [[OUT]] pub", (char*)pub.data, 32);       
        log_hexbuffer("generate_keys: [[OUT]] sec", (char*)sec_clear.data, 32);       

        crypto::secret_key_to_public_key(sec_x,pub_x);
        hw::ledger::check32("generate_keys", "pub", pub_x.data, pub.data);
        #endif

        return sec;

    }

    bool device_ledger::generate_key_derivation(const crypto::public_key &pub, const crypto::secret_key &sec, crypto::key_derivation &derivation) {
        AUTO_LOCK_CMD();
        bool r = false;

        #ifdef DEBUG_HWDEVICE
        const crypto::public_key pub_x = pub;
        const crypto::secret_key sec_x = (sec == rct::rct2sk(rct::I)) ? sec: hw::ledger::decrypt(sec);
        crypto::key_derivation derivation_x;
        log_hexbuffer("generate_key_derivation: [[IN]]  pub       ", pub_x.data, 32);
        log_hexbuffer("generate_key_derivation: [[IN]]  sec       ", sec_x.data, 32);
        this->controle_device->generate_key_derivation(pub_x, sec_x, derivation_x);
        log_hexbuffer("generate_key_derivation: [[OUT]] derivation", derivation_x.data, 32);
        #endif

      if ((this->mode == TRANSACTION_PARSE)  && has_view_key) {
        //A derivation is resquested in PASRE mode and we have the view key,
        //so do that wihtout the device and return the derivation unencrypted.
        MDEBUG( "generate_key_derivation  : PARSE mode with known viewkey");     
        //Note derivation in PARSE mode can only happen with viewkey, so assert it!
        assert(is_fake_view_key(sec));
        r = crypto::generate_key_derivation(pub, this->viewkey, derivation);
      } else {
        int offset = set_command_header_noopt(INS_GEN_KEY_DERIVATION);
        //pub
        memmove(this->buffer_send+offset, pub.data, 32);
        offset += 32;
         //sec
        this->send_secret((unsigned char*)sec.data, offset);

        this->buffer_send[4] = offset-5;
        this->length_send = offset;
        this->exchange();

        offset = 0;
        //derivattion data
        this->receive_secret((unsigned char*)derivation.data, offset);

        r = true;
      }
      #ifdef DEBUG_HWDEVICE
      crypto::key_derivation derivation_clear ;
      if ((this->mode == TRANSACTION_PARSE)  && has_view_key) {
        derivation_clear  = derivation;
      } else {
        derivation_clear  = hw::ledger::decrypt(derivation);
      }
      hw::ledger::check32("generate_key_derivation", "derivation", derivation_x.data, derivation_clear.data);
      #endif

      return r;
    }

    bool device_ledger::conceal_derivation(crypto::key_derivation &derivation, const crypto::public_key &tx_pub_key, const std::vector<crypto::public_key> &additional_tx_pub_keys, const crypto::key_derivation &main_derivation, const std::vector<crypto::key_derivation> &additional_derivations) {
      const crypto::public_key *pkey=NULL;
      if (derivation == main_derivation) {        
        pkey = &tx_pub_key;
        MDEBUG("conceal derivation with main tx pub key");
      } else {
        for(size_t n=0; n < additional_derivations.size();++n) {
          if(derivation == additional_derivations[n]) {
            pkey = &additional_tx_pub_keys[n];
            MDEBUG("conceal derivation with additionnal tx pub key");
            break;
          }
        }
      }
      ASSERT_X(pkey, "Mismatched derivation on scan info");
      return this->generate_key_derivation(*pkey,  crypto::null_skey, derivation);
    } 

    bool device_ledger::derivation_to_scalar(const crypto::key_derivation &derivation, const size_t output_index, crypto::ec_scalar &res) {
        AUTO_LOCK_CMD();

        #ifdef DEBUG_HWDEVICE
        const crypto::key_derivation derivation_x = hw::ledger::decrypt(derivation);
        const size_t output_index_x               = output_index;
        crypto::ec_scalar res_x;
        log_hexbuffer("derivation_to_scalar: [[IN]]  derivation    ", derivation_x.data, 32);
        log_message  ("derivation_to_scalar: [[IN]]  output_index  ", std::to_string(output_index_x));
        this->controle_device->derivation_to_scalar(derivation_x, output_index_x, res_x);
        log_hexbuffer("derivation_to_scalar: [[OUT]] res          ", res_x.data, 32);
        #endif

        int offset = set_command_header_noopt(INS_DERIVATION_TO_SCALAR);
        //derivation
        this->send_secret((unsigned char*)derivation.data, offset);

        //index
        this->buffer_send[offset+0] = output_index>>24;
        this->buffer_send[offset+1] = output_index>>16;
        this->buffer_send[offset+2] = output_index>>8;
        this->buffer_send[offset+3] = output_index>>0;
        offset += 4;

        this->buffer_send[4] = offset-5;
        this->length_send = offset;
        this->exchange();

        //derivation data
        offset = 0;
        this->receive_secret((unsigned char*)res.data, offset);

        #ifdef DEBUG_HWDEVICE
        crypto::ec_scalar res_clear  = hw::ledger::decrypt(res);
        hw::ledger::check32("derivation_to_scalar", "res", res_x.data, res_clear.data);
        #endif

        return true;
    }

    bool device_ledger::derive_secret_key(const crypto::key_derivation &derivation, const std::size_t output_index, const crypto::secret_key &sec, crypto::secret_key &derived_sec) {
        AUTO_LOCK_CMD();

        #ifdef DEBUG_HWDEVICE
        const crypto::key_derivation derivation_x   = hw::ledger::decrypt(derivation);
        const std::size_t            output_index_x = output_index;
        const crypto::secret_key     sec_x          = hw::ledger::decrypt(sec);
        crypto::secret_key           derived_sec_x;
        log_hexbuffer("derive_secret_key: [[IN]]  derivation ", derivation_x.data, 32);
        log_message  ("derive_secret_key: [[IN]]  index      ", std::to_string(output_index_x));
        log_hexbuffer("derive_secret_key: [[IN]]  sec        ", sec_x.data, 32);
        this->controle_device->derive_secret_key(derivation_x, output_index_x, sec_x, derived_sec_x);
        log_hexbuffer("derive_secret_key: [[OUT]] derived_sec", derived_sec_x.data, 32);
        #endif

        int offset = set_command_header_noopt(INS_DERIVE_SECRET_KEY);
        //derivation
        this->send_secret((unsigned char*)derivation.data, offset);
        //index
        this->buffer_send[offset+0] = output_index>>24;
        this->buffer_send[offset+1] = output_index>>16;
        this->buffer_send[offset+2] = output_index>>8;
        this->buffer_send[offset+3] = output_index>>0;
        offset += 4;
        //sec
        this->send_secret((unsigned char*)sec.data, offset);

        this->buffer_send[4] = offset-5;
        this->length_send = offset;
        this->exchange();

        offset = 0;
        //sec key
        this->receive_secret((unsigned char*)derived_sec.data, offset);

        #ifdef DEBUG_HWDEVICE
        crypto::secret_key derived_sec_clear = hw::ledger::decrypt(derived_sec);
        hw::ledger::check32("derive_secret_key", "derived_sec", derived_sec_x.data, derived_sec_clear.data);
        #endif

        return true;
    }

    bool device_ledger::derive_public_key(const crypto::key_derivation &derivation, const std::size_t output_index, const crypto::public_key &pub, crypto::public_key &derived_pub){
        AUTO_LOCK_CMD();
      
        #ifdef DEBUG_HWDEVICE
        const crypto::key_derivation derivation_x   = hw::ledger::decrypt(derivation);
        const std::size_t            output_index_x = output_index;
        const crypto::public_key     pub_x        = pub;
        crypto::public_key           derived_pub_x;
        log_hexbuffer("derive_public_key: [[IN]]  derivation  ", derivation_x.data, 32);
        log_message  ("derive_public_key: [[IN]]  output_index", std::to_string(output_index_x));
        log_hexbuffer("derive_public_key: [[IN]]  pub         ", pub_x.data, 32);
        this->controle_device->derive_public_key(derivation_x, output_index_x, pub_x, derived_pub_x);
        log_hexbuffer("derive_public_key: [[OUT]] derived_pub ", derived_pub_x.data, 32);
        #endif

        int offset = set_command_header_noopt(INS_DERIVE_PUBLIC_KEY);
        //derivation
        this->send_secret((unsigned char*)derivation.data, offset);
        //index
        this->buffer_send[offset+0] = output_index>>24;
        this->buffer_send[offset+1] = output_index>>16;
        this->buffer_send[offset+2] = output_index>>8;
        this->buffer_send[offset+3] = output_index>>0;
        offset += 4;
        //pub
        memmove(this->buffer_send+offset, pub.data, 32);
        offset += 32;

        this->buffer_send[4] = offset-5;
        this->length_send = offset;
        this->exchange();

        //pub key
        memmove(derived_pub.data, &this->buffer_recv[0], 32);

        #ifdef DEBUG_HWDEVICE
        hw::ledger::check32("derive_public_key", "derived_pub", derived_pub_x.data, derived_pub.data);
        #endif

        return true;
    }

    bool device_ledger::secret_key_to_public_key(const crypto::secret_key &sec, crypto::public_key &pub) {
        AUTO_LOCK_CMD();

        #ifdef DEBUG_HWDEVICE
        const crypto::secret_key sec_x = hw::ledger::decrypt(sec);
        crypto::public_key       pub_x;
        log_hexbuffer("secret_key_to_public_key: [[IN]] sec ", sec_x.data, 32);
        bool rc = this->controle_device->secret_key_to_public_key(sec_x, pub_x);
        log_hexbuffer("secret_key_to_public_key: [[OUT]] pub", pub_x.data, 32);
        if (!rc){
          log_message("FAIL secret_key_to_public_key", "secret_key rejected");
        }
        #endif

        int offset = set_command_header_noopt(INS_SECRET_KEY_TO_PUBLIC_KEY);
        //sec key
        this->send_secret((unsigned char*)sec.data, offset);

        this->buffer_send[4] = offset-5;
        this->length_send = offset;
        this->exchange();

        //pub key
        memmove(pub.data, &this->buffer_recv[0], 32);

        #ifdef DEBUG_HWDEVICE
        hw::ledger::check32("secret_key_to_public_key", "pub", pub_x.data, pub.data);
        #endif

        return true;
    }

    bool device_ledger::generate_key_image(const crypto::public_key &pub, const crypto::secret_key &sec, crypto::key_image &image){
        AUTO_LOCK_CMD();

        #ifdef DEBUG_HWDEVICE
        const crypto::public_key pub_x = pub;
        const crypto::secret_key sec_x = hw::ledger::decrypt(sec);
        crypto::key_image        image_x;
        log_hexbuffer("generate_key_image: [[IN]]  pub ", pub_x.data, 32);
        log_hexbuffer("generate_key_image: [[IN]]  sec ", sec_x.data, 32);
        this->controle_device->generate_key_image(pub_x, sec_x, image_x);
        log_hexbuffer("generate_key_image: [[OUT]] image ", image_x.data, 32);
        #endif

        int offset = set_command_header_noopt(INS_GEN_KEY_IMAGE);
        //pub
        memmove(this->buffer_send+offset, pub.data, 32);
        offset += 32;
        //sec
        this->send_secret((unsigned char*)sec.data, offset);

        this->buffer_send[4] = offset-5;
        this->length_send = offset;
        this->exchange();

        //pub key
        memmove(image.data, &this->buffer_recv[0], 32);

        #ifdef DEBUG_HWDEVICE
        hw::ledger::check32("generate_key_image", "image", image_x.data, image.data);
        #endif

        return true;
    }

    /* ======================================================================= */
    /*                               TRANSACTION                               */
    /* ======================================================================= */

    void device_ledger::generate_tx_proof(const crypto::hash &prefix_hash, 
                                          const crypto::public_key &R, const crypto::public_key &A, const boost::optional<crypto::public_key> &B, const crypto::public_key &D, const crypto::secret_key &r, 
                                          crypto::signature &sig)  {

      AUTO_LOCK_CMD();

      #ifdef DEBUG_HWDEVICE
      const crypto::hash prefix_hash_x = prefix_hash;
      const crypto::public_key R_x = R;
      const crypto::public_key A_x = A;
      const boost::optional<crypto::public_key> B_x = B;
      const crypto::public_key D_x = D;
      const crypto::secret_key r_x = hw::ledger::decrypt(r); 
      crypto::signature sig_x;
      log_hexbuffer("generate_tx_proof: [[IN]]  prefix_hash ", prefix_hash_x.data, 32);
      log_hexbuffer("generate_tx_proof: [[IN]]  R ", R_x.data, 32);
      log_hexbuffer("generate_tx_proof: [[IN]]  A ", A_x.data, 32);
      if (B_x) {
        log_hexbuffer("generate_tx_proof: [[IN]]  B ", (*B_x).data, 32);
      }
      log_hexbuffer("generate_tx_proof: [[IN]]  D ", D_x.data, 32);
      log_hexbuffer("generate_tx_proof: [[IN]]  r ", r_x.data, 32);       
      #endif

         
      int offset = set_command_header(INS_GET_TX_PROOF);
      //options
      this->buffer_send[offset] = B?0x01:0x00;
      offset += 1;   
      //prefix_hash
      memmove(&this->buffer_send[offset], prefix_hash.data, 32);
      offset += 32;
      // R
      memmove(&this->buffer_send[offset], R.data, 32);
      offset += 32;
      // A
      memmove(&this->buffer_send[offset], A.data, 32);
      offset += 32;
      // B
      if (B) {
        memmove(&this->buffer_send[offset], (*B).data, 32);
      } else {
        memset(&this->buffer_send[offset], 0, 32);
      }
      offset += 32;
      // D
      memmove(&this->buffer_send[offset], D.data, 32);
      offset += 32;
      // r
      this->send_secret((unsigned char*)r.data, offset);

      this->buffer_send[4] = offset-5;
      this->length_send = offset;
      this->exchange();

      memmove(sig.c.data, &this->buffer_recv[0], 32);
      memmove(sig.r.data, &this->buffer_recv[32], 32);
      #ifdef DEBUG_HWDEVICE      
      log_hexbuffer("GENERATE_TX_PROOF: **c**   ", sig.c.data, sizeof( sig.c.data));
      log_hexbuffer("GENERATE_TX_PROOF: **r**   ", sig.r.data, sizeof( sig.r.data));

      this->controle_device->generate_tx_proof(prefix_hash_x, R_x, A_x, B_x, D_x, r_x, sig_x);      
      MDEBUG("FAIL is normal if random is not fixed in proof");
      hw::ledger::check32("generate_tx_proof", "c", sig_x.c.data, sig.c.data);
      hw::ledger::check32("generate_tx_proof", "r", sig_x.r.data, sig.r.data);

      #endif
    }

    bool device_ledger::open_tx(crypto::secret_key &tx_key) {
        AUTO_LOCK_CMD();
        this->lock();
        key_map.clear();
        hmac_map.clear();
        this->tx_in_progress = true;
        int offset = set_command_header_noopt(INS_OPEN_TX, 0x01);

        //account
        this->buffer_send[offset+0] = 0x00;
        this->buffer_send[offset+1] = 0x00;
        this->buffer_send[offset+2] = 0x00;
        this->buffer_send[offset+3] = 0x00;
        offset += 4;

        this->buffer_send[4] = offset-5;
        this->length_send = offset;
        this->exchange();

        //skip R, receive: r, r_hmac, fake_a, a_hmac, fake_b, hmac_b
        unsigned char tmp[32];
        offset = 32;
        this->receive_secret((unsigned char*)tx_key.data, offset);
        this->receive_secret(tmp, offset);
        this->receive_secret(tmp, offset);

        #ifdef DEBUG_HWDEVICE
        const crypto::secret_key r_x = hw::ledger::decrypt(tx_key); 
        log_hexbuffer("open_tx: [[OUT]] R ", (char*)&this->buffer_recv[0], 32);
        log_hexbuffer("open_tx: [[OUT]] r ", r_x.data, 32);
        #endif
        return true;
    }

    bool device_ledger::encrypt_payment_id(crypto::hash8 &payment_id, const crypto::public_key &public_key, const crypto::secret_key &secret_key) {
        AUTO_LOCK_CMD();

        #ifdef DEBUG_HWDEVICE
        const crypto::public_key public_key_x = public_key;
        const crypto::secret_key secret_key_x = hw::ledger::decrypt(secret_key);
        crypto::hash8 payment_id_x = payment_id;
        log_hexbuffer("encrypt_payment_id: [[IN]] payment_id ", payment_id_x.data, 32);
        log_hexbuffer("encrypt_payment_id: [[IN]] public_key ", public_key_x.data, 32);
        log_hexbuffer("encrypt_payment_id: [[IN]] secret_key ", secret_key_x.data, 32);
        this->controle_device->encrypt_payment_id(payment_id_x, public_key_x, secret_key_x);
        log_hexbuffer("encrypt_payment_id: [[OUT]] payment_id ", payment_id_x.data, 32);
        #endif

        int offset = set_command_header_noopt(INS_STEALTH);
        //pub
        memmove(&this->buffer_send[offset], public_key.data, 32);
        offset += 32;
        //sec
        this->send_secret((unsigned char*)secret_key.data, offset);
        //id
        memmove(&this->buffer_send[offset], payment_id.data, 8);
        offset += 8;

        this->buffer_send[4] = offset-5;
        this->length_send = offset;
        this->exchange();
        memmove(payment_id.data, &this->buffer_recv[0], 8);

        #ifdef DEBUG_HWDEVICE
        hw::ledger::check8("stealth", "payment_id", payment_id_x.data, payment_id.data);
        #endif

        return true;
    }


    bool device_ledger::generate_output_ephemeral_keys(const size_t tx_version, const cryptonote::account_keys &sender_account_keys, const crypto::public_key &txkey_pub,  const crypto::secret_key &tx_key,
                                                       const cryptonote::tx_destination_entry &dst_entr, const boost::optional<cryptonote::account_public_address> &change_addr, const size_t output_index,
                                                       const bool &need_additional_txkeys,  const std::vector<crypto::secret_key> &additional_tx_keys,
                                                       std::vector<crypto::public_key> &additional_tx_public_keys,
                                                       std::vector<rct::key> &amount_keys,
                                                       crypto::public_key &out_eph_public_key) {
      AUTO_LOCK_CMD();

      #ifdef DEBUG_HWDEVICE
      const size_t                             &tx_version_x                   = tx_version;
      const cryptonote::account_keys           sender_account_keys_x          = hw::ledger::decrypt(sender_account_keys);
      memmove((void*)sender_account_keys_x.m_view_secret_key.data, dbg_viewkey.data, 32);

      const crypto::public_key                 txkey_pub_x                    = txkey_pub;
      const crypto::secret_key                 tx_key_x                       = hw::ledger::decrypt(tx_key);
      const cryptonote::tx_destination_entry   dst_entr_x                     = dst_entr;
      const boost::optional<cryptonote::account_public_address> change_addr_x = change_addr;
      const size_t                             output_index_x                 = output_index;
      const bool                               need_additional_txkeys_x       = need_additional_txkeys;
      
      std::vector<crypto::secret_key>    additional_tx_keys_x;
      for (const auto k: additional_tx_keys) {
        additional_tx_keys_x.push_back(hw::ledger::decrypt(k));
      }
      
      std::vector<crypto::public_key>          additional_tx_public_keys_x;
      std::vector<rct::key>                    amount_keys_x;
      crypto::public_key                       out_eph_public_key_x;

      log_message  ("generate_output_ephemeral_keys: [[IN]] tx_version", std::to_string(tx_version_x));
      //log_hexbuffer("generate_output_ephemeral_keys: [[IN]] sender_account_keys.view", sender_account_keys.m_sview_secret_key.data, 32);
      //log_hexbuffer("generate_output_ephemeral_keys: [[IN]] sender_account_keys.spend", sender_account_keys.m_spend_secret_key.data, 32);
      log_hexbuffer("generate_output_ephemeral_keys: [[IN]] txkey_pub", txkey_pub_x.data, 32);
      log_hexbuffer("generate_output_ephemeral_keys: [[IN]] tx_key", tx_key_x.data, 32);
      log_hexbuffer("generate_output_ephemeral_keys: [[IN]] dst_entr.view", dst_entr_x.addr.m_view_public_key.data, 32);
      log_hexbuffer("generate_output_ephemeral_keys: [[IN]] dst_entr.spend", dst_entr_x.addr.m_spend_public_key.data, 32);
      if (change_addr) {
        log_hexbuffer("generate_output_ephemeral_keys: [[IN]] change_addr.view", (*change_addr_x).m_view_public_key.data, 32);
        log_hexbuffer("generate_output_ephemeral_keys: [[IN]] change_addr.spend", (*change_addr_x).m_spend_public_key.data, 32);
      }
      log_message  ("generate_output_ephemeral_keys: [[IN]] output_index",  std::to_string(output_index_x));
      log_message  ("generate_output_ephemeral_keys: [[IN]] need_additional_txkeys",  std::to_string(need_additional_txkeys_x));
      if(need_additional_txkeys_x) {
        log_hexbuffer("generate_output_ephemeral_keys: [[IN]] additional_tx_keys[oi]", additional_tx_keys_x[output_index].data, 32);
      }
      this->controle_device->generate_output_ephemeral_keys(tx_version_x, sender_account_keys_x, txkey_pub_x, tx_key_x, dst_entr_x, change_addr_x, output_index_x, need_additional_txkeys_x,  additional_tx_keys_x,
                                                            additional_tx_public_keys_x, amount_keys_x, out_eph_public_key_x);
      if(need_additional_txkeys_x) {
        log_hexbuffer("additional_tx_public_keys_x: [[OUT]] additional_tx_public_keys_x", additional_tx_public_keys_x.back().data, 32);
      }
      log_hexbuffer("generate_output_ephemeral_keys: [[OUT]] amount_keys ", (char*)amount_keys_x.back().bytes, 32);
      log_hexbuffer("generate_output_ephemeral_keys: [[OUT]] out_eph_public_key ", out_eph_public_key_x.data, 32);
      #endif

      ASSERT_X(tx_version > 1, "TX version not supported"<<tx_version);

      // make additional tx pubkey if necessary
      cryptonote::keypair additional_txkey;
      if (need_additional_txkeys) {
          additional_txkey.sec = additional_tx_keys[output_index];
      }

      int offset = set_command_header_noopt(INS_GEN_TXOUT_KEYS);
      //tx_version
      this->buffer_send[offset+0] = tx_version>>24;
      this->buffer_send[offset+1] = tx_version>>16;
      this->buffer_send[offset+2] = tx_version>>8;
      this->buffer_send[offset+3] = tx_version>>0;
      offset += 4;
      //tx_key
      this->send_secret((unsigned char*)tx_key.data, offset);
      //txkey_pub
      memmove(&this->buffer_send[offset], txkey_pub.data, 32);
      offset += 32;
      //Aout
      memmove(&this->buffer_send[offset], dst_entr.addr.m_view_public_key.data, 32);
      offset += 32;
      //Bout
      memmove(&this->buffer_send[offset], dst_entr.addr.m_spend_public_key.data, 32);
      offset += 32;
      //output index
      this->buffer_send[offset+0] = output_index>>24;
      this->buffer_send[offset+1] = output_index>>16;
      this->buffer_send[offset+2] = output_index>>8;
      this->buffer_send[offset+3] = output_index>>0;
      offset += 4;
      //is_change,
      bool is_change = (change_addr && dst_entr.addr == *change_addr);
      this->buffer_send[offset] = is_change;
      offset++;
      //is_subaddress
      this->buffer_send[offset] = dst_entr.is_subaddress;
      offset++;
      //need_additional_key
      this->buffer_send[offset] = need_additional_txkeys;
      offset++;
      //additional_tx_key
      if (need_additional_txkeys) {
        this->send_secret((unsigned char*)additional_txkey.sec.data, offset);
      } else {
        memset(&this->buffer_send[offset], 0, 32);
        offset += 32;
      }

      this->buffer_send[4] = offset-5;
      this->length_send = offset;
      this->exchange();

      offset = 0;
      unsigned int recv_len = this->length_recv;
      
      //if (tx_version > 1)
      {
        ASSERT_X(recv_len>=32, "Not enought data from device");
        crypto::secret_key scalar1;
        this->receive_secret((unsigned char*)scalar1.data, offset);
        amount_keys.push_back(rct::sk2rct(scalar1));
        recv_len -= 32;
      }
      ASSERT_X(recv_len>=32, "Not enought data from device");
      memmove(out_eph_public_key.data, &this->buffer_recv[offset], 32);
      recv_len -= 32;
      offset += 32;

      if (need_additional_txkeys)
      {
        ASSERT_X(recv_len>=32, "Not enought data from device");
        memmove(additional_txkey.pub.data, &this->buffer_recv[offset], 32);
        additional_tx_public_keys.push_back(additional_txkey.pub);
        offset += 32;
        recv_len -= 32;
      }

      // add ABPkeys
      this->add_output_key_mapping(dst_entr.addr.m_view_public_key, dst_entr.addr.m_spend_public_key, dst_entr.is_subaddress, is_change,
                                   need_additional_txkeys, output_index,
                                   amount_keys.back(), out_eph_public_key);

      #ifdef DEBUG_HWDEVICE
      log_hexbuffer("generate_output_ephemeral_keys: clear amount_key", (const char*)hw::ledger::decrypt(amount_keys.back()).bytes, 32);
      hw::ledger::check32("generate_output_ephemeral_keys", "amount_key", (const char*)amount_keys_x.back().bytes, (const char*)hw::ledger::decrypt(amount_keys.back()).bytes);
      if (need_additional_txkeys) {
        hw::ledger::check32("generate_output_ephemeral_keys", "additional_tx_key", additional_tx_public_keys_x.back().data, additional_tx_public_keys.back().data);
      }
      hw::ledger::check32("generate_output_ephemeral_keys", "out_eph_public_key", out_eph_public_key_x.data, out_eph_public_key.data);
      #endif

      return true;
    }

    bool  device_ledger::add_output_key_mapping(const crypto::public_key &Aout, const crypto::public_key &Bout, const bool is_subaddress, const bool is_change,
                                                const bool need_additional, const size_t real_output_index,
                                                const rct::key &amount_key,  const crypto::public_key &out_eph_public_key)  {
        key_map.add(ABPkeys(rct::pk2rct(Aout),rct::pk2rct(Bout), is_subaddress, is_change, need_additional, real_output_index, rct::pk2rct(out_eph_public_key), amount_key));
        return true;
    }

    rct::key device_ledger::genCommitmentMask(const rct::key &AKout) {
        #ifdef DEBUG_HWDEVICE
        const rct::key AKout_x =  hw::ledger::decrypt(AKout);
        rct::key mask_x;
        mask_x = this->controle_device->genCommitmentMask(AKout_x);
        #endif

        rct::key mask;
        int offset = set_command_header_noopt(INS_GEN_COMMITMENT_MASK);
        // AKout
        this->send_secret(AKout.bytes, offset);

        this->buffer_send[4] = offset-5;
        this->length_send = offset;
        this->exchange();

        memmove(mask.bytes, &this->buffer_recv[0],  32);

        #ifdef DEBUG_HWDEVICE
        hw::ledger::check32("genCommitmentMask", "mask", (const char*)mask_x.bytes, (const char*)mask.bytes);
        #endif
        
        return mask;
    }

    bool  device_ledger::ecdhEncode(rct::ecdhTuple & unmasked, const rct::key & AKout, bool short_amount) {
        AUTO_LOCK_CMD();

        #ifdef DEBUG_HWDEVICE
        const rct::key AKout_x =   hw::ledger::decrypt(AKout);
        rct::ecdhTuple unmasked_x = unmasked;
        this->controle_device->ecdhEncode(unmasked_x, AKout_x, short_amount);
        #endif

        int offset = set_command_header(INS_BLIND);
        //options
        this->buffer_send[offset] = short_amount?0x02:0x00;
        offset += 1;        
        // AKout
        this->send_secret(AKout.bytes, offset);
        //mask k
        memmove(this->buffer_send+offset, unmasked.mask.bytes, 32);
        offset += 32;
        //value v
        memmove(this->buffer_send+offset, unmasked.amount.bytes, 32);
        offset += 32;

        this->buffer_send[4] = offset-5;
        this->length_send = offset;
        this->exchange();

        memmove(unmasked.amount.bytes, &this->buffer_recv[0],  32);
        memmove(unmasked.mask.bytes,  &this->buffer_recv[32], 32);

        #ifdef DEBUG_HWDEVICE
        MDEBUG("ecdhEncode: Akout: "<<AKout_x);
        hw::ledger::check32("ecdhEncode", "amount", (char*)unmasked_x.amount.bytes, (char*)unmasked.amount.bytes);
        hw::ledger::check32("ecdhEncode", "mask", (char*)unmasked_x.mask.bytes, (char*)unmasked.mask.bytes);

        log_hexbuffer("Blind AKV input", (char*)&this->buffer_recv[64], 3*32);
        #endif

        return true;
    }

    bool  device_ledger::ecdhDecode(rct::ecdhTuple & masked, const rct::key & AKout, bool short_amount) {
        AUTO_LOCK_CMD();

        #ifdef DEBUG_HWDEVICE
        const rct::key AKout_x =   hw::ledger::decrypt(AKout);
        rct::ecdhTuple masked_x = masked;
        this->controle_device->ecdhDecode(masked_x, AKout_x, short_amount);
        #endif

        int offset = set_command_header(INS_UNBLIND);
        //options
        this->buffer_send[offset] = short_amount?0x02:0x00;
        offset += 1;        
        // AKout
        this->send_secret(AKout.bytes, offset);
        //mask k
        memmove(this->buffer_send+offset, masked.mask.bytes, 32);
        offset += 32;
        //value v
        memmove(this->buffer_send+offset, masked.amount.bytes, 32);
        offset += 32;

        this->buffer_send[4] = offset-5;
        this->length_send = offset;
        this->exchange();

        memmove(masked.amount.bytes, &this->buffer_recv[0],  32);
        memmove(masked.mask.bytes,  &this->buffer_recv[32], 32);

        #ifdef DEBUG_HWDEVICE
        MDEBUG("ecdhEncode: Akout: "<<AKout_x);
        hw::ledger::check32("ecdhDecode", "amount", (char*)masked_x.amount.bytes, (char*)masked.amount.bytes);
        hw::ledger::check32("ecdhDecode", "mask", (char*)masked_x.mask.bytes,(char*) masked.mask.bytes);
        #endif

        return true;
    }

    bool device_ledger::mlsag_prehash(const std::string &blob, size_t inputs_size, size_t outputs_size,
                                     const rct::keyV &hashes, const rct::ctkeyV &outPk,
                                     rct::key &prehash) {
        AUTO_LOCK_CMD();
        unsigned int  data_offset, C_offset, kv_offset, i;
        const char *data;

        #ifdef DEBUG_HWDEVICE
        const std::string blob_x  = blob;
        size_t inputs_size_x      = inputs_size;
        size_t outputs_size_x     = outputs_size;
        const rct::keyV hashes_x  = hashes;
        const rct::ctkeyV outPk_x = outPk;
        rct::key prehash_x;
        this->controle_device->mlsag_prehash(blob_x, inputs_size_x, outputs_size_x, hashes_x, outPk_x, prehash_x);
        if (inputs_size) {
          log_message("mlsag_prehash", (std::string("inputs_size not null: ") +  std::to_string(inputs_size)).c_str());
        }
        this->key_map.log();
        #endif

        data = blob.data();

        // ======  u8 type, varint txnfee ======
        int offset = set_command_header(INS_VALIDATE, 0x01, 0x01);
        //options
        this->buffer_send[offset] = (inputs_size == 0)?0x00:0x80;
        offset += 1;

        //type
        uint8_t type = data[0];
        this->buffer_send[offset] = data[0];
        offset += 1;

        //txnfee
        data_offset = 1;
        while (data[data_offset]&0x80) {
          this->buffer_send[offset] = data[data_offset];
          offset += 1;
          data_offset += 1;
        }
        this->buffer_send[offset] = data[data_offset];
        offset += 1;
        data_offset += 1;

        this->buffer_send[4] = offset-5;
        this->length_send = offset;
        // check fee user input
        CHECK_AND_ASSERT_THROW_MES(this->exchange_wait_on_input() == 0, "Fee denied on device.");

        //pseudoOuts
        if (type == rct::RCTTypeSimple) {
          for ( i = 0; i < inputs_size; i++) {
            offset = set_command_header(INS_VALIDATE, 0x01, i+2);
            //options
            this->buffer_send[offset] = (i==inputs_size-1)? 0x00:0x80;
            offset += 1;
            //pseudoOut
            memmove(this->buffer_send+offset, data+data_offset,32);
            offset += 32;
            data_offset += 32;

            this->buffer_send[4] = offset-5;
            this->length_send = offset;
            this->exchange();
          }
        }

        // ======  Aout, Bout, AKout, C, v, k ======
        kv_offset = data_offset;
        if (type==rct::RCTTypeBulletproof2) {
          C_offset = kv_offset+ (8)*outputs_size;
        } else {
          C_offset = kv_offset+ (32+32)*outputs_size;
        }
        for ( i = 0; i < outputs_size; i++) {
          ABPkeys outKeys;
          bool found;

          found = this->key_map.find(outPk[i].dest, outKeys);
          if (!found) {
            log_hexbuffer("Pout not found", (char*)outPk[i].dest.bytes, 32);
            CHECK_AND_ASSERT_THROW_MES(found, "Pout not found");
          }
          offset = set_command_header(INS_VALIDATE, 0x02, i+1);
          //options
          this->buffer_send[offset] = (i==outputs_size-1)? 0x00:0x80 ;
          this->buffer_send[offset] |= (type==rct::RCTTypeBulletproof2)?0x02:0x00;
          offset += 1;
          //is_subaddress
          this->buffer_send[offset] = outKeys.is_subaddress;
          offset++;
          //is_change_address
          this->buffer_send[offset] = outKeys.is_change_address;
          offset++;
          //Aout
          memmove(this->buffer_send+offset, outKeys.Aout.bytes, 32);
          offset+=32;
          //Bout
          memmove(this->buffer_send+offset, outKeys.Bout.bytes, 32);
          offset+=32;
          //AKout
          this->send_secret(outKeys.AKout.bytes, offset);

          //C
          memmove(this->buffer_send+offset, data+C_offset,32);
          offset += 32;
          C_offset += 32;
          if (type==rct::RCTTypeBulletproof2) {
            //k
            memset(this->buffer_send+offset, 0, 32);
            offset += 32;
            //v
            memset(this->buffer_send+offset, 0, 32);
            memmove(this->buffer_send+offset, data+kv_offset,8);
            offset += 32;
            kv_offset += 8;
          } else {
            //k
            memmove(this->buffer_send+offset, data+kv_offset,32);
            offset += 32;
            kv_offset += 32;
            //v
            memmove(this->buffer_send+offset, data+kv_offset,32);
            offset += 32;
            kv_offset += 32;
          }

          this->buffer_send[4] = offset-5;
          this->length_send = offset;
          // check transaction user input
          CHECK_AND_ASSERT_THROW_MES(this->exchange_wait_on_input() == 0, "Transaction denied on device.");
          #ifdef DEBUG_HWDEVICE
          log_hexbuffer("Prehash AKV input", (char*)&this->buffer_recv[64], 3*32);
          #endif
        }

        // ======   C[], message, proof======
        C_offset = kv_offset;
        for (i = 0; i < outputs_size; i++) {
          offset = set_command_header(INS_VALIDATE, 0x03, i+1);
          //options
          this->buffer_send[offset] = 0x80 ;
          offset += 1;
          //C
          memmove(this->buffer_send+offset, data+C_offset,32);
          offset += 32;
          C_offset += 32;

          this->buffer_send[4] = offset-5;
          this->length_send = offset;
          this->exchange();

        }

        offset = set_command_header_noopt(INS_VALIDATE, 0x03, i+1);
        //message
        memmove(this->buffer_send+offset, hashes[0].bytes,32);
        offset += 32;
        //proof
        memmove(this->buffer_send+offset,  hashes[2].bytes,32);
        offset += 32;

        this->buffer_send[4] = offset-5;
        this->length_send = offset;
        this->exchange();

        memmove(prehash.bytes, this->buffer_recv,  32);

        #ifdef DEBUG_HWDEVICE
        hw::ledger::check32("mlsag_prehash", "prehash", (char*)prehash_x.bytes, (char*)prehash.bytes);
        #endif

        return true;
    }


    bool device_ledger::mlsag_prepare(const rct::key &H, const rct::key &xx,
                                     rct::key &a, rct::key &aG, rct::key &aHP, rct::key &II) {
        AUTO_LOCK_CMD();

        #ifdef DEBUG_HWDEVICE
        const rct::key H_x = H;
        const rct::key xx_x = hw::ledger::decrypt(xx);
        rct::key a_x;
        rct::key aG_x;
        rct::key aHP_x;
        rct::key II_x;
        #endif

        int offset = set_command_header_noopt(INS_MLSAG, 0x01);
        //value H
        memmove(this->buffer_send+offset, H.bytes, 32);
        offset += 32;
        //mask xin
        this->send_secret(xx.bytes, offset);

        this->buffer_send[4] = offset-5;
        this->length_send = offset;
        this->exchange();

        offset = 0;
        this->receive_secret(a.bytes, offset);
        memmove(aG.bytes,  &this->buffer_recv[offset], 32);
        offset +=32;
        memmove(aHP.bytes, &this->buffer_recv[offset], 32);
        offset +=32;
        memmove(II.bytes,  &this->buffer_recv[offset], 32);

        #ifdef DEBUG_HWDEVICE
        a_x = hw::ledger::decrypt(a);

        rct::scalarmultBase(aG_x, a_x);
        rct::scalarmultKey(aHP_x, H_x, a_x);
        rct::scalarmultKey(II_x, H_x, xx_x);
        hw::ledger::check32("mlsag_prepare", "AG", (char*)aG_x.bytes, (char*)aG.bytes);
        hw::ledger::check32("mlsag_prepare", "aHP", (char*)aHP_x.bytes, (char*)aHP.bytes);
        hw::ledger::check32("mlsag_prepare", "II", (char*)II_x.bytes, (char*)II.bytes);
        #endif

        return true;
    }

    bool device_ledger::mlsag_prepare(rct::key &a, rct::key &aG) {
        AUTO_LOCK_CMD();
        int offset;

        #ifdef DEBUG_HWDEVICE
        rct::key a_x;
        rct::key aG_x;
        #endif

        send_simple(INS_MLSAG, 0x01);

        offset = 0;
        this->receive_secret(a.bytes, offset);
        memmove(aG.bytes,  &this->buffer_recv[offset], 32);

        #ifdef DEBUG_HWDEVICE
        a_x = hw::ledger::decrypt(a);
        rct::scalarmultBase(aG_x, a_x);
        hw::ledger::check32("mlsag_prepare", "AG", (char*)aG_x.bytes, (char*)aG.bytes);
        #endif

        return true;
    }

    bool device_ledger::mlsag_hash(const rct::keyV &long_message, rct::key &c) {
        AUTO_LOCK_CMD();
        size_t cnt;

        #ifdef DEBUG_HWDEVICE
        const rct::keyV long_message_x = long_message;
        rct::key c_x;
        this->controle_device->mlsag_hash(long_message_x, c_x);
        #endif

        cnt = long_message.size();
        for (size_t i = 0; i<cnt; i++) {
          int offset = set_command_header(INS_MLSAG, 0x02, i+1);
          //options
          this->buffer_send[offset] =
              (i==(cnt-1))?0x00:0x80;  //last
          offset += 1;
          //msg part
          memmove(this->buffer_send+offset, long_message[i].bytes, 32);
          offset += 32;

          this->buffer_send[4] = offset-5;
          this->length_send = offset;
          this->exchange();
        }

        memmove(c.bytes, &this->buffer_recv[0], 32);

        #ifdef DEBUG_HWDEVICE
        hw::ledger::check32("mlsag_hash", "c", (char*)c_x.bytes, (char*)c.bytes);
        #endif

        return true;
    }

    bool device_ledger::mlsag_sign(const rct::key &c, const rct::keyV &xx, const rct::keyV &alpha, const size_t rows, const size_t dsRows, rct::keyV &ss) {
        AUTO_LOCK_CMD();

        CHECK_AND_ASSERT_THROW_MES(dsRows<=rows, "dsRows greater than rows");
        CHECK_AND_ASSERT_THROW_MES(xx.size() == rows, "xx size does not match rows");
        CHECK_AND_ASSERT_THROW_MES(alpha.size() == rows, "alpha size does not match rows");
        CHECK_AND_ASSERT_THROW_MES(ss.size() == rows, "ss size does not match rows");

        #ifdef DEBUG_HWDEVICE
        const rct::key c_x      = c;
        const rct::keyV xx_x    = hw::ledger::decrypt(xx);
        const rct::keyV alpha_x = hw::ledger::decrypt(alpha);
        const int rows_x        = rows;
        const int dsRows_x      = dsRows;
        rct::keyV ss_x(ss.size());
        this->controle_device->mlsag_sign(c_x, xx_x, alpha_x, rows_x, dsRows_x, ss_x);
        #endif

        for (size_t j = 0; j < dsRows; j++) {
          int offset = set_command_header(INS_MLSAG, 0x03, j+1);
          //options
          this->buffer_send[offset] = 0x00;
          if (j==(dsRows-1)) {
            this->buffer_send[offset]  |= 0x80;  //last
          }
          offset += 1;
          //xx
          this->send_secret(xx[j].bytes, offset);
          //alpa
          this->send_secret(alpha[j].bytes, offset);

          this->buffer_send[4] = offset-5;
          this->length_send = offset;
          this->exchange();

          //ss
          memmove(ss[j].bytes, &this->buffer_recv[0], 32);
        }

        for (size_t j = dsRows; j < rows; j++) {
          sc_mulsub(ss[j].bytes, c.bytes, xx[j].bytes, alpha[j].bytes);
        }

        #ifdef DEBUG_HWDEVICE
        for (size_t j = 0; j < rows; j++) {
           hw::ledger::check32("mlsag_sign", "ss["+std::to_string(j)+"]", (char*)ss_x[j].bytes, (char*)ss[j].bytes);
        }
        #endif

        return true;
    }

    bool device_ledger::close_tx() {
        AUTO_LOCK_CMD();
        send_simple(INS_CLOSE_TX);
        key_map.clear();
        hmac_map.clear();
        this->tx_in_progress = false;
        this->unlock();
        return true;
    }

    /* ---------------------------------------------------------- */

    static device_ledger *legder_device = NULL;
    void register_all(std::map<std::string, std::unique_ptr<device>> &registry) {
      if (!legder_device) {
        legder_device = new device_ledger();
        legder_device->set_name("Ledger");
      }
      registry.insert(std::make_pair("Ledger", std::unique_ptr<device>(legder_device)));
    }
  
  #else //WITH_DEVICE_LEDGER
  
    void register_all(std::map<std::string, std::unique_ptr<device>> &registry) {
    }

  #endif //WITH_DEVICE_LEDGER

  }
}

