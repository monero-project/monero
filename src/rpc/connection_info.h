#pragma once

#include "serialization/keyvalue_serialization.h"

namespace nodetool {
  struct connection_info
  {
    std::string uuid;
    bool is_incoming;
    uint32_t ip;
    uint16_t port;
    uint64_t peer_id;

    connection_info() = default;

    connection_info(std::string uuid, bool is_incoming, uint32_t ip, uint16_t port, uint64_t peer_id)
      : uuid(uuid)
      , is_incoming(is_incoming)
      , ip(ip)
      , port(port)
      , peer_id(peer_id)
    {}

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(uuid)
      KV_SERIALIZE(is_incoming)
      KV_SERIALIZE(ip)
      KV_SERIALIZE(port)
      KV_SERIALIZE(peer_id)
    END_KV_SERIALIZE_MAP()
  };
}
