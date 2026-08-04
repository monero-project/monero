// Copyright (c) 2026, The Monero Project
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

// Unit tests for the categorization of hw::trezor::exc exceptions.  The
// wallet API carries the result to its frontends as
// Monero::Wallet::TrezorError, which decides whether a failed device
// open offers a retry, re-prompts for a pairing code, or reports a hard
// error, so both the mapping and the integer values are a contract.

#include "gtest/gtest.h"

#include <stdexcept>

#include "device_trezor/trezor/exceptions.hpp"

namespace exc = hw::trezor::exc;
using exc::error_kind;

// ---------------------------------------------------------------------
// Communication failures: the device is off, unplugged, held by another
// process, or stopped answering.  All retryable.
// ---------------------------------------------------------------------

TEST(TrezorExceptions, not_connected_is_unreachable) {
  EXPECT_EQ(exc::classify(exc::NotConnectedException()), error_kind::unreachable);
}

TEST(TrezorExceptions, acquire_failure_is_unreachable) {
  EXPECT_EQ(exc::classify(exc::DeviceAcquireException()), error_kind::unreachable);
}

TEST(TrezorExceptions, unresponsive_device_is_unreachable) {
  EXPECT_EQ(exc::classify(exc::DeviceNotResponsiveException()), error_kind::unreachable);
  EXPECT_EQ(exc::classify(exc::TimeoutException()), error_kind::unreachable);
}

TEST(TrezorExceptions, expired_session_is_unreachable) {
  EXPECT_EQ(exc::classify(exc::SessionException()), error_kind::unreachable);
}

TEST(TrezorExceptions, bare_communication_failure_is_unreachable) {
  EXPECT_EQ(exc::classify(exc::CommunicationException()), error_kind::unreachable);
}

// ---------------------------------------------------------------------
// User cancellation.  CancelledException derives from FailureException,
// which derives from ProtocolException, so the classification has to
// match it before either base.
// ---------------------------------------------------------------------

TEST(TrezorExceptions, on_device_cancel_is_cancelled) {
  EXPECT_EQ(exc::classify(exc::proto::CancelledException()), error_kind::cancelled);
}

TEST(TrezorExceptions, cancel_carrying_a_failure_code_is_cancelled) {
  // The shape thrown by throw_failure_exception for Failure_ActionCancelled.
  const boost::optional<uint32_t> code(4);
  const boost::optional<std::string> message(std::string("Cancelled"));
  EXPECT_EQ(exc::classify(exc::proto::CancelledException(code, message)),
            error_kind::cancelled);
}

// ---------------------------------------------------------------------
// Pairing.  A rejected code is restated as PairingFailedException by
// device_trezor_base, which is the only layer that knows a CodeEntry
// exchange was in flight.
// ---------------------------------------------------------------------

TEST(TrezorExceptions, rejected_pairing_code_is_pairing_rejected) {
  EXPECT_EQ(exc::classify(exc::proto::PairingFailedException()), error_kind::pairing_rejected);
}

TEST(TrezorExceptions, pairing_rejection_wins_over_its_failure_base) {
  const exc::proto::FailureException &as_failure = exc::proto::PairingFailedException();
  EXPECT_EQ(exc::classify(as_failure), error_kind::pairing_rejected);
}

// ---------------------------------------------------------------------
// Firmware without Monero support.  Thrown by require_initialized() once
// the Features message is in, which on a first pairing is after the user
// has already typed a code, so it needs its own non-retryable category.
// ---------------------------------------------------------------------

TEST(TrezorExceptions, missing_monero_capability_is_firmware_unsupported) {
  EXPECT_EQ(exc::classify(exc::FirmwareNotSupportedException()),
            error_kind::firmware_unsupported);
}

// ---------------------------------------------------------------------
// Protocol faults: framing, encoding, and the authentication checks of
// the transport.  A retry does not help, so the frontend reports them.
// ---------------------------------------------------------------------

TEST(TrezorExceptions, protocol_fault_is_protocol) {
  EXPECT_EQ(exc::classify(exc::ProtocolException("THP frame length exceeds 16-bit field")),
            error_kind::protocol);
}

TEST(TrezorExceptions, encoding_fault_is_protocol) {
  EXPECT_EQ(exc::classify(exc::EncodingException()), error_kind::protocol);
}

TEST(TrezorExceptions, protocol_security_assertion_is_protocol) {
  EXPECT_EQ(exc::classify(exc::proto::SecurityException()), error_kind::protocol);
}

TEST(TrezorExceptions, transport_security_failure_is_protocol) {
  // SecurityException sits outside the TrezorException hierarchy.
  EXPECT_EQ(exc::classify(exc::SecurityException()), error_kind::protocol);
  EXPECT_EQ(exc::classify(exc::Poly1305TagInvalid()), error_kind::protocol);
}

TEST(TrezorExceptions, unexpected_message_is_protocol) {
  // Raised by the client when a response does not match the request,
  // even though it derives from FailureException.
  EXPECT_EQ(exc::classify(exc::proto::UnexpectedMessageException()), error_kind::protocol);
}

// ---------------------------------------------------------------------
// Everything the device itself refused for a reason of its own.
// ---------------------------------------------------------------------

TEST(TrezorExceptions, device_reported_failure_is_other) {
  EXPECT_EQ(exc::classify(exc::proto::FailureException()), error_kind::other);
  EXPECT_EQ(exc::classify(exc::proto::InvalidPinException()), error_kind::other);
  EXPECT_EQ(exc::classify(exc::proto::NotInitializedException()), error_kind::other);
  EXPECT_EQ(exc::classify(exc::proto::FirmwareErrorException()), error_kind::other);
}

TEST(TrezorExceptions, bare_device_exception_is_other) {
  EXPECT_EQ(exc::classify(exc::TrezorException("Device is in the bootloader mode")),
            error_kind::other);
}

// ---------------------------------------------------------------------
// Anything that is not a device failure at all, such as the wallet
// error raised when a password does not match.
// ---------------------------------------------------------------------

TEST(TrezorExceptions, unrelated_exception_is_none) {
  EXPECT_EQ(exc::classify(std::runtime_error("invalid password")), error_kind::none);
}

// ---------------------------------------------------------------------
// Log level and integer surface.
// ---------------------------------------------------------------------

TEST(TrezorExceptions, expected_failures_are_the_ones_the_user_answers) {
  EXPECT_TRUE(exc::is_expected_failure(error_kind::unreachable));
  EXPECT_TRUE(exc::is_expected_failure(error_kind::cancelled));
  EXPECT_TRUE(exc::is_expected_failure(error_kind::pairing_rejected));
  EXPECT_TRUE(exc::is_expected_failure(error_kind::firmware_unsupported));
  EXPECT_FALSE(exc::is_expected_failure(error_kind::protocol));
  EXPECT_FALSE(exc::is_expected_failure(error_kind::other));
  EXPECT_FALSE(exc::is_expected_failure(error_kind::none));
}

TEST(TrezorExceptions, enum_values_are_stable_across_the_api_boundary) {
  // Monero::Wallet::TrezorError mirrors these, and the GUI compares the
  // integers, so values may be appended but never reordered.
  EXPECT_EQ(static_cast<int>(error_kind::none), 0);
  EXPECT_EQ(static_cast<int>(error_kind::unreachable), 1);
  EXPECT_EQ(static_cast<int>(error_kind::cancelled), 2);
  EXPECT_EQ(static_cast<int>(error_kind::protocol), 3);
  EXPECT_EQ(static_cast<int>(error_kind::other), 4);
  EXPECT_EQ(static_cast<int>(error_kind::firmware_unsupported), 5);
  EXPECT_EQ(static_cast<int>(error_kind::pairing_rejected), 6);
}
