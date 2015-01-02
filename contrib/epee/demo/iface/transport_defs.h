#pragma once

#include "serialization/keyvalue_serialization.h"
#include "storages/portable_storage_base.h"

namespace demo
{

  struct some_test_subdata
  {
    std::string m_str;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(m_str)
    END_KV_SERIALIZE_MAP()
  };

  struct some_test_data
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
    some_test_subdata m_subobj;
    std::list<some_test_data> m_list_of_self;
    epee::serialization::storage_entry m_storage_entry_int;
    epee::serialization::storage_entry m_storage_entry_string;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(m_str)
      KV_SERIALIZE(m_uint64)
      KV_SERIALIZE(m_uint32)
      KV_SERIALIZE(m_uint16)
      KV_SERIALIZE(m_uint8)
      KV_SERIALIZE(m_int64)
      KV_SERIALIZE(m_int32)
      KV_SERIALIZE(m_int16)
      KV_SERIALIZE(m_int8)
      KV_SERIALIZE(m_double)
      KV_SERIALIZE(m_bool)
      KV_SERIALIZE(m_subobj)
      KV_SERIALIZE(m_list_of_str)
      KV_SERIALIZE(m_list_of_uint64_t)
      KV_SERIALIZE(m_list_of_uint32_t)
      KV_SERIALIZE(m_list_of_uint16_t)
      KV_SERIALIZE(m_list_of_uint8_t)
      KV_SERIALIZE(m_list_of_int64_t)
      KV_SERIALIZE(m_list_of_int32_t)
      KV_SERIALIZE(m_list_of_int16_t)
      KV_SERIALIZE(m_list_of_int8_t)
      KV_SERIALIZE(m_list_of_double)
      KV_SERIALIZE(m_list_of_bool)
      KV_SERIALIZE(m_list_of_self)
      KV_SERIALIZE(m_storage_entry_int)
      KV_SERIALIZE(m_storage_entry_string)
    END_KV_SERIALIZE_MAP()
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
	struct COMMAND_EXAMPLE_1
	{
		const static int ID = 1000;

		struct request
		{		
      std::string example_string_data;
      some_test_data sub;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(example_string_data)
        KV_SERIALIZE(sub)
      END_KV_SERIALIZE_MAP()
		};


		struct response
		{
			bool 	 m_success; 
      std::list<some_test_data> subs;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(m_success)
        KV_SERIALIZE(subs)
      END_KV_SERIALIZE_MAP()
		};
	};



	struct COMMAND_EXAMPLE_2
	{
		const static int ID = 1001;

    struct request
    {		
      std::string example_string_data2;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(example_string_data2)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      bool 	 m_success; 


      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(m_success)
      END_KV_SERIALIZE_MAP()
    };
	};


  //-------------------------------------------------------------------------------------
  //-------------------------------------------------------------------------------------
  //in debug purpose
  bool operator != (const some_test_subdata& a, const some_test_subdata& b)
  {
    return b.m_str != a.m_str;
  }

  bool operator == (const some_test_data& a, const some_test_data& b)
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
      ||  b.m_storage_entry_int.which() != a.m_storage_entry_int.which()
      ||  b.m_storage_entry_string.which() != a.m_storage_entry_string.which()
      )
      return false;
    return true;
  }

  inline some_test_data get_test_data()
  {
    some_test_data s;
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
    s.m_storage_entry_int = epee::serialization::storage_entry(uint64_t(22222));;
    s.m_storage_entry_string = epee::serialization::storage_entry(std::string("sdsvsdvs"));
    return s;
  }
}
