// Copyright (c) 2014-2022, The Monero Project
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

#pragma  once 


#define CORE_RPC_ERROR_CODE_WRONG_PARAM           -1
#define CORE_RPC_ERROR_CODE_TOO_BIG_HEIGHT        -2
#define CORE_RPC_ERROR_CODE_TOO_BIG_RESERVE_SIZE  -3
#define CORE_RPC_ERROR_CODE_WRONG_WALLET_ADDRESS  -4
#define CORE_RPC_ERROR_CODE_INTERNAL_ERROR        -5
#define CORE_RPC_ERROR_CODE_WRONG_BLOCKBLOB       -6
#define CORE_RPC_ERROR_CODE_BLOCK_NOT_ACCEPTED    -7
#define CORE_RPC_ERROR_CODE_CORE_BUSY             -9
#define CORE_RPC_ERROR_CODE_WRONG_BLOCKBLOB_SIZE  -10
#define CORE_RPC_ERROR_CODE_UNSUPPORTED_RPC       -11
#define CORE_RPC_ERROR_CODE_MINING_TO_SUBADDRESS  -12
#define CORE_RPC_ERROR_CODE_REGTEST_REQUIRED      -13
#define CORE_RPC_ERROR_CODE_PAYMENT_REQUIRED      -14
#define CORE_RPC_ERROR_CODE_INVALID_CLIENT        -15
#define CORE_RPC_ERROR_CODE_PAYMENT_TOO_LOW       -16
#define CORE_RPC_ERROR_CODE_DUPLICATE_PAYMENT     -17
#define CORE_RPC_ERROR_CODE_STALE_PAYMENT         -18
#define CORE_RPC_ERROR_CODE_RESTRICTED            -19
#define CORE_RPC_ERROR_CODE_UNSUPPORTED_BOOTSTRAP -20
#define CORE_RPC_ERROR_CODE_PAYMENTS_NOT_ENABLED  -21

static inline const char *get_rpc_server_error_message(int64_t code)
{
  switch (code)
  {
    case CORE_RPC_ERROR_CODE_WRONG_PARAM: return "Invalid parameter";
    case CORE_RPC_ERROR_CODE_TOO_BIG_HEIGHT: return "Height is too large";
    case CORE_RPC_ERROR_CODE_TOO_BIG_RESERVE_SIZE: return "Reserve size is too large";
    case CORE_RPC_ERROR_CODE_WRONG_WALLET_ADDRESS: return "Wrong wallet address";
    case CORE_RPC_ERROR_CODE_INTERNAL_ERROR: return "Internal error";
    case CORE_RPC_ERROR_CODE_WRONG_BLOCKBLOB: return "Wrong block blob";
    case CORE_RPC_ERROR_CODE_BLOCK_NOT_ACCEPTED: return "Block not accepted";
    case CORE_RPC_ERROR_CODE_CORE_BUSY: return "Core is busy";
    case CORE_RPC_ERROR_CODE_WRONG_BLOCKBLOB_SIZE: return "Wrong block blob size";
    case CORE_RPC_ERROR_CODE_UNSUPPORTED_RPC: return "Unsupported RPC";
    case CORE_RPC_ERROR_CODE_MINING_TO_SUBADDRESS: return "Mining to subaddress is not supported";
    case CORE_RPC_ERROR_CODE_REGTEST_REQUIRED: return "Regtest mode required";
    case CORE_RPC_ERROR_CODE_PAYMENT_REQUIRED: return "Payment required";
    case CORE_RPC_ERROR_CODE_INVALID_CLIENT: return "Invalid client";
    case CORE_RPC_ERROR_CODE_PAYMENT_TOO_LOW: return "Payment too low";
    case CORE_RPC_ERROR_CODE_DUPLICATE_PAYMENT: return "Duplicate payment";
    case CORE_RPC_ERROR_CODE_STALE_PAYMENT: return "Stale payment";
    case CORE_RPC_ERROR_CODE_RESTRICTED: return "Parameters beyond restricted allowance";
    case CORE_RPC_ERROR_CODE_UNSUPPORTED_BOOTSTRAP: return "Command is unsupported in bootstrap mode";
    case CORE_RPC_ERROR_CODE_PAYMENTS_NOT_ENABLED: return "Payments not enabled";
    default: MERROR("Unknown error: " << code); return "Unknown error";
  }
}

