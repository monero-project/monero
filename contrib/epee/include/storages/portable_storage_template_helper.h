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

#include <string>

#include "byte_slice.h"
#include "byte_stream.h"
#include "file_io_utils.h"
#include "serialization/wire/epee/base.h"
#include "serialization/wire/json/base.h"
#include "span.h"

namespace epee
{
  class byte_stream;

  namespace serialization
  {
    //-----------------------------------------------------------------------------------------------------------
    template<class t_struct>
    bool load_t_from_json(t_struct& out, const std::string& json_buff)
    {
      return !wire::json::from_bytes(to_span(json_buff), out);
    }
    //-----------------------------------------------------------------------------------------------------------
    template<class t_struct>
    bool load_t_from_json_file(t_struct& out, const std::string& json_file)
    {
      std::string f_buff;
      if(!file_io_utils::load_file_to_string(json_file, f_buff))
        return false;

      return load_t_from_json(out, f_buff);
    }
    //-----------------------------------------------------------------------------------------------------------
    template<class t_struct>
    bool store_t_to_json(t_struct& str_in, std::string& json_buff)
    {
      json_buff.clear();
      return !wire::json::to_bytes(json_buff, str_in);
    }
    //-----------------------------------------------------------------------------------------------------------
    template<class t_struct>
    std::string store_t_to_json(t_struct& str_in)
    {
      std::string json_buff;
      store_t_to_json(str_in, json_buff); //, indent, insert_newlines);
      return json_buff;
    }
    //-----------------------------------------------------------------------------------------------------------
    template<class t_struct>
    bool store_t_to_json_file(t_struct& str_in, const std::string& fpath)
    {
      std::string json_buff;
      store_t_to_json(str_in, json_buff);
      return file_io_utils::save_string_to_file(fpath, json_buff);
    }
    //-----------------------------------------------------------------------------------------------------------
    template<class t_struct>
    bool load_t_from_binary(t_struct& out, const epee::span<const uint8_t> binary_buff)
    {
      return !wire::epee_bin::from_bytes(binary_buff, out);
    }
    //-----------------------------------------------------------------------------------------------------------
    template<class t_struct>
    bool load_t_from_binary(t_struct& out, const std::string& binary_buff)
    {
      return load_t_from_binary(out, epee::strspan<uint8_t>(binary_buff));
    }
    //-----------------------------------------------------------------------------------------------------------
    template<class t_struct>
    bool load_t_from_binary_file(t_struct& out, const std::string& binary_file)
    {
      std::string f_buff;
      if(!file_io_utils::load_file_to_string(binary_file, f_buff))
        return false;

      return load_t_from_binary(out, f_buff);
    }
    //-----------------------------------------------------------------------------------------------------------
    template<class t_struct>
    bool store_t_to_binary(t_struct& str_in, byte_slice& binary_buff, size_t initial_buffer_size = 8192)
    {
      binary_buff = nullptr;

      byte_stream buf{};
      buf.reserve(initial_buffer_size);
      if (wire::epee_bin::to_bytes(buf, str_in))
        return false;
      binary_buff = byte_slice{std::move(buf)};
      return true;
    }
    //-----------------------------------------------------------------------------------------------------------
    template<class t_struct>
    byte_slice store_t_to_binary(t_struct& str_in, size_t initial_buffer_size = 8192)
    {
      byte_slice binary_buff;
      store_t_to_binary(str_in, binary_buff, initial_buffer_size);
      return binary_buff;
    }
    //-----------------------------------------------------------------------------------------------------------
    template<class t_struct>
    bool store_t_to_binary(t_struct& str_in, byte_stream& binary_buff)
    {
      return !wire::epee_bin::to_bytes(binary_buff, str_in);
    }

  }
}
