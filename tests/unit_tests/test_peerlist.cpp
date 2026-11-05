// Copyright (c) 2014-2024, The Monero Project
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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "gtest/gtest.h"

#include "common/util.h"
#include "p2p/net_peerlist.h"
#include "net/net_utils_base.h"

TEST(peer_list, peer_list_general)
{
  nodetool::peerlist_manager plm;
  plm.init(nodetool::peerlist_types{}, false);
#define MAKE_IPV4_ADDRESS(a,b,c,d,e) epee::net_utils::ipv4_network_address{MAKE_IP(a,b,c,d),e}
#define ADD_GRAY_NODE(addr_, id_, last_seen_) {  nodetool::peerlist_entry ple; ple.last_seen=last_seen_;ple.adr = addr_; ple.id = id_;plm.append_with_peer_gray(ple);}  
#define ADD_WHITE_NODE(addr_, id_, last_seen_) {  nodetool::peerlist_entry ple;ple.last_seen=last_seen_; ple.adr = addr_; ple.id = id_;plm.append_with_peer_white(ple);}  

#define PRINT_HEAD(step) {std::vector<nodetool::peerlist_entry> bs_head; bool r = plm.get_peerlist_head(bs_head, 100);std::cout << "step " << step << ": " << bs_head.size() << std::endl;}

  ADD_GRAY_NODE(MAKE_IPV4_ADDRESS(123,43,12,1, 8080), 121241, 34345);
  ADD_GRAY_NODE(MAKE_IPV4_ADDRESS(123,43,12,2, 8080), 121241, 34345);
  ADD_GRAY_NODE(MAKE_IPV4_ADDRESS(123,43,12,3, 8080), 121241, 34345);
  ADD_GRAY_NODE(MAKE_IPV4_ADDRESS(123,43,12,4, 8080), 121241, 34345);
  ADD_GRAY_NODE(MAKE_IPV4_ADDRESS(123,43,12,5, 8080), 121241, 34345);

  ADD_WHITE_NODE(MAKE_IPV4_ADDRESS(123,43,12,1, 8080), 121241, 34345);
  ADD_WHITE_NODE(MAKE_IPV4_ADDRESS(123,43,12,2, 8080), 121241, 34345);
  ADD_WHITE_NODE(MAKE_IPV4_ADDRESS(123,43,12,3, 8080), 121241, 34345);
  ADD_WHITE_NODE(MAKE_IPV4_ADDRESS(123,43,12,4, 8080), 121241, 34345);

  size_t gray_list_size = plm.get_gray_peers_count();
  ASSERT_EQ(gray_list_size, 1);

  std::vector<nodetool::peerlist_entry> bs_head;
  bool r = plm.get_peerlist_head(bs_head, 100);
  std::cout << bs_head.size() << std::endl;
  ASSERT_TRUE(r);

  ASSERT_EQ(bs_head.size(), 4);


  ADD_GRAY_NODE(MAKE_IPV4_ADDRESS(123,43,12,5, 8080), 121241, 34345);
  ASSERT_EQ(plm.get_gray_peers_count(), 1);
  ASSERT_EQ(plm.get_white_peers_count(), 4);
}


TEST(peer_list, rejects_ipv4_link_local_by_default)
{
  nodetool::peerlist_manager plm;
  ASSERT_TRUE(plm.init(nodetool::peerlist_types{}, false));

  const nodetool::peerlist_entry link_local{
    epee::net_utils::ipv4_network_address{MAKE_IP(169, 254, 169, 254), 18080}, 1, 0};
  const nodetool::peerlist_entry public_peer{
    epee::net_utils::ipv4_network_address{MAKE_IP(123, 45, 67, 89), 18080}, 2, 0};
  ASSERT_TRUE(plm.merge_peerlist({link_local, public_peer}));
  ASSERT_TRUE(plm.append_with_peer_white(link_local));

  std::vector<nodetool::peerlist_entry> gray;
  std::vector<nodetool::peerlist_entry> white;
  plm.get_peerlist(gray, white);

  ASSERT_EQ(1u, gray.size());
  EXPECT_EQ(public_peer.adr, gray[0].adr);
  EXPECT_TRUE(white.empty());
}

TEST(peer_list, merge_peer_lists)
{
  //([^ \t]*)\t([^ \t]*):([^ \t]*) \tlast_seen: d(\d+)\.h(\d+)\.m(\d+)\.s(\d+)\n
  //ADD_NODE_TO_PL("\2", \3, 0x\1, (1353346618 -(\4*60*60*24+\5*60*60+\6*60+\7 )));\n
  nodetool::peerlist_manager plm;
  plm.init(nodetool::peerlist_types{}, false);
  std::vector<nodetool::peerlist_entry> outer_bs;
#define ADD_NODE_TO_PL(ip_, port_, id_, timestamp_) {  nodetool::peerlist_entry ple; epee::string_tools::get_ip_int32_from_string(ple.adr.ip, ip_); ple.last_seen = timestamp_; ple.adr.port = port_; ple.id = id_;outer_bs.push_back(ple);}  
}

namespace
{
  bool check_empty(nodetool::peerlist_storage& peers, std::initializer_list<epee::net_utils::zone> zones)
  {
    bool pass = false;
    for (const epee::net_utils::zone zone : zones)
    {
      const nodetool::peerlist_types types{peers.take_zone(zone)};
      EXPECT_TRUE(types.white.empty());
      EXPECT_TRUE(types.gray.empty());
      EXPECT_TRUE(types.anchor.empty());
      pass = (types.white.empty() && types.gray.empty() && types.anchor.empty());
    }
    return pass;
  }
}

TEST(peerlist_storage, store)
{

  using address_type = epee::net_utils::address_type;
  using zone = epee::net_utils::zone;

  nodetool::peerlist_storage peers{};
  EXPECT_TRUE(check_empty(peers, {zone::invalid, zone::public_, zone::tor, zone::i2p}));

  std::string buffer{};
  {
    nodetool::peerlist_types types{};
    types.white.push_back({epee::net_utils::ipv4_network_address{1000, 10}, 44, 55});
    types.white.push_back({net::tor_address::unknown(), 64, 75});
    types.gray.push_back({net::tor_address::unknown(), 99, 88});
    types.gray.push_back({epee::net_utils::ipv4_network_address{2000, 20}, 84, 45});
    types.anchor.push_back({epee::net_utils::ipv4_network_address{999, 654}, 444, 555});
    types.anchor.push_back({net::tor_address::unknown(), 14, 33});
    types.anchor.push_back({net::tor_address::unknown(), 24, 22});

    std::ostringstream stream{};
    EXPECT_TRUE(peers.store(stream, types));
    buffer = stream.str();
  }
  EXPECT_TRUE(check_empty(peers, {zone::invalid, zone::public_, zone::tor, zone::i2p}));
  {
    std::istringstream stream{buffer};
    boost::optional<nodetool::peerlist_storage> read_peers =
      nodetool::peerlist_storage::open(stream, true);
    ASSERT_TRUE(bool(read_peers));
    peers = std::move(*read_peers);
  }
  EXPECT_TRUE(check_empty(peers, {zone::invalid, zone::i2p}));

  nodetool::peerlist_types types = peers.take_zone(zone::public_);
  EXPECT_TRUE(check_empty(peers, {zone::invalid, zone::public_, zone::i2p}));

  ASSERT_EQ(1u, types.white.size());
  ASSERT_EQ(address_type::ipv4, types.white[0].adr.get_type_id());
  EXPECT_EQ(1000u, types.white[0].adr.template as<epee::net_utils::ipv4_network_address>().ip());
  EXPECT_EQ(10u, types.white[0].adr.template as<epee::net_utils::ipv4_network_address>().port());
  EXPECT_EQ(44u, types.white[0].id);
  EXPECT_EQ(55u, types.white[0].last_seen);

  ASSERT_EQ(1u, types.gray.size());
  ASSERT_EQ(address_type::ipv4, types.gray[0].adr.get_type_id());
  EXPECT_EQ(2000u, types.gray[0].adr.template as<epee::net_utils::ipv4_network_address>().ip());
  EXPECT_EQ(20u, types.gray[0].adr.template as<epee::net_utils::ipv4_network_address>().port());
  EXPECT_EQ(84u, types.gray[0].id);
  EXPECT_EQ(45u, types.gray[0].last_seen);

  ASSERT_EQ(1u, types.anchor.size());
  ASSERT_EQ(address_type::ipv4, types.anchor[0].adr.get_type_id());
  EXPECT_EQ(999u, types.anchor[0].adr.template as<epee::net_utils::ipv4_network_address>().ip());
  EXPECT_EQ(654u, types.anchor[0].adr.template as<epee::net_utils::ipv4_network_address>().port());
  EXPECT_EQ(444u, types.anchor[0].id);
  EXPECT_EQ(555u, types.anchor[0].first_seen);
  {
    std::ostringstream stream{};
    EXPECT_TRUE(peers.store(stream, types));
    buffer = stream.str();
  }
  EXPECT_TRUE(check_empty(peers, {zone::invalid, zone::public_, zone::i2p}));

  types = peers.take_zone(zone::tor);
  EXPECT_TRUE(check_empty(peers, {zone::invalid, zone::public_, zone::i2p, zone::tor}));

  ASSERT_EQ(1u, types.white.size());
  ASSERT_EQ(address_type::tor, types.white[0].adr.get_type_id());
  EXPECT_STREQ(net::tor_address::unknown_str(), types.white[0].adr.template as<net::tor_address>().host_str());
  EXPECT_EQ(0u, types.white[0].adr.template as<net::tor_address>().port());
  EXPECT_EQ(64u, types.white[0].id);
  EXPECT_EQ(75u, types.white[0].last_seen);

  ASSERT_EQ(1u, types.gray.size());
  ASSERT_EQ(address_type::tor, types.gray[0].adr.get_type_id());
  EXPECT_STREQ(net::tor_address::unknown_str(), types.gray[0].adr.template as<net::tor_address>().host_str());
  EXPECT_EQ(0u, types.gray[0].adr.template as<net::tor_address>().port());
  EXPECT_EQ(99u, types.gray[0].id);
  EXPECT_EQ(88u, types.gray[0].last_seen);

  ASSERT_EQ(2u, types.anchor.size());
  ASSERT_EQ(address_type::tor, types.anchor[0].adr.get_type_id());
  EXPECT_STREQ(net::tor_address::unknown_str(), types.anchor[0].adr.template as<net::tor_address>().host_str());
  EXPECT_EQ(0u, types.anchor[0].adr.template as<net::tor_address>().port());
  EXPECT_EQ(14u, types.anchor[0].id);
  EXPECT_EQ(33u, types.anchor[0].first_seen);
  ASSERT_EQ(address_type::tor, types.anchor[1].adr.get_type_id());
  EXPECT_STREQ(net::tor_address::unknown_str(), types.anchor[1].adr.template as<net::tor_address>().host_str());
  EXPECT_EQ(0u, types.anchor[1].adr.template as<net::tor_address>().port());
  EXPECT_EQ(24u, types.anchor[1].id);
  EXPECT_EQ(22u, types.anchor[1].first_seen);

  {
    std::istringstream stream{buffer};
    boost::optional<nodetool::peerlist_storage> read_peers =
      nodetool::peerlist_storage::open(stream, true);
    ASSERT_TRUE(bool(read_peers));
    peers = std::move(*read_peers);
  }
  EXPECT_TRUE(check_empty(peers, {zone::invalid, zone::i2p}));

  types = peers.take_zone(zone::public_);
  EXPECT_TRUE(check_empty(peers, {zone::invalid, zone::public_, zone::i2p}));

  ASSERT_EQ(1u, types.white.size());
  ASSERT_EQ(address_type::ipv4, types.white[0].adr.get_type_id());
  EXPECT_EQ(1000u, types.white[0].adr.template as<epee::net_utils::ipv4_network_address>().ip());
  EXPECT_EQ(10u, types.white[0].adr.template as<epee::net_utils::ipv4_network_address>().port());
  EXPECT_EQ(44u, types.white[0].id);
  EXPECT_EQ(55u, types.white[0].last_seen);

  ASSERT_EQ(1u, types.gray.size());
  ASSERT_EQ(address_type::ipv4, types.gray[0].adr.get_type_id());
  EXPECT_EQ(2000u, types.gray[0].adr.template as<epee::net_utils::ipv4_network_address>().ip());
  EXPECT_EQ(20u, types.gray[0].adr.template as<epee::net_utils::ipv4_network_address>().port());
  EXPECT_EQ(84u, types.gray[0].id);
  EXPECT_EQ(45u, types.gray[0].last_seen);

  ASSERT_EQ(1u, types.anchor.size());
  ASSERT_EQ(address_type::ipv4, types.anchor[0].adr.get_type_id());
  EXPECT_EQ(999u, types.anchor[0].adr.template as<epee::net_utils::ipv4_network_address>().ip());
  EXPECT_EQ(654u, types.anchor[0].adr.template as<epee::net_utils::ipv4_network_address>().port());
  EXPECT_EQ(444u, types.anchor[0].id);
  EXPECT_EQ(555u, types.anchor[0].first_seen);

  types = peers.take_zone(zone::tor);
  EXPECT_TRUE(check_empty(peers, {zone::invalid, zone::public_, zone::i2p, zone::tor}));

  ASSERT_EQ(1u, types.white.size());
  ASSERT_EQ(address_type::tor, types.white[0].adr.get_type_id());
  EXPECT_STREQ(net::tor_address::unknown_str(), types.white[0].adr.template as<net::tor_address>().host_str());
  EXPECT_EQ(0u, types.white[0].adr.template as<net::tor_address>().port());
  EXPECT_EQ(64u, types.white[0].id);
  EXPECT_EQ(75u, types.white[0].last_seen);

  ASSERT_EQ(1u, types.gray.size());
  ASSERT_EQ(address_type::tor, types.gray[0].adr.get_type_id());
  EXPECT_STREQ(net::tor_address::unknown_str(), types.gray[0].adr.template as<net::tor_address>().host_str());
  EXPECT_EQ(0u, types.gray[0].adr.template as<net::tor_address>().port());
  EXPECT_EQ(99u, types.gray[0].id);
  EXPECT_EQ(88u, types.gray[0].last_seen);

  ASSERT_EQ(2u, types.anchor.size());
  ASSERT_EQ(address_type::tor, types.anchor[0].adr.get_type_id());
  EXPECT_STREQ(net::tor_address::unknown_str(), types.anchor[0].adr.template as<net::tor_address>().host_str());
  EXPECT_EQ(0u, types.anchor[0].adr.template as<net::tor_address>().port());
  EXPECT_EQ(14u, types.anchor[0].id);
  EXPECT_EQ(33u, types.anchor[0].first_seen);
  ASSERT_EQ(address_type::tor, types.anchor[1].adr.get_type_id());
  EXPECT_STREQ(net::tor_address::unknown_str(), types.anchor[1].adr.template as<net::tor_address>().host_str());
  EXPECT_EQ(0u, types.anchor[1].adr.template as<net::tor_address>().port());
  EXPECT_EQ(24u, types.anchor[1].id);
  EXPECT_EQ(22u, types.anchor[1].first_seen);
}

namespace
{
  std::vector<uint32_t> routable_first_octets(const uint32_t begin, const uint32_t end)
  {
    std::vector<uint32_t> octets;
    for (uint32_t a = begin; a < end; ++a)
      if (a != 10 && a != 127 && a != 172 && a != 192)
        octets.push_back(a);
    return octets;
  }

  const std::vector<uint32_t> honest_octets = routable_first_octets(1, 100);
  const std::vector<uint32_t> flood_octets = routable_first_octets(128, 224);

  epee::net_utils::network_address spread_address(const std::vector<uint32_t>& octets, const uint32_t n)
  {
    const uint32_t netgroups = octets.size() * 256;
    const uint32_t netgroup = n % netgroups;
    const uint32_t host = n / netgroups;
    return epee::net_utils::ipv4_network_address{MAKE_IP(octets[netgroup / 256], netgroup % 256, host % 256, 1 + host / 256), 18080};
  }

  void add_white(nodetool::peerlist_manager& plm, const epee::net_utils::network_address& adr, const uint64_t id, const int64_t last_seen)
  {
    nodetool::peerlist_entry pe{};
    pe.adr = adr;
    pe.id = id;
    pe.last_seen = last_seen;
    plm.append_with_peer_white(pe);
  }

  template<typename F>
  size_t count_white(nodetool::peerlist_manager& plm, const F& match)
  {
    size_t count = 0;
    plm.foreach(true, [&](const nodetool::peerlist_entry& pe) {
      if (match(pe.adr.as<epee::net_utils::ipv4_network_address>().ip()))
        ++count;
      return true;
    });
    return count;
  }

  bool is_honest(const uint32_t ip) { return (ip & 0xff) < 100; }
  bool is_flood(const uint32_t ip) { return (ip & 0xff) >= 128; }

  void fill_white(nodetool::peerlist_manager& plm, const int64_t newest)
  {
    for (uint32_t i = 0; i < P2P_LOCAL_WHITE_PEERLIST_LIMIT; ++i)
      add_white(plm, spread_address(honest_octets, i), i + 1, newest - i);
  }

  constexpr int64_t hour = 60 * 60;
  constexpr int64_t day = 24 * hour;
}

TEST(peer_list, white_flood_keeps_recently_seen_peers)
{
  nodetool::peerlist_manager plm;
  plm.init(nodetool::peerlist_types{}, false);
  const int64_t now = time(nullptr);
  const uint32_t limit = P2P_LOCAL_WHITE_PEERLIST_LIMIT;

  fill_white(plm, now - hour);
  ASSERT_EQ(limit, plm.get_white_peers_count());

  for (uint32_t i = 0; i < 2 * limit; ++i)
    add_white(plm, spread_address(flood_octets, i), limit + i + 1, now);

  EXPECT_EQ(limit, plm.get_white_peers_count());
  EXPECT_EQ(limit, count_white(plm, is_honest));
}

TEST(peer_list, white_list_replaces_stale_peers)
{
  nodetool::peerlist_manager plm;
  plm.init(nodetool::peerlist_types{}, false);
  const int64_t now = time(nullptr);
  const uint32_t limit = P2P_LOCAL_WHITE_PEERLIST_LIMIT;

  fill_white(plm, now - 31 * day);
  ASSERT_EQ(limit, plm.get_white_peers_count());

  for (uint32_t i = 0; i < limit; ++i)
    add_white(plm, spread_address(flood_octets, i), limit + i + 1, now);

  EXPECT_EQ(limit, plm.get_white_peers_count());
  EXPECT_EQ(limit, count_white(plm, is_flood));
}

TEST(peer_list, white_collision_tests_oldest_peer)
{
  nodetool::peerlist_manager plm;
  plm.init(nodetool::peerlist_types{}, false);
  const int64_t now = time(nullptr);
  const uint32_t limit = P2P_LOCAL_WHITE_PEERLIST_LIMIT;

  fill_white(plm, now - hour);
  add_white(plm, spread_address(flood_octets, 0), limit + 1, now);
  EXPECT_EQ(0, count_white(plm, is_flood));

  nodetool::peerlist_entry oldest{}, newcomer{};
  ASSERT_TRUE(plm.get_white_collision(oldest, newcomer));
  EXPECT_TRUE(oldest.adr == spread_address(honest_octets, limit - 1));
  EXPECT_TRUE(newcomer.adr == spread_address(flood_octets, 0));
  EXPECT_FALSE(plm.get_white_collision(oldest, newcomer));
}

TEST(peer_list, white_collisions_are_capped)
{
  nodetool::peerlist_manager plm;
  plm.init(nodetool::peerlist_types{}, false);
  const int64_t now = time(nullptr);
  const uint32_t limit = P2P_LOCAL_WHITE_PEERLIST_LIMIT;

  fill_white(plm, now - hour);
  for (uint32_t i = 0; i < 2 * P2P_LOCAL_WHITE_PEERLIST_COLLISIONS_MAX; ++i)
    add_white(plm, spread_address(flood_octets, i), limit + i + 1, now);

  size_t collisions = 0;
  nodetool::peerlist_entry oldest{}, newcomer{};
  while (plm.get_white_collision(oldest, newcomer))
    ++collisions;
  EXPECT_EQ(P2P_LOCAL_WHITE_PEERLIST_COLLISIONS_MAX, collisions);
}

TEST(peer_list, white_collision_takes_freed_slot)
{
  nodetool::peerlist_manager plm;
  plm.init(nodetool::peerlist_types{}, false);
  const int64_t now = time(nullptr);
  const uint32_t limit = P2P_LOCAL_WHITE_PEERLIST_LIMIT;

  fill_white(plm, now - hour);
  add_white(plm, spread_address(flood_octets, 0), limit + 1, now);

  nodetool::peerlist_entry gone{};
  gone.adr = spread_address(honest_octets, 0);
  plm.remove_from_peer_white(gone);

  nodetool::peerlist_entry oldest{}, newcomer{};
  EXPECT_FALSE(plm.get_white_collision(oldest, newcomer));
  EXPECT_EQ(limit, plm.get_white_peers_count());
  EXPECT_EQ(1, count_white(plm, is_flood));
}
