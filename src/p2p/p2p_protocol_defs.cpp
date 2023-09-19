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

#include "p2p_protocol_defs.h"

namespace
{
  constexpr bool valid_fingerprint_size(const std::size_t length) noexcept
  {
    return length == 0 || length == SSL_FINGERPRINT_SIZE;
  }
}

namespace nodetool
{
  ssl_parameters::ssl_parameters(std::string cert_finger, epee::net_utils::ssl_support_t mode)
    : ssl_parameters(std::move(cert_finger), get_encryption_mode(mode))
  {}

  ssl_parameters::ssl_parameters(std::string cert_finger, const uint8_t mode)
    : cert_finger(std::move(cert_finger)), mode(mode)
  {
    sanitize();
  }

  void ssl_parameters::sanitize()
  {
    if (!valid_fingerprint_size(cert_finger.size()))
      std::string{}.swap(cert_finger);

    if (!cert_finger.empty())
      mode = emode_ssl_enabled;
  }

  epee::net_utils::ssl_options_t ssl_parameters::get_p2p_encryption() const
  {
    namespace net = epee::net_utils;
    const auto disable_verify = [] (net::ssl_options_t ssl)
    {
      ssl.verification = net::ssl_verification_t::none;
      return ssl;
    };

    switch (mode)
    {
      default:
      case emode_ssl_autodetect:
        return disable_verify(net::ssl_support_t::e_ssl_support_autodetect);
      case emode_disabled:
        return net::ssl_support_t::e_ssl_support_disabled;
      case emode_ssl_enabled:
        break;
    }

    if (cert_finger.empty())
      return disable_verify(net::ssl_support_t::e_ssl_support_enabled);

    // Fingerprint specified, require SSL
    std::vector<std::vector<std::uint8_t>> fingers{net::convert_fingerprint(cert_finger)};
    return {std::move(fingers), {}};
  }
} // nodetool


