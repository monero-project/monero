// Copyright (c) 2014, The Monero Project
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
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

/*!
 * \file ipc_util.h
 * \brief Utility functions for both client and server
 */

#ifndef IPC_UTIL_H
#define IPC_UTIL_H

#include <stdint.h>

namespace IPC
{
  // A bunch of response statuses and error codes
  // Must start at 0, and be contiguous, for status_strings
  enum {
    STATUS_OK = 0,
    STATUS_CORE_BUSY,
    STATUS_WRONG_ADDRESS,
    STATUS_MINING_NOT_STARTED,
    STATUS_WRONG_BLOCK_ID_LENGTH,
    STATUS_INTERNAL_ERROR,
    STATUS_INVALID_TX,
    STATUS_TX_VERIFICATION_FAILED,
    STATUS_TX_NOT_RELAYED,
    STATUS_RANDOM_OUTS_FAILED,
    STATUS_MINING_NOT_STOPPED,
    STATUS_NOT_MINING,
    STATUS_INVALID_LOG_LEVEL,
    STATUS_ERROR_STORING_BLOCKCHAIN,
    STATUS_HEIGHT_TOO_BIG,
    STATUS_RESERVE_SIZE_TOO_BIG,
    STATUS_BLOCK_NOT_FOUND,
    STATUS_WRONG_TX_ID_LENGTH,
    STATUS_TX_NOT_FOUND,
    STATUS_WRONG_KEY_IMAGE_LENGTH,
    STATUS_BAD_JSON_SYNTAX,
    STATUS_BAD_IP_ADDRESS,
    STATUS_RESTRICTED_COMMAND,
    STATUS_NOT_IMPLEMENTED,

    NUM_STATUS_CODES
  };

  const char *get_status_string(int code);

  inline void write32be(uint8_t *ptr, uint32_t value) {
    *ptr++ = (value >> 24) & 0xff;
    *ptr++ = (value >> 16) & 0xff;
    *ptr++ = (value >> 8) & 0xff;
    *ptr++ = (value >> 0) & 0xff;
  }
  inline uint32_t read32be(const uint8_t *ptr) {
    uint32_t value = 0;
    value = (value << 8) | *ptr++;
    value = (value << 8) | *ptr++;
    value = (value << 8) | *ptr++;
    value = (value << 8) | *ptr++;
    return value;
  }
  inline void write64be(uint8_t *ptr, uint64_t value) {
    *ptr++ = (value >> 56) & 0xff;
    *ptr++ = (value >> 48) & 0xff;
    *ptr++ = (value >> 40) & 0xff;
    *ptr++ = (value >> 32) & 0xff;
    *ptr++ = (value >> 24) & 0xff;
    *ptr++ = (value >> 16) & 0xff;
    *ptr++ = (value >> 8) & 0xff;
    *ptr++ = (value >> 0) & 0xff;
  }
  inline uint64_t read64be(const uint8_t *ptr) {
    uint64_t value = 0;
    value = (value << 8) | *ptr++;
    value = (value << 8) | *ptr++;
    value = (value << 8) | *ptr++;
    value = (value << 8) | *ptr++;
    value = (value << 8) | *ptr++;
    value = (value << 8) | *ptr++;
    value = (value << 8) | *ptr++;
    value = (value << 8) | *ptr++;
    return value;
  }

}

#endif
