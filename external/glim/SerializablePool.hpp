#ifndef _GLIM_SERIALIZABLEPOOL_HPP_INCLUDED
#define _GLIM_SERIALIZABLEPOOL_HPP_INCLUDED

#include "gstring.hpp"
#ifndef _SERIALIZABLEPOOL_NOLDB
# include "ldb.hpp" // Reuse `ldbSerialize` and `ldbDeserialize` in the `with` method.
#endif
namespace glim {

namespace SerializablePoolHelpers {
  struct Impl {
    /**
     * Pool format: \code
     * uint32_t valuesStart;  // Network byte order.
     * uint32_t value0Offset, value1Offset, ... valueNOffset;  // Network byte order.
     * char value0[]; char zero; char value1[]; char zero; ... char valueN[]; char zero;
     * \endcode
     */
    gstring _pool;
    std::vector<gstring> _changes;
    std::vector<bool> _changed;
    bool _readOnly = false;
    Impl() = default;
    Impl (const gstring& poolBytes): _pool (poolBytes), _readOnly (false) {}
  };

  /// Can be used to avoid a deep copy of the pool and change vectors (pImpl idiom).
  /// Example: \code glim::SerializablePool pool; \endcode
  struct SharedPtr {
    std::shared_ptr<Impl> _impl;
    SharedPtr() = default;
    SharedPtr (const gstring& poolBytes): _impl (std::make_shared<Impl> (poolBytes)) {}
    Impl* get() const {return _impl.get();}
    Impl* operator->() const {return _impl.get();}
    Impl& operator*() const {return *_impl;}
    Impl* instance() {if (!_impl) _impl = std::make_shared<Impl>(); return _impl.get();}
  };

  /// Can be used instead of SharedPtr to avoid the shared_ptr indirection when the SerializablePoolTpl isn't going to be copied.
  /// Example: \code glim::InlineSerializablePool temporaryPool (bytes); \endcode
  struct InlinePtr {
    Impl _impl;
    InlinePtr() = default;
    InlinePtr (const gstring& poolBytes): _impl (poolBytes) {}
    Impl* get() const {return const_cast<Impl*> (&_impl);}
    Impl* operator->() const {return const_cast<Impl*> (&_impl);}
    Impl& operator*() const {return const_cast<Impl&> (_impl);}
    Impl* instance() {return &_impl;}
  };
}

/** Serialization with lazy parsing: fields are accessed without "unpacking" the byte array.
 * Changes are stored separately, allowing the user to know exactly what fields has been changed and compare the old values to the new ones. */
template <typename PI>
class SerializablePoolTpl {
protected:
  using Impl = SerializablePoolHelpers::Impl;
  PI _impl;
  /** @param ref Return a zero-copy view. The view should not be used outside of the pool buffer's lifetime. */
  static gstring original (const gstring& pool, uint32_t num, bool ref = false) {
    uint32_t poolLength = pool.length(); if (poolLength < 4) return gstring();
    uint32_t valuesStart = ntohl (*(uint32_t*) pool.data());
    assert (valuesStart <= poolLength);
    uint32_t valueOffsetOffset = 4 + num * 4;
    if ((int) valuesStart - (int) valueOffsetOffset < 4) return gstring(); // num > size
    uint32_t valueOffset = ntohl (*(uint32_t*) (pool.data() + valueOffsetOffset));
    valueOffsetOffset += 4;
    uint32_t nextValueOffset = ((int) valuesStart - (int) valueOffsetOffset < 4)
      ? poolLength
      : ntohl (*(uint32_t*) (pool.data() + valueOffsetOffset));
    return gstring (0, (void*) (pool.data() + valueOffset), false, nextValueOffset - 1 - valueOffset, ref);
  }
  /** How many elements are in the pool. */
  static uint32_t poolSize (const gstring& pool) {
    if (pool.length() < 4) return 0;
    uint32_t valuesStart = ntohl (*(uint32_t*) pool.data());
    return (valuesStart - 4) / 4;
  }
  void toBytes (gstring& newPool, uint32_t size, const gstring* oldPool) const {
    newPool.clear();
    const Impl* impl = _impl.get();
    const std::vector<bool>& changed = impl->_changed;
    const std::vector<gstring>& changes = impl->_changes;
    if (changed.empty()) return;
    uint32_t valuesStart = 4 + size * 4;
    uint32_t networkOrder = 0;
    newPool.append ((char*) &(networkOrder = htonl (valuesStart)), 4);
    for (uint32_t num = 0; num < size; ++num) newPool.append ((char*) &(networkOrder = 0), 4);
    for (uint32_t num = 0; num < size; ++num) {
      uint32_t start = newPool.length();
      if (num < changed.size() && changed[num]) newPool << changes[num];
      else newPool << original (oldPool ? *oldPool : impl->_pool, num);
      newPool << '\0';
      uint32_t valuesOffsetOffset = 4 + num * 4; assert (valuesOffsetOffset < valuesStart);
      *(uint32_t*)(newPool.data() + valuesOffsetOffset) = htonl (start);
    }
  }
public:
  /** Field, old value, new value. Might be used to maintain indexes. */
  typedef std::function<void(uint32_t, const gstring&, const gstring&)> ChangeVisitor;

  SerializablePoolTpl() = default;
  /** Copy the given pool bytes from the outside source (e.g. from the database). */
  SerializablePoolTpl (const gstring& poolBytes): _impl (poolBytes) {}
  /** Returns a view into the original serialized field (ignores the current changes).\n
   * Returns an empty string if the field is not in the pool (num > size).
   * @param ref Return a zero-copy view. The view becomes invalid after the value has been changed or when the pool's `Impl` is destroyed. */
  const gstring original (uint32_t num, bool ref = false) const {return original (_impl->_pool, num, ref);}
  /** Returns the original serialized field (ignores the current changes).\n
   * Returns an empty string if the field is not in the pool (num > size). */
  const char* cstringOriginal (uint32_t num) const {
    gstring gs (original (_impl->_pool, num));
    return gs.empty() ? "" : gs.data(); // All fields in the _pool are 0-terminated.
  }
  /** Returns the field.
   * @param ref Return a zero-copy view. The view becomes invalid after the value has been changed or when the pool's `Impl` is destroyed. */
  const gstring current (uint32_t num, bool ref = false) const {
    const Impl* impl = _impl.get(); if (!impl) return gstring();
    if (num < impl->_changed.size() && impl->_changed[num]) {
      const gstring& value = impl->_changes[num]; return ref ? value.ref() : value;}
    return original (impl->_pool, num);
  }
  /** Set the new value of the field. */
  void set (uint32_t num, const gstring& value) {
    Impl* impl = _impl.instance();
    if (__builtin_expect (impl->_readOnly, 0)) throw std::runtime_error ("Attempt to modify a read-only SerializablePool");
    if (num >= impl->_changed.size()) {impl->_changed.resize (num + 1); impl->_changes.resize (num + 1);}
    impl->_changed[num] = true;
    impl->_changes[num] = value;
  }
  void reserve (uint32_t fields) {
    Impl* impl = _impl.instance();
    if (__builtin_expect (impl->_readOnly, 0)) throw std::runtime_error ("Attempt to modify a read-only SerializablePool");
    impl->_changed.reserve (fields);
    impl->_changes.reserve (fields);
  }
  /** Peek into the pool.\n
   * Returned reference should not be used after the SerializablePool goes out of scope (and destroyed). */
  const gstring& originalPool() {return _impl->_pool;}
  /** Serialize the pool.
   * @param changeVisitor is called for every field that was really changed (e.g. the bytes differ). */
  void toBytes (gstring& newPool, ChangeVisitor changeVisitor = ChangeVisitor()) const {
    if (changeVisitor) {
      const Impl* impl = _impl.get();
      const std::vector<bool>& changed = impl->_changed;
      const std::vector<gstring>& changes = impl->_changes;
      for (uint32_t num = 0, size = changed.size(); num < size; ++num) if (changed[num]) {
        const gstring& from = original (impl->_pool, num); const gstring& to = changes[num];
        if (from != to) changeVisitor (num, from, to);
      }
    }
    toBytes (newPool, (uint32_t) _impl->_changed.size(), nullptr);
  }
  /**
   * Performs "delta" serialization of the pool: creates a new pool where values which has not changed are copied from the `oldPool`.\n
   * \code Use case: 1) pools X and Y are loaded from a database by users A and B;
   *           2) user A changes field 0 in pool X; 3) user B changes field 1 in pool Y;
   *           4) user A loads `oldPool` from the database, does `toBytesDelta` from pool X and saves to the database;
   *           5) user B loads `oldPool` from the database, does `toBytesDelta` from pool Y and saves to the database;
   *           result: database contains both changes (field 0 from user A and field 1 from user B). \endcode
   * @param changeVisitor is called for every field that was changed between the oldPool and the current one.
   * Returns `false` and leaves `newPool` *empty* if there are no changes found against the `oldPool`.
   */
  bool toBytesDelta (gstring& newPool, const gstring& oldPool, ChangeVisitor changeVisitor = ChangeVisitor()) const {
    newPool.clear();
    const Impl* impl = _impl.get();
    const std::vector<bool>& changed = impl->_changed;
    const std::vector<gstring>& changes = impl->_changes;
    bool verifiedChanges = false;
    for (uint32_t num = 0, size = changed.size(); num < size; ++num) if (changed[num]) {
      const gstring& from = original (oldPool, num); const gstring& to = changes[num];
      if (from != to) {
        verifiedChanges = true;
        if (changeVisitor) changeVisitor (num, from, to); else break;
      }
    }
    if (!verifiedChanges) return false;

    toBytes (newPool, std::max ((uint32_t) changed.size(), poolSize (oldPool)), &oldPool);
    return true;
  }
  /** True if the field has been `set` in this pool instance.\n
   * NB: Does *not* check if the `set` value is equal to the `original` value or not. */
  bool changed (uint32_t num) const {const auto& changed = _impl->_changed; return num < changed.size() ? changed[num] : false;}
  /** True if a field has been `set` in this pool instance. */
  bool changed() const {return !_impl->_changed.empty();}

  bool operator == (const SerializablePoolTpl<PI>& rhs) const {return _impl.get() == rhs._impl.get();}
  /** Useful for storing SerializablePool in a map. */
  intptr_t implId() const {return (intptr_t) _impl.get();}

  /** If set to `true` then modifying the pool will throw an exception.\n
   * Useful for freezing the pool before sharing it with other threads. */
  void readOnly (bool ro) {if (_impl) _impl->_readOnly = true;}
  bool readOnly() const {return (_impl ? _impl->_readOnly : false);}

  /** Number of elements in the pool. Equals to max(num)-1. */
  uint32_t size() const {
    Impl* impl = _impl.get(); if (__builtin_expect (!impl, 0)) return 0;
    return std::max (poolSize (impl->_pool), (uint32_t) impl->_changed.size());
  }

#ifndef _SERIALIZABLEPOOL_NOLDB
  /** Serialize the `value` with `ldbSerialize` and `set` it to `num`.
   * @param stackSize is the amount of space to preallocate on stack for the temporary buffer. */
  template<typename T> void serialize (uint32_t num, const T& value, uint32_t stackSize = 256) {
    GSTRING_ON_STACK (bytes, stackSize);
    ldbSerialize (bytes, value);
    set (num, bytes);
  }
  /** If the field is not empty then `ldbDeserialize` it into `value`. */
  template<typename T> void deserialize (uint32_t num, T& value) const {
    const gstring& bytes = current (num);
    if (bytes.length()) ldbDeserialize (current (num), value);
  }
  /** Deserialize the `num` field with `ldbDeserialize`, run `visitor` on it, then optionally serialize the field back using `ldbSerialize`.
   * Example: \code
   *   typedef std::map<std::string, std::string> MyMap;
   *   pool.with<MyMap> (_myMap, [](MyMap& myMap) {myMap["foo"] = "bar"; return true;});
   * \endcode
   * @param visitor must return `true` to serialize the field back to the pool.
   */
  template<typename T> void with (uint32_t num, std::function<bool(T&)> visitor) {
    const gstring& fromBytes = current (num, true);
    T value; if (fromBytes.length()) ldbDeserialize (fromBytes, value);
    if (visitor (value)) serialize (num, value, 16 + fromBytes.length() * 2);
  }
#endif
};

using SerializablePool = SerializablePoolTpl<SerializablePoolHelpers::SharedPtr>;
using InlineSerializablePool = SerializablePoolTpl<SerializablePoolHelpers::InlinePtr>;

}

#endif // _GLIM_SERIALIZABLEPOOL_HPP_INCLUDED
