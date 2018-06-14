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

#include <boost/asio/ssl.hpp>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include "misc_log_ex.h"
#include "net/net_ssl.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net"

// ################################################################################################
// local (TU local) headers
// ################################################################################################

// openssl genrsa -out /tmp/KEY 4096
// openssl req -new -key /tmp/KEY -out /tmp/REQ
// openssl x509 -req -days 999999 -sha256 -in /tmp/REQ -signkey /tmp/KEY -out /tmp/CERT

namespace epee
{
namespace net_utils
{

// https://stackoverflow.com/questions/256405/programmatically-create-x509-certificate-using-openssl
bool create_ssl_certificate(EVP_PKEY *&pkey, X509 *&cert)
{
  MGINFO("Generating SSL certificate");
  pkey = EVP_PKEY_new();
  if (!pkey)
  {
    MERROR("Failed to create new private key");
    return false;
  }
  RSA *rsa = RSA_generate_key(4096, RSA_F4, NULL, NULL);
  if (!rsa)
  {
    MERROR("Error generating RSA private key");
    EVP_PKEY_free(pkey);
    return NULL;
  }
  EVP_PKEY_assign_RSA(pkey, rsa);

  cert = X509_new();
  if (!cert)
  {
    MERROR("Failed to create new X509 certificate");
    EVP_PKEY_free(pkey);
    return false;
  }
  ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);
  X509_gmtime_adj(X509_get_notBefore(cert), 0);
  X509_gmtime_adj(X509_get_notAfter(cert), 3600 * 24 * 365 * 10); // 10 years
  X509_set_pubkey(cert, pkey);
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
  bool success = PEM_write_bio_PrivateKey(bio_pkey, pkey, NULL, NULL, 0, NULL, NULL) && PEM_write_bio_X509(bio_cert, cert);
  EVP_PKEY_free(pkey);
  X509_free(cert);
  if (!success)
  {
    BIO_free(bio_pkey);
    BIO_free(bio_cert);
    MERROR("Failed to write cert and/or pkey: " << ERR_get_error());
    return false;
  }
  BUF_MEM *buf = NULL;
  BIO_get_mem_ptr(bio_pkey, &buf);
  if (!buf || !buf->data || !buf->length)
  {
    BIO_free(bio_pkey);
    BIO_free(bio_cert);
    MERROR("Failed to write pkey: " << ERR_get_error());
    return false;
  }
  pkey_buffer = std::string(buf->data, buf->length);
  buf = NULL;
  BIO_get_mem_ptr(bio_cert, &buf);
  if (!buf || !buf->data || !buf->length)
  {
    BIO_free(bio_pkey);
    BIO_free(bio_cert);
    MERROR("Failed to write cert: " << ERR_get_error());
    return false;
  }
  cert_buffer = std::string(buf->data, buf->length);
  return success;
}

ssl_context_t create_ssl_context(const std::string &private_key_path, const std::string &certificate_path, std::list<std::string> allowed_certificates)
{
  ssl_context_t ssl_context({boost::asio::ssl::context(boost::asio::ssl::context::sslv23), std::move(allowed_certificates)});

  // disable sslv2
  ssl_context.context.set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2);
  ssl_context.context.set_default_verify_paths();
  CHECK_AND_ASSERT_THROW_MES(private_key_path.empty() == certificate_path.empty(), "private key and certificate must be either both given or both empty");
  if (certificate_path.empty())
  {
    std::string pkey, cert;
    CHECK_AND_ASSERT_THROW_MES(create_ssl_certificate(pkey, cert), "Failed to create certificate");
    ssl_context.context.use_private_key(boost::asio::buffer(pkey), boost::asio::ssl::context::pem);
    ssl_context.context.use_certificate(boost::asio::buffer(cert), boost::asio::ssl::context::pem);
  }
  else
  {
    ssl_context.context.use_private_key_file(private_key_path, boost::asio::ssl::context::pem);
    ssl_context.context.use_certificate_file(certificate_path, boost::asio::ssl::context::pem);
  }

  return ssl_context;
}

void use_ssl_certificate(ssl_context_t &ssl_context, const std::string &private_key_path, const std::string &certificate_path)
{
  ssl_context.context.use_private_key_file(private_key_path, boost::asio::ssl::context::pem);
  ssl_context.context.use_certificate_file(certificate_path, boost::asio::ssl::context::pem);
}

bool is_ssl(const unsigned char *data, size_t len)
{
  // https://security.stackexchange.com/questions/34780/checking-client-hello-for-https-classification
  MDEBUG("SSL detection buffer, " << len << " bytes: "
    << (unsigned)(unsigned char)data[0] << " " << (unsigned)(unsigned char)data[1] << " "
    << (unsigned)(unsigned char)data[2] << " " << (unsigned)(unsigned char)data[3] << " "
    << (unsigned)(unsigned char)data[4] << " " << (unsigned)(unsigned char)data[5] << " "
    << (unsigned)(unsigned char)data[6] << " " << (unsigned)(unsigned char)data[7] << " "
    << (unsigned)(unsigned char)data[8]);
  if (len >= 9)
  if (data[0] == 0x16) // record
  if (data[1] == 3) // major version
  if (data[5] == 1) // ClientHello
  if (data[6] == 0 && data[3]*256 + data[4] == data[7]*256 + data[8] + 4) // length check
    return true;
  return false;
}

bool is_certificate_allowed(boost::asio::ssl::verify_context &ctx, const std::list<std::string> &allowed_certificates)
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
  return std::find(allowed_certificates.begin(), allowed_certificates.end(), certificate) != allowed_certificates.end();
}

bool ssl_handshake(boost::asio::ssl::stream<boost::asio::ip::tcp::socket> &socket, boost::asio::ssl::stream_base::handshake_type type, const epee::net_utils::ssl_context_t &ssl_context)
{
  bool verified = false;
  socket.next_layer().set_option(boost::asio::ip::tcp::no_delay(true));
  socket.set_verify_mode(boost::asio::ssl::verify_peer);
  socket.set_verify_depth(1);
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
    if (!ssl_context.allowed_certificates.empty())
    {
      if (!is_certificate_allowed(ctx, ssl_context.allowed_certificates))
      {
        MERROR("Certificate is not in the allowed list, connection droppped");
        return false;
      }
    }
    verified = true;
    return true;
  });

  boost::system::error_code ec;
  socket.handshake(type, ec);
  if (ec)
  {
    MERROR("handshake failed, connection dropped");
    return false;
  }
  if (!ssl_context.allowed_certificates.empty() && !verified)
  {
    MERROR("Peer did not provide a certificate in the allowed list, connection dropped");
    return false;
  }
  MDEBUG("SSL handshake success");
  return true;
}

} // namespace
} // namespace

