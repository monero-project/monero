// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 



#ifndef _NET_SSL_H
#define _NET_SSL_H

#include <chrono>
#include <stdint.h>
#include <string>
#include <vector>
#include <boost/utility/string_ref.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/system/error_code.hpp>

#define SSL_FINGERPRINT_SIZE 32

namespace epee
{
namespace net_utils
{
	enum class ssl_support_t: uint8_t {
		e_ssl_support_disabled,
		e_ssl_support_enabled,
		e_ssl_support_autodetect,
	};

  enum class ssl_verification_t : uint8_t
  {
    none = 0,         //!< Do not verify peer.
    system_ca,        //!< Verify peer via system ca only (do not inspect user certificates)
    user_certificates,//!< Verify peer via specific (non-chain) certificate(s) only.
    user_ca           //!< Verify peer via specific (possibly chain) certificate(s) only.
  };

  struct ssl_authentication_t
  {
    std::string private_key_path; //!< Private key used for authentication
    std::string certificate_path; //!< Certificate used for authentication to peer.

    //! Load `private_key_path` and `certificate_path` into `ssl_context`.
    void use_ssl_certificate(boost::asio::ssl::context &ssl_context) const;
  };

  /*!
    \note `verification != disabled && support == disabled` is currently
      "allowed" via public interface but obviously invalid configuation.
   */
  class ssl_options_t
  {
    // force sorted behavior in private
    std::vector<std::vector<std::uint8_t>> fingerprints_;

  public:
    std::string ca_path;
    ssl_authentication_t auth;
    ssl_support_t support;
    ssl_verification_t verification;

    //! Verification is set to system ca unless SSL is disabled.
    ssl_options_t(ssl_support_t support)
      : fingerprints_(),
        ca_path(),
        auth(),
        support(support),
        verification(support == ssl_support_t::e_ssl_support_disabled ? ssl_verification_t::none : ssl_verification_t::system_ca)
    {}

    //! Provide user fingerprints and/or ca path. Enables SSL and user_certificate verification
    ssl_options_t(std::vector<std::vector<std::uint8_t>> fingerprints, std::string ca_path);

    ssl_options_t(const ssl_options_t&) = default;
    ssl_options_t(ssl_options_t&&) = default;

    ssl_options_t& operator=(const ssl_options_t&) = default;
    ssl_options_t& operator=(ssl_options_t&&) = default;

    //! \return False iff ssl is disabled, otherwise true.
    explicit operator bool() const noexcept { return support != ssl_support_t::e_ssl_support_disabled; }

    //! \retrurn True if `host` can be verified using `this` configuration WITHOUT system "root" CAs.
    bool has_strong_verification(boost::string_ref host) const noexcept;

    //! Search against internal fingerprints. Always false if `behavior() != user_certificate_check`.
    bool has_fingerprint(boost::asio::ssl::verify_context &ctx) const;

    boost::asio::ssl::context create_context() const;

    /*! \note If `this->support == autodetect && this->verification != none`,
          then the handshake will not fail when peer verification fails. The
          assumption is that a re-connect will be attempted, so a warning is
          logged instead of failure.

        \note It is strongly encouraged that clients using `system_ca`
          verification provide a non-empty `host` for rfc2818 verification.

        \param socket Used in SSL handshake and verification
        \param type Client or server
        \param host This parameter is only used when
          `type == client && !host.empty()`. The value is sent to the server for
          situations where multiple hostnames are being handled by a server. If
          `verification == system_ca` the client also does a rfc2818 check to
          ensure that the server certificate is to the provided hostname.

        \return True if the SSL handshake completes with peer verification
          settings. */
    bool handshake(
      boost::asio::ssl::stream<boost::asio::ip::tcp::socket> &socket,
      boost::asio::ssl::stream_base::handshake_type type,
      boost::asio::const_buffer buffer = {},
      const std::string& host = {},
      std::chrono::milliseconds timeout = std::chrono::seconds(15)) const;
  };

        // https://security.stackexchange.com/questions/34780/checking-client-hello-for-https-classification
	constexpr size_t get_ssl_magic_size() { return 9; }
	bool is_ssl(const unsigned char *data, size_t len);
	bool ssl_support_from_string(ssl_support_t &ssl, boost::string_ref s);

	bool create_ec_ssl_certificate(EVP_PKEY *&pkey, X509 *&cert);
	bool create_rsa_ssl_certificate(EVP_PKEY *&pkey, X509 *&cert);
}
}

#endif //_NET_SSL_H
