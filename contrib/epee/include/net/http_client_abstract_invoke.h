
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
#include "storages/serializeble_struct_helper.h"

namespace epee
{
  namespace net_utils
  {
    namespace http
    {
      template<class TArg, class TResult, class TTransport>
      bool invoke_http_json_remote_command(const std::string& url, TArg& out_struct, TResult& result_struct, TTransport& transport, unsigned int timeout = 5000, const std::string& method = "GET")
      {
        std::string req_param;
        StorageNamed::InMemStorageSpace::json::store_t_to_json(out_struct, req_param);

        const http_response_info* pri = NULL;
        if(!invoke_request(url, transport, timeout, &pri, method, req_param))
        {
          LOG_PRINT_L1("Failed to invoke http request to  " << url);
          return false;
        }

        if(!pri->m_response_code)
        {
          LOG_PRINT_L1("Failed to invoke http request to  " << url << ", internal error (null response ptr)");
          return false;
        }

        if(pri->m_response_code != 200)
        {
          LOG_PRINT_L1("Failed to invoke http request to  " << url << ", wrong response code: " << pri->m_response_code);
          return false;
        }

        return StorageNamed::InMemStorageSpace::json::load_t_from_json(result_struct, pri->m_body);
      }



      template<class TArg, class TResult, class TTransport>
      bool invoke_http_bin_remote_command(const std::string& url, TArg& out_struct, TResult& result_struct, TTransport& transport, unsigned int timeout = 5000, const std::string& method = "GET")
      {
        std::string req_param;
        epee::StorageNamed::save_struct_as_storage_to_buff(out_struct, req_param); 

        const http_response_info* pri = NULL;
        if(!invoke_request(url, transport, timeout, &pri, method, req_param))
        {
          LOG_PRINT_L1("Failed to invoke http request to  " << url);
          return false;
        }

        if(!pri->m_response_code)
        {
          LOG_PRINT_L1("Failed to invoke http request to  " << url << ", internal error (null response ptr)");
          return false;
        }

        if(pri->m_response_code != 200)
        {
          LOG_PRINT_L1("Failed to invoke http request to  " << url << ", wrong response code: " << pri->m_response_code);
          return false;
        }

        return epee::StorageNamed::load_struct_from_storage_buff(result_struct, pri->m_body); 
      }


    }
  }
}
