
#pragma once
#include "common.h"

using namespace cryptonote;

namespace config
{

uint64_t const CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX = 18;
uint64_t const CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX = 19;
uint64_t const CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX = 42;
uint16_t const P2P_DEFAULT_PORT = 18080;
uint16_t const RPC_DEFAULT_PORT = 18081;
uint16_t const ZMQ_RPC_DEFAULT_PORT = 18082;
boost::uuids::uuid const NETWORK_ID = {{0x12, 0x30, 0xF1, 0x71, 0x61, 0x04, 0x41, 0x61, 0x17, 0x31, 0x00, 0x82, 0x16, 0xA1, 0xA1, 0x10}}; // Bender's nightmare
std::string const GENESIS_TX = "013c01ff0001ffffffffffff03029b2e4c0281c0b02e7c53291a94d1d0cbff8883f8024f5142ee494ffbbd08807121017767aafcde9be00dcfd098715ebcf7f410daebc582fda69d24a28e9d0bc890d1";
uint32_t const GENESIS_NONCE = 10000;

namespace mainnet
{
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
  { 1, 1, 0, 1341378000 },

  // version 2 starts from block 1009827, which is on or around the 20th of March, 2016. Fork time finalised on 2015-09-20. No fork voting occurs for the v2 fork.
  { 2, 1009827, 0, 1442763710 },

  // version 3 starts from block 1141317, which is on or around the 24th of September, 2016. Fork time finalised on 2016-03-21.
  { 3, 1141317, 0, 1458558528 },
  
  // version 4 starts from block 1220516, which is on or around the 5th of January, 2017. Fork time finalised on 2016-09-18.
  { 4, 1220516, 0, 1483574400 },
  
  // version 5 starts from block 1288616, which is on or around the 15th of April, 2017. Fork time finalised on 2017-03-14.
  { 5, 1288616, 0, 1489520158 },  

  // version 6 starts from block 1400000, which is on or around the 16th of September, 2017. Fork time finalised on 2017-08-18.
  { 6, 1400000, 0, 1503046577 },

  // version 7 starts from block 1546000, which is on or around the 6th of April, 2018. Fork time finalised on 2018-03-17.
  { 7, 1546000, 0, 1521303150 },

  // version 8 starts from block 1685555, which is on or around the 18th of October, 2018. Fork time finalised on 2018-09-02.
  { 8, 1685555, 0, 1535889547 },

  // version 9 starts from block 1686275, which is on or around the 19th of October, 2018. Fork time finalised on 2018-09-02.
  { 9, 1686275, 0, 1535889548 },
};
static const uint64_t hard_fork_version_1_till = 1009826;

} // namespace mainnet
} // namespace config