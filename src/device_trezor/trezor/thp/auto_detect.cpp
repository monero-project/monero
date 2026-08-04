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

#include "auto_detect.hpp"
#include "pairing.hpp"
#include "store.hpp"
#include "../exceptions.hpp"
#include "../messages/messages-thp.pb.h"
#include "../messages/messages-common.pb.h"

#include <cstring>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "device.trezor.thp"

namespace hw { namespace trezor { namespace thp {

  ProtocolAutoDetect::ProtocolAutoDetect(bool force_thp)
    : m_force_thp(force_thp) {}
  ProtocolAutoDetect::~ProtocolAutoDetect() = default;

  void ProtocolAutoDetect::configure(const ProtocolConfig &config)
  {
    m_config = config;
  }

  AllocatedChannel ProtocolAutoDetect::probe_thp(Transport &transport)
  {
    AllocatedChannel allocation;
    const AllocationResult res = allocate_channel(transport, allocation);
    if (res == AllocationResult::codec_v1_reply) {
      throw exc::TimeoutException("THP: device replied with codec_v1; protocol-v1 device");
    }
    if (res != AllocationResult::allocated) {
      throw exc::TimeoutException("THP: no channel allocation response");
    }
    return allocation;
  }

  void ProtocolAutoDetect::session_begin(Transport &transport)
  {
    if (m_actual) {
      // Already opened.
      m_actual->session_begin(transport);
      return;
    }

    // Load the persistent THP store, if a path was configured.  This
    // gives us the host static key and any prior pairing credentials.
    ThpStore store;
    bool have_store_path = !m_config.store_path.empty();
    if (have_store_path) {
      store.load_or_init(m_config.store_path);
    }

    // A Safe 7 answers the allocation in milliseconds; a protocol-v1
    // device (Model T / Safe 3 / Safe 5) either stays silent or answers
    // with a codec_v1 report, and probe_thp reports both as a timeout.
    // Only a timeout falls back to V1: libusb stalls, encoding errors
    // and security failures are real faults and must propagate, or a
    // Safe 7 with a communication problem is reported as a V1 failure.
    AllocatedChannel allocation;
    try {
      allocation = probe_thp(transport);
    } catch (const exc::TimeoutException &e) {
      if (m_force_thp) {
        // Diagnostics asked for THP explicitly (TREZOR_FORCE_THP=1):
        // surface the probe failure instead of masking it with a V1
        // session that would fail differently.
        throw;
      }
      MDEBUG("THP probe: " << e.what() << "; falling back to ProtocolV1");
      m_actual   = std::make_shared<ProtocolV1>();
      m_selected = "v1";
      return;
    }

    MINFO("THP detected on channel " << allocation.channel_id);
    auto v2 = std::make_shared<ProtocolV2>();

    // Decide on host static key + known devices to feed the handshake.
    if (have_store_path) {
      HostStaticKey host_static = store.host_static();
      if (host_static.pub == NoisePubKey{} || host_static.priv == NoisePrivKey{}) {
        noise_crypto::x25519_keypair(host_static.pub, host_static.priv);
        store.set_host_static(host_static);
        try { store.save(m_config.store_path); }
        catch (const std::exception &e) {
          MWARNING("THP store: host static key save failed: " << e.what());
        }
      }
      v2->set_host_static_key(host_static);
      v2->set_known_devices(store.known_devices());
    }

    v2->adopt_allocation(allocation);
    v2->session_begin(transport);

    // We do NOT promote `v2` to `m_actual` until the full unpaired-or-paired
    // routing decision is made and any pairing FSM has succeeded. If
    // session_begin() succeeded but pairing then throws, m_actual must
    // remain null so the next session_begin() retries from scratch
    // rather than short-circuiting on the cached partial state.
    auto promote = [&]() {
      m_actual   = v2;
      m_selected = "v2";
    };

    if (v2->trezor_state() == STATE_UNPAIRED) {
      if (!m_config.pairing_prompt) {
        // STATE_UNPAIRED with no prompt configured: fail closed; the
        // application traffic phase isn't trustworthy without pairing.
        throw exc::SecurityException(
            "THP: device is unpaired and no pairing prompt is configured "
            "(set ProtocolConfig::pairing_prompt before session_begin)");
      }
      // Make m_v2 visible to run_code_entry_pairing; reset it (and any
      // partial promotion state) on any failure so the next reconnect
      // starts clean. The credential round-trip and promote() sit in the
      // same try/catch as pairing, so a Failure from the device during
      // credential issuance cannot leave m_v2 dangling.
      m_v2 = v2;
      try {
        run_code_entry_pairing(transport);

        // Credential round-trip on TC1, only if we have a store to
        // persist the credential to. Without persistence, requesting a
        // credential is pointless (we'd lose it on shutdown).
        if (have_store_path) {
          const auto &hs = v2->host_static();
          messages::thp::ThpCredentialRequest creq;
          creq.set_host_static_public_key(reinterpret_cast<const char *>(hs.pub.data()),
                                          hs.pub.size());
          v2->write(transport, creq);

          auto resp_msg = read_handling_buttons(transport, "ThpCredentialResponse");
          auto cresp = std::dynamic_pointer_cast<messages::thp::ThpCredentialResponse>(resp_msg);
          if (!cresp) {
            throw exc::ProtocolException("THP: expected ThpCredentialResponse");
          }

          // ThpCredentialResponse delivers the device's *unmasked* static
          // pubkey alongside the credential; the InitResponse only carries
          // the masked form. We store the unmasked form so subsequent
          // sessions can recompute the mask against each session's fresh
          // device ephemeral pubkey.
          const std::string &dev_static = cresp->trezor_static_public_key();
          if (dev_static.size() != 32) {
            throw exc::ProtocolException("THP: ThpCredentialResponse: bad trezor_static_public_key size");
          }
          KnownDevice kd;
          std::memcpy(kd.trezor_static_pubkey.data(), dev_static.data(), 32);
          kd.host_static = hs;
          const std::string &cred = cresp->credential();
          kd.pairing_credential.assign(cred.begin(), cred.end());
          store.upsert_known_device(kd);
          // The device has already issued the credential, so a store that
          // cannot be written costs one re-pair on the next connect. It is
          // not a reason to fail an open the user has just worked for.
          try { store.save(m_config.store_path); }
          catch (const std::exception &e) {
            MWARNING("THP store: credential save failed, the device will "
                     "have to be paired again: " << e.what());
          }
        }

        // Always send ThpEndRequest after pairing completes, whether or not
        // a credential was requested, to transition the device out of TC1
        // (credential phase) and into the encrypted-transport state.
        // Without this, the next message lands in TC1 and is rejected as
        // "Message unrecognized in pairing context".
        thp_end_then_create_app_session(transport);
        promote();
      } catch (...) {
        m_v2.reset();
        throw;
      }
    } else if (v2->trezor_state() == STATE_PAIRED ||
               v2->trezor_state() == STATE_PAIRED_AUTOCONNECT) {
      // The device claims a paired session. Trust it only if our store
      // contains a matching unmasked static pubkey (TOFU). A device that
      // reports paired status without a store entry is either a fresh
      // device we somehow lost the credential for, or an attempted spoof.
      if (!v2->is_known_device()) {
        throw exc::SecurityException(
            "THP: device claims paired status but is not in our known-devices "
            "list. The pairing store may have been deleted or the device may "
            "have been swapped. Re-pair the device by deleting "
            ".trezor/thp_store.bin under the Monero data directory.");
      }
      // Per THP spec (states TC1 / HC0): after HandshakeCompletionResponse with
      // STATE_PAIRED, the device is in the *credential phase* (TC1) and only
      // accepts ThpCredentialRequest or ThpEndRequest. We must send ThpEndRequest
      // to transition the device to the transport state before any application
      // traffic; otherwise the device returns "Message unrecognized in pairing
      // context" for the first GetFeatures / MoneroGetAddress / etc. The
      // unpaired branch above already does this implicitly via the credential
      // round-trip; this branch reuses an existing credential and so must
      // emit a bare ThpEndRequest of its own.
      m_v2 = v2;
      try {
        thp_end_then_create_app_session(transport);
        promote();
      } catch (...) {
        m_v2.reset();
        throw;
      }
    } else {
      throw exc::ProtocolException(
          "THP: device returned unknown state byte (got " +
          std::to_string(int(v2->trezor_state())) + ")");
    }
  }

  void ProtocolAutoDetect::thp_end_then_create_app_session(Transport &transport)
  {
    if (!m_v2) {
      throw exc::ProtocolException("THP: thp_end_then_create_app_session without ProtocolV2");
    }

    // Phase 1: leave TC1 (credential phase) -> ENCRYPTED_TRANSPORT.
    // The device may emit a ButtonRequest asking the user to confirm the
    // reconnection (non-autoconnect credentials); read_handling_buttons
    // ACKs it transparently.
    {
      messages::thp::ThpEndRequest end_req;
      m_v2->write(transport, end_req);
      auto end_resp = read_handling_buttons(transport, "ThpEndResponse");
      if (!std::dynamic_pointer_cast<messages::thp::ThpEndResponse>(end_resp)) {
        throw exc::ProtocolException("THP: expected ThpEndResponse");
      }
    }

    // Phase 2: allocate a seeded application session. Without this, the
    // device routes app traffic to a SeedlessSessionContext whose .cache
    // raises InvalidSessionError, which firmware translates to
    // Failure(code=14, "Invalid session"). The host picks session_id=1;
    // the device's handle_ThpCreateNewSession (apps/base.py) then runs
    // lock_manager.unlock_device (PIN unlock if locked, may emit
    // ButtonRequest), derive_and_store_roots, and replies Success.
    constexpr uint8_t APP_SESSION_ID = 0x01;
    m_v2->set_session_id(APP_SESSION_ID);

    messages::thp::ThpCreateNewSession sess_req;
    sess_req.set_passphrase("");          // STANDARD_WALLET (no Trezor passphrase)
    sess_req.set_derive_cardano(false);
    try {
      m_v2->write(transport, sess_req);
      auto sess_resp = read_handling_buttons(transport, "Success after ThpCreateNewSession");
      if (!std::dynamic_pointer_cast<messages::common::Success>(sess_resp)) {
        throw exc::ProtocolException("THP: expected Success after ThpCreateNewSession");
      }
    } catch (...) {
      // Roll the session_id back so a retry starts from the management
      // session and doesn't accidentally reuse a half-allocated id.
      m_v2->set_session_id(0);
      throw;
    }

    MINFO("THP: app session " << int(APP_SESSION_ID)
          << " allocated on channel " << m_v2->channel_id());
  }

  std::shared_ptr<google::protobuf::Message>
  ProtocolAutoDetect::read_handling_buttons(Transport &transport, const char *expected)
  {
    if (!m_v2) {
      throw exc::ProtocolException("THP: read_handling_buttons without ProtocolV2");
    }
    std::shared_ptr<google::protobuf::Message> msg;
    messages::MessageType mt;
    while (true) {
      m_v2->read(transport, msg, &mt);
      if (std::dynamic_pointer_cast<messages::common::ButtonRequest>(msg)) {
        messages::common::ButtonAck ack;
        m_v2->write(transport, ack);
        continue;
      }
      if (auto fail = std::dynamic_pointer_cast<messages::common::Failure>(msg)) {
        // Map to the typed exception hierarchy (ActionCancelled ->
        // CancelledException etc.) exactly like the V1 read path, so
        // an on-device cancel during pairing classifies as a user
        // cancellation rather than a generic protocol fault.
        MDEBUG("THP: device returned Failure while waiting for " << expected);
        throw_failure_exception(fail.get());
      }
      return msg;
    }
  }

  void ProtocolAutoDetect::run_code_entry_pairing(Transport &transport)
  {
    if (!m_v2) {
      throw exc::ProtocolException("THP: pairing without ProtocolV2");
    }

    // 1. Send ThpPairingRequest(host_name, app_name) and wait for
    //    ThpPairingRequestApproved.
    messages::thp::ThpPairingRequest preq;
    preq.set_host_name(m_config.host_name);
    preq.set_app_name(m_config.app_name);
    m_v2->write(transport, preq);
    {
      auto msg = read_handling_buttons(transport, "ThpPairingRequestApproved");
      if (!std::dynamic_pointer_cast<messages::thp::ThpPairingRequestApproved>(msg)) {
        throw exc::ProtocolException("THP pairing: unexpected message after PairingRequest");
      }
    }

    // 2. Send ThpSelectMethod(CodeEntry).
    messages::thp::ThpSelectMethod sel;
    sel.set_selected_pairing_method(messages::thp::CodeEntry);
    m_v2->write(transport, sel);

    // 3. Receive ThpCodeEntryCommitment, send ThpCodeEntryChallenge.
    CodeEntryPairing pairing(m_v2->handshake_hash());
    {
      auto msg = read_handling_buttons(transport, "ThpCodeEntryCommitment");
      auto cmt = std::dynamic_pointer_cast<messages::thp::ThpCodeEntryCommitment>(msg);
      if (!cmt) throw exc::ProtocolException("THP pairing: expected ThpCodeEntryCommitment");
      const std::string &c = cmt->commitment();
      pairing.consume_commitment_build_challenge(
          reinterpret_cast<const uint8_t *>(c.data()), c.size());
      messages::thp::ThpCodeEntryChallenge ch;
      ch.set_challenge(reinterpret_cast<const char *>(pairing.challenge().data()),
                       pairing.challenge().size());
      m_v2->write(transport, ch);
    }

    // 4. Receive ThpCodeEntryCpaceTrezor.
    {
      auto msg = read_handling_buttons(transport, "ThpCodeEntryCpaceTrezor");
      auto ct = std::dynamic_pointer_cast<messages::thp::ThpCodeEntryCpaceTrezor>(msg);
      if (!ct) throw exc::ProtocolException("THP pairing: expected ThpCodeEntryCpaceTrezor");
      const std::string &k = ct->cpace_trezor_public_key();
      pairing.consume_cpace_trezor(reinterpret_cast<const uint8_t *>(k.data()), k.size());
    }

    // 5. Prompt the user for the code, send ThpCodeEntryCpaceHostTag.
    //    An empty return value from the prompt is treated as a user
    //    cancellation, which is a clearer error than feeding empty into
    //    the derivation and getting a CPace tag mismatch.
    const epee::wipeable_string code = m_config.pairing_prompt();
    if (code.empty()) {
      throw exc::proto::CancelledException("THP pairing: cancelled by user");
    }
    const CpaceHostTag host_tag = pairing.build_host_tag(code);
    messages::thp::ThpCodeEntryCpaceHostTag tag_msg;
    tag_msg.set_cpace_host_public_key(host_tag.cpace_host_public_key.data(),
                                      host_tag.cpace_host_public_key.size());
    tag_msg.set_tag(host_tag.tag.data(), host_tag.tag.size());
    m_v2->write(transport, tag_msg);

    // 6. Receive ThpCodeEntrySecret, verify. If the user typed a wrong
    //    code, Trezor's CPace tag check fails and the device returns a
    //    Failure rather than ThpCodeEntrySecret; read_handling_buttons
    //    throws FailureException, which the GUI surfaces as "wrong code,
    //    try again".
    {
      auto msg = read_handling_buttons(transport, "ThpCodeEntrySecret");
      auto sec = std::dynamic_pointer_cast<messages::thp::ThpCodeEntrySecret>(msg);
      if (!sec) throw exc::ProtocolException("THP pairing: expected ThpCodeEntrySecret");
      const std::string &s = sec->secret();
      if (!pairing.consume_secret(reinterpret_cast<const uint8_t *>(s.data()), s.size())) {
        throw exc::SecurityException(
            "THP pairing: code mismatch (commitment / SHA-256 verification failed)");
      }
    }

    MINFO("THP CodeEntry pairing succeeded");
  }

  void ProtocolAutoDetect::session_end(Transport &transport)
  {
    if (m_actual) {
      m_actual->session_end(transport);
      m_actual.reset();
      m_v2.reset();
      m_selected = "unknown";
    }
  }

  void ProtocolAutoDetect::write(Transport &transport,
                                 const google::protobuf::Message &req)
  {
    if (!m_actual) {
      throw exc::ProtocolException("THP auto-detect: write before session_begin");
    }
    m_actual->write(transport, req);
  }

  void ProtocolAutoDetect::read(Transport &transport,
                                std::shared_ptr<google::protobuf::Message> &msg,
                                messages::MessageType *msg_type)
  {
    if (!m_actual) {
      throw exc::ProtocolException("THP auto-detect: read before session_begin");
    }
    m_actual->read(transport, msg, msg_type);
  }

}}}
