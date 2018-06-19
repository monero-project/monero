// Copyright (c) 2014-2018, The Monero Project
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

#include "db_bdb.h"

#include <boost/filesystem.hpp>
#include <memory>  // std::unique_ptr
#include <cstring>  // memcpy

#include "cryptonote_basic/cryptonote_format_utils.h"
#include "crypto/crypto.h"
#include "profile_tools.h"

using epee::string_tools::pod_to_hex;
#define DB_DEFAULT_TX (m_write_txn != nullptr ? *m_write_txn : (DbTxn*) nullptr)

// Increase when the DB changes in a non backward compatible way, and there
// is no automatic conversion, so that a full resync is needed.
#define VERSION 0

namespace
{

template <typename T>
inline void throw0(const T &e)
{
    LOG_PRINT_L0(e.what());
    throw e;
}

template <typename T>
inline void throw1(const T &e)
{
    LOG_PRINT_L1(e.what());
    throw e;
}

//  cursor needs to be closed when it goes out of scope,
//  this helps if the function using it throws
struct bdb_cur
{
    bdb_cur(DbTxn* txn, Db* dbi)
    {
        if (dbi->cursor(txn, &m_cur, 0))
            throw0(cryptonote::DB_ERROR("Error opening db cursor"));
        done = false;
    }

    ~bdb_cur()
    {
        close();
    }

    operator Dbc*()
    {
        return m_cur;
    }
    operator Dbc**()
    {
        return &m_cur;
    }
    Dbc* operator->()
    {
        return m_cur;
    }

    void close()
    {
        if (!done)
        {
            m_cur->close();
            done = true;
        }
    }

private:
    Dbc* m_cur;
    bool done;
};

const char* const BDB_BLOCKS = "blocks";
const char* const BDB_BLOCK_TIMESTAMPS = "block_timestamps";
const char* const BDB_BLOCK_HEIGHTS = "block_heights";
const char* const BDB_BLOCK_HASHES = "block_hashes";
const char* const BDB_BLOCK_SIZES = "block_sizes";
const char* const BDB_BLOCK_DIFFS = "block_diffs";
const char* const BDB_BLOCK_COINS = "block_coins";

const char* const BDB_TXS = "txs";
const char* const BDB_TX_UNLOCKS = "tx_unlocks";
const char* const BDB_TX_HEIGHTS = "tx_heights";
const char* const BDB_TX_OUTPUTS = "tx_outputs";

const char* const BDB_OUTPUT_TXS = "output_txs";
const char* const BDB_OUTPUT_INDICES = "output_indices";
const char* const BDB_OUTPUT_AMOUNTS = "output_amounts";
const char* const BDB_OUTPUT_KEYS = "output_keys";

const char* const BDB_SPENT_KEYS = "spent_keys";

const char* const BDB_HF_STARTING_HEIGHTS = "hf_starting_heights";
const char* const BDB_HF_VERSIONS = "hf_versions";

const char* const BDB_PROPERTIES = "properties";

const unsigned int MB = 1024 * 1024;
// ND: FIXME: db keeps running out of locks when doing full syncs. Possible bug??? Set it to 5K for now.
const unsigned int DB_MAX_LOCKS = 5000;
const unsigned int DB_BUFFER_LENGTH = 32 * MB;
// 256MB cache adjust as necessary using DB_CONFIG
const unsigned int DB_DEF_CACHESIZE = 256 * MB;

#if defined(BDB_BULK_CAN_THREAD)
const unsigned int DB_BUFFER_COUNT = tools::get_max_concurrency();
#else
const unsigned int DB_BUFFER_COUNT = 1;
#endif

template<typename T>
struct Dbt_copy: public Dbt
{
    Dbt_copy(const T &t) :
        t_copy(t)
    {
        init();
    }

    Dbt_copy()
    {
        init();
    }

    void init()
    {
        set_data(&t_copy);
        set_size(sizeof(T));
        set_ulen(sizeof(T));
        set_flags(DB_DBT_USERMEM);
    }

    operator T()
    {
        return t_copy;
    }
private:
    T t_copy;
};

template<>
struct Dbt_copy<cryptonote::blobdata>: public Dbt
{
    Dbt_copy(const cryptonote::blobdata &bd) :
        m_data(new char[bd.size()])
    {
        memcpy(m_data.get(), bd.data(), bd.size());
        set_data(m_data.get());
        set_size(bd.size());
        set_ulen(bd.size());
        set_flags(DB_DBT_USERMEM);
    }
private:
    std::unique_ptr<char[]> m_data;
};

template<>
struct Dbt_copy<const char*>: public Dbt
{
    Dbt_copy(const char *s) :
        m_data(strdup(s))
    {
        size_t len = strlen(s) + 1; // include the NUL, makes it easier for compare
        set_data(m_data.get());
        set_size(len);
        set_ulen(len);
        set_flags(DB_DBT_USERMEM);
    }
private:
    std::unique_ptr<char[]> m_data;
};

struct Dbt_safe : public Dbt
{
    Dbt_safe()
    {
        set_data(NULL);
        set_flags(DB_DBT_MALLOC);
    }
    ~Dbt_safe()
    {
        void* buf = get_data();
        if (buf != NULL)
        {
            free(buf);
        }
    }
};

} // anonymous namespace

namespace cryptonote
{

void BlockchainBDB::add_block(const block& blk, const size_t& block_size, const difficulty_type& cumulative_difficulty, const uint64_t& coins_generated, const crypto::hash& blk_hash)
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    Dbt_copy<crypto::hash> val_h(blk_hash);
    if (m_block_heights->exists(DB_DEFAULT_TX, &val_h, 0) == 0)
        throw1(BLOCK_EXISTS("Attempting to add block that's already in the db"));

    if (m_height > 0)
    {
        Dbt_copy<crypto::hash> parent_key(blk.prev_id);
        Dbt_copy<uint32_t> parent_h;
        if (m_block_heights->get(DB_DEFAULT_TX, &parent_key, &parent_h, 0))
        {
            LOG_PRINT_L3("m_height: " << m_height);
            LOG_PRINT_L3("parent_key: " << blk.prev_id);
            throw0(DB_ERROR("Failed to get top block hash to check for new block's parent"));
        }
        uint32_t parent_height = parent_h;
        if (parent_height != m_height)
            throw0(BLOCK_PARENT_DNE("Top block is not new block's parent"));
    }

    Dbt_copy<uint32_t> key(m_height + 1);

    Dbt_copy<blobdata> blob(block_to_blob(blk));
    auto res = m_blocks->put(DB_DEFAULT_TX, &key, &blob, 0);
    if (res)
        throw0(DB_ERROR("Failed to add block blob to db transaction."));

    Dbt_copy<size_t> sz(block_size);
    if (m_block_sizes->put(DB_DEFAULT_TX, &key, &sz, 0))
        throw0(DB_ERROR("Failed to add block size to db transaction."));

    Dbt_copy<uint64_t> ts(blk.timestamp);
    if (m_block_timestamps->put(DB_DEFAULT_TX, &key, &ts, 0))
        throw0(DB_ERROR("Failed to add block timestamp to db transaction."));

    Dbt_copy<difficulty_type> diff(cumulative_difficulty);
    if (m_block_diffs->put(DB_DEFAULT_TX, &key, &diff, 0))
        throw0(DB_ERROR("Failed to add block cumulative difficulty to db transaction."));

    Dbt_copy<uint64_t> coinsgen(coins_generated);
    if (m_block_coins->put(DB_DEFAULT_TX, &key, &coinsgen, 0))
        throw0(DB_ERROR("Failed to add block total generated coins to db transaction."));

    if (m_block_heights->put(DB_DEFAULT_TX, &val_h, &key, 0))
        throw0(DB_ERROR("Failed to add block height by hash to db transaction."));

    if (m_block_hashes->put(DB_DEFAULT_TX, &key, &val_h, 0))
        throw0(DB_ERROR("Failed to add block hash to db transaction."));
}

void BlockchainBDB::remove_block()
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    if (m_height == 0)
        throw0(BLOCK_DNE ("Attempting to remove block from an empty blockchain"));

    Dbt_copy<uint32_t> k(m_height);
    Dbt_copy<crypto::hash> h;
    if (m_block_hashes->get(DB_DEFAULT_TX, &k, &h, 0))
        throw1(BLOCK_DNE("Attempting to remove block that's not in the db"));

    if (m_blocks->del(DB_DEFAULT_TX, &k, 0))
        throw1(DB_ERROR("Failed to add removal of block to db transaction"));

    if (m_block_sizes->del(DB_DEFAULT_TX, &k, 0))
        throw1(DB_ERROR("Failed to add removal of block size to db transaction"));

    if (m_block_diffs->del(DB_DEFAULT_TX, &k, 0))
        throw1(DB_ERROR("Failed to add removal of block cumulative difficulty to db transaction"));

    if (m_block_coins->del(DB_DEFAULT_TX, &k, 0))
        throw1(DB_ERROR("Failed to add removal of block total generated coins to db transaction"));

    if (m_block_timestamps->del(DB_DEFAULT_TX, &k, 0))
        throw1(DB_ERROR("Failed to add removal of block timestamp to db transaction"));

    if (m_block_heights->del(DB_DEFAULT_TX, &h, 0))
        throw1(DB_ERROR("Failed to add removal of block height by hash to db transaction"));

    if (m_block_hashes->del(DB_DEFAULT_TX, &k, 0))
        throw1(DB_ERROR("Failed to add removal of block hash to db transaction"));
}

void BlockchainBDB::add_transaction_data(const crypto::hash& blk_hash, const transaction& tx, const crypto::hash& tx_hash, const crypto::hash& tx_prunable_hash)
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    Dbt_copy<crypto::hash> val_h(tx_hash);

    if (m_txs->exists(DB_DEFAULT_TX, &val_h, 0) == 0)
        throw1(TX_EXISTS("Attempting to add transaction that's already in the db"));

    Dbt_copy<blobdata> blob(tx_to_blob(tx));
    if (m_txs->put(DB_DEFAULT_TX, &val_h, &blob, 0))
        throw0(DB_ERROR("Failed to add tx blob to db transaction"));

    Dbt_copy<uint64_t> height(m_height + 1);
    if (m_tx_heights->put(DB_DEFAULT_TX, &val_h, &height, 0))
        throw0(DB_ERROR("Failed to add tx block height to db transaction"));

    Dbt_copy<uint64_t> unlock_time(tx.unlock_time);
    if (m_tx_unlocks->put(DB_DEFAULT_TX, &val_h, &unlock_time, 0))
        throw0(DB_ERROR("Failed to add tx unlock time to db transaction"));
}

void BlockchainBDB::remove_transaction_data(const crypto::hash& tx_hash, const transaction& tx)
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    Dbt_copy<crypto::hash> val_h(tx_hash);
    if (m_txs->exists(DB_DEFAULT_TX, &val_h, 0))
        throw1(TX_DNE("Attempting to remove transaction that isn't in the db"));

    if (m_txs->del(DB_DEFAULT_TX, &val_h, 0))
        throw1(DB_ERROR("Failed to add removal of tx to db transaction"));
    if (m_tx_unlocks->del(DB_DEFAULT_TX, &val_h, 0))
        throw1(DB_ERROR("Failed to add removal of tx unlock time to db transaction"));
    if (m_tx_heights->del(DB_DEFAULT_TX, &val_h, 0))
        throw1(DB_ERROR("Failed to add removal of tx block height to db transaction"));

    remove_tx_outputs(tx_hash, tx);

    auto result = m_tx_outputs->del(DB_DEFAULT_TX, &val_h, 0);
    if (result == DB_NOTFOUND)
        LOG_PRINT_L1("tx has no outputs to remove: " << tx_hash);
    else if (result)
        throw1(DB_ERROR("Failed to add removal of tx outputs to db transaction"));
}

void BlockchainBDB::add_output(const crypto::hash& tx_hash, const tx_out& tx_output, const uint64_t& local_index, const uint64_t unlock_time, const rct::key *commitment)
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    Dbt_copy<uint32_t> k(m_num_outputs + 1);
    Dbt_copy<crypto::hash> v(tx_hash);

    if (m_output_txs->put(DB_DEFAULT_TX, &k, &v, 0))
        throw0(DB_ERROR("Failed to add output tx hash to db transaction"));
    if (m_tx_outputs->put(DB_DEFAULT_TX, &v, &k, 0))
        throw0(DB_ERROR("Failed to add tx output index to db transaction"));

    Dbt_copy<uint64_t> val_local_index(local_index);
    if (m_output_indices->put(DB_DEFAULT_TX, &k, &val_local_index, 0))
        throw0(DB_ERROR("Failed to add tx output index to db transaction"));

    Dbt_copy<uint64_t> val_amount(tx_output.amount);
    if (m_output_amounts->put(DB_DEFAULT_TX, &val_amount, &k, 0))
        throw0(DB_ERROR("Failed to add output amount to db transaction."));

    if (tx_output.target.type() == typeid(txout_to_key))
    {
        output_data_t od;
        od.pubkey = boost::get < txout_to_key > (tx_output.target).key;
        od.unlock_time = unlock_time;
        od.height = m_height;

        Dbt_copy<output_data_t> data(od);
        if (m_output_keys->put(DB_DEFAULT_TX, &k, &data, 0))
            throw0(DB_ERROR("Failed to add output pubkey to db transaction"));
    }
    else
    {
      throw0(DB_ERROR("Wrong output type: expected txout_to_key"));
    }

    m_num_outputs++;
}

void BlockchainBDB::remove_tx_outputs(const crypto::hash& tx_hash, const transaction& tx)
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);

    bdb_cur cur(DB_DEFAULT_TX, m_tx_outputs);

    Dbt_copy<crypto::hash> k(tx_hash);
    Dbt_copy<uint32_t> v;

    auto result = cur->get(&k, &v, DB_SET);
    if (result == DB_NOTFOUND)
    {
        LOG_PRINT_L2("tx has no outputs, so no global output indices");
    }
    else if (result)
    {
        throw0(DB_ERROR("DB error attempting to get an output"));
    }
    else
    {
        result = cur->get(&k, &v, DB_NEXT_NODUP);
        if (result != 0 && result != DB_NOTFOUND)
            throw0(DB_ERROR("DB error attempting to get next non-duplicate tx hash"));

        if (result == 0)
            result = cur->get(&k, &v, DB_PREV);
        else if (result == DB_NOTFOUND)
            result = cur->get(&k, &v, DB_LAST);

        db_recno_t num_elems = 0;
        cur->count(&num_elems, 0);

        // remove in order: from newest to oldest
        for (uint64_t i = num_elems; i > 0; --i)
        {
            const tx_out tx_output = tx.vout[i-1];
            remove_output(v, tx_output.amount);
            if (i > 1)
            {
                cur->get(&k, &v, DB_PREV_DUP);
            }
        }
    }

    cur.close();
}

// TODO: probably remove this function
void BlockchainBDB::remove_output(const tx_out& tx_output)
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__ << " (unused version - does nothing)");
    return;
}

void BlockchainBDB::remove_output(const uint64_t& out_index, const uint64_t amount)
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    Dbt_copy<uint32_t> k(out_index);

    auto result = m_output_indices->del(DB_DEFAULT_TX, &k, 0);
    if (result == DB_NOTFOUND)
    {
        LOG_PRINT_L0("Unexpected: global output index not found in m_output_indices");
    }
    else if (result)
    {
        throw1(DB_ERROR("Error adding removal of output tx index to db transaction"));
    }

    result = m_output_txs->del(DB_DEFAULT_TX, &k, 0);
    // if (result != 0 && result != DB_NOTFOUND)
    //    throw1(DB_ERROR("Error adding removal of output tx hash to db transaction"));
    if (result == DB_NOTFOUND)
    {
        LOG_PRINT_L0("Unexpected: global output index not found in m_output_txs");
    }
    else if (result)
    {
        throw1(DB_ERROR("Error adding removal of output tx hash to db transaction"));
    }

    result = m_output_keys->del(DB_DEFAULT_TX, &k, 0);
    if (result == DB_NOTFOUND)
    {
        LOG_PRINT_L0("Unexpected: global output index not found in m_output_keys");
    }
    else if (result)
        throw1(DB_ERROR("Error adding removal of output pubkey to db transaction"));

    remove_amount_output_index(amount, out_index);

    m_num_outputs--;
}

void BlockchainBDB::remove_amount_output_index(const uint64_t amount, const uint64_t global_output_index)
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    bdb_cur cur(DB_DEFAULT_TX, m_output_amounts);

    Dbt_copy<uint64_t> k(amount);
    Dbt_copy<uint32_t> v;

    auto result = cur->get(&k, &v, DB_SET);
    if (result == DB_NOTFOUND)
        throw1(OUTPUT_DNE("Attempting to get an output index by amount and amount index, but amount not found"));
    else if (result)
        throw0(DB_ERROR("DB error attempting to get an output"));

    db_recno_t num_elems = 0;
    cur->count(&num_elems, 0);

    // workaround for Berkeley DB to start at end of k's duplicate list:
    // if next key exists:
    //   - move cursor to start of next key's duplicate list, then move back one
    //     duplicate element to reach the end of the original key's duplicate
    //     list.
    //
    // else if the next key doesn't exist:
    //   - that means we're already on the last key.
    //   - move cursor to last element in the db, which is the last element of
    //     the desired key's duplicate list.

    result = cur->get(&k, &v, DB_NEXT_NODUP);
    if (result != 0 && result != DB_NOTFOUND)
      throw0(DB_ERROR("DB error attempting to get next non-duplicate output amount"));

    if (result == 0)
      result = cur->get(&k, &v, DB_PREV);
    else if (result == DB_NOTFOUND)
      result = cur->get(&k, &v, DB_LAST);

    bool found_index = false;
    uint64_t amount_output_index = 0;
    uint64_t goi = 0;

    for (uint64_t i = num_elems; i > 0; --i)
    {
      goi = v;
      if (goi == global_output_index)
      {
        amount_output_index = i-1;
        found_index = true;
        break;
      }
      if (i > 1)
        cur->get(&k, &v, DB_PREV_DUP);
    }

    if (found_index)
    {
        // found the amount output index
        // now delete it
        result = cur->del(0);
        if (result)
            throw0(DB_ERROR(std::string("Error deleting amount output index ").append(boost::lexical_cast<std::string>(amount_output_index)).c_str()));
    }
    else
    {
        // not found
        throw1(OUTPUT_DNE("Failed to find amount output index"));
    }
    cur.close();
}

void BlockchainBDB::add_spent_key(const crypto::key_image& k_image)
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    Dbt_copy<crypto::key_image> val_key(k_image);
    if (m_spent_keys->exists(DB_DEFAULT_TX, &val_key, 0) == 0)
        throw1(KEY_IMAGE_EXISTS("Attempting to add spent key image that's already in the db"));

    Dbt_copy<char> val('\0');
    if (m_spent_keys->put(DB_DEFAULT_TX, &val_key, &val, 0))
        throw1(DB_ERROR("Error adding spent key image to db transaction."));
}

void BlockchainBDB::remove_spent_key(const crypto::key_image& k_image)
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    Dbt_copy<crypto::key_image> k(k_image);
    auto result = m_spent_keys->del(DB_DEFAULT_TX, &k, 0);
    if (result != 0 && result != DB_NOTFOUND)
        throw1(DB_ERROR("Error adding removal of key image to db transaction"));
}

bool BlockchainBDB::for_all_key_images(std::function<bool(const crypto::key_image&)> f) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    bdb_cur cur(DB_DEFAULT_TX, m_spent_keys);

    Dbt_copy<crypto::key_image> k;
    Dbt_copy<char> v;
    bool ret = true;
    int result;
    while ((result = cur->get(&k, &v, DB_NEXT)) == 0)
    {
      if (!f(k))
      {
        ret = false;
        break;
      }
    }
    if (result != DB_NOTFOUND)
      ret = false;

    cur.close();
    return ret;
}

bool BlockchainBDB::for_all_blocks(std::function<bool(uint64_t, const crypto::hash&, const cryptonote::block&)> f) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    bdb_cur cur(DB_DEFAULT_TX, m_blocks);

    Dbt_copy<uint32_t> k;
    Dbt_safe v;
    bool ret = true;
    int result;
    while ((result = cur->get(&k, &v, DB_NEXT)) == 0)
    {
      uint64_t height = k - 1;
      blobdata bd;
      bd.assign(reinterpret_cast<char*>(v.get_data()), v.get_size());
      block b;
      if (!parse_and_validate_block_from_blob(bd, b))
          throw0(DB_ERROR("Failed to parse block from blob retrieved from the db"));
      crypto::hash hash;
      if (!get_block_hash(b, hash))
          throw0(DB_ERROR("Failed to get block hash from blob retrieved from the db"));
      if (!f(height, hash, b))
      {
        ret = false;
        break;
      }
    }
    if (result != DB_NOTFOUND)
      ret = false;

    cur.close();
    return ret;
}

bool BlockchainBDB::for_all_transactions(std::function<bool(const crypto::hash&, const cryptonote::transaction&)> f, bool pruned) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    bdb_cur cur(DB_DEFAULT_TX, m_txs);

    Dbt_copy<crypto::hash> k;
    Dbt_safe v;
    bool ret = true;
    int result;
    while ((result = cur->get(&k, &v, DB_NEXT)) == 0)
    {
      blobdata bd;
      bd.assign(reinterpret_cast<char*>(v.get_data()), v.get_size());
      transaction tx;
      if (!parse_and_validate_tx_from_blob(bd, tx))
          throw0(DB_ERROR("Failed to parse tx from blob retrieved from the db"));
      if (!f(k, tx))
      {
        ret = false;
        break;
      }
    }
    if (result != DB_NOTFOUND)
      ret = false;

    cur.close();
    return ret;
}

bool BlockchainBDB::for_all_outputs(std::function<bool(uint64_t amount, const crypto::hash &tx_hash, size_t tx_idx)> f) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    bdb_cur cur(DB_DEFAULT_TX, m_output_amounts);

    Dbt_copy<uint64_t> k;
    Dbt_copy<uint32_t> v;
    bool ret = true;
    int result;
    while ((result = cur->get(&k, &v, DB_NEXT)) == 0)
    {
      uint32_t global_index = v - 1;
      tx_out_index toi = get_output_tx_and_index_from_global(global_index);
      if (!f(k, toi.first, toi.second))
      {
        ret = false;
        break;
      }
    }
    if (result != DB_NOTFOUND)
      ret = false;

    cur.close();
    return ret;
}

blobdata BlockchainBDB::output_to_blob(const tx_out& output) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    blobdata b;
    if (!t_serializable_object_to_blob(output, b))
        throw1(DB_ERROR("Error serializing output to blob"));
    return b;
}

tx_out BlockchainBDB::output_from_blob(const blobdata& blob) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    std::stringstream ss;
    ss << blob;
    binary_archive<false> ba(ss);
    tx_out o;

    if (!(::serialization::serialize(ba, o)))
        throw1(DB_ERROR("Error deserializing tx output blob"));

    return o;
}

uint64_t BlockchainBDB::get_output_global_index(const uint64_t& amount, const uint64_t& index)
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    std::vector < uint64_t > offsets;
    std::vector < uint64_t > global_indices;
    offsets.push_back(index);
    get_output_global_indices(amount, offsets, global_indices);
    if (!global_indices.size())
        throw1(OUTPUT_DNE("Attempting to get an output index by amount and amount index, but amount not found"));

    return global_indices[0];
}

void BlockchainBDB::check_open() const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    if (!m_open)
        throw0(DB_ERROR("DB operation attempted on a not-open DB instance"));
}

BlockchainBDB::~BlockchainBDB()
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);

    if (m_open)
    {
        close();
    }
}

BlockchainBDB::BlockchainBDB(bool batch_transactions) :
        BlockchainDB(),
        m_buffer(DB_BUFFER_COUNT, DB_BUFFER_LENGTH)
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    // initialize folder to something "safe" just in case
    // someone accidentally misuses this class...
    m_folder = "thishsouldnotexistbecauseitisgibberish";
    m_run_checkpoint = 0;
    m_batch_transactions = batch_transactions;
    m_write_txn = nullptr;
    m_height = 0;

    m_hardfork = nullptr;
}

void BlockchainBDB::open(const std::string& filename, const int db_flags)
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);

    if (m_open)
        throw0(DB_OPEN_FAILURE("Attempted to open db, but it's already open"));

    boost::filesystem::path direc(filename);
    if (boost::filesystem::exists(direc))
    {
        if (!boost::filesystem::is_directory(direc))
            throw0(DB_OPEN_FAILURE("DB needs a directory path, but a file was passed"));
    }
    else
    {
        if (!boost::filesystem::create_directories(direc))
            throw0(DB_OPEN_FAILURE(std::string("Failed to create directory ").append(filename).c_str()));
    }

    m_folder = filename;

    try
    {

        //Create BerkeleyDB environment
        m_env = new DbEnv(0);  // no flags needed for DbEnv

        uint32_t db_env_open_flags = DB_CREATE | DB_INIT_MPOOL | DB_INIT_LOCK | DB_INIT_LOG | DB_INIT_TXN | DB_RECOVER | DB_THREAD;

        // Set some default values for these parameters.
        // They can be overridden using the DB_CONFIG file.
        m_env->set_cachesize(0, DB_DEF_CACHESIZE, 1);
        m_env->set_lk_max_locks(DB_MAX_LOCKS);
        m_env->set_lk_max_lockers(DB_MAX_LOCKS);
        m_env->set_lk_max_objects(DB_MAX_LOCKS);

        #ifndef __OpenBSD__ //OpenBSD's DB package is too old to support this feature
        if(m_auto_remove_logs)
          m_env->log_set_config(DB_LOG_AUTO_REMOVE, 1);
        #endif

        // last parameter left 0, files will be created with default rw access
        m_env->open(filename.c_str(), db_env_open_flags, 0);
        m_env->set_flags(db_flags, 1);

        // begin transaction to init dbs
        bdb_txn_safe txn;
        m_env->txn_begin(NULL, txn, 0);

        // create Dbs in the environment
        m_blocks = new Db(m_env, 0);
        m_block_heights = new Db(m_env, 0);
        m_block_hashes = new Db(m_env, 0);
        m_block_timestamps = new Db(m_env, 0);
        m_block_sizes = new Db(m_env, 0);
        m_block_diffs = new Db(m_env, 0);
        m_block_coins = new Db(m_env, 0);

        m_txs = new Db(m_env, 0);
        m_tx_unlocks = new Db(m_env, 0);
        m_tx_heights = new Db(m_env, 0);
        m_tx_outputs = new Db(m_env, 0);

        m_output_txs = new Db(m_env, 0);
        m_output_indices = new Db(m_env, 0);
        m_output_amounts = new Db(m_env, 0);
        m_output_keys = new Db(m_env, 0);

        m_spent_keys = new Db(m_env, 0);

        m_hf_starting_heights = new Db(m_env, 0);
        m_hf_versions = new Db(m_env, 0);

        m_properties = new Db(m_env, 0);

        // Tell DB about Dbs that need duplicate support
        // Note: no need to tell about sorting,
        //   as the default is insertion order, which we want
        m_tx_outputs->set_flags(DB_DUP);
        m_output_amounts->set_flags(DB_DUP);

        // Tell DB about fixed-size values.
        m_block_hashes->set_re_len(sizeof(crypto::hash));
        m_block_timestamps->set_re_len(sizeof(uint64_t));
        m_block_sizes->set_re_len(sizeof(size_t));  // should really store block size as uint64_t...
        m_block_diffs->set_re_len(sizeof(difficulty_type));
        m_block_coins->set_re_len(sizeof(uint64_t));

        m_output_txs->set_re_len(sizeof(crypto::hash));
        m_output_indices->set_re_len(sizeof(uint64_t));
        m_output_keys->set_re_len(sizeof(output_data_t));

        m_hf_starting_heights->set_re_len(sizeof(uint64_t));
        m_hf_versions->set_re_len(sizeof(uint8_t));

        //TODO: Find out if we need to do Db::set_flags(DB_RENUMBER)
        //      for the RECNO databases.  We shouldn't as we're only
        //      inserting/removing from the end, but we'll see.

        // open Dbs in the environment
        // m_tx_outputs and m_output_amounts must be DB_HASH or DB_BTREE
        //   because they need duplicate entry support.  The rest are DB_RECNO,
        //   as it seems that will be the most performant choice.
        m_blocks->open(txn, BDB_BLOCKS, NULL, DB_RECNO, DB_CREATE, 0);

        m_block_timestamps->open(txn, BDB_BLOCK_TIMESTAMPS, NULL, DB_RECNO, DB_CREATE, 0);
        m_block_heights->open(txn, BDB_BLOCK_HEIGHTS, NULL, DB_HASH, DB_CREATE, 0);
        m_block_hashes->open(txn, BDB_BLOCK_HASHES, NULL, DB_RECNO, DB_CREATE, 0);
        m_block_sizes->open(txn, BDB_BLOCK_SIZES, NULL, DB_RECNO, DB_CREATE, 0);
        m_block_diffs->open(txn, BDB_BLOCK_DIFFS, NULL, DB_RECNO, DB_CREATE, 0);
        m_block_coins->open(txn, BDB_BLOCK_COINS, NULL, DB_RECNO, DB_CREATE, 0);

        m_txs->open(txn, BDB_TXS, NULL, DB_HASH, DB_CREATE, 0);
        m_tx_unlocks->open(txn, BDB_TX_UNLOCKS, NULL, DB_HASH, DB_CREATE, 0);
        m_tx_heights->open(txn, BDB_TX_HEIGHTS, NULL, DB_HASH, DB_CREATE, 0);
        m_tx_outputs->open(txn, BDB_TX_OUTPUTS, NULL, DB_HASH, DB_CREATE, 0);

        m_output_txs->open(txn, BDB_OUTPUT_TXS, NULL, DB_RECNO, DB_CREATE, 0);
        m_output_indices->open(txn, BDB_OUTPUT_INDICES, NULL, DB_RECNO, DB_CREATE, 0);
        m_output_amounts->open(txn, BDB_OUTPUT_AMOUNTS, NULL, DB_HASH, DB_CREATE, 0);
        m_output_keys->open(txn, BDB_OUTPUT_KEYS, NULL, DB_RECNO, DB_CREATE, 0);

        m_spent_keys->open(txn, BDB_SPENT_KEYS, NULL, DB_HASH, DB_CREATE, 0);

        m_hf_starting_heights->open(txn, BDB_HF_STARTING_HEIGHTS, NULL, DB_RECNO, DB_CREATE, 0);
        m_hf_versions->open(txn, BDB_HF_VERSIONS, NULL, DB_RECNO, DB_CREATE, 0);

        m_properties->open(txn, BDB_PROPERTIES, NULL, DB_HASH, DB_CREATE, 0);

        txn.commit();

        DB_BTREE_STAT* stats;

        // DB_FAST_STAT can apparently cause an incorrect number of records
        // to be returned.  The flag should be set to 0 instead if this proves
        // to be the case.

        // ND: The bug above can occur when a block is popped and the application
        // exits without pushing a new block to the db. Set txn to NULL and DB_FAST_STAT
        // to zero (0) for reliability.
        m_blocks->stat(NULL, &stats, 0);
        m_height = stats->bt_nkeys;
        free(stats);

        // see above comment about DB_FAST_STAT
        m_output_indices->stat(NULL, &stats, 0);
        m_num_outputs = stats->bt_nkeys;
        free(stats);

        // checks for compatibility
        bool compatible = true;

        Dbt_copy<const char*> key("version");
        Dbt_copy<uint32_t> result;
        auto get_result = m_properties->get(DB_DEFAULT_TX, &key, &result, 0);
        if (get_result == 0)
        {
            if (result > VERSION)
            {
                LOG_PRINT_RED_L0("Existing BerkeleyDB database was made by a later version. We don't know how it will change yet.");
                compatible = false;
            }
#if VERSION > 0
            else if (result < VERSION)
            {
                compatible = false;
            }
#endif
        }
        else
        {
            // if not found, but we're on version 0, it's fine. If the DB's empty, it's fine too.
            if (VERSION > 0 && m_height > 0)
                compatible = false;
        }

        if (!compatible)
        {
            m_open = false;
            LOG_PRINT_RED_L0("Existing BerkeleyDB database is incompatible with this version.");
            LOG_PRINT_RED_L0("Please delete the existing database and resync.");
            return;
        }

        if (1 /* this can't be set readonly atm */)
        {
            // only write version on an empty DB
            if (m_height == 0)
            {
                Dbt_copy<const char*> k("version");
                Dbt_copy<uint32_t> v(VERSION);
                auto put_result = m_properties->put(DB_DEFAULT_TX, &k, &v, 0);
                if (put_result != 0)
                {
                    m_open = false;
                    LOG_PRINT_RED_L0("Failed to write version to database.");
                    return;
                }
            }
        }

        // run checkpoint thread
        m_run_checkpoint = true;
        m_checkpoint_thread.reset(new boost::thread(&BlockchainBDB::checkpoint_worker, this));
    }
    catch (const std::exception& e)
    {
        throw0(DB_OPEN_FAILURE(e.what()));
    }

    m_open = true;
}

void BlockchainBDB::close()
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    this->sync();

    m_run_checkpoint = false;
    m_checkpoint_thread->join();
    m_checkpoint_thread.reset();

    // FIXME: not yet thread safe!!!  Use with care.
    m_open = false;

    // DB_FORCESYNC is only available on newer version of libdb.
    // The libdb doc says using the DB_FORCESYNC flag to DB_ENV->close
    // is "similar to calling the DB->close(0) method to close each
    // database handle". So this is what we do here as a fallback.
#ifdef DB_FORCESYNC
    m_env->close(DB_FORCESYNC);
#else
    m_blocks->close(0);
    m_block_heights->close(0);
    m_block_hashes->close(0);
    m_block_timestamps->close(0);
    m_block_sizes->close(0);
    m_block_diffs->close(0);
    m_block_coins->close(0);

    m_txs->close(0);
    m_tx_unlocks->close(0);
    m_tx_heights->close(0);
    m_tx_outputs->close(0);

    m_output_txs->close(0);
    m_output_indices->close(0);
    m_output_amounts->close(0);
    m_output_keys->close(0);

    m_spent_keys->close(0);

    m_hf_starting_heights->close(0);
    m_hf_versions->close(0);

    m_properties->close(0);

    m_env->close(0);
#endif
}

void BlockchainBDB::sync()
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    try
    {
        m_blocks->sync(0);
        m_block_heights->sync(0);
        m_block_hashes->sync(0);
        m_block_timestamps->sync(0);
        m_block_sizes->sync(0);
        m_block_diffs->sync(0);
        m_block_coins->sync(0);

        m_txs->sync(0);
        m_tx_unlocks->sync(0);
        m_tx_heights->sync(0);
        m_tx_outputs->sync(0);

        m_output_txs->sync(0);
        m_output_indices->sync(0);
        m_output_amounts->sync(0);
        m_output_keys->sync(0);

        m_spent_keys->sync(0);

        if (m_hf_starting_heights != nullptr)
          m_hf_starting_heights->sync(0);
        if (m_hf_versions != nullptr)
          m_hf_versions->sync(0);

        m_properties->sync(0);
    }
    catch (const std::exception& e)
    {
        throw0(DB_ERROR(std::string("Failed to sync database: ").append(e.what()).c_str()));
    }
}

void BlockchainBDB::reset()
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    bdb_txn_safe txn;
    if (m_env->txn_begin(NULL, txn, 0))
        throw0(DB_ERROR("Failed to create a transaction for the db"));
    m_write_txn = &txn;
    try
    {
        uint32_t count;

        m_blocks->truncate(*m_write_txn, &count, 0);
        m_block_heights->truncate(*m_write_txn, &count, 0);
        m_block_hashes->truncate(*m_write_txn, &count, 0);
        m_block_timestamps->truncate(*m_write_txn, &count, 0);
        m_block_sizes->truncate(*m_write_txn, &count, 0);
        m_block_diffs->truncate(*m_write_txn, &count, 0);
        m_block_coins->truncate(*m_write_txn, &count, 0);

        m_txs->truncate(*m_write_txn, &count, 0);
        m_tx_unlocks->truncate(*m_write_txn, &count, 0);
        m_tx_heights->truncate(*m_write_txn, &count, 0);
        m_tx_outputs->truncate(*m_write_txn, &count, 0);

        m_output_txs->truncate(*m_write_txn, &count, 0);
        m_output_indices->truncate(*m_write_txn, &count, 0);
        m_output_amounts->truncate(*m_write_txn, &count, 0);
        m_output_keys->truncate(*m_write_txn, &count, 0);

        m_spent_keys->truncate(*m_write_txn, &count, 0);

        m_hf_starting_heights->truncate(*m_write_txn, &count, 0);
        m_hf_versions->truncate(*m_write_txn, &count, 0);

        m_properties->truncate(*m_write_txn, &count, 0);
    }
    catch (const std::exception& e)
    {
        throw0(DB_ERROR(std::string("Failed to reset database: ").append(e.what()).c_str()));
    }
    m_write_txn = NULL;
}

std::vector<std::string> BlockchainBDB::get_filenames() const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    std::vector<std::string> filenames;

    char *fname, *dbname;
    const char **pfname, **pdbname;

    pfname = (const char **)&fname;
    pdbname = (const char **)&dbname;

    m_blocks->get_dbname(pfname, pdbname);
    filenames.push_back(fname);

    m_block_heights->get_dbname(pfname, pdbname);
    filenames.push_back(fname);

    m_block_hashes->get_dbname(pfname, pdbname);
    filenames.push_back(fname);

    m_block_timestamps->get_dbname(pfname, pdbname);
    filenames.push_back(fname);

    m_block_sizes->get_dbname(pfname, pdbname);
    filenames.push_back(fname);

    m_block_diffs->get_dbname(pfname, pdbname);
    filenames.push_back(fname);

    m_block_coins->get_dbname(pfname, pdbname);
    filenames.push_back(fname);

    m_txs->get_dbname(pfname, pdbname);
    filenames.push_back(fname);

    m_tx_unlocks->get_dbname(pfname, pdbname);
    filenames.push_back(fname);

    m_tx_heights->get_dbname(pfname, pdbname);
    filenames.push_back(fname);

    m_tx_outputs->get_dbname(pfname, pdbname);
    filenames.push_back(fname);

    m_output_txs->get_dbname(pfname, pdbname);
    filenames.push_back(fname);

    m_output_indices->get_dbname(pfname, pdbname);
    filenames.push_back(fname);

    m_output_amounts->get_dbname(pfname, pdbname);
    filenames.push_back(fname);

    m_output_keys->get_dbname(pfname, pdbname);
    filenames.push_back(fname);

    m_spent_keys->get_dbname(pfname, pdbname);
    filenames.push_back(fname);

    m_hf_starting_heights->get_dbname(pfname, pdbname);
    filenames.push_back(fname);

    m_hf_versions->get_dbname(pfname, pdbname);
    filenames.push_back(fname);

    m_properties->get_dbname(pfname, pdbname);
    filenames.push_back(fname);

    std::vector<std::string> full_paths;

    for (auto& filename : filenames)
    {
        boost::filesystem::path p(m_folder);
        p /= filename;
        full_paths.push_back(p.string());
    }

    return full_paths;
}

std::string BlockchainBDB::get_db_name() const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);

    return std::string("BerkeleyDB");
}

// TODO: this?
bool BlockchainBDB::lock()
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();
    return false;
}

// TODO: this?
void BlockchainBDB::unlock()
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();
}

bool BlockchainBDB::block_exists(const crypto::hash& h, uint64_t *height) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    Dbt_copy<crypto::hash> key(h);

    auto get_result = m_block_heights->exists(DB_DEFAULT_TX, &key, 0);
    if (get_result == DB_NOTFOUND)
    {
        LOG_PRINT_L3("Block with hash " << epee::string_tools::pod_to_hex(h) << " not found in db");
        return false;
    }
    else if (get_result)
        throw0(DB_ERROR("DB error attempting to fetch block index from hash"));

    if (height)
      *height = get_result - 1;

    return true;
}

block BlockchainBDB::get_block(const crypto::hash& h) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    return get_block_from_height(get_block_height(h));
}

uint64_t BlockchainBDB::get_block_height(const crypto::hash& h) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    Dbt_copy<crypto::hash> key(h);
    Dbt_copy<uint32_t> result;

    auto get_result = m_block_heights->get(DB_DEFAULT_TX, &key, &result, 0);
    if (get_result == DB_NOTFOUND)
        throw1(BLOCK_DNE("Attempted to retrieve non-existent block height"));
    else if (get_result)
        throw0(DB_ERROR("Error attempting to retrieve a block height from the db"));

    return result - 1;
}

block_header BlockchainBDB::get_block_header(const crypto::hash& h) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    // block_header object is automatically cast from block object
    return get_block(h);
}

block BlockchainBDB::get_block_from_height(const uint64_t& height) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    Dbt_copy<uint32_t> key(height + 1);
    Dbt_safe result;
    auto get_result = m_blocks->get(DB_DEFAULT_TX, &key, &result, 0);
    if (get_result == DB_NOTFOUND)
    {
        throw0(BLOCK_DNE(std::string("Attempt to get block from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- block not in db").c_str()));
    }
    else if (get_result)
        throw0(DB_ERROR("Error attempting to retrieve a block from the db"));

    blobdata bd;
    bd.assign(reinterpret_cast<char*>(result.get_data()), result.get_size());

    block b;
    if (!parse_and_validate_block_from_blob(bd, b))
        throw0(DB_ERROR("Failed to parse block from blob retrieved from the db"));

    return b;
}

uint64_t BlockchainBDB::get_block_timestamp(const uint64_t& height) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    Dbt_copy<uint32_t> key(height + 1);
    Dbt_copy<uint64_t> result;
    auto get_result = m_block_timestamps->get(DB_DEFAULT_TX, &key, &result, 0);
    if (get_result == DB_NOTFOUND)
    {
        throw0(BLOCK_DNE(std::string("Attempt to get timestamp from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- timestamp not in db").c_str()));
    }
    else if (get_result)
        throw0(DB_ERROR("Error attempting to retrieve a timestamp from the db"));

    return result;
}

uint64_t BlockchainBDB::get_top_block_timestamp() const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    // if no blocks, return 0
    if (m_height == 0)
    {
        return 0;
    }

    return get_block_timestamp(m_height - 1);
}

size_t BlockchainBDB::get_block_size(const uint64_t& height) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    Dbt_copy<uint32_t> key(height + 1);
    Dbt_copy<size_t> result;
    auto get_result = m_block_sizes->get(DB_DEFAULT_TX, &key, &result, 0);
    if (get_result == DB_NOTFOUND)
    {
        throw0(BLOCK_DNE(std::string("Attempt to get block size from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- block size not in db").c_str()));
    }
    else if (get_result)
        throw0(DB_ERROR("Error attempting to retrieve a block size from the db"));

    return result;
}

difficulty_type BlockchainBDB::get_block_cumulative_difficulty(const uint64_t& height) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__ << "  height: " << height);
    check_open();

    Dbt_copy<uint32_t> key(height + 1);
    Dbt_copy<difficulty_type> result;
    auto get_result = m_block_diffs->get(DB_DEFAULT_TX, &key, &result, 0);
    if (get_result == DB_NOTFOUND)
    {
        throw0(BLOCK_DNE(std::string("Attempt to get cumulative difficulty from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- difficulty not in db").c_str()));
    }
    else if (get_result)
        throw0(DB_ERROR("Error attempting to retrieve a cumulative difficulty from the db"));

    return result;
}

difficulty_type BlockchainBDB::get_block_difficulty(const uint64_t& height) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    difficulty_type diff1 = 0;
    difficulty_type diff2 = 0;

    diff1 = get_block_cumulative_difficulty(height);
    if (height != 0)
    {
        diff2 = get_block_cumulative_difficulty(height - 1);
    }

    return diff1 - diff2;
}

uint64_t BlockchainBDB::get_block_already_generated_coins(const uint64_t& height) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    Dbt_copy<uint32_t> key(height + 1);
    Dbt_copy<uint64_t> result;
    auto get_result = m_block_coins->get(DB_DEFAULT_TX, &key, &result, 0);
    if (get_result == DB_NOTFOUND)
    {
        throw0(BLOCK_DNE(std::string("Attempt to get generated coins from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- block size not in db").c_str()));
    }
    else if (get_result)
        throw0(DB_ERROR("Error attempting to retrieve a total generated coins from the db"));

    return result;
}

crypto::hash BlockchainBDB::get_block_hash_from_height(const uint64_t& height) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    Dbt_copy<uint32_t> key(height + 1);
    Dbt_copy<crypto::hash> result;
    auto get_result = m_block_hashes->get(DB_DEFAULT_TX, &key, &result, 0);
    if (get_result == DB_NOTFOUND)
    {
        throw0(BLOCK_DNE(std::string("Attempt to get hash from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- hash not in db").c_str()));
    }
    else if (get_result)
        throw0(DB_ERROR("Error attempting to retrieve a block hash from the db."));

    return result;
}

std::vector<block> BlockchainBDB::get_blocks_range(const uint64_t& h1, const uint64_t& h2) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();
    std::vector<block> v;

    for (uint64_t height = h1; height <= h2; ++height)
    {
        v.push_back(get_block_from_height(height));
    }

    return v;
}

std::vector<crypto::hash> BlockchainBDB::get_hashes_range(const uint64_t& h1, const uint64_t& h2) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();
    std::vector<crypto::hash> v;

    for (uint64_t height = h1; height <= h2; ++height)
    {
        v.push_back(get_block_hash_from_height(height));
    }

    return v;
}

crypto::hash BlockchainBDB::top_block_hash() const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();
    if (m_height > 0)
    {
        return get_block_hash_from_height(m_height - 1);
    }

    return null_hash;
}

block BlockchainBDB::get_top_block() const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    if (m_height > 0)
    {
        return get_block_from_height(m_height - 1);
    }

    block b;
    return b;
}

uint64_t BlockchainBDB::height() const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    return m_height;
}

bool BlockchainBDB::tx_exists(const crypto::hash& h) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    Dbt_copy<crypto::hash> key(h);

    TIME_MEASURE_START(time1);
    auto get_result = m_txs->exists(DB_DEFAULT_TX, &key, 0);
    TIME_MEASURE_FINISH(time1);
    time_tx_exists += time1;
    if (get_result == DB_NOTFOUND)
    {
        LOG_PRINT_L1("transaction with hash " << epee::string_tools::pod_to_hex(h) << " not found in db");
        return false;
    }
    else if (get_result)
        throw0(DB_ERROR("DB error attempting to fetch transaction from hash"));

    return true;
}

uint64_t BlockchainBDB::get_tx_unlock_time(const crypto::hash& h) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    Dbt_copy<crypto::hash> key(h);
    Dbt_copy<uint64_t> result;
    auto get_result = m_tx_unlocks->get(DB_DEFAULT_TX, &key, &result, 0);
    if (get_result == DB_NOTFOUND)
        throw1(TX_DNE(std::string("tx unlock time with hash ").append(epee::string_tools::pod_to_hex(h)).append(" not found in db").c_str()));
    else if (get_result)
        throw0(DB_ERROR("DB error attempting to fetch tx unlock time from hash"));

    return result;
}

transaction BlockchainBDB::get_tx(const crypto::hash& h) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    Dbt_copy<crypto::hash> key(h);
    Dbt_safe result;
    auto get_result = m_txs->get(DB_DEFAULT_TX, &key, &result, 0);
    if (get_result == DB_NOTFOUND)
        throw1(TX_DNE(std::string("tx with hash ").append(epee::string_tools::pod_to_hex(h)).append(" not found in db").c_str()));
    else if (get_result)
        throw0(DB_ERROR("DB error attempting to fetch tx from hash"));

    blobdata bd;
    bd.assign(reinterpret_cast<char*>(result.get_data()), result.get_size());

    transaction tx;
    if (!parse_and_validate_tx_from_blob(bd, tx))
        throw0(DB_ERROR("Failed to parse tx from blob retrieved from the db"));

    return tx;
}

uint64_t BlockchainBDB::get_tx_count() const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    DB_BTREE_STAT* stats;

    // DB_FAST_STAT can apparently cause an incorrect number of records
    // to be returned.  The flag should be set to 0 instead if this proves
    // to be the case.
    m_txs->stat(DB_DEFAULT_TX, &stats, DB_FAST_STAT);
    auto num_txs = stats->bt_nkeys;
    delete stats;

    return num_txs;
}

std::vector<transaction> BlockchainBDB::get_tx_list(const std::vector<crypto::hash>& hlist) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();
    std::vector<transaction> v;

for (auto& h : hlist)
    {
        v.push_back(get_tx(h));
    }

    return v;
}

uint64_t BlockchainBDB::get_tx_block_height(const crypto::hash& h) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    Dbt_copy<crypto::hash> key(h);
    Dbt_copy<uint64_t> result;
    auto get_result = m_tx_heights->get(DB_DEFAULT_TX, &key, &result, 0);
    if (get_result == DB_NOTFOUND)
    {
        throw1(TX_DNE(std::string("tx height with hash ").append(epee::string_tools::pod_to_hex(h)).append(" not found in db").c_str()));
    }
    else if (get_result)
        throw0(DB_ERROR("DB error attempting to fetch tx height from hash"));

    return (uint64_t)result - 1;
}

uint64_t BlockchainBDB::get_num_outputs(const uint64_t& amount) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    bdb_cur cur(DB_DEFAULT_TX, m_output_amounts);

    Dbt_copy<uint64_t> k(amount);
    Dbt_copy<uint32_t> v;
    auto result = cur->get(&k, &v, DB_SET);
    if (result == DB_NOTFOUND)
    {
        return 0;
    }
    else if (result)
        throw0(DB_ERROR("DB error attempting to get number of outputs of an amount"));

    db_recno_t num_elems = 0;
    cur->count(&num_elems, 0);

    cur.close();

    return num_elems;
}

output_data_t BlockchainBDB::get_output_key(const uint64_t& global_index) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    Dbt_copy<uint32_t> k(global_index);
    Dbt_copy<output_data_t> v;
    auto get_result = m_output_keys->get(DB_DEFAULT_TX, &k, &v, 0);
    if (get_result == DB_NOTFOUND)
        throw1(OUTPUT_DNE("Attempting to get output pubkey by global index, but key does not exist"));
    else if (get_result)
        throw0(DB_ERROR("Error attempting to retrieve an output pubkey from the db"));

    return v;
}

output_data_t BlockchainBDB::get_output_key(const uint64_t& amount, const uint64_t& index)
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    uint64_t glob_index = get_output_global_index(amount, index);
    return get_output_key(glob_index);
}

tx_out_index BlockchainBDB::get_output_tx_and_index(const uint64_t& amount, const uint64_t& index)
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    std::vector < uint64_t > offsets;
    std::vector<tx_out_index> indices;
    offsets.push_back(index);
    get_output_tx_and_index(amount, offsets, indices);
    if (!indices.size())
        throw1(OUTPUT_DNE("Attempting to get an output index by amount and amount index, but amount not found"));

    return indices[0];
}

std::vector<uint64_t> BlockchainBDB::get_tx_output_indices(const crypto::hash& h) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();
    std::vector<uint64_t> index_vec;

    bdb_cur cur(DB_DEFAULT_TX, m_tx_outputs);

    Dbt_copy<crypto::hash> k(h);
    Dbt_copy<uint32_t> v;
    auto result = cur->get(&k, &v, DB_SET);
    if (result == DB_NOTFOUND)
        throw1(OUTPUT_DNE("Attempting to get an output by tx hash and tx index, but output not found"));
    else if (result)
        throw0(DB_ERROR("DB error attempting to get an output"));

    db_recno_t num_elems = 0;
    cur->count(&num_elems, 0);

    for (uint64_t i = 0; i < num_elems; ++i)
    {
        index_vec.push_back(v);
        cur->get(&k, &v, DB_NEXT_DUP);
    }

    cur.close();

    return index_vec;
}

std::vector<uint64_t> BlockchainBDB::get_tx_amount_output_indices(const crypto::hash& h) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();
    std::vector<uint64_t> index_vec;
    std::vector<uint64_t> index_vec2;

    // get the transaction's global output indices first
    index_vec = get_tx_output_indices(h);
    // these are next used to obtain the amount output indices

    transaction tx = get_tx(h);

    uint64_t i = 0;
    uint64_t global_index;
    for (const auto& vout : tx.vout)
    {
        uint64_t amount =  vout.amount;

        global_index = index_vec[i];

        bdb_cur cur(DB_DEFAULT_TX, m_output_amounts);

        Dbt_copy<uint64_t> k(amount);
        Dbt_copy<uint32_t> v;

        auto result = cur->get(&k, &v, DB_SET);
        if (result == DB_NOTFOUND)
            throw1(OUTPUT_DNE("Attempting to get an output index by amount and amount index, but amount not found"));
        else if (result)
            throw0(DB_ERROR("DB error attempting to get an output"));

        db_recno_t num_elems = 0;
        cur->count(&num_elems, 0);

        uint64_t amount_output_index = 0;
        uint64_t output_index = 0;
        bool found_index = false;
        for (uint64_t j = 0; j < num_elems; ++j)
        {
            output_index = v;
            if (output_index == global_index)
            {
                amount_output_index = j;
                found_index = true;
                break;
            }
            cur->get(&k, &v, DB_NEXT_DUP);
        }
        if (found_index)
        {
            index_vec2.push_back(amount_output_index);
        }
        else
        {
            // not found
            cur.close();
            throw1(OUTPUT_DNE("specified output not found in db"));
        }

        cur.close();
        ++i;
    }

    return index_vec2;
}


tx_out_index BlockchainBDB::get_output_tx_and_index_from_global(const uint64_t& index) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    Dbt_copy<uint32_t> k(index);
    Dbt_copy<crypto::hash > v;

    auto get_result = m_output_txs->get(DB_DEFAULT_TX, &k, &v, 0);
    if (get_result == DB_NOTFOUND)
        throw1(OUTPUT_DNE("output with given index not in db"));
    else if (get_result)
        throw0(DB_ERROR("DB error attempting to fetch output tx hash"));

    crypto::hash tx_hash = v;

    Dbt_copy<uint64_t> result;
    get_result = m_output_indices->get(DB_DEFAULT_TX, &k, &result, 0);
    if (get_result == DB_NOTFOUND)
        throw1(OUTPUT_DNE("output with given index not in db"));
    else if (get_result)
        throw0(DB_ERROR("DB error attempting to fetch output tx index"));

    return tx_out_index(tx_hash, result);
}

bool BlockchainBDB::has_key_image(const crypto::key_image& img) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    Dbt_copy<crypto::key_image> val_key(img);
    if (m_spent_keys->exists(DB_DEFAULT_TX, &val_key, 0) == 0)
    {
        return true;
    }

    return false;
}

// Ostensibly BerkeleyDB has batch transaction support built-in,
// so the following few functions will be NOP.

bool BlockchainBDB::batch_start(uint64_t batch_num_blocks)
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    return false;
}

void BlockchainBDB::batch_commit()
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
}

void BlockchainBDB::batch_stop()
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
}

void BlockchainBDB::batch_abort()
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
}

void BlockchainBDB::set_batch_transactions(bool batch_transactions)
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    m_batch_transactions = batch_transactions;
    LOG_PRINT_L3("batch transactions " << (m_batch_transactions ? "enabled" : "disabled"));
}

void BlockchainBDB::block_txn_start(bool readonly)
{
  // TODO
}

void BlockchainBDB::block_txn_stop()
{
  // TODO
}

void BlockchainBDB::block_txn_abort()
{
  // TODO
}

uint64_t BlockchainBDB::add_block(const block& blk, const size_t& block_size, const difficulty_type& cumulative_difficulty, const uint64_t& coins_generated, const std::vector<transaction>& txs)
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    bdb_txn_safe txn;
    if (m_env->txn_begin(NULL, txn, 0))
        throw0(DB_ERROR("Failed to create a transaction for the db"));
    m_write_txn = &txn;

    uint64_t num_outputs = m_num_outputs;
    try
    {
        BlockchainDB::add_block(blk, block_size, cumulative_difficulty, coins_generated, txs);
        m_write_txn = NULL;

        TIME_MEASURE_START(time1);
        txn.commit();
        TIME_MEASURE_FINISH(time1);
        time_commit1 += time1;
    }
    catch (const std::exception& e)
    {
        m_num_outputs = num_outputs;
        m_write_txn = NULL;
        throw;
    }

    return ++m_height;
}

void BlockchainBDB::pop_block(block& blk, std::vector<transaction>& txs)
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    bdb_txn_safe txn;
    if (m_env->txn_begin(NULL, txn, 0))
        throw0(DB_ERROR("Failed to create a transaction for the db"));
    m_write_txn = &txn;

    uint64_t num_outputs = m_num_outputs;
    try
    {
        BlockchainDB::pop_block(blk, txs);

        m_write_txn = NULL;
        txn.commit();
    }
    catch (...)
    {
        m_num_outputs = num_outputs;
        m_write_txn = NULL;
        throw;
    }

    --m_height;
}

void BlockchainBDB::get_output_tx_and_index_from_global(const std::vector<uint64_t> &global_indices, std::vector<tx_out_index> &tx_out_indices) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();
    tx_out_indices.clear();

    for (const uint64_t &index : global_indices)
    {
        Dbt_copy<uint32_t> k(index);
        Dbt_copy<crypto::hash> v;

        auto get_result = m_output_txs->get(DB_DEFAULT_TX, &k, &v, 0);
        if (get_result == DB_NOTFOUND)
            throw1(OUTPUT_DNE("output with given index not in db"));
        else if (get_result)
            throw0(DB_ERROR("DB error attempting to fetch output tx hash"));

        crypto::hash tx_hash = v;

        Dbt_copy<uint64_t> result;
        get_result = m_output_indices->get(DB_DEFAULT_TX, &k, &result, 0);
        if (get_result == DB_NOTFOUND)
            throw1(OUTPUT_DNE("output with given index not in db"));
        else if (get_result)
            throw0(DB_ERROR("DB error attempting to fetch output tx index"));
        auto hashindex = tx_out_index(tx_hash, result);
        tx_out_indices.push_back(hashindex);
    }
}

void BlockchainBDB::get_output_global_indices(const uint64_t& amount, const std::vector<uint64_t> &offsets, std::vector<uint64_t> &global_indices)
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    TIME_MEASURE_START(txx);
    check_open();

    bdb_cur cur(DB_DEFAULT_TX, m_output_amounts);
    uint64_t max = 0;
    for (const uint64_t& index : offsets)
    {
        if (index > max)
            max = index;
    }

    // get returned keypairs count
#define DB_COUNT_RECORDS(dbt, cnt) \
        do { \
            uint32_t *_p = (uint32_t *) ((uint8_t *)(dbt)->data + \
                    (dbt)->ulen - sizeof(uint32_t)); \
                    cnt = 0; \
                    while(*_p != (uint32_t) -1) { \
                        _p -= 2; \
                        ++cnt; \
                    } \
        } while(0); \

    Dbt_copy<uint64_t> k(amount);
    Dbt_copy<uint32_t> v;
    uint64_t buflen = 0;
    uint64_t t_dbmul = 0;
    uint64_t t_dbscan = 0;

    auto result = cur->get(&k, &v, DB_SET);
    if (result == DB_NOTFOUND)
        throw1(OUTPUT_DNE("Attempting to get an output index by amount and amount index, but amount not found"));
    else if (result)
        throw0(DB_ERROR("DB error attempting to get an output"));

    db_recno_t num_elems = 0;
    cur->count(&num_elems, 0);

    if (max <= 1 && num_elems <= max)
        throw1(OUTPUT_DNE("Attempting to get an output index by amount and amount index, but output not found"));

    TIME_MEASURE_START(db2);
    if (max <= 1)
    {
        for (const uint64_t& index : offsets)
        {
            TIME_MEASURE_START(t_seek);

            auto result = cur->get(&k, &v, DB_SET);
            if (result == DB_NOTFOUND)
                throw1(OUTPUT_DNE("Attempting to get an output index by amount and amount index, but amount not found"));
            else if (result)
                throw0(DB_ERROR("DB error attempting to get an output"));

            for (uint64_t i = 0; i < index; ++i)
                cur->get(&k, &v, DB_NEXT_DUP);

            uint64_t glob_index = v;

            LOG_PRINT_L3("L0->v: " << glob_index);
            global_indices.push_back(glob_index);

            TIME_MEASURE_FINISH(t_seek);
        }
    }
    else
    {
        // setup a 256KB minimum buffer size
        uint32_t pagesize = 256 * 1024;

        // Retrieve only a suitable portion of the kvp data, up to somewhere near
        // the maximum offset value being retrieved
        buflen = (max + 1) * 4 * sizeof(uint64_t);
        buflen = ((buflen / pagesize) + ((buflen % pagesize) > 0 ? 1 : 0)) * pagesize;

        bool nomem = false;
        Dbt data;

        bool singlebuff = buflen <= m_buffer.get_buffer_size();
        buflen = buflen < m_buffer.get_buffer_size() ? buflen : m_buffer.get_buffer_size();
        bdb_safe_buffer_t::type buffer = nullptr;
        bdb_safe_buffer_autolock<bdb_safe_buffer_t> lock(m_buffer, buffer);

        data.set_data(buffer);
        data.set_ulen(buflen);
        data.set_size(buflen);
        data.set_flags(DB_DBT_USERMEM);

        uint32_t curcount = 0;
        uint32_t blockstart = 0;
        for (const uint64_t& index : offsets)
        {
            if (index >= num_elems)
            {
                LOG_PRINT_L1("Index: " << index << " Elems: " << num_elems << " partial results found for get_output_tx_and_index");
                break;
            }

            // fixme! for whatever reason, the first call to DB_MULTIPLE | DB_SET does not
            // retrieve the first value.
            if (index <= 1 || nomem)
            {
                auto result = cur->get(&k, &v, DB_SET);
                if (result == DB_NOTFOUND)
                {
                    throw1(OUTPUT_DNE("Attempting to get an output index by amount and amount index, but amount not found"));
                }
                else if (result)
                {
                    throw0(DB_ERROR("DB error attempting to get an output"));
                }

                for (uint64_t i = 0; i < index; ++i)
                    cur->get(&k, &v, DB_NEXT_DUP);
            }
            else
            {
                while (index >= curcount)
                {
                    TIME_MEASURE_START(t_db1);
                    try
                    {
                        cur->get(&k, &data, DB_MULTIPLE | (curcount == 0 ? DB_SET : DB_NEXT_DUP));
                        blockstart = curcount;

                        int count = 0;
                        DB_COUNT_RECORDS((DBT * ) &data, count);
                        curcount += count;
                    }
                    catch (const std::exception &e)
                    {
                        cur.close();
                        throw0(DB_ERROR(std::string("Failed on DB_MULTIPLE: ").append(e.what()).c_str()));
                    }

                    TIME_MEASURE_FINISH(t_db1);
                    t_dbmul += t_db1;
                    if (singlebuff)
                        break;
                }

                LOG_PRINT_L3("Records returned: " << curcount << " Index: " << index);
                TIME_MEASURE_START(t_db2);
                DBT *pdata = (DBT *) &data;

                uint8_t *value;
                uint64_t dlen = 0;

                void *pbase = ((uint8_t *) (pdata->data)) + pdata->ulen - sizeof(uint32_t);
                uint32_t *p = (uint32_t *) pbase;
                if (*p == (uint32_t) -1)
                {
                    value = NULL;
                }
                else
                {
                    p -= (index - blockstart) * 2; // index * 4 + 2; <- if DB_MULTIPLE_KEY
                    value = (uint8_t *) pdata->data + *p--;
                    dlen = *p--;
                    if (value == (uint8_t *) pdata->data)
                        value = NULL;
                }

                if (value != NULL)
                {
                    v = dlen == sizeof(uint64_t) ? *((uint64_t *) value) : *((uint32_t *) value);
                }
                TIME_MEASURE_FINISH(t_db2);
                t_dbscan += t_db2;
            }

            uint64_t glob_index = v;

            LOG_PRINT_L3("L1->v: " << glob_index);
            global_indices.push_back(glob_index);
        }
    }
    TIME_MEASURE_FINISH(db2);

    cur.close();

    TIME_MEASURE_FINISH(txx);

    LOG_PRINT_L3("blen: " << buflen << " txx: " << txx << " db1: " << t_dbmul << " db2: " << t_dbscan);

}

void BlockchainBDB::get_output_key(const uint64_t &amount, const std::vector<uint64_t> &offsets, std::vector<output_data_t> &outputs)
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();
    TIME_MEASURE_START(txx);
    outputs.clear();

    std::vector < uint64_t > global_indices;
    get_output_global_indices(amount, offsets, global_indices);

    TIME_MEASURE_START(db3);
    if (global_indices.size() > 0)
    {
        for (const uint64_t &index : global_indices)
        {
            Dbt_copy<uint32_t> k(index);
            Dbt_copy<output_data_t> v;

            auto get_result = m_output_keys->get(DB_DEFAULT_TX, &k, &v, 0);
            if (get_result == DB_NOTFOUND)
                throw1(OUTPUT_DNE("output with given index not in db"));
            else if (get_result)
                throw0(DB_ERROR("DB error attempting to fetch output tx hash"));

            output_data_t data = *(output_data_t *) v.get_data();
            outputs.push_back(data);
        }
    }

    TIME_MEASURE_FINISH(txx);
    LOG_PRINT_L3("db3: " << db3);
}

void BlockchainBDB::get_output_tx_and_index(const uint64_t& amount, const std::vector<uint64_t> &offsets, std::vector<tx_out_index> &indices)
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    std::vector < uint64_t > global_indices;
    get_output_global_indices(amount, offsets, global_indices);

    TIME_MEASURE_START(db3);
    if (global_indices.size() > 0)
        get_output_tx_and_index_from_global(global_indices, indices);
    TIME_MEASURE_FINISH(db3);

    LOG_PRINT_L3("db3: " << db3);
}

std::map<uint64_t, uint64_t>::BlockchainBDB::get_output_histogram(const std::vector<uint64_t> &amounts) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  throw1(DB_ERROR("Not implemented."));
}

void BlockchainBDB::check_hard_fork_info()
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  if (m_hf_versions == nullptr)
  {
    LOG_PRINT_L0("hf versions DB not open, so not checking");
    return;
  }

  DB_BTREE_STAT* db_stat1, * db_stat2;

  // DB_FAST_STAT can apparently cause an incorrect number of records
  // to be returned.  The flag should be set to 0 instead if this proves
  // to be the case.

  // Set txn to NULL and DB_FAST_STAT to zero (0) for reliability.
  m_blocks->stat(NULL, &db_stat1, 0);
  m_hf_versions->stat(NULL, &db_stat2, 0);
  if (db_stat1->bt_nkeys != db_stat2->bt_nkeys)
  {
    LOG_PRINT_L0("num blocks " << db_stat1->bt_nkeys << " != " << "num hf_versions " << db_stat2->bt_nkeys << " - will clear the two hard fork DBs");

    bdb_txn_safe txn;
    bdb_txn_safe* txn_ptr = &txn;
    if (m_write_txn)
      txn_ptr = m_write_txn;
    else
    {
      if (m_env->txn_begin(NULL, txn, 0))
        throw0(DB_ERROR("Failed to create a transaction for the db"));
    }

    try
    {
      uint32_t count;
      m_hf_starting_heights->truncate(*txn_ptr, &count, 0);
      LOG_PRINT_L0("hf_starting_heights count: " << count);
      m_hf_versions->truncate(*txn_ptr, &count, 0);
      LOG_PRINT_L0("hf_versions count: " << count);

      if (!m_write_txn)
        txn.commit();
    }
    catch (const std::exception& e)
    {
      throw0(DB_ERROR(std::string("Failed to clear two hard fork DBs: ").append(e.what()).c_str()));
    }
  }
  delete db_stat1;
  delete db_stat2;
}

void BlockchainBDB::drop_hard_fork_info()
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  bdb_txn_safe txn;
  bdb_txn_safe* txn_ptr = &txn;
  if (m_write_txn)
    txn_ptr = m_write_txn;
  else
  {
    if (m_env->txn_begin(NULL, txn, 0))
      throw0(DB_ERROR("Failed to create a transaction for the db"));
  }

  try
  {
    m_hf_starting_heights->close(0);
    m_hf_versions->close(0);
    m_hf_starting_heights = nullptr;
    m_hf_versions = nullptr;
    if (m_env->dbremove(*txn_ptr, BDB_HF_STARTING_HEIGHTS, NULL, 0) != 0)
      LOG_ERROR("Error removing hf_starting_heights");
    if (m_env->dbremove(*txn_ptr, BDB_HF_VERSIONS, NULL, 0) != 0)
      LOG_ERROR("Error removing hf_versions");

    if (!m_write_txn)
      txn.commit();
  }
  catch (const std::exception& e)
  {
    throw0(DB_ERROR(std::string("Failed to drop hard fork info: ").append(e.what()).c_str()));
  }
}

void BlockchainBDB::set_hard_fork_version(uint64_t height, uint8_t version)
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    Dbt_copy<uint32_t> val_key(height + 1);
    Dbt_copy<uint8_t> val(version);
    if (m_hf_versions->put(DB_DEFAULT_TX, &val_key, &val, 0))
        throw1(DB_ERROR("Error adding hard fork version to db transaction."));
}

uint8_t BlockchainBDB::get_hard_fork_version(uint64_t height) const
{
    LOG_PRINT_L3("BlockchainBDB::" << __func__);
    check_open();

    Dbt_copy<uint32_t> key(height + 1);
    Dbt_copy<uint8_t> result;

    auto get_result = m_hf_versions->get(DB_DEFAULT_TX, &key, &result, 0);
    if (get_result == DB_NOTFOUND || get_result == DB_KEYEMPTY)
        throw0(OUTPUT_DNE("Error attempting to retrieve hard fork version from the db"));
    else if (get_result)
        throw0(DB_ERROR("Error attempting to retrieve hard fork version from the db"));

    return result;
}

void BlockchainBDB::checkpoint_worker() const
{
    LOG_PRINT_L0("Entering BDB checkpoint thread.");
    int count = 0;
    while(m_run_checkpoint && m_open)
    {
        // sleep every second, so we don't delay exit condition m_run_checkpoint = false
        sleep(1);
        // checkpoint every 5 minutes
        if(count++ >= 300)
        {
            count = 0;
            if(m_env->txn_checkpoint(0, 0, 0) != 0)
            {
                LOG_PRINT_L0("BDB txn_checkpoint failed.");
                break;
            }
        }
    }
    LOG_PRINT_L0("Leaving BDB checkpoint thread.");
}

bool BlockchainBDB::is_read_only() const
{
  return false;
}

void BlockchainBDB::fixup()
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  // Always call parent as well
  BlockchainDB::fixup();
}

}  // namespace cryptonote
