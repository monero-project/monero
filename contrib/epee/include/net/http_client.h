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
#include <boost/shared_ptr.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional/optional.hpp>
#include <boost/utility/string_ref.hpp>
//#include <mbstring.h>
#include <algorithm>
#include <cctype>
#include <functional>

#include "net_helper.h"
#include "http_client_base.h"

#ifdef HTTP_ENABLE_GZIP
#include "gzip_encoding.h"
#endif 

#include "string_tools.h"
#include "reg_exp_definer.h"
#include "http_base.h" 
#include "http_auth.h"
#include "to_nonconst_iterator.h"
#include "net_parse_helpers.h"

//#include "shlwapi.h"

//#pragma comment(lib, "shlwapi.lib")

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.http"

extern epee::critical_section gregexp_lock;


namespace epee
{
namespace net_utils
{

using namespace std; 

	/*struct url 
	{
	public:
		void parse(const std::string& url_s)
		{
			const string prot_end("://");
			string::const_iterator prot_i = search(url_s.begin(), url_s.end(),
				prot_end.begin(), prot_end.end());
			protocol_.reserve(distance(url_s.begin(), prot_i));
			transform(url_s.begin(), prot_i,
				back_inserter(protocol_),
				ptr_fun<int,int>(tolower)); // protocol is icase
			if( prot_i == url_s.end() )
				return;
			advance(prot_i, prot_end.length());
			string::const_iterator path_i = find(prot_i, url_s.end(), '/');
			host_.reserve(distance(prot_i, path_i));
			transform(prot_i, path_i,
				back_inserter(host_),
				ptr_fun<int,int>(tolower)); // host is icase
			string::const_iterator query_i = find(path_i, url_s.end(), '?');
			path_.assign(path_i, query_i);
			if( query_i != url_s.end() )
				++query_i;
			query_.assign(query_i, url_s.end());
		}

		std::string protocol_;
		std::string host_;
		std::string path_;
		std::string query_;
	};*/




	//---------------------------------------------------------------------------
	static inline const char* get_hex_vals()
	{
		static const char hexVals[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
		return hexVals;
	}

	static inline const char* get_unsave_chars()
	{
		//static char unsave_chars[] = "\"<>%\\^[]`+$,@:;/!#?=&";
		static const char unsave_chars[] = "\"<>%\\^[]`+$,@:;!#&";
		return unsave_chars;
	}

	static inline bool is_unsafe(unsigned char compare_char)
	{
		if(compare_char <= 32 || compare_char >= 123)
			return true;

		const char* punsave = get_unsave_chars();

		for(int ichar_pos = 0; 0!=punsave[ichar_pos] ;ichar_pos++)
			if(compare_char == punsave[ichar_pos])
				return true;

		return false;
	}
	
	static inline 
		std::string dec_to_hex(char num, int radix)
	{
		int temp=0;
		std::string csTmp;
		int num_char;

		num_char = (int) num;
		if (num_char < 0)
			num_char = 256 + num_char;

		while (num_char >= radix)
		{
			temp = num_char % radix;
			num_char = (int)floor((float)num_char / (float)radix);
			csTmp = get_hex_vals()[temp];
		}

		csTmp += get_hex_vals()[num_char];

		if(csTmp.size() < 2)
		{
			csTmp += '0';
		}
		
		std::reverse(csTmp.begin(), csTmp.end());
		//_mbsrev((unsigned char*)csTmp.data());

		return csTmp;
	}
	static inline int get_index(const char *s, char c) { const char *ptr = (const char*)memchr(s, c, 16); return ptr ? ptr-s : -1; }
	static inline 
		std::string hex_to_dec_2bytes(const char *s)
	{
		const char *hex = get_hex_vals();
		int i0 = get_index(hex, toupper(s[0]));
		int i1 = get_index(hex, toupper(s[1]));
		if (i0 < 0 || i1 < 0)
			return std::string("%") + std::string(1, s[0]) + std::string(1, s[1]);
		return std::string(1, i0 * 16 | i1);
	}

	static inline std::string convert(char val)
	{
		std::string csRet;
		csRet += "%";
		csRet += dec_to_hex(val, 16);
		return  csRet;
	}
	static inline std::string conver_to_url_format(const std::string& uri)
	{

		std::string result;

		for(size_t i = 0; i!= uri.size(); i++)
		{
			if(is_unsafe(uri[i]))
				result += convert(uri[i]);
			else
				result += uri[i];

		}

		return result;
	}
	static inline std::string convert_from_url_format(const std::string& uri)
	{

		std::string result;

		for(size_t i = 0; i!= uri.size(); i++)
		{
			if(uri[i] == '%' && i + 2 < uri.size())
			{
				result += hex_to_dec_2bytes(uri.c_str() + i + 1);
				i += 2;
			}
			else
				result += uri[i];

		}

		return result;
	}

  static inline std::string convert_to_url_format_force_all(const std::string& uri)
  {

    std::string result;

    for(size_t i = 0; i!= uri.size(); i++)
    {
        result += convert(uri[i]);
    }

    return result;
  }





	namespace http
	{

		class http_simple_client: public i_target_handler
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


			blocked_mode_client m_net_client;
			std::string m_host_buff;
			std::string m_port;
			http_client_auth m_auth;
			std::string m_header_cache;
			http_response_info m_response_info;
			size_t m_len_in_summary;
			size_t m_len_in_remain;
			//std::string* m_ptarget_buffer;
			boost::shared_ptr<i_sub_handler> m_pcontent_encoding_handler;
			reciev_machine_state m_state;
			chunked_state m_chunked_state;
			std::string m_chunked_cache;
			critical_section m_lock;

		public:
			explicit http_simple_client()
				: i_target_handler()
				, m_net_client()
				, m_host_buff()
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
				, m_lock()
			{}

			const std::string &get_host() const { return m_host_buff; };
			const std::string &get_port() const { return m_port; };

			bool set_server(const std::string& address, boost::optional<login> user)
			{
				http::url_content parsed{};
				const bool r = parse_url(address, parsed);
				CHECK_AND_ASSERT_MES(r, false, "failed to parse url: " << address);
				set_server(std::move(parsed.host), std::to_string(parsed.port), std::move(user));
				return true;
			}

			void set_server(std::string host, std::string port, boost::optional<login> user)
			{
				CRITICAL_REGION_LOCAL(m_lock);
				disconnect();
				m_host_buff = std::move(host);
				m_port = std::move(port);
                                m_auth = user ? http_client_auth{std::move(*user)} : http_client_auth{};
			}

      bool connect(std::chrono::milliseconds timeout)
      {
        CRITICAL_REGION_LOCAL(m_lock);
        return m_net_client.connect(m_host_buff, m_port, timeout);
      }
			//---------------------------------------------------------------------------
			bool disconnect()
			{
				CRITICAL_REGION_LOCAL(m_lock);
				return m_net_client.disconnect();
			}
			//---------------------------------------------------------------------------
			bool is_connected()
			{
				CRITICAL_REGION_LOCAL(m_lock);
				return m_net_client.is_connected();
			}
			//---------------------------------------------------------------------------
			virtual bool handle_target_data(std::string& piece_of_transfer)
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
			inline 
				bool invoke_get(const boost::string_ref uri, std::chrono::milliseconds timeout, const std::string& body = std::string(), const http_response_info** ppresponse_info = NULL, const fields_list& additional_params = fields_list())
			{
					CRITICAL_REGION_LOCAL(m_lock);
					return invoke(uri, "GET", body, timeout, ppresponse_info, additional_params);
			}

			//---------------------------------------------------------------------------
			inline bool invoke(const boost::string_ref uri, const boost::string_ref method, const std::string& body, std::chrono::milliseconds timeout, const http_response_info** ppresponse_info = NULL, const fields_list& additional_params = fields_list())
			{
				CRITICAL_REGION_LOCAL(m_lock);
				if(!is_connected())
				{
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
				add_field(req_buff, "Host", m_host_buff);
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
			inline bool invoke_post(const boost::string_ref uri, const std::string& body, std::chrono::milliseconds timeout, const http_response_info** ppresponse_info = NULL, const fields_list& additional_params = fields_list())
			{
				CRITICAL_REGION_LOCAL(m_lock);
				return invoke(uri, "POST", body, timeout, ppresponse_info, additional_params);
			}
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
			inline
				bool parse_header(http_header_info& body_info, const std::string& m_cache_to_process)
			{ 
				MTRACE("http_stream_filter::parse_cached_header(*)");
				
				STATIC_REGEXP_EXPR_1(rexp_mach_field, 
					"\n?((Connection)|(Referer)|(Content-Length)|(Content-Type)|(Transfer-Encoding)|(Content-Encoding)|(Host)|(Cookie)|(User-Agent)"
					//  12            3         4                5              6                   7                  8      9        10
					"|([\\w-]+?)) ?: ?((.*?)(\r?\n))[^\t ]",	
					//11             1213   14
					boost::regex::icase | boost::regex::normal);

				boost::smatch		result;
				std::string::const_iterator it_current_bound = m_cache_to_process.begin();
				std::string::const_iterator it_end_bound = m_cache_to_process.end();



				//lookup all fields and fill well-known fields
				while( boost::regex_search( it_current_bound, it_end_bound, result, rexp_mach_field, boost::match_default) && result[0].matched) 
				{
					const size_t field_val = 13;
					//const size_t field_etc_name = 11;

					int i = 2; //start position = 2
					if(result[i++].matched)//"Connection"
						body_info.m_connection = result[field_val];
					else if(result[i++].matched)//"Referrer"
						body_info.m_referer = result[field_val];
					else if(result[i++].matched)//"Content-Length"
						body_info.m_content_length = result[field_val];
					else if(result[i++].matched)//"Content-Type"
						body_info.m_content_type = result[field_val];
					else if(result[i++].matched)//"Transfer-Encoding"
						body_info.m_transfer_encoding = result[field_val];
					else if(result[i++].matched)//"Content-Encoding"
						body_info.m_content_encoding = result[field_val];
					else if(result[i++].matched)//"Host"
					{	body_info.m_host = result[field_val];
					string_tools::trim(body_info.m_host);
					}
					else if(result[i++].matched)//"Cookie"
						body_info.m_cookie = result[field_val];
					else if(result[i++].matched)//"User-Agent"
						body_info.m_user_agent = result[field_val];
					else if(result[i++].matched)//e.t.c (HAVE TO BE MATCHED!)
						body_info.m_etc_fields.emplace_back(result[11], result[field_val]);
					else
					{CHECK_AND_ASSERT_MES(false, false, "http_stream_filter::parse_cached_header() not matched last entry in:"<<m_cache_to_process);}

					it_current_bound = result[(int)result.size()-1]. first;
				}
				return  true;

			}
			inline
				bool analize_first_response_line()
			{

				//First line response, look like this:  "HTTP/1.1 200 OK"
				STATIC_REGEXP_EXPR_1(rexp_match_first_response_line, "^HTTP/(\\d+).(\\d+) ((\\d)\\d{2})( [^\n]*)?\r?\n", boost::regex::icase | boost::regex::normal);
				//															1      2      34           5        
				//size_t match_len = 0;
				boost::smatch result;
				if(boost::regex_search( m_header_cache, result, rexp_match_first_response_line, boost::match_default) && result[0].matched)
				{
					CHECK_AND_ASSERT_MES(result[1].matched&&result[2].matched, false, "http_stream_filter::handle_invoke_reply_line() assert failed...");
					m_response_info.m_http_ver_hi   = boost::lexical_cast<int>(result[1]);
					m_response_info.m_http_ver_lo   = boost::lexical_cast<int>(result[2]);
					m_response_info.m_response_code = boost::lexical_cast<int>(result[3]);
					
					m_header_cache.erase(to_nonsonst_iterator(m_header_cache, result[0].first), to_nonsonst_iterator(m_header_cache, result[0].second));
					return true;
				}else
				{
					LOG_ERROR("http_stream_filter::handle_invoke_reply_line(): Failed to match first response line:" << m_header_cache);
					return false;
				}

			}
			inline
				bool set_reply_content_encoder()
			{
				STATIC_REGEXP_EXPR_1(rexp_match_gzip, "^.*?((gzip)|(deflate))", boost::regex::icase | boost::regex::normal);
				boost::smatch result;						//   12      3
				if(boost::regex_search( m_response_info.m_header_info.m_content_encoding, result, rexp_match_gzip, boost::match_default) && result[0].matched)
				{
#ifdef HTTP_ENABLE_GZIP
					m_pcontent_encoding_handler.reset(new content_encoding_gzip(this, result[3].matched));
#else
          m_pcontent_encoding_handler.reset(new do_nothing_sub_handler(this));
          LOG_ERROR("GZIP encoding not supported in this build, please add zlib to your project and define HTTP_ENABLE_GZIP");
          return false;
#endif
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
					MERROR("Undefinded transfer type, consider http_body_transfer_connection_close method. header: " << m_header_cache);
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
	}
}
}
