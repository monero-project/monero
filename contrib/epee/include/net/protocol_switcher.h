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



#ifndef _PROTOCOL_SWITCHER_H_
#define _PROTOCOL_SWITCHER_H_

#include "levin_base.h"
#include "http_server.h"
#include "levin_protocol_handler.h"
//#include "abstract_tcp_server.h"

namespace epee
{
namespace net_utils
{
	struct protocl_switcher_config
	{
		http::http_custom_handler::config_type m_http_config;
		levin::protocol_handler::config_type m_levin_config;
	};

	
	struct i_protocol_handler
	{
		virtual bool handle_recv(const void* ptr, size_t cb)=0;
	};

	template<class t>
	class t_protocol_handler: public i_protocol_handler
	{
	public: 
		typedef t t_type;
		t_protocol_handler(i_service_endpoint* psnd_hndlr, typename t_type::config_type& config, const connection_context& conn_context):m_hadler(psnd_hndlr, config, conn_context)
		{}
	private:
		bool handle_recv(const void* ptr, size_t cb)
		{
			return m_hadler.handle_recv(ptr, cb);
		}
		t_type m_hadler;
	};


	class protocol_switcher
	{
	public:
		typedef protocl_switcher_config config_type;

		protocol_switcher(net_utils::i_service_endpoint* psnd_hndlr, config_type& config, const net_utils::connection_context_base& conn_context);
		virtual ~protocol_switcher(){}

		virtual bool handle_recv(const void* ptr, size_t cb);

		bool after_init_connection(){return true;}
	private:
		t_protocol_handler<http::http_custom_handler> m_http_handler;
		t_protocol_handler<levin::protocol_handler> m_levin_handler;
		i_protocol_handler* pcurrent_handler;

		std::string m_cached_buff;
	};

	protocol_switcher::protocol_switcher(net_utils::i_service_endpoint* psnd_hndlr, config_type& config, const net_utils::connection_context_base& conn_context):m_http_handler(psnd_hndlr, config.m_http_config, conn_context), m_levin_handler(psnd_hndlr, config.m_levin_config, conn_context), pcurrent_handler(NULL)
	{}

	bool protocol_switcher::handle_recv(const void* ptr, size_t cb)
	{
		if(pcurrent_handler)
			return pcurrent_handler->handle_recv(ptr, cb);
		else
		{
			m_cached_buff.append((const char*)ptr, cb);
			if(m_cached_buff.size() < sizeof(uint64_t))
				return true;

			if(*((uint64_t*)&m_cached_buff[0]) == LEVIN_SIGNATURE)
			{
				pcurrent_handler = &m_levin_handler;
				return pcurrent_handler->handle_recv(m_cached_buff.data(), m_cached_buff.size());
			}
			if(m_cached_buff.substr(0, 4) == "GET " || m_cached_buff.substr(0, 4) == "POST")
			{
				pcurrent_handler = &m_http_handler;
				return pcurrent_handler->handle_recv(m_cached_buff.data(), m_cached_buff.size());
			}else
			{
				LOG_ERROR("Wrong protocol accepted on port...");
				return false;
			}
		}
			
		return true;
	}
}
}
#endif //_PROTOCOL_SWITCHER_H_