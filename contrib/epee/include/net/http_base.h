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
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

#include "string_tools.h"
namespace epee
{
namespace net_utils
{
	namespace http
	{

		enum http_method{
			http_method_get,
			http_method_post,
			http_method_put,
			http_method_head,
			http_method_etc,
			http_method_unknown
		};

		enum http_content_type
		{
			http_content_type_text_html,
			http_content_type_image_gif, 
			http_content_type_other,
			http_content_type_not_set
		};

		typedef std::list<std::pair<std::string, std::string> > fields_list;

		inline
		std::string get_value_from_fields_list(const std::string& param_name, const net_utils::http::fields_list& fields)
		{
			fields_list::const_iterator it = fields.begin();
			for(; it != fields.end(); it++)
				if(!string_tools::compare_no_case(param_name, it->first))
					break;

			if(it==fields.end())
				return std::string();

			return it->second;
		}


		inline
			std::string get_value_from_uri_line(const std::string& param_name, const std::string& uri)
		{
			std::string buff = "([\\?|&])";
			buff += param_name + "=([^&]*)";
			boost::regex match_param(buff.c_str(), boost::regex::icase | boost::regex::normal);
			boost::smatch	result;
			if(boost::regex_search(uri, result, match_param, boost::match_default) && result[0].matched) 
			{
				return result[2];
			}
			return std::string();
		}



		struct http_header_info
		{
			std::string m_connection;       //"Connection:"
			std::string m_referer;          //"Referer:"
			std::string m_content_length;   //"Content-Length:"
			std::string m_content_type;     //"Content-Type:"
			std::string m_transfer_encoding;//"Transfer-Encoding:"
			std::string m_content_encoding; //"Content-Encoding:"
			std::string m_host;             //"Host:"
			std::string m_cookie;			//"Cookie:"
			std::string m_user_agent;	//"User-Agent:"
			fields_list m_etc_fields;

			void clear()
			{
				m_connection.clear();
				m_referer.clear();
				m_content_length.clear();
				m_content_type.clear();
				m_transfer_encoding.clear();
				m_content_encoding.clear();
				m_host.clear();
				m_cookie.clear();
				m_user_agent.clear();
				m_etc_fields.clear();
			}
		};

    struct uri_content
    {
      std::string m_path;
      std::string m_query;
      std::string m_fragment;
      std::list<std::pair<std::string, std::string> > m_query_params;
    };

    struct url_content
    {
      std::string schema;
      std::string host;
      std::string uri;
      uint64_t port;
      uri_content m_uri_content;
    };


		struct http_request_info 
		{
			http_request_info():m_http_method(http_method_unknown), 
				m_http_ver_hi(0), 
				m_http_ver_lo(0), 
				m_have_to_block(false)
			{}

			http_method			  m_http_method;
			std::string       m_URI;
			std::string       m_http_method_str;
			std::string       m_full_request_str;
			std::string       m_replace_html;
            std::string       m_request_head;
			int               m_http_ver_hi;
			int               m_http_ver_lo;
            bool		      m_have_to_block;
			http_header_info	m_header_info;
      uri_content       m_uri_content;
			size_t				    m_full_request_buf_size;
			std::string			  m_body;

			void clear()
			{
				this->~http_request_info();
				new(this) http_request_info();
			}
		};


		struct http_response_info 
		{
			int					m_response_code;
			std::string			m_response_comment;
			fields_list	        m_additional_fields;
			std::string			m_body;
			std::string			m_mime_tipe;
			http_header_info    m_header_info;
			int                 m_http_ver_hi;// OUT paramter only
			int                 m_http_ver_lo;// OUT paramter only

			void clear()
			{
				this->~http_response_info();
				new(this) http_response_info();
			}
		};
	}
}
}
