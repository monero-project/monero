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

#include "misc_log_ex.h"
#include "log.hpp"

namespace hw {

  #undef MONERO_DEFAULT_LOG_CATEGORY
  #define MONERO_DEFAULT_LOG_CATEGORY "device"

  void buffer_to_str(char *to_buff,  size_t to_len, const char *buff, size_t len) {
    CHECK_AND_ASSERT_THROW_MES(to_len > (len*2), "destination buffer too short. At least" << (len*2+1) << " bytes required");
    for (size_t i=0; i<len; i++) {
      sprintf(to_buff+2*i, "%.02x", (unsigned char)buff[i]);
    }
  }

  void log_hexbuffer(const std::string &msg,  const char* buff, size_t len) {
    char logstr[1025];
    buffer_to_str(logstr, sizeof(logstr),  buff, len);
    MDEBUG(msg<< ": " << logstr);
  }

  void log_message(const std::string &msg, const std::string &info ) {
    MDEBUG(msg << ": " << info);
  }


  #ifdef WITH_DEVICE_LEDGER    
    namespace ledger {
    
    #undef MONERO_DEFAULT_LOG_CATEGORY
    #define MONERO_DEFAULT_LOG_CATEGORY "device.ledger"

    
    #ifdef DEBUG_HWDEVICE
    extern crypto::secret_key dbg_viewkey;
    extern crypto::secret_key dbg_spendkey;


    void decrypt(char* buf, size_t len) {
      #ifdef IODUMMYCRYPT_HWDEVICE
      size_t i;
      if (len == 32) {
        //view key?
        for (i = 0; i<32; i++) {
          if (buf[i] != 0) break;
        }
        if (i == 32) {
          memmove(buf, hw::ledger::dbg_viewkey.data, 32);
          return;
        }
        //spend key?
        for (i = 0; i<32; i++) {
          if (buf[i] != (char)0xff) break;
        }
        if (i == 32) {
          memmove(buf, hw::ledger::dbg_spendkey.data, 32);
          return;
        }
      }
      //std decrypt: XOR.55h
      for (i = 0; i<len;i++) {
          buf[i] ^= 0x55;
        }
      #endif
    }

    crypto::key_derivation decrypt(const crypto::key_derivation &derivation) {
       crypto::key_derivation x = derivation;
       decrypt(x.data, 32);
       return x;
    }

    cryptonote::account_keys decrypt(const cryptonote::account_keys& keys) {
       cryptonote::account_keys x = keys;
       decrypt(x.m_view_secret_key.data, 32);
       decrypt(x.m_spend_secret_key.data, 32);
       return x;
    }


    crypto::secret_key decrypt(const crypto::secret_key &sec) {
       crypto::secret_key  x = sec;
       decrypt(x.data, 32);
       return x;
    }

    rct::key  decrypt(const rct::key &sec)  {
         rct::key  x = sec;
       decrypt((char*)x.bytes, 32);
       return x;
    }

    crypto::ec_scalar decrypt(const crypto::ec_scalar &res)  {
       crypto::ec_scalar  x = res;
       decrypt((char*)x.data, 32);
       return x;
    }

    rct::keyV decrypt(const rct::keyV &keys) {
        rct::keyV x ;
        x.reserve(keys.size());
        for (unsigned int j = 0; j<keys.size(); j++) {
            x.push_back(decrypt(keys[j]));
        }
        return x;
    }

    static void check(const std::string &msg, const std::string &info, const char *h, const char *d, size_t len, bool crypted) {
      char dd[32];
      char logstr[128];

      CHECK_AND_ASSERT_THROW_MES(len <= sizeof(dd), "invalid len");
      memmove(dd,d,len);
      if (crypted) {
        CHECK_AND_ASSERT_THROW_MES(len<=32, "encrypted data greater than 32");
        decrypt(dd,len);
      }

      if (memcmp(h,dd,len)) {
          log_message("ASSERT EQ FAIL",  msg + ": "+ info );
          log_hexbuffer("    host  ", h, len);
          log_hexbuffer("    device", dd, len);

      } else {
        buffer_to_str(logstr, 128,  dd, len);
        log_message("ASSERT EQ OK",  msg + ": "+ info + ": "+ std::string(logstr) );
      }
    }

    void check32(const std::string &msg, const std::string &info, const char *h, const char *d, bool crypted) {
      check(msg, info, h, d, 32, crypted);
    }

    void check8(const std::string &msg, const std::string &info, const char *h, const char *d, bool crypted) {
      check(msg, info, h, d, 8, crypted);
    }
    #endif

  }
  #endif //WITH_DEVICE_LEDGER    

}
