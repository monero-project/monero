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

#include ""
#include "net_helper.h"
#include "levin_base.h"


namespace epee
{
namespace levin
{

  /************************************************************************
  *    levin_client_async - probably it is not really fast implementation, 
  *                each handler thread could make up to 30 ms latency. 
  *                But, handling events in reader thread will cause dead locks in
  *                case of recursive call (call invoke() to the same connection 
  *                on reader thread on remote invoke() handler)
  ***********************************************************************/


  class levin_client_async
	{
    levin_commands_handler* m_pcommands_handler;
		volatile uint32_t m_is_stop;
		volatile uint32_t m_threads_count;
		::critical_section m_send_lock;

    std::string m_local_invoke_buff;
		::critical_section m_local_invoke_buff_lock;
		volatile int m_invoke_res;

		volatile uint32_t m_invoke_data_ready;
		volatile uint32_t m_invoke_is_active;

		boost::mutex m_invoke_event;
		boost::condition_variable m_invoke_cond;
		size_t m_timeout;

		::critical_section m_recieved_packets_lock;
		struct packet_entry
		{
			bucket_head m_hd;
			std::string m_body;
			uint32_t m_connection_index;
		};
		std::list<packet_entry> m_recieved_packets;
    /*
       m_current_connection_index needed when some connection was broken and reconnected - in this 
                  case we could have some received packets in que, which shoud not be handled 
    */
		volatile uint32_t m_current_connection_index; 
		::critical_section m_invoke_lock;
		::critical_section m_reciev_packet_lock;
    ::critical_section m_connection_lock;
    net_utils::blocked_mode_client m_transport;
	public:
		levin_client_async():m_pcommands_handler(NULL), m_is_stop(0), m_threads_count(0), m_invoke_data_ready(0), m_invoke_is_active(0)
		{}
		levin_client_async(const levin_client_async& /*v*/):m_pcommands_handler(NULL), m_is_stop(0), m_threads_count(0), m_invoke_data_ready(0), m_invoke_is_active(0)
		{}
		~levin_client_async()
		{
      boost::interprocess::ipcdetail::atomic_write32(&m_is_stop, 1);
      disconnect();


			while(boost::interprocess::ipcdetail::atomic_read32(&m_threads_count))
				::Sleep(100);
		}

		void set_handler(levin_commands_handler* phandler)
		{
			m_pcommands_handler = phandler;
		}

		bool connect(uint32_t ip, uint32_t port, uint32_t timeout)
		{
			loop_call_guard();
			critical_region cr(m_connection_lock);

			m_timeout = timeout;
			bool res = false;
			CRITICAL_REGION_BEGIN(m_reciev_packet_lock);
			CRITICAL_REGION_BEGIN(m_send_lock);
			res = levin_client_impl::connect(ip, port, timeout);
			boost::interprocess::ipcdetail::atomic_inc32(&m_current_connection_index); 
			CRITICAL_REGION_END();
			CRITICAL_REGION_END();
			if(res && !boost::interprocess::ipcdetail::atomic_read32(&m_threads_count) )
			{
				//boost::interprocess::ipcdetail::atomic_write32(&m_is_stop, 0);//m_is_stop = false;
				boost::thread( boost::bind(&levin_duplex_client::reciever_thread, this) );
				boost::thread( boost::bind(&levin_duplex_client::handler_thread, this) );
				boost::thread( boost::bind(&levin_duplex_client::handler_thread, this) );
			}

			return res;
		}
		bool is_connected()
		{
			loop_call_guard();
			critical_region cr(m_cs);
			return levin_client_impl::is_connected();
		}

		inline
			bool check_connection()
		{
			loop_call_guard();
			critical_region cr(m_cs);

			if(!is_connected())
			{
				if( !reconnect() )
				{
					LOG_ERROR("Reconnect Failed. Failed to invoke() becouse not connected!");
					return false;
				}
			}
			return true;
		}

		//------------------------------------------------------------------------------
		inline 
			bool recv_n(SOCKET s, char* pbuff, size_t cb)
		{
			while(cb)
			{
				int res = ::recv(m_socket, pbuff, (int)cb, 0);

				if(SOCKET_ERROR == res)
				{
					if(!m_connected)
						return false;

					int err = ::WSAGetLastError();
					LOG_ERROR("Failed to recv(), err = " << err << " \"" << socket_errors::get_socket_error_text(err) <<"\"");
					disconnect();
					//reconnect();
					return false;
				}else if(res == 0)
				{
					disconnect();
					//reconnect();
					return false;
				}
				LOG_PRINT_L4("[" << m_socket <<"] RECV " << res);
				cb -= res;
				pbuff += res;
			}

			return true;
		}

		//------------------------------------------------------------------------------
		inline
			bool recv_n(SOCKET s, std::string& buff)
		{	
			size_t cb_remain = buff.size();
			char*  m_current_ptr = (char*)buff.data();
			return recv_n(s, m_current_ptr, cb_remain);
		}

		bool disconnect()
		{
			//boost::interprocess::ipcdetail::atomic_write32(&m_is_stop, 1);//m_is_stop = true;
			loop_call_guard();
			critical_region cr(m_cs);			
			levin_client_impl::disconnect();

			CRITICAL_REGION_BEGIN(m_local_invoke_buff_lock);
			m_local_invoke_buff.clear();
			m_invoke_res = LEVIN_ERROR_CONNECTION_DESTROYED;
			CRITICAL_REGION_END();
			boost::interprocess::ipcdetail::atomic_write32(&m_invoke_data_ready, 1); //m_invoke_data_ready = true;
			m_invoke_cond.notify_all();
			return true;
		}

		void loop_call_guard()
		{

		}

		void on_leave_invoke()
		{
			boost::interprocess::ipcdetail::atomic_write32(&m_invoke_is_active, 0);
		}

		int invoke(const GUID& target, int command, const std::string& in_buff, std::string& buff_out)
		{

			critical_region cr_invoke(m_invoke_lock);

			boost::interprocess::ipcdetail::atomic_write32(&m_invoke_is_active, 1);
			boost::interprocess::ipcdetail::atomic_write32(&m_invoke_data_ready, 0);
			misc_utils::destr_ptr hdlr = misc_utils::add_exit_scope_handler(boost::bind(&levin_duplex_client::on_leave_invoke, this));

			loop_call_guard();
			
			if(!check_connection())				
				return LEVIN_ERROR_CONNECTION_DESTROYED;


			bucket_head head = {0};
			head.m_signature = LEVIN_SIGNATURE;
			head.m_cb = in_buff.size();
			head.m_have_to_return_data = true;
			head.m_id = target;
#ifdef TRACE_LEVIN_PACKETS_BY_GUIDS
			::UuidCreate(&head.m_id);
#endif
			head.m_command = command;
			head.m_protocol_version = LEVIN_PROTOCOL_VER_1;
			head.m_flags = LEVIN_PACKET_REQUEST;
			LOG_PRINT("[" << m_socket <<"] Sending invoke data", LOG_LEVEL_4);

			CRITICAL_REGION_BEGIN(m_send_lock);
			LOG_PRINT_L4("[" << m_socket <<"] SEND " << sizeof(head));
			int res = ::send(m_socket, (const char*)&head, sizeof(head), 0);
			if(SOCKET_ERROR == res)
			{
				int err = ::WSAGetLastError();
				LOG_ERROR("Failed to send(), err = " << err << " \"" << socket_errors::get_socket_error_text(err) <<"\"");
				disconnect();
				return LEVIN_ERROR_CONNECTION_DESTROYED;
			}
			LOG_PRINT_L4("[" << m_socket <<"] SEND " << (int)in_buff.size());
			res = ::send(m_socket, in_buff.data(), (int)in_buff.size(), 0);
			if(SOCKET_ERROR == res)
			{
				int err = ::WSAGetLastError();
				LOG_ERROR("Failed to send(), err = " << err << " \"" << socket_errors::get_socket_error_text(err) <<"\"");
				disconnect();
				return LEVIN_ERROR_CONNECTION_DESTROYED;
			}
			CRITICAL_REGION_END();
			LOG_PRINT_L4("LEVIN_PACKET_SENT. [len=" << head.m_cb << ", flags=" << head.m_flags << ", is_cmd=" << head.m_have_to_return_data <<", cmd_id = " << head.m_command << ", pr_v=" << head.m_protocol_version << ", uid=" << string_tools::get_str_from_guid_a(head.m_id) << "]");

			//hard coded timeout in 10 minutes for maximum invoke period. if it happens, it could mean only some real troubles.
			boost::system_time timeout = boost::get_system_time()+ boost::posix_time::milliseconds(100);
			size_t timeout_count = 0;
			boost::unique_lock<boost::mutex> lock(m_invoke_event);

			while(!boost::interprocess::ipcdetail::atomic_read32(&m_invoke_data_ready))    
			{
				if(!m_invoke_cond.timed_wait(lock, timeout))
				{
					if(timeout_count < 10)
					{
						//workaround to avoid freezing at timed_wait called after notify_all. 
						timeout = boost::get_system_time()+ boost::posix_time::milliseconds(100);
						++timeout_count;
						continue;
					}else if(timeout_count == 10)
					{
						//workaround to avoid freezing at timed_wait called after notify_all. 
						timeout = boost::get_system_time()+ boost::posix_time::minutes(10);
						++timeout_count;
						continue;
					}else
					{
						LOG_PRINT("[" << m_socket <<"] Timeout on waiting invoke result. ", LOG_LEVEL_0);
						//disconnect();
						return LEVIN_ERROR_CONNECTION_TIMEDOUT;	
					}
				}
			}
			
			
			CRITICAL_REGION_BEGIN(m_local_invoke_buff_lock);
			buff_out.swap(m_local_invoke_buff);
			m_local_invoke_buff.clear();
			CRITICAL_REGION_END();
			return m_invoke_res;
		}	

		int notify(const GUID& target, int command, const std::string& in_buff)
		{
			if(!check_connection())
				return LEVIN_ERROR_CONNECTION_DESTROYED;

			bucket_head head = {0};
			head.m_signature = LEVIN_SIGNATURE;
			head.m_cb = in_buff.size();
			head.m_have_to_return_data = false;
			head.m_id = target;
#ifdef TRACE_LEVIN_PACKETS_BY_GUIDS
			::UuidCreate(&head.m_id);
#endif
			head.m_command = command;
			head.m_protocol_version = LEVIN_PROTOCOL_VER_1;
			head.m_flags = LEVIN_PACKET_REQUEST;
			CRITICAL_REGION_BEGIN(m_send_lock);
			LOG_PRINT_L4("[" << m_socket <<"] SEND " << sizeof(head));
			int res = ::send(m_socket, (const char*)&head, sizeof(head), 0);
			if(SOCKET_ERROR == res)
			{
				int err = ::WSAGetLastError();
				LOG_ERROR("Failed to send(), err = " << err << " \"" << socket_errors::get_socket_error_text(err) <<"\"");
				disconnect();
				return LEVIN_ERROR_CONNECTION_DESTROYED;
			}
			LOG_PRINT_L4("[" << m_socket <<"] SEND " << (int)in_buff.size());
			res = ::send(m_socket, in_buff.data(), (int)in_buff.size(), 0);
			if(SOCKET_ERROR == res)
			{
				int err = ::WSAGetLastError();
				LOG_ERROR("Failed to send(), err = " << err << " \"" << socket_errors::get_socket_error_text(err) <<"\"");
				disconnect();
				return LEVIN_ERROR_CONNECTION_DESTROYED;
			}
			CRITICAL_REGION_END();
			LOG_PRINT_L4("LEVIN_PACKET_SENT. [len=" << head.m_cb << ", flags=" << head.m_flags << ", is_cmd=" << head.m_have_to_return_data <<", cmd_id = " << head.m_command << ", pr_v=" << head.m_protocol_version << ", uid=" << string_tools::get_str_from_guid_a(head.m_id) << "]");

			return 1;
		}

		
	private:
		bool have_some_data(SOCKET sock, int interval = 1)
		{
			fd_set fds;
			FD_ZERO(&fds);
			FD_SET(sock, &fds);

			fd_set fdse;
			FD_ZERO(&fdse);
			FD_SET(sock, &fdse);


			timeval tv;
			tv.tv_sec = interval;
			tv.tv_usec = 0;

			int sel_res = select(0, &fds, 0, &fdse, &tv);
			if(0 == sel_res)
				return false;
			else if(sel_res == SOCKET_ERROR)
			{
				if(m_is_stop)
					return false;
				int err_code = ::WSAGetLastError();
				LOG_ERROR("Filed to call select, err code = " << err_code);
				disconnect();
			}else
			{
				if(fds.fd_array[0])
				{//some read operations was performed
					return true;
				}else if(fdse.fd_array[0])
				{//some error was at the socket
					return true;
				}
			}
			return false;
		}


		bool reciev_and_process_incoming_data()
		{
			bucket_head head = {0};
			uint32_t conn_index = 0;
			bool is_request = false;
			std::string local_buff;
			CRITICAL_REGION_BEGIN(m_reciev_packet_lock);//to protect from socket reconnect between head and body

			if(!recv_n(m_socket, (char*)&head, sizeof(head)))
			{
				if(m_is_stop)
					return false;
				LOG_ERROR("Failed to recv_n");
				return false;
			}

			conn_index = boost::interprocess::ipcdetail::atomic_read32(&m_current_connection_index);

			if(head.m_signature!=LEVIN_SIGNATURE) 
			{
				LOG_ERROR("Signature missmatch in response");
				return false;
			}
			
			is_request = (head.m_protocol_version == LEVIN_PROTOCOL_VER_1 && head.m_flags&LEVIN_PACKET_REQUEST);
			
			
			local_buff.resize((size_t)head.m_cb);
			if(!recv_n(m_socket, local_buff))
			{
				if(m_is_stop)
					return false;
				LOG_ERROR("Filed to reciev");
				return false;
			}
			CRITICAL_REGION_END();

			LOG_PRINT_L4("LEVIN_PACKET_RECIEVED. [len=" << head.m_cb << ", flags=" << head.m_flags << ", is_cmd=" << head.m_have_to_return_data <<", cmd_id = " << head.m_command << ", pr_v=" << head.m_protocol_version << ", uid=" << string_tools::get_str_from_guid_a(head.m_id) << "]");

			if(is_request)
			{
				CRITICAL_REGION_BEGIN(m_recieved_packets_lock);
				m_recieved_packets.resize(m_recieved_packets.size() + 1);
				m_recieved_packets.back().m_hd = head;
				m_recieved_packets.back().m_body.swap(local_buff);
				m_recieved_packets.back().m_connection_index  = conn_index;
				CRITICAL_REGION_END();
				/*

				*/
			}else
			{//this is some response
				
				CRITICAL_REGION_BEGIN(m_local_invoke_buff_lock);
				m_local_invoke_buff.swap(local_buff);
				m_invoke_res = head.m_return_code;
				CRITICAL_REGION_END();
				boost::interprocess::ipcdetail::atomic_write32(&m_invoke_data_ready, 1); //m_invoke_data_ready = true;
				m_invoke_cond.notify_all();
				
			}
			return true;
		}

		bool reciever_thread()
		{
			LOG_PRINT_L3("[" << m_socket <<"] Socket reciever thread started.[m_threads_count=" << m_threads_count << "]");
			log_space::log_singletone::set_thread_log_prefix("RECIEVER_WORKER");
			boost::interprocess::ipcdetail::atomic_inc32(&m_threads_count);

			while(!m_is_stop)
			{
				if(!m_connected)
				{
					Sleep(100);
					continue;
				}

				if(have_some_data(m_socket, 1))
				{
					if(!reciev_and_process_incoming_data())
					{
						if(m_is_stop)
						{
							break;//boost::interprocess::ipcdetail::atomic_dec32(&m_threads_count);
							//return true;
						}
						LOG_ERROR("Failed to reciev_and_process_incoming_data. shutting down");
						//boost::interprocess::ipcdetail::atomic_dec32(&m_threads_count);
						//disconnect_no_wait();
						//break;
					}
				}
			}
			
			boost::interprocess::ipcdetail::atomic_dec32(&m_threads_count);
			LOG_PRINT_L3("[" << m_socket <<"] Socket reciever thread stopped.[m_threads_count=" << m_threads_count << "]");
			return true;
		}

		bool process_recieved_packet(bucket_head& head, const std::string& local_buff, uint32_t conn_index)
		{

			net_utils::connection_context_base conn_context;
			conn_context.m_remote_ip = m_ip;
			conn_context.m_remote_port = m_port;
			if(head.m_have_to_return_data)
			{
				std::string return_buff;
				if(m_pcommands_handler)
					head.m_return_code = m_pcommands_handler->invoke(head.m_id, head.m_command, local_buff, return_buff, conn_context);
				else 
					head.m_return_code = LEVIN_ERROR_CONNECTION_HANDLER_NOT_DEFINED;



				head.m_cb = return_buff.size();
				head.m_have_to_return_data = false;
				head.m_protocol_version = LEVIN_PROTOCOL_VER_1;
				head.m_flags = LEVIN_PACKET_RESPONSE;

				std::string send_buff((const char*)&head, sizeof(head));
				send_buff += return_buff;
				CRITICAL_REGION_BEGIN(m_send_lock);
				if(conn_index != boost::interprocess::ipcdetail::atomic_read32(&m_current_connection_index))
				{//there was reconnect, send response back is not allowed
					return true;
				}
				int res = ::send(m_socket, (const char*)send_buff.data(), send_buff.size(), 0);
				if(res == SOCKET_ERROR)
				{
					int err_code = ::WSAGetLastError();
					LOG_ERROR("Failed to send, err = " << err_code);
					return false;
				}
				CRITICAL_REGION_END();
				LOG_PRINT_L4("LEVIN_PACKET_SENT. [len=" << head.m_cb << ", flags=" << head.m_flags << ", is_cmd=" << head.m_have_to_return_data <<", cmd_id = " << head.m_command << ", pr_v=" << head.m_protocol_version << ", uid=" << string_tools::get_str_from_guid_a(head.m_id) << "]");

			}
			else
			{
				if(m_pcommands_handler)
					m_pcommands_handler->notify(head.m_id, head.m_command, local_buff, conn_context);
			}

			return true;
		}

		bool handler_thread()
		{
			LOG_PRINT_L3("[" << m_socket <<"] Socket handler thread started.[m_threads_count=" << m_threads_count << "]");
			log_space::log_singletone::set_thread_log_prefix("HANDLER_WORKER");
			boost::interprocess::ipcdetail::atomic_inc32(&m_threads_count);

			while(!m_is_stop)
			{
				bool have_some_work = false;
				std::string local_buff;
				bucket_head bh = {0};
				uint32_t conn_index = 0;

				CRITICAL_REGION_BEGIN(m_recieved_packets_lock);
				if(m_recieved_packets.size())
				{
					bh = m_recieved_packets.begin()->m_hd;
					conn_index = m_recieved_packets.begin()->m_connection_index;
					local_buff.swap(m_recieved_packets.begin()->m_body);
					have_some_work = true;
					m_recieved_packets.pop_front();
				}
				CRITICAL_REGION_END();

				if(have_some_work)
				{
					process_recieved_packet(bh, local_buff, conn_index);
				}else 
				{
					//Idle when no work
					Sleep(30);
				}
			}

			boost::interprocess::ipcdetail::atomic_dec32(&m_threads_count);
			LOG_PRINT_L3("[" << m_socket <<"] Socket handler thread stopped.[m_threads_count=" << m_threads_count << "]");
			return true;
		}
	};

}
}
