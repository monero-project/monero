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

#include "misc_language.h"
#include "portable_storage_base.h"
#include "portable_storage_bin_utils.h"
#include "misc_log_ex.h"

namespace epee
{
  namespace serialization
  {

    template<class pack_value, class t_stream>
    size_t pack_varint_t(t_stream& strm, uint8_t type_or, size_t& pv)
    {
      pack_value v = pv << 2;
      v |= type_or;
      v = CONVERT_POD(v);
      strm.write((const char*)&v, sizeof(pack_value));
      return sizeof(pack_value);
    }

    template<class t_stream>
    size_t pack_varint(t_stream& strm, size_t val)
    {   //the first two bits always reserved for size information

      if(val <= 63)
      {//mean enough one byte
        return pack_varint_t<uint8_t>(strm, PORTABLE_RAW_SIZE_MARK_BYTE, val);
      }
      else if(val <= 16383)
      {//mean need word
        return pack_varint_t<uint16_t>(strm, PORTABLE_RAW_SIZE_MARK_WORD, val);
      }else if(val <= 1073741823)
      {//mean need dword
        return pack_varint_t<uint32_t>(strm, PORTABLE_RAW_SIZE_MARK_DWORD, val);
      }else
      {
        // Same as checking val <= 4611686018427387903 except that it's portable for 32-bit size_t
        CHECK_AND_ASSERT_THROW_MES(!(val >> 31 >> 31), "failed to pack varint - too big amount = " << val);
        return pack_varint_t<uint64_t>(strm, PORTABLE_RAW_SIZE_MARK_INT64, val);
      }
    }

    template<class t_stream>
    bool put_string(t_stream& strm, const std::string& v)
    {
      pack_varint(strm, v.size());
      if(v.size())
        strm.write((const char*)v.data(), v.size());        
      return true;
    }

    template<class t_stream>
    struct array_entry_store_visitor: public boost::static_visitor<bool>
    {
      t_stream& m_strm;

      template<class t_pod_type>
      bool pack_pod_array_type(uint8_t contained_type, const array_entry_t<t_pod_type>& arr_pod)
      {
        uint8_t type = contained_type|SERIALIZE_FLAG_ARRAY;
        m_strm.write((const char*)&type, 1);
        pack_varint(m_strm, arr_pod.m_array.size());
        for(t_pod_type x: arr_pod.m_array)
        {
          x = CONVERT_POD(x);
          m_strm.write((const char*)&x, sizeof(t_pod_type));
        }
        return true;
      }

      array_entry_store_visitor(t_stream& strm):m_strm(strm){}
      bool operator()(const array_entry_t<uint64_t>& v){ return pack_pod_array_type(SERIALIZE_TYPE_UINT64, v);}
      bool operator()(const array_entry_t<uint32_t>& v){ return pack_pod_array_type(SERIALIZE_TYPE_UINT32, v);}
      bool operator()(const array_entry_t<uint16_t>& v){ return pack_pod_array_type(SERIALIZE_TYPE_UINT16, v);}
      bool operator()(const array_entry_t<uint8_t>& v) { return pack_pod_array_type(SERIALIZE_TYPE_UINT8, v);}
      bool operator()(const array_entry_t<int64_t>& v) { return pack_pod_array_type(SERIALIZE_TYPE_INT64, v);}
      bool operator()(const array_entry_t<int32_t>& v) { return pack_pod_array_type(SERIALIZE_TYPE_INT32, v);}
      bool operator()(const array_entry_t<int16_t>& v) { return pack_pod_array_type(SERIALIZE_TYPE_INT16, v);}
      bool operator()(const array_entry_t<int8_t>& v)  { return pack_pod_array_type(SERIALIZE_TYPE_INT8, v);}
      bool operator()(const array_entry_t<double>& v)  { return pack_pod_array_type(SERIALIZE_TYPE_DOUBLE, v);}
      bool operator()(const array_entry_t<bool>& v)    { return pack_pod_array_type(SERIALIZE_TYPE_BOOL, v);}
      bool operator()(const array_entry_t<std::string>& arr_str)
      {
        uint8_t type = SERIALIZE_TYPE_STRING|SERIALIZE_FLAG_ARRAY;
        m_strm.write((const char*)&type, 1);
        pack_varint(m_strm, arr_str.m_array.size());
        for(const std::string& s: arr_str.m_array)
          put_string(m_strm, s);
        return true;
      }
      bool operator()(const array_entry_t<section>& arr_sec)    
      {
        uint8_t type = SERIALIZE_TYPE_OBJECT|SERIALIZE_FLAG_ARRAY;
        m_strm.write((const char*)&type, 1);
        pack_varint(m_strm, arr_sec.m_array.size());
        for(const section& s: arr_sec.m_array)
          pack_entry_to_buff(m_strm, s);
        return true;
      }
      bool operator()(const array_entry_t<array_entry>& arra_ar)    
      {
        uint8_t type = SERIALIZE_TYPE_ARRAY|SERIALIZE_FLAG_ARRAY;
        m_strm.write((const char*)&type, 1);
        pack_varint(m_strm, arra_ar.m_array.size());
        for(const array_entry& s: arra_ar.m_array)
          pack_entry_to_buff(m_strm, s);
        return true;
      }
    };

    template<class t_stream>
    struct storage_entry_store_visitor: public boost::static_visitor<bool>
    {
      t_stream& m_strm;
      storage_entry_store_visitor(t_stream& strm):m_strm(strm){}
      template<class pod_type>
      bool pack_pod_type(uint8_t type, const pod_type& v)
      {
        m_strm.write((const char*)&type, 1);
        pod_type v0 = CONVERT_POD(v);
        m_strm.write((const char*)&v0, sizeof(pod_type));
        return true;
      }
      //section, array_entry
      bool operator()(const uint64_t& v){ return pack_pod_type(SERIALIZE_TYPE_UINT64, v);}
      bool operator()(const uint32_t& v){ return pack_pod_type(SERIALIZE_TYPE_UINT32, v);}
      bool operator()(const uint16_t& v){ return pack_pod_type(SERIALIZE_TYPE_UINT16, v);}
      bool operator()(const uint8_t& v) { return pack_pod_type(SERIALIZE_TYPE_UINT8, v);}
      bool operator()(const int64_t& v) { return pack_pod_type(SERIALIZE_TYPE_INT64, v);}
      bool operator()(const int32_t& v) { return pack_pod_type(SERIALIZE_TYPE_INT32, v);}
      bool operator()(const int16_t& v) { return pack_pod_type(SERIALIZE_TYPE_INT16, v);}
      bool operator()(const int8_t& v)  { return pack_pod_type(SERIALIZE_TYPE_INT8, v);}
      bool operator()(const double& v)  { return pack_pod_type(SERIALIZE_TYPE_DOUBLE, v);}
      bool operator()(const bool& v)  { return pack_pod_type(SERIALIZE_TYPE_BOOL, v);}
      bool operator()(const std::string& v)
      {
        uint8_t type = SERIALIZE_TYPE_STRING;
        m_strm.write((const char*)&type, 1);
        put_string(m_strm, v);
        return true;
      }
      bool operator()(const section& v)  
      {
        uint8_t type = SERIALIZE_TYPE_OBJECT;
        m_strm.write((const char*)&type, 1);
        return pack_entry_to_buff(m_strm, v);
      }

      bool operator()(const array_entry& v)  
      {
        //uint8_t type = SERIALIZE_TYPE_ARRAY;
        //m_strm.write((const char*)&type, 1);
        return pack_entry_to_buff(m_strm, v);
      }
    };

    template<class t_stream>
    bool pack_entry_to_buff(t_stream& strm, const array_entry& ae)
    {
      array_entry_store_visitor<t_stream> aesv(strm);
      return boost::apply_visitor(aesv, ae);
    }

    template<class t_stream>
    bool pack_entry_to_buff(t_stream& strm, const storage_entry& se)
    {
      storage_entry_store_visitor<t_stream> sv(strm);
      return boost::apply_visitor(sv, se);
    }

    template<class t_stream>
    bool pack_entry_to_buff(t_stream& strm, const section& sec)
    {
      typedef std::map<std::string, storage_entry>::value_type section_pair;
      pack_varint(strm, sec.m_entries.size());
      for(const section_pair& se: sec.m_entries)
      {
        CHECK_AND_ASSERT_THROW_MES(se.first.size() < std::numeric_limits<uint8_t>::max(), "storage_entry_name is too long: " << se.first.size() << ", val: " << se.first);
        CHECK_AND_ASSERT_THROW_MES(!se.first.empty(), "storage_entry_name is empty");
        uint8_t len = static_cast<uint8_t>(se.first.size());
        strm.write((const char*)&len, sizeof(len));
        strm.write(se.first.data(), size_t(len));
        pack_entry_to_buff(strm, se.second);
      }
      return true;
    }
  }
}
