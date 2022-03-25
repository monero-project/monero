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
#include "misc_log_ex.h"
#include "portable_storage_base.h"
#include "portable_storage_bin_utils.h"

#ifdef EPEE_PORTABLE_STORAGE_RECURSION_LIMIT
#define EPEE_PORTABLE_STORAGE_RECURSION_LIMIT_INTERNAL EPEE_PORTABLE_STORAGE_RECURSION_LIMIT
#else 
#define EPEE_PORTABLE_STORAGE_RECURSION_LIMIT_INTERNAL 100
#endif

namespace epee
{
  namespace serialization
  {
    template<typename T>
    struct ps_min_bytes {
      static constexpr const size_t strict = 4096; // actual low bound
    };
    template<> struct ps_min_bytes<uint64_t> { static constexpr const size_t strict = 8; };
    template<> struct ps_min_bytes<int64_t> { static constexpr const size_t strict = 8; };
    template<> struct ps_min_bytes<uint32_t> { static constexpr const size_t strict = 4; };
    template<> struct ps_min_bytes<int32_t> { static constexpr const size_t strict = 4; };
    template<> struct ps_min_bytes<uint16_t> { static constexpr const size_t strict = 2; };
    template<> struct ps_min_bytes<int16_t> { static constexpr const size_t strict = 2; };
    template<> struct ps_min_bytes<uint8_t> { static constexpr const size_t strict = 1; };
    template<> struct ps_min_bytes<int8_t> { static constexpr const size_t strict = 1; };
    template<> struct ps_min_bytes<double> { static constexpr const size_t strict = 8; };
    template<> struct ps_min_bytes<bool> { static constexpr const size_t strict = 1; };
    template<> struct ps_min_bytes<std::string> { static constexpr const size_t strict = 2; };
    template<> struct ps_min_bytes<section> { static constexpr const size_t strict = 1; };
    template<> struct ps_min_bytes<array_entry> { static constexpr const size_t strict = 1; };

    struct throwable_buffer_reader
    {
      throwable_buffer_reader(const void* ptr, size_t sz);
      void read(void* target, size_t count);
      void read_sec_name(std::string& sce_name);
      template<class t_pod_type>
      void read(t_pod_type& pod_val);
      template<class t_type>
      t_type read();
      template<class type_name>
      storage_entry read_ae();
      storage_entry load_storage_array_entry(uint8_t type);
      size_t read_varint();
      template<class t_type>
      storage_entry read_se();
      storage_entry load_storage_entry();
      void read(section& sec);
      void read(std::string& str);
      void read(array_entry &ae);
      template<class t_type>
      size_t min_bytes() const;
      void set_limits(size_t objects, size_t fields, size_t strings);
    private:
      struct recursuion_limitation_guard
      {
        size_t& m_counter_ref;
        recursuion_limitation_guard(size_t& counter):m_counter_ref(counter)
        {
          ++m_counter_ref;
          CHECK_AND_ASSERT_THROW_MES(m_counter_ref < EPEE_PORTABLE_STORAGE_RECURSION_LIMIT_INTERNAL, "Wrong blob data in portable storage: recursion limitation (" << EPEE_PORTABLE_STORAGE_RECURSION_LIMIT_INTERNAL << ") exceeded");
        }
        ~recursuion_limitation_guard() noexcept(false)
        {
          CHECK_AND_ASSERT_THROW_MES(m_counter_ref != 0, "Internal error: m_counter_ref == 0 while ~recursuion_limitation_guard()");
          --m_counter_ref;
        }
      };
#define RECURSION_LIMITATION()  recursuion_limitation_guard rl(m_recursion_count)

      const uint8_t* m_ptr;
      size_t m_count;
      size_t m_recursion_count;
      size_t m_objects;
      size_t m_fields;
      size_t m_strings;

      size_t max_objects;
      size_t max_fields;
      size_t max_strings;
    };

    inline throwable_buffer_reader::throwable_buffer_reader(const void* ptr, size_t sz)
    {
      if(!ptr) 
        throw std::runtime_error("throwable_buffer_reader: ptr==nullptr");
      if(!sz)
        throw std::runtime_error("throwable_buffer_reader: sz==0");
      m_ptr = (uint8_t*)ptr;
      m_count = sz;
      m_recursion_count = 0;
      m_objects = 0;
      m_fields = 0;
      m_strings = 0;
      max_objects = std::numeric_limits<size_t>::max();
      max_fields = std::numeric_limits<size_t>::max();
      max_strings = std::numeric_limits<size_t>::max();
    }
    inline 
    void throwable_buffer_reader::read(void* target, size_t count)
    {
      RECURSION_LIMITATION();
      CHECK_AND_ASSERT_THROW_MES(m_count >= count, " attempt to read " << count << " bytes from buffer with " << m_count << " bytes remained");
      memcpy(target, m_ptr, count);
      m_ptr += count;
      m_count -= count;
    }
    inline 
    void throwable_buffer_reader::read_sec_name(std::string& sce_name)
    {
      RECURSION_LIMITATION();
      uint8_t name_len = 0;
      read(name_len);
      CHECK_AND_ASSERT_THROW_MES(name_len > 0, "Section name is missing");
      sce_name.resize(name_len);
      read((void*)sce_name.data(), name_len);
    }

    template<class t_pod_type>
    void throwable_buffer_reader::read(t_pod_type& pod_val)
    {
      RECURSION_LIMITATION();
      static_assert(std::is_pod<t_pod_type>::value, "POD type expected");
      read(&pod_val, sizeof(pod_val));
      pod_val = CONVERT_POD(pod_val);
    }
    
    template<>
    void throwable_buffer_reader::read<bool>(bool& pod_val)
    {
      RECURSION_LIMITATION();
      static_assert(std::is_pod<bool>::value, "POD type expected");
      static_assert(sizeof(bool) == sizeof(uint8_t), "We really shouldn't use bool directly in serialization code. Replace it with uint8_t if this assert triggers!");
      uint8_t t;
      read(&t, sizeof(t));
      CHECK_AND_ASSERT_THROW_MES(t <= 1, "Invalid bool value " << t);
      pod_val = (t != 0);
    }
    
    template<class t_type>
    t_type throwable_buffer_reader::read()
    {
      RECURSION_LIMITATION();
      t_type v;
      read(v);
      return v;
    }


    template<class type_name>
    storage_entry throwable_buffer_reader::read_ae()
    {
      RECURSION_LIMITATION();
      //for pod types
      array_entry_t<type_name> sa;
      size_t size = read_varint();
      CHECK_AND_ASSERT_THROW_MES(size <= m_count / ps_min_bytes<type_name>::strict, "Size sanity check failed");
      if (std::is_same<type_name, section>())
      {
        CHECK_AND_ASSERT_THROW_MES(size <= max_objects - m_objects, "Too many objects");
        m_objects += size;
      }
      else if (std::is_same<type_name, std::string>())
      {
        CHECK_AND_ASSERT_THROW_MES(size <= max_strings - m_strings, "Too many strings");
        m_strings += size;
      }

      sa.reserve(size);
      //TODO: add some optimization here later
      while(size--)
        sa.m_array.push_back(read<type_name>());
      return storage_entry(array_entry(std::move(sa)));
    }

    inline 
    storage_entry throwable_buffer_reader::load_storage_array_entry(uint8_t type)
    {
      RECURSION_LIMITATION();
      type &= ~SERIALIZE_FLAG_ARRAY;
      switch(type)
      {
      case SERIALIZE_TYPE_INT64:  return read_ae<int64_t>();
      case SERIALIZE_TYPE_INT32:  return read_ae<int32_t>();
      case SERIALIZE_TYPE_INT16:  return read_ae<int16_t>();
      case SERIALIZE_TYPE_INT8:   return read_ae<int8_t>();
      case SERIALIZE_TYPE_UINT64: return read_ae<uint64_t>();
      case SERIALIZE_TYPE_UINT32: return read_ae<uint32_t>();
      case SERIALIZE_TYPE_UINT16: return read_ae<uint16_t>();
      case SERIALIZE_TYPE_UINT8:  return read_ae<uint8_t>();
      case SERIALIZE_TYPE_DOUBLE: return read_ae<double>();
      case SERIALIZE_TYPE_BOOL:   return read_ae<bool>();
      case SERIALIZE_TYPE_STRING: return read_ae<std::string>();
      case SERIALIZE_TYPE_OBJECT: return read_ae<section>();
      case SERIALIZE_TYPE_ARRAY:  return read_ae<array_entry>();
      default: 
        CHECK_AND_ASSERT_THROW_MES(false, "unknown entry_type code = " << type);
      }
    }

    inline 
    size_t throwable_buffer_reader::read_varint()
    {
      RECURSION_LIMITATION();
      CHECK_AND_ASSERT_THROW_MES(m_count >= 1, "empty buff, expected place for varint");
      size_t v = 0;
      uint8_t size_mask = (*(uint8_t*)m_ptr) &PORTABLE_RAW_SIZE_MARK_MASK;
      switch (size_mask)
      {
      case PORTABLE_RAW_SIZE_MARK_BYTE: v = read<uint8_t>();break;
      case PORTABLE_RAW_SIZE_MARK_WORD: v = read<uint16_t>();break;
      case PORTABLE_RAW_SIZE_MARK_DWORD: v = read<uint32_t>();break;
      case PORTABLE_RAW_SIZE_MARK_INT64: v = read<uint64_t>();break;
      default:
        CHECK_AND_ASSERT_THROW_MES(false, "unknown varint size_mask = " << size_mask);
      }
      v >>= 2;
      return v;
    }

    template<class t_type>
    storage_entry throwable_buffer_reader::read_se()
    {
      RECURSION_LIMITATION();
      t_type v;
      read(v);
      return storage_entry(v);
    }

    template<>
    inline storage_entry throwable_buffer_reader::read_se<std::string>()
    {
      RECURSION_LIMITATION();
      CHECK_AND_ASSERT_THROW_MES(m_strings + 1 <= max_strings, "Too many strings");
      m_strings += 1;
      return storage_entry(read<std::string>());
    }


    template<>
    inline storage_entry throwable_buffer_reader::read_se<section>()
    {
      RECURSION_LIMITATION();
      CHECK_AND_ASSERT_THROW_MES(m_objects < max_objects, "Too many objects");
      ++m_objects;
      section s;//use extra variable due to vs bug, line "storage_entry se(section()); " can't be compiled in visual studio
      storage_entry se(std::move(s));
      section& section_entry = boost::get<section>(se);
      read(section_entry);
      return se;
    }

    template<>
    inline storage_entry throwable_buffer_reader::read_se<array_entry>()
    {
      RECURSION_LIMITATION();
      uint8_t ent_type = 0;
      read(ent_type);
      CHECK_AND_ASSERT_THROW_MES(ent_type&SERIALIZE_FLAG_ARRAY, "wrong type sequenses");
      return load_storage_array_entry(ent_type);
    }

    inline 
    storage_entry throwable_buffer_reader::load_storage_entry()
    {
      RECURSION_LIMITATION();
      uint8_t ent_type = 0;
      read(ent_type);
      if(ent_type&SERIALIZE_FLAG_ARRAY)
        return load_storage_array_entry(ent_type);

      switch(ent_type)
      {
      case SERIALIZE_TYPE_INT64:  return read_se<int64_t>();
      case SERIALIZE_TYPE_INT32:  return read_se<int32_t>();
      case SERIALIZE_TYPE_INT16:  return read_se<int16_t>();
      case SERIALIZE_TYPE_INT8:   return read_se<int8_t>();
      case SERIALIZE_TYPE_UINT64: return read_se<uint64_t>();
      case SERIALIZE_TYPE_UINT32: return read_se<uint32_t>();
      case SERIALIZE_TYPE_UINT16: return read_se<uint16_t>();
      case SERIALIZE_TYPE_UINT8:  return read_se<uint8_t>();
      case SERIALIZE_TYPE_DOUBLE: return read_se<double>();
      case SERIALIZE_TYPE_BOOL:   return read_se<bool>();
      case SERIALIZE_TYPE_STRING: return read_se<std::string>();
      case SERIALIZE_TYPE_OBJECT: return read_se<section>();
      case SERIALIZE_TYPE_ARRAY:  return read_se<array_entry>();
      default: 
        CHECK_AND_ASSERT_THROW_MES(false, "unknown entry_type code = " << ent_type);
      }
    }
    inline 
    void throwable_buffer_reader::read(section& sec)
    {
      RECURSION_LIMITATION();
      sec.m_entries.clear();
      size_t count = read_varint();
      CHECK_AND_ASSERT_THROW_MES(count <= max_fields - m_fields, "Too many object fields");
      m_fields += count;
      while(count--)
      {
        //read section name string
        std::string sec_name;
        read_sec_name(sec_name);
        const auto insert_loc = sec.m_entries.lower_bound(sec_name);
        CHECK_AND_ASSERT_THROW_MES(insert_loc == sec.m_entries.end() || insert_loc->first != sec_name, "duplicate key: " << sec_name);
        sec.m_entries.emplace_hint(insert_loc, std::move(sec_name), load_storage_entry());
      }
    }
    inline 
    void throwable_buffer_reader::read(std::string& str)
    {
      RECURSION_LIMITATION();
      size_t len = read_varint();
      CHECK_AND_ASSERT_THROW_MES(len < MAX_STRING_LEN_POSSIBLE, "to big string len value in storage: " << len);
      CHECK_AND_ASSERT_THROW_MES(m_count >= len, "string len count value " << len << " goes out of remain storage len " << m_count);
      //do this manually to avoid double memory write in huge strings (first time at resize, second at read)
      str.assign((const char*)m_ptr, len);
      m_ptr+=len;
      m_count -= len;
    }
    inline
    void throwable_buffer_reader::read(array_entry &ae)
    {
      RECURSION_LIMITATION();
      CHECK_AND_ASSERT_THROW_MES(false, "Reading array entry is not supported");
    }
    inline
    void throwable_buffer_reader::set_limits(size_t objects, size_t fields, size_t strings)
    {
      max_objects = objects;
      max_fields = fields;
      max_strings = strings;
    }
  }
}
