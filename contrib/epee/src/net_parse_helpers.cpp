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

#include "net/net_parse_helpers.h"
#include "net/http_base.h"
#include "misc_log_ex.h"
#include "reg_exp_definer.h"
#include <boost/lexical_cast.hpp>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net"

namespace epee
{
namespace net_utils
{

  static bool parse_uri_query(const std::string& query, std::list<std::pair<std::string, std::string> >& params)
  { 
    enum state
    {
      st_param_name, 
      st_param_val
    };
    state st = st_param_name;
    std::string::const_iterator start_it = query.begin();
    std::pair<std::string, std::string> e;
    for(std::string::const_iterator it = query.begin(); it != query.end(); ++it)
    {
      switch(st)
      {
      case st_param_name:
        if(*it == '=')
        {
          e.first.assign(start_it, it);
          start_it = it;++start_it;
          st = st_param_val;
        }
        break;
      case st_param_val:
        if(*it == '&')
        {
          e.second.assign(start_it, it);
          start_it = it;++start_it;
          params.push_back(e);
          e.first.clear();e.second.clear();
          st = st_param_name;
        }
        break;
      default:
        LOG_ERROR("Unknown state " << (int)st);
        return false;
      }
    }
    if(st == st_param_name)
    {
      if(start_it != query.end())
      {
        e.first.assign(start_it, query.end());
        params.push_back(e);
      }
    }else
    {
      if(start_it != query.end())
        e.second.assign(start_it, query.end());

      if(e.first.size())
        params.push_back(e);
    }
    return true;
  }
  
  bool parse_uri(const std::string uri, http::uri_content& content)
  {

    ///iframe_test.html?api_url=http://api.vk.com/api.php&api_id=3289090&api_settings=1&viewer_id=562964060&viewer_type=0&sid=0aad8d1c5713130f9ca0076f2b7b47e532877424961367d81e7fa92455f069be7e21bc3193cbd0be11895&secret=368ebbc0ef&access_token=668bc03f43981d883f73876ffff4aa8564254b359cc745dfa1b3cde7bdab2e94105d8f6d8250717569c0a7&user_id=0&group_id=0&is_app_user=1&auth_key=d2f7a895ca5ff3fdb2a2a8ae23fe679a&language=0&parent_language=0&ad_info=ElsdCQBaQlxiAQRdFUVUXiN2AVBzBx5pU1BXIgZUJlIEAWcgAUoLQg==&referrer=unknown&lc_name=9834b6a3&hash=
    content.m_query_params.clear();
    STATIC_REGEXP_EXPR_1(rexp_match_uri, "^([^?#]*)(\\?([^#]*))?(#(.*))?", boost::regex::icase | boost::regex::normal);

    boost::smatch result;	
    if(!(boost::regex_search(uri, result, rexp_match_uri, boost::match_default) && result[0].matched))
    {
      LOG_PRINT_L1("[PARSE URI] regex not matched for uri: " << uri);
      content.m_path = uri;
      return true;
    }
    if(result[1].matched)
    {
      content.m_path = result[1];
    }
    if(result[3].matched)
    {
      content.m_query = result[3];
    }
    if(result[5].matched)
    {
      content.m_fragment = result[5];
    }
    if(content.m_query.size())
    {
      parse_uri_query(content.m_query, content.m_query_params);
    }
    return true;
  }

  bool parse_url_ipv6(const std::string url_str, http::url_content& content)
  {
    STATIC_REGEXP_EXPR_1(rexp_match_uri, "^(([^:]*?)://)?(\\[(.*)\\](:(\\d+))?)(.*)?", boost::regex::icase | boost::regex::normal);
    //                                     12            3   4      5 6        7

    content.port = 0;
    boost::smatch result;
    if(!(boost::regex_search(url_str, result, rexp_match_uri, boost::match_default) && result[0].matched))
    {
      LOG_PRINT_L1("[PARSE URI] regex not matched for uri: " << rexp_match_uri);
      //content.m_path = uri;
      return false;
    }
    if(result[2].matched)
    {
      content.schema = result[2];
    }
    if(result[4].matched)
    {
      content.host = result[4];
    }
    else  // if host not matched, matching should be considered failed
    {
      return false;
    }
    if(result[6].matched)
    {
      content.port = boost::lexical_cast<uint64_t>(result[6]);
    }
    if(result[7].matched)
    {
      content.uri = result[7];
      return parse_uri(result[7], content.m_uri_content);
    }

    return true;
  }

  bool parse_url(const std::string url_str, http::url_content& content)
  {

    if (parse_url_ipv6(url_str, content)) return true;

    ///iframe_test.html?api_url=http://api.vk.com/api.php&api_id=3289090&api_settings=1&viewer_id=562964060&viewer_type=0&sid=0aad8d1c5713130f9ca0076f2b7b47e532877424961367d81e7fa92455f069be7e21bc3193cbd0be11895&secret=368ebbc0ef&access_token=668bc03f43981d883f73876ffff4aa8564254b359cc745dfa1b3cde7bdab2e94105d8f6d8250717569c0a7&user_id=0&group_id=0&is_app_user=1&auth_key=d2f7a895ca5ff3fdb2a2a8ae23fe679a&language=0&parent_language=0&ad_info=ElsdCQBaQlxiAQRdFUVUXiN2AVBzBx5pU1BXIgZUJlIEAWcgAUoLQg==&referrer=unknown&lc_name=9834b6a3&hash=
    //STATIC_REGEXP_EXPR_1(rexp_match_uri, "^([^?#]*)(\\?([^#]*))?(#(.*))?", boost::regex::icase | boost::regex::normal);
    STATIC_REGEXP_EXPR_1(rexp_match_uri, "^(([^:]*?)://)?(([^/:]*)(:(\\d+))?)(.*)?", boost::regex::icase | boost::regex::normal);
    //                                     12            34       5 6        7
    content.port = 0;
    boost::smatch result;	
    if(!(boost::regex_search(url_str, result, rexp_match_uri, boost::match_default) && result[0].matched))
    {
      LOG_PRINT_L1("[PARSE URI] regex not matched for uri: " << rexp_match_uri);
      //content.m_path = uri;
      return true;
    }
    if(result[2].matched)
    {
      content.schema = result[2];
    }
    if(result[4].matched)
    {
      content.host = result[4];
    }
    if(result[6].matched)
    {
      content.port = boost::lexical_cast<uint64_t>(result[6]);
    }
    if(result[7].matched)
    {
      content.uri = result[7];
      return parse_uri(result[7], content.m_uri_content);
    }
    
    return true;
  }

}
}
