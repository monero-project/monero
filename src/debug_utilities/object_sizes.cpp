// Copyright (c) 2017-2018, The Monero Project
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

#include <map>
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/tx_extra.h"
#include "cryptonote_core/blockchain.h"
#include "p2p/p2p_protocol_defs.h"
#include "net/connection_basic.hpp"
#include "p2p/net_peerlist.h"
#include "p2p/net_node.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "blockchain_db/lmdb/db_lmdb.h"
#include "wallet/wallet2.h"
#include "wallet/api/wallet.h"
#include "wallet/api/transaction_info.h"
#include "wallet/api/transaction_history.h"
#include "wallet/api/unsigned_transaction.h"
#include "wallet/api/pending_transaction.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "debugtools.objectsizes"

class size_logger
{
public:
  ~size_logger()
  {
    for (const auto &i: types)
      std::cout << std::to_string(i.first) << "\t" << i.second << std::endl;
  }
  void add(const char *type, size_t size) { types.insert(std::make_pair(size, type)); }
private:
  std::multimap<size_t, const std::string> types;
};
#define SL(type) sl.add(#type, sizeof(type))

int main(int argc, char* argv[])
{
  size_logger sl;

  tools::on_startup();

  mlog_configure("", true);

  SL(boost::thread);
  SL(boost::asio::io_service);
  SL(boost::asio::io_service::work);
  SL(boost::asio::deadline_timer);

  SL(cryptonote::DB_ERROR);
  SL(cryptonote::mdb_txn_safe);
  SL(cryptonote::mdb_threadinfo);

  SL(cryptonote::block_header);
  SL(cryptonote::block);
  SL(cryptonote::transaction_prefix);
  SL(cryptonote::transaction);

  SL(cryptonote::txpool_tx_meta_t);

  SL(epee::net_utils::ipv4_network_address);
  SL(epee::net_utils::network_address);
  SL(epee::net_utils::connection_context_base);
  SL(epee::net_utils::connection_basic);

  SL(nodetool::peerlist_entry);
  SL(nodetool::anchor_peerlist_entry);
  SL(nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core>>);
  SL(nodetool::p2p_connection_context_t<cryptonote::t_cryptonote_protocol_handler<cryptonote::core>::connection_context>);
  SL(nodetool::network_address_old);

  SL(tools::wallet2::transfer_details);
  SL(tools::wallet2::payment_details);
  SL(tools::wallet2::unconfirmed_transfer_details);
  SL(tools::wallet2::confirmed_transfer_details);
  SL(tools::wallet2::tx_construction_data);
  SL(tools::wallet2::pending_tx);
  SL(tools::wallet2::unsigned_tx_set);
  SL(tools::wallet2::signed_tx_set);

  SL(Monero::WalletImpl);
  SL(Monero::AddressBookRow);
  SL(Monero::TransactionInfoImpl);
  SL(Monero::TransactionHistoryImpl);
  SL(Monero::PendingTransactionImpl);
  SL(Monero::UnsignedTransactionImpl);

  return 0;
}
