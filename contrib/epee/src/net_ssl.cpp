// Copyright (c) 2018, The Monero Project
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

#include <string.h>
#include <thread>
#include <boost/asio/ssl.hpp>
#include <boost/lambda/lambda.hpp>
#include <openssl/ssl.h>
#include <openssl/pem.h>
#include "misc_log_ex.h"
#include "net/net_helper.h"
#include "net/net_ssl.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.ssl"

// openssl genrsa -out /tmp/KEY 4096
// openssl req -new -key /tmp/KEY -out /tmp/REQ
// openssl x509 -req -days 999999 -sha256 -in /tmp/REQ -signkey /tmp/KEY -out /tmp/CERT

#ifdef _WIN32
static void add_windows_root_certs(SSL_CTX *ctx) noexcept;
#endif

namespace
{
  struct openssl_bio_free
  {
    void operator()(BIO* ptr) const noexcept
    {
      BIO_free(ptr);
    }
  };
  using openssl_bio = std::unique_ptr<BIO, openssl_bio_free>;

  struct openssl_pkey_free
  {
    void operator()(EVP_PKEY* ptr) const noexcept
    {
      EVP_PKEY_free(ptr);
    }
  };
  using openssl_pkey = std::unique_ptr<EVP_PKEY, openssl_pkey_free>;

  struct openssl_rsa_free
  {
    void operator()(RSA* ptr) const noexcept
    {
      RSA_free(ptr);
    }
  };
  using openssl_rsa = std::unique_ptr<RSA, openssl_rsa_free>;

  struct openssl_bignum_free
  {
    void operator()(BIGNUM* ptr) const noexcept
    {
      BN_free(ptr);
    }
  };
  using openssl_bignum = std::unique_ptr<BIGNUM, openssl_bignum_free>;

  struct openssl_ec_key_free
  {
    void operator()(EC_KEY* ptr) const noexcept
    {
      EC_KEY_free(ptr);
    }
  };
  using openssl_ec_key = std::unique_ptr<EC_KEY, openssl_ec_key_free>;

  struct openssl_group_free
  {
    void operator()(EC_GROUP* ptr) const noexcept
    {
      EC_GROUP_free(ptr);
    }
  };
  using openssl_group = std::unique_ptr<EC_GROUP, openssl_group_free>;

  boost::system::error_code load_ca_file(boost::asio::ssl::context& ctx, const std::string& path)
  {
    SSL_CTX* const ssl_ctx = ctx.native_handle(); // could be moved from context
    if (ssl_ctx == nullptr)
      return {boost::asio::error::invalid_argument};

    if (!SSL_CTX_load_verify_locations(ssl_ctx, path.c_str(), nullptr))
    {
      return boost::system::error_code{
        int(::ERR_get_error()), boost::asio::error::get_ssl_category()
      };
    }
    return boost::system::error_code{};
  }
}

namespace epee
{
namespace net_utils
{


// https://stackoverflow.com/questions/256405/programmatically-create-x509-certificate-using-openssl
bool create_rsa_ssl_certificate(EVP_PKEY *&pkey, X509 *&cert)
{
  MGINFO("Generating SSL certificate");
  pkey = EVP_PKEY_new();
  if (!pkey)
  {
    MERROR("Failed to create new private key");
    return false;
  }

  openssl_pkey pkey_deleter{pkey};
  openssl_rsa rsa{RSA_new()};
  if (!rsa)
  {
    MERROR("Error allocating RSA private key");
    return false;
  }

  openssl_bignum exponent{BN_new()};
  if (!exponent)
  {
    MERROR("Error allocating exponent");
    return false;
  }

  BN_set_word(exponent.get(), RSA_F4);

  if (RSA_generate_key_ex(rsa.get(), 4096, exponent.get(), nullptr) != 1)
  {
    MERROR("Error generating RSA private key");
    return false;
  }

  if (EVP_PKEY_assign_RSA(pkey, rsa.get()) <= 0)
  {
    MERROR("Error assigning RSA private key");
    return false;
  }

  // the RSA key is now managed by the EVP_PKEY structure
  (void)rsa.release();

  cert = X509_new();
  if (!cert)
  {
    MERROR("Failed to create new X509 certificate");
    return false;
  }
  ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);
  X509_gmtime_adj(X509_get_notBefore(cert), 0);
  X509_gmtime_adj(X509_get_notAfter(cert), 3600 * 24 * 182); // half a year
  if (!X509_set_pubkey(cert, pkey))
  {
    MERROR("Error setting pubkey on certificate");
    X509_free(cert);
    return false;
  }
  X509_NAME *name = X509_get_subject_name(cert);
  X509_set_issuer_name(cert, name);

  if (X509_sign(cert, pkey, EVP_sha256()) == 0)
  {
    MERROR("Error signing certificate");
    X509_free(cert);
    return false;
  }
  (void)pkey_deleter.release();
  return true;
}

bool create_ec_ssl_certificate(EVP_PKEY *&pkey, X509 *&cert, int type)
{
  MGINFO("Generating SSL certificate");
  pkey = EVP_PKEY_new();
  if (!pkey)
  {
    MERROR("Failed to create new private key");
    return false;
  }

  openssl_pkey pkey_deleter{pkey};
  openssl_ec_key ec_key{EC_KEY_new()};
  if (!ec_key)
  {
    MERROR("Error allocating EC private key");
    return false;
  }

  EC_GROUP *group = EC_GROUP_new_by_curve_name(type);
  if (!group)
  {
    MERROR("Error getting EC group " << type);
    return false;
  }
  openssl_group group_deleter{group};

  EC_GROUP_set_asn1_flag(group, OPENSSL_EC_NAMED_CURVE); 
  EC_GROUP_set_point_conversion_form(group, POINT_CONVERSION_UNCOMPRESSED);

  if (!EC_GROUP_check(group, NULL))
  {
    MERROR("Group failed check: " << ERR_reason_error_string(ERR_get_error()));
    return false;
  }
  if (EC_KEY_set_group(ec_key.get(), group) != 1)
  {
    MERROR("Error setting EC group");
    return false;
  }
  if (EC_KEY_generate_key(ec_key.get()) != 1)
  {
    MERROR("Error generating EC private key");
    return false;
  }
  if (EVP_PKEY_assign_EC_KEY(pkey, ec_key.get()) <= 0)
  {
    MERROR("Error assigning EC private key");
    return false;
  }

  // the key is now managed by the EVP_PKEY structure
  (void)ec_key.release();

  cert = X509_new();
  if (!cert)
  {
    MERROR("Failed to create new X509 certificate");
    return false;
  }
  ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);
  X509_gmtime_adj(X509_get_notBefore(cert), 0);
  X509_gmtime_adj(X509_get_notAfter(cert), 3600 * 24 * 182); // half a year
  if (!X509_set_pubkey(cert, pkey))
  {
    MERROR("Error setting pubkey on certificate");
    X509_free(cert);
    return false;
  }
  X509_NAME *name = X509_get_subject_name(cert);
  X509_set_issuer_name(cert, name);

  if (X509_sign(cert, pkey, EVP_sha256()) == 0)
  {
    MERROR("Error signing certificate");
    X509_free(cert);
    return false;
  }
  (void)pkey_deleter.release();
  return true;
}

ssl_options_t::ssl_options_t(std::vector<std::vector<std::uint8_t>> fingerprints, std::string ca_path)
  : fingerprints_(std::move(fingerprints)),
    ca_path(std::move(ca_path)),
    auth(),
    support(ssl_support_t::e_ssl_support_enabled),
    verification(ssl_verification_t::user_certificates)
{
  std::sort(fingerprints_.begin(), fingerprints_.end());
}

boost::asio::ssl::context ssl_options_t::create_context() const
{
  boost::asio::ssl::context ssl_context{boost::asio::ssl::context::tls};
  if (!bool(*this))
    return ssl_context;

  // only allow tls v1.2 and up
  ssl_context.set_options(boost::asio::ssl::context::default_workarounds);
  ssl_context.set_options(boost::asio::ssl::context::no_sslv2);
  ssl_context.set_options(boost::asio::ssl::context::no_sslv3);
  ssl_context.set_options(boost::asio::ssl::context::no_tlsv1);
  ssl_context.set_options(boost::asio::ssl::context::no_tlsv1_1);

  // only allow a select handful of tls v1.3 and v1.2 ciphers to be used
  SSL_CTX_set_cipher_list(ssl_context.native_handle(), "ECDHE-ECDSA-CHACHA20-POLY1305-SHA256:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256");

  // set options on the SSL context for added security
  SSL_CTX *ctx = ssl_context.native_handle();
  CHECK_AND_ASSERT_THROW_MES(ctx, "Failed to get SSL context");
  SSL_CTX_clear_options(ctx, SSL_OP_LEGACY_SERVER_CONNECT); // SSL_CTX_SET_OPTIONS(3)
  SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_OFF); // https://stackoverflow.com/questions/22378442
#ifdef SSL_OP_NO_TICKET
  SSL_CTX_set_options(ctx, SSL_OP_NO_TICKET); // https://stackoverflow.com/questions/22378442
#endif
#ifdef SSL_OP_NO_RENEGOTIATION
  SSL_CTX_set_options(ctx, SSL_OP_NO_RENEGOTIATION);
#endif
#ifdef SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION
  SSL_CTX_set_options(ctx, SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);
#endif
#ifdef SSL_OP_NO_COMPRESSION
  SSL_CTX_set_options(ctx, SSL_OP_NO_COMPRESSION);
#endif
#ifdef SSL_OP_CIPHER_SERVER_PREFERENCE
  SSL_CTX_set_options(ctx, SSL_OP_CIPHER_SERVER_PREFERENCE);
#endif
  SSL_CTX_set_ecdh_auto(ctx, 1);

  switch (verification)
  {
    case ssl_verification_t::system_ca:
#ifdef _WIN32
      try { add_windows_root_certs(ssl_context.native_handle()); }
      catch (const std::exception &e) { ssl_context.set_default_verify_paths(); }
#else
      ssl_context.set_default_verify_paths();
#endif
      break;
    case ssl_verification_t::user_certificates:
      ssl_context.set_verify_depth(0);
      /* fallthrough */
    case ssl_verification_t::user_ca:
      if (!ca_path.empty())
      {
        const boost::system::error_code err = load_ca_file(ssl_context, ca_path);
        if (err)
          throw boost::system::system_error{err, "Failed to load user CA file at " + ca_path};
      }
      break;
    default:
      break;
  }

  CHECK_AND_ASSERT_THROW_MES(auth.private_key_path.empty() == auth.certificate_path.empty(), "private key and certificate must be either both given or both empty");
  if (auth.private_key_path.empty())
  {
    EVP_PKEY *pkey;
    X509 *cert;
    bool ok = false;

#ifdef USE_EXTRA_EC_CERT
    CHECK_AND_ASSERT_THROW_MES(create_ec_ssl_certificate(pkey, cert, NID_secp256k1), "Failed to create certificate");
    CHECK_AND_ASSERT_THROW_MES(SSL_CTX_use_certificate(ctx, cert), "Failed to use generated certificate");
    if (!SSL_CTX_use_PrivateKey(ctx, pkey))
      MERROR("Failed to use generated EC private key for " << NID_secp256k1);
    else
      ok = true;
    X509_free(cert);
    EVP_PKEY_free(pkey);
#endif

    CHECK_AND_ASSERT_THROW_MES(create_rsa_ssl_certificate(pkey, cert), "Failed to create certificate");
    CHECK_AND_ASSERT_THROW_MES(SSL_CTX_use_certificate(ctx, cert), "Failed to use generated certificate");
    if (!SSL_CTX_use_PrivateKey(ctx, pkey))
      MERROR("Failed to use generated RSA private key for RSA");
    else
      ok = true;
    X509_free(cert);
    EVP_PKEY_free(pkey);

    CHECK_AND_ASSERT_THROW_MES(ok, "Failed to use any generated certificate");
  }
  else
    auth.use_ssl_certificate(ssl_context);

  return ssl_context;
}

void ssl_authentication_t::use_ssl_certificate(boost::asio::ssl::context &ssl_context) const
{
  ssl_context.use_private_key_file(private_key_path, boost::asio::ssl::context::pem);
  ssl_context.use_certificate_chain_file(certificate_path);
}

bool is_ssl(const unsigned char *data, size_t len)
{
  if (len < get_ssl_magic_size())
    return false;

  // https://security.stackexchange.com/questions/34780/checking-client-hello-for-https-classification
  MDEBUG("SSL detection buffer, " << len << " bytes: "
    << (unsigned)(unsigned char)data[0] << " " << (unsigned)(unsigned char)data[1] << " "
    << (unsigned)(unsigned char)data[2] << " " << (unsigned)(unsigned char)data[3] << " "
    << (unsigned)(unsigned char)data[4] << " " << (unsigned)(unsigned char)data[5] << " "
    << (unsigned)(unsigned char)data[6] << " " << (unsigned)(unsigned char)data[7] << " "
    << (unsigned)(unsigned char)data[8]);
  if (data[0] == 0x16) // record
  if (data[1] == 3) // major version
  if (data[5] == 1) // ClientHello
  if (data[6] == 0 && data[3]*256 + data[4] == data[7]*256 + data[8] + 4) // length check
    return true;
  return false;
}

bool ssl_options_t::has_strong_verification(boost::string_ref host) const noexcept
{
  // onion and i2p addresses contain information about the server cert
  // which both authenticates and encrypts
  if (host.ends_with(".onion") || host.ends_with(".i2p"))
    return true;
  switch (verification)
  {
    default:
    case ssl_verification_t::none:
    case ssl_verification_t::system_ca:
      return false;
    case ssl_verification_t::user_certificates:
    case ssl_verification_t::user_ca:
      break;
  }
  return true;
}

bool ssl_options_t::has_fingerprint(boost::asio::ssl::verify_context &ctx) const
{
  // can we check the certificate against a list of fingerprints?
  if (!fingerprints_.empty()) {
    X509_STORE_CTX *sctx = ctx.native_handle();
    if (!sctx)
    {
      MERROR("Error getting verify_context handle");
      return false;
    }

    X509* cert = nullptr;
    const STACK_OF(X509)* chain = X509_STORE_CTX_get_chain(sctx);
    if (!chain || sk_X509_num(chain) < 1 || !(cert = sk_X509_value(chain, 0)))
    {
      MERROR("No certificate found in verify_context");
      return false;
    }

    // buffer for the certificate digest and the size of the result
    std::vector<uint8_t> digest(EVP_MAX_MD_SIZE);
    unsigned int size{ 0 };

    // create the digest from the certificate
    if (!X509_digest(cert, EVP_sha256(), digest.data(), &size)) {
      MERROR("Failed to create certificate fingerprint");
      return false;
    }

    // strip unnecessary bytes from the digest
    digest.resize(size);

    return std::binary_search(fingerprints_.begin(), fingerprints_.end(), digest);
  }

  return false;
}

bool ssl_options_t::handshake(
  boost::asio::ssl::stream<boost::asio::ip::tcp::socket> &socket,
  boost::asio::ssl::stream_base::handshake_type type,
  const std::string& host,
  std::chrono::milliseconds timeout) const
{
  socket.next_layer().set_option(boost::asio::ip::tcp::no_delay(true));

  /* Using system-wide CA store for client verification is funky - there is
     no expected hostname for server to verify against. If server doesn't have
     specific whitelisted certificates for client, don't require client to
     send certificate at all. */
  const bool no_verification = verification == ssl_verification_t::none ||
    (type == boost::asio::ssl::stream_base::server && fingerprints_.empty() && ca_path.empty());

  /* According to OpenSSL documentation (and SSL specifications), server must
     always send certificate unless "anonymous" cipher mode is used which are
     disabled by default. Either way, the certificate is never inspected. */
  if (no_verification)
    socket.set_verify_mode(boost::asio::ssl::verify_none);
  else
  {
    socket.set_verify_mode(boost::asio::ssl::verify_peer | boost::asio::ssl::verify_fail_if_no_peer_cert);

    // in case server is doing "virtual" domains, set hostname
    SSL* const ssl_ctx = socket.native_handle();
    if (type == boost::asio::ssl::stream_base::client && !host.empty() && ssl_ctx)
      SSL_set_tlsext_host_name(ssl_ctx, host.c_str());

    socket.set_verify_callback([&](const bool preverified, boost::asio::ssl::verify_context &ctx)
    {
      // preverified means it passed system or user CA check. System CA is never loaded
      // when fingerprints are whitelisted.
      const bool verified = preverified &&
        (verification != ssl_verification_t::system_ca || host.empty() || boost::asio::ssl::rfc2818_verification(host)(preverified, ctx));

      if (!verified && !has_fingerprint(ctx))
      {
        // autodetect will reconnect without SSL - warn and keep connection encrypted
        if (support != ssl_support_t::e_ssl_support_autodetect)
        {
          MERROR("SSL certificate is not in the allowed list, connection droppped");
          return false;
        }
        MWARNING("SSL peer has not been verified");
      }
      return true;
    });
  }

  auto& io_service = GET_IO_SERVICE(socket);
  boost::asio::steady_timer deadline(io_service, timeout);
  deadline.async_wait([&socket](const boost::system::error_code& error) {
    if (error != boost::asio::error::operation_aborted)
    {
      socket.next_layer().close();
    }
  });

  boost::system::error_code ec = boost::asio::error::would_block;
  socket.async_handshake(type, boost::lambda::var(ec) = boost::lambda::_1);
  if (io_service.stopped())
  {
    io_service.reset();
  }
  while (ec == boost::asio::error::would_block && !io_service.stopped())
  {
    // should poll_one(), can't run_one() because it can block if there is
    // another worker thread executing io_service's tasks
    // TODO: once we get Boost 1.66+, replace with run_one_for/run_until
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    io_service.poll_one();
  }

  if (ec)
  {
    MERROR("SSL handshake failed, connection dropped: " << ec.message());
    return false;
  }
  MDEBUG("SSL handshake success");
  return true;
}

bool ssl_support_from_string(ssl_support_t &ssl, boost::string_ref s)
{
  if (s == "enabled")
    ssl = epee::net_utils::ssl_support_t::e_ssl_support_enabled;
  else if (s == "disabled")
    ssl = epee::net_utils::ssl_support_t::e_ssl_support_disabled;
  else if (s == "autodetect")
    ssl = epee::net_utils::ssl_support_t::e_ssl_support_autodetect;
  else
    return false;
  return true;
}

} // namespace
} // namespace

#ifdef _WIN32

// https://stackoverflow.com/questions/40307541
// Because Windows always has to do things wonkily
#include <wincrypt.h>
static void add_windows_root_certs(SSL_CTX *ctx) noexcept
{
    HCERTSTORE hStore = CertOpenSystemStore(0, "ROOT");
    if (hStore == NULL) {
        return;
    }

    X509_STORE *store = X509_STORE_new();
    PCCERT_CONTEXT pContext = NULL;
    while ((pContext = CertEnumCertificatesInStore(hStore, pContext)) != NULL) {
        // convert from DER to internal format
        X509 *x509 = d2i_X509(NULL,
                              (const unsigned char **)&pContext->pbCertEncoded,
                              pContext->cbCertEncoded);
        if(x509 != NULL) {
            X509_STORE_add_cert(store, x509);
            X509_free(x509);
        }
    }

    CertFreeCertificateContext(pContext);
    CertCloseStore(hStore, 0);

    // attach X509_STORE to boost ssl context
    SSL_CTX_set_cert_store(ctx, store);
}
#endif

