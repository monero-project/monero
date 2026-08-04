// Copyright (c) 2017-2024, The Monero Project
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

#ifndef MONERO_EXCEPTIONS_H
#define MONERO_EXCEPTIONS_H

#include <exception>
#include <string>
#include <boost/optional.hpp>

namespace hw {
namespace trezor {
namespace exc {

  class SecurityException : public std::exception {
  protected:
    boost::optional<std::string> reason;

  public:
    SecurityException(): reason("General Security exception"){}
    explicit SecurityException(std::string what): reason(what){}

    virtual const char* what() const throw() {
      return reason.get().c_str();
    }
  };

  class Poly1305TagInvalid: public SecurityException {
  public:
    using SecurityException::SecurityException;
    Poly1305TagInvalid(): SecurityException("Poly1305 authentication tag invalid"){}
  };

  class TrezorException : public std::exception {
  protected:
    boost::optional<std::string> reason;

  public:
    TrezorException(): reason("General Trezor exception"){}
    explicit TrezorException(std::string what): reason(what){}

    virtual const char* what() const throw() {
      return reason.get().c_str();
    }
  };

  class CommunicationException: public TrezorException {
  public:
    using TrezorException::TrezorException;
    CommunicationException(): TrezorException("Trezor communication error"){}
  };

  class EncodingException: public CommunicationException {
  public:
    using CommunicationException::CommunicationException;
    EncodingException(): CommunicationException("Trezor message encoding error"){}
  };

  class NotConnectedException : public CommunicationException {
  public:
    using CommunicationException::CommunicationException;
    NotConnectedException(): CommunicationException("Trezor not connected"){}
  };

  class DeviceNotResponsiveException : public CommunicationException {
  public:
    using CommunicationException::CommunicationException;
    DeviceNotResponsiveException(): CommunicationException("Trezor does not respond to ping"){}
  };

  class DeviceAcquireException : public CommunicationException {
  public:
    using CommunicationException::CommunicationException;
    DeviceAcquireException(): CommunicationException("Trezor could not be acquired"){}
  };

  class SessionException: public CommunicationException {
  public:
    using CommunicationException::CommunicationException;
    SessionException(): CommunicationException("Trezor session expired"){}
  };

  class TimeoutException: public CommunicationException {
  public:
    using CommunicationException::CommunicationException;
    TimeoutException(): CommunicationException("Trezor communication timeout"){}
  };

  class ProtocolException: public CommunicationException {
  public:
    using CommunicationException::CommunicationException;
    ProtocolException(): CommunicationException("Trezor protocol error"){}
  };

  class FirmwareNotSupportedException: public TrezorException {
  public:
    using TrezorException::TrezorException;
    FirmwareNotSupportedException(): TrezorException("Trezor firmware version does not support Monero"){}
  };

  // Communication protocol namespace
  // Separated to distinguish between client and Trezor side exceptions.
namespace proto {

  class SecurityException : public ProtocolException {
  public:
    using ProtocolException::ProtocolException;
    SecurityException(): ProtocolException("Security assertion violated in the protocol"){}
  };

  class FailureException : public ProtocolException {
  private:
    boost::optional<uint32_t> code;
    boost::optional<std::string> message;
  public:
    using ProtocolException::ProtocolException;
    FailureException(): ProtocolException("Trezor returned failure"){}
    FailureException(boost::optional<uint32_t> code,
                     boost::optional<std::string> message)
        : code(code), message(message) {
      reason = "Trezor returned failure: code="
               + (code ? std::to_string(code.get()) : "")
               + ", message=" + (message ? message.get() : "");
    };
  };

  class UnexpectedMessageException : public FailureException {
  public:
    using FailureException::FailureException;
    UnexpectedMessageException(): FailureException("Trezor claims unexpected message received"){}
  };

  class CancelledException : public FailureException {
  public:
    using FailureException::FailureException;
    CancelledException(): FailureException("Trezor returned: cancelled operation"){}
  };

  class PinExpectedException : public FailureException {
  public:
    using FailureException::FailureException;
    PinExpectedException(): FailureException("Trezor claims PIN is expected"){}
  };

  class InvalidPinException : public FailureException {
  public:
    using FailureException::FailureException;
    InvalidPinException(): FailureException("Trezor claims PIN is invalid"){}
  };

  class NotEnoughFundsException : public FailureException {
  public:
    using FailureException::FailureException;
    NotEnoughFundsException(): FailureException("Trezor claims not enough funds"){}
  };

  class NotInitializedException : public FailureException {
  public:
    using FailureException::FailureException;
    NotInitializedException(): FailureException("Trezor claims not initialized"){}
  };

  class FirmwareErrorException : public FailureException {
  public:
    using FailureException::FailureException;
    FirmwareErrorException(): FailureException("Trezor returned firmware error"){}
  };

  class PairingFailedException : public FailureException {
  public:
    using FailureException::FailureException;
    PairingFailedException(): FailureException("Trezor rejected the pairing code"){}
  };

}

  // Broad categories of device failure.  The exception type says exactly
  // what went wrong; this collapses the hierarchy into the few cases a
  // caller has to treat differently, such as the log level to use or the
  // recovery a wallet frontend offers.  Monero::Wallet::TrezorError
  // mirrors these values across the wallet API boundary, so they are
  // append-only.
  enum class error_kind : int {
    none                 = 0,  // not a device failure
    unreachable          = 1,  // device off, unplugged, held elsewhere, or unresponsive
    cancelled            = 2,  // cancelled on the device or in a host prompt
    protocol             = 3,  // framing, Noise, or protocol assertion failure
    other                = 4,  // device refused the operation for a reason of its own
    firmware_unsupported = 5,  // firmware has no Monero support
    pairing_rejected     = 6,  // device rejected the pairing code
  };

  // Categorizes a failure by exception type.  The hierarchy nests (a
  // CancelledException is a FailureException is a ProtocolException), so
  // the tests run most derived first and the first match wins.
  inline error_kind classify(const std::exception &e) {
    if (dynamic_cast<const proto::PairingFailedException *>(&e)) {
      return error_kind::pairing_rejected;
    }
    if (dynamic_cast<const proto::CancelledException *>(&e)) {
      return error_kind::cancelled;
    }
    if (dynamic_cast<const FirmwareNotSupportedException *>(&e)) {
      return error_kind::firmware_unsupported;
    }
    // Thrown by the client when a response does not match the request,
    // which is a protocol fault despite deriving from FailureException.
    if (dynamic_cast<const proto::UnexpectedMessageException *>(&e)) {
      return error_kind::protocol;
    }
    // Anything else the device itself reported: an invalid PIN, an
    // uninitialized device, a firmware error.
    if (dynamic_cast<const proto::FailureException *>(&e)) {
      return error_kind::other;
    }
    // SecurityException sits outside TrezorException and covers the
    // transport's authentication failures.
    if (dynamic_cast<const ProtocolException *>(&e) ||
        dynamic_cast<const EncodingException *>(&e) ||
        dynamic_cast<const SecurityException *>(&e)) {
      return error_kind::protocol;
    }
    if (dynamic_cast<const CommunicationException *>(&e)) {
      return error_kind::unreachable;
    }
    if (dynamic_cast<const TrezorException *>(&e)) {
      return error_kind::other;
    }
    return error_kind::none;
  }

  // True for the failures a user is expected to meet and can respond to:
  // the device is off or busy, they cancelled, they mistyped the pairing
  // code, or the firmware has no Monero support.  Everything else means
  // a fault in the protocol or in the client.
  inline bool is_expected_failure(error_kind kind) {
    return kind == error_kind::unreachable
        || kind == error_kind::cancelled
        || kind == error_kind::pairing_rejected
        || kind == error_kind::firmware_unsupported;
  }

}
}
}
#endif //MONERO_EXCEPTIONS_H
