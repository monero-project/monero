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

#include <set>
#include <list>
#include <vector>
#include <deque>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/contains_fwd.hpp>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "serialization"

namespace epee
{
  namespace
  {
    template<class C> void hint_resize(C &container, size_t size) {}
    template<class C> void hint_resize(std::vector<C> &container, size_t size) { container.reserve(size); }
  }
  namespace serialization
  {

    //-------------------------------------------------------------------------------------------------------------------
    template<class t_type, class t_storage>
    static bool serialize_t_val_as_blob(const t_type& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
    {      
      std::string blob((const char *)&d, sizeof(d));
      return stg.set_value(pname, std::move(blob), hparent_section);
    }
    //-------------------------------------------------------------------------------------------------------------------
    template<class t_type, class t_storage>
    static bool unserialize_t_val_as_blob(t_type& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
    {
      std::string blob;
      if(!stg.get_value(pname, blob, hparent_section))
        return false;
      CHECK_AND_ASSERT_MES(blob.size() == sizeof(d), false, "unserialize_t_val_as_blob: size of " << typeid(t_type).name() << " = " << sizeof(t_type) << ", but stored blod size = " << blob.size() << ", value name = " << pname);
      d = *(const t_type*)blob.data();
      return true;
    } 
    //-------------------------------------------------------------------------------------------------------------------
    template<class serializible_type, class t_storage>
    static bool serialize_t_obj(const serializible_type& obj, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
    {
      typename t_storage::hsection	hchild_section = stg.open_section(pname, hparent_section, true);
      CHECK_AND_ASSERT_MES(hchild_section, false, "serialize_t_obj: failed to open/create section " << pname);
      return obj.store(stg, hchild_section);
    }
    //-------------------------------------------------------------------------------------------------------------------
    template<class serializible_type, class t_storage>
    static bool unserialize_t_obj(serializible_type& obj, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
    {
      typename t_storage::hsection	hchild_section = stg.open_section(pname, hparent_section, false);
      if(!hchild_section) return false;
      return obj._load(stg, hchild_section);
    }
    //-------------------------------------------------------------------------------------------------------------------
    template<class serializible_type, class t_storage>
    static bool serialize_t_obj(enableable<serializible_type>& obj, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
    {
      if(!obj.enabled)
        return true;
      return serialize_t_obj(obj.v, stg, hparent_section, pname);
    }
    //-------------------------------------------------------------------------------------------------------------------
    template<class serializible_type, class t_storage>
    static bool unserialize_t_obj(enableable<serializible_type>& obj, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
    {
      obj.enabled = false;
      typename t_storage::hsection	hchild_section = stg.open_section(pname, hparent_section, false);
      if(!hchild_section) return false;
      obj.enabled = true;
      return obj.v._load(stg, hchild_section);
    }
    //-------------------------------------------------------------------------------------------------------------------
    template<class stl_container, class t_storage>
    static bool serialize_stl_container_t_val  (const stl_container& container, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
    {
      using value_type = typename stl_container::value_type;

      if(!container.size()) return true;
      typename stl_container::const_iterator it = container.begin();
      typename t_storage::harray hval_array = stg.insert_first_value(pname, value_type(*it), hparent_section);
      CHECK_AND_ASSERT_MES(hval_array, false, "failed to insert first value to storage");
      it++;
      for(;it!= container.end();it++)
        stg.insert_next_value(hval_array, value_type(*it));

      return true;
    }
    //--------------------------------------------------------------------------------------------------------------------
    template<class stl_container, class t_storage>
    static bool unserialize_stl_container_t_val(stl_container& container, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
    {
      container.clear();
      typename stl_container::value_type exchange_val;
      typename t_storage::harray hval_array = stg.get_first_value(pname, exchange_val, hparent_section);
      if(!hval_array) return false;
      container.insert(container.end(), std::move(exchange_val));
      while(stg.get_next_value(hval_array, exchange_val))
        container.insert(container.end(), std::move(exchange_val));
      return true;
    }//--------------------------------------------------------------------------------------------------------------------
    template<class stl_container, class t_storage>
    static bool serialize_stl_container_pod_val_as_blob(const stl_container& container, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
    {
      if(!container.size()) return true;
      std::string mb;
      mb.resize(sizeof(typename stl_container::value_type)*container.size());
      typename stl_container::value_type* p_elem = (typename stl_container::value_type*)mb.data();
      BOOST_FOREACH(const typename stl_container::value_type& v, container)
      {
        *p_elem = v;
        p_elem++;
      }
      return stg.set_value(pname, std::move(mb), hparent_section);
    }
    //--------------------------------------------------------------------------------------------------------------------
    template<class stl_container, class t_storage>
    static bool unserialize_stl_container_pod_val_as_blob(stl_container& container, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
    {
      container.clear();
      std::string buff;
      bool res = stg.get_value(pname, buff, hparent_section);
      if(res)
      {
        size_t loaded_size = buff.size();
        typename stl_container::value_type* pelem =  (typename stl_container::value_type*)buff.data();
        CHECK_AND_ASSERT_MES(!(loaded_size%sizeof(typename stl_container::value_type)), 
          false, 
          "size in blob " << loaded_size << " not have not zero modulo for sizeof(value_type) = " << sizeof(typename stl_container::value_type) << ", type " << typeid(typename stl_container::value_type).name());
        size_t count = (loaded_size/sizeof(typename stl_container::value_type));
        hint_resize(container, count);
        for(size_t i = 0; i < count; i++)
          container.insert(container.end(), *(pelem++));
      }
      return res;
    }
    //--------------------------------------------------------------------------------------------------------------------
    template<class stl_container, class t_storage>
    static bool serialize_stl_container_t_obj  (const stl_container& container, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
    {
      bool res = false;
      if(!container.size()) return true;
      typename stl_container::const_iterator it = container.begin();
      typename t_storage::hsection hchild_section = nullptr;
      typename t_storage::harray hsec_array = stg.insert_first_section(pname, hchild_section, hparent_section);
      CHECK_AND_ASSERT_MES(hsec_array && hchild_section, false, "failed to insert first section with section name " << pname);
      res = it->store(stg, hchild_section);
      it++;
      for(;it!= container.end();it++)
      {
        stg.insert_next_section(hsec_array, hchild_section);
        res |= it->store(stg, hchild_section);
      }
      return res;
    }
    //--------------------------------------------------------------------------------------------------------------------
    template<class stl_container, class t_storage>
    static bool unserialize_stl_container_t_obj(stl_container& container, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
    {
      bool res = false;
      container.clear();
      typename stl_container::value_type val = typename stl_container::value_type();
      typename t_storage::hsection hchild_section = nullptr;
      typename t_storage::harray hsec_array = stg.get_first_section(pname, hchild_section, hparent_section);
      if(!hsec_array || !hchild_section) return false;
      res = val._load(stg, hchild_section);
      container.insert(container.end(), val);
      while(stg.get_next_section(hsec_array, hchild_section))
      {
        typename stl_container::value_type val_l = typename stl_container::value_type();
        res |= val_l._load(stg, hchild_section);
        container.insert(container.end(), std::move(val_l));
      }
      return res;
    }
    //--------------------------------------------------------------------------------------------------------------------
    template<bool>
    struct kv_serialization_overloads_impl_is_base_serializable_types;

    template<>
    struct kv_serialization_overloads_impl_is_base_serializable_types<true>
    {
      template<class t_type, class t_storage>
      static bool kv_serialize(const t_type& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
      {
        return stg.set_value(pname, t_type(d), hparent_section);
      }
      //-------------------------------------------------------------------------------------------------------------------
      template<class t_type, class t_storage>
      static bool kv_unserialize(t_type& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
      {
        return stg.get_value(pname, d, hparent_section);
      } 
      //-------------------------------------------------------------------------------------------------------------------
      template<class t_type, class t_storage>
      static bool kv_serialize(const std::vector<t_type>& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
      {
        return serialize_stl_container_t_val(d, stg, hparent_section, pname);
      }
      //-------------------------------------------------------------------------------------------------------------------
      template<class t_type, class t_storage>
      static bool kv_unserialize(std::vector<t_type>& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
      {
        return unserialize_stl_container_t_val(d, stg, hparent_section, pname);
      } 
      //-------------------------------------------------------------------------------------------------------------------
      template<class t_type, class t_storage>
      static bool kv_serialize(const std::deque<t_type>& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
      {
        return serialize_stl_container_t_val(d, stg, hparent_section, pname);
      }
      //-------------------------------------------------------------------------------------------------------------------
      template<class t_type, class t_storage>
      static bool kv_unserialize(std::deque<t_type>& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
      {
        return unserialize_stl_container_t_val(d, stg, hparent_section, pname);
      } 
      //-------------------------------------------------------------------------------------------------------------------
      template<class t_type, class t_storage>
      static bool kv_serialize(const std::list<t_type>& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
      {
        return serialize_stl_container_t_val(d, stg, hparent_section, pname);
      }
      //-------------------------------------------------------------------------------------------------------------------
      template<class t_type, class t_storage>
      static bool kv_unserialize(std::list<t_type>& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
      {
        return unserialize_stl_container_t_val(d, stg, hparent_section, pname);
      } 
      //-------------------------------------------------------------------------------------------------------------------
      template<class t_type, class t_storage>
      static bool kv_serialize(const std::set<t_type>& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
      {
        return serialize_stl_container_t_val(d, stg, hparent_section, pname);
      }
      //-------------------------------------------------------------------------------------------------------------------
      template<class t_type, class t_storage>
      static bool kv_unserialize(std::set<t_type>& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
      {
        return unserialize_stl_container_t_val(d, stg, hparent_section, pname);
      } 
      //-------------------------------------------------------------------------------------------------------------------
    };
    template<>
    struct kv_serialization_overloads_impl_is_base_serializable_types<false>
    {
      template<class t_type, class t_storage>
      static bool kv_serialize(const t_type& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
      {
        return serialize_t_obj(d, stg, hparent_section, pname);
      }
      //-------------------------------------------------------------------------------------------------------------------
      template<class t_type, class t_storage>
      static bool kv_unserialize(t_type& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
      {
        return unserialize_t_obj(d, stg, hparent_section, pname);
      } 

      //-------------------------------------------------------------------------------------------------------------------
      template<class t_type, class t_storage>
      static bool kv_serialize(const std::vector<t_type>& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
      {
        return serialize_stl_container_t_obj(d, stg, hparent_section, pname);
      }
      //-------------------------------------------------------------------------------------------------------------------
      template<class t_type, class t_storage>
      static bool kv_unserialize(std::vector<t_type>& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
      {
        return unserialize_stl_container_t_obj(d, stg, hparent_section, pname);
      } 
      //-------------------------------------------------------------------------------------------------------------------
      template<class t_type, class t_storage>
      static bool kv_serialize(const std::deque<t_type>& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
      {
        return serialize_stl_container_t_obj(d, stg, hparent_section, pname);
      }
      //-------------------------------------------------------------------------------------------------------------------
      template<class t_type, class t_storage>
      static bool kv_unserialize(std::deque<t_type>& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
      {
        return unserialize_stl_container_t_obj(d, stg, hparent_section, pname);
      } 
      //-------------------------------------------------------------------------------------------------------------------
      template<class t_type, class t_storage>
      static bool kv_serialize(const std::list<t_type>& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
      {
        return serialize_stl_container_t_obj(d, stg, hparent_section, pname);
      }
      //-------------------------------------------------------------------------------------------------------------------
      template<class t_type, class t_storage>
      static bool kv_unserialize(std::list<t_type>& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
      {
        return unserialize_stl_container_t_obj(d, stg, hparent_section, pname);
      } 
      //-------------------------------------------------------------------------------------------------------------------
      template<class t_type, class t_storage>
      static bool kv_serialize(const std::set<t_type>& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
      {
        return serialize_stl_container_t_obj(d, stg, hparent_section, pname);
      }
      //-------------------------------------------------------------------------------------------------------------------
      template<class t_type, class t_storage>
      static bool kv_unserialize(std::set<t_type>& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
      {
        return unserialize_stl_container_t_obj(d, stg, hparent_section, pname);
      } 
    };
    template<class t_storage>
    struct base_serializable_types: public boost::mpl::vector<uint64_t, uint32_t, uint16_t, uint8_t, int64_t, int32_t, int16_t, int8_t, double, bool, std::string, typename t_storage::meta_entry>::type
    {};
    //-------------------------------------------------------------------------------------------------------------------
    template<bool> struct selector;
    template<>
    struct selector<true>
    {
      template<class t_type, class t_storage>
      static bool serialize(const t_type& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
      {
        return kv_serialize(d, stg, hparent_section, pname);
      }

      template<class t_type, class t_storage>
      static bool serialize_stl_container_pod_val_as_blob(const t_type& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
      {
        return epee::serialization::serialize_stl_container_pod_val_as_blob(d, stg, hparent_section, pname);
      }

      template<class t_type, class t_storage>
      static bool serialize_t_val_as_blob(const t_type& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
      {
        return epee::serialization::serialize_t_val_as_blob(d, stg, hparent_section, pname);
      }


    };
    template<>
    struct selector<false>
    {
      template<class t_type, class t_storage>
      static bool serialize(t_type& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
      {
        return kv_unserialize(d, stg, hparent_section, pname);
      }
      template<class t_type, class t_storage>
      static bool serialize_stl_container_pod_val_as_blob(t_type& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
      {
        return epee::serialization::unserialize_stl_container_pod_val_as_blob(d, stg, hparent_section, pname);
      }

      template<class t_type, class t_storage>
      static bool serialize_t_val_as_blob(t_type& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
      {
        return epee::serialization::unserialize_t_val_as_blob(d, stg, hparent_section, pname);
      }
    };

    template<class t_type, class t_storage>
    bool kv_serialize(const t_type& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
    {
      return kv_serialization_overloads_impl_is_base_serializable_types<boost::mpl::contains<base_serializable_types<t_storage>, typename std::remove_const<t_type>::type>::value>::kv_serialize(d, stg, hparent_section, pname);
    }
    //-------------------------------------------------------------------------------------------------------------------
    template<class t_type, class t_storage>
    bool kv_unserialize(t_type& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
    {
      return kv_serialization_overloads_impl_is_base_serializable_types<boost::mpl::contains<base_serializable_types<t_storage>, typename std::remove_const<t_type>::type>::value>::kv_unserialize(d, stg, hparent_section, pname);
    } 
    //-------------------------------------------------------------------------------------------------------------------
    template<class t_type, class t_storage>
    bool kv_serialize(const std::vector<t_type>& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
    {
      return kv_serialization_overloads_impl_is_base_serializable_types<boost::mpl::contains<base_serializable_types<t_storage>, typename std::remove_const<t_type>::type>::value>::kv_serialize(d, stg, hparent_section, pname);
    }
    //-------------------------------------------------------------------------------------------------------------------
    template<class t_type, class t_storage>
    bool kv_unserialize(std::vector<t_type>& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
    {
      return kv_serialization_overloads_impl_is_base_serializable_types<boost::mpl::contains<base_serializable_types<t_storage>, typename std::remove_const<t_type>::type>::value>::kv_unserialize(d, stg, hparent_section, pname);
    } 
    //-------------------------------------------------------------------------------------------------------------------
    template<class t_type, class t_storage>
    bool kv_serialize(const std::deque<t_type>& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
    {
      return kv_serialization_overloads_impl_is_base_serializable_types<boost::mpl::contains<base_serializable_types<t_storage>, typename std::remove_const<t_type>::type>::value>::kv_serialize(d, stg, hparent_section, pname);
    }
    //-------------------------------------------------------------------------------------------------------------------
    template<class t_type, class t_storage>
    bool kv_unserialize(std::deque<t_type>& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
    {
      return kv_serialization_overloads_impl_is_base_serializable_types<boost::mpl::contains<base_serializable_types<t_storage>, typename std::remove_const<t_type>::type>::value>::kv_unserialize(d, stg, hparent_section, pname);
    } 
    //-------------------------------------------------------------------------------------------------------------------
    template<class t_type, class t_storage>
    bool kv_serialize(const std::list<t_type>& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
    {
      return kv_serialization_overloads_impl_is_base_serializable_types<boost::mpl::contains<base_serializable_types<t_storage>, typename std::remove_const<t_type>::type>::value>::kv_serialize(d, stg, hparent_section, pname);
    }
    //-------------------------------------------------------------------------------------------------------------------
    template<class t_type, class t_storage>
    bool kv_unserialize(std::list<t_type>& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
    {
      return kv_serialization_overloads_impl_is_base_serializable_types<boost::mpl::contains<base_serializable_types<t_storage>, typename std::remove_const<t_type>::type>::value>::kv_unserialize(d, stg, hparent_section, pname);
    } 
    //-------------------------------------------------------------------------------------------------------------------
    template<class t_type, class t_storage>
    bool kv_serialize(const std::set<t_type>& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
    {
      return kv_serialization_overloads_impl_is_base_serializable_types<boost::mpl::contains<base_serializable_types<t_storage>, typename std::remove_const<t_type>::type>::value>::kv_serialize(d, stg, hparent_section, pname);
    }
    //-------------------------------------------------------------------------------------------------------------------
    template<class t_type, class t_storage>
    bool kv_unserialize(std::set<t_type>& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
    {
      return kv_serialization_overloads_impl_is_base_serializable_types<boost::mpl::contains<base_serializable_types<t_storage>, typename std::remove_const<t_type>::type>::value>::kv_unserialize(d, stg, hparent_section, pname);
    } 
  }
}
