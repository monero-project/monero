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
#include <cstring>
#include <limits>
#include <map>
#include <string>
#include <vector>

#include "include_base_utils.h"
#include "string_tools.h"
#include "multisig/multisig_account.h"
#include "multisig/multisig_kex_msg.h"
#include "fuzzer.h"

extern "C"
{
#include "crypto/crypto-ops.h"
}

static const char* const base_privkey_hex = "2dd6e34a234c3e8b5d29a371789e4601e96dee4ea6f7ef79224d1a2d91164c01";
static const char* const base_common_privkey_hex = "fac47aecc948ce9d3531aa042abb18235b1df632087c55a361b632ffdd6ede0c";

static crypto::secret_key base_privkey;
static crypto::secret_key base_common_privkey;

static constexpr size_t MAX_INPUT_SIZE = std::numeric_limits<std::uint16_t>::max();
static constexpr size_t MAX_MSGS = 16;
static constexpr size_t MAX_SIGNERS = 6;

// interpret 32 fuzzer-controlled bytes as a valid (in-range) ed25519 scalar
static crypto::secret_key scalar_from_bytes(const uint8_t *data)
{
  crypto::secret_key sk;
  std::memcpy(&sk, data, sizeof(crypto::secret_key));
  sc_reduce32((unsigned char*)&sk);
  return sk;
}

BEGIN_INIT_SIMPLE_FUZZER()
  CHECK_AND_ASSERT_THROW_MES(epee::string_tools::hex_to_pod(base_privkey_hex, base_privkey), "Bad base privkey");
  CHECK_AND_ASSERT_THROW_MES(epee::string_tools::hex_to_pod(base_common_privkey_hex, base_common_privkey),
    "Bad base common privkey");
END_INIT_SIMPLE_FUZZER()

BEGIN_SIMPLE_FUZZER()
  if (len > MAX_INPUT_SIZE)
    return 0;

  std::vector<multisig::multisig_kex_msg> msgs;

  // fuzz the wire parser: raw fuzzed bytes essentially never pass multisig_kex_msg's signature check,
  // so this mostly just exercises parse_and_validate_msg() itself
  size_t pos = 0;
  while (pos <= len && msgs.size() < MAX_MSGS)
  {
    const uint8_t *line_end = (const uint8_t*)memchr(buf + pos, '\n', len - pos);
    const size_t line_len = line_end ? size_t(line_end - (buf + pos)) : len - pos;
    try
    {
      msgs.emplace_back(std::string((const char*)buf + pos, line_len));
    }
    catch (const std::exception &)
    {
    }
    pos += line_len + 1;
  }

  // fuzz the kex logic itself: a malicious co-signer can always produce a validly-signed message, so build
  // messages the same way, with real signatures but fuzzer-chosen round/pubkeys/privkey, so initialize_kex()
  // and kex_update() actually see adversarial input instead of just the parser rejecting garbage early
  size_t spos = 0;
  while (spos + 1 + 32 <= len && msgs.size() < MAX_MSGS)
  {
    const std::uint32_t round = buf[spos] % (MAX_SIGNERS + 2);
    const crypto::secret_key signing_privkey = scalar_from_bytes(buf + spos + 1);
    spos += 1 + 32;

    std::vector<crypto::public_key> msg_pubkeys;
    crypto::secret_key msg_privkey = crypto::null_skey;

    if (round == 1)
    {
      if (spos + 32 > len)
        break;
      msg_privkey = scalar_from_bytes(buf + spos);
      spos += 32;
    }
    else
    {
      if (spos + 1 > len)
        break;
      const size_t n_pubkeys = buf[spos] % (MAX_SIGNERS + 1);
      ++spos;
      if (spos + n_pubkeys * 32 > len)
        break;
      for (size_t i = 0; i < n_pubkeys; ++i)
      {
        crypto::public_key pubkey{};
        crypto::secret_key_to_public_key(scalar_from_bytes(buf + spos), pubkey);
        msg_pubkeys.push_back(pubkey);
        spos += 32;
      }
    }

    try
    {
      msgs.emplace_back(round, signing_privkey, msg_pubkeys, msg_privkey);
    }
    catch (const std::exception &)
    {
    }
  }

  if (msgs.empty())
    return 0;

  // group messages by round, and collect the signers from the round 1 messages
  std::map<std::uint32_t, std::vector<multisig::multisig_kex_msg>> msgs_by_round;
  std::vector<crypto::public_key> signers;
  for (const multisig::multisig_kex_msg &msg : msgs)
  {
    msgs_by_round[msg.get_round()].push_back(msg);
    if (msg.get_round() == 1)
      signers.push_back(msg.get_signing_pubkey());
  }
  if (signers.size() > MAX_SIGNERS)
    return 0;

  // try every threshold, since the group size is only implied by the messages
  for (std::uint32_t threshold = 1; threshold <= signers.size(); ++threshold)
  {
    // boost the round following a fresh account's first round
    try
    {
      const multisig::multisig_account fresh{base_privkey, base_common_privkey};
      fresh.get_multisig_kex_round_booster(threshold, signers.size(), msgs_by_round[1]);
    }
    catch (const std::exception &)
    {
    }

    // complete kex, with and without forced updates
    for (const bool force_update : {false, true})
    {
      try
      {
        multisig::multisig_account account{base_privkey, base_common_privkey};
        account.initialize_kex(threshold, signers, msgs_by_round[1]);

        for (const auto &round_msgs : msgs_by_round)
        {
          if (round_msgs.first <= 1)
            continue;
          account.kex_update(round_msgs.second, force_update);
        }
      }
      catch (const std::exception &)
      {
      }
    }
  }
END_SIMPLE_FUZZER()
