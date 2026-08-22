#pragma once

#include <cstdint>
#include <optional>

namespace hw
{
namespace trezor
{
namespace thp
{
    constexpr const std::uint8_t ACK_BIT = 0x08;
    constexpr const std::uint8_t SEQ_BIT = 0x10;
    constexpr const std::uint8_t SYNC_MASK = ACK_BIT | SEQ_BIT;

    enum class handshake_message : std::uint8_t
    {
        initiation_request,
        initiation_response,
        completion_request,
        completion_response
    };

    class control_byte
    {
        std::uint8_t value;

    public:
        explicit constexpr control_byte(std::uint8_t value_) : value(value_) {}

        static constexpr control_byte handshake(handshake_message phase)
        {
            switch (phase)
            {
            case handshake_message::initiation_request:
                return control_byte{0x00};
            case handshake_message::initiation_response:
                return control_byte{0x01};
            case handshake_message::completion_request:
                return control_byte{0x02};
            case handshake_message::completion_response:
                return control_byte{0x03};
            }
            return control_byte{0x00};
        }
    };
} // namespace thp
} // namespace trezor
} // namespace hw
