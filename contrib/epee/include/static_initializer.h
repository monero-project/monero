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



#ifndef _STATIC_INITIALIZER_H_
#define _STATIC_INITIALIZER_H_


namespace epee
{
/***********************************************************************
class initializer - useful to initialize some static classes 
                       which have init() and un_init() static members
************************************************************************/

template<class to_initialize>
class initializer
{
public:
	initializer()
	{
		to_initialize::init();
		//get_set_is_initialized(true, true);
	}
	~initializer()
	{
		to_initialize::un_init();
		//get_set_is_uninitialized(true, true);
	}

	/*static inline bool is_initialized()
	{
		return get_set_is_initialized();
	}
	static inline bool is_uninitialized()
	{
		return get_set_is_uninitialized();
	}

private: 
	static inline bool get_set_is_initialized(bool need_to_set = false, bool val_to_set= false)
	{
		static bool val_is_initialized = false;
		if(need_to_set)
			val_is_initialized = val_to_set;
		return val_is_initialized;
	}
	static inline bool get_set_is_uninitialized(bool need_to_set = false, bool val_to_set = false)
	{
		static bool val_is_uninitialized = false;
		if(need_to_set)
			val_is_uninitialized = val_to_set;
		return val_is_uninitialized;
	}*/
};

}
#endif //_STATIC_INITIALIZER_H_
