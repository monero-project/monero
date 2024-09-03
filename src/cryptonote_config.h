// Copyright (c) 2014-2019, The Monero Project
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
#include <stdexcept>
#include <chrono>

#define CRYPTONOTE_DNS_TIMEOUT_MS                       20000

#define CRYPTONOTE_MAX_BLOCK_NUMBER                     500000000
#define CRYPTONOTE_GETBLOCKTEMPLATE_MAX_BLOCK_SIZE	    196608 //size of block (bytes) that is the maximum that miners will produce
#define CRYPTONOTE_MAX_TX_SIZE                          1000000
#define CRYPTONOTE_MAX_TX_PER_BLOCK                     0x10000000
#define CRYPTONOTE_PUBLIC_ADDRESS_TEXTBLOB_VER          0
#define CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW            60
#define CURRENT_TRANSACTION_VERSION                     3
#define CURRENT_BLOCK_MAJOR_VERSION                     1
#define CURRENT_BLOCK_MINOR_VERSION                     0
#define CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT              500
#define CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE             10

#define DEFAULT_TX_MIXIN                                15

#define BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW               60
#define ORACLE_EON_PUBLISHERS                           11
#define MIN_ORACLE_EON_RATERS                           11
//Service nodes

#define SERVICE_NODE_VERSION                            5

#define STAKING_REQUIREMENT_LOCK_BLOCKS_EXCESS          20
#define STAKING_RELOCK_WINDOW_BLOCKS                    (30*6)
#define STAKING_PORTIONS                                UINT64_C(0xfffffffffffffffc)
#define STAKING_AUTHORIZATION_EXPIRATION_WINDOW         (60*60*24*7*2)  // 2 weeks
#define MAX_NUMBER_OF_CONTRIBUTORS                      4
#define MAX_NUMBER_OF_CONTRIBUTORS_V2                   100
#define MAX_NUMBER_OF_CONTRIBUTORS_V3                   1000

#define MAX_OPERATOR_V12                                35000
#define MAX_POOL_STAKERS_V12                            65000
#define MIN_OPERATOR_V12                                10000
#define MIN_POOL_STAKERS_V12                            100

#define MIN_PORTIONS                                    (STAKING_PORTIONS / MAX_NUMBER_OF_CONTRIBUTORS)
#define MIN_PORTIONS_V2                                 (STAKING_PORTIONS / MAX_NUMBER_OF_CONTRIBUTORS_V2)
#define MIN_PORTIONS_V3                                 (STAKING_PORTIONS / MAX_NUMBER_OF_CONTRIBUTORS_V3)
#define MEMPOOL_PRUNE_DEREGISTER_LIFETIME               (24 * 60 * 60) // seconds, 2 hours

static_assert(STAKING_PORTIONS % MAX_NUMBER_OF_CONTRIBUTORS == 0, "Use a multiple of four, so that it divides easily by max number of contributors.");
static_assert(STAKING_PORTIONS % 2 == 0, "Use a multiple of two, so that it divides easily by two contributors.");
static_assert(STAKING_PORTIONS % 3 == 0, "Use a multiple of three, so that it divides easily by three contributors.");

#define UPTIME_PROOF_BUFFER_IN_SECONDS                  (5*60)
#define UPTIME_PROOF_FREQUENCY_IN_SECONDS               (60*60)
#define UPTIME_PROOF_MAX_TIME_IN_SECONDS                (UPTIME_PROOF_FREQUENCY_IN_SECONDS * 2 + UPTIME_PROOF_BUFFER_IN_SECONDS)
#define UPTIME_PROOF_MAX_TIME_IN_SECONDS_V2             (UPTIME_PROOF_FREQUENCY_IN_SECONDS * 48 + UPTIME_PROOF_BUFFER_IN_SECONDS)


// MONEY_SUPPLY - total number coins to be generated
#define MONEY_SUPPLY                                    ((uint64_t)840000000000)
#define TRITON_SWAP                                     ((uint64_t)107695988100)

#define BURN_1                                          ((uint64_t)70000000000) // 7,000,000 XEQ BURN
#define MINT_BRIDGE                                     ((uint64_t)167195840000) //16,719,584 XEQ MINT for Bridge
#define BURN_2                                          ((uint64_t)40000000000) // 7,000,000 XEQ BURN

#define XEQ_REQ                                         ((uint64_t)10000)
#define CORP_MINT                                       ((uint64_t)80000000000)
#define NEW_XEQ_BRIDGE                                  ((uint64_t)20000000000)

#define EMISSION_SPEED_FACTOR_PER_MINUTE                (20)
#define FINAL_SUBSIDY_PER_MINUTE                        ((uint64_t)208207)

#define CRYPTONOTE_REWARD_BLOCKS_WINDOW                 100
#define CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2    80000 //size of block (bytes) after which reward for block calculated using block size
#define CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1    90000 //size of block (bytes) after which reward for block calculated using block size - before first fork
#define CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V5    1000000 //size of block (bytes) after which reward for block calculated using block size - second change, from v5
#define CRYPTONOTE_LONG_TERM_BLOCK_WEIGHT_WINDOW_SIZE   100000 // size in blocks of the long term block weight median window
#define CRYPTONOTE_SHORT_TERM_BLOCK_WEIGHT_SURGE_FACTOR 50
#define CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE          600
#define CRYPTONOTE_DISPLAY_DECIMAL_POINT                4

//LongTermWeight
#define CRYPTONOTE_LONG_TERM_BLOCK_WEIGHT_WINDOW_SIZE   100000 // size in blocks of the long term block weight median window
#define CRYPTONOTE_SHORT_TERM_BLOCK_WEIGHT_SURGE_FACTOR 50

// COIN - number of smallest units in one coin
#define COIN                                            ((uint64_t)10000) // pow(10, 12)

#define FEE_PER_KB_OLD                                  ((uint64_t)1) // pow(10, 10)
#define FEE_PER_KB                                      ((uint64_t)20) // 2 * pow(10, 9)
#define FEE_PER_BYTE                                    ((uint64_t)3)
#define DYNAMIC_FEE_PER_KB_BASE_FEE                     ((uint64_t)2) // 2 * pow(10,9)
#define DYNAMIC_FEE_PER_KB_BASE_BLOCK_REWARD            ((uint64_t)1) // 10 * pow(10,12)
#define DYNAMIC_FEE_PER_KB_BASE_FEE_V5                  ((uint64_t)2 * (uint64_t)CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 / CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V5)
#define DYNAMIC_FEE_REFERENCE_TRANSACTION_WEIGHT        ((uint64_t)3)

#define ORPHANED_BLOCKS_MAX_COUNT                       100

#define DIFFICULTY_TARGET_V2                            180  // seconds
#define DIFFICULTY_TARGET_V3                            120  // seconds
#define DIFFICULTY_TARGET_V1                            60  // seconds - before first fork
#define DIFFICULTY_WINDOW                               720 // blocks
#define DIFFICULTY_LAG                                  15  // !!!
#define DIFFICULTY_CUT                                  60  // timestamps to cut after sorting
#define DIFFICULTY_BLOCKS_COUNT                         DIFFICULTY_WINDOW + DIFFICULTY_LAG
#define DIFFICULTY_WINDOW_V3                            60 // blocks

#define DIFFICULTY_BLOCKS_COUNT_V3                      DIFFICULTY_WINDOW_V3 + 1

#define CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS_V1   DIFFICULTY_TARGET_V1 * CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS
#define CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS_V2   DIFFICULTY_TARGET_V2 * CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS
#define CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS_V3   DIFFICULTY_TARGET_V3 * CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS
#define CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS       1

#define DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN             DIFFICULTY_TARGET_V1 //just alias; used by tests

#define BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT          10000  //by default, blocks ids count in synchronizing
#define BLOCKS_SYNCHRONIZING_DEFAULT_COUNT_PRE_V4       100    //by default, blocks count in blocks downloading
#define BLOCKS_SYNCHRONIZING_DEFAULT_COUNT              20     //by default, blocks count in blocks downloading
#define BLOCKS_SYNCHRONIZING_MAX_COUNT                  2048   //must be a power of 2, greater than 128, equal to SEEDHASH_EPOCH_BLOCKS

#define CRYPTONOTE_MEMPOOL_TX_LIVETIME                    (86400*3) //seconds, three days
#define CRYPTONOTE_MEMPOOL_TX_FROM_ALT_BLOCK_LIVETIME     604800 //seconds, one week

#define CRYPTONOTE_DANDELIONPP_STEMS              2 // number of outgoing stem connections per epoch
#define CRYPTONOTE_DANDELIONPP_FLUFF_PROBABILITY 10 // out of 100
#define CRYPTONOTE_DANDELIONPP_MIN_EPOCH         10 // minutes
#define CRYPTONOTE_DANDELIONPP_EPOCH_RANGE       30 // seconds
#define CRYPTONOTE_DANDELIONPP_FLUSH_AVERAGE      5 // seconds average for poisson distributed fluff flush
#define CRYPTONOTE_DANDELIONPP_EMBARGO_AVERAGE  173 // seconds (see tx_pool.cpp for more info)

// see src/cryptonote_protocol/levin_notify.cpp
#define CRYPTONOTE_NOISE_MIN_EPOCH                      5      // minutes
#define CRYPTONOTE_NOISE_EPOCH_RANGE                    30     // seconds
#define CRYPTONOTE_NOISE_MIN_DELAY                      10     // seconds
#define CRYPTONOTE_NOISE_DELAY_RANGE                    5      // seconds
#define CRYPTONOTE_NOISE_BYTES                          3*1024 // 3 KiB
#define CRYPTONOTE_NOISE_CHANNELS                       2      // Max outgoing connections per zone used for noise/covert sending

#define CRYPTONOTE_MAX_FRAGMENTS                        20 // ~20 * NOISE_BYTES max payload size for covert/noise send

#define COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT           1000
#define COMMAND_RPC_GET_BLOCKS_FAST_MAX_TX_COUNT        20000

#define P2P_LOCAL_WHITE_PEERLIST_LIMIT                  1000
#define P2P_LOCAL_GRAY_PEERLIST_LIMIT                   5000

#define P2P_DEFAULT_CONNECTIONS_COUNT_OUT               8
#define P2P_DEFAULT_CONNECTIONS_COUNT_IN                32
#define P2P_DEFAULT_HANDSHAKE_INTERVAL                  60           //secondes
#define P2P_DEFAULT_PACKET_MAX_SIZE                     50000000     //50000000 bytes maximum packet size
#define P2P_DEFAULT_PEERS_IN_HANDSHAKE                  250
#define P2P_DEFAULT_CONNECTION_TIMEOUT                  5000       //5 seconds
#define P2P_DEFAULT_SOCKS_CONNECT_TIMEOUT               45         // seconds
#define P2P_DEFAULT_PING_CONNECTION_TIMEOUT             5000       //5 seconds
#define P2P_DEFAULT_INVOKE_TIMEOUT                      60*2*1000  //2 minutes
#define P2P_DEFAULT_HANDSHAKE_INVOKE_TIMEOUT            5000       //5 seconds
#define P2P_DEFAULT_WHITELIST_CONNECTIONS_PERCENT       70
#define P2P_DEFAULT_ANCHOR_CONNECTIONS_COUNT            6
#define P2P_DEFAULT_SYNC_SEARCH_CONNECTIONS_COUNT       2
#define P2P_DEFAULT_LIMIT_RATE_UP                       4096       // kB/s
#define P2P_DEFAULT_LIMIT_RATE_DOWN                     16384      // kB/s

#define P2P_FAILED_ADDR_FORGET_SECONDS                  (60*60)     //1 hour
#define P2P_IP_BLOCKTIME                                (60*60*24)  //24 hour
#define P2P_IP_FAILS_BEFORE_BLOCK                       10
#define P2P_IDLE_CONNECTION_KILL_INTERVAL               (5*60) //5 minutes

#define P2P_SUPPORT_FLAG_FLUFFY_BLOCKS                  0x01
#define P2P_SUPPORT_FLAGS                               P2P_SUPPORT_FLAG_FLUFFY_BLOCKS

#define RPC_IP_FAILS_BEFORE_BLOCK                       3

#define CRYPTONOTE_NAME                         "equilibria"
#define CRYPTONOTE_POOLDATA_FILENAME            "poolstate.bin"
#define CRYPTONOTE_BLOCKCHAINDATA_FILENAME      "data.mdb"
#define CRYPTONOTE_BLOCKCHAINDATA_LOCK_FILENAME "lock.mdb"
#define P2P_NET_DATA_FILENAME                   "p2pstate.bin"
#define MINER_CONFIG_FILE_NAME                  "miner_conf.json"

#define THREAD_STACK_SIZE                       10 * 1024 * 1024

#define HF_VERSION_DYNAMIC_FEE                  100
#define HF_VERSION_MIN_MIXIN_4                  4
#define HF_VERSION_MIN_MIXIN_15                 6
#define HF_VERSION_ENFORCE_RCT                  4
#define HF_VERSION_PER_BYTE_FEE                 100
#define HF_VERSION_SMALLER_BP                   6

#define HF_VERSION_LONG_TERM_BLOCK_WEIGHT       100
#define HF_VERSION_MIN_2_OUTPUTS                100
#define HF_VERSION_MIN_V2_COINBASE_TX           100
#define HF_VERSION_SAME_MIXIN                   100
#define HF_VERSION_REJECT_SIGS_IN_COINBASE      100
#define HF_VERSION_ENFORCE_MIN_AGE              100
#define HF_VERSION_EFFECTIVE_SHORT_TERM_MEDIAN_IN_PENALTY 100

#define HF_VERSION_FEE_BURNING                  9

#define PER_KB_FEE_QUANTIZATION_DECIMALS        4

#define HASH_OF_HASHES_STEP                     256

#define DEFAULT_TXPOOL_MAX_WEIGHT               648000000ull // 3 days at 300000, in bytes

#define BULLETPROOF_MAX_OUTPUTS                 16

#define CRYPTONOTE_PRUNING_STRIPE_SIZE          4096 // the smaller, the smoother the increase
#define CRYPTONOTE_PRUNING_LOG_STRIPES          3 // the higher, the more space saved
#define CRYPTONOTE_PRUNING_TIP_BLOCKS           5500 // the smaller, the more space saved
//#define CRYPTONOTE_PRUNING_DEBUG_SPOOF_SEED

// New constants are intended to go here
namespace config
{
    uint64_t const DEFAULT_FEE_ATOMIC_ITA_PER_KB = 500; // Just a placeholder!  Change me!
    uint8_t const FEE_CALCULATION_MAX_RETRIES = 10;
    uint64_t const DEFAULT_DUST_THRESHOLD = ((uint64_t)20); // 2 * pow(10, 9)
    uint64_t const BASE_REWARD_CLAMP_THRESHOLD = ((uint64_t)10000); // pow(10, 8)
    std::string const P2P_REMOTE_DEBUG_TRUSTED_PUB_KEY = "0000000000000000000000000000000000000000000000000000000000000000";

    uint64_t const CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX = 289;
    uint64_t const CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX = 0x629f;
    uint64_t const CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX = 0x59a0;
    uint16_t const P2P_DEFAULT_PORT = 9230;
    uint16_t const RPC_DEFAULT_PORT = 9231;
    uint16_t const ZMQ_RPC_DEFAULT_PORT = 9232;
    boost::uuids::uuid const NETWORK_ID = { {
        0x04, 0x1c, 0x2e, 0x4c, 0x6d, 0x4c, 0x012, 0xb3, 0x16, 0x12, 0x03, 0x2a, 0x8b, 0x03, 0x3c, 0x4b
      } }; // Bender's nightmare
    std::string const GENESIS_TX = "013c01ff0001ffffff03029b2e4c0281c0b02e7c53291a94d1d0cbff8883f8024f5142ee494ffbbd08807121017767aafcde9be00dcfd098715ebcf7f410daebc582fda69d24a28e9d0bc890d1";
    uint32_t const GENESIS_NONCE = 70;

  // Hash domain separators
  const char HASH_KEY_BULLETPROOF_EXPONENT[] = "bulletproof";
  const char HASH_KEY_RINGDB[] = "ringdsb";
  const char HASH_KEY_SUBADDRESS[] = "SubAddr";
  const unsigned char HASH_KEY_ENCRYPTED_PAYMENT_ID = 0x8d;
  const unsigned char HASH_KEY_WALLET = 0x8c;
  const unsigned char HASH_KEY_WALLET_CACHE = 0x8d;
  const unsigned char HASH_KEY_RPC_PAYMENT_NONCE = 0x58;
  const unsigned char HASH_KEY_MEMORY = 'k';
  const unsigned char HASH_KEY_MULTISIG[] = {'M', 'u', 'l', 't' , 'i', 's', 'i', 'g', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

  std::string const GOVERNANCE_WALLET_ADDRESS = "TvziQSEi93chTMViBzw8Y4eerEjmGq2Q6ajekvgyTyqkGcsj97YJDzF8TMnTWdv7NXQ2ZXfeWJPwRAbVHUjbgFcN2AvU35KfX";
  std::string const BRIDGE_WALLET_ADDRESS = "Tw16wVGVwjqY2sSKx11UNjQ8NAosTSwzzitYZfVrXt3iP3DgL5beLz55quDcqpqUvoQTvjyNyRb7mUXf3JKDAyLd36AtDf2ei";
  std::string const NEW_BRIDGE_WALLET_ADDRESS = "TvyjwByVHjgCqNKrngt4TQRDgJL7cazWnTXYHXmbFewsKMuN6ozKNcBVkgcpyQwVPRYZCyaAe1W7xN8SdgxqnT4S1UMStejYx";
  std::string const DEV_FUND_WALLET = "TvzdbKGga5fSr7fgCTuvR1GY4g9v3No28a6QrcdnnwBkFtisk4MKPLnARAunWBxQJ82L96nGS3ET7BQMhzM788Kp1pweuUfPD";
  std::string const NEW_GOV_WALLET = "TvzXGov4tNr6jYG2gdox7bcuEBwwSTpQYAb6w7qgSxuu4hsxY9CMgMgaL6EeqVcQ6hS7Cppn73W8ZSMU8gLMi4N42yTShfkP9";
  std::string const NEW_DEV_WALLET = "Tw1XDEVkfVsRFhvjPQJgTjFi4uXDBiMomYeaaaj43SHPSTyLj8nBkdv2KBV8t9CzuCUy1fgYkk9tse6xA3B5oPJZ1jLfHLDrh";

  namespace testnet
  {
    uint64_t const CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX = 0x536;
    uint64_t const CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX = 0x5bb8;
    uint64_t const CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX = 0xb6;
    uint16_t const P2P_DEFAULT_PORT = 9330;
    uint16_t const RPC_DEFAULT_PORT = 9331;
    uint16_t const ZMQ_RPC_DEFAULT_PORT = 9332;
    boost::uuids::uuid const NETWORK_ID = { {
       0x17 ,0x19, 0xF5, 0x67 , 0x65, 0x03 , 0x42, 0x62, 0x15, 0x21, 0x01, 0x72, 0x14, 0xA3, 0xA5, 0x14
     } }; // Bender's daydream

    std::string const GOVERNANCE_WALLET_ADDRESS = "XT1mQ4qNhqARHKawpsC4DkCmJxGSiW6EfGej4jssjY7QEzKZgSHmkeuQYHsY3gRhDv4KMt8QQX8TEPBmJQe1SEea38fHATH5C";
    std::string const BRIDGE_WALLET_ADDRESS = "XT1mQ4qNhqARHKawpsC4DkCmJxGSiW6EfGej4jssjY7QEzKZgSHmkeuQYHsY3gRhDv4KMt8QQX8TEPBmJQe1SEea38fHATH5C";
    std::string const NEW_BRIDGE_WALLET_ADDRESS = "XT2SoQm1yCJNJrp1c4fUMbFAXfsoVLkPnh85ut4BTexrWbet6DS5qBP2oDxX4aHVSNVTFXCPY1y34Hp3uG8E8EmF1qkkKBs3S";

}

  namespace stagenet
  {
   uint64_t const CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX = 0x95f34;
   uint64_t const CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX = 0x1ddf34;
   uint64_t const CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX = 0x229f34;
   //uint64_t const CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX = 289;
   //uint64_t const CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX = 289;
   //uint64_t const CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX = 289;
   uint16_t const P2P_DEFAULT_PORT = 9430;
   uint16_t const RPC_DEFAULT_PORT = 9431;
   uint16_t const ZMQ_RPC_DEFAULT_PORT = 9432;
   boost::uuids::uuid const NETWORK_ID = { {
       0x14 ,0x20, 0xF3, 0x71 , 0x60, 0x01 , 0x30, 0x62, 0x16, 0x35, 0x02, 0x82, 0x15, 0xA2, 0xA1, 0x14
     } }; // Bender's daydream

  std::string const GOVERNANCE_WALLET_ADDRESS = "XES1cnvrXNh5fkRvwKPijaBbsHJEnDwS949VHpZEa4DtB3jduSbDjDVZ6tb7ct9PDY6kJTFc1TAEtYF2Dacun4Hw5aVTo2s99z";
  std::string const BRIDGE_WALLET_ADDRESS = "XES1aQWz3JVhPdXynQUBL21tbHR8Kxi7tehFbeThzPAEeApqqmy1XKBR4mimvrMJSeJpYm8VHN3bkK22nmm9dsnB2GydcWBSjd";
  std::string const NEW_BRIDGE_WALLET_ADDRESS = "XES1PcUnsFZHR42hgfW3qoZV8hUaQkBXoaWHF3mhqNmeZRLLqPFX5pELQWtJnrUAWfUVN96yRdbVK8i48dC3r1pY9v4dugs8Nu";
  std::string const DEV_FUND_WALLET = "XES1TLv45CSe2ufGVvUF3U3FBr6vmz7hNX2pEns7U3HQ4PbTEJ4o1f2XyAf3DTbx8Y1BWvdSfrbQL4EVbfzcU1uz7na8yAk6Yi";
  std::string const NEW_GOV_WALLET = "XES1Qh2ErQHiYRSNdjbhK2YYU3vfGFGj298SwYBQVvb5eiymct5EmXcFJD2pGuVY2c1TLtDnwpicBhP2i9VatN8e6AqcYzeUHq";
  std::string const NEW_DEV_WALLET = "XES1McEbjv3gZhBwhFaYDqF7fmkcFA9sg4zEuv9wpJwgHHh6iZ891Qu1kcn2TUGHZSGBf4shzYuBndDRf1DAtsq23X4L1Enmpz";

  }
}

namespace cryptonote
{
  enum network_version
  {
    network_version_0 = 0,
    network_version_1 = 1,
    network_version_2,
    network_version_3,
    network_version_4,
    network_version_5,
    network_version_6,
    network_version_7,
    network_version_8,
    network_version_9,
    network_version_10,
    network_version_11,
    network_version_12,
    network_version_13,
    network_version_14,
    network_version_15,
    network_version_16,
    network_version_17,
    network_version_18,
    network_version_19,
    network_version_20,
    network_version_21,

    network_version_count,
  };

  enum network_type : uint8_t
  {
    MAINNET = 0,
    TESTNET,
    STAGENET,
    FAKECHAIN,
    UNDEFINED = 255
  };
  struct config_t
  {
    uint64_t CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX;
    uint64_t CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX;
    uint64_t CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX;
    uint16_t P2P_DEFAULT_PORT;
    uint16_t RPC_DEFAULT_PORT;
    uint16_t ZMQ_RPC_DEFAULT_PORT;
    boost::uuids::uuid NETWORK_ID;
    std::string GENESIS_TX;
    uint32_t GENESIS_NONCE;
    std::string const *GOVERNANCE_WALLET_ADDRESS;
    std::string const *BRIDGE_WALLET_ADDRESS;
    std::string const *NEW_BRIDGE_WALLET_ADDRESS;
    std::string const *DEV_FUND_WALLET;
    std::string const *NEW_GOV_WALLET;
    std::string const *NEW_DEV_WALLET;

  };
  inline const config_t& get_config(network_type nettype)
  {
    static config_t mainnet = {
      ::config::CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX,
      ::config::CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX,
      ::config::CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX,
      ::config::P2P_DEFAULT_PORT,
      ::config::RPC_DEFAULT_PORT,
      ::config::ZMQ_RPC_DEFAULT_PORT,
      ::config::NETWORK_ID,
      ::config::GENESIS_TX,
      ::config::GENESIS_NONCE,
      &::config::GOVERNANCE_WALLET_ADDRESS,
      &::config::BRIDGE_WALLET_ADDRESS,
      &::config::NEW_BRIDGE_WALLET_ADDRESS,
      &::config::DEV_FUND_WALLET,
      &::config::NEW_GOV_WALLET,
      &::config::NEW_DEV_WALLET,
    };
    static config_t testnet = {
      ::config::testnet::CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX,
      ::config::testnet::CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX,
      ::config::testnet::CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX,
      ::config::testnet::P2P_DEFAULT_PORT,
      ::config::testnet::RPC_DEFAULT_PORT,
      ::config::testnet::ZMQ_RPC_DEFAULT_PORT,
      ::config::testnet::NETWORK_ID,
      ::config::GENESIS_TX,
      ::config::GENESIS_NONCE,
      &::config::testnet::GOVERNANCE_WALLET_ADDRESS,
      &::config::testnet::BRIDGE_WALLET_ADDRESS,
      &::config::testnet::NEW_BRIDGE_WALLET_ADDRESS,
    };
    static config_t stagenet = {
      ::config::stagenet::CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX,
      ::config::stagenet::CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX,
      ::config::stagenet::CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX,
      ::config::stagenet::P2P_DEFAULT_PORT,
      ::config::stagenet::RPC_DEFAULT_PORT,
      ::config::stagenet::ZMQ_RPC_DEFAULT_PORT,
      ::config::stagenet::NETWORK_ID,
      ::config::GENESIS_TX,
      ::config::GENESIS_NONCE,
      &::config::stagenet::GOVERNANCE_WALLET_ADDRESS,
      &::config::stagenet::BRIDGE_WALLET_ADDRESS,
      &::config::stagenet::NEW_BRIDGE_WALLET_ADDRESS,
      &::config::stagenet::DEV_FUND_WALLET,
      &::config::stagenet::NEW_GOV_WALLET,
      &::config::stagenet::NEW_DEV_WALLET,
    };
    switch (nettype)
    {
      case MAINNET: return mainnet;
      case TESTNET: return testnet;
      case STAGENET: return stagenet;
      case FAKECHAIN: return mainnet;
      default: throw std::runtime_error("Invalid network type");
    }
  };
}
