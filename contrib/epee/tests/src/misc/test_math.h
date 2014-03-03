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

#include "misc_language.h"

namespace epee
{
  namespace tests
  {
    bool test_median()
    {
      LOG_PRINT_L0("Testing median");
      std::vector<size_t> sz;
      size_t m = misc_utils::median(sz);
      CHECK_AND_ASSERT_MES(m == 0, false, "test failed");
      sz.push_back(1);
      m = misc_utils::median(sz);
      CHECK_AND_ASSERT_MES(m == 1, false, "test failed");
      sz.push_back(10);
      m = misc_utils::median(sz);
      CHECK_AND_ASSERT_MES(m == 5, false, "test failed");
      
      sz.clear();
      sz.resize(3);
      sz[0] = 0;
      sz[1] = 9;
      sz[2] = 3;
      m = misc_utils::median(sz);
      CHECK_AND_ASSERT_MES(m == 3, false, "test failed");

      sz.clear();
      sz.resize(4);
      sz[0] = 77;
      sz[1] = 9;
      sz[2] = 22;
      sz[3] = 60;
      m = misc_utils::median(sz);
      CHECK_AND_ASSERT_MES(m == 41, false, "test failed");



      sz.clear();
      sz.resize(5);
      sz[0] = 77;
      sz[1] = 9;
      sz[2] = 22;
      sz[3] = 60;
      sz[4] = 11;
      m = misc_utils::median(sz);
      CHECK_AND_ASSERT_MES(m == 22, false, "test failed");
      return true;
    }
  }
}

