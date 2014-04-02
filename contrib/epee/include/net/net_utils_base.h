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



#ifndef _NET_UTILS_BASE_H_
#define _NET_UTILS_BASE_H_

#include <boost/uuid/uuid.hpp>
#include "string_tools.h"

#ifndef MAKE_IP
#define MAKE_IP( a1, a2, a3, a4 )	(a1|(a2<<8)|(a3<<16)|(a4<<24))
#endif


namespace epee
{
namespace net_utils
{
	/************************************************************************/
	/*                                                                      */
	/************************************************************************/
	struct connection_context_base
	{
    const boost::uuids::uuid m_connection_id;
		const uint32_t m_remote_ip;
		const uint32_t m_remote_port;
    const bool     m_is_income;
    const time_t   m_started;
    time_t   m_last_recv;
    time_t   m_last_send;
    uint64_t m_recv_cnt;
    uint64_t m_send_cnt;

    connection_context_base(boost::uuids::uuid connection_id, long remote_ip, int remote_port, bool is_income, time_t last_recv = 0, time_t last_send = 0, uint64_t recv_cnt = 0, uint64_t send_cnt = 0):
                                            m_connection_id(connection_id),
                                            m_remote_ip(remote_ip),
                                            m_remote_port(remote_port),
                                            m_is_income(is_income),
                                            m_last_recv(last_recv),
                                            m_last_send(last_send),
                                            m_recv_cnt(recv_cnt),
                                            m_send_cnt(send_cnt),
                                            m_started(time(NULL))
    {}

    connection_context_base(): m_connection_id(),
                               m_remote_ip(0),
                               m_remote_port(0),
                               m_is_income(false),
                               m_last_recv(0),
                               m_last_send(0),
                               m_recv_cnt(0),
                               m_send_cnt(0),
                               m_started(time(NULL))
    {}

    connection_context_base& operator=(const connection_context_base& a)
    {
      set_details(a.m_connection_id, a.m_remote_ip, a.m_remote_port, a.m_is_income);
      return *this;
    }
    
  private:
    template<class t_protocol_handler>
    friend class connection;
    void set_details(boost::uuids::uuid connection_id, long remote_ip, int remote_port, bool is_income)
    {
      this->~connection_context_base();
      new(this) connection_context_base(connection_id, remote_ip, remote_port, is_income);
    }

	};

	/************************************************************************/
	/*                                                                      */
	/************************************************************************/
	struct i_service_endpoint
	{
		virtual bool do_send(const void* ptr, size_t cb)=0;
    virtual bool close()=0;
    virtual bool call_run_once_service_io()=0;
    virtual bool request_callback()=0;
    virtual boost::asio::io_service& get_io_service()=0;
    //protect from deletion connection object(with protocol instance) during external call "invoke"
    virtual bool add_ref()=0;
    virtual bool release()=0;
  protected:
    virtual ~i_service_endpoint(){}
	};


  //some helpers


  inline 
    std::string print_connection_context(const connection_context_base& ctx)
  {
    std::stringstream ss;
    ss << epee::string_tools::get_ip_string_from_int32(ctx.m_remote_ip) << ":" << ctx.m_remote_port << " " << epee::string_tools::get_str_from_guid_a(ctx.m_connection_id) << (ctx.m_is_income ? " INC":" OUT");
    return ss.str();
  }

  inline 
    std::string print_connection_context_short(const connection_context_base& ctx)
  {
    std::stringstream ss;
    ss << epee::string_tools::get_ip_string_from_int32(ctx.m_remote_ip) << ":" << ctx.m_remote_port << (ctx.m_is_income ? " INC":" OUT");
    return ss.str();
  }

#define LOG_PRINT_CC(ct, message, log_level) LOG_PRINT("[" << epee::net_utils::print_connection_context_short(ct) << "]" << message, log_level)
#define LOG_PRINT_CC_GREEN(ct, message, log_level) LOG_PRINT_GREEN("[" << epee::net_utils::print_connection_context_short(ct) << "]" << message, log_level)
#define LOG_PRINT_CC_RED(ct, message, log_level) LOG_PRINT_RED("[" << epee::net_utils::print_connection_context_short(ct) << "]" << message, log_level)
#define LOG_PRINT_CC_BLUE(ct, message, log_level) LOG_PRINT_BLUE("[" << epee::net_utils::print_connection_context_short(ct) << "]" << message, log_level)
#define LOG_PRINT_CC_YELLOW(ct, message, log_level) LOG_PRINT_YELLOW("[" << epee::net_utils::print_connection_context_short(ct) << "]" << message, log_level)
#define LOG_PRINT_CC_CYAN(ct, message, log_level) LOG_PRINT_CYAN("[" << epee::net_utils::print_connection_context_short(ct) << "]" << message, log_level)
#define LOG_PRINT_CC_MAGENTA(ct, message, log_level) LOG_PRINT_MAGENTA("[" << epee::net_utils::print_connection_context_short(ct) << "]" << message, log_level)
#define LOG_ERROR_CC(ct, message) LOG_ERROR("[" << epee::net_utils::print_connection_context_short(ct) << "]" << message)

#define LOG_PRINT_CC_L0(ct, message) LOG_PRINT_L0("[" << epee::net_utils::print_connection_context_short(ct) << "]" << message)
#define LOG_PRINT_CC_L1(ct, message) LOG_PRINT_L1("[" << epee::net_utils::print_connection_context_short(ct) << "]" << message)
#define LOG_PRINT_CC_L2(ct, message) LOG_PRINT_L2("[" << epee::net_utils::print_connection_context_short(ct) << "]" << message)
#define LOG_PRINT_CC_L3(ct, message) LOG_PRINT_L3("[" << epee::net_utils::print_connection_context_short(ct) << "]" << message)
#define LOG_PRINT_CC_L4(ct, message) LOG_PRINT_L4("[" << epee::net_utils::print_connection_context_short(ct) << "]" << message)

#define LOG_PRINT_CCONTEXT_L0(message) LOG_PRINT_CC_L0(context, message)
#define LOG_PRINT_CCONTEXT_L1(message) LOG_PRINT_CC_L1(context, message)
#define LOG_PRINT_CCONTEXT_L2(message) LOG_PRINT_CC_L2(context, message)
#define LOG_PRINT_CCONTEXT_L3(message) LOG_PRINT_CC_L3(context, message)
#define LOG_ERROR_CCONTEXT(message)    LOG_ERROR_CC(context, message)
 
#define LOG_PRINT_CCONTEXT_GREEN(message, log_level) LOG_PRINT_CC_GREEN(context, message, log_level)
#define LOG_PRINT_CCONTEXT_RED(message, log_level) LOG_PRINT_CC_RED(context, message, log_level)
#define LOG_PRINT_CCONTEXT_BLUE(message, log_level) LOG_PRINT_CC_BLUE(context, message, log_level) 
#define LOG_PRINT_CCONTEXT_YELLOW(message, log_level) LOG_PRINT_CC_YELLOW(context, message, log_level) 
#define LOG_PRINT_CCONTEXT_CYAN(message, log_level) LOG_PRINT_CC_CYAN(context, message, log_level) 
#define LOG_PRINT_CCONTEXT_MAGENTA(message, log_level) LOG_PRINT_CC_MAGENTA(context, message, log_level) 

#define CHECK_AND_ASSERT_MES_CC(condition, return_val, err_message) CHECK_AND_ASSERT_MES(condition, return_val, "[" << epee::net_utils::print_connection_context_short(context) << "]" << err_message)

}
}

#endif //_NET_UTILS_BASE_H_
