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

#include "portable_storage_base.h"
#include "portable_storage_val_converters.h"
#include "misc_log_ex.h"
#include "span.h"

#include <boost/mpl/contains.hpp>

namespace epee
{
  class byte_slice;
  class byte_stream;
  namespace serialization
  {
    /************************************************************************/
    /*                                                                      */
    /************************************************************************/
    class portable_storage
    {
    public:
      typedef epee::serialization::hsection hsection;
      typedef epee::serialization::harray  harray;
      typedef storage_entry meta_entry;

      struct limits_t
      {
        size_t n_objects;
        size_t n_fields;
        size_t n_strings; // not counting field names
      };

      portable_storage(){}
      virtual ~portable_storage(){}
      hsection   open_section(const std::string& section_name,  hsection hparent_section, bool create_if_notexist = false);
      template<class t_value>
      bool       get_value(const std::string& value_name, t_value& val, hsection hparent_section);
      bool       get_value(const std::string& value_name, storage_entry& val, hsection hparent_section);
      template<class t_value>
      bool       set_value(const std::string& value_name, t_value&& target, hsection hparent_section);

      //serial access for arrays of values --------------------------------------
      //values
      template<class t_value>
      harray get_first_value(const std::string& value_name, t_value& target, hsection hparent_section);
      template<class t_value>
      bool          get_next_value(harray hval_array, t_value& target);
      template<class t_value>
      harray insert_first_value(const std::string& value_name, t_value&& target, hsection hparent_section);
      template<class t_value>
      bool          insert_next_value(harray hval_array, t_value&& target);
      //sections
      harray get_first_section(const std::string& pSectionName, hsection& h_child_section, hsection hparent_section);
      bool            get_next_section(harray hSecArray, hsection& h_child_section);
      harray insert_first_section(const std::string& pSectionName, hsection& hinserted_childsection, hsection hparent_section);
      bool            insert_next_section(harray hSecArray, hsection& hinserted_childsection);
      //------------------------------------------------------------------------
      //delete entry (section, value or array)
      bool        delete_entry(const std::string& pentry_name, hsection hparent_section = nullptr);

      //-------------------------------------------------------------------------------
      bool		store_to_binary(byte_slice& target, std::size_t initial_buffer_size = 8192);
      bool		store_to_binary(byte_stream& ss);
      bool		load_from_binary(const epee::span<const uint8_t> target, const limits_t *limits = nullptr);
      bool		load_from_binary(const std::string& target, const limits_t *limits = nullptr)
      {
        return load_from_binary(epee::strspan<uint8_t>(target), limits);
      }

      template<class trace_policy>
      bool		  dump_as_xml(std::string& targetObj, const std::string& root_name = "");
      bool		  dump_as_json(std::string& targetObj, size_t indent = 0, bool insert_newlines = true);
      bool		  load_from_json(const std::string& source);

    private:
      section m_root;
      hsection	get_root_section() {return &m_root;}
      storage_entry* find_storage_entry(const std::string& pentry_name, hsection psection);
      template<class entry_type>
      storage_entry* insert_new_entry_get_storage_entry(const std::string& pentry_name, hsection psection, entry_type&& entry);

      hsection    insert_new_section(const std::string& pentry_name, hsection psection);

#pragma pack(push)
#pragma pack(1)
      struct storage_block_header
      {
        uint32_t m_signature_a;
        uint32_t m_signature_b;
        uint8_t  m_ver;
      };
#pragma pack(pop)
    };
    
    template<class trace_policy>
    bool portable_storage::dump_as_xml(std::string& targetObj, const std::string& root_name)
    {
      return false;//TODO: don't think i ever again will use xml - ambiguous and "overtagged" format
    }    

    template<class to_type>
    struct get_value_visitor: boost::static_visitor<void>
    {
      to_type& m_target;
      get_value_visitor(to_type& target):m_target(target){}
      template<class from_type>
      void operator()(const from_type& v){convert_t(v, m_target);}
    };

    template<class t_value>
    bool portable_storage::get_value(const std::string& value_name, t_value& val, hsection hparent_section)
    {
      BOOST_MPL_ASSERT(( boost::mpl::contains<storage_entry::types, t_value> )); 
      //TRY_ENTRY();
      if(!hparent_section) hparent_section = &m_root;
      storage_entry* pentry = find_storage_entry(value_name, hparent_section);
      if(!pentry)
        return false;

      get_value_visitor<t_value> gvv(val);
      boost::apply_visitor(gvv, *pentry);
      return true;
      //CATCH_ENTRY("portable_storage::template<>get_value", false);
    }
    //---------------------------------------------------------------------------------------------------------------
    template<class t_value>
    bool portable_storage::set_value(const std::string& value_name, t_value&& v, hsection hparent_section)
    {
      using t_real_value = typename std::decay<t_value>::type;
      BOOST_MPL_ASSERT(( boost::mpl::contains<boost::mpl::push_front<storage_entry::types, storage_entry>::type, t_real_value> ));
      TRY_ENTRY();
      if(!hparent_section)
        hparent_section = &m_root;
      storage_entry* pentry = find_storage_entry(value_name, hparent_section);
      if(!pentry)
      {
        pentry = insert_new_entry_get_storage_entry(value_name, hparent_section, std::forward<t_value>(v));
        if(!pentry)
          return false;
        return true;
      }
      *pentry = std::forward<t_value>(v);
      return true;
      CATCH_ENTRY("portable_storage::template<>set_value", false);
    }
    //---------------------------------------------------------------------------------------------------------------
    template<class entry_type>
    storage_entry* portable_storage::insert_new_entry_get_storage_entry(const std::string& pentry_name, hsection psection, entry_type&& entry)
    {
      static_assert(std::is_rvalue_reference<entry_type&&>(), "unexpected copy of value");
      TRY_ENTRY();
      CHECK_AND_ASSERT(psection, nullptr);
      CHECK_AND_ASSERT(!pentry_name.empty(), nullptr);
      auto ins_res = psection->m_entries.emplace(pentry_name, std::forward<entry_type>(entry));
      return &ins_res.first->second;
      CATCH_ENTRY("portable_storage::insert_new_entry_get_storage_entry", nullptr);
    }
    //---------------------------------------------------------------------------------------------------------------
    template<class to_type>
    struct get_first_value_visitor: boost::static_visitor<bool>
    {
      to_type& m_target;
      get_first_value_visitor(to_type& target):m_target(target){}
      template<class from_type>
      bool operator()(const array_entry_t<from_type>& a)
      {
        const from_type* pv = a.get_first_val();
        if(!pv)
          return false;
        convert_t(*pv, m_target);
        return true;
      }
    };
    //---------------------------------------------------------------------------------------------------------------
    template<class t_value>
    harray portable_storage::get_first_value(const std::string& value_name, t_value& target, hsection hparent_section)
    {
      BOOST_MPL_ASSERT(( boost::mpl::contains<storage_entry::types, t_value> )); 
      //TRY_ENTRY();
      if(!hparent_section) hparent_section = &m_root;
      storage_entry* pentry = find_storage_entry(value_name, hparent_section);
      if(!pentry)
        return nullptr;
      if(pentry->type() != typeid(array_entry))
        return nullptr;
      array_entry& ar_entry = boost::get<array_entry>(*pentry);
      
      get_first_value_visitor<t_value> gfv(target);
      if(!boost::apply_visitor(gfv, ar_entry))
        return nullptr;
      return &ar_entry;
      //CATCH_ENTRY("portable_storage::get_first_value", nullptr);
    }
    //---------------------------------------------------------------------------------------------------------------
    template<class to_type>
    struct get_next_value_visitor: boost::static_visitor<bool>
    {
      to_type& m_target;
      get_next_value_visitor(to_type& target):m_target(target){}
      template<class from_type>
      bool operator()(const array_entry_t<from_type>& a)
      {
        //TODO: optimize code here: work without get_next_val function
        const from_type* pv = a.get_next_val();
        if(!pv)
          return false;
        convert_t(*pv, m_target);
        return true;
      }
    };

    template<class t_value>
    bool portable_storage::get_next_value(harray hval_array, t_value& target)
    {
      BOOST_MPL_ASSERT(( boost::mpl::contains<storage_entry::types, t_value> )); 
      //TRY_ENTRY();
      CHECK_AND_ASSERT(hval_array, false);
      array_entry& ar_entry = *hval_array;
      get_next_value_visitor<t_value> gnv(target);
      if(!boost::apply_visitor(gnv, ar_entry))
        return false;
      return true;
      //CATCH_ENTRY("portable_storage::get_next_value", false);
    } 
    //---------------------------------------------------------------------------------------------------------------
    template<class t_value>
    harray portable_storage::insert_first_value(const std::string& value_name, t_value&& target, hsection hparent_section)
    {
      using t_real_value = typename std::decay<t_value>::type;
      static_assert(std::is_rvalue_reference<t_value&&>(), "unexpected copy of value");
      TRY_ENTRY();
      if(!hparent_section) hparent_section = &m_root;
      storage_entry* pentry = find_storage_entry(value_name, hparent_section);
      if(!pentry)
      {
        pentry = insert_new_entry_get_storage_entry(value_name, hparent_section, array_entry(array_entry_t<t_real_value>()));
        if(!pentry)
          return nullptr;
      }
      if(pentry->type() != typeid(array_entry))
        *pentry = storage_entry(array_entry(array_entry_t<t_real_value>()));

      array_entry& arr = boost::get<array_entry>(*pentry);
      if(arr.type() != typeid(array_entry_t<t_real_value>))
        arr = array_entry(array_entry_t<t_real_value>());

      array_entry_t<t_real_value>& arr_typed = boost::get<array_entry_t<t_real_value> >(arr);
      arr_typed.insert_first_val(std::forward<t_value>(target));
      return &arr;
      CATCH_ENTRY("portable_storage::insert_first_value", nullptr);
    }
    //---------------------------------------------------------------------------------------------------------------
    template<class t_value>
    bool portable_storage::insert_next_value(harray hval_array, t_value&& target)
    {
      using t_real_value = typename std::decay<t_value>::type;
      static_assert(std::is_rvalue_reference<t_value&&>(), "unexpected copy of value");
      TRY_ENTRY();
      CHECK_AND_ASSERT(hval_array, false);

      CHECK_AND_ASSERT_MES(hval_array->type() == typeid(array_entry_t<t_real_value>),
        false, "unexpected type in insert_next_value: " << typeid(array_entry_t<t_real_value>).name());

      array_entry_t<t_real_value>& arr_typed = boost::get<array_entry_t<t_real_value> >(*hval_array);
      arr_typed.insert_next_value(std::forward<t_value>(target));
      return true;
      CATCH_ENTRY("portable_storage::insert_next_value", false);
    }
  }
}
