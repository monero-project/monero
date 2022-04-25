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

/**
 * \file
 * This header contains time-related helper functions, including:
 * string formatting, steady clock wrappers, and an interval executor.
 */

#pragma once

#include <chrono>
#include <cstdio>
#include <ctime>
#include <string>

namespace epee
{
namespace misc_utils
{
	/**
	 * @brief An OS-independent wrapper for converting time_t -> struct tm
	 *
	 * This function is thread-safe as long as access to t and tm is synchronized.
	 *
	 * @param t The number of seconds since 1970 Epoch
	 * @param tm A mutable reference to a tm struct where the result will be placed
	 *
	 * @return true on success, false on failure
	 */
	inline bool get_gmt_time(time_t t, struct tm &tm)
	{
#ifdef _WIN32
		return gmtime_s(&tm, &t);
#else
		return gmtime_r(&t, &tm);
#endif
	}

	/**
	 * @brief Create a human-readable timestamp in standard timezone
	 *
	 * The timestamp is always assumed to have timezone GMT
	 *
	 * Example: "Mon, 25 Apr 2022 17:44:42 GMT"
	 *
	 * This function is thread-safe as long as reads of time_ are atomic.
	 *
	 * @param time_ The input time_t instance
	 *
	 * @return formatted time string
	 */
	inline std::string get_internet_time_str(const time_t& time_)
	{
		char tmpbuf[200] = {0};
		struct tm pt;
		get_gmt_time(time_, pt);
		strftime(tmpbuf, 199, "%a, %d %b %Y %H:%M:%S GMT", &pt);
		return tmpbuf;
	}

	/**
	 * @brief Create a human-readable representation of a time interval
	 *
	 * Example:
	 *
	 * get_time_interval_string(100000) == "d1.h3.m46.s40"
	 *
	 * This function is thread-safe as long as reads of time_ are atomic.
	 *
	 * @param time_ time interval as difference in seconds
	 *
	 * @return formatted time interval string
	 */
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

	/**
	 * @brief Steady, monotonically increasing counter value in nanoseconds
	 *
	 * @return number of nanoseconds elapsed since some arbitrary point in the past
	 */
	inline uint64_t get_ns_count()
	{
		typedef std::chrono::duration<uint64_t, std::nano> ns_duration;
		const ns_duration ns_since_epoch = std::chrono::steady_clock::now().time_since_epoch();
		return ns_since_epoch.count();
	}

	/**
	 * @brief Steady, monotonically increasing counter value in microseconds
	 *
	 * @return number of microseconds elapsed since some arbitrary point in the past
	 */
	inline uint64_t get_us_count() {
		return get_ns_count() / 1000;
	}

	/**
	 * @brief Steady, monotonically increasing counter value in milliseconds
	 *
	 * This is named differently from get_us_count and get_us_count for historical
	 * reasons, but it does the exact same thing.
	 *
	 * @return number of milliseconds elapsed since some arbitrary point in the past
	 */
	inline uint64_t get_tick_count()
	{
		return get_ns_count() / 1000000;
	}
}

// class "once_a_time" used to be in math_helper.h, hence the namespace in *this* file.
// @TODO: Move all functions in this file to a meaningful namespace (e.g. ::time)

namespace math_helper
{
	/**
	 * @brief Controls execution of functor(s) in timed intervals, not necessarily constant
	 *
	 * @tparam get_interval type of functor which is instantiated and called at the beginning of each interval, returning the length of the next interval in microseconds
	 * @tparam start_immediate boolean value which determines whether or not to immediately fire
	 */
	template<typename get_interval, bool start_immediate = true>
	class once_a_time
	{
	public:
		/**
		 * @brief Default constructor
		 */
		inline once_a_time():
			m_last_worked_time(get_time()),
			m_interval(),
			m_force_do(start_immediate)
		{
			set_next_interval();
		}

		/**
		 * @brief Force functor to be executed on next call to do_call()
		 */
		inline void trigger()
		{
			m_force_do = true;
		}

		/**
		 * @brief Poll whether functr should be called at this time, execute if so
		 *
		 * @tparam functor_t Type of functr whose operater() returns a boolean-coercible value
		 * @param functr Functor to be executed
		 *
		 * @return The result of functr if executed, true otherwise.
		 */
		template<class functor_t>
		inline bool do_call(functor_t functr)
		{
			if (m_force_do || get_time() - m_last_worked_time > m_interval)
			{
				const bool res = functr();

				m_last_worked_time = get_time();
				set_next_interval();
				m_force_do = false;

				return res;
			}

			return true;
		}

	private:
		/**
		 * @brief Construct and call get_interval functor to get length of next interval
		 */
		inline void set_next_interval()
		{
			m_interval = get_interval()();
		}

		/**
		 * @brief The source of time for this class. Should tick time in microseconds.
		 *
		 * @return Clock counter value in microseconds
		 */
		static inline uint64_t get_time()
		{
			return misc_utils::get_us_count();
		}

		uint64_t m_last_worked_time;
		uint64_t m_interval;
		bool m_force_do;
	};

	/**
	 * @brief Constant length functor for get_interval param in class once_a_time
	 *
	 * @tparam N Length of interval in microseconds
	 */
	template<uint64_t N> struct get_constant_interval { public: uint64_t operator()() const { return N; } };

	/**
	 * @brief Specialization of once_a_time which fires every "default_interval" seconds
	 *
	 * @tparam default_interval Length of interval in seconds
	 * @tparam start_immediate boolean value which determines whether or not to immediately fire
	 */
	template<int default_interval, bool start_immediate = true>
	class once_a_time_seconds: public once_a_time<get_constant_interval<default_interval * (uint64_t)1000000>, start_immediate> {};

	/**
	 * @brief Specialization of once_a_time which fires every "default_interval" milliseconds
	 *
	 * @tparam default_interval Length of interval in milliseconds
	 * @tparam start_immediate boolean value which determines whether or not to immediately fire
	 */
	template<int default_interval, bool start_immediate = true>
	class once_a_time_milliseconds: public once_a_time<get_constant_interval<default_interval * (uint64_t)1000>, start_immediate> {};
}
}
