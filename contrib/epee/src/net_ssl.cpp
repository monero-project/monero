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
#include <boost/asio/ssl.hpp>
#include <openssl/ssl.h>
#include <openssl/pem.h>
#include "misc_log_ex.h"
#include "net/net_ssl.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.ssl"

// openssl genrsa -out /tmp/KEY 4096
// openssl req -new -key /tmp/KEY -out /tmp/REQ
// openssl x509 -req -days 999999 -sha256 -in /tmp/REQ -signkey /tmp/KEY -out /tmp/CERT

namespace
{
  struct openssl_bio_free
  {
    void operator()(BIO* ptr) const noexcept
    {
      if (ptr)
        BIO_free(ptr);
    }
  };
  using openssl_bio = std::unique_ptr<BIO, openssl_bio_free>;

  struct openssl_pkey_free
  {
    void operator()(EVP_PKEY* ptr) const noexcept
    {
      if (ptr)
        EVP_PKEY_free(ptr);
    }
  };
  using openssl_pkey = std::unique_ptr<EVP_PKEY, openssl_pkey_free>;

}

namespace epee
{
namespace net_utils
{

// https://stackoverflow.com/questions/256405/programmatically-create-x509-certificate-using-openssl
bool create_ssl_certificate(EVP_PKEY *&pkey, X509 *&cert)
{
  MGINFO("Generating SSL certificate");
  pkey = EVP_PKEY_new();
  openssl_pkey pkey_deleter{pkey};
  if (!pkey)
  {
    MERROR("Failed to create new private key");
    return false;
  }
  RSA *rsa = RSA_generate_key(4096, RSA_F4, NULL, NULL);
  if (!rsa)
  {
    MERROR("Error generating RSA private key");
    return false;
  }
  if (EVP_PKEY_assign_RSA(pkey, rsa) <= 0)
  {
    RSA_free(rsa);
    MERROR("Error assigning RSA private key");
    return false;
  }

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
  return true;
}

bool create_ssl_certificate(std::string &pkey_buffer, std::string &cert_buffer)
{
  EVP_PKEY *pkey;
  X509 *cert;
  if (!create_ssl_certificate(pkey, cert))
    return false;
  BIO *bio_pkey = BIO_new(BIO_s_mem()), *bio_cert = BIO_new(BIO_s_mem());
  openssl_bio bio_pkey_deleter{bio_pkey};
  bool success = PEM_write_bio_PrivateKey(bio_pkey, pkey, NULL, NULL, 0, NULL, NULL) && PEM_write_bio_X509(bio_cert, cert);
  X509_free(cert);
  if (!success)
  {
    MERROR("Failed to write cert and/or pkey: " << ERR_get_error());
    return false;
  }
  BUF_MEM *buf = NULL;
  BIO_get_mem_ptr(bio_pkey, &buf);
  if (!buf || !buf->data || !buf->length)
  {
    MERROR("Failed to write pkey: " << ERR_get_error());
    return false;
  }
  pkey_buffer = std::string(buf->data, buf->length);
  buf = NULL;
  BIO_get_mem_ptr(bio_cert, &buf);
  if (!buf || !buf->data || !buf->length)
  {
    MERROR("Failed to write cert: " << ERR_get_error());
    return false;
  }
  cert_buffer = std::string(buf->data, buf->length);
  return success;
}

ssl_context_t create_ssl_context(const std::pair<std::string, std::string> &private_key_and_certificate_path, std::list<std::string> allowed_certificates, std::vector<std::vector<uint8_t>> allowed_fingerprints, bool allow_any_cert)
{
  ssl_context_t ssl_context{boost::asio::ssl::context(boost::asio::ssl::context::tlsv12), std::move(allowed_certificates), std::move(allowed_fingerprints)};

  // only allow tls v1.2 and up
  ssl_context.context.set_options(boost::asio::ssl::context::default_workarounds);
  ssl_context.context.set_options(boost::asio::ssl::context::no_sslv2);
  ssl_context.context.set_options(boost::asio::ssl::context::no_sslv3);
  ssl_context.context.set_options(boost::asio::ssl::context::no_tlsv1);
  ssl_context.context.set_options(boost::asio::ssl::context::no_tlsv1_1);

  // only allow a select handful of tls v1.3 and v1.2 ciphers to be used
  SSL_CTX_set_cipher_list(ssl_context.context.native_handle(), "ECDHE-ECDSA-CHACHA20-POLY1305-SHA256:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES256-SHA384:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-CHACHA20-POLY1305");

  // set options on the SSL context for added security
  SSL_CTX *ctx = ssl_context.context.native_handle();
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
  ssl_context.context.set_default_verify_paths();

  CHECK_AND_ASSERT_THROW_MES(private_key_and_certificate_path.first.empty() == private_key_and_certificate_path.second.empty(), "private key and certificate must be either both given or both empty");
  if (private_key_and_certificate_path.second.empty())
  {
    std::string pkey, cert;
    CHECK_AND_ASSERT_THROW_MES(create_ssl_certificate(pkey, cert), "Failed to create certificate");
    ssl_context.context.use_private_key(boost::asio::buffer(pkey), boost::asio::ssl::context::pem);
    ssl_context.context.use_certificate(boost::asio::buffer(cert), boost::asio::ssl::context::pem);
  }
  else
  {
    ssl_context.context.use_private_key_file(private_key_and_certificate_path.first, boost::asio::ssl::context::pem);
    ssl_context.context.use_certificate_file(private_key_and_certificate_path.second, boost::asio::ssl::context::pem);
  }
  ssl_context.allow_any_cert = allow_any_cert;

  return ssl_context;
}

void use_ssl_certificate(ssl_context_t &ssl_context, const std::pair<std::string, std::string> &private_key_and_certificate_path)
{
  ssl_context.context.use_private_key_file(private_key_and_certificate_path.first, boost::asio::ssl::context::pem);
  ssl_context.context.use_certificate_file(private_key_and_certificate_path.second, boost::asio::ssl::context::pem);
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

bool is_certificate_allowed(boost::asio::ssl::verify_context &ctx, const ssl_context_t &ssl_context)
{
  X509_STORE_CTX *sctx = ctx.native_handle();
  if (!sctx)
  {
    MERROR("Error getting verify_context handle");
    return false;
  }
  X509 *cert =X509_STORE_CTX_get_current_cert(sctx);
  if (!cert)
  {
    MERROR("No certificate found in verify_context");
    return false;
  }

  // can we check the certificate against a list of fingerprints?
  if (!ssl_context.allowed_fingerprints.empty()) {
    // buffer for the certificate digest and the size of the result
    std::vector<uint8_t> digest(EVP_MAX_MD_SIZE);
    unsigned int size{ 0 };

    // create the digest from the certificate
    if (!X509_digest(cert, EVP_sha1(), digest.data(), &size)) {
      MERROR("Failed to create certificate fingerprint");
      return false;
    }

    // strip unnecessary bytes from the digest
    digest.resize(size);

    // is the certificate fingerprint inside the list of allowed fingerprints?
    if (std::find(ssl_context.allowed_fingerprints.begin(), ssl_context.allowed_fingerprints.end(), digest) != ssl_context.allowed_fingerprints.end())
      return true;
  }

  if (!ssl_context.allowed_certificates.empty()) {
    BIO *bio_cert = BIO_new(BIO_s_mem());
    bool success = PEM_write_bio_X509(bio_cert, cert);
    if (!success)
    {
      BIO_free(bio_cert);
      MERROR("Failed to print certificate");
      return false;
    }
    BUF_MEM *buf = NULL;
    BIO_get_mem_ptr(bio_cert, &buf);
    if (!buf || !buf->data || !buf->length)
    {
      BIO_free(bio_cert);
      MERROR("Failed to write certificate: " << ERR_get_error());
      return false;
    }
    std::string certificate(std::string(buf->data, buf->length));
    BIO_free(bio_cert);
    if (std::find(ssl_context.allowed_certificates.begin(), ssl_context.allowed_certificates.end(), certificate) != ssl_context.allowed_certificates.end())
      return true;
  }

  // if either checklist is non-empty we must have failed it
  return ssl_context.allowed_fingerprints.empty() && ssl_context.allowed_certificates.empty();
}

bool ssl_handshake(boost::asio::ssl::stream<boost::asio::ip::tcp::socket> &socket, boost::asio::ssl::stream_base::handshake_type type, const epee::net_utils::ssl_context_t &ssl_context)
{
  bool verified = false;
  socket.next_layer().set_option(boost::asio::ip::tcp::no_delay(true));
  socket.set_verify_mode(boost::asio::ssl::verify_peer);
  socket.set_verify_callback([&](bool preverified, boost::asio::ssl::verify_context &ctx)
  {
    if (!preverified)
    {
      const int err = X509_STORE_CTX_get_error(ctx.native_handle());
      const int depth = X509_STORE_CTX_get_error_depth(ctx.native_handle());
      if (err != X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT || depth != 0)
      {
        MERROR("Invalid SSL certificate, error " << err << " at depth " << depth << ", connection dropped");
        return false;
      }
    }
    if (!ssl_context.allow_any_cert && !is_certificate_allowed(ctx, ssl_context))
    {
      MERROR("Certificate is not in the allowed list, connection droppped");
      return false;
    }
    verified = true;
    return true;
  });

  boost::system::error_code ec;
  socket.handshake(type, ec);
  if (ec)
  {
    MERROR("handshake failed, connection dropped: " << ec.message());
    return false;
  }
  if (!ssl_context.allow_any_cert && !verified)
  {
    MERROR("Peer did not provide a certificate in the allowed list, connection dropped");
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

