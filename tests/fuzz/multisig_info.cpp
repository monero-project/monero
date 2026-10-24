// Copyright (c) 2026, The Monero Project
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

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "include_base_utils.h"
#include "wallet/wallet2.h"
#include "fuzzer.h"
#include "multisig_init.h"

static tools::wallet2 *wallet = NULL;

static constexpr size_t MAX_INPUT_SIZE = std::numeric_limits<std::uint16_t>::max();

BEGIN_INIT_SIMPLE_FUZZER()
  static tools::wallet2 local_wallet;
  wallet = &local_wallet;
  multisig_init(*wallet);
END_INIT_SIMPLE_FUZZER()

BEGIN_SIMPLE_FUZZER()
  if (len > MAX_INPUT_SIZE) return 0;

  const cryptonote::account_public_address &keys = wallet->get_account().get_keys().m_account_address;
  std::string plaintext;
  plaintext.append((const char*)&keys.m_spend_public_key, sizeof(crypto::public_key));
  plaintext.append((const char*)&keys.m_view_public_key, sizeof(crypto::public_key));
  plaintext.append((const char*)&multisig_other_signer, sizeof(crypto::public_key));
  plaintext.append((const char*)buf, len);
  const std::string blob = MULTISIG_EXPORT_FILE_MAGIC + wallet->encrypt_with_view_secret_key(plaintext);

  try
  {
    wallet->import_multisig({blob}, false);
  }
  catch (const std::exception &)
  {
  }
END_SIMPLE_FUZZER()
