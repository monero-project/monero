// Copyright (c) 2026, The Numista Coins Project
// See LICENSE for licensing and attribution details.

#pragma once

#include <cstdint>

namespace config
{
  namespace numista_production
  {
    // This profile is intentionally incomplete and is not returned by get_config().
    static const bool CONFIGURED = false;
    static const uint64_t UNSET_HEIGHT = static_cast<uint64_t>(-1);

    static const uint64_t CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX = 0;
    static const uint64_t CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX = 0;
    static const uint64_t CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX = 0;
    static const uint16_t P2P_DEFAULT_PORT = 0;
    static const uint16_t RPC_DEFAULT_PORT = 0;
    static const uint16_t ZMQ_RPC_DEFAULT_PORT = 0;
    static const uint8_t NETWORK_ID[16] = {};
    static const char GENESIS_TX[] = "";
    static const uint32_t GENESIS_NONCE = 0;
    static const uint8_t NUMISTA_ASSET_HARD_FORK_VERSION = 17;
    static const uint64_t NUMISTA_ASSET_HARD_FORK_HEIGHT = UNSET_HEIGHT;
    static const char NUMISTA_ASSET_ISSUER_PUBLIC_KEY_HEX[] = "";
  }
}
