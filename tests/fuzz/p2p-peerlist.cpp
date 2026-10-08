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

// Fuzzes the P2P peer list: peer lists received in TIMED_SYNC/HANDSHAKE responses,
// peerlist_manager state transitions and the p2pstate storage round trip.
// No sockets are opened; node_server is not involved.

#include <algorithm>
#include <cstdio>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <boost/asio/ip/address_v6.hpp>

#include "include_base_utils.h"
#include "common/pruning.h"
#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "net/i2p_address.h"
#include "net/tor_address.h"
#include "p2p/net_peerlist.h"
#include "p2p/p2p_protocol_defs.h"
#include "storages/portable_storage_template_helper.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "fuzzer.h"

#define FUZZ_CHECK(cond) \
  do { if (!(cond)) { fprintf(stderr, "check failed: %s (line %d)\n", #cond, __LINE__); abort(); } } while (0)

using epee::net_utils::ipv4_network_address;
using epee::net_utils::ipv6_network_address;
using epee::net_utils::network_address;
using epee::net_utils::zone;
using nodetool::anchor_peerlist_entry;
using nodetool::peerlist_entry;
using nodetool::peerlist_manager;
using timed_sync_response = nodetool::COMMAND_TIMED_SYNC_T<cryptonote::CORE_SYNC_DATA>::response;
using handshake_response = nodetool::COMMAND_HANDSHAKE_T<cryptonote::CORE_SYNC_DATA>::response;

static constexpr size_t kMaxOps = 32;
static constexpr size_t kMaxPeersPerMessage = 64;
static constexpr size_t kMaxBulkWhite = P2P_LOCAL_WHITE_PEERLIST_LIMIT + 100;
static constexpr size_t kMaxBulkGray = P2P_LOCAL_GRAY_PEERLIST_LIMIT + 100;

// Same limits levin applies to P2P messages.
static const epee::serialization::portable_storage::limits_t kLevinLimits = {8192, 16384, 16384};

static const char *const kOnionHosts[] = {
  "vww6ybal4bd7szmgncyruucpgfkqahzddi37ktceo3ah7ngmcopnpyyd.onion",
  "zpv4fa3szgel7vf6jdjeugizdclq2vzkelscs2bhbgnlldzzggcen3ad.onion",
};
static const char kBase32[] = "abcdefghijklmnopqrstuvwxyz234567";
static const char kHostChars[] = "abcdefghijklmnopqrstuvwxyz0123456789";
static const zone kZones[] = {zone::public_, zone::i2p, zone::tor};
static std::vector<boost::asio::ip::address_v6> ipv6_pool;

// `z` restricts the address to one zone; zone::invalid allows any.
static network_address consume_address(FuzzedDataProvider &fdp, zone z = zone::invalid)
{
  const uint16_t port = fdp.ConsumeBool() ? 18080 : fdp.ConsumeIntegral<uint16_t>();
  int kind;
  switch (z)
  {
    case zone::public_: kind = fdp.ConsumeIntegralInRange<int>(0, 4); break;
    case zone::tor: kind = 5; break;
    case zone::i2p: kind = fdp.ConsumeIntegralInRange<int>(6, 8); break;
    default: kind = fdp.ConsumeIntegralInRange<int>(0, 8); break;
  }
  switch (kind)
  {
    case 0:
      // Small public pool so the same hosts and addresses recur.
      return ipv4_network_address{MAKE_IP(1, 2, 3, fdp.ConsumeIntegralInRange<uint32_t>(1, 8)), port};
    case 1:
    {
      static const uint32_t special[] = {MAKE_IP(127, 0, 0, 1), MAKE_IP(10, 0, 0, 1), MAKE_IP(192, 168, 1, 1),
          MAKE_IP(172, 16, 0, 1), MAKE_IP(169, 254, 0, 1), 0, 0xffffffff};
      return ipv4_network_address{fdp.PickValueInArray(special), port};
    }
    case 2:
      return ipv4_network_address{fdp.ConsumeIntegral<uint32_t>(), port};
    case 3:
      return ipv6_network_address{ipv6_pool[fdp.ConsumeIntegralInRange<size_t>(0, ipv6_pool.size() - 1)], port};
    case 4:
    {
      boost::asio::ip::address_v6::bytes_type bytes{};
      fdp.ConsumeData(bytes.data(), bytes.size());
      return ipv6_network_address{boost::asio::ip::address_v6(bytes), port};
    }
    case 5:
    {
      if (fdp.ConsumeIntegralInRange<int>(0, 7) == 0)
        return net::tor_address::unknown();
      const auto tor = net::tor_address::make(fdp.PickValueInArray(kOnionHosts), port);
      FUZZ_CHECK(tor);
      return *tor;
    }
    case 6:
    {
      const uint8_t seed = fdp.ConsumeIntegral<uint8_t>();
      std::string host;
      for (size_t i = 0; i < net::b32_length; ++i)
        host.push_back(kBase32[(seed * 31u + i * 7u) % (sizeof(kBase32) - 1)]);
      const auto i2p = net::i2p_address::make(host + net::tld_b32);
      FUZZ_CHECK(i2p);
      return *i2p;
    }
    case 7:
    {
      std::string host;
      const size_t length = fdp.ConsumeIntegralInRange<size_t>(1, 12);
      for (size_t i = 0; i < length; ++i)
        host.push_back(kHostChars[fdp.ConsumeIntegralInRange<size_t>(0, sizeof(kHostChars) - 2)]);
      const auto i2p = net::i2p_address::make(host + net::tld_i2p);
      FUZZ_CHECK(i2p);
      return *i2p;
    }
    default:
      return net::i2p_address::unknown();
  }
}

static uint32_t consume_pruning_seed(FuzzedDataProvider &fdp)
{
  switch (fdp.ConsumeIntegralInRange<int>(0, 2))
  {
    case 0:
      return 0;
    case 1:
      return tools::make_pruning_seed(fdp.ConsumeIntegralInRange<uint32_t>(1, 1u << CRYPTONOTE_PRUNING_LOG_STRIPES), CRYPTONOTE_PRUNING_LOG_STRIPES);
    default:
      return fdp.ConsumeIntegral<uint32_t>();
  }
}

static peerlist_entry consume_entry(FuzzedDataProvider &fdp, zone z = zone::invalid)
{
  peerlist_entry pe{};
  pe.adr = consume_address(fdp, z);
  pe.id = fdp.ConsumeBool() ? fdp.ConsumeIntegralInRange<uint64_t>(0, 7) : fdp.ConsumeIntegral<uint64_t>();
  pe.last_seen = fdp.ConsumeIntegral<int64_t>();
  pe.pruning_seed = consume_pruning_seed(fdp);
  pe.rpc_port = fdp.ConsumeBool() ? 0 : fdp.ConsumeIntegral<uint16_t>();
  pe.rpc_credits_per_hash = fdp.ConsumeBool() ? 0 : fdp.ConsumeIntegral<uint32_t>();
  return pe;
}

static void touch_address(FuzzedDataProvider &fdp, const network_address &adr)
{
  adr.str();
  adr.host_str();
  adr.is_blockable();
  epee::net_utils::zone_to_string(adr.get_zone());
  epee::net_utils::get_ipv4_mapped_address(adr);
  if (adr.get_type_id() == ipv6_network_address::get_type_id())
  {
    const auto ip = adr.as<ipv6_network_address>().ip();
    epee::net_utils::should_group_ipv6_by_prefix(ip);
    epee::net_utils::get_ipv6_subnet_address(ip, fdp.ConsumeIntegralInRange<size_t>(0, 136));
  }
}

// Mirrors node_server::sanitize_peerlist (without the forbidden-IPv6 check, which is private).
static void sanitize(std::vector<peerlist_entry> &peers)
{
  const uint32_t min_seed = tools::make_pruning_seed(1, CRYPTONOTE_PRUNING_LOG_STRIPES);
  const uint32_t max_seed = tools::make_pruning_seed(1u << CRYPTONOTE_PRUNING_LOG_STRIPES, CRYPTONOTE_PRUNING_LOG_STRIPES);
  peers.erase(std::remove_if(peers.begin(), peers.end(), [&](const peerlist_entry &pe) {
    if (pe.adr.is_loopback() || pe.adr.is_local())
      return true;
    if (pe.adr.get_type_id() == ipv4_network_address::get_type_id())
    {
      const auto &ipv4 = pe.adr.as<ipv4_network_address>();
      if (ipv4.ip() == 0 || ipv4.port() == pe.rpc_port)
        return true;
    }
    return pe.pruning_seed && (pe.pruning_seed < min_seed || pe.pruning_seed > max_seed);
  }), peers.end());
  for (auto &pe : peers)
    pe.last_seen = 0;
}

static bool same_entry(const peerlist_entry &a, const peerlist_entry &b)
{
  return a.adr == b.adr && a.id == b.id && a.last_seen == b.last_seen && a.pruning_seed == b.pruning_seed &&
    a.rpc_port == b.rpc_port && a.rpc_credits_per_hash == b.rpc_credits_per_hash;
}

template<typename T>
static void sort_by_address(std::vector<T> &entries)
{
  std::sort(entries.begin(), entries.end(), [](const T &a, const T &b) { return a.adr < b.adr; });
}

// is_same_host() semantics: IPv4-mapped IPv6 addresses count as their IPv4 host.
static std::pair<int, std::string> host_key(const network_address &adr)
{
  const auto mapped = epee::net_utils::get_ipv4_mapped_address(adr);
  if (mapped)
    return {int(ipv4_network_address::get_type_id()), mapped->host_str()};
  return {int(adr.get_type_id()), adr.host_str()};
}

static void check_invariants(peerlist_manager &pl, bool allow_local)
{
  std::vector<peerlist_entry> gray, white;
  pl.get_peerlist(gray, white);
  FUZZ_CHECK(white.size() <= P2P_LOCAL_WHITE_PEERLIST_LIMIT);
  FUZZ_CHECK(gray.size() <= P2P_LOCAL_GRAY_PEERLIST_LIMIT);
  FUZZ_CHECK(white.size() == pl.get_white_peers_count());
  FUZZ_CHECK(gray.size() == pl.get_gray_peers_count());

  std::set<network_address> white_addresses;
  std::set<std::pair<int, std::string>> white_hosts;
  for (const auto &pe : white)
  {
    FUZZ_CHECK(!pe.adr.is_loopback());
    FUZZ_CHECK(allow_local || !pe.adr.is_local());
    white_addresses.insert(pe.adr);
    FUZZ_CHECK(white_hosts.insert(host_key(pe.adr)).second);
  }
  for (const auto &pe : gray)
  {
    FUZZ_CHECK(!pe.adr.is_loopback());
    FUZZ_CHECK(allow_local || !pe.adr.is_local());
    FUZZ_CHECK(white_addresses.count(pe.adr) == 0);
  }
}

static void receive_peerlist(FuzzedDataProvider &fdp, peerlist_manager &pl)
{
  const zone remote_zone = fdp.PickValueInArray(kZones);
  // Occasionally send peers from other zones, which node_server rejects as a whole.
  const bool mixed_zones = fdp.ConsumeIntegralInRange<int>(0, 7) == 0;
  std::vector<peerlist_entry> sent;
  const size_t n_peers = fdp.ConsumeIntegralInRange<size_t>(0, kMaxPeersPerMessage);
  for (size_t i = 0; i < n_peers; ++i)
    sent.push_back(consume_entry(fdp, mixed_zones ? zone::invalid : remote_zone));

  cryptonote::CORE_SYNC_DATA sync{};
  sync.current_height = fdp.ConsumeIntegral<uint64_t>();
  sync.cumulative_difficulty = fdp.ConsumeIntegral<uint64_t>();
  sync.top_version = fdp.ConsumeIntegral<uint8_t>();
  sync.pruning_seed = consume_pruning_seed(fdp);

  const bool handshake = fdp.ConsumeBool();
  epee::byte_slice slice;
  if (handshake)
  {
    handshake_response msg{};
    msg.node_data.peer_id = fdp.ConsumeIntegral<uint64_t>();
    msg.node_data.my_port = fdp.ConsumeIntegral<uint32_t>();
    msg.node_data.rpc_port = fdp.ConsumeIntegral<uint16_t>();
    msg.node_data.support_flags = fdp.ConsumeIntegral<uint32_t>();
    msg.payload_data = sync;
    msg.local_peerlist_new = sent;
    FUZZ_CHECK(epee::serialization::store_t_to_binary(msg, slice));
  }
  else
  {
    timed_sync_response msg{};
    msg.payload_data = sync;
    msg.local_peerlist_new = sent;
    FUZZ_CHECK(epee::serialization::store_t_to_binary(msg, slice));
  }

  // Corrupt a few bytes of the encoded message, as a malicious peer could.
  std::string blob(reinterpret_cast<const char *>(slice.data()), slice.size());
  const size_t n_mutations = fdp.ConsumeIntegralInRange<size_t>(0, 4);
  for (size_t i = 0; i < n_mutations && !blob.empty(); ++i)
    blob[fdp.ConsumeIntegralInRange<size_t>(0, blob.size() - 1)] = fdp.ConsumeIntegral<char>();
  const bool truncated = fdp.ConsumeIntegralInRange<int>(0, 15) == 0;
  if (truncated)
    blob.resize(fdp.ConsumeIntegralInRange<size_t>(0, blob.size()));

  std::vector<peerlist_entry> received;
  bool loaded;
  if (handshake)
  {
    handshake_response msg{};
    loaded = epee::serialization::load_t_from_binary(msg, epee::strspan<uint8_t>(blob), &kLevinLimits);
    received = std::move(msg.local_peerlist_new);
  }
  else
  {
    timed_sync_response msg{};
    loaded = epee::serialization::load_t_from_binary(msg, epee::strspan<uint8_t>(blob), &kLevinLimits);
    received = std::move(msg.local_peerlist_new);
  }
  if (n_mutations == 0 && !truncated)
  {
    FUZZ_CHECK(loaded);
    FUZZ_CHECK(received.size() == sent.size());
    for (size_t i = 0; i < sent.size(); ++i)
      FUZZ_CHECK(same_entry(received[i], sent[i]));
  }
  if (!loaded)
    return;

  nodetool::print_peerlist_to_string(received);
  for (const auto &pe : received)
    touch_address(fdp, pe.adr);

  // Same acceptance rules as node_server::handle_remote_peerlist.
  if (received.size() > P2P_MAX_PEERS_IN_HANDSHAKE)
    return;
  sanitize(received);
  for (const auto &pe : received)
    if (pe.adr.get_zone() != remote_zone)
      return;
  pl.merge_peerlist(received);
}

static void store_and_reload(peerlist_manager &pl, bool allow_local)
{
  nodetool::peerlist_types before;
  pl.get_peerlist(before);

  std::ostringstream out;
  FUZZ_CHECK(nodetool::peerlist_storage{}.store(out, before));
  std::istringstream in{out.str()};
  auto reopened = nodetool::peerlist_storage::open(in, true);
  FUZZ_CHECK(reopened);

  nodetool::peerlist_types after;
  for (const zone z : kZones)
  {
    peerlist_manager zone_pl;
    FUZZ_CHECK(zone_pl.init(reopened->take_zone(z), allow_local));
    zone_pl.get_peerlist(after);
  }

  sort_by_address(before.white);
  sort_by_address(before.gray);
  sort_by_address(before.anchor);
  sort_by_address(after.white);
  sort_by_address(after.gray);
  sort_by_address(after.anchor);
  FUZZ_CHECK(before.white.size() == after.white.size());
  FUZZ_CHECK(before.gray.size() == after.gray.size());
  FUZZ_CHECK(before.anchor.size() == after.anchor.size());
  for (size_t i = 0; i < before.white.size(); ++i)
    FUZZ_CHECK(same_entry(before.white[i], after.white[i]));
  for (size_t i = 0; i < before.gray.size(); ++i)
    FUZZ_CHECK(same_entry(before.gray[i], after.gray[i]));
  for (size_t i = 0; i < before.anchor.size(); ++i)
  {
    FUZZ_CHECK(before.anchor[i].adr == after.anchor[i].adr);
    FUZZ_CHECK(before.anchor[i].id == after.anchor[i].id);
    FUZZ_CHECK(before.anchor[i].first_seen == after.anchor[i].first_seen);
  }
}

// Enough distinct public hosts to push the lists past their trim limits.
static void bulk_insert(FuzzedDataProvider &fdp, peerlist_manager &pl)
{
  const bool white = fdp.ConsumeBool();
  const size_t count = fdp.ConsumeIntegralInRange<size_t>(0, white ? kMaxBulkWhite : kMaxBulkGray);
  const uint32_t base = fdp.ConsumeIntegral<uint16_t>();
  std::vector<peerlist_entry> peers;
  for (size_t i = 0; i < count; ++i)
  {
    peerlist_entry pe{};
    const uint32_t n = base + i;
    pe.adr = ipv4_network_address{MAKE_IP(5, (n >> 16) & 0xff, (n >> 8) & 0xff, n & 0xff), 18080};
    pe.id = n;
    pe.last_seen = int64_t(n) * 7 % 1000;
    peers.push_back(pe);
  }
  if (white)
    for (const auto &pe : peers)
      pl.append_with_peer_white(pe, true);
  else
    pl.merge_peerlist(peers);
}

static bool pick_existing(FuzzedDataProvider &fdp, peerlist_manager &pl, bool white, peerlist_entry &out)
{
  std::vector<peerlist_entry> gray, whites;
  pl.get_peerlist(gray, whites);
  const auto &list = white ? whites : gray;
  if (list.empty())
    return false;
  out = list[fdp.ConsumeIntegralInRange<size_t>(0, list.size() - 1)];
  return true;
}

BEGIN_INIT_SIMPLE_FUZZER()
  for (const char *ip : {"::", "::1", "fe80::1", "fec0::1", "fc00::1", "fd12:3456::1", "ff02::1",
        "2001:db8::1", "2001:db8::2", "2001:db8:0:1::1", "::ffff:1.2.3.4", "::ffff:127.0.0.1"})
    ipv6_pool.push_back(boost::asio::ip::make_address_v6(ip));
END_INIT_SIMPLE_FUZZER()

BEGIN_SIMPLE_FUZZER()
  FuzzedDataProvider fdp(buf, len);
  const bool allow_local = fdp.ConsumeBool();
  peerlist_manager pl;
  FUZZ_CHECK(pl.init(nodetool::peerlist_types{}, allow_local));

  for (size_t op = 0; op < kMaxOps && fdp.remaining_bytes() > 0; ++op)
  {
    switch (fdp.ConsumeIntegralInRange<int>(0, 12))
    {
      case 0:
        receive_peerlist(fdp, pl);
        break;
      case 1:
      {
        const peerlist_entry pe = consume_entry(fdp);
        pl.append_with_peer_white(pe, fdp.ConsumeBool());
        break;
      }
      case 2:
        pl.append_with_peer_gray(consume_entry(fdp));
        break;
      case 3:
      {
        peerlist_entry pe;
        if (!fdp.ConsumeBool() || !pick_existing(fdp, pl, false, pe))
          pe = consume_entry(fdp);
        pl.set_peer_just_seen(pe.id, pe.adr, pe.pruning_seed, pe.rpc_port, pe.rpc_credits_per_hash);
        break;
      }
      case 4:
      {
        anchor_peerlist_entry ape{};
        ape.adr = consume_address(fdp);
        ape.id = fdp.ConsumeIntegral<uint64_t>();
        ape.first_seen = fdp.ConsumeIntegral<int64_t>();
        pl.append_with_peer_anchor(ape);
        break;
      }
      case 5:
      {
        const bool white = fdp.ConsumeBool();
        peerlist_entry pe;
        if (!pick_existing(fdp, pl, white, pe))
          pe = consume_entry(fdp);
        if (white)
          pl.remove_from_peer_white(pe);
        else
          pl.remove_from_peer_gray(pe);
        break;
      }
      case 6:
        pl.remove_from_peer_anchor(consume_address(fdp));
        break;
      case 7:
      {
        const bool anonymize = fdp.ConsumeBool();
        const uint32_t depth = fdp.ConsumeIntegralInRange<uint32_t>(0, P2P_DEFAULT_PEERS_IN_HANDSHAKE);
        std::vector<peerlist_entry> head;
        FUZZ_CHECK(pl.get_peerlist_head(head, anonymize, depth));
        FUZZ_CHECK(head.size() <= depth);
        FUZZ_CHECK(head.size() <= pl.get_white_peers_count());
        for (size_t i = 0; i < head.size(); ++i)
        {
          if (anonymize)
            FUZZ_CHECK(head[i].last_seen == 0);
          else if (i > 0)
            FUZZ_CHECK(head[i - 1].last_seen >= head[i].last_seen);
        }
        break;
      }
      case 8:
      {
        const bool white = fdp.ConsumeBool();
        const zone z = fdp.PickValueInArray(kZones);
        const uint16_t port = fdp.ConsumeIntegral<uint16_t>();
        const auto drop = [&](const peerlist_entry &pe) { return pe.adr.get_zone() == z && pe.adr.port() <= port; };
        pl.filter(white, drop);
        FUZZ_CHECK(pl.foreach(white, [&](const peerlist_entry &pe) { return !drop(pe); }));
        break;
      }
      case 9:
      {
        std::vector<anchor_peerlist_entry> anchors, again;
        FUZZ_CHECK(pl.get_and_empty_anchor_peerlist(anchors));
        FUZZ_CHECK(pl.get_and_empty_anchor_peerlist(again));
        FUZZ_CHECK(again.empty());
        break;
      }
      case 10:
        store_and_reload(pl, allow_local);
        break;
      case 11:
        bulk_insert(fdp, pl);
        break;
      default:
      {
        // get_random_gray_peer is random, so only its call is exercised; eviction uses a fuzz-picked peer.
        peerlist_entry random_pe;
        pl.get_random_gray_peer(random_pe);
        const bool white = fdp.ConsumeBool();
        peerlist_entry pe;
        if (pick_existing(fdp, pl, fdp.ConsumeBool(), pe))
          pl.evict_host_from_peerlist(white, pe);
        break;
      }
    }
    check_invariants(pl, allow_local);
  }
END_SIMPLE_FUZZER()
