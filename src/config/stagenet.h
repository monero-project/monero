#pragma once
#include "common.h"

using namespace cryptonote;

namespace config
{
namespace stagenet
{
uint64_t const CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX = 24;
uint64_t const CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX = 25;
uint64_t const CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX = 36;
uint16_t const P2P_DEFAULT_PORT = 38080;
uint16_t const RPC_DEFAULT_PORT = 38081;
uint16_t const ZMQ_RPC_DEFAULT_PORT = 38082;
boost::uuids::uuid const NETWORK_ID = {{0x12, 0x30, 0xF1, 0x71, 0x61, 0x04, 0x41, 0x61, 0x17, 0x31, 0x00, 0x82, 0x16, 0xA1, 0xA1, 0x12}}; // Bender's daydream
std::string const GENESIS_TX = "013c01ff0001ffffffffffff0302df5d56da0c7d643ddd1ce61901c7bdc5fb1738bfe39fbe69c28a3a7032729c0f2101168d0c4ca86fb55a4cf6a36d31431be1c53a3bd7411bb24e8832410289fa6f3b";
uint32_t const GENESIS_NONCE = 10002;

static const config_t data = {
    CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX,
    CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX,
    CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX,
    P2P_DEFAULT_PORT,
    RPC_DEFAULT_PORT,
    ZMQ_RPC_DEFAULT_PORT,
    NETWORK_ID,
    GENESIS_TX,
    GENESIS_NONCE};

} // namespace stagenet
} // namespace config
