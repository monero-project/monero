// Copyright (c) 2014-2018, The Monero Project
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

#pragma once

#include <string>
#include <boost/uuid/uuid.hpp>

#define CRYPTONOTE_DNS_TIMEOUT_MS                       20000

#define CRYPTONOTE_MAX_BLOCK_NUMBER                     500000000
#define CRYPTONOTE_MAX_BLOCK_SIZE                       500000000  // block header blob limit, never used!
#define CRYPTONOTE_GETBLOCKTEMPLATE_MAX_BLOCK_SIZE	196608 //size of block (bytes) that is the maximum that miners will produce
#define CRYPTONOTE_MAX_TX_SIZE                          1000000000
#define CRYPTONOTE_PUBLIC_ADDRESS_TEXTBLOB_VER          0
#define CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW            30
#define CURRENT_TRANSACTION_VERSION                     2
#define CURRENT_BLOCK_MAJOR_VERSION                     6
#define CURRENT_BLOCK_MINOR_VERSION                     6
#define CURRENT_BLOCK_MAJOR_VERSION_TESTNET             7
#define CURRENT_BLOCK_MINOR_VERSION_TESTNET             7
#define CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT_V2           60*10
#define CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE             10

#define STAKING_REQUIREMENT_LOCK_BLOCKS                 (30*24*31)
#define STAKING_RELOCK_WINDOW_BLOCKS                    (30*6)

#define BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW               11

// MONEY_SUPPLY - total number coins to be generated
#define MONEY_SUPPLY                                    ((uint64_t)(-1))
#define EMISSION_SPEED_FACTOR_PER_MINUTE                (20)
#define FINAL_SUBSIDY_PER_MINUTE                        ((uint64_t)300000000000) // 3 * pow(10, 11)
#define EMISSION_LINEAR_BASE                            ((uint64_t)(1) << 58)
#define YEARLY_INFLATION_INVERSE                        200 // 0.5% yearly inflation, inverted for integer division
#define EMISSION_SUPPLY_MULTIPLIER                      19
#define EMISSION_SUPPLY_DIVISOR                         10
#define EMISSION_DIVISOR                                2000000

#define CRYPTONOTE_REWARD_BLOCKS_WINDOW                 100
#define CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2    60000 //size of block (bytes) after which reward for block calculated using block size
#define CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1    20000 //size of block (bytes) after which reward for block calculated using block size - before first fork
#define CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V5    300000 //size of block (bytes) after which reward for block calculated using block size - second change, from v5
#define CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE          600
#define CRYPTONOTE_DISPLAY_DECIMAL_POINT                9
// COIN - number of smallest units in one coin
#define COIN                                            ((uint64_t)1000000000) // pow(10, 9)

#define FEE_PER_KB_OLD                                  ((uint64_t)10000000000) // pow(10, 10)
#define FEE_PER_KB                                      ((uint64_t)2000000000) // 2 * pow(10, 9)
#define DYNAMIC_FEE_PER_KB_BASE_FEE                     ((uint64_t)2000000000) // 2 * pow(10,9)
#define DYNAMIC_FEE_PER_KB_BASE_BLOCK_REWARD            ((uint64_t)10000000000000) // 10 * pow(10,12)
#define DYNAMIC_FEE_PER_KB_BASE_FEE_V5                  ((uint64_t)2000000000 * (uint64_t)CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 / CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V5)

#define ORPHANED_BLOCKS_MAX_COUNT                       100


#define DIFFICULTY_TARGET_V2                            120  // seconds
#define DIFFICULTY_WINDOW_V2                            60
#define DIFFICULTY_LAG                                  15  // !!!
#define DIFFICULTY_CUT                                  60  // timestamps to cut after sorting
#define DIFFICULTY_BLOCKS_COUNT_V2                      (DIFFICULTY_WINDOW_V2 + 1) // added +1 to make N=N


#define CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS_V2   DIFFICULTY_TARGET_V2 * CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS
#define CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS       1


#define DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN             DIFFICULTY_TARGET_V2 //just alias; used by tests


#define BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT          10000  //by default, blocks ids count in synchronizing
#define BLOCKS_SYNCHRONIZING_DEFAULT_COUNT_PRE_V4       100    //by default, blocks count in blocks downloading
#define BLOCKS_SYNCHRONIZING_DEFAULT_COUNT              20     //by default, blocks count in blocks downloading

#define CRYPTONOTE_MEMPOOL_TX_LIVETIME                    (86400*3) //seconds, three days
#define CRYPTONOTE_MEMPOOL_TX_FROM_ALT_BLOCK_LIVETIME     604800 //seconds, one week

#define COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT           1000

#define P2P_LOCAL_WHITE_PEERLIST_LIMIT                  1000
#define P2P_LOCAL_GRAY_PEERLIST_LIMIT                   5000

#define P2P_DEFAULT_CONNECTIONS_COUNT                   8
#define P2P_DEFAULT_HANDSHAKE_INTERVAL                  60           //secondes
#define P2P_DEFAULT_PACKET_MAX_SIZE                     50000000     //50000000 bytes maximum packet size
#define P2P_DEFAULT_PEERS_IN_HANDSHAKE                  250
#define P2P_DEFAULT_CONNECTION_TIMEOUT                  5000       //5 seconds
#define P2P_DEFAULT_PING_CONNECTION_TIMEOUT             2000       //2 seconds
#define P2P_DEFAULT_INVOKE_TIMEOUT                      60*2*1000  //2 minutes
#define P2P_DEFAULT_HANDSHAKE_INVOKE_TIMEOUT            5000       //5 seconds
#define P2P_DEFAULT_WHITELIST_CONNECTIONS_PERCENT       70
#define P2P_DEFAULT_ANCHOR_CONNECTIONS_COUNT            2

#define P2P_FAILED_ADDR_FORGET_SECONDS                  (60*60)     //1 hour
#define P2P_IP_BLOCKTIME                                (60*60*24)  //24 hour
#define P2P_IP_FAILS_BEFORE_BLOCK                       10
#define P2P_IDLE_CONNECTION_KILL_INTERVAL               (5*60) //5 minutes

#define P2P_SUPPORT_FLAG_FLUFFY_BLOCKS                  0x01
#define P2P_SUPPORT_FLAGS                               P2P_SUPPORT_FLAG_FLUFFY_BLOCKS

#define ALLOW_DEBUG_COMMANDS

#define CRYPTONOTE_NAME                         "loki"
#define CRYPTONOTE_POOLDATA_FILENAME            "poolstate.bin"
#define CRYPTONOTE_BLOCKCHAINDATA_FILENAME      "data.mdb"
#define CRYPTONOTE_BLOCKCHAINDATA_LOCK_FILENAME "lock.mdb"
#define P2P_NET_DATA_FILENAME                   "p2pstate.bin"
#define MINER_CONFIG_FILE_NAME                  "miner_conf.json"

#define THREAD_STACK_SIZE                       5 * 1024 * 1024

#define HF_VERSION_DYNAMIC_FEE                  4
#define HF_VERSION_MIN_MIXIN_9                  7
#define HF_VERSION_ENFORCE_RCT                  6

#define PER_KB_FEE_QUANTIZATION_DECIMALS        8

#define HASH_OF_HASHES_STEP                     256

#define DEFAULT_TXPOOL_MAX_SIZE                 648000000ull // 3 days at 300000, in bytes

// New constants are intended to go here
namespace config
{
  uint64_t const DEFAULT_FEE_ATOMIC_XMR_PER_KB = 500; // Just a placeholder!  Change me!
  uint8_t const FEE_CALCULATION_MAX_RETRIES = 10;
  uint64_t const DEFAULT_DUST_THRESHOLD = ((uint64_t)2000000000); // 2 * pow(10, 9)
  uint64_t const BASE_REWARD_CLAMP_THRESHOLD = ((uint64_t)100000000); // pow(10, 8)
  std::string const P2P_REMOTE_DEBUG_TRUSTED_PUB_KEY = "0000000000000000000000000000000000000000000000000000000000000000";

  uint64_t const CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX = 114;
  uint64_t const CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX = 115;
  uint64_t const CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX = 116;
  uint16_t const P2P_DEFAULT_PORT = 22022;
  uint16_t const RPC_DEFAULT_PORT = 22023;
  uint16_t const ZMQ_RPC_DEFAULT_PORT = 22024;
  boost::uuids::uuid const NETWORK_ID = { {
        0x46 ,0x61, 0x72, 0x62 ,0x61, 0x75, 0x74, 0x69, 0x2a, 0x4c, 0x61, 0x75, 0x66, 0x65, 0x79
    } }; // Bender's nightmare
  std::string const GENESIS_TX = "021e01ff000380808d93f5d771027c4fd4553bc9886f1f49e3f76d945bf71e8632a94e6c177b19cbc780e7e6bdb48080b4ccd4dfc60302c8b9f6461f58ef3f2107e577c7425d06af584a1c7482bf19060e84059c98b4c3808088fccdbcc32302732b53b0b0db706fcc3087074fb4b786da5ab72b2065699f9453448b0db27f892101ed71f2ce3fc70d7b2036f8a4e4b3fb75c66c12184b55a908e7d1a1d6995566cf00";
  uint32_t const GENESIS_NONCE = 1022201;

  std::string const GOVERNANCE_WALLET_ADDRESS = "LCFxT37LAogDn1jLQKf4y7aAqfi21DjovX9qyijaLYQSdrxY1U5VGcnMJMjWrD9RhjeK5Lym67wZ73uh9AujXLQ1RKmXEyL";
  
  namespace testnet
  {
    uint64_t const CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX = 156;
    uint64_t const CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX = 157;
    uint64_t const CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX = 158;
    uint16_t const P2P_DEFAULT_PORT = 38150;
    uint16_t const RPC_DEFAULT_PORT = 38151;
    uint16_t const ZMQ_RPC_DEFAULT_PORT = 38152;
    boost::uuids::uuid const NETWORK_ID = { {
        0x44, 0xb3, 0xe9, 0xef, 0x80, 0xc0, 0x08, 0x19, 0xb4, 0x2c, 0x83, 0xec, 0xef, 0x59, 0x24, 0xfd
      } }; // Bender's daydream
    std::string const GENESIS_TX = "021e01ff000380808d93f5d77102036a65eb633efc8d73133cce56de1fbc4478f62a2d3f74f4e18c6ed14b0212358080b4ccd4dfc6030217f5a14c129669199643d3890b0c575d4e6048807c1ae7b2fe9468f755383e17808088fccdbcc32302d1137109f8fa24e3cfb224dc0ce594eabf9e976640a9e6ca78656936677d9af82101ab8b15dea6402670113dc2ff59653d3b2e859e2d56af7f7c2773f9dcac8385d200";
    uint32_t const GENESIS_NONCE = 10001;

    std::string const GOVERNANCE_WALLET_ADDRESS = "T6U5dnmhyw1XsDryZUZL4hAwkNDYahFLEbhyfipTJ2ZZLV1Epm5zHTf6SQzxre9V8R9zjSPN6WWiT9adBQYt7Wut2xjon6dgT";
  }

  namespace stagenet
  {
    uint64_t const CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX = 24;
    uint64_t const CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX = 25;
    uint64_t const CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX = 36;
    uint16_t const P2P_DEFAULT_PORT = 38153;
    uint16_t const RPC_DEFAULT_PORT = 38154;
    uint16_t const ZMQ_RPC_DEFAULT_PORT = 38155;
    boost::uuids::uuid const NETWORK_ID = { {
        0xbb ,0x37, 0x9B, 0x22 , 0x0A, 0x66 , 0x69, 0x1E, 0x09, 0xB2, 0x97, 0x8A, 0xCC, 0xA1, 0xDF, 0x9C
      } }; // Beep Boop
    std::string const GENESIS_TX = "021e01ff000380808d93f5d771027e4490431900c66a6532917ad9e6a1de634a209b708f653097e7b48efc1238c68080b4ccd4dfc60302ba19a224e6474371f9161b2e6271a36d060cbdc2e479ad78f1be64c56576fa07808088fccdbcc32302bccf9c13ba1b5bb02638de6e557acdd46bf48953e42cf98a12d2ad2900cc316121018fc6728d9e3c062d3afae3b2317998d2abee1e12f51271ba1c0d3cdd236b81d200";
    uint32_t const GENESIS_NONCE = 10002;

    std::string const GOVERNANCE_WALLET_ADDRESS = "59f7FCwYMiwMnFr8HwsnfJ2hK3DYB1tryhjsfmXqEBJojKyqKeNWoaDaZaauoZPiZHUYp2wJuy5s9H96qy4q9xUVCXXHmTU";
  }
}

namespace cryptonote
{
  enum network_type : uint8_t
  {
    MAINNET = 0,
    TESTNET,
    STAGENET,
    FAKECHAIN,
    UNDEFINED = 255
  };
}
