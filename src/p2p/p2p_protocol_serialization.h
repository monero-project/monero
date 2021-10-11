#pragma once

#include "p2p/p2p_protocol_defs.h"
#include "p2p/portable_scheme/scheme.h"

namespace portable_scheme {

namespace scheme_space {

struct addr_t {
  struct {
    bool addr;
    bool m_ip;
    bool host;
    bool m_port;
    bool port;
  } absent;
  boost::asio::ip::address_v6::bytes_type ipv6;
  std::uint32_t ipv4;
  std::string host;
  std::uint16_t port;
};

template<>
struct scheme<addr_t> {
  using T = addr_t;
  using tags = map_tag<
    field_tag<KEY("addr"), span_tag<>>,
    field_tag<KEY("host"), span_tag<>>,
    field_tag<KEY("m_ip"), base_tag<std::uint32_t, endian_t::native>>,
    field_tag<KEY("m_port"), base_tag<std::uint16_t>>,
    field_tag<KEY("port"), base_tag<std::uint16_t>>
  >;
  BEGIN_READ(T)
    READ_FIELD_POD_OPT_NAME("addr", t.ipv6, {return not t.absent.addr;})
    READ_FIELD_OPT_NAME("m_ip", t.ipv4, {return not t.absent.m_ip;})
    READ_FIELD_LIST_POD_OPT(host, {return not t.absent.host;})
    READ_FIELD_OPT_NAME("m_port", t.port, {return not t.absent.m_port;})
    READ_FIELD_OPT(port, {return not t.absent.port;})
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD_POD_OPT_NAME("addr", t.ipv6, {t.absent.addr = true; return true;})
    WRITE_FIELD_OPT_NAME("m_ip", t.ipv4, {t.absent.m_ip = true; return true;})
    WRITE_FIELD_LIST_POD_OPT(host, {t.absent.host = true; return true;})
    WRITE_FIELD_OPT_NAME("m_port", t.port, {t.absent.m_port = true; return true;})
    WRITE_FIELD_OPT(port, {t.absent.port = true; return true;})
  END_WRITE()
};

template<>
struct scheme<epee::net_utils::network_address> {
  using T = epee::net_utils::network_address;
  using tags = map_tag<
    field_tag<KEY("addr"), scheme<addr_t>::tags>,
    field_tag<KEY("type"), base_tag<std::uint8_t>>
  >;
  using ipv4_addr = epee::net_utils::ipv4_network_address;
  using ipv6_addr = epee::net_utils::ipv6_network_address;
  using tor_addr = net::tor_address;
  using i2p_addr = net::i2p_address;
  using addr_type = epee::net_utils::address_type;
  BEGIN_READ_RAW(
    private:
      addr_t addr;
      std::uint8_t type_id;
    public:
      read_t(const T &t):
        addr{
          .absent = {
            .addr = t.get_type_id() != addr_type::ipv6,
            .m_ip = t.get_type_id() != addr_type::ipv4,
            .host = t.get_type_id() != addr_type::tor and t.get_type_id() != addr_type::i2p,
            .m_port = t.get_type_id() != addr_type::ipv4 and t.get_type_id() != addr_type::ipv6,
            .port = t.get_type_id() != addr_type::tor and t.get_type_id() != addr_type::i2p,
          },
        },
        type_id{
          static_cast<std::uint8_t>(t.get_type_id())
        }
      {
        switch(t.get_type_id()) {
          case addr_type::ipv4: {addr.ipv4 = t.as<ipv4_addr>().ip(); addr.port = t.as<ipv4_addr>().port(); break;}
          case addr_type::ipv6: {addr.ipv6 = t.as<ipv6_addr>().ip().to_bytes(); addr.port = t.as<ipv6_addr>().port(); break;}
          case addr_type::i2p: {addr.host = t.as<i2p_addr>().host_str(); addr.port = t.as<i2p_addr>().port(); break;}
          case addr_type::tor: {addr.host = t.as<tor_addr>().host_str(); addr.port = t.as<tor_addr>().port(); break;}
          default: break;
        }
      }
  )
    READ_FIELD_NAME("addr", addr)
    READ_FIELD_NAME("type", type_id)
  END_READ()
  BEGIN_WRITE_RAW(
    private:
      T &t;
      addr_t addr{};
      std::uint8_t type_id{};
    public:
      write_t(T &t): t(t) {}
  )
    WRITE_FIELD_NAME("addr", addr)
    WRITE_FIELD_NAME("type", type_id)
    WRITE_CHECK(
      switch (static_cast<addr_type>(type_id)) {
        case addr_type::ipv4:
          if (addr.absent.m_ip or addr.absent.m_port or not addr.absent.port)
            return false;
          t = ipv4_addr(addr.ipv4, addr.port);
          break;
        case addr_type::ipv6:
          if (addr.absent.addr or addr.absent.m_port or not addr.absent.port)
            return false;
          t = ipv6_addr(boost::asio::ip::address_v6(addr.ipv6), addr.port);
          break;
        case addr_type::tor: {
          if (addr.absent.host or addr.absent.port or not addr.absent.m_port)
            return false;
          auto&& result = tor_addr::make(addr.host, addr.port);
          if (not result)
            return false;
          t = result.value();
          break;
        }
        case addr_type::i2p: {
          if (addr.absent.host or addr.absent.port or not addr.absent.m_port)
            return false;
          auto&& result = i2p_addr::make(addr.host, addr.port);
          if (not result)
            return false;
          t = result.value();
          break;
        }
        default:
          return false;
      }
      return true;
    )
  END_WRITE()
};

template<>
struct scheme<nodetool::peerlist_entry_base<epee::net_utils::network_address>> {
  using T = nodetool::peerlist_entry_base<epee::net_utils::network_address>;
  using tags = map_tag<
    field_tag<KEY("adr"), scheme<decltype(T::adr)>::tags>,
    field_tag<KEY("id"), base_tag<std::uint64_t>>,
    field_tag<KEY("last_seen"), base_tag<std::int64_t>>,
    field_tag<KEY("pruning_seed"), base_tag<std::uint32_t>>,
    field_tag<KEY("rpc_credits_per_hash"), base_tag<std::uint32_t>>,
    field_tag<KEY("rpc_port"), base_tag<std::uint16_t>>
  >;
  BEGIN_READ(T)
    READ_FIELD(adr)
    READ_FIELD_WITH_DEFAULT(id, 0)
    READ_FIELD_WITH_DEFAULT(last_seen, 0)
    READ_FIELD_WITH_DEFAULT(pruning_seed, 0)
    READ_FIELD_WITH_DEFAULT(rpc_credits_per_hash, 0)
    READ_FIELD_WITH_DEFAULT(rpc_port, 0)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD(adr)
    WRITE_FIELD_WITH_DEFAULT(id, 0)
    WRITE_FIELD_WITH_DEFAULT(last_seen, 0)
    WRITE_FIELD_WITH_DEFAULT(pruning_seed, 0)
    WRITE_FIELD_WITH_DEFAULT(rpc_credits_per_hash, 0)
    WRITE_FIELD_WITH_DEFAULT(rpc_port, 0)
  END_WRITE()
};

template<>
struct scheme<nodetool::basic_node_data> {
  using T = nodetool::basic_node_data;
  using tags = map_tag<
    field_tag<KEY("my_port"), base_tag<std::uint32_t>>,
    field_tag<KEY("network_id"), span_tag<>>,
    field_tag<KEY("peer_id"), base_tag<std::uint64_t>>,
    field_tag<KEY("rpc_credits_per_hash"), base_tag<std::uint32_t>>,
    field_tag<KEY("rpc_port"), base_tag<std::uint16_t>>,
    field_tag<KEY("support_flags"), base_tag<std::uint32_t>>
  >;
  BEGIN_READ(T)
    READ_FIELD(my_port)
    READ_FIELD_POD(network_id)
    READ_FIELD(peer_id)
    READ_FIELD_WITH_DEFAULT(rpc_credits_per_hash, 0)
    READ_FIELD_WITH_DEFAULT(rpc_port, 0)
    READ_FIELD_WITH_DEFAULT(support_flags, 0)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD(my_port)
    WRITE_FIELD_POD(network_id)
    WRITE_FIELD(peer_id)
    WRITE_FIELD_WITH_DEFAULT(rpc_credits_per_hash, 0)
    WRITE_FIELD_WITH_DEFAULT(rpc_port, 0)
    WRITE_FIELD_WITH_DEFAULT(support_flags, 0)
  END_WRITE()
};

template<typename T>
struct scheme<T, typename std::enable_if<(std::is_same<T, typename nodetool::COMMAND_HANDSHAKE_T<decltype(T::payload_data)>::request>::value), void>::type> {
  using tags = map_tag<
    field_tag<KEY("node_data"), typename scheme<decltype(T::node_data)>::tags>,
    field_tag<KEY("payload_data"), typename scheme<decltype(T::payload_data)>::tags>
  >;
  BEGIN_READ(T)
    READ_FIELD(node_data)
    READ_FIELD(payload_data)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD(node_data)
    WRITE_FIELD(payload_data)
  END_WRITE()
};

struct peerlist_t {
  using T = std::vector<nodetool::peerlist_entry_base<epee::net_utils::network_address>>;
  using S = scheme<container_tag<T, scheme<T::value_type>, 0, P2P_MAX_PEERS_IN_HANDSHAKE>>;
  using tags = S::tags;
  using read_t = S::read_t;
  using write_t = S::write_t;
};

template<typename T>
struct scheme<T, typename std::enable_if<(std::is_same<T, typename nodetool::COMMAND_HANDSHAKE_T<decltype(T::payload_data)>::response>::value), void>::type> {
  using tags = map_tag<
    field_tag<KEY("local_peerlist_new"), peerlist_t::tags>,
    field_tag<KEY("node_data"), typename scheme<decltype(T::node_data)>::tags>,
    field_tag<KEY("payload_data"), typename scheme<decltype(T::payload_data)>::tags>
  >;
  BEGIN_READ(T)
    READ_FIELD_LIST_CUSTOM(local_peerlist_new, peerlist_t)
    READ_FIELD(node_data)
    READ_FIELD(payload_data)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD_LIST_CUSTOM(local_peerlist_new, peerlist_t)
    WRITE_FIELD(node_data)
    WRITE_FIELD(payload_data)
  END_WRITE()
};

template<typename T>
struct scheme<T, typename std::enable_if<(std::is_same<T, typename nodetool::COMMAND_TIMED_SYNC_T<decltype(T::payload_data)>::request>::value), void>::type> {
  using tags = map_tag<
    field_tag<KEY("payload_data"), typename scheme<decltype(T::payload_data)>::tags>
  >;
  BEGIN_READ(T)
    READ_FIELD(payload_data)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD(payload_data)
  END_WRITE()
};

template<typename T>
struct scheme<T, typename std::enable_if<(std::is_same<T, typename nodetool::COMMAND_TIMED_SYNC_T<decltype(T::payload_data)>::response>::value), void>::type> {
  using tags = map_tag<
    field_tag<KEY("local_peerlist_new"), peerlist_t::tags>,
    field_tag<KEY("payload_data"), typename scheme<decltype(T::payload_data)>::tags>
  >;
  BEGIN_READ(T)
    READ_FIELD_LIST_CUSTOM(local_peerlist_new, peerlist_t)
    READ_FIELD(payload_data)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD_LIST_CUSTOM(local_peerlist_new, peerlist_t)
    WRITE_FIELD(payload_data)
  END_WRITE()
};

template<>
struct scheme<nodetool::COMMAND_PING::request> {
  using T = nodetool::COMMAND_PING::request;
  using tags = map_tag<>;
  READ(T)
  WRITE(T)
};

template<>
struct scheme<nodetool::COMMAND_PING::response> {
  using T = nodetool::COMMAND_PING::response;
  using tags = map_tag<
    field_tag<KEY("peer_id"), base_tag<std::uint64_t>>,
    field_tag<KEY("status"), scheme<container_pod_tag<decltype(T::status)>>::tags>
  >;
  BEGIN_READ(T)
    READ_FIELD_LIST_POD(status)
    READ_FIELD(peer_id)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD_LIST_POD(status)
    WRITE_FIELD(peer_id)
  END_WRITE()
};

template<>
struct scheme<nodetool::COMMAND_REQUEST_SUPPORT_FLAGS::request> {
  using T = nodetool::COMMAND_REQUEST_SUPPORT_FLAGS::request;
  using tags = map_tag<>;
  READ(T)
  WRITE(T)
};

template<>
struct scheme<nodetool::COMMAND_REQUEST_SUPPORT_FLAGS::response> {
  using T = nodetool::COMMAND_REQUEST_SUPPORT_FLAGS::response;
  using tags = map_tag<
    field_tag<KEY("support_flags"), base_tag<std::uint32_t>>
  >;
  BEGIN_READ(T)
    READ_FIELD(support_flags)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD(support_flags)
  END_WRITE()
};

}

}
