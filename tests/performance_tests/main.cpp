// Copyright (c) 2014-2017, The Monero Project
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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "performance_tests.h"
#include "performance_utils.h"

// tests
#include "construct_tx.h"
#include "check_tx_signature.h"
#include "cn_slow_hash.h"
#include "derive_public_key.h"
#include "derive_secret_key.h"
#include "ge_frombytes_vartime.h"
#include "generate_key_derivation.h"
#include "generate_key_image.h"
#include "generate_key_image_helper.h"
#include "generate_keypair.h"
#include "is_out_to_acc.h"
#include "sc_reduce32.h"
#include "cn_fast_hash.h"

int main(int argc, char** argv)
{
  set_process_affinity(1);
  set_thread_high_priority();

  mlog_configure(mlog_get_default_log_path("performance_tests.log"), true);
  mlog_set_log_level(0);

  performance_timer timer;
  timer.start();

  TEST_PERFORMANCE3(test_construct_tx, 1, 1, false);
  TEST_PERFORMANCE3(test_construct_tx, 1, 2, false);
  TEST_PERFORMANCE3(test_construct_tx, 1, 10, false);
  TEST_PERFORMANCE3(test_construct_tx, 1, 100, false);
  TEST_PERFORMANCE3(test_construct_tx, 1, 1000, false);

  TEST_PERFORMANCE3(test_construct_tx, 2, 1, false);
  TEST_PERFORMANCE3(test_construct_tx, 2, 2, false);
  TEST_PERFORMANCE3(test_construct_tx, 2, 10, false);
  TEST_PERFORMANCE3(test_construct_tx, 2, 100, false);

  TEST_PERFORMANCE3(test_construct_tx, 10, 1, false);
  TEST_PERFORMANCE3(test_construct_tx, 10, 2, false);
  TEST_PERFORMANCE3(test_construct_tx, 10, 10, false);
  TEST_PERFORMANCE3(test_construct_tx, 10, 100, false);

  TEST_PERFORMANCE3(test_construct_tx, 100, 1, false);
  TEST_PERFORMANCE3(test_construct_tx, 100, 2, false);
  TEST_PERFORMANCE3(test_construct_tx, 100, 10, false);
  TEST_PERFORMANCE3(test_construct_tx, 100, 100, false);

  TEST_PERFORMANCE3(test_construct_tx, 2, 1, true);
  TEST_PERFORMANCE3(test_construct_tx, 2, 2, true);
  TEST_PERFORMANCE3(test_construct_tx, 2, 10, true);

  TEST_PERFORMANCE3(test_construct_tx, 10, 1, true);
  TEST_PERFORMANCE3(test_construct_tx, 10, 2, true);
  TEST_PERFORMANCE3(test_construct_tx, 10, 10, true);

  TEST_PERFORMANCE3(test_construct_tx, 100, 1, true);
  TEST_PERFORMANCE3(test_construct_tx, 100, 2, true);
  TEST_PERFORMANCE3(test_construct_tx, 100, 10, true);

  TEST_PERFORMANCE2(test_check_tx_signature, 1, false);
  TEST_PERFORMANCE2(test_check_tx_signature, 2, false);
  TEST_PERFORMANCE2(test_check_tx_signature, 10, false);
  TEST_PERFORMANCE2(test_check_tx_signature, 100, false);

  TEST_PERFORMANCE2(test_check_tx_signature, 2, true);
  TEST_PERFORMANCE2(test_check_tx_signature, 10, true);
  TEST_PERFORMANCE2(test_check_tx_signature, 100, true);

  TEST_PERFORMANCE0(test_is_out_to_acc);
  TEST_PERFORMANCE0(test_generate_key_image_helper);
  TEST_PERFORMANCE0(test_generate_key_derivation);
  TEST_PERFORMANCE0(test_generate_key_image);
  TEST_PERFORMANCE0(test_derive_public_key);
  TEST_PERFORMANCE0(test_derive_secret_key);
  TEST_PERFORMANCE0(test_ge_frombytes_vartime);
  TEST_PERFORMANCE0(test_generate_keypair);
  TEST_PERFORMANCE0(test_sc_reduce32);

  TEST_PERFORMANCE0(test_cn_slow_hash);
  TEST_PERFORMANCE1(test_cn_fast_hash, 32);
  TEST_PERFORMANCE1(test_cn_fast_hash, 16384);

  std::cout << "Tests finished. Elapsed time: " << timer.elapsed_ms() / 1000 << " sec" << std::endl;

  return 0;
}
