// Copyright (c) 2017-2022, The Monero Project
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

#include <string>
#include "file_io_utils.h"

#ifdef OSSFUZZ

#define BEGIN_INIT_SIMPLE_FUZZER() \
  static int init() \
  { \
    try \
    {

#define END_INIT_SIMPLE_FUZZER() \
    } \
    catch (const std::exception &e) \
    { \
      fprintf(stderr, "Exception: %s\n", e.what()); \
      return 1; \
    } \
    return 0; \
  }

#define BEGIN_SIMPLE_FUZZER() \
extern "C" { \
  int LLVMFuzzerTestOneInput(const uint8_t *buf, size_t len) \
  { \
    try \
    { \
      static bool first = true; \
      if (first) \
      { \
        if (init()) \
          return 1; \
        first = false; \
      } \

#define END_SIMPLE_FUZZER() \
    } \
    catch (const std::exception &e) \
    { \
      fprintf(stderr, "Exception: %s\n", e.what()); \
      delete el::base::elStorage; \
      el::base::elStorage = NULL; \
      return 0; \
    } \
    delete el::base::elStorage; \
    el::base::elStorage = NULL; \
    return 0; \
  } \
}

#else

class Fuzzer
{
public:
  virtual int init() { return 0; }
  virtual int run(const std::string &filename) = 0;
};

int run_fuzzer(int argc, const char **argv, Fuzzer &fuzzer);

#define BEGIN_INIT_SIMPLE_FUZZER() \
  class SimpleFuzzer: public Fuzzer \
  { \
    virtual int init() \
    { \
      try \
      {

#define END_INIT_SIMPLE_FUZZER() \
      } \
      catch (const std::exception &e) \
      { \
        fprintf(stderr, "Exception: %s\n", e.what()); \
        return 1; \
      } \
      return 0; \
    }

#define BEGIN_SIMPLE_FUZZER() \
    virtual int run(const std::string &filename) \
    { \
      try \
      { \
        std::string s; \
        if (!epee::file_io_utils::load_file_to_string(filename, s)) \
        { \
          std::cout << "Error: failed to load file " << filename << std::endl; \
          return 1; \
        } \
        const uint8_t *buf = (const uint8_t*)s.data(); \
        const size_t len = s.size(); \
        {

#define END_SIMPLE_FUZZER() \
        } \
      } \
      catch (const std::exception &e) \
      { \
        fprintf(stderr, "Exception: %s\n", e.what()); \
        delete el::base::elStorage; \
        el::base::elStorage = NULL; \
        return 0; \
      } \
      delete el::base::elStorage; \
      el::base::elStorage = NULL; \
      return 0; \
    } \
  }; \
  int main(int argc, const char **argv) \
  { \
    TRY_ENTRY(); \
    SimpleFuzzer fuzzer; \
    return run_fuzzer(argc, argv, fuzzer); \
    CATCH_ENTRY_L0("main", 1); \
  }

#endif
