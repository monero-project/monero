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

#include <stdint.h>
#include <string>
#include <list>
#include <boost/utility/string_ref.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>

namespace epee
{
namespace net_utils
{
	enum class ssl_support_t: uint8_t {
		e_ssl_support_disabled,
		e_ssl_support_enabled,
		e_ssl_support_autodetect,
	};
  
	struct ssl_context_t
	{
		boost::asio::ssl::context context;
		std::list<std::string> allowed_certificates;
		std::vector<std::vector<uint8_t>> allowed_fingerprints;
		bool allow_any_cert;
	};

        // https://security.stackexchange.com/questions/34780/checking-client-hello-for-https-classification
	constexpr size_t get_ssl_magic_size() { return 9; }
	bool is_ssl(const unsigned char *data, size_t len);
	ssl_context_t create_ssl_context(const std::pair<std::string, std::string> &private_key_and_certificate_path, std::list<std::string> allowed_certificates, std::vector<std::vector<uint8_t>> allowed_fingerprints, bool allow_any_cert);
	void use_ssl_certificate(ssl_context_t &ssl_context, const std::pair<std::string, std::string> &private_key_and_certificate_path);
	bool create_ssl_certificate(std::string &pkey_buffer, std::string &cert_buffer);
	bool is_certificate_allowed(boost::asio::ssl::verify_context &ctx, const ssl_context_t &ssl_context);
	bool ssl_handshake(boost::asio::ssl::stream<boost::asio::ip::tcp::socket> &socket, boost::asio::ssl::stream_base::handshake_type type, const epee::net_utils::ssl_context_t &ssl_context);
        bool ssl_support_from_string(ssl_support_t &ssl, boost::string_ref s);
}
}

#endif //_NET_SSL_H
