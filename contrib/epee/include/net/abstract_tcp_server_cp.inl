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


#pragma comment(lib, "Ws2_32.lib")

namespace epee
{
namespace net_utils
{
template<class TProtocol> 
cp_server_impl<TProtocol>::cp_server_impl():
									m_port(0), m_stop(false),
									m_worker_thread_counter(0), m_listen_socket(INVALID_SOCKET)
{
}
//-------------------------------------------------------------
template<class TProtocol>
cp_server_impl<TProtocol>::~cp_server_impl()
{
	deinit_server();
}
//-------------------------------------------------------------
template<class TProtocol> 
bool cp_server_impl<TProtocol>::init_server(int port_no)
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

	m_listen_socket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if(INVALID_SOCKET == m_listen_socket)
	{
		err = ::WSAGetLastError();
		LOG_ERROR("Failed to create socket, err = " << err << " \"" << socket_errors::get_socket_error_text(err) <<"\"");
		return false;
	}


	int opt = 1;
	err = setsockopt (m_listen_socket, SOL_SOCKET,SO_REUSEADDR, reinterpret_cast<char*>(&opt), sizeof(int));
	if(SOCKET_ERROR == err )
	{
		err = ::WSAGetLastError();
		LOG_PRINT("Failed to setsockopt(SO_REUSEADDR), err = " << err << " \"" << socket_errors::get_socket_error_text(err) <<"\"", LOG_LEVEL_1);
		deinit_server();
		return false;
	}


	sockaddr_in adr = {0};
	adr.sin_family = AF_INET;
	adr.sin_addr.s_addr = htonl(INADDR_ANY);
	adr.sin_port = (u_short)htons(m_port);

	//binding
	err = bind(m_listen_socket, (const sockaddr*)&adr, sizeof(adr ));
	if(SOCKET_ERROR == err )
	{
		err = ::WSAGetLastError();
		LOG_PRINT("Failed to Bind, err = " << err << " \"" << socket_errors::get_socket_error_text(err) <<"\"", LOG_LEVEL_1);
		deinit_server();
		return false;
	}


	m_completion_port = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if(INVALID_HANDLE_VALUE == m_completion_port)
	{
		err = ::WSAGetLastError();
		LOG_PRINT("Failed to CreateIoCompletionPort, err = " << err << " \"" << socket_errors::get_socket_error_text(err) <<"\"", LOG_LEVEL_1);
		deinit_server();
		return false;
	}


	return true;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
static int CALLBACK CPConditionFunc(
						   IN LPWSABUF lpCallerId,
						   IN LPWSABUF lpCallerData,
						   IN OUT LPQOS lpSQOS,
						   IN OUT LPQOS lpGQOS,
						   IN LPWSABUF lpCalleeId,
						   OUT LPWSABUF lpCalleeData,
						   OUT GROUP FAR *g,
						   IN DWORD_PTR dwCallbackData
						   )
{

	/*cp_server_impl* pthis = (cp_server_impl*)dwCallbackData;
	if(!pthis)
		return CF_REJECT;*/
	/*if(pthis->get_active_connections_num()>=FD_SETSIZE-1)
	{
		LOG_PRINT("Maximum connections count overfull.", LOG_LEVEL_2);
		return CF_REJECT;
	}*/
	
	return  CF_ACCEPT;
}
//-------------------------------------------------------------
template<class TProtocol>
size_t cp_server_impl<TProtocol>::get_active_connections_num()
{
	return m_connections.size();
}
//-------------------------------------------------------------
template<class TProtocol>
unsigned CALLBACK cp_server_impl<TProtocol>::worker_thread(void* param)
{
	if(!param)
		return 0;

	cp_server_impl<TProtocol>* pthis = (cp_server_impl<TProtocol>*)param;
	pthis->worker_thread_member();
	return 1;
}
//-------------------------------------------------------------
template<class TProtocol> 
bool cp_server_impl<TProtocol>::worker_thread_member()
{
	LOG_PRINT("Worker thread STARTED", LOG_LEVEL_1);
	bool stop_handling = false;
	while(!stop_handling)
	{
		PROFILE_FUNC("[worker_thread]Worker Loop");
		DWORD bytes_transfered = 0;
		connection<TProtocol>* pconnection = 0;
		io_data_base* pio_data = 0;
		
		{
			PROFILE_FUNC("[worker_thread]GetQueuedCompletionStatus");
			BOOL res = ::GetQueuedCompletionStatus (m_completion_port, &bytes_transfered ,	(PULONG_PTR)&pconnection, (LPOVERLAPPED *)&pio_data, INFINITE);
			if (res == 0)
			{
				// check return code for error
				int err = GetLastError();
				LOG_PRINT("GetQueuedCompletionStatus failed with error " << err << " " << log_space::get_win32_err_descr(err), LOG_LEVEL_1);

				if(pio_data)
					::InterlockedExchange(&pio_data->m_is_in_use, 0);


				continue;
			}
		}

		if(pio_data)
			::InterlockedExchange(&pio_data->m_is_in_use, 0);



		if(!bytes_transfered && !pconnection && !pio_data)
		{
			//signal to stop
			break;
		}
		if(!pconnection || !pio_data)
		{
			LOG_PRINT("BIG FAIL: pconnection or pio_data is empty: pconnection=" << pconnection << " pio_data=" << pio_data, LOG_LEVEL_0);
			break;
		}



		if(::InterlockedCompareExchange(&pconnection->m_connection_shutwoned, 0, 0))
		{
			LOG_ERROR("InterlockedCompareExchange(&pconnection->m_connection_shutwoned, 0, 0)");
			//DebugBreak();
		}

		if(pio_data->m_op_type == op_type_stop)
		{
			if(!pconnection)
			{
				LOG_ERROR("op_type=op_type_stop, but pconnection is empty!!!");
				continue;
			}
			shutdown_connection(pconnection);
			continue;//
		}
		else if(pio_data->m_op_type == op_type_send)
		{
			continue;
			//do nothing, just queuing request
		}else if(pio_data->m_op_type == op_type_recv)
		{
			PROFILE_FUNC("[worker_thread]m_tprotocol_handler.handle_recv");
			if(bytes_transfered)
			{
				bool res = pconnection->m_tprotocol_handler.handle_recv(pio_data->Buffer, bytes_transfered);
				if(!res)
					pconnection->query_shutdown();
			}
			else
			{
				pconnection->query_shutdown();
				continue;
			}

		}

		//preparing new request, 

		{
			PROFILE_FUNC("[worker_thread]RECV Request small loop");
			int res = 0;
			while(true)
			{
				LOG_PRINT("Prepearing data for WSARecv....", LOG_LEVEL_3);
				ZeroMemory(&pio_data->m_overlapped, sizeof(OVERLAPPED));
				pio_data->DataBuf.len = pio_data->TotalBuffBytes;
				pio_data->DataBuf.buf = pio_data->Buffer;
				pio_data->m_op_type = op_type_recv;
				//calling WSARecv() and go to completion waiting
				DWORD bytes_recvd = 0;
				DWORD flags = 0;

				LOG_PRINT("Calling WSARecv....", LOG_LEVEL_3);
				::InterlockedExchange(&pio_data->m_is_in_use, 1);
				res = WSARecv(pconnection->m_sock, &(pio_data->DataBuf), 1, &bytes_recvd , &flags, &(pio_data->m_overlapped), NULL);
				if(res == SOCKET_ERROR )
				{
					int err = ::WSAGetLastError();
					if(WSA_IO_PENDING == err )
					{//go pending, ok
						LOG_PRINT("WSARecv return WSA_IO_PENDING", LOG_LEVEL_3);
						break;
					}
					LOG_ERROR("BIG FAIL: WSARecv error code not correct, res=" << res << " last_err=" << err);
					::InterlockedExchange(&pio_data->m_is_in_use, 0);
					pconnection->query_shutdown();
					break;
				}
				break;
				/*else if(0 == res)
				{
					if(!bytes_recvd)
					{
						::InterlockedExchange(&pio_data->m_is_in_use, 0);
						LOG_PRINT("WSARecv return 0, bytes_recvd=0, graceful close.", LOG_LEVEL_3);
						int err = ::WSAGetLastError();
						//LOG_ERROR("BIG FAIL: WSARecv error code not correct, res=" << res << " last_err=" << err);
						//pconnection->query_shutdown();
						break;
					}else
					{
						LOG_PRINT("WSARecv return immediatily 0, bytes_recvd=" << bytes_recvd, LOG_LEVEL_3);
						//pconnection->m_tprotocol_handler.handle_recv(pio_data->Buffer, bytes_recvd);
					}
				}*/
			}
		}
	}

	
	LOG_PRINT("Worker thread STOPED", LOG_LEVEL_1);
	::InterlockedDecrement(&m_worker_thread_counter);
	return true;
}
//-------------------------------------------------------------
template<class TProtocol> 
bool cp_server_impl<TProtocol>::shutdown_connection(connection<TProtocol>* pconn)
{
	PROFILE_FUNC("[shutdown_connection]");
	
	if(!pconn)
	{
		LOG_ERROR("Attempt to remove null pptr connection!");
		return false;
	}
	else 
	{
		LOG_PRINT("Shutting down connection ("<< pconn << ")", LOG_LEVEL_3);
	}
	m_connections_lock.lock();
	connections_container::iterator it = m_connections.find(pconn->m_sock);
	m_connections_lock.unlock();
	if(it == m_connections.end())
	{
		LOG_ERROR("Failed to find closing socket=" << pconn->m_sock);
		return false;
	}
	SOCKET sock = it->second->m_sock;
	{
		PROFILE_FUNC("[shutdown_connection] shutdown, close");
		::shutdown(it->second->m_sock,  SD_SEND );
	}
	size_t close_sock_wait_count = 0;
	{
		LOG_PRINT("Entered to 'in_use wait zone'", LOG_LEVEL_3);
		PROFILE_FUNC("[shutdown_connection] wait for in_use");
		while(::InterlockedCompareExchange(&it->second->m_precv_data->m_is_in_use, 1, 1))
		{

			Sleep(100);
			close_sock_wait_count++;
		}
		LOG_PRINT("First step to 'in_use wait zone'", LOG_LEVEL_3);


		while(::InterlockedCompareExchange(&it->second->m_psend_data->m_is_in_use, 1, 1))
		{
			Sleep(100);
			close_sock_wait_count++;
		}
		LOG_PRINT("Leaved 'in_use wait zone'", LOG_LEVEL_3);
	}

	::closesocket(it->second->m_sock);

	::InterlockedExchange(&it->second->m_connection_shutwoned, 1);
	m_connections_lock.lock();
	m_connections.erase(it);
	m_connections_lock.unlock();
	LOG_PRINT("Socked " << sock << " closed, wait_count=" << close_sock_wait_count, LOG_LEVEL_2);
	return true;
}
//-------------------------------------------------------------
template<class TProtocol> 
bool cp_server_impl<TProtocol>::run_server(int threads_count = 0)
{
	int err = listen(m_listen_socket, 100);
	if(SOCKET_ERROR == err )
	{
		err = ::WSAGetLastError();
		LOG_ERROR("Failed to listen, err = " << err << " \"" << socket_errors::get_socket_error_text(err) <<"\"");
		return false;
	}

	if(!threads_count)
	{
		SYSTEM_INFO si = {0};
		::GetSystemInfo(&si);
		threads_count = si.dwNumberOfProcessors + 2;
	}
	for(int i = 0; i != threads_count; i++)
	{
		boost::thread(boost::bind(&cp_server_impl::worker_thread_member, this));
		//HANDLE h_thread = threads_helper::create_thread(worker_thread, this);
		InterlockedIncrement(&m_worker_thread_counter);
		//::CloseHandle(h_thread);
	}

	LOG_PRINT("Numbers of worker threads started: " << threads_count, LOG_LEVEL_1);

	m_stop = false;
	while(!m_stop)
	{
		PROFILE_FUNC("[run_server] main_loop");
		TIMEVAL tv = {0};
		tv.tv_sec = 0;
		tv.tv_usec = 100;
		fd_set sock_set;
		sock_set.fd_count = 1;
		sock_set.fd_array[0] = m_listen_socket;
		int select_res = 0;
		{
			PROFILE_FUNC("[run_server] select");
			select_res = select(0, &sock_set, &sock_set, NULL, &tv);
		}
		
		if(SOCKET_ERROR == select_res)
		{
			err = ::WSAGetLastError();
			LOG_ERROR("Failed to select, err = " << err << " \"" << socket_errors::get_socket_error_text(err) <<"\"");
			return false;
		}
		if(!select_res)
		{
			on_net_idle();
			continue;
		}
		else
		{
			sockaddr_in adr_from = {0};
			int adr_len = sizeof(adr_from);
			SOCKET new_sock = INVALID_SOCKET;
			{
				PROFILE_FUNC("[run_server] WSAAccept");
				new_sock = ::WSAAccept(m_listen_socket, (sockaddr *)&adr_from, &adr_len, CPConditionFunc, (DWORD_PTR)this);
			}
			
			if(INVALID_SOCKET == new_sock)
			{
				if(m_stop)
					break;
				int err = ::WSAGetLastError();
				LOG_PRINT("Failed to WSAAccept, err = " << err << " \"" << socket_errors::get_socket_error_text(err) <<"\"", LOG_LEVEL_2);
				continue;
			}
			LOG_PRINT("Accepted connection (new socket=" << new_sock << ")", LOG_LEVEL_2);
			{
				PROFILE_FUNC("[run_server] Add new connection");
				add_new_connection(new_sock, adr_from.sin_addr.s_addr, adr_from.sin_port);
			}
			
		}

	}
	LOG_PRINT("Closing connections("<< m_connections.size() << ") and waiting...", LOG_LEVEL_2);
	m_connections_lock.lock();
	for(connections_container::iterator it = m_connections.begin(); it != m_connections.end(); it++)
	{
		::shutdown(it->second->m_sock, SD_BOTH);
		::closesocket(it->second->m_sock);
	}
	m_connections_lock.unlock();
	size_t wait_count = 0;
	while(m_connections.size() && wait_count < 100)
	{
		::Sleep(100);
		wait_count++;
	}
	LOG_PRINT("Connections closed OK (wait_count=" << wait_count << ")", LOG_LEVEL_2);


	LOG_PRINT("Stopping worker threads("<< m_worker_thread_counter << ").", LOG_LEVEL_2);
	for(int i = 0; i<m_worker_thread_counter; i++)
	{
		::PostQueuedCompletionStatus(m_completion_port, 0, 0, 0);
	}

	wait_count = 0;
	while(InterlockedCompareExchange(&m_worker_thread_counter, 0, 0) && wait_count < 100)
	{
		Sleep(100);
		wait_count++;
	}

	LOG_PRINT("Net Server STOPPED, wait_count = " << wait_count, LOG_LEVEL_1);
	return true;
}
//-------------------------------------------------------------
template<class TProtocol>
bool cp_server_impl<TProtocol>::add_new_connection(SOCKET new_sock, long ip_from, int port_from)
{
	PROFILE_FUNC("[add_new_connection]");
	
	LOG_PRINT("Add new connection zone: entering lock", LOG_LEVEL_3);
	m_connections_lock.lock();

	boost::shared_ptr<connection<TProtocol> > ptr;
	ptr.reset(new connection<TProtocol>(m_config));

	connection<TProtocol>& conn = *ptr.get();
	m_connections[new_sock] = ptr;
	LOG_PRINT("Add new connection zone: leaving lock", LOG_LEVEL_3);
	m_connections_lock.unlock();
	conn.init_buffers();
	conn.m_sock = new_sock;
	conn.context.m_remote_ip = ip_from;
	conn.context.m_remote_port = port_from;
	conn.m_completion_port = m_completion_port;
	{
		PROFILE_FUNC("[add_new_connection] CreateIoCompletionPort");
		::CreateIoCompletionPort((HANDLE)new_sock, m_completion_port, (ULONG_PTR)&conn, 0);
	}

	//if(NULL == ::CreateIoCompletionPort((HANDLE)new_sock, m_completion_port, (ULONG_PTR)&conn, 0))
	//{
    //	int err = ::GetLastError();
	//	LOG_PRINT("Failed to CreateIoCompletionPort(associate socket and completion port), err = " << err << " \"" << socket_errors::get_socket_error_text(err) <<"\"", LOG_LEVEL_2);
	//	return false;
	//}

	conn.m_tprotocol_handler.after_init_connection();
	{
		PROFILE_FUNC("[add_new_connection] starting loop");
		int res = 0;
		while(true)//res!=SOCKET_ERROR)
		{
			PROFILE_FUNC("[add_new_connection] in loop time");
			conn.m_precv_data->TotalBuffBytes = LEVIN_DEFAULT_DATA_BUFF_SIZE;
			ZeroMemory(&conn.m_precv_data->m_overlapped, sizeof(OVERLAPPED));
			conn.m_precv_data->DataBuf.len = conn.m_precv_data->TotalBuffBytes;
			conn.m_precv_data->DataBuf.buf = conn.m_precv_data->Buffer;
			conn.m_precv_data->m_op_type = op_type_recv;
			InterlockedExchange(&conn.m_precv_data->m_is_in_use, 1);
			DWORD bytes_recvd = 0;
			DWORD flags = 0;

			::InterlockedExchange(&conn.m_precv_data->m_is_in_use, 1);
			{
				PROFILE_FUNC("[add_new_connection] ::WSARecv");
				res = ::WSARecv(conn.m_sock, &(conn.m_precv_data->DataBuf), 1, &bytes_recvd , &flags, &(conn.m_precv_data->m_overlapped), NULL);
			}
			if(res == SOCKET_ERROR )
			{
				int err = ::WSAGetLastError();
				if(WSA_IO_PENDING == err )
				{
					break;
				}
				LOG_ERROR("BIG FAIL: WSARecv error code not correct, res=" << res << " last_err=" << err << " " << log_space::get_win32_err_descr(err));
				::InterlockedExchange(&conn.m_precv_data->m_is_in_use, 0);
				conn.query_shutdown();
				//shutdown_connection(&conn);
				break;
			}
			

			break;
			/*else if(0 == res)
			{
				if(!bytes_recvd)
				{
					PROFILE_FUNC("[add_new_connection] shutdown_connection");
					::InterlockedExchange(&conn.m_precv_data->m_is_in_use, 0);
					conn.query_shutdown();
					//shutdown_connection(&conn);
					break;
				}else
				{
					PROFILE_FUNC("[add_new_connection] handle_recv");
				}
			}*/
		}
	}



	return true;
}
//-------------------------------------------------------------
template<class TProtocol>
bool cp_server_impl<TProtocol>::deinit_server()
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

//-------------------------------------------------------------
template<class TProtocol>
bool cp_server_impl<TProtocol>::send_stop_signal()
{
	::InterlockedExchange(&m_stop, 1);
	return true;
}
//-------------------------------------------------------------
template<class TProtocol>
bool cp_server_impl<TProtocol>::is_stop_signal()
{
	return m_stop?true:false;	
}
//-------------------------------------------------------------
}
}