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

#include "net/http_base.h"
#include "memwipe.h"
#include "string_tools.h"

#include <boost/regex.hpp>
#include <string>
#include <utility>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.http"

namespace epee
{
namespace net_utils
{
namespace http
{
    std::string get_value_from_fields_list(const std::string& param_name, const net_utils::http::fields_list& fields)
    {
        fields_list::const_iterator it = fields.begin();
        for(; it != fields.end(); ++it)
            if(!string_tools::compare_no_case(param_name, it->first))
                break;

        if(it==fields.end())
            return std::string();

        return it->second;
    }

    std::string get_value_from_uri_line(const std::string& param_name, const std::string& uri)
    {
        std::string buff = "([\\?|&])";
        buff += param_name + "=([^&]*)";
        boost::regex match_param(buff.c_str(), boost::regex::icase | boost::regex::normal);
        boost::smatch	result;
        if(boost::regex_search(uri, result, match_param, boost::match_default) && result[0].matched) 
        {
            return result[2];
        }
        return std::string();
    }
}
}
}
