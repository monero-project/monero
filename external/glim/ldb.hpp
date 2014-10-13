#ifndef _GLIM_LDB_HPP_INCLUDED
#define _GLIM_LDB_HPP_INCLUDED

/**
 * Leveldb (http://code.google.com/p/leveldb/) wrapper.
 * @code
Copyright 2012 Kozarezov Artem Aleksandrovich

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
 * @endcode
 * @file
 */

#include <string>
//#include <unordered_map> // having SIGFPE as in http://stackoverflow.com/q/13580823/257568
#include <map>
#include <climits> // CHAR_MAX

#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <leveldb/filter_policy.h>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/noncopyable.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/iterator_range.hpp> // http://www.boost.org/doc/libs/1_52_0/libs/range/doc/html/range/reference/utilities/iterator_range.html

#include <arpa/inet.h> // htonl, ntohl
#include <sys/stat.h> // mkdir
#include <sys/types.h> // mkdir
#include <string.h> // strerror
#include <errno.h>
#include <sstream>

#include "gstring.hpp"
#include "exception.hpp"

namespace glim {

G_DEFINE_EXCEPTION (LdbEx);

template <typename T> inline void ldbSerialize (gstring& bytes, const T& data) {
  gstring_stream stream (bytes);
  boost::archive::binary_oarchive oa (stream, boost::archive::no_header);
  oa << data;
}
template <typename V> inline void ldbDeserialize (const gstring& bytes, V& data) {
  gstring_stream stream (const_cast<gstring&> (bytes));
  boost::archive::binary_iarchive ia (stream, boost::archive::no_header);
  ia >> data;
}

/** uint32_t keys are stored big-endian (network byte order) in order to be compatible with lexicographic ordering. */
template <> inline void ldbSerialize<uint32_t> (gstring& bytes, const uint32_t& ui) {
  uint32_t nui = htonl (ui); bytes.append ((const char*) &nui, sizeof (uint32_t));}
/** Deserialize uint32_t from big-endian (network byte order). */
template <> inline void ldbDeserialize<uint32_t> (const gstring& bytes, uint32_t& ui) {
  if (bytes.size() != sizeof (uint32_t)) GNTHROW (LdbEx, "Not uint32_t, wrong number of bytes");
  uint32_t nui = * (uint32_t*) bytes.data(); ui = ntohl (nui);}

/** If the data is `gstring` then use the data's buffer directly, no copy. */
template <> inline void ldbSerialize<gstring> (gstring& bytes, const gstring& data) {
  bytes = gstring (0, (void*) data.data(), false, data.length());}
/** Deserializing into `gstring` copies the bytes into it, reusing its buffer. */
template <> inline void ldbDeserialize<gstring> (const gstring& bytes, gstring& data) {
  data.clear() << bytes;}

/** If the data is `std::string` then use the data's buffer directly, no copy. */
template <> inline void ldbSerialize<std::string> (gstring& bytes, const std::string& data) {
  bytes = gstring (0, (void*) data.data(), false, data.length());}
/** Deserializing into `std::string` copies the bytes into it, reusing its buffer. */
template <> inline void ldbDeserialize<std::string> (const gstring& bytes, std::string& data) {
  data.clear(); data.append (bytes.data(), bytes.size());}

/**
 * Header-only Leveldb wrapper.\n
 * Uses Boost Serialization to pack keys and values (glim::gstring can be used for raw bytes).\n
 * Allows semi-automatic indexing with triggers.
 */
struct Ldb {
  std::shared_ptr<leveldb::DB> _db;
  std::shared_ptr<const leveldb::FilterPolicy> _filter;

  struct IteratorEntry { ///< Something to be `dereference`d from the Iterator. Also a pImpl allowing to keep the `_valid` and the `_lit` in sync.
    leveldb::Iterator* _lit;
    bool _valid:1;

    IteratorEntry (const IteratorEntry&) = delete; // Owns `leveldb::Iterator`, should not be copied.
    IteratorEntry (IteratorEntry&&) = default;

    IteratorEntry (leveldb::Iterator* lit, bool valid = false): _lit (lit), _valid (valid) {}
    ~IteratorEntry() {delete _lit;}

    /** Zero-copy view of the current key bytes. Should *not* be used after the Iterator is changed or destroyed.
     * @param ref If true then the *copies* of the returned gstring will keep pointing to LevelDB memory which is valid only until the iterator changes. */
    const gstring keyView (bool ref = false) const {
      if (!_valid) return gstring();
      const leveldb::Slice& key = _lit->key();
      return gstring (0, (void*) key.data(), false, key.size(), ref);} // Zero copy.
    /** Zero-copy view of the current value bytes. Should *not* be used after the Iterator is changed or destroyed.
     * @param ref If true then the *copies* of the returned gstring will keep pointing to LevelDB memory which is valid only until the iterator changes. */
    const gstring valueView (bool ref = false) const {
      if (!_valid) return gstring();
      const leveldb::Slice& val = _lit->value();
      return gstring (0, (void*) val.data(), false, val.size(), ref);} // Zero copy.
    /** Deserialize into `key`. */
    template <typename T> void getKey (T& key) const {ldbDeserialize (keyView (true), key);}
    /** Deserialize the key into a temporary and return it. */
    template <typename T> T getKey() const {T key; getKey (key); return key;}
    /** Deserialize into `value`. */
    template <typename T> void getValue (T& value) const {ldbDeserialize (valueView (true), value);}
    /** Deserialize the value into a temporary and return it. */
    template <typename T> T getValue() const {T value; getValue (value); return value;}
  };
  struct NoSeekFlag {}; ///< Tells the `Iterator` constructor not to seek to the beginning of the database.
  /** Wraps Leveldb iterator.
   * Note: "In fact the iterator is a light-weight snapshot. It will see exactly the version of the DB that existed when the iterator was created
   * (i.e., any insert/delete done after the iterator is created, regardless of which thread does them) will be invisible to the iterator"
   * (https://groups.google.com/d/msg/leveldb/nX8S5KKiSn4/PI92Yf1Hf6UJ). */
  struct Iterator: public boost::iterator_facade<Iterator, IteratorEntry, boost::bidirectional_traversal_tag> {
    std::shared_ptr<IteratorEntry> _entry; ///< The Iterator might be copied around, therefore we keep the real iterator and the state in the shared_ptr.

    Iterator (const Iterator&) = default;
    Iterator (Iterator&&) = default;
    Iterator& operator= (const Iterator&) = default;
    Iterator& operator= (Iterator&&) = default;
    /** Iterate from the beginning or the end of the database.
     * @param position can be MDB_FIRST or MDB_LAST */
    Iterator (Ldb* ldb, leveldb::ReadOptions options = leveldb::ReadOptions()):
      _entry (std::make_shared<IteratorEntry> (ldb->_db->NewIterator (options))) {
      IteratorEntry* entry = _entry.get();
      entry->_lit->SeekToFirst();
      entry->_valid = entry->_lit->Valid();
    }

    Iterator (Ldb* ldb, NoSeekFlag, leveldb::ReadOptions options = leveldb::ReadOptions()):
      _entry (std::make_shared<IteratorEntry> (ldb->_db->NewIterator (options))) {}
    /** True if the iterator isn't pointing anywhere. */
    bool end() const {return !_entry->_valid;}

    bool equal (const Iterator& other) const {
      bool weAreValid = _entry->_valid, theyAreValid = other._entry->_valid;
      if (!weAreValid) return !theyAreValid;
      if (!theyAreValid) return false;
      auto&& ourKey = _entry->_lit->key(), theirKey = other._entry->_lit->key();
      if (ourKey.size() != theirKey.size()) return false;
      return memcmp (ourKey.data(), theirKey.data(), ourKey.size()) == 0;
    }
    IteratorEntry& dereference() const {
      // NB: Boost iterator_facade expects the `dereference` to be a `const` method.
      //     I guess Iterator is not modified, so the `dereference` is `const`, even though the Entry can be modified.
      return *_entry;
    }
    virtual void increment() {
      IteratorEntry* entry = _entry.get();
      if (entry->_valid) entry->_lit->Next();
      else entry->_lit->SeekToFirst();
      entry->_valid = entry->_lit->Valid();
    }
    virtual void decrement() {
      IteratorEntry* entry = _entry.get();
      if (entry->_valid) entry->_lit->Prev();
      else entry->_lit->SeekToLast();
      entry->_valid = entry->_lit->Valid();
    }

    Iterator& seek (const gstring& key) {
      IteratorEntry* entry = _entry.get();
      entry->_lit->Seek (leveldb::Slice (key.data(), key.size()));
      entry->_valid = entry->_lit->Valid();
      return *this;
    }
  };
  Iterator begin() {return Iterator (this);}
  Iterator end() {return Iterator (this, NoSeekFlag());}

  /** Range from `from` (inclusive) to `till` (exclusive). */
  template <typename K>
  boost::iterator_range<Iterator> range (const K& from, const K& till, leveldb::ReadOptions options = leveldb::ReadOptions()) {
    char kbuf[64]; // Allow up to 64 bytes to be serialized without heap allocations.
    gstring kbytes (sizeof (kbuf), kbuf, false, 0);
    ldbSerialize (kbytes, from);
    Iterator fit (this, NoSeekFlag(), options); fit.seek (kbytes);

    ldbSerialize (kbytes.clear(), till);
    Iterator tit (this, NoSeekFlag(), options); tit.seek (kbytes);

    return boost::iterator_range<Iterator> (fit, tit);
  }

  struct StartsWithIterator: public Iterator {
    gstring _starts;
    StartsWithIterator (const StartsWithIterator&) = default;
    StartsWithIterator (StartsWithIterator&&) = default;
    StartsWithIterator& operator= (const StartsWithIterator&) = default;
    StartsWithIterator& operator= (StartsWithIterator&&) = default;

    StartsWithIterator (Ldb* ldb, const char* data, uint32_t length, leveldb::ReadOptions options = leveldb::ReadOptions()):
      Iterator (ldb, NoSeekFlag(), options), _starts (data, length) {
      IteratorEntry* entry = _entry.get();
      entry->_lit->Seek (leveldb::Slice (data, length));
      entry->_valid = checkValidity();
    }
    /** End iterator, pointing nowhere. */
    StartsWithIterator (Ldb* ldb, const char* data, uint32_t length, NoSeekFlag, leveldb::ReadOptions options = leveldb::ReadOptions()):
      Iterator (ldb, NoSeekFlag(), options), _starts (data, length) {}

    bool checkValidity() const {
      IteratorEntry* entry = _entry.get();
      return entry->_lit->Valid() && entry->_lit->key().starts_with (leveldb::Slice (_starts.data(), _starts.length()));
    }
    virtual void increment() override {
      IteratorEntry* entry = _entry.get();
      if (entry->_valid) entry->_lit->Next();
      else entry->_lit->Seek (leveldb::Slice (_starts.data(), _starts.length()));
      entry->_valid = checkValidity();
    }
    virtual void decrement() override {
      IteratorEntry* entry = _entry.get();
      if (entry->_valid) entry->_lit->Prev(); else seekLast();
      entry->_valid = checkValidity();
    }
    void seekLast() {
      leveldb::Iterator* lit = _entry->_lit;
      // Go somewhere *below* the `_starts` prefix.
      char after[_starts.length()]; if (sizeof (after)) {
        memcpy (after, _starts.data(), sizeof (after));
        uint32_t pos = sizeof (after); while (--pos >= 0) if (after[pos] < CHAR_MAX) {++after[pos]; break;}
        if (pos >= 0) {lit->Seek (leveldb::Slice (after, sizeof (after))); if (!lit->Valid()) lit->SeekToLast();} else lit->SeekToLast();
      } else lit->SeekToLast();
      // Seek back until we are in the `_starts` prefix.
      for (leveldb::Slice prefix (_starts.data(), _starts.length()); lit->Valid(); lit->Prev()) {
        leveldb::Slice key (lit->key());
        if (key.starts_with (prefix)) break; // We're "back" in the `_starts` prefix.
        if (key.compare (prefix) < 0) break; // Gone too far (no prefix entries in the db).
      }
    }
  };

  /** Range over entries starting with `key`. */
  template <typename K>
  boost::iterator_range<StartsWithIterator> startsWith (const K& key) {
    char kbuf[64]; // Allow up to 64 bytes to be serialized without heap allocations.
    gstring kbytes (sizeof (kbuf), kbuf, false, 0);
    ldbSerialize (kbytes, key);
    return boost::iterator_range<StartsWithIterator> (
      StartsWithIterator (this, kbytes.data(), kbytes.length()),
      StartsWithIterator (this, kbytes.data(), kbytes.length(), NoSeekFlag()));
  }

  struct Trigger {
    virtual gstring triggerName() const {return C2GSTRING ("defaultTriggerName");};
    virtual void put (Ldb& ldb, void* key, gstring& kbytes, void* value, gstring& vbytes, leveldb::WriteBatch& batch) = 0;
    virtual void del (Ldb& ldb, void* key, gstring& kbytes, leveldb::WriteBatch& batch) = 0;
  };
  std::map<gstring, std::shared_ptr<Trigger>> _triggers;

  /** Register the trigger (by its `triggerName`). */
  void putTrigger (std::shared_ptr<Trigger> trigger) {
    _triggers[trigger->triggerName()] = trigger;
  }

 public:

  Ldb() {}

  /** Opens Leveldb database. */
  Ldb (const char* path, leveldb::Options* options = nullptr, mode_t mode = 0770) {
    int rc = ::mkdir (path, mode);
    if (rc && errno != EEXIST) GNTHROW (LdbEx, std::string ("Can't create ") + path + ": " + ::strerror (errno));
    leveldb::DB* db;
    leveldb::Status status;
    if (options) {
      status = leveldb::DB::Open (*options, path, &db);
    } else {
      leveldb::Options localOptions;
      localOptions.create_if_missing = true;
      _filter.reset (leveldb::NewBloomFilterPolicy (8));
      localOptions.filter_policy = _filter.get();
      status = leveldb::DB::Open (localOptions, path, &db);
    }
    if (!status.ok()) GNTHROW (LdbEx, std::string ("Ldb: Can't open ") + path + ": " + status.ToString());
    _db.reset (db);
  }

  /** Wraps an existing Leveldb handler. */
  Ldb (std::shared_ptr<leveldb::DB> db): _db (db) {}

  template <typename K, typename V> void put (const K& key, const V& value, leveldb::WriteBatch& batch) {
    char kbuf[64]; // Allow up to 64 bytes to be serialized without heap allocations.
    gstring kbytes (sizeof (kbuf), kbuf, false, 0);
    ldbSerialize (kbytes, key);

    char vbuf[64]; // Allow up to 64 bytes to be serialized without heap allocations.
    gstring vbytes (sizeof (vbuf), vbuf, false, 0);
    ldbSerialize (vbytes, value);

    for (auto& trigger: _triggers) trigger.second->put (*this, (void*) &key, kbytes, (void*) &value, vbytes, batch);

    batch.Put (leveldb::Slice (kbytes.data(), kbytes.size()), leveldb::Slice (vbytes.data(), vbytes.size()));
  }
  template <typename K, typename V> void put (const K& key, const V& value) {
    leveldb::WriteBatch batch;
    put (key, value, batch);
    leveldb::Status status (_db->Write (leveldb::WriteOptions(), &batch));
    if (!status.ok()) GNTHROW (LdbEx, "Ldb: add: " + status.ToString());
  }

  /** Returns `true` if the key exists. Throws on error. */
  template <typename K> bool have (const K& key, leveldb::ReadOptions options = leveldb::ReadOptions()) {
    char kbuf[64]; // Allow up to 64 bytes to be serialized without heap allocations.
    gstring kbytes (sizeof (kbuf), kbuf, false, 0);
    ldbSerialize (kbytes, key);
    leveldb::Slice keySlice (kbytes.data(), kbytes.size());

    // NB: "BloomFilter only helps for Get() calls" - https://groups.google.com/d/msg/leveldb/oEiDztqHiHc/LMY3tHxzRGAJ
    //     "Apart from the lack of Bloom filter functionality, creating an iterator is really quite slow" - qpu2jSA8mCEJ
    std::string str;
    leveldb::Status status (_db->Get (options, keySlice, &str));
    if (status.ok()) return true;
    else if (status.IsNotFound()) return false;
    else GTHROW ("Ldb.have: " + status.ToString());
  }

  /** Returns `true` and modifies `value` if `key` is found. */
  template <typename K, typename V> bool get (const K& key, V& value, leveldb::ReadOptions options = leveldb::ReadOptions()) {
    char kbuf[64]; // Allow up to 64 bytes to be serialized without heap allocations.
    gstring kbytes (sizeof (kbuf), kbuf, false, 0);
    ldbSerialize (kbytes, key);
    leveldb::Slice keySlice (kbytes.data(), kbytes.size());

    // NB: "BloomFilter only helps for Get() calls" - https://groups.google.com/d/msg/leveldb/oEiDztqHiHc/LMY3tHxzRGAJ
    //     "Apart from the lack of Bloom filter functionality, creating an iterator is really quite slow" - qpu2jSA8mCEJ
    std::string str;
    leveldb::Status status (_db->Get (options, keySlice, &str));
    if (status.ok()) {
      ldbDeserialize (gstring (0, (void*) str.data(), false, str.size()), value);
      return true;
    } else if (status.IsNotFound()) return false;
    else GTHROW ("Ldb.get: " + status.ToString());
  }

  template <typename K> void del (const K& key, leveldb::WriteBatch& batch) {
    char kbuf[64]; // Allow up to 64 bytes to be serialized without heap allocations.
    gstring kbytes (sizeof (kbuf), kbuf, false, 0);
    ldbSerialize (kbytes, key);
    if (kbytes.empty()) GNTHROW (LdbEx, "del: key is empty");

    for (auto& trigger: _triggers) trigger.second->del (*this, (void*) &key, kbytes, batch);

    batch.Delete (leveldb::Slice (kbytes.data(), kbytes.size()));
  }
  template <typename K> void del (const K& key) {
    leveldb::WriteBatch batch;
    del (key, batch);
    leveldb::Status status (_db->Write (leveldb::WriteOptions(), &batch));
    if (!status.ok()) GNTHROW (LdbEx, "Ldb: del: " + status.ToString());
  }

  /** Writes the batch. Throws LdbEx if not successfull. */
  void write (leveldb::WriteBatch& batch, leveldb::WriteOptions options = leveldb::WriteOptions()) {
    leveldb::Status status (_db->Write (options, &batch));
    if (!status.ok()) GNTHROW (LdbEx, status.ToString());
  }

  virtual ~Ldb() {
    _triggers.clear(); // Destroy triggers before closing the database.
  }
};

} // namespace glim

#endif // _GLIM_LDB_HPP_INCLUDED
