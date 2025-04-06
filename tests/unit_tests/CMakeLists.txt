# Copyright (c) 2014-2024, The Monero Project
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are
# permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this list of
#    conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this list
#    of conditions and the following disclaimer in the documentation and/or other
#    materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors may be
#    used to endorse or promote products derived from this software without specific
#    prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
# THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
# THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set(unit_tests_sources
  account.cpp
  apply_permutation.cpp
  address_from_url.cpp
  base58.cpp
  blockchain_db.cpp
  block_queue.cpp
  block_reward.cpp
  bootstrap_node_selector.cpp
  bulletproofs.cpp
  bulletproofs_plus.cpp
  canonical_amounts.cpp
  carrot_core.cpp
  carrot_fcmp.cpp
  carrot_impl.cpp
  carrot_legacy.cpp
  carrot_mock_helpers.cpp
  carrot_transcript_fixed.cpp
  carrot_tx_builder.cpp
  chacha.cpp
  checkpoints.cpp
  command_line.cpp
  curve_trees.cpp
  crypto.cpp
  decompose_amount_into_digits.cpp
  device.cpp
  difficulty.cpp
  dns_resolver.cpp
  epee_boosted_tcp_server.cpp
  epee_levin_protocol_handler_async.cpp
  epee_http_server.cpp
  epee_serialization.cpp
  epee_utils.cpp
  expect.cpp
  fcmp_pp.cpp
  json_serialization.cpp
  get_xtype_from_string.cpp
  hashchain.cpp
  hmac_keccak.cpp
  http.cpp
  keccak.cpp
  levin.cpp
  logging.cpp
  long_term_block_weight.cpp
  lmdb.cpp
  main.cpp
  memwipe.cpp
  mlocker.cpp
  mnemonics.cpp
  mul_div.cpp
  multiexp.cpp
  multisig.cpp
  net.cpp
  node_server.cpp
  notify.cpp
  output_distribution.cpp
  parse_amount.cpp
  pruning.cpp
  random.cpp
  rolling_median.cpp
  scaling_2021.cpp
  serialization.cpp
  sha256.cpp
  slow_memmem.cpp
  subaddress.cpp
  test_tx_utils.cpp
  test_peerlist.cpp
  test_protocol_pack.cpp
  threadpool.cpp
  tree_cache.cpp
  tx_proof.cpp
  hardfork.cpp
  unbound.cpp
  uri.cpp
  variant.cpp
  util.cpp
  varint.cpp
  ver_rct_non_semantics_simple_cached.cpp
  ringct.cpp
  output_selection.cpp
  vercmp.cpp
  ringdb.cpp
  wallet_scanning.cpp
  wallet_storage.cpp
  wallet_tx_builder.cpp
  wipeable_string.cpp
  is_hdd.cpp
  aligned.cpp
  rpc_version_str.cpp
  x25519.cpp
  zmq_rpc.cpp)

set(unit_tests_headers
  unit_tests_utils.h)

monero_add_minimal_executable(unit_tests
  ${unit_tests_sources}
  ${unit_tests_headers})
target_link_libraries(unit_tests
  PRIVATE
    ringct
    carrot_core
    carrot_impl
    cryptonote_protocol
    cryptonote_core
    daemon_messages
    daemon_rpc_server
    blockchain_db
    fcmp_pp
    lmdb_lib
    rpc
    net
    serialization
    seraphis_crypto
    wallet
    p2p
    version
    ${Boost_CHRONO_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${GTEST_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${EXTRA_LIBRARIES}
    PkgConfig::libzmq)
set_property(TARGET unit_tests
  PROPERTY
    FOLDER "tests")

set_property(TARGET unit_tests
  APPEND_STRING
  PROPERTY
    COMPILE_FLAGS " -Wno-undef -Wno-sign-compare")

SET_PROPERTY(SOURCE memwipe.cpp PROPERTY COMPILE_FLAGS -Ofast)

add_test(
  NAME    unit_tests
  COMMAND unit_tests --data-dir "${TEST_DATA_DIR}")

monero_add_minimal_executable(test_notifier test_notifier.cpp)
target_link_libraries(test_notifier ${EXTRA_LIBRARIES})
set_property(TARGET test_notifier PROPERTY FOLDER "tests")
