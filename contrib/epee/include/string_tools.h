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



#ifndef _STRING_TOOLS_H_
#define _STRING_TOOLS_H_

// Previously pulled in by ASIO, further cleanup still required ...
#ifdef _WIN32
# include <winsock2.h>
# include <windows.h>
#endif

#include <string.h>
#include <locale>
#include <cstdlib>
#include <string>
#include <type_traits>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "hex.h"
#include "memwipe.h"
#include "mlocker.h"
#include "span.h"
#include "warnings.h"


#ifndef OUT
	#define OUT
#endif

#ifdef WINDOWS_PLATFORM
#pragma comment (lib, "Rpcrt4.lib")
#endif

namespace epee
{
namespace string_tools
{
	//----------------------------------------------------------------------------
	inline std::string get_str_from_guid_a(const boost::uuids::uuid& rid)
	{
		return boost::lexical_cast<std::string>(rid);
	}
	//----------------------------------------------------------------------------
	inline bool get_guid_from_string(OUT boost::uuids::uuid& inetifer, const std::string& str_id)
	{
		std::string local_str_id = str_id;
		if(local_str_id.size() < 36)
			return false;

		if('{' == *local_str_id.begin())
			local_str_id.erase(0, 1);

		if('}' == *(--local_str_id.end()))
			local_str_id.erase(--local_str_id.end());

		try
		{
			inetifer = boost::lexical_cast<boost::uuids::uuid>(local_str_id);
			return true;
		}
		catch(...)
		{
			return false;
		}
	}
  //----------------------------------------------------------------------------
  inline std::string buff_to_hex_nodelimer(const std::string& src)
  {
    return to_hex::string(to_byte_span(to_span(src)));
  }
  //----------------------------------------------------------------------------
  template<class CharT>
  bool parse_hexstr_to_binbuff(const std::basic_string<CharT>& s, std::basic_string<CharT>& res, bool allow_partial_byte = false)
  {
    res.clear();
    if (!allow_partial_byte && (s.size() & 1))
      return false;
    try
    {
      long v = 0;
      for(size_t i = 0; i < (s.size() + 1) / 2; i++)
      {
        CharT byte_str[3];
        size_t copied = s.copy(byte_str, 2, 2 * i);
        byte_str[copied] = CharT(0);
        CharT* endptr;
        v = strtoul(byte_str, &endptr, 16);
        if (v < 0 || 0xFF < v || endptr != byte_str + copied)
        {
          return false;
        }
        res.push_back(static_cast<unsigned char>(v));
      }

      return true;
    }catch(...)
    {
      return false;
    }
  }
  //----------------------------------------------------------------------------
  template<class t_pod_type>
  bool parse_tpod_from_hex_string(const std::string& str_hash, t_pod_type& t_pod)
  {
    static_assert(std::is_pod<t_pod_type>::value, "expected pod type");
    std::string buf;
    bool res = epee::string_tools::parse_hexstr_to_binbuff(str_hash, buf);
    if (!res || buf.size() != sizeof(t_pod_type))
    {
      return false;
    }
    else
    {
      buf.copy(reinterpret_cast<char *>(&t_pod), sizeof(t_pod_type));
      return true;
    }
  }
  //----------------------------------------------------------------------------
PUSH_WARNINGS
DISABLE_GCC_WARNING(maybe-uninitialized)
  template<class XType>
  inline bool get_xtype_from_string(OUT XType& val, const std::string& str_id)
  {
    if (std::is_integral<XType>::value && !std::numeric_limits<XType>::is_signed && !std::is_same<XType, bool>::value)
    {
      for (char c : str_id)
      {
        if (!std::isdigit(c))
          return false;
      }
    }

    try
    {
      val = boost::lexical_cast<XType>(str_id);
      return true;
    }
    catch(const std::exception& /*e*/)
    {
      //const char* pmsg = e.what();
      return false;
    }
    catch(...)
    {
      return false;
    }

    return true;
  }
POP_WARNINGS
	//----------------------------------------------------------------------------
	template<class XType>
	inline bool xtype_to_string(const  XType& val, std::string& str)
	{
		try
		{
			str = boost::lexical_cast<std::string>(val);
		}
		catch(...)
		{
			return false;
		}

		return true;
	}
	//----------------------------------------------------------------------------
	std::string get_ip_string_from_int32(uint32_t ip);
	//----------------------------------------------------------------------------
	bool get_ip_int32_from_string(uint32_t& ip, const std::string& ip_str);
  //----------------------------------------------------------------------------
  inline bool parse_peer_from_string(uint32_t& ip, uint16_t& port, const std::string& addres)
  {
    //parse ip and address
    std::string::size_type p = addres.find(':');
    std::string ip_str, port_str;
    if(p == std::string::npos)
    {
      port = 0;
      ip_str = addres;
    }
    else
    {
      ip_str = addres.substr(0, p);
      port_str = addres.substr(p+1, addres.size());
    }

    if(!get_ip_int32_from_string(ip, ip_str))
    {
      return false;
    }

    if(p != std::string::npos && !get_xtype_from_string(port, port_str))
    {
      return false;
    }
    return true;
  }

  inline bool parse_ipv6_peer_from_string(std::string& ip, uint16_t& port, const std::string& address)
  {
    //parse ip and address

    size_t last_colon_position = 0;

    if (address.find("::") == std::string::npos) return false;

    last_colon_position = address.find_last_of(':');

    if (address[last_colon_position - 1] == ':')
    {
      ip = address;
      port = 0;
      return true;
    }

    std::string ip_str, port_str;
    if(last_colon_position == std::string::npos || last_colon_position == (address.size() - 1))
    {
      return false;
    }
    else
    {
      ip = address.substr(0, last_colon_position);
      port_str = address.substr(last_colon_position + 1, address.size());
      if (port_str.size() == 0) return false;
    }

    if(!get_xtype_from_string(port, port_str))
    {
      return false;
    }
    return true;
  }

	inline std::string num_to_string_fast(int64_t val)
	{
		/*
		char  buff[30] = {0};
		i64toa_s(val, buff, sizeof(buff)-1, 10);
		return buff;*/
		return boost::lexical_cast<std::string>(val);
	}
	//----------------------------------------------------------------------------
	
	inline bool compare_no_case(const std::string& str1, const std::string& str2)
	{
		
		return !boost::iequals(str1, str2);
	}
	//----------------------------------------------------------------------------
	inline std::string& get_current_module_name()
	{
		static std::string module_name;
		return module_name;
	}
	//----------------------------------------------------------------------------
	inline std::string& get_current_module_folder()
	{	
		static std::string module_folder;
		return module_folder;
	}
  //----------------------------------------------------------------------------
#ifdef _WIN32
  inline std::string get_current_module_path()
  {
    char pname [5000] = {0};
    GetModuleFileNameA( NULL, pname, sizeof(pname));
    pname[sizeof(pname)-1] = 0; //be happy ;)
    return pname;
  }
#endif
	//----------------------------------------------------------------------------
	inline bool set_module_name_and_folder(const std::string& path_to_process_)
	{
    std::string path_to_process = path_to_process_;
#ifdef _WIN32
    path_to_process = get_current_module_path();
#endif 
		std::string::size_type a = path_to_process.rfind( '\\' );
		if(a == std::string::npos )
		{
			a = path_to_process.rfind( '/' );
		}
		if ( a != std::string::npos )
		{	
			get_current_module_name() = path_to_process.substr(a+1, path_to_process.size());
			get_current_module_folder() = path_to_process.substr(0, a);
			return true;
		}else
			return false;

	}

	//----------------------------------------------------------------------------
	inline bool trim_left(std::string& str)
	{
		for(std::string::iterator it = str.begin(); it!= str.end() && isspace(static_cast<unsigned char>(*it));)
			str.erase(str.begin());
			
		return true;
	}
	//----------------------------------------------------------------------------
	inline bool trim_right(std::string& str)
	{

		for(std::string::reverse_iterator it = str.rbegin(); it!= str.rend() && isspace(static_cast<unsigned char>(*it));)
			str.erase( --((it++).base()));

		return true;
	}
	//----------------------------------------------------------------------------
	inline std::string& trim(std::string& str)
	{

		trim_left(str);
		trim_right(str);
		return str;
	}
  //----------------------------------------------------------------------------
  inline std::string trim(const std::string& str_)
  {
    std::string str = str_;
    trim_left(str);
    trim_right(str);
    return str;
  }
  //----------------------------------------------------------------------------
  inline std::string pad_string(std::string s, size_t n, char c = ' ', bool prepend = false)
  {
    if (s.size() < n)
    {
      if (prepend)
        s = std::string(n - s.size(), c) + s;
      else
        s.append(n - s.size(), c);
    }
    return s;
  }
  //----------------------------------------------------------------------------
  template<class t_pod_type>
  std::string pod_to_hex(const t_pod_type& s)
  {
    static_assert(std::is_standard_layout<t_pod_type>(), "expected standard layout type");
    return to_hex::string(as_byte_span(s));
  }
  //----------------------------------------------------------------------------
  template<class t_pod_type>
  bool hex_to_pod(const std::string& hex_str, t_pod_type& s)
  {
    static_assert(std::is_pod<t_pod_type>::value, "expected pod type");
    std::string hex_str_tr = trim(hex_str);
    if(sizeof(s)*2 != hex_str.size())
      return false;
    std::string bin_buff;
    if(!parse_hexstr_to_binbuff(hex_str_tr, bin_buff))
      return false;
    if(bin_buff.size()!=sizeof(s))
      return false;

    s = *(t_pod_type*)bin_buff.data();
    return true;
  }
  //----------------------------------------------------------------------------
  template<class t_pod_type>
  bool hex_to_pod(const std::string& hex_str, tools::scrubbed<t_pod_type>& s)
  {
    return hex_to_pod(hex_str, unwrap(s));
  }
  //----------------------------------------------------------------------------
  template<class t_pod_type>
  bool hex_to_pod(const std::string& hex_str, epee::mlocked<t_pod_type>& s)
  {
    return hex_to_pod(hex_str, unwrap(s));
  }
  //----------------------------------------------------------------------------
  bool validate_hex(uint64_t length, const std::string& str);
  //----------------------------------------------------------------------------
	inline std::string get_extension(const std::string& str)
	{
		std::string res;
		std::string::size_type pos = str.rfind('.');
		if(std::string::npos == pos)
			return res;
		
		res = str.substr(pos+1, str.size()-pos);
		return res;
	}
	//----------------------------------------------------------------------------
	inline std::string cut_off_extension(const std::string& str)
	{
		std::string res;
		std::string::size_type pos = str.rfind('.');
		if(std::string::npos == pos)
			return str;

		res = str.substr(0, pos);
		return res;
	}
  //----------------------------------------------------------------------------
#ifdef _WIN32
  inline std::wstring utf8_to_utf16(const std::string& str)
  {
    if (str.empty())
      return {};
    int wstr_size = MultiByteToWideChar(CP_UTF8, 0, &str[0], str.size(), NULL, 0);
    if (wstr_size == 0)
    {
      throw std::runtime_error(std::error_code(GetLastError(), std::system_category()).message());
    }
    std::wstring wstr(wstr_size, wchar_t{});
    if (!MultiByteToWideChar(CP_UTF8, 0, &str[0], str.size(), &wstr[0], wstr_size))
    {
      throw std::runtime_error(std::error_code(GetLastError(), std::system_category()).message());
    }
    return wstr;
  }
  inline std::string utf16_to_utf8(const std::wstring& wstr)
  {
    if (wstr.empty())
      return {};
    int str_size = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size(), NULL, 0, NULL, NULL);
    if (str_size == 0)
    {
      throw std::runtime_error(std::error_code(GetLastError(), std::system_category()).message());
    }
    std::string str(str_size, char{});
    if (!WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size(), &str[0], str_size, NULL, NULL))
    {
      throw std::runtime_error(std::error_code(GetLastError(), std::system_category()).message());
    }
    return str;
  }
#endif
}
}
#endif //_STRING_TOOLS_H_
