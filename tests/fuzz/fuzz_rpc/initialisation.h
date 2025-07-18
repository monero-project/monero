#pragma once
#include "rpc/core_rpc_server.h"
#include <fuzzer/FuzzedDataProvider.h>

// Define templates for dummy protocol object
template class nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core>>;

class DummyProtocol : public cryptonote::i_cryptonote_protocol {
public:
  bool is_synchronized() const;
  bool relay_transactions(cryptonote::NOTIFY_NEW_TRANSACTIONS::request&, const boost::uuids::uuid&, epee::net_utils::zone, cryptonote::relay_method);
  bool relay_block(cryptonote::NOTIFY_NEW_FLUFFY_BLOCK::request&, cryptonote::cryptonote_connection_context&);
  bool get_payload_sync_data(cryptonote::CORE_SYNC_DATA&);
  bool get_payload_sync_data(epee::byte_slice&);
  bool on_idle();
  void on_connection_closing(cryptonote::cryptonote_connection_context&);
  void for_each_connection(std::function<bool(cryptonote::cryptonote_connection_context&, nodetool::peerid_type)>);
  void set_p2p_endpoint(nodetool::i_p2p_endpoint<cryptonote::cryptonote_connection_context>*);
  void set_update_block_timestamps();
};

struct RpcServerBundle {
  std::unique_ptr<cryptonote::t_cryptonote_protocol_handler<cryptonote::core>> proto_handler;
  std::unique_ptr<nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core>>> dummy_p2p;
  std::unique_ptr<cryptonote::core_rpc_server> rpc;
};

std::unique_ptr<cryptonote::core> initialise_rpc_core();
std::unique_ptr<RpcServerBundle> initialise_rpc_server(cryptonote::core&, FuzzedDataProvider&, bool);
bool generate_random_blocks(cryptonote::core&, FuzzedDataProvider&);
