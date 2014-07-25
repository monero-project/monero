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
#include "storages/portable_storage.h"

namespace epee
{
  namespace tests
  {


    struct test_struct
    {      

      std::string m_str;
      unsigned int m_uint;
      bool m_bool;
      std::list<std::string> m_list_of_str;
      std::list<int> m_list_of_int;
      std::list<test_struct> m_list_of_self;


      BEGIN_NAMED_SERIALIZE_MAP()
        SERIALIZE_STL_ANSI_STRING(m_str)
        SERIALIZE_POD(m_uint)
        SERIALIZE_POD(m_bool)
        SERIALIZE_STL_CONTAINER_ANSII_STRING(m_list_of_str)
        SERIALIZE_STL_CONTAINER_POD(m_list_of_int)
        SERIALIZE_STL_CONTAINER_T(m_list_of_self)
      END_NAMED_SERIALIZE_MAP()

    };


    bool operator == (const test_struct& a, const test_struct& b)
    {
      if(   b.m_str != a.m_str 
        ||  b.m_uint != a.m_uint 
        ||  b.m_bool != a.m_bool 
        ||  b.m_list_of_str != a.m_list_of_str 
        ||  b.m_list_of_int != a.m_list_of_int 
        ||  b.m_list_of_self != a.m_list_of_self 
        )
        return false;
      return true;
    }

    inline test_struct get_test_struct()
    {
      test_struct t = boost::value_initialized<test_struct>();
      t.m_bool = true;
      t.m_str = "ackamdc'kmecemcececmacmecmcm[aicm[oeicm[oeicm[qaicm[qoe";
      t.m_uint = 233242;
      for(int i = 0; i!=500; i++)
        t.m_list_of_int.push_back(i);

      for(int i = 0; i!=500; i++)
        t.m_list_of_str.push_back("ssccd");

      for(int i = 0; i!=5; i++)
      {
        t.m_list_of_self.push_back(t);
      }
      return t; 
    }

    bool test_storages(const std::string& tests_folder)
    {
      
      epee::serialization::portable_storage  ps;
      auto s = ps.open_section("zzz", nullptr);
      uint64_t i = 0;
      ps.get_value("afdsdf", i, s);
      
      
      LOG_PRINT_L0("Generating test struct...");
      boost::filesystem::path storage_folder = tests_folder;
      storage_folder /= "storages";


      test_struct t = get_test_struct();

      LOG_PRINT_L0("Loading test struct from storage...");
      test_struct t2;
      bool res = epee::StorageNamed::load_struct_from_storage_file(t2, (storage_folder /+ "valid_storage.bin").string());
      CHECK_AND_ASSERT_MES(res, false, "Failed to load valid_storage.bin");

      LOG_PRINT_L0("Comparing generated and loaded test struct...");
      if(!(t == t2))
        return false;

      LOG_PRINT_L0("Loading broken archive 1...");
      test_struct t3;
      res = epee::StorageNamed::load_struct_from_storage_file(t3, (storage_folder /+ "invalid_storage_1.bin").string());
      CHECK_AND_ASSERT_MES(!res, false, "invalid_storage_1.bin loaded, but should not ");


      LOG_PRINT_L0("Loading broken archive 2...");
      res = epee::StorageNamed::load_struct_from_storage_file(t3, (storage_folder /+ "invalid_storage_2.bin").string());
      CHECK_AND_ASSERT_MES(!res, false, "invalid_storage_2.bin loaded, but should not ");

      LOG_PRINT_L0("Loading broken archive 3...");
      res = epee::StorageNamed::load_struct_from_storage_file(t3, (storage_folder /+ "invalid_storage_3.bin").string());
      CHECK_AND_ASSERT_MES(!res, false, "invalid_storage_3.bin loaded, but should not ");

      LOG_PRINT_L0("Loading broken archive 4...");
      res = epee::StorageNamed::load_struct_from_storage_file(t3, (storage_folder /+ "invalid_storage_4.bin").string());
      CHECK_AND_ASSERT_MES(!res, false, "invalid_storage_3.bin loaded, but should not ");

      return true;
    }
  }
}

