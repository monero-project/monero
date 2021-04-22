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

#include "string_tools.h"
#include "string_tools_lexical.h"


// Previously pulled in by ASIO, further cleanup still required ...
#ifdef _WIN32
# include <winsock2.h>
# include <windows.h>
#endif

#include <locale>
#include <cstdlib>
#include <string>
#include <type_traits>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/utility/string_ref.hpp>
#include "misc_log_ex.h"
#include "storages/parserse_base_utils.h"
#include "hex.h"
#include "memwipe.h"
#include "mlocker.h"
#include "span.h"
#include "warnings.h"

#include <ctype.h>

#ifdef _WIN32
# include <winsock2.h>
#else
# include <arpa/inet.h>
# include <netinet/in.h>
#endif

namespace epee
{
namespace string_tools
{
  std::string get_ip_string_from_int32(uint32_t ip)
  {
    in_addr adr;
    adr.s_addr = ip;
    const char* pbuf = inet_ntoa(adr);
    if(pbuf)
      return pbuf;
    else
      return "[failed]";
  }
  //----------------------------------------------------------------------------
  bool get_ip_int32_from_string(uint32_t& ip, const std::string& ip_str)
  {
    ip = inet_addr(ip_str.c_str());
    if(INADDR_NONE == ip)
      return false;

    return true;
  }
  //----------------------------------------------------------------------------
  bool validate_hex(uint64_t length, const std::string& str)
  {
    if (str.size() != length)
      return false;
    for (char c: str)
      if (!isxdigit(c))
        return false;
    return true;
  }
  //----------------------------------------------------------------------------
  bool parse_peer_from_string(uint32_t& ip, uint16_t& port, const std::string& addres)
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
  
  std::string num_to_string_fast(int64_t val)
  {
    /*
    char  buff[30] = {0};
    i64toa_s(val, buff, sizeof(buff)-1, 10);
    return buff;*/
    return boost::lexical_cast<std::string>(val);
  }
	
	
    bool compare_no_case(const std::string& str1, const std::string& str2)
	{
		
		return !boost::iequals(str1, str2);
	}
	//----------------------------------------------------------------------------
	 std::string& get_current_module_name()
	{
		static std::string module_name;
		return module_name;
	}
	//----------------------------------------------------------------------------
	 std::string& get_current_module_folder()
	{	
		static std::string module_folder;
		return module_folder;
	}
	
#ifdef _WIN32
  std::string get_current_module_path()
  {
    char pname [5000] = {0};
    GetModuleFileNameA( NULL, pname, sizeof(pname));
    pname[sizeof(pname)-1] = 0; //be happy ;)
    return pname;
  }
#endif
	
    bool set_module_name_and_folder(const std::string& path_to_process_)
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
	bool trim_left(std::string& str)
	{
		for(std::string::iterator it = str.begin(); it!= str.end() && isspace(static_cast<unsigned char>(*it));)
			str.erase(str.begin());
			
		return true;
	}
	//----------------------------------------------------------------------------
	bool trim_right(std::string& str)
	{

		for(std::string::reverse_iterator it = str.rbegin(); it!= str.rend() && isspace(static_cast<unsigned char>(*it));)
			str.erase( --((it++).base()));

		return true;
	}
  //----------------------------------------------------------------------------
  std::string pad_string(std::string s, size_t n, char c, bool prepend)
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
  
    std::string get_extension(const std::string& str)
	{
		std::string res;
		std::string::size_type pos = str.rfind('.');
		if(std::string::npos == pos)
			return res;
		
		res = str.substr(pos+1, str.size()-pos);
		return res;
	}
	//----------------------------------------------------------------------------
	std::string cut_off_extension(const std::string& str)
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
  std::wstring utf8_to_utf16(const std::string& str)
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
  std::string utf16_to_utf8(const std::wstring& wstr)
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

