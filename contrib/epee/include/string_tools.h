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

//#include <objbase.h>
#include <locale>
#include <cstdlib>
//#include <strsafe.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string.hpp>
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
	inline std::wstring get_str_from_guid(const boost::uuids::uuid& rid)
	{
		return boost::lexical_cast<std::wstring>(rid);
	}
	//----------------------------------------------------------------------------
	inline std::string get_str_from_guid_a(const boost::uuids::uuid& rid)
	{
		return boost::lexical_cast<std::string>(rid);
	}
	//----------------------------------------------------------------------------
	inline bool get_guid_from_string( boost::uuids::uuid& inetifer, std::wstring str_id)
	{
		if(str_id.size() < 36)
			return false;

		if('{' == *str_id.begin())
			str_id.erase(0, 1);

		if('}' == *(--str_id.end()))
			str_id.erase(--str_id.end());

		try
		{
			inetifer = boost::lexical_cast<boost::uuids::uuid>(str_id);
			return true;
		}
		catch(...)
		{
			return false;
		}
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
  template<class CharT>
  std::basic_string<CharT> buff_to_hex(const std::basic_string<CharT>& s)
  {
    using namespace std;
    basic_stringstream<CharT> hexStream;
    hexStream << hex << noshowbase << setw(2);

    for(typename std::basic_string<CharT>::const_iterator it = s.begin(); it != s.end(); it++)
    {
      hexStream << "0x"<< static_cast<unsigned int>(static_cast<unsigned char>(*it)) << " ";
    }
    return hexStream.str();
  }
  //----------------------------------------------------------------------------
  template<class CharT>
  std::basic_string<CharT> buff_to_hex_nodelimer(const std::basic_string<CharT>& s)
  {
    using namespace std;
    basic_stringstream<CharT> hexStream;
    hexStream << hex << noshowbase;

    for(typename std::basic_string<CharT>::const_iterator it = s.begin(); it != s.end(); it++)
    {
      hexStream << setw(2) << setfill('0') << static_cast<unsigned int>(static_cast<unsigned char>(*it));
    }
    return hexStream.str();
  }
  //----------------------------------------------------------------------------
  template<class CharT>
  bool parse_hexstr_to_binbuff(const std::basic_string<CharT>& s, std::basic_string<CharT>& res)
  {
    res.clear();
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
    catch(std::exception& /*e*/)
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
	//---------------------------------------------------
	template<typename int_t>
	bool get_xnum_from_hex_string(const std::string str, int_t& res )
	{
		try
		{
			std::stringstream ss;
			ss << std::hex << str;
			ss >> res;
			return true;	
		}
		catch(...)
		{
			return false;
		}
	}
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
    
	typedef std::map<std::string, std::string> command_line_params_a;
	typedef std::map<std::wstring, std::wstring> command_line_params_w;

	template<class t_string>
	bool parse_commandline(std::map<t_string, t_string>& res, int argc, char** argv)
	{
    t_string key;
    for(int i = 1; i < argc; i++)
    {
      if(!argv[i])
        break;
      t_string s = argv[i];
      std::string::size_type p = s.find('=');
      if(std::string::npos == p)
      {
        res[s] = "";
      }else
      {
        std::string ss;
        t_string nm = s.substr(0, p);
        t_string vl = s.substr(p+1, s.size());
        res[nm] = vl;
      }
    }
    return true;
	}

/*  template<typename t_type>
  bool get_xparam_from_command_line(const std::map<std::string, std::string>& res, const std::basic_string<typename t_string::value_type> & key, t_type& val)
  {

  }
  */

	template<class t_string, typename t_type>
	bool get_xparam_from_command_line(const std::map<t_string, t_string>& res, const t_string & key, t_type& val)
	{
		typename std::map<t_string, t_string>::const_iterator it = res.find(key);
		if(it == res.end())
			return false;

		if(it->second.size())
		{
			return get_xtype_from_string(val, it->second);
		}

		return true;
	}

    template<class t_string, typename t_type>
    t_type get_xparam_from_command_line(const std::map<t_string, t_string>& res, const t_string & key, const t_type& default_value)
    {
        typename std::map<t_string, t_string>::const_iterator it = res.find(key);
        if(it == res.end())
            return default_value;

        if(it->second.size())
        {
            t_type s;
            get_xtype_from_string(s, it->second);
            return s;
        }

        return default_value;
    }

  template<class t_string>
  bool have_in_command_line(const std::map<t_string, t_string>& res, const std::basic_string<typename t_string::value_type>& key)
  {
    typename std::map<t_string, t_string>::const_iterator it = res.find(key);
    if(it == res.end())
      return false;

    return true;
  }

	//----------------------------------------------------------------------------
//#ifdef _WINSOCK2API_
	inline std::string get_ip_string_from_int32(uint32_t ip)
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
	inline bool get_ip_int32_from_string(uint32_t& ip, const std::string& ip_str)
	{
		ip = inet_addr(ip_str.c_str());
		if(INADDR_NONE == ip)
			return false;

		return true;
	}
  //----------------------------------------------------------------------------
  inline bool parse_peer_from_string(uint32_t& ip, uint32_t& port, const std::string& addres)
  {
    //parse ip and address
    std::string::size_type p = addres.find(':');
    if(p == std::string::npos)
    {
      return false;
    }
    std::string ip_str = addres.substr(0, p);
    std::string port_str = addres.substr(p+1, addres.size());

    if(!get_ip_int32_from_string(ip, ip_str))
    {
      return false;
    }

    if(!get_xtype_from_string(port, port_str))
    {
      return false;
    }
    return true;
  }

//#endif
	//----------------------------------------------------------------------------
  template<typename t>
	inline std::string get_t_as_hex_nwidth(const t& v, std::streamsize w = 8)
  {
    std::stringstream ss;
    ss << std::setfill ('0') << std::setw (w) << std::hex << std::noshowbase;
    ss << v;
    return ss.str();
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
	inline bool string_to_num_fast(const std::string& buff, int64_t& val)
	{
		//return get_xtype_from_string(val, buff);
#if (defined _MSC_VER)
    val = _atoi64(buff.c_str());
#else
    val = atoll(buff.c_str());
#endif
		/*
		 * val = atoi64(buff.c_str());
     */
		if(buff != "0" && val == 0)
			return false;
		return true;
	}
	//----------------------------------------------------------------------------
	inline bool string_to_num_fast(const std::string& buff, int& val)
	{
		val = atoi(buff.c_str());
		if(buff != "0" && val == 0)
			return false;

		return true;
	}
	//----------------------------------------------------------------------------
#ifdef WINDOWS_PLATFORM
	inline std::string system_time_to_string(const SYSTEMTIME& st)
	{
	
		/*
		TIME_ZONE_INFORMATION tzi;
		GetTimeZoneInformation(&tzi);
		SystemTimeToTzSpecificLocalTime(&tzi, &stUTC, &stLocal);
		*/

		char szTime[25], szDate[25];
		::GetTimeFormatA(
			LOCALE_USER_DEFAULT,    // locale
			TIME_FORCE24HOURFORMAT, // options
			&st,					// time
			NULL,                   // time format string
			szTime,                 // formatted string buffer
			25                      // size of string buffer
			);

		::GetDateFormatA(
			LOCALE_USER_DEFAULT,    // locale
			NULL,                   // options
			&st,					// date
			NULL,                   // date format
			szDate,                 // formatted string buffer
			25                      // size of buffer
			);
		szTime[24] = szDate[24] = 0; //be happy :)
		
		std::string res = szDate;
		(res += " " )+= szTime;
		return res; 

	}
#endif
	//----------------------------------------------------------------------------
	
	inline bool compare_no_case(const std::string& str1, const std::string& str2)
	{
		
		return !boost::iequals(str1, str2);
	}
	//----------------------------------------------------------------------------
	inline bool compare_no_case(const std::wstring& str1, const std::wstring& str2)
	{
		return !boost::iequals(str1, str2);
	}
	//----------------------------------------------------------------------------
	inline bool is_match_prefix(const std::wstring& str1, const std::wstring& prefix)
	{	
		if(prefix.size()>str1.size())
			return false;

		if(!compare_no_case(str1.substr(0, prefix.size()), prefix))
			return true;
		else
			return false;
	}
	//----------------------------------------------------------------------------
	inline bool is_match_prefix(const std::string& str1, const std::string& prefix)
	{	
		if(prefix.size()>str1.size())
			return false;

		if(!compare_no_case(str1.substr(0, prefix.size()), prefix))
			return true;
		else
			return false;
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
  template<class t_pod_type>
  std::string pod_to_hex(const t_pod_type& s)
  {
    std::string buff;
    buff.assign(reinterpret_cast<const char*>(&s), sizeof(s));
    return buff_to_hex_nodelimer(buff);
  }
  //----------------------------------------------------------------------------
  template<class t_pod_type>
  bool hex_to_pod(const std::string& hex_str, t_pod_type& s)
  {
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
	inline std::string get_filename_from_path(const std::string& str)
	{
		std::string res;
		std::string::size_type pos = str.rfind('\\');
		if(std::string::npos == pos)
			return str;

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
#ifdef _WININET_
	inline std::string get_string_from_systemtime(const SYSTEMTIME& sys_time)
	{
		std::string result_string;

		char buff[100] = {0};
		BOOL res = ::InternetTimeFromSystemTimeA(&sys_time, INTERNET_RFC1123_FORMAT, buff, 99);
		if(!res)
		{
			LOG_ERROR("Failed to load SytemTime to string");
		}

		result_string = buff;
		return result_string;

	}
	//-------------------------------------------------------------------------------------
	inline SYSTEMTIME get_systemtime_from_string(const std::string& buff)
	{
		SYSTEMTIME result_time = {0};

		BOOL res = ::InternetTimeToSystemTimeA(buff.c_str(), &result_time, NULL);
		if(!res)
		{
			LOG_ERROR("Failed to load SytemTime from string " << buff << "interval set to 15 minutes");
		}

		return result_time;
	}
#endif

#ifdef WINDOWS_PLATFORM
	static const DWORD INFO_BUFFER_SIZE = 10000;

	static const wchar_t* get_pc_name()
	{
		static wchar_t	info[INFO_BUFFER_SIZE];
		static DWORD	bufCharCount = INFO_BUFFER_SIZE;
		static bool		init = false;

		if (!init) {
			if (!GetComputerNameW( info, &bufCharCount ))
				info[0] = 0;
			else
				init = true;
		}

		return info;
	}

	static const wchar_t* get_user_name()
	{
		static wchar_t	info[INFO_BUFFER_SIZE];
		static DWORD	bufCharCount = INFO_BUFFER_SIZE;
		static bool		init = false;

		if (!init) {
			if (!GetUserNameW( info, &bufCharCount ))
				info[0] = 0;
			else
				init = true;
		}

		return info;
	}
#endif

#ifdef _LM_
	static const wchar_t* get_domain_name()
	{
		static wchar_t	info[INFO_BUFFER_SIZE];
		static DWORD	bufCharCount = 0;
		static bool		init = false;

		if (!init) {
			LPWSTR domain( NULL );
			NETSETUP_JOIN_STATUS status;
			info[0] = 0;

			if (NET_API_STATUS result = NetGetJoinInformation( NULL, &domain, &status )) 
			{
				LOG_ERROR("get_domain_name error: " << log_space::get_win32_err_descr(result));
			} else
			{
				StringCchCopyW( info, sizeof(info)/sizeof( info[0] ), domain );
				NetApiBufferFree((void*)domain);
				init = true;
			}
		}

		return info;
	}
#endif
#ifdef WINDOWS_PLATFORM
	inline
	std::string load_resource_string_a(int id, const char* pmodule_name = NULL)
	{
		//slow realization 
		HMODULE h = ::GetModuleHandleA( pmodule_name );
		
		char buff[2000] = {0};
		
		::LoadStringA( h, id, buff, sizeof(buff));
		buff[sizeof(buff)-1] = 0; //be happy :)
		return buff;
	}
	inline
	std::wstring load_resource_string_w(int id, const char* pmodule_name = NULL)
	{
		//slow realization 
		HMODULE h = ::GetModuleHandleA( pmodule_name );
		
		wchar_t buff[2000] = {0};
		
		::LoadStringW( h, id, buff, sizeof(buff) / sizeof( buff[0] ) );
		buff[(sizeof(buff)/sizeof(buff[0]))-1] = 0; //be happy :)
		return buff;	
	}
#endif
}
}
#endif //_STRING_TOOLS_H_
