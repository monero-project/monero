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

#include <typeinfo>
#include <boost/asio/io_service.hpp>
#include <boost/uuid/uuid.hpp>
#include "serialization/keyvalue_serialization.h"
#include "net/local_ip.h"
#include "string_tools.h"
#include "misc_log_ex.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net"

#ifndef MAKE_IP
#define MAKE_IP( a1, a2, a3, a4 )	(a1|(a2<<8)|(a3<<16)|(a4<<24))
#endif


namespace epee
{
namespace net_utils
{
	struct network_address_base
	{
	public:
		bool operator==(const network_address_base &other) const { return m_full_id == other.m_full_id; }
		bool operator!=(const network_address_base &other) const { return !operator==(other); }
		bool operator<(const network_address_base &other) const { return m_full_id < other.m_full_id; }
		bool is_same_host(const network_address_base &other) const { return m_host_id == other.m_host_id; }
		virtual std::string str() const = 0;
		virtual std::string host_str() const = 0;
		virtual bool is_loopback() const = 0;
		virtual bool is_local() const = 0;
		virtual uint8_t get_type_id() const = 0;
	protected:
		// A very simple non cryptographic hash function by Fowler, Noll, Vo
		uint64_t fnv1a(const uint8_t *data, size_t len) const {
			uint64_t h = 0xcbf29ce484222325;
			while (len--)
				h = (h ^ *data++) * 0x100000001b3;
			return h;
		}
		uint64_t m_host_id;
		uint64_t m_full_id;
	};
	struct ipv4_network_address: public network_address_base
	{
		void init_ids()
		{
			m_host_id = fnv1a((const uint8_t*)&m_ip, sizeof(m_ip));
			m_full_id = fnv1a((const uint8_t*)&m_ip, sizeof(m_ip) + sizeof(m_port));
		}
	public:
		ipv4_network_address(uint32_t ip, uint16_t port): network_address_base(), m_ip(ip), m_port(port) { init_ids(); }
		uint32_t ip() const { return m_ip; }
		uint16_t port() const { return m_port; }
		virtual std::string str() const { return epee::string_tools::get_ip_string_from_int32(m_ip) + ":" + std::to_string(m_port); }
		virtual std::string host_str() const { return epee::string_tools::get_ip_string_from_int32(m_ip); }
		virtual bool is_loopback() const { return epee::net_utils::is_ip_loopback(m_ip); }
		virtual bool is_local() const { return epee::net_utils::is_ip_local(m_ip); }
		virtual uint8_t get_type_id() const { return ID; }
	public: // serialization
		static const uint8_t ID = 1;
#pragma pack(push)
#pragma pack(1)
		uint32_t m_ip;
		uint16_t m_port;
#pragma pack(pop)
		BEGIN_KV_SERIALIZE_MAP()
			KV_SERIALIZE(m_ip)
			KV_SERIALIZE(m_port)
			if (!is_store)
				const_cast<ipv4_network_address&>(this_ref).init_ids();
		END_KV_SERIALIZE_MAP()
	};
	class network_address: public boost::shared_ptr<network_address_base>
	{
	public:
		network_address() {}
		network_address(ipv4_network_address *address): boost::shared_ptr<network_address_base>(address) {}
		bool operator==(const network_address &other) const { return (*this)->operator==(*other); }
		bool operator!=(const network_address &other) const { return (*this)->operator!=(*other); }
		bool operator<(const network_address &other) const { return (*this)->operator<(*other); }
		bool is_same_host(const network_address &other) const { return (*this)->is_same_host(*other); }
		std::string str() const { return (*this) ? (*this)->str() : "<none>"; }
		std::string host_str() const { return (*this) ? (*this)->host_str() : "<none>"; }
		bool is_loopback() const { return (*this)->is_loopback(); }
		bool is_local() const { return (*this)->is_local(); }
		uint8_t get_type_id() const { return (*this)->get_type_id(); }
		template<typename Type> Type &as() { if (get_type_id() != Type::ID) throw std::runtime_error("Bad type"); return *(Type*)get(); }
		template<typename Type> const Type &as() const { if (get_type_id() != Type::ID) throw std::runtime_error("Bad type"); return *(const Type*)get(); }

		BEGIN_KV_SERIALIZE_MAP()
			uint8_t type = is_store ? this_ref.get_type_id() : 0;
			epee::serialization::selector<is_store>::serialize(type, stg, hparent_section, "type");
			switch (type)
			{
				case ipv4_network_address::ID:
					if (!is_store)
						const_cast<network_address&>(this_ref).reset(new ipv4_network_address(0, 0));
					KV_SERIALIZE(template as<ipv4_network_address>());
					break;
				default: MERROR("Unsupported network address type: " << type); return false;
			}
		END_KV_SERIALIZE_MAP()
	};
	inline bool create_network_address(network_address &address, const std::string &string, uint16_t default_port = 0)
	{
		uint32_t ip;
		uint16_t port;
		if (epee::string_tools::parse_peer_from_string(ip, port, string))
		{
			if (default_port && !port)
				port = default_port;
			address.reset(new ipv4_network_address(ip, port));
			return true;
		}
		return false;
	}
	/************************************************************************/
	/*                                                                      */
	/************************************************************************/
	struct connection_context_base
	{
    const boost::uuids::uuid m_connection_id;
    const network_address m_remote_address;
    const bool     m_is_income;
    const time_t   m_started;
    time_t   m_last_recv;
    time_t   m_last_send;
    uint64_t m_recv_cnt;
    uint64_t m_send_cnt;
    double m_current_speed_down;
    double m_current_speed_up;

    connection_context_base(boost::uuids::uuid connection_id,
                            const network_address &remote_address, bool is_income,
                            time_t last_recv = 0, time_t last_send = 0,
                            uint64_t recv_cnt = 0, uint64_t send_cnt = 0):
                                            m_connection_id(connection_id),
                                            m_remote_address(remote_address),
                                            m_is_income(is_income),
                                            m_started(time(NULL)),
                                            m_last_recv(last_recv),
                                            m_last_send(last_send),
                                            m_recv_cnt(recv_cnt),
                                            m_send_cnt(send_cnt),
                                            m_current_speed_down(0),
                                            m_current_speed_up(0)
    {}

    connection_context_base(): m_connection_id(),
                               m_remote_address(new ipv4_network_address(0,0)),
                               m_is_income(false),
                               m_started(time(NULL)),
                               m_last_recv(0),
                               m_last_send(0),
                               m_recv_cnt(0),
                               m_send_cnt(0),
                               m_current_speed_down(0),
                               m_current_speed_up(0)
    {}

    connection_context_base& operator=(const connection_context_base& a)
    {
      set_details(a.m_connection_id, a.m_remote_address, a.m_is_income);
      return *this;
    }
    
  private:
    template<class t_protocol_handler>
    friend class connection;
    void set_details(boost::uuids::uuid connection_id, const network_address &remote_address, bool is_income)
    {
      this->~connection_context_base();
      new(this) connection_context_base(connection_id, remote_address, is_income);
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
    virtual ~i_service_endpoint() noexcept(false) {}
	};


  //some helpers


  inline 
    std::string print_connection_context(const connection_context_base& ctx)
  {
    std::stringstream ss;
    ss << ctx.m_remote_address->str() << " " << epee::string_tools::get_str_from_guid_a(ctx.m_connection_id) << (ctx.m_is_income ? " INC":" OUT");
    return ss.str();
  }

  inline 
    std::string print_connection_context_short(const connection_context_base& ctx)
  {
    std::stringstream ss;
    ss << ctx.m_remote_address->str() << (ctx.m_is_income ? " INC":" OUT");
    return ss.str();
  }

inline MAKE_LOGGABLE(connection_context_base, ct, os)
{
  os << "[" << epee::net_utils::print_connection_context_short(ct) << "] ";
  return os;
}

#define LOG_ERROR_CC(ct, message) MERROR(ct << message)
#define LOG_WARNING_CC(ct, message) MWARNING(ct << message)
#define LOG_INFO_CC(ct, message) MINFO(ct << message)
#define LOG_DEBUG_CC(ct, message) MDEBUG(ct << message)
#define LOG_TRACE_CC(ct, message) MTRACE(ct << message)
#define LOG_CC(level, ct, message) MLOG(level, ct << message)

#define LOG_PRINT_CC_L0(ct, message) LOG_PRINT_L0(ct << message)
#define LOG_PRINT_CC_L1(ct, message) LOG_PRINT_L1(ct << message)
#define LOG_PRINT_CC_L2(ct, message) LOG_PRINT_L2(ct << message)
#define LOG_PRINT_CC_L3(ct, message) LOG_PRINT_L3(ct << message)
#define LOG_PRINT_CC_L4(ct, message) LOG_PRINT_L4(ct << message)

#define LOG_PRINT_CCONTEXT_L0(message) LOG_PRINT_CC_L0(context, message)
#define LOG_PRINT_CCONTEXT_L1(message) LOG_PRINT_CC_L1(context, message)
#define LOG_PRINT_CCONTEXT_L2(message) LOG_PRINT_CC_L2(context, message)
#define LOG_PRINT_CCONTEXT_L3(message) LOG_PRINT_CC_L3(context, message)
#define LOG_ERROR_CCONTEXT(message)    LOG_ERROR_CC(context, message)
 
#define CHECK_AND_ASSERT_MES_CC(condition, return_val, err_message) CHECK_AND_ASSERT_MES(condition, return_val, "[" << epee::net_utils::print_connection_context_short(context) << "]" << err_message)

}
}

#endif //_NET_UTILS_BASE_H_
