#ifndef _GLIM_MDB_HPP_INCLUDED
#define _GLIM_MDB_HPP_INCLUDED

/**
 * A C++ wrapper around MDB (http://www.symas.com/mdb/).
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

#include <mdb.h>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/noncopyable.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/iterator_range.hpp>

#include <arpa/inet.h> // htonl, ntohl

#include "gstring.hpp"

namespace glim {

struct MdbEx: public std::runtime_error {MdbEx (std::string message): std::runtime_error (message) {}};

template <typename T> inline void mdbSerialize (gstring& bytes, const T& data) {
  gstring_stream stream (bytes);
  boost::archive::binary_oarchive oa (stream, boost::archive::no_header);
  oa << data;
}
template <typename V> inline void mdbDeserialize (const gstring& bytes, V& data) {
  gstring_stream stream (const_cast<gstring&> (bytes));
  boost::archive::binary_iarchive ia (stream, boost::archive::no_header);
  ia >> data;
}

/** uint32_t keys are stored big-endian (network byte order) in order to be compatible with lexicographic ordering. */
template <> inline void mdbSerialize<uint32_t> (gstring& bytes, const uint32_t& ui) {
  uint32_t nui = htonl (ui); bytes.append ((const char*) &nui, sizeof (uint32_t));}
/** Deserialize uint32_t from big-endian (network byte order). */
template <> inline void mdbDeserialize<uint32_t> (const gstring& bytes, uint32_t& ui) {
  if (bytes.size() != sizeof (uint32_t)) throw MdbEx ("Not uint32_t, wrong number of bytes");
  uint32_t nui = * (uint32_t*) bytes.data(); ui = ntohl (nui);}

/** If the data is `gstring` then use the data's buffer directly, no copy. */
template <> inline void mdbSerialize<gstring> (gstring& bytes, const gstring& data) {
  bytes = gstring (0, (void*) data.data(), false, data.length());}
/** Deserializing into `gstring` copies the bytes into it, reusing its buffer. */
template <> inline void mdbDeserialize<gstring> (const gstring& bytes, gstring& data) {
  data.clear() << bytes;}

/**
 * Header-only C++ wrapper around OpenLDAP-MDB.\n
 * Uses Boost Serialization to pack keys and values (glim::gstring can be used for raw bytes).\n
 * Allows semi-automatic indexing with triggers.\n
 * Known issues: http://www.openldap.org/its/index.cgi?findid=7448
 */
struct Mdb {
  std::shared_ptr<MDB_env> _env;
  MDB_dbi _dbi = 0;

  typedef std::unique_ptr<MDB_txn, void(*)(MDB_txn*)> Transaction;

  /** Holds the current key and value of the Iterator. */
  struct IteratorEntry {
    MDB_val _key = {0, 0}, _val = {0, 0};
    /** Zero-copy view of the current key bytes. Should *not* be used after the Iterator is changed or destroyed. */
    const gstring keyView() const {return gstring (0, _key.mv_data, false, _key.mv_size, true);} // Zero copy.
    /** Zero-copy view of the current value bytes. Should *not* be used after the Iterator is changed or destroyed. */
    const gstring valueView() const {return gstring (0, _val.mv_data, false, _val.mv_size, true);} // Zero copy.
    /** Deserialize into `key`. */
    template <typename T> void getKey (T& key) const {mdbDeserialize (keyView(), key);}
    /** Deserialize the key into a temporary and return it. */
    template <typename T> T getKey() const {T key; getKey (key); return key;}
    /** Deserialize into `value`. */
    template <typename T> void getValue (T& value) const {mdbDeserialize (valueView(), value);}
    /** Deserialize the value into a temporary and return it. */
    template <typename T> T getValue() const {T value; getValue (value); return value;}
  };
  /** Holds the Iterator's unique transaction and cursor, allowing the Iterator to be copied. */
  struct IteratorImpl: boost::noncopyable {
    Mdb* _mdb;
    Transaction _txn;
    MDB_cursor* _cur;
    IteratorImpl (Mdb* mdb, Transaction&& txn, MDB_cursor* cur): _mdb (mdb), _txn (std::move (txn)), _cur (cur) {}
    ~IteratorImpl() {
      if (_cur) {::mdb_cursor_close (_cur); _cur = nullptr;}
      if (_mdb && _txn) {_mdb->commitTransaction (_txn); _mdb = nullptr;}
    }
  };
  /** Wraps MDB cursor and cursor's transaction. */
  struct Iterator: public boost::iterator_facade<Iterator, IteratorEntry, boost::bidirectional_traversal_tag> {
    std::shared_ptr<IteratorImpl> _impl; // Iterator might be copied around, thus we keep the unique things in IteratorImpl.
    IteratorEntry _entry;
    bool _stayInKey = false;

    Iterator (const Iterator&) = default;
    Iterator (Iterator&&) = default;
    /** Iterate from the beginning or the end of the database.
     * @param position can be MDB_FIRST or MDB_LAST */
    Iterator (Mdb* mdb, int position = 0): _stayInKey (false) {
      Transaction txn (mdb->beginTransaction());
      MDB_cursor* cur = nullptr; int rc = ::mdb_cursor_open (txn.get(), mdb->_dbi, &cur);
      if (rc) throw MdbEx ("mdb_cursor_open");
      _impl = std::make_shared<IteratorImpl> (mdb, std::move (txn), cur);
      if (position == ::MDB_FIRST || position == ::MDB_LAST) {
        rc = ::mdb_cursor_get (cur, &_entry._key, &_entry._val, (MDB_cursor_op) position);
        if (rc) throw MdbEx ("mdb_cursor_get");
      }
    }
    /** Iterate over `key` values.
     * @param stayInKey if `false` then iterator can go farther than the `key`. */
    Iterator (Mdb* mdb, const gstring& key, bool stayInKey = true): _stayInKey (stayInKey) {
      Transaction txn (mdb->beginTransaction());
      MDB_cursor* cur = nullptr; int rc = ::mdb_cursor_open (txn.get(), mdb->_dbi, &cur);
      if (rc) throw MdbEx ("mdb_cursor_open");
      _impl = std::make_shared<IteratorImpl> (mdb, std::move (txn), cur);
      _entry._key = {key.size(), (void*) key.data()};
      rc = ::mdb_cursor_get (cur, &_entry._key, &_entry._val, ::MDB_SET_KEY);
      if (rc == MDB_NOTFOUND) {_entry._key = {0, 0}; _entry._val = {0, 0};}
      else if (rc) throw MdbEx ("mdb_cursor_get");
    }

    struct EndIteratorFlag {};
    /** The "end" iterator does not open an MDB transaction (this is essential for having a pair of iterators without a deadlock). */
    Iterator (EndIteratorFlag): _stayInKey (false) {}
    /** True if the iterator isn't pointing anywhere. */
    bool end() const {return _entry._key.mv_size == 0;}

    bool equal (const Iterator& other) const {
      IteratorImpl* impl = _impl.get();
      if (mdb_cmp (impl->_txn.get(), impl->_mdb->_dbi, &_entry._key, &other._entry._key)) return false;
      if (mdb_dcmp (impl->_txn.get(), impl->_mdb->_dbi, &_entry._val, &other._entry._val)) return false;
      return true;
    }
    IteratorEntry& dereference() const {
      // NB: Boost iterator_facade expects the `dereference` to be a `const` method.
      //     I guess Iterator is not modified, so the `dereference` is `const`, even though the Entry can be modified.
      return const_cast<IteratorEntry&> (_entry);}
    void increment() {
      int rc = ::mdb_cursor_get (_impl->_cur, &_entry._key, &_entry._val, _stayInKey ? ::MDB_NEXT_DUP : ::MDB_NEXT);
      if (rc) {_entry._key = {0,0}; _entry._val = {0,0};}
    }
    void decrement() {
      int rc = ::mdb_cursor_get (_impl->_cur, &_entry._key, &_entry._val, _stayInKey ? ::MDB_PREV_DUP : ::MDB_PREV);
      if (rc) {_entry._key = {0,0}; _entry._val = {0,0};}
    }
  };
  Iterator begin() {return Iterator (this, ::MDB_FIRST);}
  const Iterator end() {return Iterator (Iterator::EndIteratorFlag());}
  /** Position the cursor at the first `key` record.\n
   * The iterator increment will use `MDB_NEXT_DUP`, staying withing the `key`.\n
   * See also the `all` method. */
  template <typename K> Iterator values (const K& key) {
    char kbuf[64]; // Allow up to 64 bytes to be serialized without heap allocations.
    gstring kbytes (sizeof (kbuf), kbuf, false, 0);
    mdbSerialize (kbytes, key);
    return Iterator (this, kbytes);
  }
  /** Range over the `key` values.\n
   * See also the `all` method. */
  template <typename K> boost::iterator_range<Iterator> valuesRange (const K& key) {return boost::iterator_range<Iterator> (values (key), end());}

  struct Trigger {
    virtual gstring getTriggerName() const {return C2GSTRING ("defaultTriggerName");};
    virtual void add (Mdb& mdb, void* key, gstring& kbytes, void* value, gstring& vbytes, Transaction& txn) = 0;
    virtual void erase (Mdb& mdb, void* key, gstring& kbytes, Transaction& txn) = 0;
    virtual void eraseKV (Mdb& mdb, void* key, gstring& kbytes, void* value, gstring& vbytes, Transaction& txn) = 0;
  };
  std::map<gstring, std::shared_ptr<Trigger>> _triggers;

  void setTrigger (std::shared_ptr<Trigger> trigger) {
    _triggers[trigger->getTriggerName()] = trigger;
  }

  /** `flags` can be `MDB_RDONLY` */
  Transaction beginTransaction (unsigned flags = 0) {
    MDB_txn* txn = 0; int rc = ::mdb_txn_begin (_env.get(), nullptr, flags, &txn);
    if (rc) throw MdbEx (std::string ("mdb_txn_begin: ") + ::strerror (rc));
    return Transaction (txn, ::mdb_txn_abort);
  }
  void commitTransaction (Transaction& txn) {
    int rc = ::mdb_txn_commit (txn.get());
    txn.release(); // Must prevent `mdb_txn_abort` from happening (even if rc != 0).
    if (rc) throw MdbEx (std::string ("mdb_txn_commit: ") + ::strerror (rc));
  }

  virtual unsigned envFlags (uint8_t sync) {
    unsigned flags = MDB_NOSUBDIR;
    if (sync < 1) flags |= MDB_NOSYNC; else if (sync < 2) flags |= MDB_NOMETASYNC;
    return flags;
  }
  /** Used before `mdb_env_open`. By default sets the number of database to 32. */
  virtual void envConf (MDB_env* env) {
    int rc = ::mdb_env_set_maxdbs (env, 32);
    if (rc) throw MdbEx (std::string ("envConf: ") + ::strerror (rc));
  }
  virtual void dbFlags (unsigned& flags) {}

 protected:
  void open (const char* dbName, bool dup) {
    auto txn = beginTransaction();
    unsigned flags = MDB_CREATE;
    if (dup) flags |= MDB_DUPSORT;
    dbFlags (flags);
    int rc = ::mdb_open (txn.get(), dbName, flags, &_dbi);
    if (rc) throw MdbEx (std::string ("mdb_open (") + dbName + "): " + ::strerror (rc));
    commitTransaction (txn);
  }
 public:

  /** Opens MDB environment and MDB database. */
  Mdb (const char* path, size_t maxSizeMb = 1024, const char* dbName = "main", uint8_t sync = 0, bool dup = true, mode_t mode = 0660) {
    MDB_env* env = 0; int rc = ::mdb_env_create (&env);
    if (rc) throw MdbEx (std::string ("mdb_env_create: ") + ::strerror (rc));
    _env.reset (env, ::mdb_env_close);
    rc = ::mdb_env_set_mapsize (env, maxSizeMb * 1024 * 1024);
    if (rc) throw MdbEx (std::string ("mdb_env_set_mapsize: ") + ::strerror (rc));
    envConf (env);
    rc = ::mdb_env_open (env, path, envFlags (sync), mode);
    _dbi = 0; open (dbName, dup);
  }

  /** Opens MDB database in the provided environment. */
  Mdb (std::shared_ptr<MDB_env> env, const char* dbName, bool dup = true): _env (env), _dbi (0) {
    open (dbName, dup);
  }

  template <typename K, typename V> void add (const K& key, const V& value, Transaction& txn) {
    char kbuf[64]; // Allow up to 64 bytes to be serialized without heap allocations.
    gstring kbytes (sizeof (kbuf), kbuf, false, 0);
    mdbSerialize (kbytes, key);
    MDB_val mkey = {kbytes.size(), (void*) kbytes.data()};

    char vbuf[64]; // Allow up to 64 bytes to be serialized without heap allocations.
    gstring vbytes (sizeof (vbuf), vbuf, false, 0);
    mdbSerialize (vbytes, value);
    MDB_val mvalue = {vbytes.size(), (void*) vbytes.data()};

    for (auto& trigger: _triggers) trigger.second->add (*this, (void*) &key, kbytes, (void*) &value, vbytes, txn);

    int rc = ::mdb_put (txn.get(), _dbi, &mkey, &mvalue, 0);
    if (rc) throw MdbEx (std::string ("mdb_put: ") + ::strerror (rc));
  }
  template <typename K, typename V> void add (const K& key, const V& value) {
    Transaction txn (beginTransaction());
    add (key, value, txn);
    commitTransaction (txn);
  }

  template <typename K, typename V> bool first (const K& key, V& value, Transaction& txn) {
    char kbuf[64]; // Allow up to 64 bytes to be serialized without heap allocations.
    gstring kbytes (sizeof (kbuf), kbuf, false, 0);
    mdbSerialize (kbytes, key);
    MDB_val mkey = {kbytes.size(), (void*) kbytes.data()};
    MDB_val mvalue;
    int rc = ::mdb_get (txn.get(), _dbi, &mkey, &mvalue);
    if (rc == MDB_NOTFOUND) return false;
    if (rc) throw MdbEx (std::string ("mdb_get: ") + ::strerror (rc));
    gstring vstr (0, mvalue.mv_data, false, mvalue.mv_size);
    mdbDeserialize (vstr, value);
    return true;
  }
  template <typename K, typename V> bool first (const K& key, V& value) {
    Transaction txn (beginTransaction (MDB_RDONLY));
    bool rb = first (key, value, txn);
    commitTransaction (txn);
    return rb;
  }

  /** Iterate over `key` values until `visitor` returns `false`. Return the number of values visited. */
  template <typename K, typename V> int32_t all (const K& key, std::function<bool(const V&)> visitor, Transaction& txn) {
    char kbuf[64]; // Allow up to 64 bytes to be serialized without heap allocations.
    gstring kbytes (sizeof (kbuf), kbuf, false, 0);
    mdbSerialize (kbytes, key);
    MDB_val mkey = {kbytes.size(), (void*) kbytes.data()};

    MDB_cursor* cur = 0; int rc = ::mdb_cursor_open (txn.get(), _dbi, &cur);
    if (rc) throw MdbEx (std::string ("mdb_cursor_open: ") + ::strerror (rc));
    std::unique_ptr<MDB_cursor, void(*)(MDB_cursor*)> curHolder (cur, ::mdb_cursor_close);
    MDB_val mval = {0, 0};
    rc = ::mdb_cursor_get (cur, &mkey, &mval, ::MDB_SET_KEY); if (rc == MDB_NOTFOUND) return 0;
    if (rc) throw MdbEx (std::string ("mdb_cursor_get: ") + ::strerror (rc));

    V value;
    gstring vstr (0, mval.mv_data, false, mval.mv_size);
    mdbDeserialize (vstr, value);
    bool goOn = visitor (value);
    int32_t count = 1;

    while (goOn) {
      rc = ::mdb_cursor_get (cur, &mkey, &mval, ::MDB_NEXT_DUP); if (rc == MDB_NOTFOUND) return count;
      if (rc) throw MdbEx (std::string ("mdb_cursor_get: ") + ::strerror (rc));
      gstring vstr (0, mval.mv_data, false, mval.mv_size);
      mdbDeserialize (vstr, value);
      goOn = visitor (value);
      ++count;
    }
    return count;
  }
  /** Iterate over `key` values until `visitor` returns `false`. Return the number of values visited. */
  template <typename K, typename V> int32_t all (const K& key, std::function<bool(const V&)> visitor) {
    Transaction txn (beginTransaction (MDB_RDONLY));
    int32_t count = all (key, visitor, txn);
    commitTransaction (txn);
    return count;
  }

  template <typename K, typename V> bool eraseKV (const K& key, const V& value, Transaction& txn) {
    char kbuf[64]; // Allow up to 64 bytes to be serialized without heap allocations.
    gstring kbytes (sizeof (kbuf), kbuf, false, 0);
    mdbSerialize (kbytes, key);
    if (kbytes.empty()) throw MdbEx ("eraseKV: key is empty");
    MDB_val mkey = {kbytes.size(), (void*) kbytes.data()};

    char vbuf[64]; // Allow up to 64 bytes to be serialized without heap allocations.
    gstring vbytes (sizeof (vbuf), vbuf, false, 0);
    mdbSerialize (vbytes, value);
    MDB_val mvalue = {vbytes.size(), (void*) vbytes.data()};

    for (auto& trigger: _triggers) trigger.second->eraseKV (*this, (void*) &key, kbytes, (void*) &value, vbytes, txn);

    int rc = ::mdb_del (txn.get(), _dbi, &mkey, &mvalue);
    if (rc == MDB_NOTFOUND) return false;
    if (rc) throw MdbEx (std::string ("mdb_del: ") + ::strerror (rc));
    return true;
  }
  template <typename K, typename V> bool eraseKV (const K& key, const V& value) {
    Transaction txn (beginTransaction());
    bool rb = eraseKV (key, value, txn);
    commitTransaction (txn);
    return rb;
  }

  /** Erase all values of the `key`. */
  template <typename K> bool erase (const K& key, Transaction& txn) {
    char kbuf[64]; // Allow up to 64 bytes to be serialized without heap allocations.
    gstring kbytes (sizeof (kbuf), kbuf, false, 0);
    mdbSerialize (kbytes, key);
    if (kbytes.empty()) throw MdbEx ("erase: key is empty");
    MDB_val mkey = {kbytes.size(), (void*) kbytes.data()};

    for (auto& trigger: _triggers) trigger.second->erase (*this, (void*) &key, kbytes, txn);

    int rc = ::mdb_del (txn.get(), _dbi, &mkey, nullptr);
    if (rc == MDB_NOTFOUND) return false;
    if (rc) throw MdbEx (std::string ("mdb_del: ") + ::strerror (rc));
    return true;
  }
  /** Erase all values of the `key`. */
  template <typename K> bool erase (const K& key) {
    Transaction txn (beginTransaction());
    bool rb = erase (key, txn);
    commitTransaction (txn);
    return rb;
  }

  static void test (Mdb& mdb) {
    mdb.add (std::string ("foo"), std::string ("bar"));
    // NB: "MDB_DUPSORT doesn't allow duplicate duplicates" (Howard Chu)
    mdb.add (std::string ("foo"), std::string ("bar"));
    mdb.add ((uint32_t) 123, 1);
    mdb.add ((uint32_t) 123, 2);
    mdb.add (C2GSTRING ("foo"), 3);
    mdb.add (C2GSTRING ("foo"), 4);
    mdb.add (C2GSTRING ("gsk"), C2GSTRING ("gsv"));
    string ts; int ti; gstring tgs;
    auto fail = [](string msg) {throw std::runtime_error ("assertion failed: " + msg);};
    if (!mdb.first (std::string ("foo"), ts) || ts != "bar") fail ("!foo=bar");
    if (!mdb.first ((uint32_t) 123, ti) || ti < 1 || ti > 2) fail ("!123");
    if (!mdb.first (C2GSTRING ("foo"), ti) || ti < 3 || ti > 4) fail ("!foo=3,4");
    if (!mdb.first (C2GSTRING ("gsk"), tgs) || tgs != "gsv") fail ("!gsk=gsv");

    // Test range-based for.
    int count = 0; bool haveGskGsv = false;
    for (auto&& entry: mdb) {
      if (!entry._key.mv_size || !entry._val.mv_size) fail ("!entry");
      if (entry.keyView() == "gsk") {
        if (entry.getKey<gstring>() != "gsk") fail ("getKey(gsk)!=gsk");
        if (entry.getValue<gstring>() != "gsv") fail ("getValue(gsk)!=gsv");
        haveGskGsv = true;
      }
      ++count;}
    if (count != 6) fail ("count!=6"); // foo=bar, 123=1, 123=2, foo=3, foo=4, gsk=gsv
    if (!haveGskGsv) fail ("!haveGskGsv");

    // Test `values`.
    count = 0; int sum = 0;
    for (auto&& entry: mdb.valuesRange ((uint32_t) 123)) {
      if (entry.getKey<uint32_t>() != (uint32_t) 123) fail("values(123).key!=123");
      ++count; sum += entry.getValue<int>();
    }
    if (count != 2) fail ("count(123)!=2");
    if (sum != 3) fail ("sum(123)!=3");

    if (!mdb.eraseKV ((uint32_t) 123, 1)) fail ("!eraseKV(123,1)");
    if (!mdb.first ((uint32_t) 123, ti) || ti != 2) fail ("!123=2");
    if (!mdb.eraseKV ((uint32_t) 123, 2)) fail ("!eraseKV(123,2)");
    if (mdb.first ((uint32_t) 123, ti)) fail ("123");

    if (!mdb.erase (C2GSTRING ("foo"))) fail ("!erase(g(foo))");
    if (mdb.first (C2GSTRING ("foo"), ti)) fail ("foo");
    if (!mdb.erase (std::string ("foo"))) fail ("!erase(str(foo))");

    { // We've erased "123" and "foo", the only key left is "gsk" (gsk=gsv), let's test the iterator boundaries on this small dataset.
      auto&& it = mdb.begin();
      if (it->getKey<gstring>() != "gsk") fail ("first key !gsk " + it->keyView().str());
      if (!(++it).end()) fail ("++it != end");
      if ((--it)->getKey<gstring>() != "gsk") fail ("can't go back to gsk");
      if (!(--it).end()) fail ("--it != end");
      if ((++it)->getKey<gstring>() != "gsk") fail ("can't go forward to gsk");
    }

    struct SimpleIndexTrigger: public Trigger {
      const char* _name; Mdb _indexDb;
      SimpleIndexTrigger (Mdb& mdb, const char* name = "index"): _name (name), _indexDb (mdb._env, name) {}
      gstring getTriggerName() {return gstring (0, (void*) _name, false, strlen (_name), true);}
      void add (Mdb& mdb, void* key, gstring& kbytes, void* value, gstring& vbytes, Transaction& txn) {
        MDB_val mkey = {vbytes.size(), (void*) vbytes.data()};
        MDB_val mvalue = {kbytes.size(), (void*) kbytes.data()};
        int rc = ::mdb_put (txn.get(), _indexDb._dbi, &mkey, &mvalue, 0);
        if (rc) throw MdbEx (std::string ("index, mdb_put: ") + ::strerror (rc));
      }
      void erase (Mdb& mdb, void* ekey, gstring& kbytes, Transaction& txn) {
        // Get all the values and remove them from the index.
        MDB_cursor* cur = 0; int rc = ::mdb_cursor_open (txn.get(), mdb._dbi, &cur);
        if (rc) throw MdbEx (std::string ("index, erase, mdb_cursor_open: ") + ::strerror (rc));
        std::unique_ptr<MDB_cursor, void(*)(MDB_cursor*)> curHolder (cur, ::mdb_cursor_close);
        MDB_val mkey = {kbytes.size(), (void*) kbytes.data()}, val = {0, 0};
        rc = ::mdb_cursor_get (cur, &mkey, &val, ::MDB_SET_KEY); if (rc == MDB_NOTFOUND) return;
        if (rc) throw MdbEx (std::string ("index, erase, mdb_cursor_get: ") + ::strerror (rc));
        rc = ::mdb_del (txn.get(), _indexDb._dbi, &val, &mkey);
        if (rc && rc != MDB_NOTFOUND) throw MdbEx (std::string ("index, erase, mdb_del: ") + ::strerror (rc));
        for (;;) {
          rc = ::mdb_cursor_get (cur, &mkey, &val, ::MDB_NEXT_DUP); if (rc == MDB_NOTFOUND) return;
          if (rc) throw MdbEx (std::string ("index, erase, mdb_cursor_get: ") + ::strerror (rc));
          rc = ::mdb_del (txn.get(), _indexDb._dbi, &val, &mkey);
          if (rc && rc != MDB_NOTFOUND) throw MdbEx (std::string ("index, erase, mdb_del: ") + ::strerror (rc));
        }
      }
      void eraseKV (Mdb& mdb, void* key, gstring& kbytes, void* value, gstring& vbytes, Transaction& txn) {
        MDB_val mkey = {vbytes.size(), (void*) vbytes.data()};
        MDB_val mvalue = {kbytes.size(), (void*) kbytes.data()};
        int rc = ::mdb_del (txn.get(), _indexDb._dbi, &mkey, &mvalue);
        if (rc && rc != MDB_NOTFOUND) throw MdbEx (std::string ("index, mdb_del: ") + ::strerror (rc));
      }
    };
    auto indexTrigger = std::make_shared<SimpleIndexTrigger> (mdb); mdb.setTrigger (indexTrigger); auto& indexDb = indexTrigger->_indexDb;
    mdb.erase (C2GSTRING ("gsk")); // NB: "gsk" wasn't indexed here. `IndexTrigger.erase` should handle this gracefully.

    // Add indexed.
    mdb.add (C2GSTRING ("ik"), C2GSTRING ("iv1"));
    mdb.add (C2GSTRING ("ik"), string ("iv2"));
    mdb.add (C2GSTRING ("ik"), 3);
    // Check the index.
    gstring ik;
    if (!indexDb.first (C2GSTRING ("iv1"), ik) || ik != "ik") fail ("!iv1=ik");
    if (!indexDb.first (string ("iv2"), ik) || ik != "ik") fail ("!iv2=ik");
    if (!indexDb.first (3, ik) || ik != "ik") fail ("!iv3=ik");

    // Remove indexed.
    mdb.eraseKV (C2GSTRING ("ik"), string ("iv2"));
    // Check the index.
    if (!indexDb.first (C2GSTRING ("iv1"), ik) || ik != "ik") fail ("!iv1=ik");
    if (indexDb.first (string ("iv2"), ik)) fail ("iv2=ik");
    if (!indexDb.first (3, ik) || ik != "ik") fail ("!iv3=ik");

    // Remove indexed.
    mdb.erase (C2GSTRING ("ik"));
    // Check the index.
    if (indexDb.first (C2GSTRING ("iv1"), ik)) fail ("iv1");
    if (indexDb.first (3, ik)) fail ("iv3");
    // Check the data.
    if (mdb.first (C2GSTRING ("ik"), ik)) fail ("ik");
  }

  virtual ~Mdb() {
    _triggers.clear(); // Destroy triggers before closing the database.
    if (_dbi) {::mdb_close (_env.get(), _dbi); _dbi = 0;}
  }
};

} // namespace glim

#endif // _GLIM_MDB_HPP_INCLUDED
