// Copyright (c) 2014, The Monero Project
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

#include "cryptonote_core/blockchain_db.h"
#include "cryptonote_format_utils.h"

namespace cryptonote
{

void BlockchainDB::pop_block()
{
  block blk;
  std::vector<transaction> txs;
  pop_block(blk, txs);
}

void BlockchainDB::add_transaction(const crypto::hash& blk_hash, const transaction& tx)
{
  crypto::hash tx_hash = get_transaction_hash(tx);

  add_transaction_data(blk_hash, tx);

  // iterate tx.vout using indices instead of C++11 foreach syntax because
  // we need the index
  for (uint64_t i = 0; i < tx.vout.size(); ++i)
  {
    add_output(tx_hash, tx.vout[i], i);
  }

  for (const txin_v& tx_input : tx.vin)
  {
    if (tx_input.type() == typeid(txin_to_key))
    {
      add_spent_key(boost::get<txin_to_key>(tx_input).k_image);
    }
  }
}

uint64_t BlockchainDB::add_block( const block& blk
                  , const size_t& block_size
                  , const difficulty_type& cumulative_difficulty
                  , const uint64_t& coins_generated
                  , const std::vector<transaction>& txs
                  )
{
  try
  {
    // call out to subclass implementation to add the block & metadata
    add_block(blk, block_size, cumulative_difficulty, coins_generated);

    crypto::hash blk_hash = get_block_hash(blk);
    // call out to add the transactions
    for (const transaction& tx : txs)
    {
      add_transaction(blk_hash, tx);
    }
  }
  // in case any of the add_block process goes awry, undo
  catch (const std::exception& e)
  {
    LOG_ERROR("Error adding block to db: " << e.what());
    try
    {
      pop_block();
    }
    // if undoing goes wrong as well, we need to throw, as blockchain
    // will be in a bad state
    catch (const std::exception& e)
    {
      LOG_ERROR("Error undoing partially added block: " << e.what());
      throw;
    }
    throw;
  }

  return height();
}

void BlockchainDB::pop_block(block& blk, std::vector<transaction>& txs)
{
  blk = get_top_block();
  
  for (const auto& h : blk.tx_hashes)
  {
    txs.push_back(get_tx(h));
    remove_transaction(h);
  }
}

void BlockchainDB::remove_transaction(const crypto::hash& tx_hash)
{
  transaction tx = get_tx(tx_hash);

  for (const tx_out& tx_output : tx.vout)
  {
    remove_output(tx_output);
  }

  for (const txin_v& tx_input : tx.vin)
  {
    if (tx_input.type() == typeid(txin_to_key))
    {
      remove_spent_key(boost::get<txin_to_key>(tx_input).k_image);
    }
  }

  remove_transaction_data(tx_hash);
}

}  // namespace cryptonote
