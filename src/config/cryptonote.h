#pragma once
#include <stdexcept>

#include "common.h"
#include "seeds.h"

#include "mainnet.h"
#include "stagenet.h"
#include "testnet.h"

using namespace config;

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

inline const config_t &get_config(network_type nettype)
{

  switch (nettype)
  {
  case MAINNET:
    return mainnet::data;
  case TESTNET:
    return config::testnet::data;
  case STAGENET:
    return config::stagenet::data;
  case FAKECHAIN:
    return mainnet::data;
  default:
    throw std::runtime_error("Invalid network type");
  }
};

} // namespace cryptonote