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

#include <cstdint> // uint64_t

#ifdef _WIN32
#include <sysinfoapi.h> // GetSystemTimeAsFileTime
#else
#include <sys/time.h> // gettimeofday
#endif

namespace epee
{
namespace math_helper
{
	template<typename get_interval, bool start_immediate = true>
	class once_a_time
	{
    uint64_t get_time() const
    {
#ifdef _WIN32
      FILETIME fileTime;
      GetSystemTimeAsFileTime(&fileTime);
      unsigned __int64 present = 0;
      present |= fileTime.dwHighDateTime;
      present = present << 32;
      present |= fileTime.dwLowDateTime;
      present /= 10;  // mic-sec
      return present;
#else
      struct timeval tv;
      gettimeofday(&tv, NULL);
      return tv.tv_sec * 1000000 + tv.tv_usec;
#endif
    }

    void set_next_interval()
    {
      m_interval = get_interval()();
    }

	public:
		once_a_time()
		{
			m_last_worked_time = 0;
      if(!start_immediate)
        m_last_worked_time = get_time();
      set_next_interval();
		}

    void trigger()
    {
      m_last_worked_time = 0;
    }

		template<class functor_t>
		bool do_call(functor_t functr)
		{
			uint64_t current_time = get_time();

      if(current_time - m_last_worked_time > m_interval)
			{
				bool res = functr();
				m_last_worked_time = get_time();
        set_next_interval();
				return res;
			}
			return true;
		}

	private:
		uint64_t m_last_worked_time;
		uint64_t m_interval;
	};

  template<uint64_t N> struct get_constant_interval { public: uint64_t operator()() const { return N; } };

  template<int default_interval, bool start_immediate = true>
  class once_a_time_seconds: public once_a_time<get_constant_interval<default_interval * (uint64_t)1000000>, start_immediate> {};
  template<int default_interval, bool start_immediate = true>
  class once_a_time_milliseconds: public once_a_time<get_constant_interval<default_interval * (uint64_t)1000>, start_immediate> {};
  template<typename get_interval, bool start_immediate = true>
  class once_a_time_seconds_range: public once_a_time<get_interval, start_immediate> {};
}
}
