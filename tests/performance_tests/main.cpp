// Copyright (c) 2014-2018, The Monero Project
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

#include <boost/regex.hpp>

#include "common/util.h"
#include "common/command_line.h"
#include "performance_tests.h"
#include "performance_utils.h"

// tests
#include "construct_tx.h"
#include "check_tx_signature.h"
#include "cn_slow_hash.h"
#include "derive_public_key.h"
#include "derive_secret_key.h"
#include "ge_frombytes_vartime.h"
#include "ge_tobytes.h"
#include "generate_key_derivation.h"
#include "generate_key_image.h"
#include "generate_key_image_helper.h"
#include "generate_keypair.h"
#include "signature.h"
#include "is_out_to_acc.h"
#include "subaddress_expand.h"
#include "sc_reduce32.h"
#include "sc_check.h"
#include "cn_fast_hash.h"
#include "rct_mlsag.h"
#include "equality.h"
#include "range_proof.h"
#include "rct_mlsag.h"
#include "bulletproof.h"
#include "crypto_ops.h"
#include "multiexp.h"

namespace po = boost::program_options;

int main(int argc, char** argv)
{
  TRY_ENTRY();
  tools::on_startup();
  set_process_affinity(1);
  set_thread_high_priority();

  mlog_configure(mlog_get_default_log_path("performance_tests.log"), true);

  po::options_description desc_options("Command line options");
  const command_line::arg_descriptor<std::string> arg_filter = { "filter", "Regular expression filter for which tests to run" };
  const command_line::arg_descriptor<bool> arg_verbose = { "verbose", "Verbose output", false };
  const command_line::arg_descriptor<bool> arg_stats = { "stats", "Including statistics (min/median)", false };
  const command_line::arg_descriptor<unsigned> arg_loop_multiplier = { "loop-multiplier", "Run for that many times more loops", 1 };
  command_line::add_arg(desc_options, arg_filter);
  command_line::add_arg(desc_options, arg_verbose);
  command_line::add_arg(desc_options, arg_stats);
  command_line::add_arg(desc_options, arg_loop_multiplier);

  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc_options, [&]()
  {
    po::store(po::parse_command_line(argc, argv, desc_options), vm);
    po::notify(vm);
    return true;
  });
  if (!r)
    return 1;

  const std::string filter = tools::glob_to_regex(command_line::get_arg(vm, arg_filter));
  Params p;
  p.verbose = command_line::get_arg(vm, arg_verbose);
  p.stats = command_line::get_arg(vm, arg_stats);
  p.loop_multiplier = command_line::get_arg(vm, arg_loop_multiplier);

  performance_timer timer;
  timer.start();

  TEST_PERFORMANCE3(filter, p, test_construct_tx, 1, 1, false);
  TEST_PERFORMANCE3(filter, p, test_construct_tx, 1, 2, false);
  TEST_PERFORMANCE3(filter, p, test_construct_tx, 1, 10, false);
  TEST_PERFORMANCE3(filter, p, test_construct_tx, 1, 100, false);
  TEST_PERFORMANCE3(filter, p, test_construct_tx, 1, 1000, false);

  TEST_PERFORMANCE3(filter, p, test_construct_tx, 2, 1, false);
  TEST_PERFORMANCE3(filter, p, test_construct_tx, 2, 2, false);
  TEST_PERFORMANCE3(filter, p, test_construct_tx, 2, 10, false);
  TEST_PERFORMANCE3(filter, p, test_construct_tx, 2, 100, false);

  TEST_PERFORMANCE3(filter, p, test_construct_tx, 10, 1, false);
  TEST_PERFORMANCE3(filter, p, test_construct_tx, 10, 2, false);
  TEST_PERFORMANCE3(filter, p, test_construct_tx, 10, 10, false);
  TEST_PERFORMANCE3(filter, p, test_construct_tx, 10, 100, false);

  TEST_PERFORMANCE3(filter, p, test_construct_tx, 100, 1, false);
  TEST_PERFORMANCE3(filter, p, test_construct_tx, 100, 2, false);
  TEST_PERFORMANCE3(filter, p, test_construct_tx, 100, 10, false);
  TEST_PERFORMANCE3(filter, p, test_construct_tx, 100, 100, false);

  TEST_PERFORMANCE3(filter, p, test_construct_tx, 2, 1, true);
  TEST_PERFORMANCE3(filter, p, test_construct_tx, 2, 2, true);
  TEST_PERFORMANCE3(filter, p, test_construct_tx, 2, 10, true);

  TEST_PERFORMANCE3(filter, p, test_construct_tx, 10, 1, true);
  TEST_PERFORMANCE3(filter, p, test_construct_tx, 10, 2, true);
  TEST_PERFORMANCE3(filter, p, test_construct_tx, 10, 10, true);

  TEST_PERFORMANCE3(filter, p, test_construct_tx, 100, 1, true);
  TEST_PERFORMANCE3(filter, p, test_construct_tx, 100, 2, true);
  TEST_PERFORMANCE3(filter, p, test_construct_tx, 100, 10, true);

  TEST_PERFORMANCE4(filter, p, test_construct_tx, 2, 1, true, rct::RangeProofPaddedBulletproof);
  TEST_PERFORMANCE4(filter, p, test_construct_tx, 2, 2, true, rct::RangeProofPaddedBulletproof);
  TEST_PERFORMANCE4(filter, p, test_construct_tx, 2, 10, true, rct::RangeProofPaddedBulletproof);

  TEST_PERFORMANCE4(filter, p, test_construct_tx, 10, 1, true, rct::RangeProofPaddedBulletproof);
  TEST_PERFORMANCE4(filter, p, test_construct_tx, 10, 2, true, rct::RangeProofPaddedBulletproof);
  TEST_PERFORMANCE4(filter, p, test_construct_tx, 10, 10, true, rct::RangeProofPaddedBulletproof);

  TEST_PERFORMANCE4(filter, p, test_construct_tx, 100, 1, true, rct::RangeProofPaddedBulletproof);
  TEST_PERFORMANCE4(filter, p, test_construct_tx, 100, 2, true, rct::RangeProofPaddedBulletproof);
  TEST_PERFORMANCE4(filter, p, test_construct_tx, 100, 10, true, rct::RangeProofPaddedBulletproof);

  TEST_PERFORMANCE3(filter, p, test_check_tx_signature, 1, 2, false);
  TEST_PERFORMANCE3(filter, p, test_check_tx_signature, 2, 2, false);
  TEST_PERFORMANCE3(filter, p, test_check_tx_signature, 10, 2, false);
  TEST_PERFORMANCE3(filter, p, test_check_tx_signature, 100, 2, false);
  TEST_PERFORMANCE3(filter, p, test_check_tx_signature, 2, 10, false);

  TEST_PERFORMANCE4(filter, p, test_check_tx_signature, 2, 2, true, rct::RangeProofBorromean);
  TEST_PERFORMANCE4(filter, p, test_check_tx_signature, 10, 2, true, rct::RangeProofBorromean);
  TEST_PERFORMANCE4(filter, p, test_check_tx_signature, 100, 2, true, rct::RangeProofBorromean);
  TEST_PERFORMANCE4(filter, p, test_check_tx_signature, 2, 10, true, rct::RangeProofBorromean);

  TEST_PERFORMANCE4(filter, p, test_check_tx_signature, 2, 2, true, rct::RangeProofPaddedBulletproof);
  TEST_PERFORMANCE4(filter, p, test_check_tx_signature, 2, 2, true, rct::RangeProofMultiOutputBulletproof);
  TEST_PERFORMANCE4(filter, p, test_check_tx_signature, 10, 2, true, rct::RangeProofPaddedBulletproof);
  TEST_PERFORMANCE4(filter, p, test_check_tx_signature, 10, 2, true, rct::RangeProofMultiOutputBulletproof);
  TEST_PERFORMANCE4(filter, p, test_check_tx_signature, 100, 2, true, rct::RangeProofPaddedBulletproof);
  TEST_PERFORMANCE4(filter, p, test_check_tx_signature, 100, 2, true, rct::RangeProofMultiOutputBulletproof);
  TEST_PERFORMANCE4(filter, p, test_check_tx_signature, 2, 10, true, rct::RangeProofPaddedBulletproof);
  TEST_PERFORMANCE4(filter, p, test_check_tx_signature, 2, 10, true, rct::RangeProofMultiOutputBulletproof);

  TEST_PERFORMANCE3(filter, p, test_check_tx_signature_aggregated_bulletproofs, 2, 2, 64);
  TEST_PERFORMANCE3(filter, p, test_check_tx_signature_aggregated_bulletproofs, 10, 2, 64);
  TEST_PERFORMANCE3(filter, p, test_check_tx_signature_aggregated_bulletproofs, 100, 2, 64);
  TEST_PERFORMANCE3(filter, p, test_check_tx_signature_aggregated_bulletproofs, 2, 10, 64);

  TEST_PERFORMANCE4(filter, p, test_check_tx_signature_aggregated_bulletproofs, 2, 2, 62, 4);
  TEST_PERFORMANCE4(filter, p, test_check_tx_signature_aggregated_bulletproofs, 10, 2, 62, 4);
  TEST_PERFORMANCE4(filter, p, test_check_tx_signature_aggregated_bulletproofs, 2, 2, 56, 16);
  TEST_PERFORMANCE4(filter, p, test_check_tx_signature_aggregated_bulletproofs, 10, 2, 56, 16);

  TEST_PERFORMANCE0(filter, p, test_is_out_to_acc);
  TEST_PERFORMANCE0(filter, p, test_is_out_to_acc_precomp);
  TEST_PERFORMANCE0(filter, p, test_generate_key_image_helper);
  TEST_PERFORMANCE0(filter, p, test_generate_key_derivation);
  TEST_PERFORMANCE0(filter, p, test_generate_key_image);
  TEST_PERFORMANCE0(filter, p, test_derive_public_key);
  TEST_PERFORMANCE0(filter, p, test_derive_secret_key);
  TEST_PERFORMANCE0(filter, p, test_ge_frombytes_vartime);
  TEST_PERFORMANCE0(filter, p, test_ge_tobytes);
  TEST_PERFORMANCE0(filter, p, test_generate_keypair);
  TEST_PERFORMANCE0(filter, p, test_sc_reduce32);
  TEST_PERFORMANCE0(filter, p, test_sc_check);
  TEST_PERFORMANCE1(filter, p, test_signature, false);
  TEST_PERFORMANCE1(filter, p, test_signature, true);

  TEST_PERFORMANCE2(filter, p, test_wallet2_expand_subaddresses, 50, 200);

  TEST_PERFORMANCE0(filter, p, test_cn_slow_hash);
  TEST_PERFORMANCE1(filter, p, test_cn_fast_hash, 32);
  TEST_PERFORMANCE1(filter, p, test_cn_fast_hash, 16384);

  TEST_PERFORMANCE3(filter, p, test_ringct_mlsag, 1, 3, false);
  TEST_PERFORMANCE3(filter, p, test_ringct_mlsag, 1, 5, false);
  TEST_PERFORMANCE3(filter, p, test_ringct_mlsag, 1, 10, false);
  TEST_PERFORMANCE3(filter, p, test_ringct_mlsag, 1, 100, false);
  TEST_PERFORMANCE3(filter, p, test_ringct_mlsag, 1, 3, true);
  TEST_PERFORMANCE3(filter, p, test_ringct_mlsag, 1, 5, true);
  TEST_PERFORMANCE3(filter, p, test_ringct_mlsag, 1, 10, true);
  TEST_PERFORMANCE3(filter, p, test_ringct_mlsag, 1, 100, true);

  TEST_PERFORMANCE2(filter, p, test_equality, memcmp32, true);
  TEST_PERFORMANCE2(filter, p, test_equality, memcmp32, false);
  TEST_PERFORMANCE2(filter, p, test_equality, verify32, false);
  TEST_PERFORMANCE2(filter, p, test_equality, verify32, false);

  TEST_PERFORMANCE1(filter, p, test_range_proof, true);
  TEST_PERFORMANCE1(filter, p, test_range_proof, false);

  TEST_PERFORMANCE2(filter, p, test_bulletproof, true, 1); // 1 bulletproof with 1 amount
  TEST_PERFORMANCE2(filter, p, test_bulletproof, false, 1);

  TEST_PERFORMANCE2(filter, p, test_bulletproof, true, 2); // 1 bulletproof with 2 amounts
  TEST_PERFORMANCE2(filter, p, test_bulletproof, false, 2);

  TEST_PERFORMANCE2(filter, p, test_bulletproof, true, 15); // 1 bulletproof with 15 amounts
  TEST_PERFORMANCE2(filter, p, test_bulletproof, false, 15);

  TEST_PERFORMANCE6(filter, p, test_aggregated_bulletproof, false, 2, 1, 1, 0, 4);
  TEST_PERFORMANCE6(filter, p, test_aggregated_bulletproof, true, 2, 1, 1, 0, 4); // 4 proofs, each with 2 amounts
  TEST_PERFORMANCE6(filter, p, test_aggregated_bulletproof, false, 8, 1, 1, 0, 4);
  TEST_PERFORMANCE6(filter, p, test_aggregated_bulletproof, true, 8, 1, 1, 0, 4); // 4 proofs, each with 8 amounts
  TEST_PERFORMANCE6(filter, p, test_aggregated_bulletproof, false, 1, 1, 2, 0, 4);
  TEST_PERFORMANCE6(filter, p, test_aggregated_bulletproof, true, 1, 1, 2, 0, 4); // 4 proofs with 1, 2, 4, 8 amounts
  TEST_PERFORMANCE6(filter, p, test_aggregated_bulletproof, false, 1, 8, 1, 1, 4);
  TEST_PERFORMANCE6(filter, p, test_aggregated_bulletproof, true, 1, 8, 1, 1, 4); // 32 proofs, with 1, 2, 3, 4 amounts, 8 of each
  TEST_PERFORMANCE6(filter, p, test_aggregated_bulletproof, false, 2, 1, 1, 0, 64);
  TEST_PERFORMANCE6(filter, p, test_aggregated_bulletproof, true, 2, 1, 1, 0, 64); // 64 proof, each with 2 amounts

  TEST_PERFORMANCE3(filter, p, test_ringct_mlsag, 1, 3, false);
  TEST_PERFORMANCE3(filter, p, test_ringct_mlsag, 1, 5, false);
  TEST_PERFORMANCE3(filter, p, test_ringct_mlsag, 1, 10, false);
  TEST_PERFORMANCE3(filter, p, test_ringct_mlsag, 1, 100, false);
  TEST_PERFORMANCE3(filter, p, test_ringct_mlsag, 1, 3, true);
  TEST_PERFORMANCE3(filter, p, test_ringct_mlsag, 1, 5, true);
  TEST_PERFORMANCE3(filter, p, test_ringct_mlsag, 1, 10, true);
  TEST_PERFORMANCE3(filter, p, test_ringct_mlsag, 1, 100, true);

  TEST_PERFORMANCE1(filter, p, test_crypto_ops, op_sc_add);
  TEST_PERFORMANCE1(filter, p, test_crypto_ops, op_sc_sub);
  TEST_PERFORMANCE1(filter, p, test_crypto_ops, op_sc_mul);
  TEST_PERFORMANCE1(filter, p, test_crypto_ops, op_ge_add_raw);
  TEST_PERFORMANCE1(filter, p, test_crypto_ops, op_ge_add_p3_p3);
  TEST_PERFORMANCE1(filter, p, test_crypto_ops, op_addKeys);
  TEST_PERFORMANCE1(filter, p, test_crypto_ops, op_scalarmultBase);
  TEST_PERFORMANCE1(filter, p, test_crypto_ops, op_scalarmultKey);
  TEST_PERFORMANCE1(filter, p, test_crypto_ops, op_scalarmultH);
  TEST_PERFORMANCE1(filter, p, test_crypto_ops, op_scalarmult8);
  TEST_PERFORMANCE1(filter, p, test_crypto_ops, op_ge_dsm_precomp);
  TEST_PERFORMANCE1(filter, p, test_crypto_ops, op_ge_double_scalarmult_base_vartime);
  TEST_PERFORMANCE1(filter, p, test_crypto_ops, op_ge_double_scalarmult_precomp_vartime);
  TEST_PERFORMANCE1(filter, p, test_crypto_ops, op_ge_double_scalarmult_precomp_vartime2);
  TEST_PERFORMANCE1(filter, p, test_crypto_ops, op_addKeys2);
  TEST_PERFORMANCE1(filter, p, test_crypto_ops, op_addKeys3);
  TEST_PERFORMANCE1(filter, p, test_crypto_ops, op_addKeys3_2);
  TEST_PERFORMANCE1(filter, p, test_crypto_ops, op_isInMainSubgroup);
  TEST_PERFORMANCE1(filter, p, test_crypto_ops, op_zeroCommitUncached);
  TEST_PERFORMANCE1(filter, p, test_crypto_ops, op_zeroCommitCached);

  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_bos_coster, 2);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_bos_coster, 4);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_bos_coster, 8);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_bos_coster, 16);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_bos_coster, 32);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_bos_coster, 64);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_bos_coster, 128);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_bos_coster, 256);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_bos_coster, 512);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_bos_coster, 1024);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_bos_coster, 2048);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_bos_coster, 4096);

  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_straus, 2);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_straus, 4);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_straus, 8);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_straus, 16);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_straus, 32);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_straus, 64);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_straus, 128);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_straus, 256);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_straus, 512);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_straus, 1024);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_straus, 2048);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_straus, 4096);

  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_straus_cached, 2);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_straus_cached, 4);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_straus_cached, 8);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_straus_cached, 16);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_straus_cached, 32);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_straus_cached, 64);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_straus_cached, 128);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_straus_cached, 256);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_straus_cached, 512);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_straus_cached, 1024);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_straus_cached, 2048);
  TEST_PERFORMANCE2(filter, p, test_multiexp, multiexp_straus_cached, 4096);

#if 1
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 2, 1);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 4, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 8, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 16, 3);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 32, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 64, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 128, 5);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 256, 6);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 512, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 1024, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 2048, 8);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 4096, 9);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 2, 1);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 4, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 8, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 16, 3);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 32, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 64, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 128, 5);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 256, 6);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 512, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 1024, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 2048, 8);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 4096, 9);
#else
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 2, 1);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 2, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 2, 3);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 2, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 2, 5);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 2, 6);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 2, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 2, 8);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 2, 9);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 4, 1);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 4, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 4, 3);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 4, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 4, 5);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 4, 6);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 4, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 4, 8);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 4, 9);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 8, 1);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 8, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 8, 3);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 8, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 8, 5);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 8, 6);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 8, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 8, 8);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 8, 9);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 16, 1);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 16, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 16, 3);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 16, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 16, 5);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 16, 6);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 16, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 16, 8);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 16, 9);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 32, 1);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 32, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 32, 3);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 32, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 32, 5);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 32, 6);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 32, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 32, 8);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 32, 9);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 64, 1);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 64, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 64, 3);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 64, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 64, 5);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 64, 6);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 64, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 64, 8);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 64, 9);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 128, 1);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 128, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 128, 3);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 128, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 128, 5);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 128, 6);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 128, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 128, 8);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 128, 9);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 256, 1);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 256, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 256, 3);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 256, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 256, 5);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 256, 6);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 256, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 256, 8);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 256, 9);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 512, 1);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 512, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 512, 3);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 512, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 512, 5);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 512, 6);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 512, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 512, 8);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 512, 9);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 1024, 1);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 1024, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 1024, 3);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 1024, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 1024, 5);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 1024, 6);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 1024, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 1024, 8);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 1024, 9);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 2048, 1);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 2048, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 2048, 3);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 2048, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 2048, 5);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 2048, 6);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 2048, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 2048, 8);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 2048, 9);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 4096, 1);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 4096, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 4096, 3);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 4096, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 4096, 5);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 4096, 6);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 4096, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 4096, 8);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger_cached, 4096, 9);

  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 2, 1);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 2, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 2, 3);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 2, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 2, 5);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 2, 6);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 2, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 2, 8);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 2, 9);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 4, 1);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 4, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 4, 3);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 4, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 4, 5);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 4, 6);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 4, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 4, 8);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 4, 9);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 8, 1);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 8, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 8, 3);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 8, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 8, 5);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 8, 6);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 8, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 8, 8);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 8, 9);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 16, 1);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 16, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 16, 3);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 16, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 16, 5);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 16, 6);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 16, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 16, 8);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 16, 9);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 32, 1);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 32, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 32, 3);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 32, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 32, 5);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 32, 6);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 32, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 32, 8);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 32, 9);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 64, 1);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 64, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 64, 3);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 64, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 64, 5);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 64, 6);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 64, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 64, 8);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 64, 9);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 128, 1);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 128, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 128, 3);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 128, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 128, 5);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 128, 6);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 128, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 128, 8);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 128, 9);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 256, 1);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 256, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 256, 3);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 256, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 256, 5);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 256, 6);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 256, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 256, 8);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 256, 9);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 512, 1);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 512, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 512, 3);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 512, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 512, 5);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 512, 6);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 512, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 512, 8);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 512, 9);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 1024, 1);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 1024, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 1024, 3);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 1024, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 1024, 5);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 1024, 6);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 1024, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 1024, 8);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 1024, 9);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 2048, 1);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 2048, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 2048, 3);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 2048, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 2048, 5);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 2048, 6);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 2048, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 2048, 8);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 2048, 9);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 4096, 1);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 4096, 2);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 4096, 3);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 4096, 4);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 4096, 5);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 4096, 6);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 4096, 7);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 4096, 8);
  TEST_PERFORMANCE3(filter, p, test_multiexp, multiexp_pippenger, 4096, 9);
#endif

  std::cout << "Tests finished. Elapsed time: " << timer.elapsed_ms() / 1000 << " sec" << std::endl;

  return 0;
  CATCH_ENTRY_L0("main", 1);
}
