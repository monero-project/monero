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


#ifndef _LEVIN_CP_SERVER_H_
#define _LEVIN_CP_SERVER_H_

#include <winsock2.h>
#include <rpc.h>
#include <string>
#include <map>
#include <boost/shared_ptr.hpp>

#include "misc_log_ex.h"
//#include "threads_helper.h"
#include "syncobj.h"
#define ENABLE_PROFILING
#include "profile_tools.h"
#include "net_utils_base.h"
#include "pragma_comp_defs.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net"

#define LEVIN_DEFAULT_DATA_BUFF_SIZE       2000  

namespace epee
{
namespace net_utils
{

	template<class TProtocol>
	class cp_server_impl//: public abstract_handler
	{
	public:
		cp_server_impl(/*abstract_handler* phandler = NULL*/);
		virtual ~cp_server_impl();

		bool init_server(int port_no);
		bool deinit_server();
		bool run_server(int threads_count = 0);
		bool send_stop_signal();
		bool is_stop_signal();
		virtual bool on_net_idle(){return true;}
		size_t get_active_connections_num();
		typename TProtocol::config_type& get_config_object(){return m_config;}
	private:
		enum overlapped_operation_type
		{
			op_type_recv,
			op_type_send,
			op_type_stop
		};

		struct io_data_base
		{
			OVERLAPPED m_overlapped;
			WSABUF DataBuf;
			overlapped_operation_type m_op_type;
			DWORD TotalBuffBytes;
			volatile LONG m_is_in_use;
			char Buffer[1];
		};

PRAGMA_WARNING_PUSH
PRAGMA_WARNING_DISABLE_VS(4355)
		template<class TProtocol>
		struct connection: public net_utils::i_service_endpoint
		{
			connection(typename TProtocol::config_type& ref_config):m_sock(INVALID_SOCKET), m_tprotocol_handler(this, ref_config, context), m_psend_data(NULL), m_precv_data(NULL), m_asked_to_shutdown(0), m_connection_shutwoned(0)
			{
			}

			//connection():m_sock(INVALID_SOCKET), m_tprotocol_handler(this, m_dummy_config, context), m_psend_data(NULL), m_precv_data(NULL), m_asked_to_shutdown(0), m_connection_shutwoned(0)
			//{
			//}

			connection<TProtocol>& operator=(const connection<TProtocol>& obj)
			{
				return *this;
			}

			bool init_buffers()
			{
				m_psend_data = (io_data_base*)new char[sizeof(io_data_base) + LEVIN_DEFAULT_DATA_BUFF_SIZE-1];
				m_psend_data->TotalBuffBytes = LEVIN_DEFAULT_DATA_BUFF_SIZE;
				m_precv_data = (io_data_base*)new char[sizeof(io_data_base) + LEVIN_DEFAULT_DATA_BUFF_SIZE-1];
				m_precv_data->TotalBuffBytes = LEVIN_DEFAULT_DATA_BUFF_SIZE;
				return true;
			}
			
			bool query_shutdown()
			{
				if(!::InterlockedCompareExchange(&m_asked_to_shutdown, 1, 0))
				{
					m_psend_data->m_op_type = op_type_stop;
					::PostQueuedCompletionStatus(m_completion_port, 0, (ULONG_PTR)this, &m_psend_data->m_overlapped);
				}
				return true;
			}

			//bool set_config(typename TProtocol::config_type& config)
			//{
			//	this->~connection();
			//	new(this) connection<TProtocol>(config);
			//	return true;
			//}
			~connection()
			{
				if(m_psend_data)
					delete m_psend_data;

				if(m_precv_data)
					delete m_precv_data;
			}
			virtual bool handle_send(const void* ptr, size_t cb)
			{
				PROFILE_FUNC("[handle_send]");
				if(m_psend_data->TotalBuffBytes < cb)
					resize_send_buff((DWORD)cb);
				
				ZeroMemory(&m_psend_data->m_overlapped, sizeof(OVERLAPPED));
				m_psend_data->DataBuf.len = (u_long)cb;//m_psend_data->TotalBuffBytes;
				m_psend_data->DataBuf.buf = m_psend_data->Buffer;
				memcpy(m_psend_data->DataBuf.buf, ptr, cb);
				m_psend_data->m_op_type = op_type_send;
				InterlockedExchange(&m_psend_data->m_is_in_use, 1);
				DWORD bytes_sent = 0;
				DWORD flags = 0;
				int res = 0;
				{
					PROFILE_FUNC("[handle_send] ::WSASend");
					res = ::WSASend(m_sock, &(m_psend_data->DataBuf), 1, &bytes_sent, flags, &(m_psend_data->m_overlapped), NULL);
				}
				
				if(res == SOCKET_ERROR )
				{
					int err = ::WSAGetLastError();
					if(WSA_IO_PENDING == err )
						return true;
					}
					LOG_ERROR("BIG FAIL: WSASend error code not correct, res=" << res << " last_err=" << err);
					::InterlockedExchange(&m_psend_data->m_is_in_use, 0);
					query_shutdown();
					//closesocket(m_psend_data);
					return false;
				}else if(0 == res)
				{
					::InterlockedExchange(&m_psend_data->m_is_in_use, 0);
					if(!bytes_sent || bytes_sent != cb)
					{
						int err = ::WSAGetLastError();
						LOG_ERROR("BIG FAIL: WSASend immediatly complete? but bad results, res=" << res << " last_err=" << err);
						query_shutdown();
						return false;
					}else
					{
						return true;
					}
				}

				return true;
			}
			bool resize_send_buff(DWORD new_size)
			{
				if(m_psend_data->TotalBuffBytes >= new_size)
					return true;

				delete m_psend_data;
				m_psend_data = (io_data_base*)new char[sizeof(io_data_base) + new_size-1];
				m_psend_data->TotalBuffBytes = new_size;
				LOG_PRINT("Connection buffer resized up to " << new_size, LOG_LEVEL_3);
				return true;
			}


			SOCKET m_sock;
			net_utils::connection_context_base context;
			TProtocol m_tprotocol_handler;
			typename TProtocol::config_type m_dummy_config;
			io_data_base* m_precv_data;
			io_data_base* m_psend_data;
			HANDLE m_completion_port;
			volatile LONG m_asked_to_shutdown;			
			volatile LONG m_connection_shutwoned;			
		};
PRAGMA_WARNING_POP

		bool worker_thread_member();
		static unsigned CALLBACK worker_thread(void* param);

		bool add_new_connection(SOCKET new_sock, long ip_from, int port_from);
		bool shutdown_connection(connection<TProtocol>* pconn);


		typedef std::map<SOCKET, boost::shared_ptr<connection<TProtocol> > > connections_container;
		SOCKET m_listen_socket;
		HANDLE m_completion_port;
		connections_container m_connections;
		critical_section m_connections_lock;
		int	   m_port;
		volatile LONG m_stop;
		//abstract_handler* m_phandler;
		bool m_initialized;
		volatile LONG m_worker_thread_counter;
		typename TProtocol::config_type m_config;
	};
}
}
#include "abstract_tcp_server_cp.inl"


#endif //_LEVIN_SERVER_H_
