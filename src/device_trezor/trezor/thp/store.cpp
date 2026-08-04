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

#include "store.hpp"

#include "device_trezor/trezor/exceptions.hpp"
#include "common/util.h"
#include "file_io_utils.h"
#include "memwipe.h"
#include "misc_log_ex.h"
#include "string_tools.h"

#include <boost/filesystem.hpp>
#include "crypto/crypto.h"

#include <sodium/utils.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <exception>
#include <string>

#if defined(_WIN32)
#  include <windows.h>
#else
#  include <fcntl.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "device.trezor.thp.store"

namespace hw {
namespace trezor {
namespace thp {

  namespace {

    // 1-byte tag, varint length, `length` bytes. Tag 0 ends the stream.
    constexpr uint8_t TAG_END                       = 0;
    constexpr uint8_t TAG_HOST_STATIC_PUB           = 1;
    constexpr uint8_t TAG_HOST_STATIC_PRIV          = 2;
    constexpr uint8_t TAG_KNOWN_DEVICE              = 3;
    // Inside KNOWN_DEVICE. Tag 1 held the masked static in MTHPSTR1, which
    // rotates per session; MTHPSTR2 persists the unmasked one instead.
    constexpr uint8_t KD_TAG_END                    = 0;
    constexpr uint8_t KD_TAG_TREZOR_STATIC          = 1;
    constexpr uint8_t KD_TAG_PAIRING_CREDENTIAL     = 2;
    constexpr uint8_t KD_TAG_HOST_STATIC_PUB        = 3;
    constexpr uint8_t KD_TAG_HOST_STATIC_PRIV       = 4;
    // Tag 5 was last_path, written but never read; dropped again, and older
    // stores carrying it still load because unknown tags are skipped.

    // MTHPSTR1 would load a masked static as if it were the real one, so the
    // bump forces a re-pair on upgrade.
    constexpr uint8_t MAGIC[8] = {
      'M','T','H','P','S','T','R','2'
    };

    // Every known device costs a SHA-256 and an X25519 on every connect
    // (NoiseXxInitiator::consume_init_response).
    constexpr size_t MAX_KNOWN_DEVICES              = 64;
    // An opaque device-issued blob, well under 100 bytes in current firmware.
    constexpr size_t MAX_PAIRING_CREDENTIAL_BYTES   = 1024;
    // Above a full store of maximum-size records (~72 KiB).
    constexpr size_t MAX_STORE_FILE_BYTES           = 128 * 1024;

    const char TMP_SUFFIX[] = ".tmp.";
    // Old enough that a save in flight in another process is never swept.
    constexpr std::time_t STALE_TEMP_AGE_SECONDS    = 3600;

    // HostStaticKey and KnownDevice scrub themselves (noise.hpp); the raw
    // byte buffers here do not.
    void wipe_string(std::string &s) {
      if (!s.empty()) memwipe(&s[0], s.size());
    }

    struct string_wiper {
      std::string &buf;
      ~string_wiper() { wipe_string(buf); }
    };

    // The one case ~KnownDevice cannot cover: assigning a shorter credential
    // over a longer one leaves the tail of the old buffer intact.
    void wipe_device_secrets(KnownDevice &kd) {
      if (!kd.pairing_credential.empty())
        memwipe(kd.pairing_credential.data(), kd.pairing_credential.size());
    }

    // An all-zero X25519 public key is not a valid point: crypto_scalarmult
    // fails on it, and the known-device scan runs ahead of any per-device
    // error handling, so one such record disables Trezor support outright.
    bool is_zero_key(const uint8_t *p, size_t n) {
      uint8_t acc = 0;
      for (size_t i = 0; i < n; ++i) acc |= p[i];
      return acc == 0;
    }

    // False (out empty) if the file is missing, unreadable, not regular, or
    // implausibly large for a credential store.
    bool read_store_file(const std::string &path, std::string &out)
    {
      out.clear();
      boost::system::error_code ec;
      // load_file_to_string opens without O_NONBLOCK, so a FIFO planted at
      // the store path would block the wallet forever. Symlinks are followed.
      if (!boost::filesystem::is_regular_file(path, ec)) return false;
      if (!epee::file_io_utils::load_file_to_string(path, out, MAX_STORE_FILE_BYTES)) {
        wipe_string(out);
        out.clear();
        return false;
      }
      return true;
    }

    size_t varint_size(uint64_t v) {
      size_t n = 1;
      while (v >= 0x80) { v >>= 7; ++n; }
      return n;
    }

    size_t record_size(size_t len) {
      return 1 + varint_size(len) + len;
    }

    size_t device_blob_size(const KnownDevice &kd) {
      return record_size(kd.trezor_static_pubkey.size()) +
             record_size(kd.host_static.pub.size()) +
             record_size(kd.host_static.priv.size()) +
             record_size(kd.pairing_credential.size()) +
             1;  // KD_TAG_END
    }

    void write_varint(std::string &buf, uint64_t v) {
      while (v >= 0x80) { buf.push_back(char((v & 0x7F) | 0x80)); v >>= 7; }
      buf.push_back(char(v));
    }

    // Canonical (shortest) encodings only.
    bool read_varint(const std::string &buf, size_t &off, uint64_t &v) {
      v = 0;
      int shift = 0;
      const size_t start = off;
      while (off < buf.size()) {
        const uint8_t b = uint8_t(buf[off++]);
        if (shift == 63 && (b & 0x7F) > 1) return false;
        v |= uint64_t(b & 0x7F) << shift;
        if ((b & 0x80) == 0) return b != 0 || off == start + 1;
        shift += 7;
        if (shift > 63) return false;
      }
      return false;
    }

    void write_record(std::string &buf, uint8_t tag,
                      const uint8_t *data, size_t len) {
      buf.push_back(char(tag));
      write_varint(buf, len);
      if (len) buf.append(reinterpret_cast<const char *>(data), len);
    }

    std::string serialise(const HostStaticKey &host,
                          const std::vector<KnownDevice> &known)
    {
      // Reserve exactly: a reallocation part-way through would leave an
      // unscrubbed copy of a host static private key on the heap.
      size_t total = sizeof(MAGIC) + 1 /* TAG_END */ +
                     record_size(host.pub.size()) +
                     record_size(host.priv.size());
      for (const auto &kd : known)
        total += record_size(device_blob_size(kd));

      std::string buf;
      buf.reserve(total);
      buf.append(reinterpret_cast<const char *>(MAGIC), sizeof(MAGIC));
      write_record(buf, TAG_HOST_STATIC_PUB,  host.pub.data(),  host.pub.size());
      write_record(buf, TAG_HOST_STATIC_PRIV, host.priv.data(), host.priv.size());

      std::string inner;
      string_wiper inner_wiper{inner};  // holds a host static private key
      for (const auto &kd : known) {
        inner.clear();
        inner.reserve(device_blob_size(kd));
        write_record(inner, KD_TAG_TREZOR_STATIC,
                     kd.trezor_static_pubkey.data(),
                     kd.trezor_static_pubkey.size());
        write_record(inner, KD_TAG_HOST_STATIC_PUB,
                     kd.host_static.pub.data(),  kd.host_static.pub.size());
        write_record(inner, KD_TAG_HOST_STATIC_PRIV,
                     kd.host_static.priv.data(), kd.host_static.priv.size());
        write_record(inner, KD_TAG_PAIRING_CREDENTIAL,
                     kd.pairing_credential.data(),
                     kd.pairing_credential.size());
        inner.push_back(char(KD_TAG_END));
        write_record(buf, TAG_KNOWN_DEVICE,
                     reinterpret_cast<const uint8_t *>(inner.data()), inner.size());
        wipe_string(inner);
      }
      buf.push_back(char(TAG_END));
      return buf;
    }

    void deserialise(const std::string &buf,
                     HostStaticKey &host,
                     std::vector<KnownDevice> &known)
    {
      known.clear();
      if (buf.size() < sizeof(MAGIC) ||
          std::memcmp(buf.data(), MAGIC, sizeof(MAGIC)) != 0) {
        throw exc::EncodingException("THP store: bad magic header");
      }
      size_t off = sizeof(MAGIC);

      while (off < buf.size()) {
        const uint8_t tag = uint8_t(buf[off++]);
        if (tag == TAG_END) break;
        uint64_t len = 0;
        // Subtract rather than add: len is attacker-controlled, and off + len
        // would wrap past buf.size(). off <= buf.size() holds here.
        if (!read_varint(buf, off, len) || len > buf.size() - off) {
          throw exc::EncodingException("THP store: truncated record");
        }
        if (tag == TAG_HOST_STATIC_PUB && len == host.pub.size()) {
          std::memcpy(host.pub.data(), buf.data() + off, len);
        } else if (tag == TAG_HOST_STATIC_PRIV && len == host.priv.size()) {
          std::memcpy(host.priv.data(), buf.data() + off, len);
        } else if (tag == TAG_KNOWN_DEVICE) {
          KnownDevice kd;
          size_t inner_off = 0;
          while (inner_off < len) {
            const uint8_t itag = uint8_t(buf[off + inner_off++]);
            if (itag == KD_TAG_END) break;
            uint64_t ilen = 0;
            // read_varint bounds against buf.size(), not the inner record,
            // so a hostile store could spill the varint into the next outer
            // record; reject that explicitly.
            size_t parent_off = off + inner_off;
            if (!read_varint(buf, parent_off, ilen)) {
              throw exc::EncodingException("THP store: truncated inner varint");
            }
            if (parent_off - off > len) {
              throw exc::EncodingException("THP store: inner varint past record");
            }
            inner_off = parent_off - off;
            // Again subtract rather than add: ilen is attacker-controlled.
            if (ilen > len - inner_off) {
              throw exc::EncodingException("THP store: inner record overflow");
            }
            const uint8_t *idata =
              reinterpret_cast<const uint8_t *>(buf.data()) + off + inner_off;
            if (itag == KD_TAG_TREZOR_STATIC && ilen == kd.trezor_static_pubkey.size())
              std::memcpy(kd.trezor_static_pubkey.data(), idata, ilen);
            else if (itag == KD_TAG_HOST_STATIC_PUB  && ilen == kd.host_static.pub.size())
              std::memcpy(kd.host_static.pub.data(),  idata, ilen);
            else if (itag == KD_TAG_HOST_STATIC_PRIV && ilen == kd.host_static.priv.size())
              std::memcpy(kd.host_static.priv.data(), idata, ilen);
            else if (itag == KD_TAG_PAIRING_CREDENTIAL) {
              if (ilen > MAX_PAIRING_CREDENTIAL_BYTES) {
                throw exc::EncodingException("THP store: oversized pairing credential");
              }
              kd.pairing_credential.assign(idata, idata + ilen);
            }
            // else: forward-compat, skip unknown tag
            inner_off += ilen;
          }
          if (is_zero_key(kd.trezor_static_pubkey.data(),
                          kd.trezor_static_pubkey.size())) {
            MWARNING("THP store: dropping a device record with an all-zero static key");
          } else if (known.size() >= MAX_KNOWN_DEVICES) {
            throw exc::EncodingException("THP store: too many device records");
          } else {
            known.push_back(std::move(kd));
          }
        }
        // else: forward-compat, skip unknown tag
        off += len;
      }
    }

    // Fold devices present on disk but absent from `mine` into `mine`, so two
    // wallet processes sharing the store do not clobber each other's
    // credentials.  The in-memory entry wins on a conflict; we may have just
    // re-paired it.  The on-disk top-level host static key is deliberately
    // not adopted: each KnownDevice carries the keypair it was paired with.
    void merge_devices_from_disk(const std::string &path,
                                 std::vector<KnownDevice> &mine)
    {
      std::string image;
      string_wiper image_wiper{image};
      if (!read_store_file(path, image)) return;

      HostStaticKey            disk_host{};
      std::vector<KnownDevice> disk_known;
      try {
        deserialise(image, disk_host, disk_known);
      } catch (const std::exception &) {
        disk_known.clear();  // corrupt on-disk state; ours wins wholesale
      }
      for (const auto &dk : disk_known) {
        if (mine.size() >= MAX_KNOWN_DEVICES) break;
        bool known = false;
        for (const auto &mk : mine) {
          if (sodium_memcmp(mk.trezor_static_pubkey.data(),
                            dk.trezor_static_pubkey.data(),
                            dk.trezor_static_pubkey.size()) == 0) {
            known = true;
            break;
          }
        }
        if (!known) mine.push_back(dk);
      }
    }

    // Only a directory we created ourselves is tightened to owner-only, so
    // this can never lock down one the wallet does not own.
    void ensure_store_directory(const boost::filesystem::path &dir)
    {
      namespace fs = boost::filesystem;
      if (dir.empty()) return;

      boost::system::error_code ec;
      const bool created = fs::create_directories(dir, ec);
      if (ec && !fs::is_directory(dir, ec)) {
        throw exc::CommunicationException("THP store: cannot create directory " +
                                          dir.string() + ": " + ec.message());
      }
#if !defined(_WIN32)
      // create_directories() honours the umask, so tighten afterwards.
      if (created && ::chmod(dir.string().c_str(), S_IRWXU) != 0) {
        MWARNING("THP store: cannot restrict " << dir.string()
                 << " to its owner: " << std::strerror(errno));
      }
#else
      // A new directory inherits the parent's ACL, which is owner-only under
      // the per-user data directory default_path() requires and is not under
      // a machine-wide one. Hence the requirement, rather than a DACL here.
      (void)created;
#endif
    }

    // Unguessable, so a temp file planted in advance is out of reach.
    std::string unique_temp_suffix()
    {
      uint8_t r[8];
      crypto::generate_random_bytes_thread_safe(sizeof(r), r);
      return epee::string_tools::buff_to_hex_nodelimer(
        std::string(reinterpret_cast<const char *>(r), sizeof(r)));
    }

    // A crash between creating the temp file and renaming it strands a copy
    // of the host static private key under a name nothing else removes.
    void remove_stale_temp_files(const boost::filesystem::path &store)
    {
      namespace fs = boost::filesystem;
      const std::string prefix = store.filename().string() + TMP_SUFFIX;
      const std::time_t cutoff = std::time(nullptr) - STALE_TEMP_AGE_SECONDS;

      boost::system::error_code ec;
      fs::directory_iterator it(store.parent_path(), ec), end;
      for (; !ec && it != end; it.increment(ec)) {
        const std::string name = it->path().filename().string();
        if (name.size() <= prefix.size() ||
            name.compare(0, prefix.size(), prefix) != 0) continue;
        boost::system::error_code stat_ec;
        const std::time_t mtime = fs::last_write_time(it->path(), stat_ec);
        if (stat_ec || mtime > cutoff) continue;
        boost::system::error_code ignored;
        fs::remove(it->path(), ignored);
      }
    }

#if defined(_WIN32)
    // epee's writer does not reach stable storage on its own.
    void flush_to_disk(const std::string &path)
    {
      std::wstring wide;
      try {
        wide = epee::string_tools::utf8_to_utf16(path);
      } catch (const std::exception &e) {
        MWARNING("THP store: cannot flush " << path << ": " << e.what());
        return;
      }
      HANDLE fh = ::CreateFileW(wide.c_str(), GENERIC_WRITE, 0, nullptr,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
      if (fh == INVALID_HANDLE_VALUE) {
        MWARNING("THP store: cannot reopen " << path << " to flush (error "
                 << ::GetLastError() << ")");
        return;
      }
      if (!::FlushFileBuffers(fh))
        MWARNING("THP store: FlushFileBuffers failed: " << ::GetLastError());
      ::CloseHandle(fh);
    }
#endif

  } // anon

  std::string ThpStore::default_path(const std::string &user_data_dir)
  {
    boost::filesystem::path p(user_data_dir);
    p /= ".trezor";
    p /= "thp_store.bin";
    return p.string();
  }

  void ThpStore::load_or_init(const std::string &path)
  {
    auto reset_empty = [this]() {
      m_host_static = HostStaticKey{};
      m_known_devices.clear();
    };

    remove_stale_temp_files(boost::filesystem::path(path));

    std::string image;
    string_wiper image_wiper{image};  // the raw image holds the host static private key
    if (!read_store_file(path, image)) {
      if (boost::filesystem::exists(path))
        MWARNING("THP store: cannot read " << path << "; treating as empty");
      reset_empty();
      return;
    }
    try {
      deserialise(image, m_host_static, m_known_devices);
    } catch (const std::exception &e) {
      // Corrupt or older-format (e.g. MTHPSTR1): re-pair, don't brick open.
      MWARNING("THP store: failed to parse " << path << " (" << e.what()
               << "); discarding and re-initialising. The Trezor will "
                  "need to be re-paired.");
      reset_empty();
    }
  }

  void ThpStore::save(const std::string &path) const
  {
    namespace fs = boost::filesystem;
    const fs::path p(path);
    ensure_store_directory(p.parent_path());

    // Fold in whatever another process persisted since we loaded.
    std::vector<KnownDevice> devices = m_known_devices;
    merge_devices_from_disk(path, devices);

    std::string image = serialise(m_host_static, devices);
    string_wiper image_wiper{image};

    // Unique temp file in the same directory, renamed over the destination.
    const fs::path tmp = p.parent_path() /
                         (p.filename().string() + TMP_SUFFIX + unique_temp_suffix());

#if !defined(_WIN32)
    // private_file::create() with O_EXCL gives an exclusively-created,
    // owner-only, flock-held file and unlinks it from its destructor, so no
    // path out of here strands the host static private key in a temp file.
    tools::private_file file = tools::private_file::create(tmp.string(), O_EXCL);
    if (!file.handle()) {
      throw exc::CommunicationException("THP store: cannot create " + tmp.string());
    }
    std::FILE *fh = file.handle();
    if (std::fwrite(image.data(), 1, image.size(), fh) != image.size() ||
        std::fflush(fh) != 0) {
      throw exc::CommunicationException("THP store: cannot write " + tmp.string());
    }
    // private_file does not reach stable storage on its own; without this
    // the rename can expose a zero-length store after a crash.
    if (::fsync(::fileno(fh)) != 0 && errno != EINVAL /* tmpfs */) {
      MWARNING("THP store: fsync failed: " << std::strerror(errno));
    }
    const std::error_code ec = tools::replace_file(tmp.string(), path);
    if (ec) {
      throw exc::CommunicationException("THP store: cannot replace " + path +
                                        ": " + ec.message());
    }
    // file's destructor now unlinks a name the rename already consumed: a
    // no-op here, the cleanup path on every failure above.
#else
    // tools::private_file is unusable here: it opens with
    // FILE_FLAG_DELETE_ON_CLOSE, so the file cannot survive the close that
    // has to precede the rename.
    if (!epee::file_io_utils::save_string_to_file(tmp.string(), image)) {
      boost::system::error_code ignored;
      fs::remove(tmp, ignored);
      throw exc::CommunicationException("THP store: cannot write " + tmp.string());
    }
    flush_to_disk(tmp.string());
    const std::error_code ec = tools::replace_file(tmp.string(), path);
    if (ec) {
      boost::system::error_code ignored;
      fs::remove(tmp, ignored);
      throw exc::CommunicationException("THP store: cannot replace " + path +
                                        ": " + ec.message());
    }
#endif

#if !defined(_WIN32)
    // The rename is a directory-metadata update, so a credential issued
    // moments ago only survives a crash if the parent is synced too.
    if (!p.parent_path().empty()) {
      const int dfd = ::open(p.parent_path().string().c_str(),
                             O_RDONLY | O_DIRECTORY | O_CLOEXEC);
      if (dfd >= 0) {
        ::fsync(dfd);
        ::close(dfd);
      }
    }
#endif
  }

  void ThpStore::upsert_known_device(const KnownDevice &kd)
  {
    if (is_zero_key(kd.trezor_static_pubkey.data(),
                    kd.trezor_static_pubkey.size())) {
      MWARNING("THP store: refusing to store a device with an all-zero static key");
      return;
    }
    for (auto &existing : m_known_devices) {
      if (sodium_memcmp(existing.trezor_static_pubkey.data(),
                        kd.trezor_static_pubkey.data(),
                        kd.trezor_static_pubkey.size()) == 0) {
        if (&existing == &kd) return;
        wipe_device_secrets(existing);
        existing = kd;
        return;
      }
    }
    if (m_known_devices.size() >= MAX_KNOWN_DEVICES) {
      // Drop the oldest rather than refuse the new one.
      wipe_device_secrets(m_known_devices.front());
      m_known_devices.erase(m_known_devices.begin());
    }
    m_known_devices.push_back(kd);
  }

}
}
}
