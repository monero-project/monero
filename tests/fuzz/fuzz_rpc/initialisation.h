#pragma once
#include "crypto/hash.h"
#include "rpc/core_rpc_server.h"
#include <fuzzer/FuzzedDataProvider.h>

// Define templates for dummy protocol object
template class nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core>>;

struct DummyProtocol : public cryptonote::i_cryptonote_protocol {
public:
  bool is_synchronized() const;
  bool relay_transactions(cryptonote::NOTIFY_NEW_TRANSACTIONS::request&, const boost::uuids::uuid&, epee::net_utils::zone, cryptonote::relay_method);
  bool relay_block(cryptonote::NOTIFY_NEW_FLUFFY_BLOCK::request&, cryptonote::cryptonote_connection_context&);
};

struct RpcServerBundle {
  std::unique_ptr<cryptonote::t_cryptonote_protocol_handler<cryptonote::core>> proto_handler;
  std::unique_ptr<nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core>>> dummy_p2p;
  std::unique_ptr<cryptonote::core_rpc_server> rpc;
};

struct CoreEnv {
  std::unique_ptr<DummyProtocol> protocol;
  std::unique_ptr<cryptonote::core> core;
};

std::unique_ptr<CoreEnv> initialise_rpc_core();
std::unique_ptr<RpcServerBundle> initialise_rpc_server(cryptonote::core&, FuzzedDataProvider&, bool);
bool generate_random_blocks(cryptonote::core&, FuzzedDataProvider&);

extern std::vector<crypto::hash> cached_tx_hashes;
