#pragma once

#include <cstdint>

namespace hw
{
namespace trezor
{
namespace thp
{
    constexpr const std::uint16_t CHANNEL_MIN_ID = 0x0001;
    constexpr const std::uint16_t CHANNEL_MAX_ID = 0xFFEF;
    constexpr const std::uint16_t BROADCAST_CHANNEL_ID = 0xFFFF;

    constexpr const std::size_t CHANNEL_NONCE_LEN = 8;
    constexpr const std::size_t MAX_PAYLOAD_LEN = 60000;
    constexpr const std::size_t CHECKSUM_LEN = 4;
} // namespace thp
} // namespace trezor
} // namespace hw
