// Copyright (c) 2016-2018, The Monero Project
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <string>
#include <stdio.h>
#include <memory>
#include "misc_log_ex.h"

namespace tools
{

class PerformanceTimer;

extern el::Level performance_timer_log_level;

uint64_t get_tick_count();
uint64_t get_ticks_per_ns();
uint64_t ticks_to_ns(uint64_t ticks);

class PerformanceTimer
{
public:
  PerformanceTimer(bool paused = false);
  ~PerformanceTimer();
  void pause();
  void resume();

  uint64_t value() const { return ticks; }

protected:
  uint64_t ticks;
  bool started;
  bool paused;
};

class LoggingPerformanceTimer: public PerformanceTimer
{
public:
  LoggingPerformanceTimer(const std::string &s, const std::string &cat, uint64_t unit, el::Level l = el::Level::Debug);
  ~LoggingPerformanceTimer();

private:
  std::string name;
  std::string cat;
  uint64_t unit;
  el::Level level;
};

void set_performance_timer_log_level(el::Level level);

#define PERF_TIMER_UNIT(name, unit) tools::LoggingPerformanceTimer pt_##name(#name, "perf." MONERO_DEFAULT_LOG_CATEGORY, unit, tools::performance_timer_log_level)
#define PERF_TIMER_UNIT_L(name, unit, l) tools::LoggingPerformanceTimer pt_##name(#name, "perf." MONERO_DEFAULT_LOG_CATEGORY, unit, l)
#define PERF_TIMER(name) PERF_TIMER_UNIT(name, 1000000)
#define PERF_TIMER_L(name, l) PERF_TIMER_UNIT_L(name, 1000000, l)
#define PERF_TIMER_START_UNIT(name, unit) std::unique_ptr<tools::LoggingPerformanceTimer> pt_##name(new tools::LoggingPerformanceTimer(#name, "perf." MONERO_DEFAULT_LOG_CATEGORY, unit, el::Level::Info))
#define PERF_TIMER_START(name) PERF_TIMER_START_UNIT(name, 1000000)
#define PERF_TIMER_STOP(name) do { pt_##name.reset(NULL); } while(0)
#define PERF_TIMER_PAUSE(name) pt_##name->pause()
#define PERF_TIMER_RESUME(name) pt_##name->resume()

}
