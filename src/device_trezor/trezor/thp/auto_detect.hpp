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

#ifndef MONERO_TREZOR_THP_AUTO_DETECT_H
#define MONERO_TREZOR_THP_AUTO_DETECT_H

#include "device_trezor/trezor/transport.hpp"
#include "noise.hpp"
#include "wipeable_string.h"
#include "protocol_v2.hpp"

#include <functional>
#include <memory>
#include <string>

namespace hw { namespace trezor { namespace thp {

  // Pairing UI hook: invoked by ProtocolAutoDetect when a fresh THP
  // device requires CodeEntry pairing.  The host (GUI) renders a modal
  // asking the user for the 6-digit code shown on the Trezor; the
  // returned string is the code.  Throws on user cancel, which aborts
  // the pairing flow and closes the channel.
  using PairingPromptHook = std::function<epee::wipeable_string()>;

  // Configuration plumbed into ProtocolAutoDetect from the device-glue
  // layer.  All fields are optional except `pairing_prompt`, which must
  // be non-null on first-time pairing.
  struct ProtocolConfig {
    std::string         host_name      = "Monero Wallet";
    std::string         app_name       = "monero-wallet-gui";
    PairingPromptHook   pairing_prompt;
    // Path to the host static key + credentials store (created if absent).
    // If empty, the host static key is generated fresh per session and not
    // persisted (TOFU + no recognition of returning devices).
    std::string         store_path;
  };

  // ProtocolAutoDetect defers protocol selection (v1 vs v2) until the
  // first session_begin.  It probes the device by attempting a THP
  // ChannelAllocationRequest.  If that succeeds, it transitions to
  // ProtocolV2 (running the handshake, persisted-key lookup, and, on
  // the unpaired path, the CodeEntry pairing FSM).  If the probe times
  // out or returns a non-THP response, it falls back to ProtocolV1
  // transparently.
  //
  // Lifecycle and threading match the existing Protocol contract: a
  // single instance is bound to one open transport for the lifetime
  // of the session.
  class ProtocolAutoDetect : public Protocol {
  public:
    // force_thp: diagnostics knob (TREZOR_FORCE_THP=1).  The channel-
    // allocation probe still runs, since THP always needs one, but a
    // probe timeout propagates instead of silently selecting V1, so a
    // misbehaving THP device is reported as such.  The full THP
    // bring-up (pairing, credential store, app session) is unchanged.
    explicit ProtocolAutoDetect(bool force_thp = false);
    ~ProtocolAutoDetect() override;

    // Caller wires up the GUI prompt and the persistent store path
    // before session_begin.  May be called repeatedly until then.
    void configure(const ProtocolConfig &config);

    void session_begin(Transport &transport) override;
    void session_end  (Transport &transport) override;
    void write(Transport &transport,
               const google::protobuf::Message &req) override;
    void read (Transport &transport,
               std::shared_ptr<google::protobuf::Message> &msg,
               messages::MessageType *msg_type = nullptr) override;

    // After session_begin, returns "v1" or "v2" ("unknown" before).
    // Used by tests and diagnostics; not part of the Protocol contract.
    // Compare the content (strcmp / std::string), not the pointer.
    const char *selected_protocol() const { return m_selected; }


  private:
    // Attempt a THP channel-allocation handshake; returns the
    // allocated channel on success, throws on any failure.  The
    // caller swallows the throw and falls back to v1.
    AllocatedChannel probe_thp(Transport &transport);

    // Drive the pairing FSM via the configured prompt.  Sends/receives
    // pairing messages via `m_v2`'s ProtocolV2 (which already has the
    // post-handshake transport cipherstates set up).
    void run_code_entry_pairing(Transport &transport);

    // Read the next encrypted message from `m_v2`, transparently ACKing
    // any common.ButtonRequest and surfacing common.Failure as a typed
    // FailureException. Returns the first non-button, non-failure
    // message. `expected` names the message we're waiting for, used in
    // the diagnostic if a Failure arrives without its own message.
    std::shared_ptr<google::protobuf::Message>
    read_handling_buttons(Transport &transport, const char *expected);

    // Run the post-handshake bring-up sequence that promotes the channel
    // from TC1 (credential phase) to a seeded application session ready
    // for Monero traffic:
    //
    //   1. Send ThpEndRequest on session 0 -> read ThpEndResponse (handles
    //      ButtonRequest if the firmware prompts the user to confirm the
    //      reconnection for non-autoconnect credentials).
    //   2. Bump m_v2->set_session_id(1) so the next encrypted-transport
    //      plaintext header carries session_id=1.
    //   3. Send ThpCreateNewSession(passphrase="", derive_cardano=false)
    //      -> read Success (handles ButtonRequest from the firmware's
    //      lock_manager.unlock_device PIN-prompt step inside
    //      handle_ThpCreateNewSession).
    //
    // After this, ProtocolV2::write embeds session_id=1 in every encrypted
    // plaintext header so the device routes the message to the seeded
    // SessionContext for that session_id.
    void thp_end_then_create_app_session(Transport &transport);

    ProtocolConfig                        m_config;
    std::shared_ptr<Protocol>             m_actual;     // v1 or v2 once selected
    std::shared_ptr<ProtocolV2>           m_v2;         // shortcut typed alias
    const char                           *m_selected = "unknown";
    bool                                  m_force_thp = false;
  };

}}}

#endif // MONERO_TREZOR_THP_AUTO_DETECT_H
