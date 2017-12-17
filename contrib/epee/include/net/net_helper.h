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

//#include <Winsock2.h>
//#include <Ws2tcpip.h>
#include <string>
#include <boost/version.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/interprocess/detail/atomic.hpp>
#include "net/net_utils_base.h"
#include "misc_language.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net"

#ifndef MAKE_IP
#define MAKE_IP( a1, a2, a3, a4 )	(a1|(a2<<8)|(a3<<16)|(a4<<24))
#endif


namespace epee
{
namespace net_utils
{

  class blocked_mode_client
	{
		
		
				struct handler_obj
				{
					handler_obj(boost::system::error_code& error,	size_t& bytes_transferred):ref_error(error), ref_bytes_transferred(bytes_transferred)
					{}
					handler_obj(const handler_obj& other_obj):ref_error(other_obj.ref_error), ref_bytes_transferred(other_obj.ref_bytes_transferred)
					{}

					boost::system::error_code& ref_error;
					size_t& ref_bytes_transferred;

					void operator()(const boost::system::error_code& error, // Result of operation.
						std::size_t bytes_transferred           // Number of bytes read.
						)
					{
						ref_error = error;
						ref_bytes_transferred = bytes_transferred;
					}
				};
		
	public:
		inline
			blocked_mode_client():m_initialized(false), 
                            m_connected(false), 
                            m_deadline(m_io_service), 
                            m_shutdowned(0),
                            m_ssl(false),
                            m_ctx(boost::asio::ssl::context::sslv23),
                            m_ssl_socket(m_io_service,m_ctx)
		{
			
			
			m_initialized = true;


			// No deadline is required until the first socket operation is started. We
			// set the deadline to positive infinity so that the actor takes no action
			// until a specific deadline is set.
			m_deadline.expires_at(std::chrono::steady_clock::time_point::max());

			// Start the persistent actor that checks for deadline expiry.
			check_deadline();

		}
		inline
			~blocked_mode_client()
		{
			//profile_tools::local_coast lc("~blocked_mode_client()", 3);
			shutdown();
		}

    inline
      bool connect(const std::string& addr, int port, std::chrono::milliseconds timeout, bool ssl = false, const std::string& bind_ip = "0.0.0.0")
    {
      return connect(addr, std::to_string(port), timeout, ssl, bind_ip);
    }

    inline
			bool connect(const std::string& addr, const std::string& port, std::chrono::milliseconds timeout, bool ssl = false, const std::string& bind_ip = "0.0.0.0")
		{
      m_connected = false;
      m_ssl = ssl;
			try
			{
				m_ssl_socket.next_layer().close();

				// Set SSL options
				// disable sslv2
				m_ctx.set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2);
				m_ctx.set_default_verify_paths();

				// Get a list of endpoints corresponding to the server name.
				

				//////////////////////////////////////////////////////////////////////////

				boost::asio::ip::tcp::resolver resolver(m_io_service);
				boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), addr, port, boost::asio::ip::tcp::resolver::query::canonical_name);
				boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
				boost::asio::ip::tcp::resolver::iterator end;
				if(iterator == end)
				{
					LOG_ERROR("Failed to resolve " << addr);
					return false;
				}

				//////////////////////////////////////////////////////////////////////////


				//boost::asio::ip::tcp::endpoint remote_endpoint(boost::asio::ip::address::from_string(addr.c_str()), port);
				boost::asio::ip::tcp::endpoint remote_endpoint(*iterator);


				m_ssl_socket.next_layer().open(remote_endpoint.protocol());
				if(bind_ip != "0.0.0.0" && bind_ip != "0" && bind_ip != "" )
				{
					boost::asio::ip::tcp::endpoint local_endpoint(boost::asio::ip::address::from_string(addr.c_str()), 0);
					m_ssl_socket.next_layer().bind(local_endpoint);
				}

				
				m_deadline.expires_from_now(timeout);


				boost::system::error_code ec = boost::asio::error::would_block;

				m_ssl_socket.next_layer().async_connect(remote_endpoint, boost::lambda::var(ec) = boost::lambda::_1);
				while (ec == boost::asio::error::would_block)
				{	
					m_io_service.run_one(); 
				}
				
				if (!ec && m_ssl_socket.next_layer().is_open())
				{
					m_connected = true;
					m_deadline.expires_at(std::chrono::steady_clock::time_point::max());
					// SSL Options
					if(m_ssl) {
						// Disable verification of host certificate
						m_ssl_socket.set_verify_mode(boost::asio::ssl::verify_peer);
						// Handshake
						m_ssl_socket.next_layer().set_option(boost::asio::ip::tcp::no_delay(true));
						m_ssl_socket.handshake(boost::asio::ssl::stream_base::client);
					}
					return true;
				}else
				{
					MWARNING("Some problems at connect, message: " << ec.message());
					return false;
				}

			}
			catch(const boost::system::system_error& er)
			{
				MDEBUG("Some problems at connect, message: " << er.what());
				return false;
			}
			catch(...)
			{
				MDEBUG("Some fatal problems.");
				return false;
			}

			return true;
		}

		inline 
		bool disconnect()
		{
			try
			{	
				if(m_connected)
				{
					m_connected = false;
					if(m_ssl)
						shutdown_ssl();
					m_ssl_socket.next_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both);
				}
			}
			
			catch(const boost::system::system_error& /*er*/)
			{
				//LOG_ERROR("Some problems at disconnect, message: " << er.what());
				return false;
			}
			catch(...)
			{
				//LOG_ERROR("Some fatal problems.");
				return false;
			}
			return true;
		}


		inline 
		bool send(const std::string& buff, std::chrono::milliseconds timeout)
		{

			try
			{
				m_deadline.expires_from_now(timeout);

				// Set up the variable that receives the result of the asynchronous
				// operation. The error code is set to would_block to signal that the
				// operation is incomplete. Asio guarantees that its asynchronous
				// operations will never fail with would_block, so any other value in
				// ec indicates completion.
				boost::system::error_code ec = boost::asio::error::would_block;

				// Start the asynchronous operation itself. The boost::lambda function
				// object is used as a callback and will update the ec variable when the
				// operation completes. The blocking_udp_client.cpp example shows how you
				// can use boost::bind rather than boost::lambda.
				async_write(buff.c_str(), buff.size(), ec);

				// Block until the asynchronous operation has completed.
				while (ec == boost::asio::error::would_block)
				{
					m_io_service.run_one(); 
				}

				if (ec)
				{
					LOG_PRINT_L3("Problems at write: " << ec.message());
          m_connected = false;
					return false;
				}else
				{
					m_deadline.expires_at(std::chrono::steady_clock::time_point::max());
				}
			}

			catch(const boost::system::system_error& er)
			{
				LOG_ERROR("Some problems at connect, message: " << er.what());
				return false;
			}
			catch(...)
			{
				LOG_ERROR("Some fatal problems.");
				return false;
			}

			return true;
		}

		inline 
			bool send(const void* data, size_t sz)
		{
			try
			{
				/*
				m_deadline.expires_from_now(boost::posix_time::milliseconds(m_reciev_timeout));

				// Set up the variable that receives the result of the asynchronous
				// operation. The error code is set to would_block to signal that the
				// operation is incomplete. Asio guarantees that its asynchronous
				// operations will never fail with would_block, so any other value in
				// ec indicates completion.
				boost::system::error_code ec = boost::asio::error::would_block;

				// Start the asynchronous operation itself. The boost::lambda function
				// object is used as a callback and will update the ec variable when the
				// operation completes. The blocking_udp_client.cpp example shows how you
				// can use boost::bind rather than boost::lambda.
				boost::asio::async_write(m_socket, boost::asio::buffer(data, sz), boost::lambda::var(ec) = boost::lambda::_1);

				// Block until the asynchronous operation has completed.
				while (ec == boost::asio::error::would_block)
				{
					m_io_service.run_one();
				}
				*/
				boost::system::error_code ec;

				size_t writen = write(data, sz, ec);

				if (!writen || ec)
				{
					LOG_PRINT_L3("Problems at write: " << ec.message());
          m_connected = false;
					return false;
				}else
				{
					m_deadline.expires_at(std::chrono::steady_clock::time_point::max());
				}
			}

			catch(const boost::system::system_error& er)
			{
				LOG_ERROR("Some problems at send, message: " << er.what());
        m_connected = false;
				return false;
			}
			catch(...)
			{
				LOG_ERROR("Some fatal problems.");
				return false;
			}

			return true;
		}

		bool is_connected()
		{
			return m_connected && m_ssl_socket.next_layer().is_open();
		}

		inline 
		bool recv(std::string& buff, std::chrono::milliseconds timeout)
		{

			try
			{
				// Set a deadline for the asynchronous operation. Since this function uses
				// a composed operation (async_read_until), the deadline applies to the
				// entire operation, rather than individual reads from the socket.
				m_deadline.expires_from_now(timeout);

				// Set up the variable that receives the result of the asynchronous
				// operation. The error code is set to would_block to signal that the
				// operation is incomplete. Asio guarantees that its asynchronous
				// operations will never fail with would_block, so any other value in
				// ec indicates completion.
				//boost::system::error_code ec = boost::asio::error::would_block;

				// Start the asynchronous operation itself. The boost::lambda function
				// object is used as a callback and will update the ec variable when the
				// operation completes. The blocking_udp_client.cpp example shows how you
				// can use boost::bind rather than boost::lambda.

				boost::system::error_code ec = boost::asio::error::would_block;
				size_t bytes_transfered = 0;
			
				handler_obj hndlr(ec, bytes_transfered);

				char local_buff[10000] = {0};
				
				async_read(local_buff, sizeof(local_buff), boost::asio::transfer_at_least(1), hndlr);

				// Block until the asynchronous operation has completed.
				while (ec == boost::asio::error::would_block && !boost::interprocess::ipcdetail::atomic_read32(&m_shutdowned))
				{
					m_io_service.run_one(); 
				}


				if (ec)
				{
                    MTRACE("READ ENDS: Connection err_code " << ec.value());
                    if(ec == boost::asio::error::eof)
                    {
                      MTRACE("Connection err_code eof.");
                      //connection closed there, empty
                      return true;
                    }

					MDEBUG("Problems at read: " << ec.message());
                    m_connected = false;
					return false;
				}else
				{
                    MTRACE("READ ENDS: Success. bytes_tr: " << bytes_transfered);
					m_deadline.expires_at(std::chrono::steady_clock::time_point::max());
				}

				/*if(!bytes_transfered)
					return false;*/

				buff.assign(local_buff, bytes_transfered);
				return true;
			}

			catch(const boost::system::system_error& er)
			{
				LOG_ERROR("Some problems at read, message: " << er.what());
        m_connected = false;
				return false;
			}
			catch(...)
			{
				LOG_ERROR("Some fatal problems at read.");
				return false;
			}



			return false;

		}

		inline bool recv_n(std::string& buff, int64_t sz, std::chrono::milliseconds timeout)
		{

			try
			{
				// Set a deadline for the asynchronous operation. Since this function uses
				// a composed operation (async_read_until), the deadline applies to the
				// entire operation, rather than individual reads from the socket.
				m_deadline.expires_from_now(timeout);

				// Set up the variable that receives the result of the asynchronous
				// operation. The error code is set to would_block to signal that the
				// operation is incomplete. Asio guarantees that its asynchronous
				// operations will never fail with would_block, so any other value in
				// ec indicates completion.
				//boost::system::error_code ec = boost::asio::error::would_block;

				// Start the asynchronous operation itself. The boost::lambda function
				// object is used as a callback and will update the ec variable when the
				// operation completes. The blocking_udp_client.cpp example shows how you
				// can use boost::bind rather than boost::lambda.

				buff.resize(static_cast<size_t>(sz));
				boost::system::error_code ec = boost::asio::error::would_block;
				size_t bytes_transfered = 0;

				
				handler_obj hndlr(ec, bytes_transfered);
				async_read((char*)buff.data(), buff.size(), boost::asio::transfer_at_least(buff.size()), hndlr);
				
				// Block until the asynchronous operation has completed.
				while (ec == boost::asio::error::would_block && !boost::interprocess::ipcdetail::atomic_read32(&m_shutdowned))
				{
					m_io_service.run_one(); 
				}

				if (ec)
				{
					LOG_PRINT_L3("Problems at read: " << ec.message());
          m_connected = false;
					return false;
				}else
				{
					m_deadline.expires_at(std::chrono::steady_clock::time_point::max());
				}

				if(bytes_transfered != buff.size())
				{
					LOG_ERROR("Transferred mismatch with transfer_at_least value: m_bytes_transferred=" << bytes_transfered << " at_least value=" << buff.size());
					return false;
				}

				return true;
			}

			catch(const boost::system::system_error& er)
			{
				LOG_ERROR("Some problems at read, message: " << er.what());
        m_connected = false;
				return false;
			}
			catch(...)
			{
				LOG_ERROR("Some fatal problems at read.");
				return false;
			}



			return false;
		}
		
		bool shutdown()
		{
			m_deadline.cancel();
			boost::system::error_code ec;
			if(m_ssl)
				shutdown_ssl();
			m_ssl_socket.next_layer().cancel(ec);
			if(ec)
				MDEBUG("Problems at cancel: " << ec.message());
			m_ssl_socket.next_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
			if(ec)
				MDEBUG("Problems at shutdown: " << ec.message());
			m_ssl_socket.next_layer().close(ec);
			if(ec)
				MDEBUG("Problems at close: " << ec.message());
			boost::interprocess::ipcdetail::atomic_write32(&m_shutdowned, 1);
      m_connected = false;
			return true;
		}
		
		void set_connected(bool connected)
		{
			m_connected = connected;
		}
		boost::asio::io_service& get_io_service()
		{
			return m_io_service;
		}

		boost::asio::ip::tcp::socket& get_socket()
		{
			return m_ssl_socket.next_layer();
		}
		
	private:

		void check_deadline()
		{
			// Check whether the deadline has passed. We compare the deadline against
			// the current time since a new asynchronous operation may have moved the
			// deadline before this actor had a chance to run.
			if (m_deadline.expires_at() <= std::chrono::steady_clock::now())
			{
				// The deadline has passed. The socket is closed so that any outstanding
				// asynchronous operations are cancelled. This allows the blocked
				// connect(), read_line() or write_line() functions to return.
				LOG_PRINT_L3("Timed out socket");
        m_connected = false;
				m_ssl_socket.next_layer().close();

				// There is no longer an active deadline. The expiry is set to positive
				// infinity so that the actor takes no action until a new deadline is set.
				m_deadline.expires_at(std::chrono::steady_clock::time_point::max());
			}

			// Put the actor back to sleep.
			m_deadline.async_wait(boost::bind(&blocked_mode_client::check_deadline, this));
		}

		void shutdown_ssl() {
			// ssl socket shutdown blocks if server doesn't respond. We close after 2 secs
			boost::system::error_code ec = boost::asio::error::would_block;
			m_deadline.expires_from_now(std::chrono::milliseconds(2000));
			m_ssl_socket.async_shutdown(boost::lambda::var(ec) = boost::lambda::_1);
			while (ec == boost::asio::error::would_block)
			{
				m_io_service.run_one();
			}
			// Ignore "short read" error
			if (ec.category() == boost::asio::error::get_ssl_category() &&
			    ec.value() !=
#if BOOST_VERSION >= 106200
			    boost::asio::ssl::error::stream_truncated
#else // older Boost supports only OpenSSL 1.0, so 1.0-only macros are appropriate
			    ERR_PACK(ERR_LIB_SSL, 0, SSL_R_SHORT_READ)
#endif
			    )
				MDEBUG("Problems at ssl shutdown: " << ec.message());
		}
		
	protected:
		bool write(const void* data, size_t sz, boost::system::error_code& ec)
		{
			bool success;
			if(m_ssl)
				success = boost::asio::write(m_ssl_socket, boost::asio::buffer(data, sz), ec);
			else
				success = boost::asio::write(m_ssl_socket.next_layer(), boost::asio::buffer(data, sz), ec);
			return success;
		}
		
		void async_write(const void* data, size_t sz, boost::system::error_code& ec) 
		{
			if(m_ssl)
				boost::asio::async_write(m_ssl_socket, boost::asio::buffer(data, sz), boost::lambda::var(ec) = boost::lambda::_1);
			else
				boost::asio::async_write(m_ssl_socket.next_layer(), boost::asio::buffer(data, sz), boost::lambda::var(ec) = boost::lambda::_1);
		}
		
		void async_read(char* buff, size_t sz, boost::asio::detail::transfer_at_least_t transfer_at_least, handler_obj& hndlr)
		{
			if(!m_ssl)
				boost::asio::async_read(m_ssl_socket.next_layer(), boost::asio::buffer(buff, sz), transfer_at_least, hndlr);
			else
				boost::asio::async_read(m_ssl_socket, boost::asio::buffer(buff, sz), transfer_at_least, hndlr);
			
		}
		
	protected:
		boost::asio::io_service m_io_service;
		boost::asio::ssl::context m_ctx;
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket> m_ssl_socket;
		bool m_ssl;
		bool m_initialized;
		bool m_connected;
		boost::asio::steady_timer m_deadline;
		volatile uint32_t m_shutdowned;
	};


	/************************************************************************/
	/*                                                                      */
	/************************************************************************/
	class async_blocked_mode_client: public blocked_mode_client
	{
	public:
		async_blocked_mode_client():m_send_deadline(blocked_mode_client::m_io_service)
		{

			// No deadline is required until the first socket operation is started. We
			// set the deadline to positive infinity so that the actor takes no action
			// until a specific deadline is set.
			m_send_deadline.expires_at(boost::posix_time::pos_infin);

			// Start the persistent actor that checks for deadline expiry.
			check_send_deadline();
		}
		~async_blocked_mode_client()
		{
			m_send_deadline.cancel();
		}
		
		bool shutdown()
		{
			blocked_mode_client::shutdown();
			m_send_deadline.cancel();
			return true;
		}

		inline 
			bool send(const void* data, size_t sz)
		{
			try
			{
				/*
				m_send_deadline.expires_from_now(boost::posix_time::milliseconds(m_reciev_timeout));

				// Set up the variable that receives the result of the asynchronous
				// operation. The error code is set to would_block to signal that the
				// operation is incomplete. Asio guarantees that its asynchronous
				// operations will never fail with would_block, so any other value in
				// ec indicates completion.
				boost::system::error_code ec = boost::asio::error::would_block;

				// Start the asynchronous operation itself. The boost::lambda function
				// object is used as a callback and will update the ec variable when the
				// operation completes. The blocking_udp_client.cpp example shows how you
				// can use boost::bind rather than boost::lambda.
				boost::asio::async_write(m_socket, boost::asio::buffer(data, sz), boost::lambda::var(ec) = boost::lambda::_1);

				// Block until the asynchronous operation has completed.
				while(ec == boost::asio::error::would_block)
				{
					m_io_service.run_one();
				}*/
				
				boost::system::error_code ec;

				size_t writen = write(data, sz, ec);
				
				if (!writen || ec)
				{
					LOG_PRINT_L3("Problems at write: " << ec.message());
					return false;
				}else
				{
					m_send_deadline.expires_at(boost::posix_time::pos_infin);
				}
			}

			catch(const boost::system::system_error& er)
			{
				LOG_ERROR("Some problems at connect, message: " << er.what());
				return false;
			}
			catch(...)
			{
				LOG_ERROR("Some fatal problems.");
				return false;
			}

			return true;
		}


	private:

		boost::asio::deadline_timer m_send_deadline;

		void check_send_deadline()
		{
			// Check whether the deadline has passed. We compare the deadline against
			// the current time since a new asynchronous operation may have moved the
			// deadline before this actor had a chance to run.
			if (m_send_deadline.expires_at() <= boost::asio::deadline_timer::traits_type::now())
			{
				// The deadline has passed. The socket is closed so that any outstanding
				// asynchronous operations are cancelled. This allows the blocked
				// connect(), read_line() or write_line() functions to return.
				LOG_PRINT_L3("Timed out socket");
				m_ssl_socket.next_layer().close();

				// There is no longer an active deadline. The expiry is set to positive
				// infinity so that the actor takes no action until a new deadline is set.
				m_send_deadline.expires_at(boost::posix_time::pos_infin);
			}

			// Put the actor back to sleep.
			m_send_deadline.async_wait(boost::bind(&async_blocked_mode_client::check_send_deadline, this));
		}
	};
}
}
