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

#include <boost/utility/value_init.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>
namespace epee
{
#define AUTO_VAL_INIT(v)   boost::value_initialized<decltype(v)>()

namespace misc_utils
{
  bool sleep_no_w(long ms);

  template <typename T>
  T get_mid(const T &a, const T &b)
  {
    //returns the average of two numbers; overflow safe and works with at least all integral and floating point types
    //(a+b)/2 = (a/2) + (b/2) + ((a - 2*(a/2)) + (b - 2*(b/2)))/2
    return (a/2) + (b/2) + ((a - 2*(a/2)) + (b - 2*(b/2)))/2;
  }

  template<class type_vec_type>
  type_vec_type median(std::vector<type_vec_type> &v)
  {
    if(v.empty())
      return boost::value_initialized<type_vec_type>();
    if(v.size() == 1)
      return v[0];

    size_t n = (v.size()) / 2;
    std::sort(v.begin(), v.end());
    //nth_element(v.begin(), v.begin()+n-1, v.end());
    if(v.size()%2)
    {//1, 3, 5...
      return v[n];
    }else 
    {//2, 4, 6...
      return get_mid<type_vec_type>(v[n-1],v[n]);
    }

  }

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/

  struct call_befor_die_base
  {
    virtual ~call_befor_die_base(){}
  };

  typedef boost::shared_ptr<call_befor_die_base> auto_scope_leave_caller;


  template<class t_scope_leave_handler>
  struct call_befor_die: public call_befor_die_base
  {
    t_scope_leave_handler m_func;
    call_befor_die(t_scope_leave_handler f):m_func(f)
    {}
    ~call_befor_die()
    {
      try { m_func(); }
      catch (...) { /* ignore */ }
    }
  };

  template<class t_scope_leave_handler>
  auto_scope_leave_caller create_scope_leave_handler(t_scope_leave_handler f)
  {
    auto_scope_leave_caller slc(new call_befor_die<t_scope_leave_handler>(f));
    return slc;
  }

  template<typename T> struct struct_init: T
  {
    struct_init(): T{} {}
  };

}
}
