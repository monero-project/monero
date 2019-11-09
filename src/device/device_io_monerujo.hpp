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

#if defined(HAVE_MONERUJO)

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief LedgerFind - find Ledger Device and return it's name
 * @param buffer     - buffer for name of found device
 * @param len        - length of buffer
 * @return  0 - success
 *         -1 - no device connected / found
 *         -2 - JVM not found
 */
int LedgerFind(char *buffer, size_t len);

/**
 * @brief LedgerExchange - exchange data with Ledger Device
 * @param command        - buffer for data to send
 * @param cmd_len        - length of send to send
 * @param response       - buffer for received data
 * @param max_resp_len   - size of receive buffer
 *
 * @return length of received data in response or -1 if error
 */
int  LedgerExchange(unsigned char *command, unsigned int cmd_len, unsigned char *response, unsigned int max_resp_len);

#ifdef __cplusplus
}
#endif

#include "device_io.hpp"

#pragma once

namespace hw {
  namespace io {
    class device_io_monerujo: device_io {
    public:
      device_io_monerujo() {};
      ~device_io_monerujo() {};

      void init() {};
      void release() {};

      void connect(void *params) {};
      void disconnect() {};
      bool connected() const {return true;}; // monerujo is always connected before it gets here

      // returns number of bytes read or -1 on error
      int  exchange(unsigned char *command, unsigned int cmd_len, unsigned char *response, unsigned int max_resp_len, bool user_input) {
        return LedgerExchange(command, cmd_len, response, max_resp_len);
      }
    };
  };
};

#endif //#if defined(HAVE_MONERUJO)
