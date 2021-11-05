// Copyright (c) 2018-2022, The Monero Project

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

#include <lmdb.h>
#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/filesystem.hpp>
#include "common/util.h"
#include "misc_log_ex.h"
#include "misc_language.h"
#include "wallet_errors.h"
#include "ringdb.h"
#include "cryptonote_config.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet.ringdb"

#define V1TAG ((uint64_t)798237759845202)

static int compare_hash32(const MDB_val *a, const MDB_val *b)
{
  uint32_t *va = (uint32_t*) a->mv_data;
  uint32_t *vb = (uint32_t*) b->mv_data;
  for (int n = 7; n >= 0; n--)
  {
    if (va[n] == vb[n])
      continue;
    return va[n] < vb[n] ? -1 : 1;
  }

  return 0;
}

static int compare_uint64(const MDB_val *a, const MDB_val *b)
{
  const uint64_t va = *(const uint64_t*) a->mv_data;
  const uint64_t vb = *(const uint64_t*) b->mv_data;
  return va < vb ? -1 : va > vb;
}

static std::string compress_ring(const std::vector<uint64_t> &ring, uint64_t tag)
{
  std::string s;
  s += tools::get_varint_data(tag);
  for (uint64_t out: ring)
    s += tools::get_varint_data(out);
  return s;
}

static std::vector<uint64_t> decompress_ring(const std::string &s, uint64_t tag)
{
  std::vector<uint64_t> ring;
  int read = 0;
  for (std::string::const_iterator i = s.begin(); i != s.cend(); std::advance(i, read))
  {
    uint64_t out;
    std::string tmp(i, s.cend());
    read = tools::read_varint(tmp.begin(), tmp.end(), out);
    THROW_WALLET_EXCEPTION_IF(read <= 0 || read > 256, tools::error::wallet_internal_error, "Internal error decompressing ring");
    if (tag)
    {
      if (tag != out)
        return {};
      tag = 0;
      continue;
    }
    ring.push_back(out);
  }
  return ring;
}

std::string get_rings_filename(boost::filesystem::path filename)
{
  if (!boost::filesystem::is_directory(filename))
    filename.remove_filename();
  return filename.string();
}

static crypto::chacha_iv make_iv(const crypto::key_image &key_image, const crypto::chacha_key &key, uint8_t field)
{
  uint8_t buffer[sizeof(key_image) + sizeof(key) + sizeof(config::HASH_KEY_RINGDB) + sizeof(field)];
  memcpy(buffer, &key_image, sizeof(key_image));
  memcpy(buffer + sizeof(key_image), &key, sizeof(key));
  memcpy(buffer + sizeof(key_image) + sizeof(key), config::HASH_KEY_RINGDB, sizeof(config::HASH_KEY_RINGDB));
  memcpy(buffer + sizeof(key_image) + sizeof(key) + sizeof(config::HASH_KEY_RINGDB), &field, sizeof(field));
  crypto::hash hash;
  // if field is 0, backward compat mode: hash without the field
  crypto::cn_fast_hash(buffer, sizeof(buffer) - !field, hash.data);
  static_assert(sizeof(hash) >= CHACHA_IV_SIZE, "Incompatible hash and chacha IV sizes");
  crypto::chacha_iv iv;
  memcpy(&iv, &hash, CHACHA_IV_SIZE);
  return iv;
}

static std::string encrypt(const std::string &plaintext, const crypto::key_image &key_image, const crypto::chacha_key &key, uint8_t field)
{
  const crypto::chacha_iv iv = make_iv(key_image, key, field);
  std::string ciphertext;
  ciphertext.resize(plaintext.size() + sizeof(iv));
  crypto::chacha20(plaintext.data(), plaintext.size(), key, iv, &ciphertext[sizeof(iv)]);
  memcpy(&ciphertext[0], &iv, sizeof(iv));
  return ciphertext;
}

static std::string encrypt(const crypto::key_image &key_image, const crypto::chacha_key &key, uint8_t field)
{
  return encrypt(std::string((const char*)&key_image, sizeof(key_image)), key_image, key, field);
}

static std::string decrypt(const std::string &ciphertext, const crypto::key_image &key_image, const crypto::chacha_key &key, uint8_t field)
{
  const crypto::chacha_iv iv = make_iv(key_image, key, field);
  std::string plaintext;
  THROW_WALLET_EXCEPTION_IF(ciphertext.size() < sizeof(iv), tools::error::wallet_internal_error, "Bad ciphertext text");
  plaintext.resize(ciphertext.size() - sizeof(iv));
  crypto::chacha20(ciphertext.data() + sizeof(iv), ciphertext.size() - sizeof(iv), key, iv, &plaintext[0]);
  return plaintext;
}

static void store_relative_ring(MDB_txn *txn, MDB_dbi &dbi, const crypto::key_image &key_image, const std::vector<uint64_t> &relative_ring, const crypto::chacha_key &chacha_key)
{
  MDB_val key, data;
  std::string key_ciphertext = encrypt(key_image, chacha_key, 0);
  key.mv_data = (void*)key_ciphertext.data();
  key.mv_size = key_ciphertext.size();
  std::string compressed_ring = compress_ring(relative_ring, V1TAG);
  std::string data_ciphertext = encrypt(compressed_ring, key_image, chacha_key, 1);
  data.mv_size = data_ciphertext.size();
  data.mv_data = (void*)data_ciphertext.c_str();
  int dbr = mdb_put(txn, dbi, &key, &data, 0);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to set ring for key image in LMDB table: " + std::string(mdb_strerror(dbr)));
}

static int resize_env(MDB_env *env, const char *db_path, size_t needed)
{
  MDB_envinfo mei;
  MDB_stat mst;
  int ret;

  needed = std::max(needed, (size_t)(100ul * 1024 * 1024)); // at least 100 MB

  ret = mdb_env_info(env, &mei);
  if (ret)
    return ret;
  ret = mdb_env_stat(env, &mst);
  if (ret)
    return ret;
  uint64_t size_used = mst.ms_psize * mei.me_last_pgno;
  uint64_t mapsize = mei.me_mapsize;
  if (size_used + needed > mei.me_mapsize)
  {
    try
    {
      boost::filesystem::path path(db_path);
      boost::filesystem::space_info si = boost::filesystem::space(path);
      if(si.available < needed)
      {
        MERROR("!! WARNING: Insufficient free space to extend database !!: " << (si.available >> 20L) << " MB available");
        return ENOSPC;
      }
    }
    catch(...)
    {
      // print something but proceed.
      MWARNING("Unable to query free disk space.");
    }

    mapsize += needed;
  }
  return mdb_env_set_mapsize(env, mapsize);
}

static size_t get_ring_data_size(size_t n_entries)
{
  return n_entries * (32 + 1024); // highball 1kB for the ring data to make sure
}

enum { BLACKBALL_BLACKBALL, BLACKBALL_UNBLACKBALL, BLACKBALL_QUERY, BLACKBALL_CLEAR};

namespace tools
{

ringdb::ringdb(std::string filename, const std::string &genesis):
  filename(filename),
  env(NULL)
{
  MDB_txn *txn;
  bool tx_active = false;
  int dbr;

  tools::create_directories_if_necessary(filename);

  dbr = mdb_env_create(&env);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to create LDMB environment: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_env_set_maxdbs(env, 2);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to set max env dbs: " + std::string(mdb_strerror(dbr)));
  const std::string actual_filename = get_rings_filename(filename); 
  dbr = mdb_env_open(env, actual_filename.c_str(), 0, 0664);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to open rings database file '"
      + actual_filename + "': " + std::string(mdb_strerror(dbr)));

  dbr = mdb_txn_begin(env, NULL, 0, &txn);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
  epee::misc_utils::auto_scope_leave_caller txn_dtor = epee::misc_utils::create_scope_leave_handler([&](){if (tx_active) mdb_txn_abort(txn);});
  tx_active = true;

  dbr = mdb_dbi_open(txn, ("rings-" + genesis).c_str(), MDB_CREATE, &dbi_rings);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));
  mdb_set_compare(txn, dbi_rings, compare_hash32);

  dbr = mdb_dbi_open(txn, ("blackballs2-" + genesis).c_str(), MDB_CREATE | MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED, &dbi_blackballs);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));
  mdb_set_dupsort(txn, dbi_blackballs, compare_uint64);

  dbr = mdb_txn_commit(txn);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to commit txn creating/opening database: " + std::string(mdb_strerror(dbr)));
  tx_active = false;
}

ringdb::~ringdb()
{
  close();
}

void ringdb::close()
{
  if (env)
  {
    mdb_dbi_close(env, dbi_rings);
    mdb_dbi_close(env, dbi_blackballs);
    mdb_env_close(env);
    env = NULL;
  }
}

bool ringdb::add_rings(const crypto::chacha_key &chacha_key, const cryptonote::transaction_prefix &tx)
{
  MDB_txn *txn;
  int dbr;
  bool tx_active = false;

  dbr = resize_env(env, filename.c_str(), get_ring_data_size(tx.vin.size()));
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to set env map size");
  dbr = mdb_txn_begin(env, NULL, 0, &txn);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
  epee::misc_utils::auto_scope_leave_caller txn_dtor = epee::misc_utils::create_scope_leave_handler([&](){if (tx_active) mdb_txn_abort(txn);});
  tx_active = true;

  for (const auto &in: tx.vin)
  {
    if (in.type() != typeid(cryptonote::txin_to_key))
      continue;
    const auto &txin = boost::get<cryptonote::txin_to_key>(in);
    const uint32_t ring_size = txin.key_offsets.size();
    if (ring_size == 1)
      continue;

    store_relative_ring(txn, dbi_rings, txin.k_image, txin.key_offsets, chacha_key);
  }

  dbr = mdb_txn_commit(txn);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to commit txn adding ring to database: " + std::string(mdb_strerror(dbr)));
  tx_active = false;
  return true;
}

bool ringdb::remove_rings(const crypto::chacha_key &chacha_key, const std::vector<crypto::key_image> &key_images)
{
  MDB_txn *txn;
  int dbr;
  bool tx_active = false;

  dbr = resize_env(env, filename.c_str(), 0);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to set env map size");
  dbr = mdb_txn_begin(env, NULL, 0, &txn);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
  epee::misc_utils::auto_scope_leave_caller txn_dtor = epee::misc_utils::create_scope_leave_handler([&](){if (tx_active) mdb_txn_abort(txn);});
  tx_active = true;

  for (const crypto::key_image &key_image: key_images)
  {
    MDB_val key, data;
    std::string key_ciphertext = encrypt(key_image, chacha_key, 0);
    key.mv_data = (void*)key_ciphertext.data();
    key.mv_size = key_ciphertext.size();

    dbr = mdb_get(txn, dbi_rings, &key, &data);
    THROW_WALLET_EXCEPTION_IF(dbr && dbr != MDB_NOTFOUND, tools::error::wallet_internal_error, "Failed to look for key image in LMDB table: " + std::string(mdb_strerror(dbr)));
    if (dbr == MDB_NOTFOUND)
      continue;
    THROW_WALLET_EXCEPTION_IF(data.mv_size <= 0, tools::error::wallet_internal_error, "Invalid ring data size");

    MDEBUG("Removing ring data for key image " << key_image);
    dbr = mdb_del(txn, dbi_rings, &key, NULL);
    THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to remove ring to database: " + std::string(mdb_strerror(dbr)));
  }

  dbr = mdb_txn_commit(txn);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to commit txn removing ring to database: " + std::string(mdb_strerror(dbr)));
  tx_active = false;
  return true;
}

bool ringdb::remove_rings(const crypto::chacha_key &chacha_key, const cryptonote::transaction_prefix &tx)
{
  std::vector<crypto::key_image> key_images;
  key_images.reserve(tx.vin.size());
  for (const auto &in: tx.vin)
  {
    if (in.type() != typeid(cryptonote::txin_to_key))
      continue;
    const auto &txin = boost::get<cryptonote::txin_to_key>(in);
    const uint32_t ring_size = txin.key_offsets.size();
    if (ring_size == 1)
      continue;
    key_images.push_back(txin.k_image);
  }
  return remove_rings(chacha_key, key_images);
}

bool ringdb::get_rings(const crypto::chacha_key &chacha_key, const std::vector<crypto::key_image> &key_images, std::vector<std::vector<uint64_t>> &all_outs)
{
  MDB_txn *txn;
  int dbr;
  bool tx_active = false;

  all_outs.clear();
  all_outs.reserve(key_images.size());

  dbr = resize_env(env, filename.c_str(), 0);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to set env map size: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_txn_begin(env, NULL, 0, &txn);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
  epee::misc_utils::auto_scope_leave_caller txn_dtor = epee::misc_utils::create_scope_leave_handler([&](){if (tx_active) mdb_txn_abort(txn);});
  tx_active = true;

  for (size_t i = 0; i < key_images.size(); ++i)
  {
  const crypto::key_image &key_image = key_images[i];

  MDB_val key, data;
  std::string key_ciphertext = encrypt(key_image, chacha_key, 0);
  key.mv_data = (void*)key_ciphertext.data();
  key.mv_size = key_ciphertext.size();
  dbr = mdb_get(txn, dbi_rings, &key, &data);
  THROW_WALLET_EXCEPTION_IF(dbr && dbr != MDB_NOTFOUND, tools::error::wallet_internal_error, "Failed to look for key image in LMDB table: " + std::string(mdb_strerror(dbr)));
  if (dbr == MDB_NOTFOUND)
    return false;
  THROW_WALLET_EXCEPTION_IF(data.mv_size <= 0, tools::error::wallet_internal_error, "Invalid ring data size");

  std::vector<uint64_t> outs;
  bool try_v0 = false;
  std::string data_plaintext = decrypt(std::string((const char*)data.mv_data, data.mv_size), key_image, chacha_key, 1);
  try { outs = decompress_ring(data_plaintext, V1TAG); if (outs.empty()) try_v0 = true; }
  catch(...) { try_v0 = true; }
  if (try_v0)
  {
    data_plaintext = decrypt(std::string((const char*)data.mv_data, data.mv_size), key_image, chacha_key, 0);
    outs = decompress_ring(data_plaintext, 0);
  }
  MDEBUG("Found ring for key image " << key_image << ":");
  MDEBUG("Relative: " << boost::join(outs | boost::adaptors::transformed([](uint64_t out){return std::to_string(out);}), " "));
  outs = cryptonote::relative_output_offsets_to_absolute(outs);
  MDEBUG("Absolute: " << boost::join(outs | boost::adaptors::transformed([](uint64_t out){return std::to_string(out);}), " "));
  all_outs.push_back(std::move(outs));

  }

  dbr = mdb_txn_commit(txn);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to commit txn getting ring from database: " + std::string(mdb_strerror(dbr)));
  tx_active = false;
  return true;
}

bool ringdb::get_ring(const crypto::chacha_key &chacha_key, const crypto::key_image &key_image, std::vector<uint64_t> &outs)
{
  std::vector<std::vector<uint64_t>> all_outs;
  if (!get_rings(chacha_key, std::vector<crypto::key_image>(1, key_image), all_outs))
    return false;
  outs = std::move(all_outs.front());
  return true;
}

bool ringdb::set_rings(const crypto::chacha_key &chacha_key, const std::vector<std::pair<crypto::key_image, std::vector<uint64_t>>> &rings, bool relative)
{
  MDB_txn *txn;
  int dbr;
  bool tx_active = false;

  size_t n_outs = 0;
  for (const auto &e: rings)
    n_outs += e.second.size();
  dbr = resize_env(env, filename.c_str(), n_outs * 64);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to set env map size: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_txn_begin(env, NULL, 0, &txn);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
  epee::misc_utils::auto_scope_leave_caller txn_dtor = epee::misc_utils::create_scope_leave_handler([&](){if (tx_active) mdb_txn_abort(txn);});
  tx_active = true;

  for (const auto &e: rings)
    store_relative_ring(txn, dbi_rings, e.first, relative ? e.second : cryptonote::absolute_output_offsets_to_relative(e.second), chacha_key);

  dbr = mdb_txn_commit(txn);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to commit txn setting ring to database: " + std::string(mdb_strerror(dbr)));
  tx_active = false;
  return true;
}

bool ringdb::set_ring(const crypto::chacha_key &chacha_key, const crypto::key_image &key_image, const std::vector<uint64_t> &outs, bool relative)
{
  std::vector<std::pair<crypto::key_image, std::vector<uint64_t>>> rings;
  rings.push_back(std::make_pair(key_image, outs));
  return set_rings(chacha_key, rings, relative);
}

bool ringdb::blackball_worker(const std::vector<std::pair<uint64_t, uint64_t>> &outputs, int op)
{
  MDB_txn *txn;
  MDB_cursor *cursor;
  int dbr;
  bool tx_active = false;
  bool ret = true;

  THROW_WALLET_EXCEPTION_IF(outputs.size() > 1 && op == BLACKBALL_QUERY, tools::error::wallet_internal_error, "Blackball query only makes sense for a single output");

  dbr = resize_env(env, filename.c_str(), 32 * 2 * outputs.size()); // a pubkey, and some slack
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to set env map size: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_txn_begin(env, NULL, 0, &txn);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
  epee::misc_utils::auto_scope_leave_caller txn_dtor = epee::misc_utils::create_scope_leave_handler([&](){if (tx_active) mdb_txn_abort(txn);});
  tx_active = true;

  dbr = mdb_cursor_open(txn, dbi_blackballs, &cursor);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to create cursor for blackballs table: " + std::string(mdb_strerror(dbr)));

  MDB_val key, data;
  for (const std::pair<uint64_t, uint64_t> &output: outputs)
  {
    key.mv_data = (void*)&output.first;
    key.mv_size = sizeof(output.first);
    data.mv_data = (void*)&output.second;
    data.mv_size = sizeof(output.second);

    switch (op)
    {
      case BLACKBALL_BLACKBALL:
        MDEBUG("Marking output " << output.first << "/" << output.second << " as spent");
        dbr = mdb_cursor_put(cursor, &key, &data, MDB_NODUPDATA);
        if (dbr == MDB_KEYEXIST)
          dbr = 0;
        break;
      case BLACKBALL_UNBLACKBALL:
        MDEBUG("Marking output " << output.first << "/" << output.second << " as unspent");
        dbr = mdb_cursor_get(cursor, &key, &data, MDB_GET_BOTH);
        if (dbr == 0)
          dbr = mdb_cursor_del(cursor, 0);
        break;
      case BLACKBALL_QUERY:
        dbr = mdb_cursor_get(cursor, &key, &data, MDB_GET_BOTH);
        THROW_WALLET_EXCEPTION_IF(dbr && dbr != MDB_NOTFOUND, tools::error::wallet_internal_error, "Failed to lookup in blackballs table: " + std::string(mdb_strerror(dbr)));
        ret = dbr != MDB_NOTFOUND;
        if (dbr == MDB_NOTFOUND)
          dbr = 0;
        break;
      case BLACKBALL_CLEAR:
        break;
      default:
        THROW_WALLET_EXCEPTION(tools::error::wallet_internal_error, "Invalid blackball op");
    }
    THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to query blackballs table: " + std::string(mdb_strerror(dbr)));
  }

  mdb_cursor_close(cursor);

  if (op == BLACKBALL_CLEAR)
  {
    dbr = mdb_drop(txn, dbi_blackballs, 0);
    THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to clear blackballs table: " + std::string(mdb_strerror(dbr)));
  }

  dbr = mdb_txn_commit(txn);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to commit txn blackballing output to database: " + std::string(mdb_strerror(dbr)));
  tx_active = false;
  return ret;
}

bool ringdb::blackball(const std::vector<std::pair<uint64_t, uint64_t>> &outputs)
{
  return blackball_worker(outputs, BLACKBALL_BLACKBALL);
}

bool ringdb::blackball(const std::pair<uint64_t, uint64_t> &output)
{
  std::vector<std::pair<uint64_t, uint64_t>> outputs(1, output);
  return blackball_worker(outputs, BLACKBALL_BLACKBALL);
}

bool ringdb::unblackball(const std::pair<uint64_t, uint64_t> &output)
{
  std::vector<std::pair<uint64_t, uint64_t>> outputs(1, output);
  return blackball_worker(outputs, BLACKBALL_UNBLACKBALL);
}

bool ringdb::blackballed(const std::pair<uint64_t, uint64_t> &output)
{
  std::vector<std::pair<uint64_t, uint64_t>> outputs(1, output);
  return blackball_worker(outputs, BLACKBALL_QUERY);
}

bool ringdb::clear_blackballs()
{
  return blackball_worker(std::vector<std::pair<uint64_t, uint64_t>>(), BLACKBALL_CLEAR);
}

}
