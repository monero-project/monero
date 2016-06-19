// Copyright (c) 2016, The Monero Project
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

#include "common/stack_trace.h"
#include "misc_log_ex.h"
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <cxxabi.h>
#include <dlfcn.h>

// from http://stackoverflow.com/questions/11665829/how-can-i-print-stack-trace-for-caught-exceptions-in-c-code-injection-in-c
#ifdef STATICLIB
#define CXA_THROW __wrap___cxa_throw
extern "C" void __real___cxa_throw(void *ex, void *info, void (*dest)(void*));
#else
#define CXA_THROW __cxa_throw
#endif

extern "C" void CXA_THROW(void *ex, void *info, void (*dest)(void*))
{

  int status;
  char *dsym = abi::__cxa_demangle(((const std::type_info*)info)->name(), NULL, NULL, &status);
  tools::log_stack_trace((std::string("Exception: ")+((!status && dsym) ? dsym : (const char*)info)).c_str());
  free(dsym);

#ifdef STATICLIB
  __real___cxa_throw(ex, info, dest);
#else
  static void (*const rethrow)(void*, void*, void(*)(void*)) __attribute__((noreturn)) = (void(*)(void*, void*, void(*)(void*)))dlsym(RTLD_NEXT, "__cxa_throw");
  rethrow(ex, info, dest);
#endif
}

namespace
{
  std::string stack_trace_log;
}

namespace tools
{

void set_stack_trace_log(const std::string &log)
{
  stack_trace_log = log;
}

void log_stack_trace(const char *msg)
{
#ifdef HAVE_LIBUNWIND
  unw_context_t ctx;
  unw_cursor_t cur;
  unw_word_t ip, off;
  unsigned level;
  char sym[512], *dsym;
  int status;
  const char *log = stack_trace_log.empty() ? NULL : stack_trace_log.c_str();

  if (msg)
    LOG_PRINT2(log, msg, LOG_LEVEL_0);
  LOG_PRINT2(log, "Unwinded call stack:", LOG_LEVEL_0);
  if (unw_getcontext(&ctx) < 0) {
    LOG_PRINT2(log, "Failed to create unwind context", LOG_LEVEL_0);
    return;
  }
  if (unw_init_local(&cur, &ctx) < 0) {
    LOG_PRINT2(log, "Failed to find the first unwind frame", LOG_LEVEL_0);
    return;
  }
  for (level = 1; level < 999; ++level) { // 999 for safety
    int ret = unw_step(&cur);
    if (ret < 0) {
      LOG_PRINT2(log, "Failed to find the next frame", LOG_LEVEL_0);
      return;
    }
    if (ret == 0)
      break;
    if (unw_get_reg(&cur, UNW_REG_IP, &ip) < 0) {
      LOG_PRINT2(log, "  " << std::setw(4) << level, LOG_LEVEL_0);
      continue;
    }
    if (unw_get_proc_name(&cur, sym, sizeof(sym), &off) < 0) {
      LOG_PRINT2(log, "  " << std::setw(4) << level << std::setbase(16) << std::setw(20) << "0x" << ip, LOG_LEVEL_0);
      continue;
    }
    dsym = abi::__cxa_demangle(sym, NULL, NULL, &status);
    LOG_PRINT2(log, "  " << std::setw(4) << level << std::setbase(16) << std::setw(20) << "0x" << ip << " " << (!status && dsym ? dsym : sym) << " + " << "0x" << off, LOG_LEVEL_0);
    free(dsym);
  }
#else
#warning libunwind disabled, no stack traces
#endif
}

}  // namespace tools
