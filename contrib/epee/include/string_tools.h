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

#include "hex.h"
#include "mlocker.h"

#include <boost/utility/string_ref.hpp>
#include <sstream>
#include <string>
#include <cstdint>

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
  inline std::string buff_to_hex_nodelimer(const std::string& src)
  {
    return to_hex::string(to_byte_span(to_span(src)));
  }
  //----------------------------------------------------------------------------
  inline bool parse_hexstr_to_binbuff(const boost::string_ref s, std::string& res)
  {
    return from_hex::to_string(res, s);
  }
  
  std::string get_ip_string_from_int32(uint32_t ip);
  bool get_ip_int32_from_string(uint32_t& ip, const std::string& ip_str);
  bool parse_peer_from_string(uint32_t& ip, uint16_t& port, const std::string& addres);
  std::string num_to_string_fast(int64_t val);
	
  bool compare_no_case(const std::string& str1, const std::string& str2);
  std::string& get_current_module_name();
  std::string& get_current_module_folder();
#ifdef _WIN32
   std::string get_current_module_path();
#endif
  bool set_module_name_and_folder(const std::string& path_to_process_);
  bool trim_left(std::string& str);
  bool trim_right(std::string& str);
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
  std::string pad_string(std::string s, size_t n, char c = ' ', bool prepend = false);
  
  //----------------------------------------------------------------------------
  template<class t_pod_type>
  std::string pod_to_hex(const t_pod_type& s)
  {
    static_assert(std::is_standard_layout<t_pod_type>(), "expected standard layout type");
    return to_hex::string(as_byte_span(s));
  }
  //----------------------------------------------------------------------------
  template<class t_pod_type>
  bool hex_to_pod(const boost::string_ref hex_str, t_pod_type& s)
  {
    static_assert(std::is_standard_layout<t_pod_type>(), "expected standard layout type");
    return from_hex::to_buffer(as_mut_byte_span(s), hex_str);
  }
  //----------------------------------------------------------------------------
  template<class t_pod_type>
  bool hex_to_pod(const boost::string_ref hex_str, tools::scrubbed<t_pod_type>& s)
  {
    return hex_to_pod(hex_str, unwrap(s));
  }
  //----------------------------------------------------------------------------
  template<class t_pod_type>
  bool hex_to_pod(const boost::string_ref hex_str, epee::mlocked<t_pod_type>& s)
  {
    return hex_to_pod(hex_str, unwrap(s));
  }
  	//----------------------------------------------------------------------------
	template<typename T>
	inline std::string to_string_hex(const T &val)
	{
		static_assert(std::is_arithmetic<T>::value, "only arithmetic types");
		std::stringstream ss;
		ss << std::hex << val;
		std::string s;
		ss >> s;
		return s;
	}
  
  bool validate_hex(uint64_t length, const std::string& str);
  std::string get_extension(const std::string& str);
  std::string cut_off_extension(const std::string& str);
  
#ifdef _WIN32
   std::wstring utf8_to_utf16(const std::string& str);
   std::string utf16_to_utf8(const std::wstring& wstr);
#endif
}
}
#endif //_STRING_TOOLS_H_
