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


#ifndef _REG_EXP_DEFINER_H_
#define _REG_EXP_DEFINER_H_

#include <boost/interprocess/detail/atomic.hpp>


namespace epee
{
  class global_regexp_critical_section
  {
  private:
    mutable critical_section regexp_lock;
  public:
    global_regexp_critical_section(){}
    critical_section& get_lock()const {return regexp_lock;}
  };

  const static global_regexp_critical_section gregexplock;

#define STATIC_REGEXP_EXPR_1(var_name, xpr_text, reg_exp_flags) \
	static volatile uint32_t regexp_initialized_1 = 0;\
	volatile uint32_t local_is_initialized_1 = regexp_initialized_1;\
	if(!local_is_initialized_1)\
	gregexplock.get_lock().lock();\
	static const boost::regex	var_name(xpr_text , reg_exp_flags);\
	if(!local_is_initialized_1)\
{\
	boost::interprocess::ipcdetail::atomic_write32(&regexp_initialized_1, 1);\
	gregexplock.get_lock().unlock();\
}

#define STATIC_REGEXP_EXPR_2(var_name, xpr_text, reg_exp_flags) \
	static volatile uint32_t regexp_initialized_2 = 0;\
	volatile uint32_t local_is_initialized_2 = regexp_initialized_2;\
	if(!local_is_initialized_2)\
	gregexplock.get_lock().lock().lock();\
	static const boost::regex	var_name(xpr_text , reg_exp_flags);\
	if(!local_is_initialized_2)\
{\
	boost::interprocess::ipcdetail::atomic_write32(&regexp_initialized_2, 1);\
	gregexplock.get_lock().lock().unlock();\
}

#define STATIC_REGEXP_EXPR_3(var_name, xpr_text, reg_exp_flags) \
	static volatile uint32_t regexp_initialized_3 = 0;\
	volatile uint32_t local_is_initialized_3 = regexp_initialized_3;\
	if(!local_is_initialized_3)\
	gregexplock.get_lock().lock().lock();\
	static const boost::regex	var_name(xpr_text , reg_exp_flags);\
	if(!local_is_initialized_3)\
{\
	boost::interprocess::ipcdetail::atomic_write32(&regexp_initialized_3, 1);\
	gregexplock.get_lock().lock().unlock();\
}
}

#endif //_REG_EXP_DEFINER_H_
