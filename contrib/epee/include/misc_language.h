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

#include <limits>
#include <boost/thread.hpp>
#include <boost/utility/value_init.hpp>
namespace epee
{
#define STD_TRY_BEGIN() try {

#define STD_TRY_CATCH(where_, ret_val) \
	} \
	catch (const std::exception  &e) \
	{ \
		LOG_ERROR("EXCEPTION: " << where_  << ", mes: "<< e.what());  \
		return ret_val; \
	} \
	catch (...) \
	{ \
		LOG_ERROR("EXCEPTION: " << where_ ); \
		return ret_val; \
	}



#define AUTO_VAL_INIT(v)   boost::value_initialized<decltype(v)>()

namespace misc_utils
{
	template<typename t_type>
		t_type get_max_t_val(t_type t)
		{
			return (std::numeric_limits<t_type>::max)();
		}

		
	template<typename t_iterator>
		t_iterator move_it_forward(t_iterator it, size_t count)
		{
			while(count--)
				it++;
			return it;
		}

    template<typename t_iterator>
    t_iterator move_it_backward(t_iterator it, size_t count)
    {
      while(count--)
        it--;
      return it;
    }


	// TEMPLATE STRUCT less
	template<class _Ty>
	struct less_as_pod
		: public std::binary_function<_Ty, _Ty, bool>
	{	// functor for operator<
		bool operator()(const _Ty& _Left, const _Ty& _Right) const
		{	// apply operator< to operands
			return memcmp(&_Left, &_Right, sizeof(_Left)) < 0;
		}
	};

  template<class _Ty>
  bool is_less_as_pod(const _Ty& _Left, const _Ty& _Right)
  {	// apply operator< to operands
      return memcmp(&_Left, &_Right, sizeof(_Left)) < 0;
  }
	

	inline
	bool sleep_no_w(long ms )
	{
		boost::this_thread::sleep( 
			boost::get_system_time() + 
			boost::posix_time::milliseconds( std::max<long>(ms,0) ) );
		
		return true;
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
      return (v[n-1] + v[n])/2;
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
