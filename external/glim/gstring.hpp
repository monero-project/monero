#ifndef _GSTRING_INCLUDED
#define _GSTRING_INCLUDED

/**
 * A C++ char string.\n
 * Can reuse (stack-allocated) buffers.\n
 * Can create zero-copy views.
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

#include <assert.h>
#include <stdlib.h>  // malloc, realloc, free
#include <stdint.h>
#include <string.h>  // memcpy, memmem
#include <stdio.h>  // snprintf
#include <stdexcept>
#include <iostream>
#include <iterator>

#include "exception.hpp"

/// Make a read-only gstring from a C string: `const gstring foo = C2GSTRING("foo")`.
#define C2GSTRING(CSTR) ::glim::gstring (::glim::gstring::ReferenceConstructor(), CSTR, sizeof (CSTR) - 1, true)
/// Usage: GSTRING_ON_STACK (buf, 64) << "foo" << "bar";
#define GSTRING_ON_STACK(NAME, SIZE) char NAME##Buf[SIZE]; ::glim::gstring NAME (SIZE, NAME##Buf, false, 0); NAME.self()

namespace glim {

/**
 * Based on: C++ version 0.4 char* style "itoa": Written by Lukás Chmela, http://www.strudel.org.uk/itoa/ (GPLv3).
 * Returns a pointer to the end of the string.
 * NB about `inline`: http://stackoverflow.com/a/1759575/257568
 * @param base Maximum is 36 (see http://en.wikipedia.org/wiki/Base_36).
 */
inline char* itoa (char* ptr, int64_t value, const int base = 10) {
  // check that the base is valid
  if (base < 2 || base > 36) {*ptr = '\0'; return ptr;}

  char *ptr1 = ptr;
  int64_t tmp_value;

  do {
    tmp_value = value;
    value /= base;
    *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
  } while (value);

  // Apply negative sign
  if (tmp_value < 0) *ptr++ = '-';
  char* end = ptr;
  *ptr-- = '\0';
  char tmp_char;
  while (ptr1 < ptr) {
    tmp_char = *ptr;
    *ptr--= *ptr1;
    *ptr1++ = tmp_char;
  }
  return end;
}

class gstring_stream;

class gstring {
  enum Flags {
    FREE_FLAG = 0x80000000, // 1st bit; `_buf` needs `free`ing
    FREE_OFFSET = 31,
    REF_FLAG = 0x40000000, // 2nd bit; `_buf` has an extended life-time (such as C string literals) and can be shared (passed by reference)
    REF_OFFSET = 30,
    CAPACITY_MASK = 0x3F000000, // 3..8 bits; `_buf` size is 2^this
    CAPACITY_OFFSET = 24,
    LENGTH_MASK = 0x00FFFFFF, // 9th bit; allocated capacity
  };
  uint32_t _meta;
public:
  void* _buf;
public:
  constexpr gstring() noexcept: _meta (0), _buf (nullptr) {}
  /**
   * Reuse `buf` of size `bufSize`.
   * To fully use `buf` the `bufSize` should be the power of two.
   * @param bufSize The size of the memory allocated to `buf`.
   * @param buf The memory region to be reused.
   * @param free Whether the `buf` should be `free`d on resize or gstring destruction.
   * @param length String length inside the `buf`.
   * @param ref If true then the `buf` isn't copied by gstring's copy constructors.
   *            This is useful for wrapping C string literals.
   */
  explicit gstring (uint32_t bufSize, void* buf, bool free, uint32_t length, bool ref = false) noexcept {
    uint32_t power = 0; while (((uint32_t) 1 << (power + 1)) <= bufSize) ++power;
    _meta = ((uint32_t) free << FREE_OFFSET) |
            ((uint32_t) ref << REF_OFFSET) |
            (power << CAPACITY_OFFSET) |
            (length & LENGTH_MASK);
    _buf = buf;
  }

  struct ReferenceConstructor {};
  /// Make a view to the given cstring.
  /// @param buf The memory region to be reused.
  /// @param length String length inside the `buf`.
  /// @param ref If true then the `buf` isn't copied by gstring's copy constructors.
  ///            This is useful for wrapping C string literals.
  explicit constexpr gstring (ReferenceConstructor, const char* buf, uint32_t length, bool ref = false) noexcept:
    _meta (((uint32_t) ref << REF_OFFSET) | (length & LENGTH_MASK)), _buf ((void*) buf) {}

  /// Copy the characters into `gstring`.
  gstring (const char* chars): _meta (0), _buf (nullptr) {
    if (chars && *chars) {
      size_t length = ::strlen (chars);
      _buf = ::malloc (length);
      ::memcpy (_buf, chars, length);
      _meta = (uint32_t) FREE_FLAG |
              (length & LENGTH_MASK);
    }
  }

  /// Copy the characters into `gstring`.
  gstring (const char* chars, size_t length) {
    if (length != 0) {
      _buf = ::malloc (length);
      ::memcpy (_buf, chars, length);
    } else _buf = nullptr;
    _meta = (uint32_t) FREE_FLAG |
            (length & LENGTH_MASK);
  }

  /// Copy into `gstring`.
  gstring (const std::string& str): _meta (0), _buf (nullptr) {
    if (!str.empty()) {
      _buf = ::malloc (str.length());
      ::memcpy (_buf, str.data(), str.length());
      _meta = (uint32_t) FREE_FLAG |
              (str.length() & LENGTH_MASK);
    }
  }

  /// If `gstr` is `copiedByReference` then make a shallow copy of it,
  /// otherwise copy `gstr` contents into a `malloc`ed buffer.
  gstring (const gstring& gstr) {
    uint32_t glen = gstr.length();
    if (glen != 0) {
      if (gstr.copiedByReference()) {
        _meta = gstr._meta; _buf = gstr._buf;
      } else {
        _buf = ::malloc (glen);
        if (!_buf) GTHROW ("!malloc");
        ::memcpy (_buf, gstr._buf, glen);
        _meta = (uint32_t) FREE_FLAG |
                (glen & LENGTH_MASK);
      }
    } else {
      _meta = 0; _buf = nullptr;
    }
  }
  gstring (gstring&& gstr) noexcept: _meta (gstr._meta), _buf (gstr._buf) {
    gstr._meta = 0; gstr._buf = nullptr;
  }
  gstring& operator = (const gstring& gstr) {
    // cf. http://stackoverflow.com/questions/9322174/move-assignment-operator-and-if-this-rhs
    if (this != &gstr) {
      uint32_t glen = gstr.length();
      uint32_t power = 0;
      uint32_t capacity = this->capacity();
      if (glen <= capacity && capacity > 1) { // `capacity <= 1` means there is no _buf.
        // We reuse existing buffer. Keep capacity info.
        power = (_meta & CAPACITY_MASK) >> CAPACITY_OFFSET;
      } else {
        if (_buf != nullptr && needsFreeing()) ::free (_buf);
        if (gstr.copiedByReference()) {
          _meta = gstr._meta; _buf = gstr._buf;
          return *this;
        }
        _buf = ::malloc (glen);
        if (_buf == nullptr) GTHROW ("malloc failed");
      }
      ::memcpy (_buf, gstr._buf, glen);
      _meta = (uint32_t) FREE_FLAG |
              (power << CAPACITY_OFFSET) |
              (glen & LENGTH_MASK);
    }
    return *this;
  }
  gstring& operator = (gstring&& gstr) noexcept {
    assert (this != &gstr);
    if (_buf != nullptr && needsFreeing()) free (_buf);
    _meta = gstr._meta; _buf = gstr._buf;
    gstr._meta = 0; gstr._buf = nullptr;
    return *this;
  }

  /// Return a copy of the string.
  gstring clone() const {return gstring (data(), length());}
  /// If the gstring's buffer is not owned then copy the bytes into the owned one.
  /// Useful for turning a stack-allocated gstring into a heap-allocated gstring.
  gstring& owned() {if (!needsFreeing()) *this = gstring (data(), length()); return *this;}
  /** Returns a reference to the gstring: when the reference is copied the internal buffer is not copied but referenced (shallow copy).\n
   * This method should only be used if it is know that the life-time of the reference and its copies is less than the life-time of the buffer. */
  gstring ref() const noexcept {return gstring (0, _buf, false, length(), true);}

  bool needsFreeing() const noexcept {return _meta & FREE_FLAG;}
  bool copiedByReference() const noexcept {return _meta & REF_FLAG;}
  /// Current buffer capacity (memory allocated to the string). Returns 1 if no memory allocated.
  uint32_t capacity() const noexcept {return 1 << ((_meta & CAPACITY_MASK) >> CAPACITY_OFFSET);}
  uint32_t length() const noexcept {return _meta & LENGTH_MASK;}
  size_t size() const noexcept {return _meta & LENGTH_MASK;}
  bool empty() const noexcept {return (_meta & LENGTH_MASK) == 0;}
  std::string str() const {size_t len = size(); return len ? std::string ((const char*) _buf, len) : std::string();}
  /// NB: might move the string to a new buffer.
  const char* c_str() const {
    uint32_t len = length(); if (len == 0) return "";
    uint32_t cap = capacity();
    // c_str should work even for const gstring's, otherwise it's too much of a pain.
    if (cap < len + 1) const_cast<gstring*> (this) ->reserve (len + 1);
    char* buf = (char*) _buf; buf[len] = 0; return buf;
  }
  bool equals (const char* cstr) const noexcept {
    const char* cstr_; uint32_t clen_;
    if (cstr != nullptr) {cstr_ = cstr; clen_ = strlen (cstr);} else {cstr_ = ""; clen_ = 0;}
    const uint32_t len = length();
    if (len != clen_) return false;
    const char* gstr_ = _buf != nullptr ? (const char*) _buf : "";
    return memcmp (gstr_, cstr_, len) == 0;
  }
  bool equals (const gstring& gs) const noexcept {
    uint32_t llen = length(), olen = gs.length();
    if (llen != olen) return false;
    return memcmp ((const char*) _buf, (const char*) gs._buf, llen) == 0;
  }

  char& operator[] (unsigned index) noexcept {return ((char*)_buf)[index];}
  const char& operator[] (unsigned index) const noexcept {return ((const char*)_buf)[index];}

  /// Access `_buf` as `char*`. `_buf` might be nullptr.
  char* data() noexcept {return (char*)_buf;}
  const char* data() const noexcept {return (const char*)_buf;}

  char* endp() noexcept {return (char*)_buf + length();}
  const char* endp() const noexcept {return (const char*)_buf + length();}

  gstring view (uint32_t pos, int32_t count = -1) noexcept {
    return gstring (0, data() + pos, false, count >= 0 ? count : length() - pos, copiedByReference());}
  const gstring view (uint32_t pos, int32_t count = -1) const noexcept {
    return gstring (0, (void*)(data() + pos), false, count >= 0 ? count : length() - pos, copiedByReference());}

  // http://en.cppreference.com/w/cpp/concept/Iterator
  template<typename CT> struct iterator_t: public std::iterator<std::random_access_iterator_tag, CT, int32_t> {
    CT* _ptr;
    iterator_t () noexcept: _ptr (nullptr) {}
    iterator_t (CT* ptr) noexcept: _ptr (ptr) {}
    iterator_t (const iterator_t<CT>& it) noexcept: _ptr (it._ptr) {}

    CT& operator*() const noexcept {return *_ptr;}
    CT* operator->() const noexcept {return _ptr;}
    CT& operator[](int32_t ofs) const noexcept {return _ptr[ofs];}

    iterator_t<CT>& operator++() noexcept {++_ptr; return *this;}
    iterator_t<CT> operator++(int) noexcept {return iterator_t<CT> (_ptr++);};
    iterator_t<CT>& operator--() noexcept {--_ptr; return *this;}
    iterator_t<CT> operator--(int) noexcept {return iterator_t<CT> (_ptr--);};
    bool operator == (const iterator_t<CT>& i2) const noexcept {return _ptr == i2._ptr;}
    bool operator != (const iterator_t<CT>& i2) const noexcept {return _ptr != i2._ptr;}
    bool operator < (const iterator_t<CT>& i2) const noexcept {return _ptr < i2._ptr;}
    bool operator > (const iterator_t<CT>& i2) const noexcept {return _ptr > i2._ptr;}
    bool operator <= (const iterator_t<CT>& i2) const noexcept {return _ptr <= i2._ptr;}
    bool operator >= (const iterator_t<CT>& i2) const noexcept {return _ptr >= i2._ptr;}
    iterator_t<CT> operator + (int32_t ofs) const noexcept {return iterator (_ptr + ofs);}
    iterator_t<CT>& operator += (int32_t ofs) noexcept {_ptr += ofs; return *this;}
    iterator_t<CT> operator - (int32_t ofs) const noexcept {return iterator (_ptr - ofs);}
    iterator_t<CT>& operator -= (int32_t ofs) noexcept {_ptr -= ofs; return *this;}
  };
  // http://en.cppreference.com/w/cpp/concept/Container
  typedef char value_type;
  typedef char& reference;
  typedef const char& const_reference;
  typedef uint32_t size_type;
  typedef int32_t difference_type;
  typedef iterator_t<char> iterator;
  typedef iterator_t<const char> const_iterator;
  iterator begin() noexcept {return iterator ((char*) _buf);}
  const_iterator begin() const noexcept {return const_iterator ((char*) _buf);}
  iterator end() noexcept {return iterator ((char*) _buf + size());}
  const_iterator end() const noexcept {return const_iterator ((char*) _buf + size());}
  const_iterator cbegin() const noexcept {return const_iterator ((char*) _buf);}
  const_iterator cend() const noexcept {return const_iterator ((char*) _buf + size());}

  /** Returns -1 if not found. */
  int32_t find (const char* str, int32_t pos, int32_t count) const noexcept {
    const int32_t hlen = (int32_t) length() - pos;
    if (hlen <= 0) return -1;
    char* haystack = (char*) _buf + pos;
    void* mret = memmem (haystack, hlen, str, count);
    if (mret == 0) return -1;
    return (char*) mret - (char*) _buf;
  }
  int32_t find (const char* str, int32_t pos = 0) const noexcept {return find (str, pos, strlen (str));}

  /** Index of `ch` inside the string or -1 if not found. */
  int32_t indexOf (char ch) const noexcept {
    void* ret = memchr (_buf, ch, size());
    return ret == nullptr ? -1 : (char*) ret - (char*) _buf;
  }

  // Helps to workaround the "statement has no effect" warning in `GSTRING_ON_STACK`.
  gstring& self() noexcept {return *this;}

  /** Grow buffer to be at least `to` characters long. */
  void reserve (uint32_t to) {
    uint32_t power = (_meta & CAPACITY_MASK) >> CAPACITY_OFFSET;
    if (((uint32_t) 1 << power) < to) {
      ++power;
      while (((uint32_t) 1 << power) < to) ++power;
      if (power > 24) {GSTRING_ON_STACK (error, 64) << "gstring too large: " << (int) to; GTHROW (error.str());}
    } else if (power) {
      // No need to grow.
      return;
    }
    _meta = (_meta & ~CAPACITY_MASK) | (power << CAPACITY_OFFSET);
    if (needsFreeing() && _buf != nullptr) {
      _buf = ::realloc (_buf, capacity());
      if (_buf == nullptr) GTHROW ("realloc failed");
    } else {
      const char* oldBuf = (const char*) _buf;
      _buf = ::malloc (capacity());
      if (_buf == nullptr) GTHROW ("malloc failed");
      if (oldBuf != nullptr) ::memcpy (_buf, oldBuf, length());
      _meta |= FREE_FLAG;
    }
  }

  /** Length setter. Useful when you manually write into the buffer or to cut the string. */
  void length (uint32_t len) noexcept {
    _meta = (_meta & ~LENGTH_MASK) | (len & LENGTH_MASK);
  }

protected:
  friend class gstring_stream;
public:
  /** Appends an integer to the string.
   * @param base Radix, from 1 to 36 (default 10).
   * @param bytes How many bytes to reserve (24 by default). */
  void append64 (int64_t iv, int base = 10, uint_fast8_t bytes = 24) {
    uint32_t pos = length();
    if (capacity() < pos + bytes) reserve (pos + bytes);
    length (itoa ((char*) _buf + pos, iv, base) - (char*) _buf);
  }
  void append (char ch) {
    uint32_t pos = length();
    const uint32_t cap = capacity();
    if (pos >= cap || cap <= 1) reserve (pos + 1);
    ((char*)_buf)[pos] = ch;
    length (++pos);
  }
  void append (const char* cstr, uint32_t clen) {
    uint32_t len = length();
    uint32_t need = len + clen;
    const uint32_t cap = capacity();
    if (need > cap || cap <= 1) reserve (need);
    ::memcpy ((char*) _buf + len, cstr, clen);
    length (need);
  }
  /** This one is for http://code.google.com/p/re2/; `clear` then `append`. */
  bool ParseFrom (const char* cstr, int clen) {
    if (clen < 0 || clen > (int) LENGTH_MASK) return false;
    length (0); append (cstr, (uint32_t) clen); return true;}
  gstring& operator << (const gstring& gs) {append (gs.data(), gs.length()); return *this;}
  gstring& operator << (const std::string& str) {append (str.data(), str.length()); return *this;}
  gstring& operator << (const char* cstr) {if (cstr) append (cstr, ::strlen (cstr)); return *this;}
  gstring& operator << (char ch) {append (ch); return *this;}
  gstring& operator << (int iv) {append64 (iv, 10, sizeof (int) * 3); return *this;}
  gstring& operator << (long iv) {append64 (iv, 10, sizeof (long) * 3); return *this;}
  gstring& operator << (long long iv) {append64 (iv, 10, sizeof (long long) * 3); return *this;}
  gstring& operator << (double dv) {
    uint32_t len = length();
    reserve (len + 32);
    int rc = snprintf (endp(), 31, "%f", dv);
    if (rc > 0) {length (len + std::min (rc, 31));}
    return *this;
  }

  bool operator < (const gstring &gs) const noexcept {
    uint32_t len1 = length(); uint32_t len2 = gs.length();
    if (len1 == len2) return ::strncmp (data(), gs.data(), len1) < 0;
    int cmp = ::strncmp (data(), gs.data(), std::min (len1, len2));
    if (cmp) return cmp < 0;
    return len1 < len2;
  }

  /// Asks `strftime` to generate a time string. Capacity is increased if necessary (up to a limit of +1024 bytes).
  gstring& appendTime (const char* format, struct tm* tmv) {
    int32_t pos = length(), cap = capacity(), left = cap - pos;
    if (left < 8) {reserve (pos + 8); return appendTime (format, tmv);}
    size_t got = strftime ((char*) _buf + pos, left, format, tmv);
    if (got == 0) {
      if (left > 1024) return *this;  // Guard against perpetual growth.
      reserve (pos + left * 2); return appendTime (format, tmv);
    }
    length (pos + got);
    return *this;
  }

  /// Append the characters to this `gstring` wrapping them in the netstring format.
  gstring& appendNetstring (const char* cstr, uint32_t clen) {
    *this << (int) clen; append (':'); append (cstr, clen); append (','); return *this;}
  /// Append the `gstr` wrapping it in the netstring format.
  gstring& appendNetstring (const gstring& gstr) {return appendNetstring (gstr.data(), gstr.length());}

  std::ostream& writeAsNetstring (std::ostream& stream) const;

  /// Parse netstring at `pos` and return a `gstring` *pointing* at the parsed netstring.\n
  /// No heap space allocated.\n
  /// Throws std::runtime_error if netstring parsing fails.\n
  /// If parsing was successfull, then `after` is set to point after the parsed netstring.
  gstring netstringAt (uint32_t pos, uint32_t* after = nullptr) const {
    const uint32_t len = length(); char* buf = (char*) _buf;
    if (buf == nullptr) GTHROW ("gstring: netstringAt: nullptr");
    uint32_t next = pos;
    while (next < len && buf[next] >= '0' && buf[next] <= '9') ++next;
    if (next >= len || buf[next] != ':' || next - pos > 10) GTHROW ("gstring: netstringAt: no header");
    char* endptr = 0;
    long nlen = ::strtol (buf + pos, &endptr, 10);
    if (endptr != buf + next) GTHROW ("gstring: netstringAt: unexpected header end");
    pos = next + 1; next = pos + nlen;
    if (next >= len || buf[next] != ',') GTHROW ("gstring: netstringAt: no body");
    if (after) *after = next + 1;
    return gstring (0, buf + pos, false, next - pos);
  }

  /// Wrapper around strtol, not entirely safe (make sure the string is terminated with a non-digit, by calling c_str, for example).
  long intAt (uint32_t pos, uint32_t* after = nullptr, int base = 10) const {
    // BTW: http://www.kumobius.com/2013/08/c-string-to-int/
    const uint32_t len = length(); char* buf = (char*) _buf;
    if (pos >= len || buf == nullptr) GTHROW ("gstring: intAt: pos >= len");
    char* endptr = 0;
    long lv = ::strtol (buf + pos, &endptr, base);
    uint32_t next = endptr - buf;
    if (next > len) GTHROW ("gstring: intAt: endptr > len");
    if (after) *after = next;
    return lv;
  }

  /// Wrapper around strtol. Copies the string into a temporary buffer in order to pass it to strtol. Empty string returns 0.
  long toInt (int base = 10) const noexcept {
    const uint32_t len = length(); if (len == 0) return 0;
    char buf[len + 1]; memcpy (buf, _buf, len); buf[len] = 0;
    return ::strtol (buf, nullptr, base);
  }

  /// Get a single netstring from the `stream` and append it to the end of `gstring`.
  /// Throws an exception if the input is not a well-formed netstring.
  gstring& readNetstring (std::istream& stream) {
    int32_t nlen; stream >> nlen;
    if (!stream.good() || nlen < 0) GTHROW ("!netstring");
    int ch = stream.get();
    if (!stream.good() || ch != ':') GTHROW ("!netstring");
    uint32_t glen = length();
    const uint32_t cap = capacity();
    if (cap < glen + nlen || cap <= 1) reserve (glen + nlen);
    stream.read ((char*) _buf + glen, nlen);
    if (!stream.good()) GTHROW ("!netstring");
    ch = stream.get();
    if (ch != ',') GTHROW ("!netstring");
    length (glen + nlen);
    return *this;
  }

  /// Set length to 0. `_buf` not changed.
  gstring& clear() noexcept {length (0); return *this;}

  /// Removes `count` characters starting at `pos`.
  gstring& erase (uint32_t pos, uint32_t count = 1) noexcept {
    const char* buf = (const char*) _buf;
    const char* pt1 = buf + pos;
    const char* pt2 = pt1 + count;
    uint32_t len = length();
    const char* end = buf + len;
    if (pt2 <= end) {
      length (len - count);
      ::memmove ((void*) pt1, (void*) pt2, end - pt2);
    }
    return *this;
  }
  /// Remove characters [from,till) and return `from`.\n
  /// Compatible with "boost/algorithm/string/trim.hpp".
  iterator_t<char> erase (iterator_t<char> from, iterator_t<char> till) noexcept {
    intptr_t ipos = from._ptr - (char*) _buf;
    intptr_t count = till._ptr - from._ptr;
    if (ipos >= 0 && count > 0) erase (ipos, count);
    return from;
  }

  ~gstring() noexcept {
    if (_buf != nullptr && needsFreeing()) {::free (_buf); _buf = nullptr;}
  }
};

inline bool operator == (const gstring& gs1, const gstring& gs2) noexcept {return gs1.equals (gs2);}
inline bool operator == (const char* cstr, const gstring& gstr) noexcept {return gstr.equals (cstr);}
inline bool operator == (const gstring& gstr, const char* cstr) noexcept {return gstr.equals (cstr);}
inline bool operator != (const gstring& gs1, const gstring& gs2) noexcept {return !gs1.equals (gs2);}
inline bool operator != (const char* cstr, const gstring& gstr) noexcept {return !gstr.equals (cstr);}
inline bool operator != (const gstring& gstr, const char* cstr) noexcept {return !gstr.equals (cstr);}

inline bool operator == (const gstring& gstr, const std::string& str) noexcept {
  return gstr.equals (gstring (gstring::ReferenceConstructor(), str.data(), str.size()));}
inline bool operator != (const gstring& gstr, const std::string& str) noexcept {return !(gstr == str);}
inline bool operator == (const std::string& str, const gstring& gstr) noexcept {return gstr == str;}
inline bool operator != (const std::string& str, const gstring& gstr) noexcept {return !(gstr == str);}
inline std::string operator += (std::string& str, const gstring& gstr) {return str.append (gstr.data(), gstr.size());}
inline std::string operator + (const std::string& str, const gstring& gstr) {return std::string (str) .append (gstr.data(), gstr.size());}

inline std::ostream& operator << (std::ostream& os, const gstring& gstr) {
  if (gstr._buf != nullptr) os.write ((const char*) gstr._buf, gstr.length());
  return os;
}

/// Encode this `gstring` into `stream` as a netstring.
inline std::ostream& gstring::writeAsNetstring (std::ostream& stream) const {
  stream << length() << ':' << *this << ',';
  return stream;
}

// http://www.mr-edd.co.uk/blog/beginners_guide_streambuf
// http://www.dreamincode.net/code/snippet2499.htm
// http://spec.winprog.org/streams/
class gstring_stream: public std::basic_streambuf<char, std::char_traits<char> > {
  gstring& _gstr;
public:
  gstring_stream (gstring& gstr) noexcept: _gstr (gstr) {
    char* buf = (char*) gstr._buf;
    if (buf != nullptr) setg (buf, buf, buf + gstr.length());
  }
protected:
  virtual int_type overflow (int_type ch) {
    if (__builtin_expect (ch != traits_type::eof(), 1)) _gstr.append ((char) ch);
    return 0;
  }
  // no copying
  gstring_stream (const gstring_stream &);
  gstring_stream& operator = (const gstring_stream &);
};

} // namespace glim

// hash specialization
// cf. http://stackoverflow.com/questions/8157937/how-to-specialize-stdhashkeyoperator-for-user-defined-type-in-unordered
namespace std {
  template <> struct hash<glim::gstring> {
    size_t operator()(const glim::gstring& gs) const noexcept {
      // cf. http://stackoverflow.com/questions/7666509/hash-function-for-string
      // Would be nice to use https://131002.net/siphash/ here.
      uint32_t hash = 5381;
      uint32_t len = gs.length();
      if (len) {
        const char* str = (const char*) gs._buf;
        const char* end = str + len;
        while (str < end) hash = ((hash << 5) + hash) + *str++; /* hash * 33 + c */
      }
      return hash;
    }
  };
}

#endif // _GSTRING_INCLUDED
