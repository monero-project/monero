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


#ifndef _TINY_INI_H_
#define _TINY_INI_H_

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include "string_tools.h"

namespace epee
{
namespace tiny_ini
{

	inline 
		bool get_param_value(const std::string& param_name, const std::string& ini_entry, std::string& res)
	{
		std::string expr_str = std::string() + "^("+ param_name +") *=(.*?)$";
		const boost::regex match_ini_entry( expr_str, boost::regex::icase | boost::regex::normal); 
		boost::smatch result;	
		if(!boost::regex_search(ini_entry, result, match_ini_entry, boost::match_default))
			return false;
		res = result[2];
		string_tools::trim(res);
		return true;
	}
	inline 
		std::string get_param_value(const std::string& param_name, const std::string& ini_entry)
	{
		std::string buff;
		get_param_value(param_name, ini_entry, buff);
		return buff;
	}

	template<class T>
		bool get_param_value_as_t(const std::string& param_name, const std::string& ini_entry, T& res)
	{
		std::string str_res = get_param_value(param_name, ini_entry);
	
		string_tools::trim(str_res);
		if(!str_res.size())
			return false;
		
		return string_tools::get_xtype_from_string(res, str_res);
	}

}
}

#endif //_TINY_INI_H_
