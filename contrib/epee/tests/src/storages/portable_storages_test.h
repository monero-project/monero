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

#include <list>
#include <string>
#include "storages/serializeble_struct_helper.h"
#include "serialization/keyvalue_serialization.h"
#include "storages/portable_storage.h"
#include "storages/portable_storage_template_helper.h"

namespace epee
{
  namespace tests
  {

    struct port_test_struct_sub
    {
      std::string m_str;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_VAL(m_str)
      END_KV_SERIALIZE_MAP()
    };

#pragma pack (push, 1)
    struct some_pod_struct
    {
      uint64_t a;
      int32_t b;
    };
#pragma pack(pop)

    struct port_test_struct
    {      
      std::string m_str;
      uint64_t m_uint64;
      uint32_t m_uint32;
      uint16_t m_uint16;
      uint8_t m_uint8;
      int64_t m_int64;
      int32_t m_int32;
      int16_t m_int16;      
      int8_t m_int8;
      double m_double;
      bool m_bool;
      some_pod_struct m_pod;
      std::list<std::string> m_list_of_str;
      std::list<uint64_t> m_list_of_uint64_t;
      std::list<uint32_t> m_list_of_uint32_t;
      std::list<uint16_t> m_list_of_uint16_t;
      std::list<uint8_t> m_list_of_uint8_t;
      std::list<int64_t> m_list_of_int64_t;
      std::list<int32_t> m_list_of_int32_t;
      std::list<int16_t> m_list_of_int16_t;
      std::list<int8_t> m_list_of_int8_t;
      std::list<double> m_list_of_double;
      std::list<bool> m_list_of_bool;
      port_test_struct_sub m_subobj;
      std::list<port_test_struct> m_list_of_self;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_VAL(m_str)
        KV_SERIALIZE_VAL(m_uint64)
        KV_SERIALIZE_VAL(m_uint32)
        KV_SERIALIZE_VAL(m_uint16)
        KV_SERIALIZE_VAL(m_uint8)
        KV_SERIALIZE_VAL(m_int64)
        KV_SERIALIZE_VAL(m_int32)
        KV_SERIALIZE_VAL(m_int16)
        KV_SERIALIZE_VAL(m_int8)
        KV_SERIALIZE_VAL(m_double)
        KV_SERIALIZE_VAL(m_bool)
        KV_SERIALIZE_VAL_POD_AS_BLOB(m_pod)
        KV_SERIALIZE_OBJ(m_subobj)
        KV_SERIALIZE_CONTAINER_VAL(m_list_of_str)
        KV_SERIALIZE_CONTAINER_VAL(m_list_of_uint64_t)
        KV_SERIALIZE_CONTAINER_VAL(m_list_of_uint32_t)
        KV_SERIALIZE_CONTAINER_VAL(m_list_of_uint16_t)
        KV_SERIALIZE_CONTAINER_VAL(m_list_of_uint8_t)
        KV_SERIALIZE_CONTAINER_VAL(m_list_of_int64_t)
        KV_SERIALIZE_CONTAINER_VAL(m_list_of_int32_t)
        KV_SERIALIZE_CONTAINER_VAL(m_list_of_int16_t)
        KV_SERIALIZE_CONTAINER_VAL(m_list_of_int8_t)
        KV_SERIALIZE_CONTAINER_VAL(m_list_of_double)
        KV_SERIALIZE_CONTAINER_VAL(m_list_of_bool)
        KV_SERIALIZE_CONTAINER_OBJ(m_list_of_self)
      END_KV_SERIALIZE_MAP()
    };

    bool operator != (const port_test_struct_sub& a, const port_test_struct_sub& b)
    {
       return b.m_str != a.m_str;
    }

    bool operator == (const port_test_struct& a, const port_test_struct& b)
    {
      if(   b.m_str != a.m_str 
        ||  b.m_uint64 != a.m_uint64
        ||  b.m_uint32 != a.m_uint32
        ||  b.m_uint16 != a.m_uint16
        ||  b.m_uint8 != a.m_uint8
        ||  b.m_int64 != a.m_int64
        ||  b.m_int32 != a.m_int32
        ||  b.m_int16 != a.m_int16
        ||  b.m_int8 != a.m_int8
        ||  b.m_double != a.m_double
        ||  b.m_bool != a.m_bool
        ||  b.m_pod.a != a.m_pod.a 
        ||  b.m_pod.b != a.m_pod.b 
        ||  b.m_list_of_str != a.m_list_of_str
        ||  b.m_list_of_uint64_t != a.m_list_of_uint64_t
        ||  b.m_list_of_uint32_t != a.m_list_of_uint32_t
        ||  b.m_list_of_uint16_t != a.m_list_of_uint16_t
        ||  b.m_list_of_uint8_t != a.m_list_of_uint8_t
        ||  b.m_list_of_int64_t != a.m_list_of_int64_t
        ||  b.m_list_of_int32_t != a.m_list_of_int32_t
        ||  b.m_list_of_int16_t != a.m_list_of_int16_t
        ||  b.m_list_of_int8_t != a.m_list_of_int8_t
        ||  b.m_list_of_double != a.m_list_of_double
        ||  b.m_list_of_bool != a.m_list_of_bool
        ||  b.m_subobj != a.m_subobj
        ||  b.m_list_of_self != a.m_list_of_self
        )
        return false;
      return true;
    }

    void fill_struct_with_test_values(port_test_struct& s)
    {
      s.m_str = "zuzuzuzuzuz";
      s.m_uint64 = 111111111111111;
      s.m_uint32 = 2222222;
      s.m_uint16 = 2222;
      s.m_uint8 = 22;
      s.m_int64 = -111111111111111;
      s.m_int32 = -2222222;
      s.m_int16 = -2222;
      s.m_int8 = -24;
      s.m_double = 0.11111;
      s.m_bool = true;
      s.m_pod.a = 32342342342342;
      s.m_pod.b = -342342;
      s.m_list_of_str.push_back("1112121");
      s.m_list_of_uint64_t.push_back(1111111111);
      s.m_list_of_uint64_t.push_back(2222222222);
      s.m_list_of_uint32_t.push_back(1111111);
      s.m_list_of_uint32_t.push_back(2222222);
      s.m_list_of_uint16_t.push_back(1111);
      s.m_list_of_uint16_t.push_back(2222);
      s.m_list_of_uint8_t.push_back(11);
      s.m_list_of_uint8_t.push_back(22);


      s.m_list_of_int64_t.push_back(-1111111111);
      s.m_list_of_int64_t.push_back(-222222222);
      s.m_list_of_int32_t.push_back(-1111111);
      s.m_list_of_int32_t.push_back(-2222222);
      s.m_list_of_int16_t.push_back(-1111);
      s.m_list_of_int16_t.push_back(-2222);
      s.m_list_of_int8_t.push_back(-11);
      s.m_list_of_int8_t.push_back(-22);

      s.m_list_of_double.push_back(0.11111);
      s.m_list_of_double.push_back(0.22222);
      s.m_list_of_bool.push_back(true);
      s.m_list_of_bool.push_back(false);

      s.m_subobj.m_str = "subszzzzzzzz";
      s.m_list_of_self.push_back(s);
    }

    bool test_portable_storages(const std::string& tests_folder)
    {
      serialization::portable_storage ps, ps2;
      port_test_struct s1, s2;
      fill_struct_with_test_values(s1);

      s1.store(ps);
      std::string binbuf;
      bool r = ps.store_to_binary(binbuf);

      ps2.load_from_binary(binbuf);
      s2.load(ps2);
      if(!(s1 == s2))
      {
        LOG_ERROR("Portable storage  test failed!");
        return false;
      }


      port_test_struct ss1, ss2;
      fill_struct_with_test_values(ss1);
      std::string json_buff = epee::serialization::store_t_to_json(ss1);
      epee::serialization::load_t_from_json(ss2, json_buff);
      if(!(ss1 == ss2))
      {
        LOG_ERROR("Portable storage  test failed!");
        return false;
      }

      return true;
    }

  }
}
