#include "ldb.hpp"
using glim::Ldb;
using glim::gstring;
#include <iostream>
using std::cout; using std::flush; using std::endl;
#include <assert.h>
#include <boost/filesystem.hpp>

void test1 (Ldb& ldb) {
  ldb.put (std::string ("foo_"), std::string ("bar"));
  ldb.put ((uint32_t) 123, 1);
  ldb.put ((uint32_t) 123, 2);
  ldb.put (C2GSTRING ("foo"), 3);
  ldb.put (C2GSTRING ("foo"), 4);
  ldb.put (C2GSTRING ("gsk"), C2GSTRING ("gsv"));
  std::string ts; int ti; gstring tgs;
  auto fail = [](std::string msg) {throw std::runtime_error ("assertion failed: " + msg);};
  if (!ldb.get (std::string ("foo_"), ts) || ts != "bar") fail ("!foo_=bar");
  if (!ldb.get ((uint32_t) 123, ti) || ti != 2) fail ("!123=2");
  if (!ldb.get (C2GSTRING ("foo"), ti) || ti != 4) fail ("!foo=4");
  if (!ldb.get (C2GSTRING ("gsk"), tgs) || tgs != "gsv") fail ("!gsk=gsv");

  // Test range-based for.
  int count = 0; bool haveGskGsv = false;
  for (auto&& entry: ldb) {
    if (!entry._lit->Valid()) fail ("!entry");
    if (entry.keyView() == "gsk") {
      if (entry.getKey<gstring>() != "gsk") fail ("getKey(gsk)!=gsk");
      if (entry.getValue<gstring>() != "gsv") fail ("getValue(gsk)!=gsv");
      haveGskGsv = true;
    }
    ++count;}
  if (count != 4) fail ("count!=4"); // foo_=bar, 123=2, foo=4, gsk=gsv
  if (!haveGskGsv) fail ("!haveGskGsv");

  ldb.del ((uint32_t) 123); if (ldb.get ((uint32_t) 123, ti)) fail ("123");
  ldb.del (C2GSTRING ("foo")); if (ldb.get (C2GSTRING ("foo"), ti)) fail ("foo");
  ldb.del (std::string ("foo_"));

  { // We've erased "123" and "foo", the only key left is "gsk" (gsk=gsv), let's test the iterator boundaries on this small dataset.
    auto&& it = ldb.begin();
    if (it->getKey<gstring>() != "gsk") fail ("first key !gsk " + it->keyView().str());
    if (!(++it).end()) fail ("++it != end");
    if ((--it)->getKey<gstring>() != "gsk") fail ("can't go back to gsk");
    if (!(--it).end()) fail ("--it != end");
    if ((++it)->getKey<gstring>() != "gsk") fail ("can't go forward to gsk");
  }

// todo: index trigger example
//    struct SimpleIndexTrigger: public Trigger { // Uses key space partitioning (cf. http://stackoverflow.com/a/12503799/257568)
//      const char* _name; Ldb _indexDb;
//      SimpleIndexTrigger (Ldb& ldb, const char* name = "index"): _name (name), _indexDb (ldb._env, name) {}
//      gstring triggerName() {return gstring (0, (void*) _name, false, strlen (_name), true);}
//      void add (Ldb& ldb, void* key, gstring& kbytes, void* value, gstring& vbytes, Transaction& txn) {
//        MDB_val mkey = {vbytes.size(), (void*) vbytes.data()};
//        MDB_val mvalue = {kbytes.size(), (void*) kbytes.data()};
//        int rc = ::ldb_put (txn.get(), _indexDb._dbi, &mkey, &mvalue, 0);
//        if (rc) GNTHROW (LdbEx, std::string ("index, ldb_put: ") + ::strerror (rc));
//      }
//      void erase (Ldb& ldb, void* ekey, gstring& kbytes, Transaction& txn) {
//        // Get all the values and remove them from the index.
//        MDB_cursor* cur = 0; int rc = ::ldb_cursor_open (txn.get(), ldb._dbi, &cur);
//        if (rc) GNTHROW (LdbEx, std::string ("index, erase, ldb_cursor_open: ") + ::strerror (rc));
//        std::unique_ptr<MDB_cursor, void(*)(MDB_cursor*)> curHolder (cur, ::ldb_cursor_close);
//        MDB_val mkey = {kbytes.size(), (void*) kbytes.data()}, val = {0, 0};
//        rc = ::ldb_cursor_get (cur, &mkey, &val, ::MDB_SET_KEY); if (rc == MDB_NOTFOUND) return;
//        if (rc) GNTHROW (LdbEx, std::string ("index, erase, ldb_cursor_get: ") + ::strerror (rc));
//        rc = ::ldb_del (txn.get(), _indexDb._dbi, &val, &mkey);
//        if (rc && rc != MDB_NOTFOUND) GNTHROW (LdbEx, std::string ("index, erase, ldb_del: ") + ::strerror (rc));
//        for (;;) {
//          rc = ::ldb_cursor_get (cur, &mkey, &val, ::MDB_NEXT_DUP); if (rc == MDB_NOTFOUND) return;
//          if (rc) GNTHROW (LdbEx, std::string ("index, erase, ldb_cursor_get: ") + ::strerror (rc));
//          rc = ::ldb_del (txn.get(), _indexDb._dbi, &val, &mkey);
//          if (rc && rc != MDB_NOTFOUND) GNTHROW (LdbEx, std::string ("index, erase, ldb_del: ") + ::strerror (rc));
//        }
//      }
//      void eraseKV (Ldb& ldb, void* key, gstring& kbytes, void* value, gstring& vbytes, Transaction& txn) {
//        MDB_val mkey = {vbytes.size(), (void*) vbytes.data()};
//        MDB_val mvalue = {kbytes.size(), (void*) kbytes.data()};
//        int rc = ::ldb_del (txn.get(), _indexDb._dbi, &mkey, &mvalue);
//        if (rc && rc != MDB_NOTFOUND) GNTHROW (LdbEx, std::string ("index, ldb_del: ") + ::strerror (rc));
//      }
//    };
//    auto indexTrigger = std::make_shared<SimpleIndexTrigger> (ldb); ldb.setTrigger (indexTrigger); auto& indexDb = indexTrigger->_indexDb;
//    ldb.erase (C2GSTRING ("gsk")); // NB: "gsk" wasn't indexed here. `IndexTrigger.erase` should handle this gracefully.

  // Add indexed.
//    ldb.put (C2GSTRING ("ik"), C2GSTRING ("iv1"));
//    ldb.put (C2GSTRING ("ik"), string ("iv2"));
//    ldb.put (C2GSTRING ("ik"), 3);
  // Check the index.
//    gstring ik;
//    if (!indexDb.first (C2GSTRING ("iv1"), ik) || ik != "ik") fail ("!iv1=ik");
//    if (!indexDb.first (string ("iv2"), ik) || ik != "ik") fail ("!iv2=ik");
//    if (!indexDb.first (3, ik) || ik != "ik") fail ("!iv3=ik");

  // Remove indexed.
//    ldb.eraseKV (C2GSTRING ("ik"), string ("iv2"));
  // Check the index.
//    if (!indexDb.first (C2GSTRING ("iv1"), ik) || ik != "ik") fail ("!iv1=ik");
//    if (indexDb.first (string ("iv2"), ik)) fail ("iv2=ik");
//    if (!indexDb.first (3, ik) || ik != "ik") fail ("!iv3=ik");

  // Remove indexed.
//    ldb.erase (C2GSTRING ("ik"));
  // Check the index.
//    if (indexDb.first (C2GSTRING ("iv1"), ik)) fail ("iv1");
//    if (indexDb.first (3, ik)) fail ("iv3");
  // Check the data.
//    if (ldb.first (C2GSTRING ("ik"), ik)) fail ("ik");
}

void testStartsWith (Ldb& ldb) {
  // Using `gstring`s because the Boost Serialization encoding for CStrings is not prefix-friendly.
  ldb.put (C2GSTRING ("01"), ""); ldb.put (C2GSTRING ("02"), "");
  ldb.put (C2GSTRING ("11"), "");
  ldb.put (C2GSTRING ("21"), ""); ldb.put (C2GSTRING ("222"), ""); ldb.put (C2GSTRING ("2"), "");

  auto range = ldb.startsWith (C2GSTRING ("0")); auto it = range.begin();
  assert (it->keyView() == "01"); assert (it != range.end());
  assert ((++it)->keyView() == "02"); assert (it != range.end());
  assert ((++it)->keyView().empty()); assert (it == range.end());
  assert ((--it)->keyView() == "02"); assert (it != range.end());
  assert ((--it)->keyView() == "01"); assert (it != range.end());
  assert ((--it)->keyView().empty()); assert (it == range.end());
  // `it` and `range.begin` point to the same `leveldb::Iterator`.
  assert (range.begin()._entry->_lit == it._entry->_lit);
  assert (!range.begin()._entry->_valid); assert (range.begin()->keyView().empty());

  range = ldb.startsWith (C2GSTRING ("0")); it = range.end();
  assert (it.end() && it->keyView().empty()); assert (it != range.begin());
  assert ((--it)->keyView() == "02"); assert (it != range.begin());
  assert ((--it)->keyView() == "01"); assert (it == range.begin());
  assert ((--it)->keyView().empty()); assert (it != range.begin());

  int8_t count = 0; for (auto& en: ldb.startsWith (C2GSTRING ("1"))) {en.keyView(); ++count;} assert (count == 1);
  count = 0; for (auto& en: ldb.startsWith (C2GSTRING ("2"))) {en.keyView(); ++count;} assert (count == 3);
  count = 0; for (auto& en: ldb.startsWith (C2GSTRING ("-"))) {en.keyView(); ++count;} assert (count == 0);
  count = 0; for (auto& en: ldb.startsWith (C2GSTRING (""))) {en.keyView(); ++count;} assert (count == 6);

  assert (ldb.startsWith (C2GSTRING ("-")) .empty());

  count = 0; for (auto& en: boost::make_iterator_range (ldb.end().seek ("1"), ldb.end().seek ("2"))) {en.keyView(); ++count;} assert (count == 1);
  count = 0; for (auto& en: boost::make_iterator_range (ldb.end().seek ("2"), ldb.end().seek ("3"))) {en.keyView(); ++count;} assert (count == 3);
  count = 0; for (auto& en: ldb.range (C2GSTRING ("1"), C2GSTRING ("2"))) {en.keyView(); ++count;} assert (count == 1);

  { auto range = ldb.range (C2GSTRING ("0"), C2GSTRING ("1"));  // 01 and 02, but not 11.
    count = 0; for (auto& en: range) {en.keyView(); ++count;} assert (count == 2); }
}

int main() {
  cout << "Testing ldb.hpp ... " << flush;
  boost::filesystem::remove_all ("/dev/shm/ldbTest");

  Ldb ldb ("/dev/shm/ldbTest");
  test1 (ldb);

  for (auto& en: ldb) ldb.del (en.keyView());
  testStartsWith (ldb);

  ldb._db.reset(); // Close.
  boost::filesystem::remove_all ("/dev/shm/ldbTest");
  cout << "pass." << endl;
  return 0;
}
