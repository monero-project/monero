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



#ifndef _ABSTRACT_TCP_SERVER_H_
#define _ABSTRACT_TCP_SERVER_H_

#include <process.h>
#include <list>
#include <winsock2.h>
#include "winobj.h"
//#include "threads_helper.h"
#include "net_utils_base.h"

#pragma comment(lib, "Ws2_32.lib")

namespace epee 
{
namespace net_utils
{
	/************************************************************************/
	/*                                                                      */
	/************************************************************************/
	class soket_sender: public i_service_endpoint
	{
	public:
		soket_sender(SOCKET sock):m_sock(sock){}
	private:
		virtual bool handle_send(const void* ptr, size_t cb)
		{
			if(cb  != send(m_sock, (char*)ptr, (int)cb, 0))
			{
				int sock_err = WSAGetLastError();
				LOG_ERROR("soket_sender: Failed to send " << cb << " bytes, Error=" << sock_err);
				return false;
			}
			return true;

		}

		SOCKET m_sock;
	};

	

	/************************************************************************/
	/*                                                                      */
	/************************************************************************/
	template<class THandler>
	class abstract_tcp_server
	{
	public:
		abstract_tcp_server();

		bool init_server(int port_no);
		bool deinit_server();
		bool run_server();
		bool send_stop_signal();

		typename THandler::config_type& get_config_object(){return m_config;}

	private:
		bool invoke_connection(SOCKET hnew_sock, long ip_from, int post_from);
		static unsigned __stdcall ConnectionHandlerProc(void* lpParameter);

		class thread_context;
		typedef std::list<thread_context> connections_container;
		typedef typename connections_container::iterator connections_iterator;

		struct thread_context
		{
			HANDLE m_htread;
			SOCKET m_socket;
			abstract_tcp_server* powner;
			connection_context m_context;
			typename connections_iterator m_self_it;
		};

		SOCKET m_listen_socket;
		int m_port;
		bool   m_initialized;
		volatile LONG  m_stop_server;
		volatile LONG  m_threads_count;
		typename THandler::config_type m_config;
		connections_container m_connections;
		critical_section m_connections_lock;
	};
	
	template<class THandler>
	unsigned __stdcall abstract_tcp_server<THandler>::ConnectionHandlerProc(void* lpParameter)
	{

		thread_context* pthread_context = (thread_context*)lpParameter;
		if(!pthread_context)
			 return 0;
		abstract_tcp_server<THandler>* pthis = pthread_context->powner;

		::InterlockedIncrement(&pthis->m_threads_count);
		
		::CoInitialize(NULL);


		LOG_PRINT("Handler thread STARTED with socket=" << pthread_context->m_socket,  LOG_LEVEL_2);
		int res = 0;

		soket_sender sndr(pthread_context->m_socket);
		THandler srv(&sndr, pthread_context->powner->m_config, pthread_context->m_context);


		srv.after_init_connection();

		char buff[1000] = {0};
		std::string ansver;
		while ( (res = recv(pthread_context->m_socket, (char*)buff, 1000, 0)) > 0)
		{
			LOG_PRINT("Data in, " << res << " bytes", LOG_LEVEL_3);
			if(!srv.handle_recv(buff, res))
				break;
		}
		shutdown(pthread_context->m_socket, SD_BOTH);
		closesocket(pthread_context->m_socket);

		abstract_tcp_server* powner = pthread_context->powner;
		LOG_PRINT("Handler thread with socket=" << pthread_context->m_socket << " STOPPED",  LOG_LEVEL_2);
		powner->m_connections_lock.lock();
		::CloseHandle(pthread_context->m_htread);
		pthread_context->powner->m_connections.erase(pthread_context->m_self_it);
		powner->m_connections_lock.unlock();
		CoUninitialize();
		::InterlockedDecrement(&pthis->m_threads_count);
		return 1;
	}
	//----------------------------------------------------------------------------------------
	template<class THandler>
	abstract_tcp_server<THandler>::abstract_tcp_server():m_listen_socket(INVALID_SOCKET), 
														 m_initialized(false), 
														 m_stop_server(0), m_port(0), m_threads_count(0)
	{

	}

	//----------------------------------------------------------------------------------------
	template<class THandler>
	bool abstract_tcp_server<THandler>::init_server(int port_no)
	{
		m_port = port_no;
		WSADATA wsad = {0};
		int err = ::WSAStartup(MAKEWORD(2,2), &wsad);
		if ( err != 0  || LOBYTE( wsad.wVersion ) != 2 || HIBYTE( wsad.wVersion ) != 2 )
		{
			LOG_ERROR("Could not find a usable WinSock DLL, err = " << err << " \"" << socket_errors::get_socket_error_text(err) <<"\"");
			return false;
		}

		m_initialized = true;

		m_listen_socket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
		if(INVALID_SOCKET == m_listen_socket)
		{
			err = ::WSAGetLastError();
			LOG_ERROR("Failed to create socket, err = " << err << " \"" << socket_errors::get_socket_error_text(err) <<"\"");
			return false;
		}

		int opt = 1;
		setsockopt (m_listen_socket, SOL_SOCKET,SO_REUSEADDR, reinterpret_cast<char*>(&opt), sizeof(int));

		sockaddr_in adr = {0};
		adr.sin_family = AF_INET;
		adr.sin_addr.s_addr = htonl(INADDR_ANY);
		adr.sin_port = (u_short)htons(port_no);

		err = bind(m_listen_socket, (const sockaddr*)&adr, sizeof(adr ));
		if(SOCKET_ERROR == err )
		{
			err = ::WSAGetLastError();
			LOG_PRINT("Failed to Bind, err = " << err << " \"" << socket_errors::get_socket_error_text(err) <<"\"", LOG_LEVEL_2);
			deinit_server();
			return false;
		}

		::InterlockedExchange(&m_stop_server, 0);

		return true;
	}
	//----------------------------------------------------------------------------------------
	template<class THandler>
	bool abstract_tcp_server<THandler>::deinit_server()
	{

		if(!m_initialized)
			return true;

		if(INVALID_SOCKET != m_listen_socket)
		{
			shutdown(m_listen_socket, SD_BOTH);
			int res = closesocket(m_listen_socket);
			if(SOCKET_ERROR == res)
			{
				int err = ::WSAGetLastError();
				LOG_ERROR("Failed to closesocket(), err = " << err << " \"" << socket_errors::get_socket_error_text(err) <<"\"");
			}
			m_listen_socket = INVALID_SOCKET;
		}

		int res = ::WSACleanup();
		if(SOCKET_ERROR == res)
		{
			int err = ::WSAGetLastError();
			LOG_ERROR("Failed to WSACleanup(), err = " << err << " \"" << socket_errors::get_socket_error_text(err) <<"\"");
		}
		m_initialized = false;

		return true;
	}
	//----------------------------------------------------------------------------------------
	template<class THandler>
	bool abstract_tcp_server<THandler>::send_stop_signal()
	{
		InterlockedExchange(&m_stop_server, 1);
		return true;
	}
	//----------------------------------------------------------------------------------------
	template<class THandler>
	bool abstract_tcp_server<THandler>::run_server()
	{
		int err = listen(m_listen_socket, 10000);
		if(SOCKET_ERROR == err )
		{
			err = ::WSAGetLastError();
			LOG_ERROR("Failed to listen, err = " << err << " \"" << socket_errors::get_socket_error_text(err) <<"\"");
			return false;
		}
		
		LOG_PRINT("Listening port "<< m_port << "...." , LOG_LEVEL_2);

		while(!m_stop_server)
		{
			sockaddr_in adr_from = {0};
			int adr_len = sizeof(adr_from);
			fd_set read_fs = {0};
			read_fs.fd_count = 1;
			read_fs.fd_array[0] = m_listen_socket;
			TIMEVAL tv = {0};
			tv.tv_usec = 100;
			int select_res = select(0, &read_fs, NULL, NULL, &tv);
			if(!select_res)
				continue;
			SOCKET new_sock = WSAAccept(m_listen_socket, (sockaddr *)&adr_from, &adr_len, NULL, NULL);
			LOG_PRINT("Accepted connection on socket=" << new_sock, LOG_LEVEL_2);
			invoke_connection(new_sock, adr_from.sin_addr.s_addr, adr_from.sin_port);
		}

		deinit_server();

#define ABSTR_TCP_SRV_WAIT_COUNT_MAX 5000
#define ABSTR_TCP_SRV_WAIT_COUNT_INTERVAL 1000
		
		int wait_count = 0;

		while(m_threads_count && wait_count*1000 < ABSTR_TCP_SRV_WAIT_COUNT_MAX)
		{
			::Sleep(ABSTR_TCP_SRV_WAIT_COUNT_INTERVAL);
			wait_count++;
		}
		LOG_PRINT("abstract_tcp_server exit with wait count=" << wait_count*ABSTR_TCP_SRV_WAIT_COUNT_INTERVAL << "(max=" << ABSTR_TCP_SRV_WAIT_COUNT_MAX <<")", LOG_LEVEL_0);

		return true;
	}
	//----------------------------------------------------------------------------------------
	template<class THandler>
	bool abstract_tcp_server<THandler>::invoke_connection(SOCKET hnew_sock, long ip_from, int post_from)
	{
		m_connections_lock.lock();
		m_connections.push_back(thread_context());
		m_connections_lock.unlock();
		m_connections.back().m_socket = hnew_sock;
		m_connections.back().powner = this;
		m_connections.back().m_self_it = --m_connections.end();
		m_connections.back().m_context.m_remote_ip = ip_from;
		m_connections.back().m_context.m_remote_port = post_from;
		m_connections.back().m_htread = threads_helper::create_thread(ConnectionHandlerProc, &m_connections.back());

		return true;
	}
	//----------------------------------------------------------------------------------------

	//----------------------------------------------------------------------------------------
	//----------------------------------------------------------------------------------------
}
}
#endif //_ABSTRACT_TCP_SERVER_H_
