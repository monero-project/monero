#pragma once
#include "common.h"
using namespace cryptonote;

namespace config
{
namespace testnet
{
uint64_t const CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX = 53;
uint64_t const CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX = 54;
uint64_t const CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX = 63;
uint16_t const P2P_DEFAULT_PORT = 28080;
uint16_t const RPC_DEFAULT_PORT = 28081;
uint16_t const ZMQ_RPC_DEFAULT_PORT = 28082;
boost::uuids::uuid const NETWORK_ID = {{0x12, 0x30, 0xF1, 0x71, 0x61, 0x04, 0x41, 0x61, 0x17, 0x31, 0x00, 0x82, 0x16, 0xA1, 0xA1, 0x11}}; // Bender's daydream
std::string const GENESIS_TX = "013c01ff0001ffffffffffff03029b2e4c0281c0b02e7c53291a94d1d0cbff8883f8024f5142ee494ffbbd08807121017767aafcde9be00dcfd098715ebcf7f410daebc582fda69d24a28e9d0bc890d1";
uint32_t const GENESIS_NONCE = 10001;

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

static const hard_fork_t hard_forks[] = {
    // version 1 from the start of the blockchain
    {1, 1, 0, 1341378000},

    // version 2 starts from block 624634, which is on or around the 23rd of November, 2015. Fork time finalised on 2015-11-20. No fork voting occurs for the v2 fork.
    {2, 624634, 0, 1445355000},

    // versions 3-5 were passed in rapid succession from September 18th, 2016
    {3, 800500, 0, 1472415034},
    {4, 801219, 0, 1472415035},
    {5, 802660, 0, 1472415036 + 86400 * 180}, // add 5 months on testnet to shut the update warning up since there's a large gap to v6

    {6, 971400, 0, 1501709789},
    {7, 1057027, 0, 1512211236},
    {8, 1057058, 0, 1533211200},
    {9, 1057778, 0, 1533297600},
};
static const uint64_t hard_fork_version_1_till = 624633;

} // namespace testnet
} // namespace config