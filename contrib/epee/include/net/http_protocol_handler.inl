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


#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include "http_protocol_handler.h"
#include "reg_exp_definer.h"
#include "string_tools.h"
#include "file_io_utils.h"
#include "net_parse_helpers.h"

#define HTTP_MAX_URI_LEN		 9000 
#define HTTP_MAX_HEADER_LEN		 100000

namespace epee
{
namespace net_utils
{
	namespace http
	{

		struct multipart_entry
		{
			std::list<std::pair<std::string, std::string> > m_etc_header_fields;
			std::string m_content_disposition;
			std::string m_content_type;
			std::string m_body;
		};

		inline
			bool match_boundary(const std::string& content_type, std::string& boundary)
		{
			STATIC_REGEXP_EXPR_1(rexp_match_boundary, "boundary=(.*?)(($)|([;\\s,]))", boost::regex::icase | boost::regex::normal);
			//											        1
			boost::smatch result;	
			if(boost::regex_search(content_type, result, rexp_match_boundary, boost::match_default) && result[0].matched)
			{
				boundary = result[1];
				return true;
			}

			return false;
		}

		inline 
			bool parse_header(std::string::const_iterator it_begin, std::string::const_iterator it_end, multipart_entry& entry)
		{
			STATIC_REGEXP_EXPR_1(rexp_mach_field, 
				"\n?((Content-Disposition)|(Content-Type)"
				//  12                     3 
				"|([\\w-]+?)) ?: ?((.*?)(\r?\n))[^\t ]",	
				//4               56    7 
				boost::regex::icase | boost::regex::normal);

			boost::smatch		result;
			std::string::const_iterator it_current_bound = it_begin;
			std::string::const_iterator it_end_bound = it_end;

			//lookup all fields and fill well-known fields
			while( boost::regex_search( it_current_bound, it_end_bound, result, rexp_mach_field, boost::match_default) && result[0].matched) 
			{
				const size_t field_val = 6;
				const size_t field_etc_name = 4;

				int i = 2; //start position = 2
				if(result[i++].matched)//"Content-Disposition"
					entry.m_content_disposition = result[field_val];
				else if(result[i++].matched)//"Content-Type"
					entry.m_content_type = result[field_val];
				else if(result[i++].matched)//e.t.c (HAVE TO BE MATCHED!)
					entry.m_etc_header_fields.push_back(std::pair<std::string, std::string>(result[field_etc_name], result[field_val]));
				else
				{
					LOG_ERROR("simple_http_connection_handler::parse_header() not matched last entry in:"<<std::string(it_current_bound, it_end));
				}

				it_current_bound = result[(int)result.size()-1].first;
			}
			return  true;
		}

		inline
			bool handle_part_of_multipart(std::string::const_iterator it_begin, std::string::const_iterator it_end, multipart_entry& entry)
		{
			std::string end_str = "\r\n\r\n";
			std::string::const_iterator end_header_it = std::search(it_begin, it_end, end_str.begin(), end_str.end());
			if(end_header_it == it_end)
			{
				//header not matched 
				return false;
			}

			if(!parse_header(it_begin, end_header_it+4, entry))
			{
				LOG_ERROR("Failed to parse header:" << std::string(it_begin, end_header_it+2));
				return false;
			}
		
			entry.m_body.assign(end_header_it+4, it_end);

			return true;
		}

		inline
			bool parse_multipart_body(const std::string& content_type, const std::string& body, std::list<multipart_entry>& out_values)
		{
			//bool res = file_io_utils::load_file_to_string("C:\\public\\multupart_data", body);

			std::string boundary;
			if(!match_boundary(content_type, boundary))
			{
				LOG_PRINT("Failed to match boundary in content type: " << content_type, LOG_LEVEL_0);
				return false;
			}
			
			boundary+="\r\n";
			bool is_stop = false;
			bool first_step = true;
			
			std::string::const_iterator it_begin = body.begin();
			std::string::const_iterator it_end;
			while(!is_stop)
			{
				std::string::size_type pos = body.find(boundary, std::distance(body.begin(), it_begin));
			
				if(std::string::npos == pos)
				{
					is_stop = true;
					boundary.erase(boundary.size()-2, 2);
					boundary+= "--";
					pos = body.find(boundary, std::distance(body.begin(), it_begin));
					if(std::string::npos == pos)
					{
						LOG_PRINT("Error: Filed to match closing multipart tag", LOG_LEVEL_0);
						it_end = body.end();
					}else
					{
						it_end =  body.begin() + pos;
					}
				}else
					it_end =  body.begin() + pos;

			
				if(first_step && !is_stop)
				{
					first_step = false;
					it_begin = it_end + boundary.size();
					std::string temp = "\r\n--";
					boundary = temp + boundary;
					continue;
				}
				
				out_values.push_back(multipart_entry());
				if(!handle_part_of_multipart(it_begin, it_end, out_values.back()))
				{
					LOG_PRINT("Failed to handle_part_of_multipart", LOG_LEVEL_0);
					return false;
				}

				it_begin = it_end + boundary.size();
			}

			return true;
		}




		//--------------------------------------------------------------------------------------------
		template<class t_connection_context>
		simple_http_connection_handler<t_connection_context>::simple_http_connection_handler(i_service_endpoint* psnd_hndlr, config_type& config):
		m_state(http_state_retriving_comand_line),
		m_body_transfer_type(http_body_transfer_undefined),
        m_is_stop_handling(false),
		m_len_summary(0),
		m_len_remain(0),
		m_config(config), 
		m_want_close(false),
        m_psnd_hndlr(psnd_hndlr)
	{

	}
	//--------------------------------------------------------------------------------------------
    template<class t_connection_context>
	bool simple_http_connection_handler<t_connection_context>::set_ready_state()
	{
		m_is_stop_handling = false;
		m_state = http_state_retriving_comand_line;
		m_body_transfer_type = http_body_transfer_undefined;
		m_query_info.clear();
		m_len_summary = 0;
		return true;
	}
	//--------------------------------------------------------------------------------------------
  template<class t_connection_context>
	bool simple_http_connection_handler<t_connection_context>::handle_recv(const void* ptr, size_t cb)
	{
		std::string buf((const char*)ptr, cb);
		//LOG_PRINT_L0("HTTP_RECV: " << ptr << "\r\n" << buf);
		//file_io_utils::save_string_to_file(string_tools::get_current_module_folder() + "/" + boost::lexical_cast<std::string>(ptr), std::string((const char*)ptr, cb));

		bool res = handle_buff_in(buf);
		if(m_want_close/*m_state == http_state_connection_close || m_state == http_state_error*/)
			return false;
		return res;
	}
	//--------------------------------------------------------------------------------------------
  template<class t_connection_context>
	bool simple_http_connection_handler<t_connection_context>::handle_buff_in(std::string& buf)
	{

		if(m_cache.size())
			m_cache += buf;
		else
			m_cache.swap(buf);

		m_is_stop_handling = false;
		while(!m_is_stop_handling)
		{
			switch(m_state)
			{
			case http_state_retriving_comand_line:
				//The HTTP protocol does not place any a priori limit on the length of a URI.  (c)RFC2616
				//but we forebly restirct it len to HTTP_MAX_URI_LEN to make it more safely
				if(!m_cache.size())
					break;

				//check_and_handle_fake_response();
				if((m_cache[0] == '\r' || m_cache[0] == '\n'))
				{
          //some times it could be that before query line cold be few line breaks
          //so we have to be calm without panic with assers
					m_cache.erase(0, 1);
					break;
				}

				if(std::string::npos != m_cache.find('\n', 0))
					handle_invoke_query_line();
				else
				{
					m_is_stop_handling = true;
					if(m_cache.size() > HTTP_MAX_URI_LEN)
					{
						LOG_ERROR("simple_http_connection_handler::handle_buff_out: Too long URI line");
						m_state = http_state_error;
						return false;
					}
				}
				break;
			case http_state_retriving_header:
				{
					std::string::size_type pos = match_end_of_header(m_cache);
					if(std::string::npos == pos)
					{
						m_is_stop_handling = true;
						if(m_cache.size() > HTTP_MAX_HEADER_LEN)
						{
							LOG_ERROR("simple_http_connection_handler::handle_buff_in: Too long header area");
							m_state = http_state_error;
							return false;
						}	
						break;
					}
					analize_cached_request_header_and_invoke_state(pos);
					break;
				}
			case http_state_retriving_body:
				return handle_retriving_query_body();
			case http_state_connection_close:
				return false;
			default:
				LOG_ERROR("simple_http_connection_handler::handle_char_out: Wrong state: " << m_state);
				return false;
			case http_state_error:
				LOG_ERROR("simple_http_connection_handler::handle_char_out: Error state!!!");
				return false;
			}

			if(!m_cache.size())
				m_is_stop_handling = true;
		}

		return true;
	}
	//--------------------------------------------------------------------------------------------
	inline bool analize_http_method(const boost::smatch& result, http::http_method& method, int& http_ver_major, int& http_ver_minor)
	{
		CHECK_AND_ASSERT_MES(result[0].matched, false, "simple_http_connection_handler::analize_http_method() assert failed...");
		http_ver_major = boost::lexical_cast<int>(result[11]);
		http_ver_minor = boost::lexical_cast<int>(result[12]);
		if(result[4].matched)
			method = http::http_method_get;
		else if(result[5].matched)
			method = http::http_method_head;
		else if(result[6].matched)
			method = http::http_method_post;
		else if(result[7].matched)
			method = http::http_method_put;
		else 
			method = http::http_method_etc;

		return true;
	}

  //--------------------------------------------------------------------------------------------
  template<class t_connection_context>
	bool simple_http_connection_handler<t_connection_context>::handle_invoke_query_line()
	{ 
		LOG_FRAME("simple_http_connection_handler<t_connection_context>::handle_recognize_protocol_out(*)", LOG_LEVEL_3);

		STATIC_REGEXP_EXPR_1(rexp_match_command_line, "^(((OPTIONS)|(GET)|(HEAD)|(POST)|(PUT)|(DELETE)|(TRACE)) (\\S+) HTTP/(\\d+).(\\d+))\r?\n", boost::regex::icase | boost::regex::normal);
		//											    123         4     5      6      7     8        9        10          11     12    
		//size_t match_len = 0;
		boost::smatch result;	
		if(boost::regex_search(m_cache, result, rexp_match_command_line, boost::match_default) && result[0].matched)
		{
			analize_http_method(result, m_query_info.m_http_method, m_query_info.m_http_ver_hi, m_query_info.m_http_ver_hi);
			m_query_info.m_URI = result[10];
      parse_uri(m_query_info.m_URI, m_query_info.m_uri_content);
			m_query_info.m_http_method_str = result[2];
			m_query_info.m_full_request_str = result[0];

			m_cache.erase(m_cache.begin(),  to_nonsonst_iterator(m_cache, result[0].second));

			m_state = http_state_retriving_header;

			return true;
		}else
		{
			m_state = http_state_error;
			LOG_ERROR("simple_http_connection_handler<t_connection_context>::handle_invoke_query_line(): Failed to match first line: " << m_cache);
			return false;
		}

		return false;
	}
	//--------------------------------------------------------------------------------------------
  template<class t_connection_context>
	std::string::size_type simple_http_connection_handler<t_connection_context>::match_end_of_header(const std::string& buf)
	{

    //Here we returning head size, including terminating sequence (\r\n\r\n or \n\n)
		std::string::size_type res = buf.find("\r\n\r\n");
		if(std::string::npos != res)
			return res+4;
		res = buf.find("\n\n");
		if(std::string::npos != res)
			return res+2;
		return res;
	}
	//--------------------------------------------------------------------------------------------
  template<class t_connection_context>
	bool simple_http_connection_handler<t_connection_context>::analize_cached_request_header_and_invoke_state(size_t pos)
	{ 
		//LOG_PRINT_L4("HTTP HEAD:\r\n" << m_cache.substr(0, pos));

		LOG_FRAME("simple_http_connection_handler<t_connection_context>::analize_cached_request_header_and_invoke_state(*)", LOG_LEVEL_3);

		m_query_info.m_full_request_buf_size = pos;
    m_query_info.m_request_head.assign(m_cache.begin(), m_cache.begin()+pos); 

		if(!parse_cached_header(m_query_info.m_header_info, m_cache, pos))
		{
			LOG_ERROR("simple_http_connection_handler<t_connection_context>::analize_cached_request_header_and_invoke_state(): failed to anilize request header: " << m_cache);
			m_state = http_state_error;
		}

		m_cache.erase(0, pos);

		std::string req_command_str = m_query_info.m_full_request_str;
    //if we have POST or PUT command, it is very possible tha we will get body
    //but now, we suppose than we have body only in case of we have "ContentLength" 
		if(m_query_info.m_header_info.m_content_length.size())
		{
			m_state = http_state_retriving_body;
			m_body_transfer_type = http_body_transfer_measure;
			if(!get_len_from_content_lenght(m_query_info.m_header_info.m_content_length, m_len_summary))
			{
				LOG_ERROR("simple_http_connection_handler<t_connection_context>::analize_cached_request_header_and_invoke_state(): Failed to get_len_from_content_lenght();, m_query_info.m_content_length="<<m_query_info.m_header_info.m_content_length);
				m_state = http_state_error;
				return false;
			}
			if(0 == m_len_summary)
			{	//current query finished, next will be next query
				if(handle_request_and_send_response(m_query_info))
					set_ready_state();
				else
					m_state = http_state_error;
			}
			m_len_remain = m_len_summary;
		}else
		{//current query finished, next will be next query
			handle_request_and_send_response(m_query_info);
			set_ready_state();
		}

		return true;
	}
	//-----------------------------------------------------------------------------------
  template<class t_connection_context>
	bool simple_http_connection_handler<t_connection_context>::handle_retriving_query_body()
	{
		switch(m_body_transfer_type)
		{
		case http_body_transfer_measure:
			return handle_query_measure();
		case http_body_transfer_chunked:
		case http_body_transfer_connection_close:
		case http_body_transfer_multipart:
		case http_body_transfer_undefined:
		default:
			LOG_ERROR("simple_http_connection_handler<t_connection_context>::handle_retriving_query_body(): Unexpected m_body_query_type state:" << m_body_transfer_type);
			m_state = http_state_error;
			return false;
		}

		return true;
	}
	//-----------------------------------------------------------------------------------
  template<class t_connection_context>
	bool simple_http_connection_handler<t_connection_context>::handle_query_measure()
	{

		if(m_len_remain >= m_cache.size())
		{
			m_len_remain -= m_cache.size();
			m_query_info.m_body += m_cache;
			m_cache.clear();
		}else
		{
			m_query_info.m_body.append(m_cache.begin(), m_cache.begin() + m_len_remain);
			m_cache.erase(0, m_len_remain);
			m_len_remain = 0;
		}

		if(!m_len_remain)
		{
			if(handle_request_and_send_response(m_query_info))
				set_ready_state();
			else
				m_state = http_state_error;
		}
		return true;
	}
	//--------------------------------------------------------------------------------------------
  template<class t_connection_context>
	bool simple_http_connection_handler<t_connection_context>::parse_cached_header(http_header_info& body_info, const std::string& m_cache_to_process, size_t pos)
	{ 
		LOG_FRAME("http_stream_filter::parse_cached_header(*)", LOG_LEVEL_3);

		STATIC_REGEXP_EXPR_1(rexp_mach_field, 
			"\n?((Connection)|(Referer)|(Content-Length)|(Content-Type)|(Transfer-Encoding)|(Content-Encoding)|(Host)|(Cookie)"
			//  12            3         4                5              6                   7                  8      9    
			"|([\\w-]+?)) ?: ?((.*?)(\r?\n))[^\t ]",	
			//10             1112   13 
			boost::regex::icase | boost::regex::normal);

		boost::smatch		result;
		std::string::const_iterator it_current_bound = m_cache_to_process.begin();
		std::string::const_iterator it_end_bound = m_cache_to_process.begin()+pos;

		body_info.clear();

		//lookup all fields and fill well-known fields
		while( boost::regex_search( it_current_bound, it_end_bound, result, rexp_mach_field, boost::match_default) && result[0].matched) 
		{
			const size_t field_val = 12;
			const size_t field_etc_name = 10;

			int i = 2; //start position = 2
			if(result[i++].matched)//"Connection"
				body_info.m_connection = result[field_val];
			else if(result[i++].matched)//"Referer"
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
				body_info.m_host = result[field_val];
			else if(result[i++].matched)//"Cookie"
				body_info.m_cookie = result[field_val];
			else if(result[i++].matched)//e.t.c (HAVE TO BE MATCHED!)
				body_info.m_etc_fields.push_back(std::pair<std::string, std::string>(result[field_etc_name], result[field_val]));
			else
			{
				LOG_ERROR("simple_http_connection_handler<t_connection_context>::parse_cached_header() not matched last entry in:"<<m_cache_to_process);
			}

			it_current_bound = result[(int)result.size()-1]. first;
		}
		return  true;
	}
	//-----------------------------------------------------------------------------------
  template<class t_connection_context>
	bool simple_http_connection_handler<t_connection_context>::get_len_from_content_lenght(const std::string& str, size_t& OUT len)
	{
		STATIC_REGEXP_EXPR_1(rexp_mach_field, "\\d+", boost::regex::normal);
		std::string res;
		boost::smatch result;
		if(!(boost::regex_search( str, result, rexp_mach_field, boost::match_default) && result[0].matched))
			return false;

		len = boost::lexical_cast<size_t>(result[0]);
		return true;
	}
	//-----------------------------------------------------------------------------------
  template<class t_connection_context>
	bool simple_http_connection_handler<t_connection_context>::handle_request_and_send_response(const http::http_request_info& query_info)
	{
		http_response_info response;
		bool res = handle_request(query_info, response);
		//CHECK_AND_ASSERT_MES(res, res, "handle_request(query_info, response) returned false" );

		std::string response_data = get_response_header(response);
		
		//LOG_PRINT_L0("HTTP_SEND: << \r\n" << response_data + response.m_body);
    LOG_PRINT_L3("HTTP_RESPONSE_HEAD: << \r\n" << response_data);
		
		m_psnd_hndlr->do_send((void*)response_data.data(), response_data.size());
		if(response.m_body.size())
			m_psnd_hndlr->do_send((void*)response.m_body.data(), response.m_body.size());
		return res;
	}
	//-----------------------------------------------------------------------------------
  template<class t_connection_context>
	bool simple_http_connection_handler<t_connection_context>::handle_request(const http::http_request_info& query_info, http_response_info& response)
	{

		std::string uri_to_path = query_info.m_uri_content.m_path;
		if("/" == uri_to_path)
			uri_to_path = "/index.html";

		//slash_to_back_slash(uri_to_path);
		m_config.m_lock.lock();
		std::string destination_file_path = m_config.m_folder + uri_to_path;
		m_config.m_lock.unlock();
		if(!file_io_utils::load_file_to_string(destination_file_path.c_str(), response.m_body))
		{
			LOG_PRINT("URI \""<< query_info.m_full_request_str.substr(0, query_info.m_full_request_str.size()-2) << "\" [" << destination_file_path << "] Not Found (404 )" , LOG_LEVEL_1);
			response.m_body = get_not_found_response_body(query_info.m_URI);
			response.m_response_code = 404;
			response.m_response_comment = "Not found";
			response.m_mime_tipe = "text/html";
			return true;
		}

		LOG_PRINT(" -->> " << query_info.m_full_request_str << "\r\n<<--OK" , LOG_LEVEL_3);
		response.m_response_code = 200;
		response.m_response_comment = "OK";
		response.m_mime_tipe = get_file_mime_tipe(uri_to_path);


		return true;
	}
	//-----------------------------------------------------------------------------------
  template<class t_connection_context>
	std::string simple_http_connection_handler<t_connection_context>::get_response_header(const http_response_info& response)
	{
		std::string buf = "HTTP/1.1 ";
		buf += boost::lexical_cast<std::string>(response.m_response_code) + " " + response.m_response_comment + "\r\n" +
			"Server: Epee-based\r\n"
			"Content-Length: ";
		buf += boost::lexical_cast<std::string>(response.m_body.size()) + "\r\n";
		buf += "Content-Type: ";
		buf += response.m_mime_tipe + "\r\n";

		buf += "Last-Modified: ";
		time_t tm;
		time(&tm);
		buf += misc_utils::get_internet_time_str(tm) + "\r\n";
		buf += "Accept-Ranges: bytes\r\n";
		//Wed, 01 Dec 2010 03:27:41 GMT"

		string_tools::trim(m_query_info.m_header_info.m_connection);
		if(m_query_info.m_header_info.m_connection.size())
		{
			if(!string_tools::compare_no_case("close", m_query_info.m_header_info.m_connection))
			{
        //closing connection after sending
				buf += "Connection: close\r\n";
				m_state = http_state_connection_close;
				m_want_close = true;
			}
		}
		//add additional fields, if it is
		for(fields_list::const_iterator it = response.m_additional_fields.begin(); it!=response.m_additional_fields.end(); it++)
			buf += it->first + ":" + it->second + "\r\n";

		buf+="\r\n";

		return buf;
	}
	//-----------------------------------------------------------------------------------
	template<class t_connection_context>
  std::string simple_http_connection_handler<t_connection_context>::get_file_mime_tipe(const std::string& path)
	{
		std::string result;
		std::string ext = string_tools::get_extension(path);
		if(!string_tools::compare_no_case(ext, "gif"))
			result = "image/gif";
		else if(!string_tools::compare_no_case(ext, "jpg"))
			result = "image/jpeg";
		else if(!string_tools::compare_no_case(ext, "html"))
			result = "text/html";
		else if(!string_tools::compare_no_case(ext, "htm"))
			result = "text/html";
		else if(!string_tools::compare_no_case(ext, "js"))
			result = "application/x-javascript";
		else if(!string_tools::compare_no_case(ext, "css"))
			result = "text/css";
		else if(!string_tools::compare_no_case(ext, "xml"))
			result = "application/xml";
    else if(!string_tools::compare_no_case(ext, "svg"))
      result = "image/svg+xml";


		return result;
	}
	//-----------------------------------------------------------------------------------
	template<class t_connection_context>
  std::string simple_http_connection_handler<t_connection_context>::get_not_found_response_body(const std::string& URI)
	{
		std::string body = 
			"<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n"
			"<html><head>\r\n"
			"<title>404 Not Found</title>\r\n"
			"</head><body>\r\n"
			"<h1>Not Found</h1>\r\n"
			"<p>The requested URL \r\n";
		body += URI;
		body += "was not found on this server.</p>\r\n"
			"</body></html>\r\n";

		return body;
	}
	//--------------------------------------------------------------------------------------------
	template<class t_connection_context>
  bool simple_http_connection_handler<t_connection_context>::slash_to_back_slash(std::string& str)
	{
		for(std::string::iterator it = str.begin(); it!=str.end(); it++)
			if('/' == *it)
				*it = '\\';
		return true;
	}
	}
}
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------