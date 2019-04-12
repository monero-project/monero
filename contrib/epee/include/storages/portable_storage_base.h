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

#include <boost/variant.hpp>
#include <boost/any.hpp>
#include <string>
#include <vector>
#include <deque>

#define PORTABLE_STORAGE_SIGNATUREA 0x01011101
#define PORTABLE_STORAGE_SIGNATUREB 0x01020101 // bender's nightmare 
#define PORTABLE_STORAGE_FORMAT_VER 1

#define PORTABLE_RAW_SIZE_MARK_MASK   0x03 
#define PORTABLE_RAW_SIZE_MARK_BYTE   0
#define PORTABLE_RAW_SIZE_MARK_WORD   1
#define PORTABLE_RAW_SIZE_MARK_DWORD  2
#define PORTABLE_RAW_SIZE_MARK_INT64  3

#ifndef MAX_STRING_LEN_POSSIBLE       
#define MAX_STRING_LEN_POSSIBLE       2000000000 //do not let string be so big
#endif

//data types 
#define SERIALIZE_TYPE_INT64                1
#define SERIALIZE_TYPE_INT32                2
#define SERIALIZE_TYPE_INT16                3
#define SERIALIZE_TYPE_INT8                 4
#define SERIALIZE_TYPE_UINT64               5
#define SERIALIZE_TYPE_UINT32               6
#define SERIALIZE_TYPE_UINT16               7
#define SERIALIZE_TYPE_UINT8                8
#define SERIALIZE_TYPE_DUOBLE               9
#define SERIALIZE_TYPE_STRING               10
#define SERIALIZE_TYPE_BOOL                 11
#define SERIALIZE_TYPE_OBJECT               12
#define SERIALIZE_TYPE_ARRAY                13

#define SERIALIZE_FLAG_ARRAY              0x80


namespace epee
{
  namespace serialization
  {
    struct section;

    template<typename T> struct entry_container { typedef std::vector<T> type; static void reserve(type &t, size_t n) { t.reserve(n); } };
    template<> struct entry_container<bool> { typedef std::deque<bool> type; static void reserve(type &t, size_t n) {} };

    /************************************************************************/
    /*                                                                      */
    /************************************************************************/
    template<class t_entry_type>
    struct array_entry_t
    {
      array_entry_t():m_it(m_array.end()){}        
      array_entry_t(const array_entry_t& other):m_array(other.m_array), m_it(m_array.end()){}

      const t_entry_type* get_first_val() const 
      {
        m_it = m_array.begin();
        return get_next_val();
      }

      t_entry_type* get_first_val() 
      {
        m_it = m_array.begin();
        return get_next_val();
      }


      const t_entry_type* get_next_val() const 
      {
        if(m_it == m_array.end())
          return nullptr;
        return &(*(m_it++));
      }

      t_entry_type* get_next_val() 
      {
        if(m_it == m_array.end())
          return nullptr;
        return (t_entry_type*)&(*(m_it++));//fuckoff
      }

      t_entry_type& insert_first_val(const t_entry_type& v)
      {
        m_array.clear();
        m_it = m_array.end();
        return insert_next_value(v);
      }

      t_entry_type& insert_next_value(const t_entry_type& v)
      {
        m_array.push_back(v);
        return m_array.back();
      }

      void reserve(size_t n)
      {
        entry_container<t_entry_type>::reserve(m_array, n);
      }

      typename entry_container<t_entry_type>::type m_array;
      mutable typename entry_container<t_entry_type>::type::const_iterator m_it;
    };


    typedef  boost::make_recursive_variant<
      array_entry_t<section>, 
      array_entry_t<uint64_t>, 
      array_entry_t<uint32_t>, 
      array_entry_t<uint16_t>, 
      array_entry_t<uint8_t>, 
      array_entry_t<int64_t>, 
      array_entry_t<int32_t>, 
      array_entry_t<int16_t>, 
      array_entry_t<int8_t>, 
      array_entry_t<double>, 
      array_entry_t<bool>, 
      array_entry_t<std::string>,
      array_entry_t<section>, 
      array_entry_t<boost::recursive_variant_> 
    >::type array_entry;

    typedef boost::variant<uint64_t, uint32_t, uint16_t, uint8_t, int64_t, int32_t, int16_t, int8_t, double, bool, std::string, section, array_entry> storage_entry;

    typedef std::string binarybuffer;//it's ok      

    /************************************************************************/
    /*                                                                      */
    /************************************************************************/
    struct section
    {
      std::map<std::string, storage_entry> m_entries;
    };

    //handle-like aliases
    typedef section*      hsection;  
    typedef array_entry*  harray;
  }
}
