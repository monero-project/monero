#pragma once

#include <string>
#include <boost/uuid/uuid.hpp>

namespace config
{
uint64_t const DEFAULT_FEE_ATOMIC_XMR_PER_KB = 500; // Just a placeholder!  Change me!
uint8_t const FEE_CALCULATION_MAX_RETRIES = 10;
uint64_t const DEFAULT_DUST_THRESHOLD = ((uint64_t)2000000000);     // 2 * pow(10, 9)
uint64_t const BASE_REWARD_CLAMP_THRESHOLD = ((uint64_t)100000000); // pow(10, 8)
std::string const P2P_REMOTE_DEBUG_TRUSTED_PUB_KEY = "0000000000000000000000000000000000000000000000000000000000000000";
} // namespace config

namespace cryptonote
{

struct config_t
{
  uint64_t const CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX;
  uint64_t const CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX;
  uint64_t const CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX;
  uint16_t const P2P_DEFAULT_PORT;
  uint16_t const RPC_DEFAULT_PORT;
  uint16_t const ZMQ_RPC_DEFAULT_PORT;
  boost::uuids::uuid const NETWORK_ID;
  std::string const GENESIS_TX;
  uint32_t const GENESIS_NONCE;
};

struct hard_fork_t
{
  uint8_t version;
  uint64_t height;
  uint8_t threshold;
  time_t time;
};

} // namespace cryptonote