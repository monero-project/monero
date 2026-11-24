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
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of
//    conditions and the following disclaimer in the documentation and/or other materials
//    provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific prior
//    written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
// TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Fuzzes the MMS (Monero Messaging System) message store: the signer-config blob parser
// (binary_archive of authorized signers arriving from another participant), the auto-config
// token parser, and the full MMS state-file chain (file_data parse -> chacha20 decrypt with
// the wallet's view-key-derived key -> binary_archive parse of the message store), using the
// key-aware-writer pattern so the decrypted plaintext is attacker-controlled.

#include <cstdio>
#include <cstring>
#include <string>

#include "include_base_utils.h"
#include "crypto/crypto.h"
#include "file_io_utils.h"
#include "wallet/message_store.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "fuzzer.h"

#define FUZZ_CHECK(cond) \
  do { if (!(cond)) { fprintf(stderr, "check failed: %s (line %d)\n", #cond, __LINE__); abort(); } } while (0)

BEGIN_INIT_SIMPLE_FUZZER()
END_INIT_SIMPLE_FUZZER()

BEGIN_SIMPLE_FUZZER()
  FuzzedDataProvider fdp(buf, len);

  // Fixed deterministic wallet state; the MMS file key derives from the view secret key.
  mms::multisig_wallet_state state{};
  state.nettype = cryptonote::MAINNET;
  memset(state.view_secret_key.data, 0xcd, sizeof(state.view_secret_key.data));
  state.multisig = true;
  state.multisig_is_ready = true;
  state.multisig_kex_is_done = true;
  state.mms_file = "/tmp/mms-fuzz.mms";

  // Configuration from the back.
  const std::size_t n_messages = fdp.ConsumeIntegralInRange<std::size_t>(0, 3);
  const unsigned blob_len = fdp.ConsumeIntegralInRange<unsigned>(0, 256);

  const std::string blob = fdp.ConsumeBytesAsString(
      std::min<std::size_t>(blob_len, fdp.remaining_bytes()));

  // 1) Signer-config blob: binary_archive of a vector of authorized signers, label
  //    sanitization, and matching against the resident signers.
  {
    mms::message_store ms(nullptr);
    ms.init(state, "me", "own-transport", 2, 2);
    try { ms.process_signer_config(state, blob); } catch (...) {}
    std::string adjusted;
    try { ms.check_auto_config_token(blob.substr(0, std::min<std::size_t>(blob.size(), 24)), adjusted); }
    catch (...) {}
  }

  // 2) MMS state-file chain: build a store with fuzz messages, write it (serialize +
  //    encrypt), then read back both a fuzz-mutated copy (bad-structure / decrypt-to-garbage
  //    paths) and the intact file (full message_store parse).
  {
    mms::message_store ms(nullptr);
    ms.init(state, "me", "own-transport", 2, 2);
    for (std::size_t i = 0; i < n_messages; ++i)
    {
      const std::string content = fdp.ConsumeBytesAsString(
          std::min<std::size_t>(fdp.remaining_bytes() / 2, 128));
      const mms::message_type type = (mms::message_type)fdp.ConsumeIntegralInRange<unsigned>(0, 6);
      const mms::message_direction dir = (mms::message_direction)fdp.ConsumeIntegralInRange<unsigned>(0, 1);
      try { ms.add_message(state, i % 2, type, dir, content); } catch (...) {}
    }
    try { ms.write_to_file(state, state.mms_file); } catch (...) {}

    std::string file_buf;
    if (epee::file_io_utils::load_file_to_string(state.mms_file, file_buf) && !file_buf.empty())
    {
      file_buf[fdp.ConsumeIntegralInRange<std::size_t>(0, file_buf.size() - 1)] ^= 0xff;
      epee::file_io_utils::save_string_to_file("/tmp/mms-fuzz-mut.mms", file_buf);
      mms::message_store ms_mut(nullptr);
      ms_mut.init(state, "me", "own-transport", 2, 2);
      try { ms_mut.read_from_file(state, "/tmp/mms-fuzz-mut.mms"); } catch (...) {}
    }

    try { ms.read_from_file(state, state.mms_file); } catch (...) { FUZZ_CHECK(false); } // self-written file must parse back
  }
END_SIMPLE_FUZZER()
