#ifndef BLOCKCHAIN_OBJECTS_H
#define BLOCKCHAIN_OBJECTS_H

#include "cryptonote_core/blockchain.h"
#include "cryptonote_core/tx_pool.h"
#include "cryptonote_core/master_node_list.h"
#include "cryptonote_core/master_node_deregister.h"

// NOTE(beldex): This is done this way because of the circular constructors.
struct blockchain_objects_t
{
  cryptonote::Blockchain m_blockchain;
  cryptonote::tx_memory_pool m_mempool;
  master_nodes::master_node_list m_master_node_list;
  master_nodes::deregister_vote_pool m_deregister_vote_pool;
  blockchain_objects_t() :
    m_blockchain(m_mempool, m_master_node_list, m_deregister_vote_pool),
    m_master_node_list(m_blockchain),
    m_mempool(m_blockchain) { }
};

#endif // BLOCKCHAIN_OBJECTS_H
