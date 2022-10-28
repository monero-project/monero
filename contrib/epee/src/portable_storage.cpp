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

#include "byte_slice.h"
#include "byte_stream.h"
#include "misc_log_ex.h"
#include "span.h"
#include "storages/portable_storage.h"
#include "storages/portable_storage_to_json.h"
#include "storages/portable_storage_from_json.h"
#include "storages/portable_storage_to_bin.h"
#include "storages/portable_storage_from_bin.h"

#include <boost/utility/string_ref.hpp>

#include <string>
#include <sstream>

namespace epee
{
namespace serialization
{
  bool portable_storage::store_to_binary(byte_slice& target, const std::size_t initial_buffer_size)
  {
    TRY_ENTRY();
    byte_stream ss;
    ss.reserve(initial_buffer_size);
    store_to_binary(ss);
    target = epee::byte_slice{std::move(ss), false};
    return true;
    CATCH_ENTRY("portable_storage::store_to_binary", false);
  }

  bool portable_storage::store_to_binary(byte_stream& ss)
  {
    TRY_ENTRY();
    storage_block_header sbh{};
    sbh.m_signature_a = SWAP32LE(PORTABLE_STORAGE_SIGNATUREA);
    sbh.m_signature_b = SWAP32LE(PORTABLE_STORAGE_SIGNATUREB);
    sbh.m_ver = PORTABLE_STORAGE_FORMAT_VER;
    ss.write(epee::as_byte_span(sbh));
    pack_entry_to_buff(ss, m_root);
    return true;
    CATCH_ENTRY("portable_storage::store_to_binary", false);
  }

    bool portable_storage::dump_as_json(std::string& buff, size_t indent, bool insert_newlines)
    {
      TRY_ENTRY();
      std::stringstream ss;
      epee::serialization::dump_as_json(ss, m_root, indent, insert_newlines);
      buff = ss.str();
      return true;
      CATCH_ENTRY("portable_storage::dump_as_json", false)
    }

    bool portable_storage::load_from_json(const std::string& source)
    {
      TRY_ENTRY();
      return json::load_from_json(source, *this);
      CATCH_ENTRY("portable_storage::load_from_json", false)
    }

    bool portable_storage::load_from_binary(const epee::span<const uint8_t> source, const limits_t *limits)
    {
      m_root.m_entries.clear();
      if(source.size() < sizeof(storage_block_header))
      {
        LOG_ERROR("portable_storage: wrong binary format, packet size = " << source.size() << " less than expected sizeof(storage_block_header)=" << sizeof(storage_block_header));
        return false;
      }
      storage_block_header* pbuff = (storage_block_header*)source.data();
      if(pbuff->m_signature_a != SWAP32LE(PORTABLE_STORAGE_SIGNATUREA) ||
        pbuff->m_signature_b != SWAP32LE(PORTABLE_STORAGE_SIGNATUREB)
        )
      {
        LOG_ERROR("portable_storage: wrong binary format - signature mismatch");
        return false;
      }
      if(pbuff->m_ver != PORTABLE_STORAGE_FORMAT_VER)
      {
        LOG_ERROR("portable_storage: wrong binary format - unknown format ver = " << pbuff->m_ver);
        return false;
      }
      TRY_ENTRY();
      throwable_buffer_reader buf_reader(source.data()+sizeof(storage_block_header), source.size()-sizeof(storage_block_header));
      if (limits)
        buf_reader.set_limits(limits->n_objects, limits->n_fields, limits->n_strings);
      buf_reader.read(m_root);
      return true;//TODO:
      CATCH_ENTRY("portable_storage::load_from_binary", false);
    }
    
    hsection portable_storage::open_section(const std::string& section_name,  hsection hparent_section, bool create_if_notexist)
    {
      TRY_ENTRY();
      hparent_section = hparent_section ? hparent_section:&m_root;
      storage_entry* pentry = find_storage_entry(section_name, hparent_section);
      if(!pentry)
      {
        if(!create_if_notexist)
          return nullptr;
        return insert_new_section(section_name, hparent_section);
      }
      CHECK_AND_ASSERT(pentry , nullptr);
      //check that section_entry we find is real "CSSection"
      if(pentry->type() != typeid(section))
      {
        if(create_if_notexist)
          *pentry = storage_entry(section());//replace
        else
          return nullptr;
      }
      return &boost::get<section>(*pentry);
      CATCH_ENTRY("portable_storage::open_section", nullptr);
    }
    
    bool portable_storage::get_value(const std::string& value_name, storage_entry& val, hsection hparent_section)
    {
      //TRY_ENTRY();
      if(!hparent_section) hparent_section = &m_root;
      storage_entry* pentry = find_storage_entry(value_name, hparent_section);
      if(!pentry)
        return false;

      val = *pentry;
      return true;
      //CATCH_ENTRY("portable_storage::template<>get_value", false);
    }
    
    storage_entry* portable_storage::find_storage_entry(const std::string& pentry_name, hsection psection)
    {
      TRY_ENTRY();
      CHECK_AND_ASSERT(psection, nullptr);
      auto it = psection->m_entries.find(pentry_name);
      if(it == psection->m_entries.end())
        return nullptr;

      return &it->second;
      CATCH_ENTRY("portable_storage::find_storage_entry", nullptr);
    }
    
    hsection portable_storage::insert_new_section(const std::string& pentry_name, hsection psection)
    {
      TRY_ENTRY();
      storage_entry* pse = insert_new_entry_get_storage_entry(pentry_name, psection, section());
      if(!pse) return nullptr;
      return &boost::get<section>(*pse);
      CATCH_ENTRY("portable_storage::insert_new_section", nullptr);
    }
    
    harray portable_storage::get_first_section(const std::string& sec_name, hsection& h_child_section, hsection hparent_section)
    {
      TRY_ENTRY();
      if(!hparent_section) hparent_section = &m_root;
      storage_entry* pentry = find_storage_entry(sec_name, hparent_section);
      if(!pentry)
        return nullptr;
      if(pentry->type() != typeid(array_entry))
        return nullptr;
      array_entry& ar_entry = boost::get<array_entry>(*pentry);
      if(ar_entry.type() != typeid(array_entry_t<section>))
        return nullptr;
      array_entry_t<section>& sec_array = boost::get<array_entry_t<section>>(ar_entry);
      section* psec = sec_array.get_first_val();
      if(!psec)
        return nullptr;
      h_child_section = psec;
      return &ar_entry;
      CATCH_ENTRY("portable_storage::get_first_section", nullptr);
    }
    
    bool portable_storage::get_next_section(harray hsec_array, hsection& h_child_section)
    {
      TRY_ENTRY();
      CHECK_AND_ASSERT(hsec_array, false);
      if(hsec_array->type() != typeid(array_entry_t<section>))
        return false;
      array_entry_t<section>& sec_array = boost::get<array_entry_t<section>>(*hsec_array);
      h_child_section = sec_array.get_next_val();
      if(!h_child_section)
        return false;
      return true;
      CATCH_ENTRY("portable_storage::get_next_section", false);
    }
    
    harray portable_storage::insert_first_section(const std::string& sec_name, hsection& hinserted_childsection, hsection hparent_section)
    {
      TRY_ENTRY();
      if(!hparent_section) hparent_section = &m_root;
      storage_entry* pentry = find_storage_entry(sec_name, hparent_section);
      if(!pentry)
      {
        pentry = insert_new_entry_get_storage_entry(sec_name, hparent_section, array_entry(array_entry_t<section>()));
        if(!pentry)
          return nullptr;
      }
      if(pentry->type() != typeid(array_entry))
        *pentry = storage_entry(array_entry(array_entry_t<section>()));

      array_entry& ar_entry = boost::get<array_entry>(*pentry);
      if(ar_entry.type() != typeid(array_entry_t<section>))
        ar_entry = array_entry(array_entry_t<section>());

      array_entry_t<section>& sec_array = boost::get<array_entry_t<section>>(ar_entry);
      hinserted_childsection = &sec_array.insert_first_val(section());
      return &ar_entry;
      CATCH_ENTRY("portable_storage::insert_first_section", nullptr);
    }
    
    bool portable_storage::insert_next_section(harray hsec_array, hsection& hinserted_childsection)
    {
      TRY_ENTRY();
      CHECK_AND_ASSERT(hsec_array, false);
      CHECK_AND_ASSERT_MES(hsec_array->type() == typeid(array_entry_t<section>), 
        false, "unexpected type(not 'section') in insert_next_section, type: " << hsec_array->type().name());

      array_entry_t<section>& sec_array = boost::get<array_entry_t<section>>(*hsec_array);
      hinserted_childsection = &sec_array.insert_next_value(section());
      return true;
      CATCH_ENTRY("portable_storage::insert_next_section", false);
    }
}
}
