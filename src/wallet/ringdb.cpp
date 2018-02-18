// Copyright (c) 2018, The Monero Project
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
#include "misc_log_ex.h"
#include "misc_language.h"
#include "wallet_errors.h"
#include "ringdb.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet.ringdb"

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

static std::string compress_ring(const std::vector<uint64_t> &ring)
{
  std::string s;
  for (uint64_t out: ring)
    s += tools::get_varint_data(out);
  return s;
}

static std::vector<uint64_t> decompress_ring(const std::string &s)
{
  std::vector<uint64_t> ring;
  int read = 0;
  for (std::string::const_iterator i = s.begin(); i != s.cend(); std::advance(i, read))
  {
    uint64_t out;
    std::string tmp(i, s.cend());
    read = tools::read_varint(tmp.begin(), tmp.end(), out);
    THROW_WALLET_EXCEPTION_IF(read <= 0 || read > 256, tools::error::wallet_internal_error, "Internal error decompressing ring");
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

static crypto::chacha_iv make_iv(const crypto::key_image &key_image, const crypto::chacha_key &key)
{
  static const char salt[] = "ringdsb";

  uint8_t buffer[sizeof(key_image) + sizeof(key) + sizeof(salt)];
  memcpy(buffer, &key_image, sizeof(key_image));
  memcpy(buffer + sizeof(key_image), &key, sizeof(key));
  memcpy(buffer + sizeof(key_image) + sizeof(key), salt, sizeof(salt));
  crypto::hash hash;
  crypto::cn_fast_hash(buffer, sizeof(buffer), hash.data);
  static_assert(sizeof(hash) >= CHACHA_IV_SIZE, "Incompatible hash and chacha IV sizes");
  crypto::chacha_iv iv;
  memcpy(&iv, &hash, CHACHA_IV_SIZE);
  return iv;
}

static std::string encrypt(const std::string &plaintext, const crypto::key_image &key_image, const crypto::chacha_key &key)
{
  const crypto::chacha_iv iv = make_iv(key_image, key);
  std::string ciphertext;
  ciphertext.resize(plaintext.size() + sizeof(iv));
  crypto::chacha20(plaintext.data(), plaintext.size(), key, iv, &ciphertext[sizeof(iv)]);
  memcpy(&ciphertext[0], &iv, sizeof(iv));
  return ciphertext;
}

static std::string encrypt(const crypto::key_image &key_image, const crypto::chacha_key &key)
{
  return encrypt(std::string((const char*)&key_image, sizeof(key_image)), key_image, key);
}

static std::string decrypt(const std::string &ciphertext, const crypto::key_image &key_image, const crypto::chacha_key &key)
{
  const crypto::chacha_iv iv = make_iv(key_image, key);
  std::string plaintext;
  THROW_WALLET_EXCEPTION_IF(ciphertext.size() < sizeof(iv), tools::error::wallet_internal_error, "Bad ciphertext text");
  plaintext.resize(ciphertext.size() - sizeof(iv));
  crypto::chacha20(ciphertext.data() + sizeof(iv), ciphertext.size() - sizeof(iv), key, iv, &plaintext[0]);
  return plaintext;
}

static int resize_env(MDB_env *env, const char *db_path, size_t n_entries)
{
  MDB_envinfo mei;
  MDB_stat mst;
  int ret;

  ret = mdb_env_info(env, &mei);
  if (ret)
    return ret;
  ret = mdb_env_stat(env, &mst);
  if (ret)
    return ret;
  uint64_t size_used = mst.ms_psize * mei.me_last_pgno;
  const size_t needed = n_entries * (32 + 1024); // highball 1kB for the ring data to make sure
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

namespace tools { namespace ringdb
{

bool add_rings(const std::string &filename, const crypto::chacha_key &chacha_key, const cryptonote::transaction_prefix &tx)
{
  MDB_env *env;
  MDB_dbi dbi;
  MDB_txn *txn;
  int dbr;
  bool tx_active = false;

  if (filename.empty())
    return true;
  tools::create_directories_if_necessary(filename);

  dbr = mdb_env_create(&env);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to create LDMB environment: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_env_set_maxdbs(env, 1);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to set max env dbs: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_env_open(env, get_rings_filename(filename).c_str(), 0, 0664);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to open rings database file: " + std::string(mdb_strerror(dbr)));
  epee::misc_utils::auto_scope_leave_caller env_dtor = epee::misc_utils::create_scope_leave_handler([&](){mdb_env_close(env);});
  dbr = resize_env(env, filename.c_str(), tx.vin.size());
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to set env map size: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_txn_begin(env, NULL, 0, &txn);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
  epee::misc_utils::auto_scope_leave_caller txn_dtor = epee::misc_utils::create_scope_leave_handler([&](){if (tx_active) mdb_txn_abort(txn);});
  tx_active = true;
  dbr = mdb_dbi_open(txn, "rings", MDB_CREATE, &dbi);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));
  epee::misc_utils::auto_scope_leave_caller dbi_dtor = epee::misc_utils::create_scope_leave_handler([&](){mdb_dbi_close(env, dbi);});
  mdb_set_compare(txn, dbi, compare_hash32);

  for (const auto &in: tx.vin)
  {
    if (in.type() != typeid(cryptonote::txin_to_key))
      continue;
    const auto &txin = boost::get<cryptonote::txin_to_key>(in);
    const uint32_t ring_size = txin.key_offsets.size();
    if (ring_size == 1)
      continue;

    MDB_val key, data;
    std::string key_ciphertext = encrypt(txin.k_image, chacha_key);
    key.mv_data = (void*)key_ciphertext.data();
    key.mv_size = key_ciphertext.size();
    MDEBUG("Saving relative ring for key image " << txin.k_image << ": " <<
        boost::join(txin.key_offsets | boost::adaptors::transformed([](uint64_t out){return std::to_string(out);}), " "));
    std::string compressed_ring = compress_ring(txin.key_offsets);
    std::string data_ciphertext = encrypt(compressed_ring, txin.k_image, chacha_key);
    data.mv_size = data_ciphertext.size();
    data.mv_data = (void*)data_ciphertext.c_str();
    dbr = mdb_put(txn, dbi, &key, &data, 0);
    THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to add ring to database: " + std::string(mdb_strerror(dbr)));
  }

  dbr = mdb_txn_commit(txn);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to commit txn adding ring to database: " + std::string(mdb_strerror(dbr)));
  tx_active = false;
  return true;
}

bool remove_rings(const std::string &filename, const crypto::chacha_key &chacha_key, const cryptonote::transaction_prefix &tx)
{
  MDB_env *env;
  MDB_dbi dbi;
  MDB_txn *txn;
  int dbr;
  bool tx_active = false;

  if (filename.empty())
    return true;
  tools::create_directories_if_necessary(filename);

  dbr = mdb_env_create(&env);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to create LDMB environment: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_env_set_maxdbs(env, 1);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to set max env dbs: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_env_open(env, get_rings_filename(filename).c_str(), 0, 0664);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to open rings database file: " + std::string(mdb_strerror(dbr)));
  epee::misc_utils::auto_scope_leave_caller env_dtor = epee::misc_utils::create_scope_leave_handler([&](){mdb_env_close(env);});
  dbr = resize_env(env, filename.c_str(), tx.vin.size());
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to set env map size: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_txn_begin(env, NULL, 0, &txn);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
  epee::misc_utils::auto_scope_leave_caller txn_dtor = epee::misc_utils::create_scope_leave_handler([&](){if (tx_active) mdb_txn_abort(txn);});
  tx_active = true;
  dbr = mdb_dbi_open(txn, "rings", MDB_CREATE, &dbi);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));
  epee::misc_utils::auto_scope_leave_caller dbi_dtor = epee::misc_utils::create_scope_leave_handler([&](){mdb_dbi_close(env, dbi);});
  mdb_set_compare(txn, dbi, compare_hash32);

  for (const auto &in: tx.vin)
  {
    if (in.type() != typeid(cryptonote::txin_to_key))
      continue;
    const auto &txin = boost::get<cryptonote::txin_to_key>(in);
    const uint32_t ring_size = txin.key_offsets.size();
    if (ring_size == 1)
      continue;

    MDB_val key, data;
    std::string key_ciphertext = encrypt(txin.k_image, chacha_key);
    key.mv_data = (void*)key_ciphertext.data();
    key.mv_size = key_ciphertext.size();

    dbr = mdb_get(txn, dbi, &key, &data);
    THROW_WALLET_EXCEPTION_IF(dbr && dbr != MDB_NOTFOUND, tools::error::wallet_internal_error, "Failed to look for key image in LMDB table: " + std::string(mdb_strerror(dbr)));
    if (dbr == MDB_NOTFOUND)
      continue;
    THROW_WALLET_EXCEPTION_IF(data.mv_size <= 0, tools::error::wallet_internal_error, "Invalid ring data size");

    MDEBUG("Removing ring data for key image " << txin.k_image);
    dbr = mdb_del(txn, dbi, &key, NULL);
    THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to remove ring to database: " + std::string(mdb_strerror(dbr)));
  }

  dbr = mdb_txn_commit(txn);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to commit txn removing ring to database: " + std::string(mdb_strerror(dbr)));
  tx_active = false;
  return true;
}

bool get_ring(const std::string &filename, const crypto::chacha_key &chacha_key, const crypto::key_image &key_image, std::vector<uint64_t> &outs)
{
  MDB_env *env;
  MDB_dbi dbi;
  MDB_txn *txn;
  int dbr;
  bool tx_active = false;

  if (filename.empty())
    return false;
  tools::create_directories_if_necessary(filename);

  dbr = mdb_env_create(&env);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to create LDMB environment: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_env_set_maxdbs(env, 1);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to set max env dbs: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_env_open(env, get_rings_filename(filename).c_str(), 0, 0664);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to open rings database file: " + std::string(mdb_strerror(dbr)));
  epee::misc_utils::auto_scope_leave_caller env_dtor = epee::misc_utils::create_scope_leave_handler([&](){mdb_env_close(env);});
  dbr = resize_env(env, filename.c_str(), 0);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to set env map size: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_txn_begin(env, NULL, 0, &txn);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
  epee::misc_utils::auto_scope_leave_caller txn_dtor = epee::misc_utils::create_scope_leave_handler([&](){if (tx_active) mdb_txn_abort(txn);});
  tx_active = true;
  dbr = mdb_dbi_open(txn, "rings", MDB_CREATE, &dbi);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));
  epee::misc_utils::auto_scope_leave_caller dbi_dtor = epee::misc_utils::create_scope_leave_handler([&](){mdb_dbi_close(env, dbi);});
  mdb_set_compare(txn, dbi, compare_hash32);

  MDB_val key, data;
  std::string key_ciphertext = encrypt(key_image, chacha_key);
  key.mv_data = (void*)key_ciphertext.data();
  key.mv_size = key_ciphertext.size();
  dbr = mdb_get(txn, dbi, &key, &data);
  THROW_WALLET_EXCEPTION_IF(dbr && dbr != MDB_NOTFOUND, tools::error::wallet_internal_error, "Failed to look for key image in LMDB table: " + std::string(mdb_strerror(dbr)));
  if (dbr == MDB_NOTFOUND)
    return false;
  THROW_WALLET_EXCEPTION_IF(data.mv_size <= 0, tools::error::wallet_internal_error, "Invalid ring data size");

  std::string data_plaintext = decrypt(std::string((const char*)data.mv_data, data.mv_size), key_image, chacha_key);
  outs = decompress_ring(data_plaintext);
  MDEBUG("Found ring for key image " << key_image << ":");
  MDEBUG("Relative: " << boost::join(outs | boost::adaptors::transformed([](uint64_t out){return std::to_string(out);}), " "));
  outs = cryptonote::relative_output_offsets_to_absolute(outs);
  MDEBUG("Absolute: " << boost::join(outs | boost::adaptors::transformed([](uint64_t out){return std::to_string(out);}), " "));

  dbr = mdb_txn_commit(txn);
  THROW_WALLET_EXCEPTION_IF(dbr, tools::error::wallet_internal_error, "Failed to commit txn getting ring from database: " + std::string(mdb_strerror(dbr)));
  tx_active = false;
  return true;
}

}}
