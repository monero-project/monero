# Copyright (c) 2014-2018, The Monero Project
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

set(performance_tests_sources
  main.cpp)

set(performance_tests_headers
  check_tx_signature.h
  cn_slow_hash.h
  construct_tx.h
  derive_public_key.h
  derive_secret_key.h
  ge_frombytes_vartime.h
  generate_key_derivation.h
  generate_key_image.h
  generate_key_image_helper.h
  generate_keypair.h
  signature.h
  is_out_to_acc.h
  subaddress_expand.h
  range_proof.h
  bulletproof.h
  crypto_ops.h
  multiexp.h
  multi_tx_test_base.h
  performance_tests.h
  performance_utils.h
  single_tx_test_base.h)

add_executable(performance_tests
  ${performance_tests_sources}
  ${performance_tests_headers})
target_link_libraries(performance_tests
  PRIVATE
    wallet
    cryptonote_core
    common
    cncrypto
    epee
    ${Boost_CHRONO_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${EXTRA_LIBRARIES})
set_property(TARGET performance_tests
  PROPERTY
    FOLDER "tests")
