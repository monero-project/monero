
#pragma once
#include "serialization/keyvalue_serialization.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/cryptonote_boost_serialization.h"
#include "cryptonote_basic/account_boost_serialization.h"
#include "cryptonote_basic/cryptonote_basic.h"

namespace tools
{
namespace messaging_daemon_rpc
{
  struct message
  {
    cryptonote::account_public_address source_monero_address;
    std::string source_transport_address;
    cryptonote::account_public_address destination_monero_address;
    std::string destination_transport_address;
    crypto::chacha_iv iv;
    crypto::public_key encryption_public_key;
    uint64_t timestamp;
    uint32_t type;
    std::string content;
    crypto::hash hash;
    std::string signature;
    std::string transport_id;
    
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(source_monero_address)
      KV_SERIALIZE(source_transport_address)
      KV_SERIALIZE(destination_monero_address)
      KV_SERIALIZE(destination_transport_address)
      KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE(iv)
      KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE(encryption_public_key)
      KV_SERIALIZE(timestamp)
      KV_SERIALIZE(type)
      KV_SERIALIZE(content)
      KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE(hash)
      KV_SERIALIZE(signature)
      KV_SERIALIZE(transport_id)
    END_KV_SERIALIZE_MAP()
  };
  
  struct COMMAND_RPC_SEND_MESSAGE
  {
    struct request
    {
      message msg;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(msg)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_RECEIVE_MESSAGES
  {
    struct request
    {
      cryptonote::account_public_address destination_monero_address;
      std::string destination_transport_address;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(destination_monero_address)
        KV_SERIALIZE(destination_transport_address)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::vector<message> msg;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(msg)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_DELETE_MESSAGE
  {
    struct request
    {
      std::string transport_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(transport_id)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_ADD
  {
    struct request
    {
      uint32_t first;
      uint32_t second;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(first)
        KV_SERIALIZE(second)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint32_t sum;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(sum)
      END_KV_SERIALIZE_MAP()
    };
  };

}

}
