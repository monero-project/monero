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

#include <chrono>
#include <cstdio>
#include <ctime>
#include <string>

namespace epee
{
namespace misc_utils
{
	inline bool get_gmt_time(time_t t, struct tm &tm)
	{
#ifdef _WIN32
		return gmtime_s(&tm, &t);
#else
		return gmtime_r(&t, &tm);
#endif
	}

	inline std::string get_internet_time_str(const time_t& time_)
	{
		char tmpbuf[200] = {0};
		struct tm pt;
		get_gmt_time(time_, pt);
		strftime(tmpbuf, 199, "%a, %d %b %Y %H:%M:%S GMT", &pt);
		return tmpbuf;
	}

	inline std::string get_time_interval_string(const time_t& time_)
	{
		time_t tail = time_;
		const int days = static_cast<int>(tail/(60*60*24));
		tail = tail%(60*60*24);
		const int hours = static_cast<int>(tail/(60*60));
		tail = tail%(60*60);
		const int minutes = static_cast<int>(tail/60);
		tail = tail%(60);
		const int seconds = static_cast<int>(tail);

		char tmpbuf[64] = {0};
		snprintf(tmpbuf, sizeof(tmpbuf) - 1, "d%d.h%d.m%d.s%d", days, hours, minutes, seconds);

		return tmpbuf;
	}

	inline uint64_t get_ns_count()
	{
		typedef std::chrono::duration<uint64_t, std::nano> ns_duration;
		const ns_duration ns_since_epoch = std::chrono::steady_clock::now().time_since_epoch();
		return ns_since_epoch.count();
	}

	inline uint64_t get_tick_count()
	{
		return get_ns_count() / 1000000;
	}
}
}
