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

#ifndef MONERO_TREZOR_THP_STORE_H
#define MONERO_TREZOR_THP_STORE_H

#include "noise.hpp"

#include <string>
#include <vector>

namespace hw {
namespace trezor {
namespace thp {

  // Disk-backed cache of the host's static X25519 keypair and any THP
  // pairing credentials previously issued by Trezor devices, held in a small
  // tag-length-value binary file.
  //
  // Layout (tag-prefixed records; unknown tags are skipped on load so the
  // format can grow - see store.cpp for the tag constants):
  //   tag 1: bytes host_static_pub  (32)
  //   tag 2: bytes host_static_priv (32)
  //   tag 3: repeated KnownDevice blob, itself tag-prefixed:
  //     inner tag 1: bytes trezor_static_pubkey (32)
  //     inner tag 2: bytes pairing_credential (variable)
  //     inner tag 3: bytes host_static_pub  (32)
  //     inner tag 4: bytes host_static_priv (32)
  //
  // The host static private key is written to disk in plaintext, matching
  // the existing Trezor v1 flow, which persists hardware-wallet state
  // without additional encryption. That only holds up while the file lives
  // somewhere no other local account can read, which is why default_path()
  // requires a per-user directory.
  class ThpStore {
  public:
    ThpStore() = default;

    // Loads the file from `path`. A missing, unreadable, or malformed file
    // resets the store to the empty initial state instead of throwing: the
    // worst case is that the device is re-paired.
    void load_or_init(const std::string &path);

    // Atomically writes the current state to `path` (unique temp file in the
    // same directory, flushed to stable storage, renamed over the
    // destination). Devices present on disk but unknown to this instance are
    // folded in rather than dropped, so a second wallet process does not
    // usually lose the credential this one just persisted. There is no
    // locking, so two saves that overlap can still lose one entry; the cost
    // is one re-pair. On POSIX the file is created owner-only; on Windows it
    // inherits the owner-only ACL of the per-user directory default_path()
    // requires.
    void save(const std::string &path) const;

    // A store that never had a key (fresh init, or a file written before
    // the key existed) reports an all-zero keypair; callers detect this
    // and generate + set_host_static() a fresh one.
    const HostStaticKey            &host_static()    const { return m_host_static; }
    const std::vector<KnownDevice> &known_devices()  const { return m_known_devices; }

    // Replace or insert a known device, keyed on its Trezor static pubkey.
    // An all-zero pubkey is refused: it is not a valid X25519 point and
    // would make every subsequent handshake fail. At most MAX_KNOWN_DEVICES
    // entries are kept, oldest dropped first.
    void upsert_known_device(const KnownDevice &kd);

    // Set host static key (called once when an empty store is initialised).
    void set_host_static(const HostStaticKey &key) { m_host_static = key; }

    // Location of the store inside `user_data_dir`.
    //
    // The caller MUST pass a directory private to the current user and
    // specific to the network in use. The file holds the host static private
    // key and every pairing credential in the clear, and save() merges
    // whatever it finds on disk into the caller's state, so a machine-wide
    // directory lets local accounts read and cross-pollinate each other's
    // credentials. On Windows tools::get_default_data_dir() resolves to
    // CSIDL_COMMON_APPDATA (C:\ProgramData\bitmonero) and is NOT suitable.
    static std::string default_path(const std::string &user_data_dir);

  private:
    // Both types scrub themselves on destruction (noise.hpp), which is what
    // makes vector growth and the corrupt-file reset paths safe here.
    HostStaticKey              m_host_static{};
    std::vector<KnownDevice>   m_known_devices;
  };

}
}
}

#endif // MONERO_TREZOR_THP_STORE_H
