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



#pragma once
#include <ctype.h>
#include <boost/shared_ptr.hpp>
#include <boost/regex.hpp>
#include <boost/optional/optional.hpp>
#include <boost/utility/string_ref.hpp>
#include <algorithm>
#include <cctype>
#include <functional>

#include "net_helper.h"
#include "http_client_base.h"
#include "string_tools.h"
#include "string_tools_lexical.h"
#include "reg_exp_definer.h"
#include "abstract_http_client.h"
#include "http_base.h" 
#include "http_auth.h"
#include "net_parse_helpers.h"
#include "syncobj.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.http"

namespace epee
{
namespace net_utils
{
	//---------------------------------------------------------------------------
	namespace http
	{

		template<typename net_client_type>
    class http_simple_client_template : public i_target_handler, public abstract_http_client
		{
		private:
			enum reciev_machine_state
			{
				reciev_machine_state_header,
				reciev_machine_state_body_content_len,
				reciev_machine_state_body_connection_close,
				reciev_machine_state_body_chunked,
				reciev_machine_state_done,
				reciev_machine_state_error
			};



			enum chunked_state{
				http_chunked_state_chunk_head,
				http_chunked_state_chunk_body,
				http_chunked_state_done,
				http_chunked_state_undefined
			};


			net_client_type m_net_client;
			std::string m_host_buff;      // Host IP address
			std::string m_virt_host_buff; // Value of 'Host:' header field
			std::string m_port;
			http_client_auth m_auth;
			std::string m_header_cache;
			http_response_info m_response_info;
			size_t m_len_in_summary;
			size_t m_len_in_remain;
			boost::shared_ptr<i_sub_handler> m_pcontent_encoding_handler;
			reciev_machine_state m_state;
			chunked_state m_chunked_state;
			std::string m_chunked_cache;
			bool m_auto_connect;
			critical_section m_lock;

		public:
			explicit http_simple_client_template()
				: i_target_handler(), abstract_http_client()
				, m_net_client()
				, m_host_buff()
				, m_virt_host_buff()
				, m_port()
				, m_auth()
				, m_header_cache()
				, m_response_info()
				, m_len_in_summary(0)
				, m_len_in_remain(0)
				, m_pcontent_encoding_handler(nullptr)
				, m_state()
				, m_chunked_state()
				, m_chunked_cache()
				, m_auto_connect(true)
				, m_lock()
			{}

			const std::string &get_host() const { return m_host_buff; };
			const std::string &get_port() const { return m_port; };

			using abstract_http_client::set_server;

			void set_server(std::string host, std::string port, boost::optional<login> user, ssl_options_t ssl_options = ssl_support_t::e_ssl_support_autodetect, const std::string& virtual_host = {}) override
			{
				CRITICAL_REGION_LOCAL(m_lock);
				disconnect();
				m_host_buff = std::move(host);
				m_virt_host_buff = !virtual_host.empty() ? virtual_host : m_host_buff; // Unless otherwise specified, virtual host matches host
				m_port = std::move(port);
				m_auth = user ? http_client_auth{std::move(*user)} : http_client_auth{};
				m_net_client.set_ssl(std::move(ssl_options));
			}

			void set_auto_connect(bool auto_connect) override
			{
				m_auto_connect = auto_connect;
			}

			template<typename F>
			void set_connector(F connector)
			{
				CRITICAL_REGION_LOCAL(m_lock);
				m_net_client.set_connector(std::move(connector));
			}

      bool connect(std::chrono::milliseconds timeout) override
      {
        CRITICAL_REGION_LOCAL(m_lock);
        return m_net_client.connect(m_host_buff, m_port, timeout);
      }
			//---------------------------------------------------------------------------
			bool disconnect() override
			{
				CRITICAL_REGION_LOCAL(m_lock);
				return m_net_client.disconnect();
			}
			//---------------------------------------------------------------------------
			bool is_connected(bool *ssl = NULL) override
			{
				CRITICAL_REGION_LOCAL(m_lock);
				return m_net_client.is_connected(ssl);
			}
			//---------------------------------------------------------------------------
			virtual bool handle_target_data(std::string& piece_of_transfer) override
			{
				CRITICAL_REGION_LOCAL(m_lock);
				m_response_info.m_body += piece_of_transfer;
        piece_of_transfer.clear();
				return true;
			}
			//---------------------------------------------------------------------------
			virtual bool on_header(const http_response_info &headers)
      {
        return true;
      }
			//---------------------------------------------------------------------------
			inline bool invoke(const boost::string_ref uri, const boost::string_ref method, const boost::string_ref body, std::chrono::milliseconds timeout, const http_response_info** ppresponse_info = NULL, const fields_list& additional_params = fields_list()) override
			{
				CRITICAL_REGION_LOCAL(m_lock);
				if(!is_connected())
				{
					if (!m_auto_connect)
					{
						MWARNING("Auto connect attempt to " << m_host_buff << ":" << m_port << " disabled");
						return false;
					}
					MDEBUG("Reconnecting...");
					if(!connect(timeout))
					{
						MDEBUG("Failed to connect to " << m_host_buff << ":" << m_port);
						return false;
					}
				}

				std::string req_buff{};
				req_buff.reserve(2048);
				req_buff.append(method.data(), method.size()).append(" ").append(uri.data(), uri.size()).append(" HTTP/1.1\r\n");
				add_field(req_buff, "Host", m_virt_host_buff);
				add_field(req_buff, "Content-Length", std::to_string(body.size()));

				//handle "additional_params"
				for(const auto& field : additional_params)
					add_field(req_buff, field);

				for (unsigned sends = 0; sends < 2; ++sends)
				{
					const std::size_t initial_size = req_buff.size();
					const auto auth = m_auth.get_auth_field(method, uri);
					if (auth)
						add_field(req_buff, *auth);

					req_buff += "\r\n";
					//--

					bool res = m_net_client.send(req_buff, timeout);
					CHECK_AND_ASSERT_MES(res, false, "HTTP_CLIENT: Failed to SEND");
					if(body.size())
						res = m_net_client.send(body, timeout);
					CHECK_AND_ASSERT_MES(res, false, "HTTP_CLIENT: Failed to SEND");

					m_response_info.clear();
					m_state = reciev_machine_state_header;
					if (!handle_reciev(timeout))
						return false;
					if (m_response_info.m_response_code != 401)
					{
						if(ppresponse_info)
							*ppresponse_info = std::addressof(m_response_info);
						return true;
					}

					switch (m_auth.handle_401(m_response_info))
					{
					case http_client_auth::kSuccess:
						break;
					case http_client_auth::kBadPassword:
                                                sends = 2;
						break;
					default:
					case http_client_auth::kParseFailure:
						LOG_ERROR("Bad server response for authentication");
						return false;
					}
					req_buff.resize(initial_size); // rollback for new auth generation
				}
				LOG_ERROR("Client has incorrect username/password for server requiring authentication");
				return false;
			}
			//---------------------------------------------------------------------------
			bool test(const std::string &s, std::chrono::milliseconds timeout) // TEST FUNC ONLY
			{
				CRITICAL_REGION_LOCAL(m_lock);
				m_net_client.set_test_data(s);
				m_state = reciev_machine_state_header;
				return handle_reciev(timeout);
			}
			//---------------------------------------------------------------------------
			uint64_t get_bytes_sent() const override
			{
				return m_net_client.get_bytes_sent();
			}
			//---------------------------------------------------------------------------
			uint64_t get_bytes_received() const override
			{
				return m_net_client.get_bytes_received();
			}
			//---------------------------------------------------------------------------
			void wipe_response()
			{
				m_response_info.wipe();
			}
			//---------------------------------------------------------------------------
		private: 
			//---------------------------------------------------------------------------
			inline bool handle_reciev(std::chrono::milliseconds timeout)
			{
				CRITICAL_REGION_LOCAL(m_lock);
				bool keep_handling = true;
				bool need_more_data = true;
				std::string recv_buffer;
				while(keep_handling)
				{
					if(need_more_data)
					{
						if(!m_net_client.recv(recv_buffer, timeout))
						{
							MERROR("Unexpected recv fail");
							m_state = reciev_machine_state_error;
            }
            if(!recv_buffer.size())
            {
              //connection is going to be closed
              if(reciev_machine_state_body_connection_close != m_state)
              {
                m_state = reciev_machine_state_error;
              }
            }
            need_more_data = false;
					}
					switch(m_state)
					{
					case reciev_machine_state_header:
						keep_handling = handle_header(recv_buffer, need_more_data);
						break;
					case reciev_machine_state_body_content_len:
						keep_handling = handle_body_content_len(recv_buffer, need_more_data);
						break;
					case reciev_machine_state_body_connection_close:
						keep_handling = handle_body_connection_close(recv_buffer, need_more_data);
						break;
					case reciev_machine_state_body_chunked:
						keep_handling = handle_body_body_chunked(recv_buffer, need_more_data);
						break;
					case reciev_machine_state_done:
						keep_handling = false;
						break;
					case reciev_machine_state_error:
						keep_handling = false;
						break;
					}

				}
				m_header_cache.clear();
				if(m_state != reciev_machine_state_error)
				{
					if(m_response_info.m_header_info.m_connection.size() && !string_tools::compare_no_case("close", m_response_info.m_header_info.m_connection))
						disconnect();

					return true;
				}
				else
                {
                  LOG_PRINT_L3("Returning false because of wrong state machine. state: " << m_state);
                  return false;
                }
			}
			//---------------------------------------------------------------------------
			inline
				bool handle_header(std::string& recv_buff, bool& need_more_data)
			{
 
				CRITICAL_REGION_LOCAL(m_lock);
        if(!recv_buff.size())
        {
          LOG_ERROR("Connection closed at handle_header");
          m_state = reciev_machine_state_error;
          return false;
        }

				m_header_cache += recv_buff;
				recv_buff.clear();
				std::string::size_type pos = m_header_cache.find("\r\n\r\n");
				if(pos != std::string::npos)
				{
					recv_buff.assign(m_header_cache.begin()+pos+4, m_header_cache.end());
					m_header_cache.erase(m_header_cache.begin()+pos+4, m_header_cache.end());

					analize_cached_header_and_invoke_state();
          if (!on_header(m_response_info))
          {
            MDEBUG("Connection cancelled by on_header");
            m_state = reciev_machine_state_done;
            return false;
          }
					m_header_cache.clear();
					if(!recv_buff.size() && (m_state != reciev_machine_state_error && m_state != reciev_machine_state_done))
						need_more_data = true;

					return true;
				}else
					need_more_data = true;
				return true;
			}
			//---------------------------------------------------------------------------
			inline
				bool handle_body_content_len(std::string& recv_buff, bool& need_more_data)
			{
				CRITICAL_REGION_LOCAL(m_lock);
				if(!recv_buff.size())
				{
					MERROR("Warning: Content-Len mode, but connection unexpectedly closed");
					m_state = reciev_machine_state_done;
					return true;
				}
				CHECK_AND_ASSERT_MES(m_len_in_remain >= recv_buff.size(), false, "m_len_in_remain >= recv_buff.size()");
				m_len_in_remain -= recv_buff.size();
				if (!m_pcontent_encoding_handler->update_in(recv_buff))
				{
					m_state = reciev_machine_state_done;
					return false;
				}

				if(m_len_in_remain == 0)
					m_state = reciev_machine_state_done;
				else
					need_more_data = true;

				return true;
			}
			//---------------------------------------------------------------------------
			inline
				bool handle_body_connection_close(std::string& recv_buff, bool& need_more_data)
			{
				CRITICAL_REGION_LOCAL(m_lock);
				if(!recv_buff.size())
				{
					m_state = reciev_machine_state_done;
					return true;
				}
        need_more_data = true;
				m_pcontent_encoding_handler->update_in(recv_buff);


				return true;
			}
			//---------------------------------------------------------------------------
			inline bool is_hex_symbol(char ch)
			{

				if( (ch >= '0' && ch <='9')||(ch >= 'A' && ch <='F')||(ch >= 'a' && ch <='f'))
					return true;
				else
					return false;
			}
			//---------------------------------------------------------------------------
			inline
				bool get_len_from_chunk_head(const std::string &chunk_head, size_t& result_size)
			{
				std::stringstream str_stream;
				str_stream << std::hex;
				if(!(str_stream << chunk_head && str_stream >> result_size))
					return false;

				return true;
			}
			//---------------------------------------------------------------------------
			inline
				bool get_chunk_head(std::string& buff, size_t& chunk_size, bool& is_matched)
			{
				is_matched = false;
				size_t offset = 0;
				for(std::string::iterator it = buff.begin(); it!= buff.end(); it++, offset++)
				{
					if(!is_hex_symbol(*it))
					{
						if(*it == '\r' || *it == ' ' )
						{
							offset--;
							continue;
						}
						else if(*it == '\n')
						{	
							std::string chunk_head = buff.substr(0, offset);
							if(!get_len_from_chunk_head(chunk_head, chunk_size))
								return false;

							if(0 == chunk_size)
							{
								//Here is a small confusion
								//In brief - if the chunk is the last one we need to get terminating sequence
								//along with the cipher, generally in the "ddd\r\n\r\n" form

								for(it++;it != buff.end(); it++)
								{
									if('\r' == *it)
										continue;
									else if('\n' == *it)
										break;
									else
									{
										LOG_ERROR("http_stream_filter: Wrong last chunk terminator");
										return false;
									}
								}

								if(it == buff.end())
									return true;
							}

							buff.erase(buff.begin(), ++it);

							is_matched = true;				
							return true;
						}
						else
							return false;
					}
				}

				return true;
			}
			//---------------------------------------------------------------------------
			inline
				bool handle_body_body_chunked(std::string& recv_buff, bool& need_more_data)
			{
        CRITICAL_REGION_LOCAL(m_lock);
				if(!recv_buff.size())
				{
					MERROR("Warning: CHUNKED mode, but connection unexpectedly closed");
					m_state = reciev_machine_state_done;
					return true;
				}
				m_chunked_cache += recv_buff;
				recv_buff.clear();
				bool is_matched = false;

				while(true)
				{
					if(!m_chunked_cache.size())
					{
						need_more_data = true;
						break;
					}

					switch(m_chunked_state)
					{
					case http_chunked_state_chunk_head:
						if(m_chunked_cache[0] == '\n' || m_chunked_cache[0] == '\r')
						{
							//optimize a bit
							if(m_chunked_cache[0] == '\r' && m_chunked_cache.size()>1 && m_chunked_cache[1] == '\n')
								m_chunked_cache.erase(0, 2);
							else
								m_chunked_cache.erase(0, 1);
							break;
						}
						if(!get_chunk_head(m_chunked_cache, m_len_in_remain, is_matched))
						{
							LOG_ERROR("http_stream_filter::handle_chunked(*) Failed to get length from chunked head:" << m_chunked_cache);
							m_state = reciev_machine_state_error;
							return false;
						}

						if(!is_matched)
						{
							need_more_data = true;
							return true;
						}else
						{
							m_chunked_state = http_chunked_state_chunk_body;
							if(m_len_in_remain == 0)
							{//last chunk, let stop the stream and fix the chunk queue.
								m_state = reciev_machine_state_done;
								return true;
							}
							m_chunked_state = http_chunked_state_chunk_body;
							break;
						}
						break;
					case http_chunked_state_chunk_body:
						{
							std::string chunk_body;
							if(m_len_in_remain >= m_chunked_cache.size())
							{
								m_len_in_remain -= m_chunked_cache.size();
								chunk_body.swap(m_chunked_cache);
							}else
							{
								chunk_body.assign(m_chunked_cache, 0, m_len_in_remain);
								m_chunked_cache.erase(0, m_len_in_remain);
								m_len_in_remain = 0;
							}

							if (!m_pcontent_encoding_handler->update_in(chunk_body))
							{
								m_state = reciev_machine_state_error;
								return false;
							}

							if(!m_len_in_remain)
								m_chunked_state = http_chunked_state_chunk_head;
						}
						break;
					case http_chunked_state_done:
						m_state = reciev_machine_state_done;
						return true;
					case http_chunked_state_undefined:
					default:
						LOG_ERROR("http_stream_filter::handle_chunked(): Wrong state" << m_chunked_state);
						return false;
					}
				}

				return true;
			}
			//---------------------------------------------------------------------------
			inline bool parse_header(http_header_info& body_info, const std::string& m_cache_to_process)
			{
				MTRACE("http_stream_filter::parse_cached_header(*)");

				const char *ptr = m_cache_to_process.c_str();
				while (ptr[0] != '\r' || ptr[1] != '\n')
				{
					// optional \n
					if (*ptr == '\n')
						++ptr;
					// an identifier composed of letters or -
					const char *key_pos = ptr;
					while (isalnum(*ptr) || *ptr == '_' || *ptr == '-')
						++ptr;
					const char *key_end = ptr;
					// optional space (not in RFC, but in previous code)
					if (*ptr == ' ')
						++ptr;
					CHECK_AND_ASSERT_MES(*ptr == ':', true, "http_stream_filter::parse_cached_header() invalid header in: " << m_cache_to_process);
					++ptr;
					// optional whitespace, but not newlines - line folding is obsolete, let's ignore it
					while (isblank(*ptr))
						++ptr;
					const char *value_pos = ptr;
					while (*ptr != '\r' && *ptr != '\n')
						++ptr;
					const char *value_end = ptr;
					// optional trailing whitespace
					while (value_end > value_pos && isblank(*(value_end-1)))
						--value_end;
					if (*ptr == '\r')
						++ptr;
					CHECK_AND_ASSERT_MES(*ptr == '\n', true, "http_stream_filter::parse_cached_header() invalid header in: " << m_cache_to_process);
					++ptr;

					const std::string key = std::string(key_pos, key_end - key_pos);
					const std::string value = std::string(value_pos, value_end - value_pos);
					if (!key.empty())
					{
						if (!string_tools::compare_no_case(key, "Connection"))
							body_info.m_connection = value;
						else if(!string_tools::compare_no_case(key, "Referrer"))
							body_info.m_referer = value;
						else if(!string_tools::compare_no_case(key, "Content-Length"))
							body_info.m_content_length = value;
						else if(!string_tools::compare_no_case(key, "Content-Type"))
							body_info.m_content_type = value;
						else if(!string_tools::compare_no_case(key, "Transfer-Encoding"))
							body_info.m_transfer_encoding = value;
						else if(!string_tools::compare_no_case(key, "Content-Encoding"))
							body_info.m_content_encoding = value;
						else if(!string_tools::compare_no_case(key, "Host"))
							body_info.m_host = value;
						else if(!string_tools::compare_no_case(key, "Cookie"))
							body_info.m_cookie = value;
						else if(!string_tools::compare_no_case(key, "User-Agent"))
							body_info.m_user_agent = value;
						else if(!string_tools::compare_no_case(key, "Origin"))
							body_info.m_origin = value;
						else
							body_info.m_etc_fields.emplace_back(key, value);
					}
				}
				return true;
			}
			//---------------------------------------------------------------------------
			inline bool analize_first_response_line()
			{
				//First line response, look like this:	"HTTP/1.1 200 OK"
				const char *ptr = m_header_cache.c_str();
				CHECK_AND_ASSERT_MES(!memcmp(ptr, "HTTP/", 5), false, "Invalid first response line: " + m_header_cache);
				ptr += 5;
				CHECK_AND_ASSERT_MES(epee::misc_utils::parse::isdigit(*ptr), false, "Invalid first response line: " + m_header_cache);
				unsigned long ul;
				char *end;
				ul = strtoul(ptr, &end, 10);
				CHECK_AND_ASSERT_MES(ul <= INT_MAX && *end =='.', false, "Invalid first response line: " + m_header_cache);
				m_response_info.m_http_ver_hi = ul;
				ptr = end + 1;
				CHECK_AND_ASSERT_MES(epee::misc_utils::parse::isdigit(*ptr), false, "Invalid first response line: " + m_header_cache + ", ptr: " << ptr);
				ul = strtoul(ptr, &end, 10);
				CHECK_AND_ASSERT_MES(ul <= INT_MAX && isblank(*end), false, "Invalid first response line: " + m_header_cache + ", ptr: " << ptr);
				m_response_info.m_http_ver_lo = ul;
				ptr = end + 1;
				while (isblank(*ptr))
					++ptr;
				CHECK_AND_ASSERT_MES(epee::misc_utils::parse::isdigit(*ptr), false, "Invalid first response line: " + m_header_cache);
				ul = strtoul(ptr, &end, 10);
				CHECK_AND_ASSERT_MES(ul >= 100 && ul <= 999 && isspace(*end), false, "Invalid first response line: " + m_header_cache);
				m_response_info.m_response_code = ul;
				ptr = end;
				// ignore the optional text, till the end
				while (*ptr != '\r' && *ptr != '\n')
					++ptr;
				if (*ptr == '\r')
					++ptr;
				CHECK_AND_ASSERT_MES(*ptr == '\n', false, "Invalid first response line: " << m_header_cache);
				++ptr;

				m_header_cache.erase(0, ptr - m_header_cache.c_str());
				return true;
			}
			inline
				bool set_reply_content_encoder()
			{
				STATIC_REGEXP_EXPR_1(rexp_match_gzip, "^.*?((gzip)|(deflate))", boost::regex::icase | boost::regex::normal);
				boost::smatch result;						//   12      3
				if(boost::regex_search( m_response_info.m_header_info.m_content_encoding, result, rexp_match_gzip, boost::match_default) && result[0].matched)
				{
          m_pcontent_encoding_handler.reset(new do_nothing_sub_handler(this));
          LOG_ERROR("GZIP encoding not supported");
          return false;
				}
				else 
				{
					m_pcontent_encoding_handler.reset(new do_nothing_sub_handler(this));
				}

				return true;
			}
			inline	
				bool analize_cached_header_and_invoke_state()
			{
				m_response_info.clear();
				analize_first_response_line();
				std::string fake_str; //gcc error workaround

				bool res = parse_header(m_response_info.m_header_info, m_header_cache);
				CHECK_AND_ASSERT_MES(res, false, "http_stream_filter::analize_cached_reply_header_and_invoke_state(): failed to anilize reply header: " << m_header_cache);

				set_reply_content_encoder();

				m_len_in_summary = 0;
				bool content_len_valid = false;
				if(m_response_info.m_header_info.m_content_length.size())
					content_len_valid = string_tools::get_xtype_from_string(m_len_in_summary, m_response_info.m_header_info.m_content_length);



				if(!m_len_in_summary && ((m_response_info.m_response_code>=100&&m_response_info.m_response_code<200) 
					|| 204 == m_response_info.m_response_code 
					|| 304 == m_response_info.m_response_code) )
				{//There will be no response body, server will display the local page with error
					m_state = reciev_machine_state_done;
					return true;
				}else if(m_response_info.m_header_info.m_transfer_encoding.size())
				{
					string_tools::trim(m_response_info.m_header_info.m_transfer_encoding);
					if(string_tools::compare_no_case(m_response_info.m_header_info.m_transfer_encoding, "chunked"))
					{
						LOG_ERROR("Wrong Transfer-Encoding:" << m_response_info.m_header_info.m_transfer_encoding);
						m_state = reciev_machine_state_error;
						return false;
					}
					m_state = reciev_machine_state_body_chunked;
					m_chunked_state = http_chunked_state_chunk_head;
					return true;
				}
				else if(!m_response_info.m_header_info.m_content_length.empty())
				{ 
					//In the response header the length was specified
					if(!content_len_valid)
					{
						LOG_ERROR("http_stream_filter::analize_cached_reply_header_and_invoke_state(): Failed to get_len_from_content_lenght();, m_query_info.m_content_length="<<m_response_info.m_header_info.m_content_length);
						m_state = reciev_machine_state_error;
						return false;
					}
					if(!m_len_in_summary)
					{
						m_state = reciev_machine_state_done;
						return true;
					}
					else
					{
						m_len_in_remain = m_len_in_summary;
						m_state = reciev_machine_state_body_content_len;
						return true;
					}
				}else if(!m_response_info.m_header_info.m_connection.empty() && is_connection_close_field(m_response_info.m_header_info.m_connection))
				{   //By indirect signs we suspect that data transfer will end with a connection break
					m_state = reciev_machine_state_body_connection_close;
				}else if(is_multipart_body(m_response_info.m_header_info, fake_str))
				{
					m_state = reciev_machine_state_error;
					LOG_ERROR("Unsupported MULTIPART BODY.");
					return false;
				}else
				{   //Apparently there are no signs of the form of transfer, will receive data until the connection is closed
					m_state = reciev_machine_state_error;
					MERROR("Undefined transfer type, consider http_body_transfer_connection_close method. header: " << m_header_cache);
					return false;
				} 
				return false;
			}
			inline 
				bool is_connection_close_field(const std::string& str)
			{
				STATIC_REGEXP_EXPR_1(rexp_match_close, "^\\s*close", boost::regex::icase | boost::regex::normal);
				boost::smatch result;
				if(boost::regex_search( str, result, rexp_match_close, boost::match_default) && result[0].matched)
					return true;
				else
					return false;
			}
			inline
				bool is_multipart_body(const http_header_info& head_info, OUT std::string& boundary)
			{
				//Check whether this is multi part - if yes, capture boundary immediately
				STATIC_REGEXP_EXPR_1(rexp_match_multipart_type, "^\\s*multipart/([\\w\\-]+); boundary=((\"(.*?)\")|(\\\\\"(.*?)\\\\\")|([^\\s;]*))", boost::regex::icase | boost::regex::normal);
				boost::smatch result;
				if(boost::regex_search(head_info.m_content_type, result, rexp_match_multipart_type, boost::match_default) && result[0].matched)
				{
					if(result[4].matched)
						boundary = result[4];
					else if(result[6].matched)
						boundary = result[6];
					else if(result[7].matched)
						boundary = result[7];
					else 
					{
						LOG_ERROR("Failed to match boundary in content-type=" << head_info.m_content_type);
						return false;
					}
					return true;
				}
				else
					return false;

				return true;
			}
		};
		typedef http_simple_client_template<blocked_mode_client> http_simple_client;
	}
}
}
