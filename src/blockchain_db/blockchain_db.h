// Copyright (c) 2014-2017, The Monero Project
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
#ifndef BLOCKCHAIN_DB_H
#define BLOCKCHAIN_DB_H

#pragma once

#include <list>
#include <string>
#include <exception>
#include "crypto/hash.h"
#include "cryptonote_protocol/blobdatatype.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/difficulty.h"
#include "cryptonote_basic/hardfork.h"

/** \file
 * Cryptonote Blockchain Database Interface
 *
 * The DB interface is a store for the canonical block chain.
 * It serves as a persistent storage for the blockchain.
 *
 * For the sake of efficiency, a concrete implementation may also
 * store some blockchain data outside of the blocks, such as spent
 * transfer key images, unspent transaction outputs, etc.
 *
 * Examples are as follows:
 *
 * Transactions are duplicated so that we don't have to fetch a whole block
 * in order to fetch a transaction from that block.
 *
 * Spent key images are duplicated outside of the blocks so it is quick
 * to verify an output hasn't already been spent
 *
 * Unspent transaction outputs are duplicated to quickly gather random
 * outputs to use for mixins
 *
 * Indices and Identifiers:
 * The word "index" is used ambiguously throughout this code. It is
 * particularly confusing when talking about the output or transaction
 * tables since their indexing can refer to themselves or each other.
 * I have attempted to clarify these usages here:
 *
 * Blocks, transactions, and outputs are all identified by a hash.
 * For storage efficiency, a 64-bit integer ID is used instead of the hash
 * inside the DB. Tables exist to map between hash and ID. A block ID is
 * also referred to as its "height". Transactions and outputs generally are
 * not referred to by ID outside of this module, but the tx ID is returned
 * by tx_exists() and used by get_tx_amount_output_indices(). Like their
 * corresponding hashes, IDs are globally unique.
 *
 * The remaining uses of the word "index" refer to local offsets, and are
 * not globally unique. An "amount output index" N refers to the Nth output
 * of a specific amount. An "output local index" N refers to the Nth output
 * of a specific tx.
 *
 * Exceptions:
 *   DB_ERROR -- generic
 *   DB_OPEN_FAILURE
 *   DB_CREATE_FAILURE
 *   DB_SYNC_FAILURE
 *   BLOCK_DNE
 *   BLOCK_PARENT_DNE
 *   BLOCK_EXISTS
 *   BLOCK_INVALID -- considering making this multiple errors
 *   TX_DNE
 *   TX_EXISTS
 *   OUTPUT_DNE
 *   OUTPUT_EXISTS
 *   KEY_IMAGE_EXISTS
 */

namespace cryptonote
{

/** a pair of <transaction hash, output index>, typedef for convenience */
typedef std::pair<crypto::hash, uint64_t> tx_out_index;

#pragma pack(push, 1)

/**
 * @brief a struct containing output metadata
 */
struct output_data_t
{
  crypto::public_key pubkey;       //!< the output's public key (for spend verification)
  uint64_t           unlock_time;  //!< the output's unlock time (or height)
  uint64_t           height;       //!< the height of the block which created the output
  rct::key           commitment;   //!< the output's amount commitment (for spend verification)
};
#pragma pack(pop)

#pragma pack(push, 1)
struct tx_data_t
{
  uint64_t tx_id;
  uint64_t unlock_time;
  uint64_t block_id;
};
#pragma pack(pop)

/**
 * @brief a struct containing txpool per transaction metadata
 */
struct txpool_tx_meta_t
{
  crypto::hash max_used_block_id;
  crypto::hash last_failed_id;
  uint64_t blob_size;
  uint64_t fee;
  uint64_t max_used_block_height;
  uint64_t last_failed_height;
  uint64_t receive_time;
  uint64_t last_relayed_time;
  // 112 bytes
  uint8_t kept_by_block;
  uint8_t relayed;
  uint8_t do_not_relay;

  uint8_t padding[77]; // till 192 bytes
};

#define DBF_SAFE       1
#define DBF_FAST       2
#define DBF_FASTEST    4
#define DBF_RDONLY     8
#define DBF_SALVAGE 0x10

/***********************************
 * Exception Definitions
 ***********************************/

/**
 * @brief A base class for BlockchainDB exceptions
 */
class DB_EXCEPTION : public std::exception
{
  private:
    std::string m;

  protected:
    DB_EXCEPTION(const char *s) : m(s) { }

  public:
    virtual ~DB_EXCEPTION() { }

    const char* what() const throw()
    {
      return m.c_str();
    }
};

/**
 * @brief A generic BlockchainDB exception
 */
class DB_ERROR : public DB_EXCEPTION
{
  public:
    DB_ERROR() : DB_EXCEPTION("Generic DB Error") { }
    DB_ERROR(const char* s) : DB_EXCEPTION(s) { }
};

/**
 * @brief thrown when there is an error starting a DB transaction
 */
class DB_ERROR_TXN_START : public DB_EXCEPTION
{
  public:
    DB_ERROR_TXN_START() : DB_EXCEPTION("DB Error in starting txn") { }
    DB_ERROR_TXN_START(const char* s) : DB_EXCEPTION(s) { }
};

/**
 * @brief thrown when opening the BlockchainDB fails
 */
class DB_OPEN_FAILURE : public DB_EXCEPTION
{
  public:
    DB_OPEN_FAILURE() : DB_EXCEPTION("Failed to open the db") { }
    DB_OPEN_FAILURE(const char* s) : DB_EXCEPTION(s) { }
};

/**
 * @brief thrown when creating the BlockchainDB fails
 */
class DB_CREATE_FAILURE : public DB_EXCEPTION
{
  public:
    DB_CREATE_FAILURE() : DB_EXCEPTION("Failed to create the db") { }
    DB_CREATE_FAILURE(const char* s) : DB_EXCEPTION(s) { }
};

/**
 * @brief thrown when synchronizing the BlockchainDB to disk fails
 */
class DB_SYNC_FAILURE : public DB_EXCEPTION
{
  public:
    DB_SYNC_FAILURE() : DB_EXCEPTION("Failed to sync the db") { }
    DB_SYNC_FAILURE(const char* s) : DB_EXCEPTION(s) { }
};

/**
 * @brief thrown when a requested block does not exist
 */
class BLOCK_DNE : public DB_EXCEPTION
{
  public:
    BLOCK_DNE() : DB_EXCEPTION("The block requested does not exist") { }
    BLOCK_DNE(const char* s) : DB_EXCEPTION(s) { }
};

/**
 * @brief thrown when a block's parent does not exist (and it needed to)
 */
class BLOCK_PARENT_DNE : public DB_EXCEPTION
{
  public:
    BLOCK_PARENT_DNE() : DB_EXCEPTION("The parent of the block does not exist") { }
    BLOCK_PARENT_DNE(const char* s) : DB_EXCEPTION(s) { }
};

/**
 * @brief thrown when a block exists, but shouldn't, namely when adding a block
 */
class BLOCK_EXISTS : public DB_EXCEPTION
{
  public:
    BLOCK_EXISTS() : DB_EXCEPTION("The block to be added already exists!") { }
    BLOCK_EXISTS(const char* s) : DB_EXCEPTION(s) { }
};

/**
 * @brief thrown when something is wrong with the block to be added
 */
class BLOCK_INVALID : public DB_EXCEPTION
{
  public:
    BLOCK_INVALID() : DB_EXCEPTION("The block to be added did not pass validation!") { }
    BLOCK_INVALID(const char* s) : DB_EXCEPTION(s) { }
};

/**
 * @brief thrown when a requested transaction does not exist
 */
class TX_DNE : public DB_EXCEPTION
{
  public:
    TX_DNE() : DB_EXCEPTION("The transaction requested does not exist") { }
    TX_DNE(const char* s) : DB_EXCEPTION(s) { }
};

/**
 * @brief thrown when a transaction exists, but shouldn't, namely when adding a block
 */
class TX_EXISTS : public DB_EXCEPTION
{
  public:
    TX_EXISTS() : DB_EXCEPTION("The transaction to be added already exists!") { }
    TX_EXISTS(const char* s) : DB_EXCEPTION(s) { }
};

/**
 * @brief thrown when a requested output does not exist
 */
class OUTPUT_DNE : public DB_EXCEPTION
{
  public:
    OUTPUT_DNE() : DB_EXCEPTION("The output requested does not exist!") { }
    OUTPUT_DNE(const char* s) : DB_EXCEPTION(s) { }
};

/**
 * @brief thrown when an output exists, but shouldn't, namely when adding a block
 */
class OUTPUT_EXISTS : public DB_EXCEPTION
{
  public:
    OUTPUT_EXISTS() : DB_EXCEPTION("The output to be added already exists!") { }
    OUTPUT_EXISTS(const char* s) : DB_EXCEPTION(s) { }
};

/**
 * @brief thrown when a spent key image exists, but shouldn't, namely when adding a block
 */
class KEY_IMAGE_EXISTS : public DB_EXCEPTION
{
  public:
    KEY_IMAGE_EXISTS() : DB_EXCEPTION("The spent key image to be added already exists!") { }
    KEY_IMAGE_EXISTS(const char* s) : DB_EXCEPTION(s) { }
};

/***********************************
 * End of Exception Definitions
 ***********************************/


/**
 * @brief The BlockchainDB backing store interface declaration/contract
 *
 * This class provides a uniform interface for using BlockchainDB to store
 * a blockchain.  Any implementation of this class will also implement all
 * functions exposed here, so one can use this class without knowing what
 * implementation is being used.  Refer to each pure virtual function's
 * documentation here when implementing a BlockchainDB subclass.
 *
 * A subclass which encounters an issue should report that issue by throwing
 * a DB_EXCEPTION which adequately conveys the issue.
 */
class BlockchainDB
{
private:
  /*********************************************************************
   * private virtual members
   *********************************************************************/

  /**
   * @brief add the block and metadata to the db
   *
   * The subclass implementing this will add the specified block and
   * block metadata to its backing store.  This does not include its
   * transactions, those are added in a separate step.
   *
   * If any of this cannot be done, the subclass should throw the corresponding
   * subclass of DB_EXCEPTION
   *
   * @param blk the block to be added
   * @param block_size the size of the block (transactions and all)
   * @param cumulative_difficulty the accumulated difficulty after this block
   * @param coins_generated the number of coins generated total after this block
   * @param blk_hash the hash of the block
   */
  virtual void add_block( const block& blk
                , const size_t& block_size
                , const difficulty_type& cumulative_difficulty
                , const uint64_t& coins_generated
                , const crypto::hash& blk_hash
                ) = 0;

  /**
   * @brief remove data about the top block
   *
   * The subclass implementing this will remove the block data from the top
   * block in the chain.  The data to be removed is that which was added in
   * BlockchainDB::add_block(const block& blk, const size_t& block_size, const difficulty_type& cumulative_difficulty, const uint64_t& coins_generated, const crypto::hash& blk_hash)
   *
   * If any of this cannot be done, the subclass should throw the corresponding
   * subclass of DB_EXCEPTION
   */
  virtual void remove_block() = 0;

  /**
   * @brief store the transaction and its metadata
   *
   * The subclass implementing this will add the specified transaction data
   * to its backing store.  This includes only the transaction blob itself
   * and the other data passed here, not the separate outputs of the
   * transaction.
   *
   * It returns a tx ID, which is a mapping from the tx_hash. The tx ID
   * is used in #add_tx_amount_output_indices().
   *
   * If any of this cannot be done, the subclass should throw the corresponding
   * subclass of DB_EXCEPTION
   *
   * @param blk_hash the hash of the block containing the transaction
   * @param tx the transaction to be added
   * @param tx_hash the hash of the transaction
   * @return the transaction ID
   */
  virtual uint64_t add_transaction_data(const crypto::hash& blk_hash, const transaction& tx, const crypto::hash& tx_hash) = 0;

  /**
   * @brief remove data about a transaction
   *
   * The subclass implementing this will remove the transaction data 
   * for the passed transaction.  The data to be removed was added in
   * add_transaction_data().  Additionally, current subclasses have behavior
   * which requires the transaction itself as a parameter here.  Future
   * implementations should note that this parameter is subject to be removed
   * at a later time.
   *
   * If any of this cannot be done, the subclass should throw the corresponding
   * subclass of DB_EXCEPTION
   *
   * @param tx_hash the hash of the transaction to be removed
   * @param tx the transaction
   */
  virtual void remove_transaction_data(const crypto::hash& tx_hash, const transaction& tx) = 0;

  /**
   * @brief store an output
   *
   * The subclass implementing this will add the output data passed to its
   * backing store in a suitable manner.  In addition, the subclass is responsible
   * for keeping track of the global output count in some manner, so that
   * outputs may be indexed by the order in which they were created.  In the
   * future, this tracking (of the number, at least) should be moved to
   * this class, as it is necessary and the same among all BlockchainDB.
   *
   * It returns an amount output index, which is the index of the output
   * for its specified amount.
   *
   * This data should be stored in such a manner that the only thing needed to
   * reverse the process is the tx_out.
   *
   * If any of this cannot be done, the subclass should throw the corresponding
   * subclass of DB_EXCEPTION
   *
   * @param tx_hash hash of the transaction the output was created by
   * @param tx_output the output
   * @param local_index index of the output in its transaction
   * @param unlock_time unlock time/height of the output
   * @param commitment the rct commitment to the output amount
   * @return amount output index
   */
  virtual uint64_t add_output(const crypto::hash& tx_hash, const tx_out& tx_output, const uint64_t& local_index, const uint64_t unlock_time, const rct::key *commitment) = 0;

  /**
   * @brief store amount output indices for a tx's outputs
   *
   * The subclass implementing this will add the amount output indices to its
   * backing store in a suitable manner. The tx_id will be the same one that
   * was returned from #add_output().
   *
   * If any of this cannot be done, the subclass should throw the corresponding
   * subclass of DB_EXCEPTION
   *
   * @param tx_id ID of the transaction containing these outputs
   * @param amount_output_indices the amount output indices of the transaction
   */
  virtual void add_tx_amount_output_indices(const uint64_t tx_id, const std::vector<uint64_t>& amount_output_indices) = 0;

  /**
   * @brief store a spent key
   *
   * The subclass implementing this will store the spent key image.
   *
   * If any of this cannot be done, the subclass should throw the corresponding
   * subclass of DB_EXCEPTION
   *
   * @param k_image the spent key image to store
   */
  virtual void add_spent_key(const crypto::key_image& k_image) = 0;

  /**
   * @brief remove a spent key
   *
   * The subclass implementing this will remove the key image.
   *
   * If any of this cannot be done, the subclass should throw the corresponding
   * subclass of DB_EXCEPTION
   *
   * @param k_image the spent key image to remove
   */
  virtual void remove_spent_key(const crypto::key_image& k_image) = 0;


  /*********************************************************************
   * private concrete members
   *********************************************************************/
  /**
   * @brief private version of pop_block, for undoing if an add_block fails
   *
   * This function simply calls pop_block(block& blk, std::vector<transaction>& txs)
   * with dummy parameters, as the returns-by-reference can be discarded.
   */
  void pop_block();

  // helper function to remove transaction from blockchain
  /**
   * @brief helper function to remove transaction from the blockchain
   *
   * This function encapsulates aspects of removing a transaction.
   *
   * @param tx_hash the hash of the transaction to be removed
   */
  void remove_transaction(const crypto::hash& tx_hash);

  uint64_t num_calls = 0;  //!< a performance metric
  uint64_t time_blk_hash = 0;  //!< a performance metric
  uint64_t time_add_block1 = 0;  //!< a performance metric
  uint64_t time_add_transaction = 0;  //!< a performance metric


protected:

  /**
   * @brief helper function for add_transactions, to add each individual transaction
   *
   * This function is called by add_transactions() for each transaction to be
   * added.
   *
   * @param blk_hash hash of the block which has the transaction
   * @param tx the transaction to add
   * @param tx_hash_ptr the hash of the transaction, if already calculated
   */
  void add_transaction(const crypto::hash& blk_hash, const transaction& tx, const crypto::hash* tx_hash_ptr = NULL);

  mutable uint64_t time_tx_exists = 0;  //!< a performance metric
  uint64_t time_commit1 = 0;  //!< a performance metric
  bool m_auto_remove_logs = true;  //!< whether or not to automatically remove old logs

  HardFork* m_hardfork;

public:

  /**
   * @brief An empty destructor.
   */
  virtual ~BlockchainDB() { };

  /**
   * @brief reset profiling stats
   */
  void reset_stats();

  /**
   * @brief show profiling stats
   *
   * This function prints current performance/profiling data to whichever
   * log file(s) are set up (possibly including stdout or stderr)
   */
  void show_stats();

  /**
   * @brief open a db, or create it if necessary.
   *
   * This function opens an existing database or creates it if it
   * does not exist.
   *
   * The subclass implementing this will handle all file opening/creation,
   * and is responsible for maintaining its state.
   *
   * The parameter <filename> may not refer to a file name, necessarily, but
   * could be an IP:PORT for a database which needs it, and so on.  Calling it
   * <filename> is convenient and should be descriptive enough, however.
   *
   * For now, db_flags are
   * specific to the subclass being instantiated.  This is subject to change,
   * and the db_flags parameter may be deprecated.
   *
   * If any of this cannot be done, the subclass should throw the corresponding
   * subclass of DB_EXCEPTION
   *
   * @param filename a string referring to the BlockchainDB to open
   * @param db_flags flags relevant to how to open/use the BlockchainDB
   */
  virtual void open(const std::string& filename, const int db_flags = 0) = 0;

  /**
   * @brief Gets the current open/ready state of the BlockchainDB
   *
   * @return true if open/ready, otherwise false
   */
  bool is_open() const;

  /**
   * @brief close the BlockchainDB
   *
   * At minimum, this call ensures that further use of the BlockchainDB
   * instance will not have effect.  In any case where it is necessary
   * to do so, a subclass implementing this will sync with disk.
   *
   * If any of this cannot be done, the subclass should throw the corresponding
   * subclass of DB_EXCEPTION
   */
  virtual void close() = 0;

  /**
   * @brief sync the BlockchainDB with disk
   *
   * This function should write any changes to whatever permanent backing
   * store the subclass uses.  Example: a BlockchainDB instance which
   * keeps the whole blockchain in RAM won't need to regularly access a
   * disk, but should write out its state when this is called.
   *
   * If any of this cannot be done, the subclass should throw the corresponding
   * subclass of DB_EXCEPTION
   */
  virtual void sync() = 0;

  /**
   * @brief toggle safe syncs for the DB
   *
   * Used to switch DBF_SAFE on or off after starting up with DBF_FAST.
   */
  virtual void safesyncmode(const bool onoff) = 0;

  /**
   * @brief Remove everything from the BlockchainDB
   *
   * This function should completely remove all data from a BlockchainDB.
   *
   * Use with caution!
   *
   * If any of this cannot be done, the subclass should throw the corresponding
   * subclass of DB_EXCEPTION
   */
  virtual void reset() = 0;

  /**
   * @brief get all files used by the BlockchainDB (if any)
   *
   * This function is largely for ease of automation, namely for unit tests.
   *
   * The subclass implementation should return all filenames it uses.
   *
   * @return a list of filenames
   */
  virtual std::vector<std::string> get_filenames() const = 0;

  // return the name of the folder the db's file(s) should reside in
  /**
   * @brief gets the name of the folder the BlockchainDB's file(s) should be in
   *
   * The subclass implementation should return the name of the folder in which
   * it stores files, or an empty string if there is none.
   *
   * @return the name of the folder with the BlockchainDB's files, if any.
   */
  virtual std::string get_db_name() const = 0;


  // FIXME: these are just for functionality mocking, need to implement
  // RAII-friendly and multi-read one-write friendly locking mechanism
  //
  // acquire db lock
  /**
   * @brief acquires the BlockchainDB lock
   *
   * This function is a stub until such a time as locking is implemented at
   * this level.
   *
   * The subclass implementation should return true unless implementing a
   * locking scheme of some sort, in which case it should return true upon
   * acquisition of the lock and block until then.
   *
   * If any of this cannot be done, the subclass should throw the corresponding
   * subclass of DB_EXCEPTION
   *
   * @return true, unless at a future time false makes sense (timeout, etc)
   */
  virtual bool lock() = 0;

  // release db lock
  /**
   * @brief This function releases the BlockchainDB lock
   *
   * The subclass, should it have implemented lock(), will release any lock
   * held by the calling thread.  In the case of recursive locking, it should
   * release one instance of a lock.
   *
   * If any of this cannot be done, the subclass should throw the corresponding
   * subclass of DB_EXCEPTION
   */
  virtual void unlock() = 0;

  /**
   * @brief tells the BlockchainDB to start a new "batch" of blocks
   *
   * If the subclass implements a batching method of caching blocks in RAM to
   * be added to a backing store in groups, it should start a batch which will
   * end either when <batch_num_blocks> has been added or batch_stop() has
   * been called.  In either case, it should end the batch and write to its
   * backing store.
   *
   * If a batch is already in-progress, this function must return false.
   * If a batch was started by this call, it must return true.
   *
   * If any of this cannot be done, the subclass should throw the corresponding
   * subclass of DB_EXCEPTION
   *
   * @param batch_num_blocks number of blocks to batch together
   *
   * @return true if we started the batch, false if already started
   */
  virtual bool batch_start(uint64_t batch_num_blocks=0) = 0;

  /**
   * @brief ends a batch transaction
   *
   * If the subclass implements batching, this function should store the
   * batch it is currently on and mark it finished.
   *
   * If no batch is in-progress, this function should throw a DB_ERROR.
   * This exception may change in the future if it is deemed necessary to
   * have a more granular exception type for this scenario.
   *
   * If any of this cannot be done, the subclass should throw the corresponding
   * subclass of DB_EXCEPTION
   */
  virtual void batch_stop() = 0;

  /**
   * @brief sets whether or not to batch transactions
   *
   * If the subclass implements batching, this function tells it to begin
   * batching automatically.
   *
   * If the subclass implements batching and has a batch in-progress, a
   * parameter of false should disable batching and call batch_stop() to
   * store the current batch.
   *
   * If any of this cannot be done, the subclass should throw the corresponding
   * subclass of DB_EXCEPTION
   *
   * @param bool batch whether or not to use batch transactions.
   */
  virtual void set_batch_transactions(bool) = 0;

  virtual void block_txn_start(bool readonly=false) = 0;
  virtual void block_txn_stop() = 0;
  virtual void block_txn_abort() = 0;

  virtual void set_hard_fork(HardFork* hf);

  // adds a block with the given metadata to the top of the blockchain, returns the new height
  /**
   * @brief handles the addition of a new block to BlockchainDB
   *
   * This function organizes block addition and calls various functions as
   * necessary.
   *
   * NOTE: subclass implementations of this (or the functions it calls) need
   * to handle undoing any partially-added blocks in the event of a failure.
   *
   * If any of this cannot be done, the subclass should throw the corresponding
   * subclass of DB_EXCEPTION
   *
   * @param blk the block to be added
   * @param block_size the size of the block (transactions and all)
   * @param cumulative_difficulty the accumulated difficulty after this block
   * @param coins_generated the number of coins generated total after this block
   * @param txs the transactions in the block
   *
   * @return the height of the chain post-addition
   */
  virtual uint64_t add_block( const block& blk
                            , const size_t& block_size
                            , const difficulty_type& cumulative_difficulty
                            , const uint64_t& coins_generated
                            , const std::vector<transaction>& txs
                            );

  /**
   * @brief checks if a block exists
   *
   * @param h the hash of the requested block
   * @param height if non NULL, returns the block's height if found
   *
   * @return true of the block exists, otherwise false
   */
  virtual bool block_exists(const crypto::hash& h, uint64_t *height = NULL) const = 0;

  /**
   * @brief fetches the block with the given hash
   *
   * The subclass should return the requested block.
   *
   * If the block does not exist, the subclass should throw BLOCK_DNE
   *
   * @param h the hash to look for
   *
   * @return the block requested
   */
  virtual cryptonote::blobdata get_block_blob(const crypto::hash& h) const = 0;

  /**
   * @brief fetches the block with the given hash
   *
   * Returns the requested block.
   *
   * If the block does not exist, the subclass should throw BLOCK_DNE
   *
   * @param h the hash to look for
   *
   * @return the block requested
   */
  virtual block get_block(const crypto::hash& h) const;

  /**
   * @brief gets the height of the block with a given hash
   *
   * The subclass should return the requested height.
   *
   * If the block does not exist, the subclass should throw BLOCK_DNE
   *
   * @param h the hash to look for
   *
   * @return the height
   */
  virtual uint64_t get_block_height(const crypto::hash& h) const = 0;

  /**
   * @brief fetch a block header
   *
   * The subclass should return the block header from the block with
   * the given hash.
   *
   * If the block does not exist, the subclass should throw BLOCK_DNE
   *
   * @param h the hash to look for
   *
   * @return the block header
   */
  virtual block_header get_block_header(const crypto::hash& h) const = 0;

  /**
   * @brief fetch a block blob by height
   *
   * The subclass should return the block at the given height.
   *
   * If the block does not exist, that is to say if the blockchain is not
   * that high, then the subclass should throw BLOCK_DNE
   *
   * @param height the height to look for
   *
   * @return the block blob
   */
  virtual cryptonote::blobdata get_block_blob_from_height(const uint64_t& height) const = 0;

  /**
   * @brief fetch a block by height
   *
   * If the block does not exist, that is to say if the blockchain is not
   * that high, then the subclass should throw BLOCK_DNE
   *
   * @param height the height to look for
   *
   * @return the block
   */
  virtual block get_block_from_height(const uint64_t& height) const;

  /**
   * @brief fetch a block's timestamp
   *
   * The subclass should return the timestamp of the block with the
   * given height.
   *
   * If the block does not exist, the subclass should throw BLOCK_DNE
   *
   * @param height the height requested
   *
   * @return the timestamp
   */
  virtual uint64_t get_block_timestamp(const uint64_t& height) const = 0;

  /**
   * @brief fetch the top block's timestamp
   *
   * The subclass should return the timestamp of the most recent block.
   *
   * @return the top block's timestamp
   */
  virtual uint64_t get_top_block_timestamp() const = 0;

  /**
   * @brief fetch a block's size
   *
   * The subclass should return the size of the block with the
   * given height.
   *
   * If the block does not exist, the subclass should throw BLOCK_DNE
   *
   * @param height the height requested
   *
   * @return the size
   */
  virtual size_t get_block_size(const uint64_t& height) const = 0;

  /**
   * @brief fetch a block's cumulative difficulty
   *
   * The subclass should return the cumulative difficulty of the block with the
   * given height.
   *
   * If the block does not exist, the subclass should throw BLOCK_DNE
   *
   * @param height the height requested
   *
   * @return the cumulative difficulty
   */
  virtual difficulty_type get_block_cumulative_difficulty(const uint64_t& height) const = 0;

  /**
   * @brief fetch a block's difficulty
   *
   * The subclass should return the difficulty of the block with the
   * given height.
   *
   * If the block does not exist, the subclass should throw BLOCK_DNE
   *
   * @param height the height requested
   *
   * @return the difficulty
   */
  virtual difficulty_type get_block_difficulty(const uint64_t& height) const = 0;

  /**
   * @brief fetch a block's already generated coins
   *
   * The subclass should return the total coins generated as of the block
   * with the given height.
   *
   * If the block does not exist, the subclass should throw BLOCK_DNE
   *
   * @param height the height requested
   *
   * @return the already generated coins
   */
  virtual uint64_t get_block_already_generated_coins(const uint64_t& height) const = 0;

  /**
   * @brief fetch a block's hash
   *
   * The subclass should return hash of the block with the
   * given height.
   *
   * If the block does not exist, the subclass should throw BLOCK_DNE
   *
   * @param height the height requested
   *
   * @return the hash
   */
  virtual crypto::hash get_block_hash_from_height(const uint64_t& height) const = 0;

  /**
   * @brief fetch a list of blocks
   *
   * The subclass should return a vector of blocks with heights starting at
   * h1 and ending at h2, inclusively.
   *
   * If the height range requested goes past the end of the blockchain,
   * the subclass should throw BLOCK_DNE.  (current implementations simply
   * don't catch this exception as thrown by methods called within)
   *
   * @param h1 the start height
   * @param h2 the end height
   *
   * @return a vector of blocks
   */
  virtual std::vector<block> get_blocks_range(const uint64_t& h1, const uint64_t& h2) const = 0;

  /**
   * @brief fetch a list of block hashes
   *
   * The subclass should return a vector of block hashes from blocks with
   * heights starting at h1 and ending at h2, inclusively.
   *
   * If the height range requested goes past the end of the blockchain,
   * the subclass should throw BLOCK_DNE.  (current implementations simply
   * don't catch this exception as thrown by methods called within)
   *
   * @param h1 the start height
   * @param h2 the end height
   *
   * @return a vector of block hashes
   */
  virtual std::vector<crypto::hash> get_hashes_range(const uint64_t& h1, const uint64_t& h2) const = 0;

  /**
   * @brief fetch the top block's hash
   *
   * The subclass should return the hash of the most recent block
   *
   * @return the top block's hash
   */
  virtual crypto::hash top_block_hash() const = 0;

  /**
   * @brief fetch the top block
   *
   * The subclass should return most recent block
   *
   * @return the top block
   */
  virtual block get_top_block() const = 0;

  /**
   * @brief fetch the current blockchain height
   *
   * The subclass should return the current blockchain height
   *
   * @return the current blockchain height
   */
  virtual uint64_t height() const = 0;


  /**
   * <!--
   * TODO: Rewrite (if necessary) such that all calls to remove_* are
   *       done in concrete members of this base class.
   * -->
   *
   * @brief pops the top block off the blockchain
   *
   * The subclass should remove the most recent block from the blockchain,
   * along with all transactions, outputs, and other metadata created as
   * a result of its addition to the blockchain.  Most of this is handled
   * by the concrete members of the base class provided the subclass correctly
   * implements remove_* functions.
   *
   * The subclass should return by reference the popped block and
   * its associated transactions
   *
   * @param blk return-by-reference the block which was popped
   * @param txs return-by-reference the transactions from the popped block
   */
  virtual void pop_block(block& blk, std::vector<transaction>& txs);


  /**
   * @brief check if a transaction with a given hash exists
   *
   * The subclass should check if a transaction is stored which has the
   * given hash and return true if so, false otherwise.
   *
   * @param h the hash to check against
   * @param tx_id (optional) returns the tx_id for the tx hash
   *
   * @return true if the transaction exists, otherwise false
   */
  virtual bool tx_exists(const crypto::hash& h) const = 0;
  virtual bool tx_exists(const crypto::hash& h, uint64_t& tx_id) const = 0;

  // return unlock time of tx with hash <h>
  /**
   * @brief fetch a transaction's unlock time/height
   *
   * The subclass should return the stored unlock time for the transaction
   * with the given hash.
   *
   * If no such transaction exists, the subclass should throw TX_DNE.
   *
   * @param h the hash of the requested transaction
   *
   * @return the unlock time/height
   */
  virtual uint64_t get_tx_unlock_time(const crypto::hash& h) const = 0;

  // return tx with hash <h>
  // throw if no such tx exists
  /**
   * @brief fetches the transaction with the given hash
   *
   * If the transaction does not exist, the subclass should throw TX_DNE.
   *
   * @param h the hash to look for
   *
   * @return the transaction with the given hash
   */
  virtual transaction get_tx(const crypto::hash& h) const;

  /**
   * @brief fetches the transaction with the given hash
   *
   * If the transaction does not exist, the subclass should return false.
   *
   * @param h the hash to look for
   *
   * @return true iff the transaction was found
   */
  virtual bool get_tx(const crypto::hash& h, transaction &tx) const;

  /**
   * @brief fetches the transaction blob with the given hash
   *
   * The subclass should return the transaction stored which has the given
   * hash.
   *
   * If the transaction does not exist, the subclass should return false.
   *
   * @param h the hash to look for
   *
   * @return true iff the transaction was found
   */
  virtual bool get_tx_blob(const crypto::hash& h, cryptonote::blobdata &tx) const = 0;

  /**
   * @brief fetches the total number of transactions ever
   *
   * The subclass should return a count of all the transactions from
   * all blocks.
   *
   * @return the number of transactions in the blockchain
   */
  virtual uint64_t get_tx_count() const = 0;

  /**
   * @brief fetches a list of transactions based on their hashes
   *
   * The subclass should attempt to fetch each transaction referred to by
   * the hashes passed.
   *
   * Currently, if any of the transactions is not in BlockchainDB, the call
   * to get_tx in the implementation will throw TX_DNE.
   *
   * <!-- TODO: decide if this behavior is correct for missing transactions -->
   *
   * @param hlist a list of hashes
   *
   * @return the list of transactions
   */
  virtual std::vector<transaction> get_tx_list(const std::vector<crypto::hash>& hlist) const = 0;

  // returns height of block that contains transaction with hash <h>
  /**
   * @brief fetches the height of a transaction's block
   *
   * The subclass should attempt to return the height of the block containing
   * the transaction with the given hash.
   *
   * If the transaction cannot be found, the subclass should throw TX_DNE.
   *
   * @param h the hash of the transaction
   *
   * @return the height of the transaction's block
   */
  virtual uint64_t get_tx_block_height(const crypto::hash& h) const = 0;

  // returns the total number of outputs of amount <amount>
  /**
   * @brief fetches the number of outputs of a given amount
   *
   * The subclass should return a count of outputs of the given amount,
   * or zero if there are none.
   *
   * <!-- TODO: should outputs spent with a low mixin (especially 0) be
   * excluded from the count? -->
   *
   * @param amount the output amount being looked up
   *
   * @return the number of outputs of the given amount
   */
  virtual uint64_t get_num_outputs(const uint64_t& amount) const = 0;

  /**
   * @brief return index of the first element (should be hidden, but isn't)
   *
   * @return the index
   */
  virtual uint64_t get_indexing_base() const { return 0; }

  /**
   * @brief get some of an output's data
   *
   * The subclass should return the public key, unlock time, and block height
   * for the output with the given amount and index, collected in a struct.
   *
   * If the output cannot be found, the subclass should throw OUTPUT_DNE.
   *
   * If any of these parts cannot be found, but some are, the subclass
   * should throw DB_ERROR with a message stating as much.
   *
   * @param amount the output amount
   * @param index the output's index (indexed by amount)
   *
   * @return the requested output data
   */
  virtual output_data_t get_output_key(const uint64_t& amount, const uint64_t& index) = 0;

  /**
   * @brief get some of an output's data
   *
   * The subclass should return the public key, unlock time, and block height
   * for the output with the given global index, collected in a struct.
   *
   * If the output cannot be found, the subclass should throw OUTPUT_DNE.
   *
   * If any of these parts cannot be found, but some are, the subclass
   * should throw DB_ERROR with a message stating as much.
   *
   * @param global_index the output's index (global)
   *
   * @return the requested output data
   */
  virtual output_data_t get_output_key(const uint64_t& global_index) const = 0;

  /**
   * @brief gets an output's tx hash and index
   *
   * The subclass should return the hash of the transaction which created the
   * output with the global index given, as well as its index in that transaction.
   *
   * @param index an output's global index
   *
   * @return the tx hash and output index
   */
  virtual tx_out_index get_output_tx_and_index_from_global(const uint64_t& index) const = 0;

  /**
   * @brief gets an output's tx hash and index
   *
   * The subclass should return the hash of the transaction which created the
   * output with the amount and index given, as well as its index in that
   * transaction.
   *
   * @param amount an output amount
   * @param index an output's amount-specific index
   *
   * @return the tx hash and output index
   */
  virtual tx_out_index get_output_tx_and_index(const uint64_t& amount, const uint64_t& index) const = 0;

  /**
   * @brief gets some outputs' tx hashes and indices
   *
   * This function is a mirror of
   * get_output_tx_and_index(const uint64_t& amount, const uint64_t& index),
   * but for a list of outputs rather than just one.
   *
   * @param amount an output amount
   * @param offsets a list of amount-specific output indices
   * @param indices return-by-reference a list of tx hashes and output indices (as pairs)
   */
  virtual void get_output_tx_and_index(const uint64_t& amount, const std::vector<uint64_t> &offsets, std::vector<tx_out_index> &indices) const = 0;

  /**
   * @brief gets outputs' data
   *
   * This function is a mirror of
   * get_output_data(const uint64_t& amount, const uint64_t& index)
   * but for a list of outputs rather than just one.
   *
   * @param amount an output amount
   * @param offsets a list of amount-specific output indices
   * @param outputs return-by-reference a list of outputs' metadata
   */
  virtual void get_output_key(const uint64_t &amount, const std::vector<uint64_t> &offsets, std::vector<output_data_t> &outputs, bool allow_partial = false) = 0;
  
  /*
   * FIXME: Need to check with git blame and ask what this does to
   * document it
   */
  virtual bool can_thread_bulk_indices() const = 0;

  /**
   * @brief gets output indices (amount-specific) for a transaction's outputs
   *
   * The subclass should fetch the amount-specific output indices for each
   * output in the transaction with the given ID.
   *
   * If the transaction does not exist, the subclass should throw TX_DNE.
   *
   * If an output cannot be found, the subclass should throw OUTPUT_DNE.
   *
   * @param tx_id a transaction ID
   *
   * @return a list of amount-specific output indices
   */
  virtual std::vector<uint64_t> get_tx_amount_output_indices(const uint64_t tx_id) const = 0;

  /**
   * @brief check if a key image is stored as spent
   *
   * @param img the key image to check for
   *
   * @return true if the image is present, otherwise false
   */
  virtual bool has_key_image(const crypto::key_image& img) const = 0;

  /**
   * @brief add a txpool transaction
   *
   * @param details the details of the transaction to add
   */
  virtual void add_txpool_tx(const transaction &tx, const txpool_tx_meta_t& details) = 0;

  /**
   * @brief update a txpool transaction's metadata
   *
   * @param txid the txid of the transaction to update
   * @param details the details of the transaction to update
   */
  virtual void update_txpool_tx(const crypto::hash &txid, const txpool_tx_meta_t& details) = 0;

  /**
   * @brief get the number of transactions in the txpool
   */
  virtual uint64_t get_txpool_tx_count() const = 0;

  /**
   * @brief check whether a txid is in the txpool
   */
  virtual bool txpool_has_tx(const crypto::hash &txid) const = 0;

  /**
   * @brief remove a txpool transaction
   *
   * @param txid the transaction id of the transation to remove
   */
  virtual void remove_txpool_tx(const crypto::hash& txid) = 0;

  /**
   * @brief get a txpool transaction's metadata
   *
   * @param txid the transaction id of the transation to lookup
   *
   * @return the metadata associated with that transaction
   */
  virtual txpool_tx_meta_t get_txpool_tx_meta(const crypto::hash& txid) const = 0;

  /**
   * @brief get a txpool transaction's blob
   *
   * @param txid the transaction id of the transation to lookup
   * @param bd the blob to return
   *
   * @return true if the txid was in the txpool, false otherwise
   */
  virtual bool get_txpool_tx_blob(const crypto::hash& txid, cryptonote::blobdata &bd) const = 0;

  /**
   * @brief get a txpool transaction's blob
   *
   * @param txid the transaction id of the transation to lookup
   *
   * @return the blob for that transaction
   */
  virtual cryptonote::blobdata get_txpool_tx_blob(const crypto::hash& txid) const = 0;

  /**
   * @brief runs a function over all txpool transactions
   *
   * The subclass should run the passed function for each txpool tx it has
   * stored, passing the tx id and metadata as its parameters.
   *
   * If any call to the function returns false, the subclass should return
   * false.  Otherwise, the subclass returns true.
   *
   * @param std::function fn the function to run
   *
   * @return false if the function returns false for any transaction, otherwise true
   */
  virtual bool for_all_txpool_txes(std::function<bool(const crypto::hash&, const txpool_tx_meta_t&, const cryptonote::blobdata*)>, bool include_blob = false) const = 0;

  /**
   * @brief runs a function over all key images stored
   *
   * The subclass should run the passed function for each key image it has
   * stored, passing the key image as its parameter.
   *
   * If any call to the function returns false, the subclass should return
   * false.  Otherwise, the subclass returns true.
   *
   * @param std::function fn the function to run
   *
   * @return false if the function returns false for any key image, otherwise true
   */
  virtual bool for_all_key_images(std::function<bool(const crypto::key_image&)>) const = 0;

  /**
   * @brief runs a function over a range of blocks
   *
   * The subclass should run the passed function for each block in the
   * specified range, passing (block_height, block_hash, block) as its parameters.
   *
   * If any call to the function returns false, the subclass should return
   * false.  Otherwise, the subclass returns true.
   *
   * The subclass should throw DB_ERROR if any of the expected values are
   * not found.  Current implementations simply return false.
   *
   * @param h1 the start height
   * @param h2 the end height
   * @param std::function fn the function to run
   *
   * @return false if the function returns false for any block, otherwise true
   */
  virtual bool for_blocks_range(const uint64_t& h1, const uint64_t& h2, std::function<bool(uint64_t, const crypto::hash&, const cryptonote::block&)>) const = 0;

  /**
   * @brief runs a function over all transactions stored
   *
   * The subclass should run the passed function for each transaction it has
   * stored, passing (transaction_hash, transaction) as its parameters.
   *
   * If any call to the function returns false, the subclass should return
   * false.  Otherwise, the subclass returns true.
   *
   * The subclass should throw DB_ERROR if any of the expected values are
   * not found.  Current implementations simply return false.
   *
   * @param std::function fn the function to run
   *
   * @return false if the function returns false for any transaction, otherwise true
   */
  virtual bool for_all_transactions(std::function<bool(const crypto::hash&, const cryptonote::transaction&)>) const = 0;

  /**
   * @brief runs a function over all outputs stored
   *
   * The subclass should run the passed function for each output it has
   * stored, passing (amount, transaction_hash, tx_local_output_index)
   * as its parameters.
   *
   * If any call to the function returns false, the subclass should return
   * false.  Otherwise, the subclass returns true.
   *
   * The subclass should throw DB_ERROR if any of the expected values are
   * not found.  Current implementations simply return false.
   *
   * @param std::function f the function to run
   *
   * @return false if the function returns false for any output, otherwise true
   */
  virtual bool for_all_outputs(std::function<bool(uint64_t amount, const crypto::hash &tx_hash, size_t tx_idx)> f) const = 0;


  //
  // Hard fork related storage
  //

  /**
   * @brief sets which hardfork version a height is on
   *
   * @param height the height
   * @param version the version
   */
  virtual void set_hard_fork_version(uint64_t height, uint8_t version) = 0;

  /**
   * @brief checks which hardfork version a height is on
   *
   * @param height the height
   *
   * @return the version
   */
  virtual uint8_t get_hard_fork_version(uint64_t height) const = 0;

  /**
   * @brief verify hard fork info in database
   */
  virtual void check_hard_fork_info() = 0;

  /**
   * @brief delete hard fork info from database
   */
  virtual void drop_hard_fork_info() = 0;

  /**
   * @brief return a histogram of outputs on the blockchain
   *
   * @param amounts optional set of amounts to lookup
   * @param unlocked whether to restrict count to unlocked outputs
   * @param recent_cutoff timestamp to determine whether an output is recent
   *
   * @return a set of amount/instances
   */
  virtual std::map<uint64_t, std::tuple<uint64_t, uint64_t, uint64_t>> get_output_histogram(const std::vector<uint64_t> &amounts, bool unlocked, uint64_t recent_cutoff) const = 0;

  /**
   * @brief is BlockchainDB in read-only mode?
   *
   * @return true if in read-only mode, otherwise false
   */
  virtual bool is_read_only() const = 0;

  // TODO: this should perhaps be (or call) a series of functions which
  // progressively update through version updates
  /**
   * @brief fix up anything that may be wrong due to past bugs
   */
  virtual void fixup();

  /**
   * @brief set whether or not to automatically remove logs
   *
   * This function is only relevant for one implementation (BlockchainBDB), but
   * is here to keep BlockchainDB users implementation-agnostic.
   *
   * @param auto_remove whether or not to auto-remove logs
   */
  void set_auto_remove_logs(bool auto_remove) { m_auto_remove_logs = auto_remove; }

  bool m_open;  //!< Whether or not the BlockchainDB is open/ready for use
  mutable epee::critical_section m_synchronization_lock;  //!< A lock, currently for when BlockchainLMDB needs to resize the backing db file

};  // class BlockchainDB

BlockchainDB *new_db(const std::string& db_type);

}  // namespace cryptonote

#endif  // BLOCKCHAIN_DB_H
