// Copyright (c) 2014-2016, The Monero Project
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
 * \file ipc_util.cpp
 * \brief Utility functions for both client and server
 */

#include "ipc_util.h"

namespace
{
  const char * const status_strings[IPC::NUM_STATUS_CODES] = {
    "OK",
    "core busy",
    "wrong address",
    "mining not started",
    "wrong block id length",
    "internal error",
    "invalid tx",
    "tx verification failed",
    "tx not relayed",
    "random-outs failed",
    "mining not stopped",
    "not mining",
    "invalid log level",
    "error storing blockchain",
    "height too big",
    "reserve size too big",
    "block not found",
    "wrong txid length",
    "tx not found",
    "wrong key image length",
    "bad JSON syntax",
    "bad IP address",
    "restricted command",
    "not implemented",
  };
}

namespace IPC
{
  const char *get_status_string(int code)
  {
    if (code == -1)
      return "communication error";
    if (code >= 0 && code < NUM_STATUS_CODES)
      return status_strings[code];
    return "unknown error code";
  }

}

