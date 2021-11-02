#pragma once

#include "tags.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>
#include <utility>
#include <boost/predef/other/endian.h>

namespace portable_scheme {

namespace binary_codec {

enum : std::uint8_t {
  kInt64   = 0x01,
  kInt32   = 0x02,
  kInt16   = 0x03,
  kInt8    = 0x04,
  kUInt64  = 0x05,
  kUInt32  = 0x06,
  kUInt16  = 0x07,
  kUInt8   = 0x08,
  kDouble  = 0x09,
  kSpan    = 0x0A,
  kBool    = 0x0B,
  kMap     = 0x0C,
  kList    = 0x0D,
  kListAny = 0x80,
};

constexpr std::uint64_t kSign = 0x0102010101011101;
constexpr std::uint8_t kVersion = 1;

using namespace tags_space;

#if BOOST_ENDIAN_LITTLE_BYTE == BOOST_VERSION_NUMBER_AVAILABLE
constexpr endian_t native_endian = endian_t::little;
#elif BOOST_ENDIAN_BIG_BYTE == BOOST_VERSION_NUMBER_AVAILABLE
constexpr endian_t native_endian = endian_t::big;
#endif

static_assert(native_endian != endian_t::native, "");

template<typename T>
struct mark_t;

template<typename T, endian_t endian>
struct mark_t<base_tag<T, endian>> {
  static constexpr std::uint8_t value() {
    return (
      std::is_same<std::int64_t,  T>::value ? kInt64 :
      std::is_same<std::int32_t,  T>::value ? kInt32 :
      std::is_same<std::int16_t,  T>::value ? kInt16 :
      std::is_same<std::int8_t,   T>::value ? kInt8 :
      std::is_same<std::uint64_t, T>::value ? kUInt64 :
      std::is_same<std::uint32_t, T>::value ? kUInt32 :
      std::is_same<std::uint16_t, T>::value ? kUInt16 :
      std::is_same<std::uint8_t,  T>::value ? kUInt8 :
      std::is_same<double,        T>::value ? kDouble :
      std::is_same<bool,          T>::value ? kBool :
      0
    );
  }
  static_assert(value() != 0, "");
};

template<std::size_t MIN>
struct mark_t<span_tag<MIN>> {
  static constexpr std::uint8_t value() { return kSpan; }
};

template<typename ...T>
struct mark_t<map_tag<T...>> {
  static constexpr std::uint8_t value() { return kMap; }
};

template<typename T, std::size_t MIN>
struct mark_t<list_tag<T, MIN>> {
  static constexpr std::uint8_t value() { return kListAny | mark_t<T>::value(); }
};

template<typename T, std::size_t MIN, std::size_t MIN_>
struct mark_t<list_tag<list_tag<T, MIN>, MIN_>> {
  static constexpr std::uint8_t value() { return kListAny | kList; }
};

template<endian_t endian, typename T>
T swap_endian(const T &b) {
  static_assert(std::is_pod<T>::value, "");
  if (endian != endian_t::native && endian != native_endian && sizeof(T) >= 2) {
    T a;
    for (std::size_t i = 0; i < sizeof(T); ++i) {
      reinterpret_cast<std::uint8_t *>(&a)[i] = reinterpret_cast<const std::uint8_t *>(&b)[sizeof(T) - i - 1];
    }
    return a;
  }
  return b;
}

template<typename T>
struct codec;

template<typename T, endian_t endian>
struct codec<base_tag<T, endian>> {
  static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "");
  static size_t encode(const T &t, std::uint8_t *out, std::size_t out_len) {
    if (size() <= out_len) {
      *reinterpret_cast<T *>(out) = swap_endian<endian>(t);
    }
    return size();
  }
  static constexpr std::size_t size(const T &) {
    return sizeof(T);
  }
  static constexpr std::size_t size() {
    return sizeof(T);
  }
  static std::pair<bool, std::size_t> decode(T &t, const std::uint8_t *in, std::size_t in_len, std::size_t recursion_limit = 0) {
    if (size() <= in_len) {
      t = swap_endian<endian>(*reinterpret_cast<const T *>(in));
      return {true, size()};
    }
    return {false, in_len};
  }
  static std::pair<bool, std::size_t> skip(const std::uint8_t *in, std::size_t in_len, std::size_t recursion_limit = 0) {
    if (size() <= in_len) {
      return {true, size()};
    }
    return {false, in_len};
  }
};

template<endian_t endian>
struct codec<base_tag<bool, endian>> {
  using T = bool;
  static size_t encode(const T &t, std::uint8_t *out, std::size_t out_len) {
    if (size() <= out_len) {
      *reinterpret_cast<std::uint8_t *>(out) = t;
    }
    return size();
  }
  static constexpr std::size_t size(const T &) {
    return size();
  }
  static constexpr std::size_t size() {
    return sizeof(std::uint8_t);
  }
  static std::pair<bool, std::size_t> decode(T &t, const std::uint8_t *in, std::size_t in_len, std::size_t recursion_limit = 0) {
    if (size() <= in_len) {
      t = *reinterpret_cast<const std::uint8_t *>(in);
      return {true, size()};
    }
    return {false, in_len};
  }
  static std::pair<bool, std::size_t> skip(const std::uint8_t *in, std::size_t in_len, std::size_t recursion_limit = 0) {
    if (size() <= in_len) {
      return {true, size()};
    }
    return {false, in_len};
  }
};

template<typename T>
struct codec<varint_tag<T>> {
  static_assert(std::numeric_limits<T>::is_integer, "");
  static_assert(not std::numeric_limits<T>::is_signed, "");
  static_assert(std::numeric_limits<T>::max() <= std::numeric_limits<uint64_t>::max(), "");
  static std::size_t encode(const T &t, std::uint8_t *out, std::size_t out_len) {
    assert(t <= (std::numeric_limits<std::uint64_t>::max() >> 2));
    const std::size_t size = codec<varint_tag<T>>::size(t);
    const std::uint8_t size_id = codec<varint_tag<T>>::size_id(t);
    if (size <= out_len) {
      switch (size_id) {
        case 0: *reinterpret_cast<std::uint8_t *>(out) = swap_endian<endian_t::little>((t << 2) | size_id); break;
        case 1: *reinterpret_cast<std::uint16_t *>(out) = swap_endian<endian_t::little>((t << 2) | size_id); break;
        case 2: *reinterpret_cast<std::uint32_t *>(out) = swap_endian<endian_t::little>((t << 2) | size_id); break;
        case 3: *reinterpret_cast<std::uint64_t *>(out) = swap_endian<endian_t::little>((t << 2) | size_id); break;
        default: break;
      }
    }
    return size;
  }
  static constexpr std::size_t size(const T &t) {
    return 1 << size_id(t);
  }
  static constexpr std::size_t size() {
    return 1;
  }
  static constexpr std::uint8_t size_id(const T &t) {
    return (
      t < (1 << (8 * sizeof(std::uint8_t) - 2)) ? 0 :
      t < (1 << (8 * sizeof(std::uint16_t) - 2)) ? 1 :
      t < (1 << (8 * sizeof(std::uint32_t) - 2)) ? 2 : 3
    );
  }
  static std::pair<bool, std::size_t> decode(T &t, const std::uint8_t *in, std::size_t in_len) {
    if (not in_len) {
      return {false, 0};
    }
    const std::uint8_t size_id = (*reinterpret_cast<const std::uint8_t *>(in) & 0x3);
    const std::size_t size = (1 << size_id);
    if (size > in_len) {
      return {false, size};
    }
    std::uint64_t value = {};
    switch (size_id) {
      case 0: value = (swap_endian<endian_t::little>(*reinterpret_cast<const std::uint8_t *>(in)) >> 2); break;
      case 1: value = (swap_endian<endian_t::little>(*reinterpret_cast<const std::uint16_t *>(in)) >> 2); break;
      case 2: value = (swap_endian<endian_t::little>(*reinterpret_cast<const std::uint32_t *>(in)) >> 2); break;
      case 3: value = (swap_endian<endian_t::little>(*reinterpret_cast<const std::uint64_t *>(in)) >> 2); break;
      default: assert(false); break;
    }
    t = value;
    return {value <= std::numeric_limits<T>::max(), size};
  }
  static std::pair<bool, std::size_t> skip(const std::uint8_t *in, std::size_t in_len) {
    if (not in_len) {
      return {false, 0};
    }
    const std::uint8_t size_id = (*reinterpret_cast<const std::uint8_t *>(in) & 0x3);
    const std::size_t size = (1 << size_id);
    return {size <= in_len, size};
  }
};

template<std::size_t MIN>
struct codec<span_tag<MIN>> {
  template<typename T>
  static size_t encode(const T &t, std::uint8_t *out, std::size_t out_len) {
    std::size_t size = codec<varint_tag<std::size_t>>::encode(t.size(), out, out_len);
    if (size + t.size() <= out_len) {
      t.get(out + size);
    }
    size += t.size();
    return size;
  }
  template<typename T>
  static size_t size(const T &t) {
    return codec<varint_tag<std::size_t>>::size(t.size()) + t.size();
  }
  static constexpr size_t size() {
    return codec<varint_tag<std::size_t>>::size(MIN) + MIN;
  }
  template<typename T>
  static std::pair<bool, std::size_t> decode(T &&t, const std::uint8_t *in, std::size_t in_len, std::size_t = 0) {
    void const *data = {};
    std::size_t size = {};
    const auto parse_varint = codec<varint_tag<std::size_t>>::decode(size, in, in_len);
    if (not parse_varint.first) {
      return {false, parse_varint.second};
    }
    data = in + parse_varint.second;
    if (parse_varint.second + size <= in_len) {
      return {t.set(data, static_cast<const std::size_t &>(size)), parse_varint.second + size};
    }
    return {false, in_len};
  }
  static std::pair<bool, std::size_t> skip(const std::uint8_t *in, std::size_t in_len, std::size_t = 0) {
    std::uint64_t data_size;
    const auto parse_varint = codec<varint_tag<std::uint64_t>>::decode(data_size, in, in_len);
    const std::size_t size = parse_varint.second;
    if (not parse_varint.first) {
      return {false, size};
    }
    if (size + data_size <= in_len) {
      return {true, size + data_size};
    }
    return {false, in_len};
  }
};

template<typename TT, std::size_t MIN>
struct codec<list_tag<TT, MIN>> {
  template<typename T>
  static size_t encode(const T &t, std::uint8_t *out, std::size_t out_len) {
    std::size_t size = codec<varint_tag<std::size_t>>::encode(t.size(), out, out_len);
    auto &&it = t.cbegin();
    for (std::size_t i = 0; i < t.size(); ++i) {
      size += codec<TT>::encode(it.next(), out + size, size <= out_len ? out_len - size : 0);
    }
    return size;
  }
  template<typename T>
  static std::size_t size(const T &t) {
    std::size_t size = codec<varint_tag<std::size_t>>::size(t.size());
    auto &&it = t.cbegin();
    for (std::size_t i = 0; i < t.size(); ++i) {
      size += codec<TT>::size(it.next());
    }
    return size;
  }
  static constexpr std::size_t size() {
    return codec<varint_tag<std::size_t>>::size(MIN) + MIN * codec<TT>::size();
  }
  template<typename T>
  static std::pair<bool, std::size_t> decode(T &&t, const std::uint8_t *in, std::size_t in_len, std::size_t recursion_limit = 0) {
    std::size_t list_size{};
    const auto parse_list_size = codec<varint_tag<std::size_t>>::decode(list_size, in, in_len);
    std::size_t size = parse_list_size.second;
    if (not parse_list_size.first) {
      return {false, size};
    }
    static_assert(codec<TT>::size(), "");
    const bool force_item_size = t.template force_item_size<codec<TT>::size()>(list_size);
    if (force_item_size and list_size > (in_len - size) / codec<TT>::size()) {
      return {false, 0};
    }
    if (not t.resize(list_size)) {
      return {false, size};
    }
    auto &&it = t.begin();
    if (force_item_size) {
      std::size_t effective_in_len = in_len - list_size * codec<TT>::size();
      while (effective_in_len < in_len) {
        effective_in_len += codec<TT>::size();
        const auto parse_item = codec<TT>::decode(it.next(), in + size, effective_in_len - size, recursion_limit);
        size += parse_item.second;
        if (not parse_item.first) {
          return {false, size};
        }
      }
    }
    else {
      for (std::size_t i = 0; i < list_size; ++i) {
        const auto parse_item = codec<TT>::decode(it.next(), in + size, in_len - size, recursion_limit);
        size += parse_item.second;
        if (not parse_item.first) {
          return {false, size};
        }
      }
    }
    return {true, size};
  }
  static std::pair<bool, std::size_t> skip(const std::uint8_t *in, std::size_t in_len, std::size_t recursion_limit = 0) {
    std::size_t list_size{};
    const auto parse_list_size = codec<varint_tag<std::size_t>>::decode(list_size, in, in_len);
    size_t size = parse_list_size.second;
    if (not parse_list_size.first) {
      return {false, size};
    }
    for (std::size_t i = 0; i < list_size; ++i) {
      const auto skip_item = codec<TT>::skip(in + size, in_len - size, recursion_limit);
      size += skip_item.second;
      if (not skip_item.first) {
        return {false, size};
      }
    }
    return {true, size};
  }
};

template<>
struct codec<list_tag<list_tag<void>>> {
  static std::pair<bool, std::size_t> skip(const std::uint8_t *in, std::size_t in_len, std::size_t recursion_limit = 0) {
    if (recursion_limit == 0) {
      return {false, 0};
    }
    std::size_t list_size;
    const auto parse_list_size = codec<varint_tag<std::size_t>>::decode(list_size, in, in_len);
    std::size_t size = parse_list_size.second;
    if (not parse_list_size.first) {
      return {false, size};
    }
    for (std::size_t i = 0; i < list_size; ++i) {
      std::uint8_t mark = {};
      const auto parse_mark = codec<base_tag<std::uint8_t>>::decode(mark, in + size, in_len - size);
      size += parse_mark.second;
      if (not parse_mark.first) {
        return {false, size};
      }
      std::pair<bool, std::size_t> skip_item = {};
      switch (mark) {
        case kListAny | kInt64:  skip_item = codec<list_tag<base_tag<std::int64_t>> >::skip(in, in_len); break;
        case kListAny | kInt32:  skip_item = codec<list_tag<base_tag<std::int32_t>> >::skip(in, in_len); break;
        case kListAny | kInt16:  skip_item = codec<list_tag<base_tag<std::int16_t>> >::skip(in, in_len); break;
        case kListAny | kInt8:   skip_item = codec<list_tag<base_tag<std::int8_t>>  >::skip(in, in_len); break;
        case kListAny | kUInt64: skip_item = codec<list_tag<base_tag<std::uint64_t>>>::skip(in, in_len); break;
        case kListAny | kUInt32: skip_item = codec<list_tag<base_tag<std::uint32_t>>>::skip(in, in_len); break;
        case kListAny | kUInt16: skip_item = codec<list_tag<base_tag<std::uint16_t>>>::skip(in, in_len); break;
        case kListAny | kUInt8:  skip_item = codec<list_tag<base_tag<std::uint8_t>> >::skip(in, in_len); break;
        case kListAny | kDouble: skip_item = codec<list_tag<base_tag<double>>       >::skip(in, in_len); break;
        case kListAny | kBool:   skip_item = codec<list_tag<base_tag<bool>>         >::skip(in, in_len); break;
        case kListAny | kSpan:   skip_item = codec<list_tag<span_tag<>>             >::skip(in, in_len); break;
        case kListAny | kMap:    skip_item = codec<list_tag<map_tag<void>>          >::skip(in, in_len, recursion_limit - 1); break;
        case kListAny | kList:   skip_item = codec<list_tag<list_tag<void>>         >::skip(in, in_len, recursion_limit - 1); break;
        default: skip_item = {false, 0}; break;
      }
      size += skip_item.second;
      if (not skip_item.first) {
        return {false, size};
      }
    }
    return {true, size};
  }
};

struct key_codec {
  template<typename T>
  static std::size_t encode(const T &t, std::uint8_t *out, std::size_t out_len) {
    const std::size_t size = codec<base_tag<std::uint8_t>>::encode(t.size, out, out_len);
    if (size + t.size <= out_len) {
      std::memcpy(out + size, t.data, t.size);
    }
    return size + t.size;
  }
  template<typename K>
  static constexpr std::size_t size() {
    return codec<base_tag<std::uint8_t>>::size(K::size) + K::size;
  }
  template<typename T>
  static std::pair<bool, std::size_t> decode(T &&t, const std::uint8_t *in, std::size_t in_len) {
    const auto parse_size = codec<base_tag<std::uint8_t>>::decode(t.size, in, in_len);
    const std::size_t size = parse_size.second;
    if (not parse_size.first) {
      return {false, size};
    }
    t.data = reinterpret_cast<char const *>(in + size);
    if (size + t.size <= in_len) {
      return {true, size + t.size};
    }
    return {false, in_len};
  }
  static std::pair<bool, std::size_t> skip(const std::uint8_t *in, std::size_t in_len) {
    std::uint8_t key_size{};
    const auto parse_size = codec<base_tag<std::uint8_t>>::decode(key_size, in, in_len);
    if (not parse_size.first) {
      return {false, parse_size.second};
    }
    if (parse_size.second + key_size <= in_len) {
      return {true, parse_size.second + key_size};
    }
    return {false, in_len};
  }
};

template<>
struct codec<map_tag<void>> {
  static std::pair<bool, std::size_t> skip(const std::uint8_t *in, std::size_t in_len, std::size_t recursion_limit = 0) {
    if (recursion_limit == 0) {
      return {false, 0};
    }
    std::size_t map_size{};
    const auto parse_map_size = codec<varint_tag<std::size_t>>::decode(map_size, in, in_len);
    size_t size = parse_map_size.second;
    if (not parse_map_size.first) {
      return {false, size};
    }
    const auto skip_key_value = codec<map_tag<void>>::skip_key_value(map_size, in + size, in_len - size, recursion_limit);
    size += skip_key_value.second;
    if (not skip_key_value.first) {
      return {false, size};
    }
    return {true, size};
  }
  static std::pair<bool, std::size_t> skip_key_value(std::size_t left_keys, const std::uint8_t *in, std::size_t in_len, std::size_t recursion_limit = 0) {
    if (recursion_limit == 0) {
      return {false, 0};
    }
    std::size_t size = 0;
    for (; left_keys > 0; --left_keys) {
      const auto skip_key = key_codec::skip(in + size, in_len - size);
      size += skip_key.second;
      if (skip_key.first) {
        std::uint8_t mark = {};
        const auto parse_mark = codec<base_tag<std::uint8_t>>::decode(mark, in + size, in_len - size);
        size += parse_mark.second;
        if (not parse_mark.first) {
          return {false, size};
        }
        const auto skip_val = codec<map_tag<void>>::skip_value(mark, in + size, in_len - size, recursion_limit);
        size += skip_val.second;
        if (not skip_val.first) {
          return {false, size};
        }
      }
      else {
        return {false, size};
      }
    }
    return {true, size};
  }
  static std::pair<bool, std::size_t> skip_value(const std::uint8_t &mark, const std::uint8_t *in, std::size_t in_len, std::size_t recursion_limit = 0) {
    if (recursion_limit == 0) {
      return {false, 0};
    }
    switch (mark) {
      case kInt64:  return codec<base_tag<std::int64_t> >::skip(in, in_len);
      case kInt32:  return codec<base_tag<std::int32_t> >::skip(in, in_len);
      case kInt16:  return codec<base_tag<std::int16_t> >::skip(in, in_len);
      case kInt8:   return codec<base_tag<std::int8_t>  >::skip(in, in_len);
      case kUInt64: return codec<base_tag<std::uint64_t>>::skip(in, in_len);
      case kUInt32: return codec<base_tag<std::uint32_t>>::skip(in, in_len);
      case kUInt16: return codec<base_tag<std::uint16_t>>::skip(in, in_len);
      case kUInt8:  return codec<base_tag<std::uint8_t> >::skip(in, in_len);
      case kDouble: return codec<base_tag<double>       >::skip(in, in_len);
      case kBool:   return codec<base_tag<bool>         >::skip(in, in_len);
      case kSpan:   return codec<span_tag<>             >::skip(in, in_len);
      case kMap:    return codec<map_tag<void>          >::skip(in, in_len, recursion_limit - 1);
      case kListAny | kInt64:  return codec<list_tag<base_tag<std::int64_t>> >::skip(in, in_len);
      case kListAny | kInt32:  return codec<list_tag<base_tag<std::int32_t>> >::skip(in, in_len);
      case kListAny | kInt16:  return codec<list_tag<base_tag<std::int16_t>> >::skip(in, in_len);
      case kListAny | kInt8:   return codec<list_tag<base_tag<std::int8_t>>  >::skip(in, in_len);
      case kListAny | kUInt64: return codec<list_tag<base_tag<std::uint64_t>>>::skip(in, in_len);
      case kListAny | kUInt32: return codec<list_tag<base_tag<std::uint32_t>>>::skip(in, in_len);
      case kListAny | kUInt16: return codec<list_tag<base_tag<std::uint16_t>>>::skip(in, in_len);
      case kListAny | kUInt8:  return codec<list_tag<base_tag<std::uint8_t>> >::skip(in, in_len);
      case kListAny | kDouble: return codec<list_tag<base_tag<double>>       >::skip(in, in_len);
      case kListAny | kBool:   return codec<list_tag<base_tag<bool>>         >::skip(in, in_len);
      case kListAny | kSpan:   return codec<list_tag<span_tag<>>             >::skip(in, in_len);
      case kListAny | kMap:    return codec<list_tag<map_tag<void>>          >::skip(in, in_len, recursion_limit - 1);
      case kListAny | kList:   return codec<list_tag<list_tag<void>>         >::skip(in, in_len, recursion_limit - 1);
      default: return {false, 0};
    }
  }
  template<typename T>
  static std::pair<bool, std::size_t> skip_to_key(
    const T &key,
    int &key_cmp,
    std::size_t &left_keys,
    const std::uint8_t &mark,
    bool &mark_match,
    const std::uint8_t *in,
    std::size_t in_len,
    std::size_t recursion_limit = 0
  ) {
    if (recursion_limit == 0) {
      return {false, 0};
    }
    size_t size = 0;
    for (; left_keys > 0; --left_keys) {
      struct {
        char const *data;
        std::uint8_t size;
      } found_key;
      const auto parse_key = key_codec::decode(found_key, in + size, in_len - size);
      size += parse_key.second;
      if (parse_key.first) {
        std::uint8_t found_mark;
        const auto parse_mark = codec<base_tag<std::uint8_t>>::decode(found_mark, in + size, in_len - size);
        size += parse_mark.second;
        if (not parse_mark.first) {
          return {false, size};
        }
        auto lexicographical_compare = [](const void *lhs, std::size_t lhs_size, const void *rhs, std::size_t rhs_size) {
          int cmp = std::memcmp(lhs, rhs, lhs_size < rhs_size ? lhs_size : rhs_size);
          return (
            cmp != 0 ? cmp :
            lhs_size < rhs_size ? -1 :
            lhs_size > rhs_size ? 1 :
            0
          );
        };
        const int cmp = lexicographical_compare(found_key.data, found_key.size, key.data, key.size);
        if (cmp == 0) {
          key_cmp = cmp;
          mark_match = (found_mark == mark);
          if (mark_match) {
            return {true, size};
          }
          else {
            size -= parse_key.second;
            size -= parse_mark.second;
            return {true, size};
          }
        }
        else if (cmp > 0) {
          size -= parse_key.second;
          size -= parse_mark.second;
          key_cmp = cmp;
          mark_match = (found_mark == mark);
          return {true, size};
        }
        const auto skip_val = skip_value(found_mark, in + size, in_len - size, recursion_limit);
        size += skip_val.second;
        if (not skip_val.first) {
          return {false, size};
        }
      }
      else {
        return {false, size};
      }
    }
    return {true, size};
  }
  template<typename ...T>
  struct less;
  template<char ...C, char ...CC>
  struct less<key_space::key_t<C...>, key_space::key_t<CC...>> {
    using L = key_space::key_t<C...>;
    using R = key_space::key_t<CC...>;
    template<std::size_t N> struct arg_t {};
    template<std::size_t N>
    static constexpr bool less_t(arg_t<N>) {
      return L::data[N] != R::data[N] ? L::data[N] < R::data[N] : less_t(arg_t<N + 1>{});
    }
    static constexpr bool less_t(arg_t<(sizeof...(C) < sizeof...(CC) ? sizeof...(C) : sizeof...(CC))>) {
      return sizeof...(C) < sizeof...(CC);
    };
    static constexpr bool value = less_t(arg_t<0>{});
  };
  template<template<typename> class S, typename ...T>
  struct sorted {
    template<typename ...> struct arg_t {};
    static constexpr bool value_t(arg_t<>) { return true; }
    template<typename TT>
    static constexpr bool value_t(arg_t<TT>) { return true; }
    template<typename TT, typename TTT, typename ...TTTT>
    static constexpr bool value_t(arg_t<TT, TTT, TTTT...>) {
       return not less<typename S<TTT>::type, typename S<TT>::type>::value and value_t(arg_t<TTT, TTTT...>{});
    }
    static constexpr bool value = value_t(arg_t<T...>{});
  };
  template<typename>
  struct key_selector;
  template<typename T, typename TT, optional_t optional>
  struct key_selector<field_tag<T, TT, optional>> { using type = T; };
};

template<typename ...>
struct map_codec;
template<typename Key, typename Tag, optional_t optional, typename ...Tags>
struct map_codec<field_tag<Key, Tag, optional>, Tags...> {
  template<typename T>
  static std::size_t fields(const T &t) {
    return (optional == optional_t::required or t.template enabled<Key, typename field_key<Tag>::type>()) + map_codec<Tags...>::fields(t);
  }
  static constexpr std::size_t fields() {
    return (optional != optional_t::optional ? 1 : 0) + map_codec<Tags...>::fields();
  }
  template<typename T>
  static std::size_t encode(const T &t, std::uint8_t *out, std::size_t out_len) {
    size_t size{};
    if (optional == optional_t::required or t.template enabled<Key, typename field_key<Tag>::type>()) {
      size = key_codec::encode(Key::to_array(), out, out_len);
      size += codec<base_tag<std::uint8_t>>::encode(mark_t<Tag>::value(), out + size, size <= out_len ? out_len - size : 0);
      size += codec<Tag>::encode(t.template get<Key, typename field_key<Tag>::type>(), out + size, size <= out_len ? out_len - size : 0);
    }
    size += map_codec<Tags...>::encode(t, out + size, size <= out_len ? out_len - size : 0);
    return size;

  }
  template<typename T>
  static std::size_t size(const T &t) {
    return (
      (optional == optional_t::required or t.template enabled<Key, typename field_key<Tag>::type>()) ?
      (
       key_codec::size<Key>() +
       codec<base_tag<std::uint8_t>>::size(mark_t<Tag>::value()) +
       codec<Tag>::size(t.template get<Key, typename field_key<Tag>::type>())
      ) : 0
    ) + map_codec<Tags...>::size(t);
  }
  static constexpr std::size_t size() {
    return (
      (optional != optional_t::optional ? codec<Tag>::size() : 0) ?
      (
       key_codec::size<Key>() +
       codec<base_tag<std::uint8_t>>::size(mark_t<Tag>::value()) +
       codec<Tag>::size()
      ) : 0
    ) + map_codec<Tags...>::size();
  }
  template<typename T>
  static std::pair<bool, std::size_t> decode(T &&t, std::size_t left_keys, const std::uint8_t *in, std::size_t in_len, std::size_t recursion_limit = 0) {
    std::size_t size = 0;
    if (left_keys > 0) {
      int key_cmp{};
      bool mark_match{};
      const auto skip_to_key = codec<map_tag<void>>::skip_to_key(
          Key::to_array(),
          key_cmp,
          left_keys,
          mark_t<Tag>::value(),
          mark_match,
          in,
          in_len,
          recursion_limit
          );
      size += skip_to_key.second;
      if (skip_to_key.first && key_cmp == 0 && mark_match) {
        --left_keys;
        auto &&field = t.template get<Key, typename field_key<Tag>::type>();
        const auto decode_field = codec<Tag>::decode(
          field,
          in + size,
          size <= in_len ? in_len - size : 0,
          recursion_limit
        );
        size += decode_field.second;
        if (not decode_field.first) {
          return {false, size};
        }
      }
      else if (not skip_to_key.first) {
        return {false, size};
      }
      else if (not t.template set_default<Key, typename field_key<Tag>::type>()) {
        return {false, size};
      }
    }
    else {
      if (not t.template set_default<Key, typename field_key<Tag>::type>()) {
        return {false, size};
      }
    }
    const auto decode_fields = map_codec<Tags...>::decode(
      t,
      left_keys,
      in + size,
      size <= in_len ? in_len - size : 0,
      recursion_limit
    );
    size += decode_fields.second;
    return {decode_fields.first, size};
  }
};

template<>
struct map_codec<> {
  template<typename T>
  static std::size_t fields(const T &) {
    return 0;
  }
  static constexpr std::size_t fields() {
    return 0;
  }
  template<typename T>
  static size_t encode(const T &, std::uint8_t *out, std::size_t out_len) {
    return 0;
  }
  template<typename T>
  static size_t size(const T &) {
    return 0;
  }
  static constexpr std::size_t size() {
    return 0;
  }
  template<typename T>
  static std::pair<bool, size_t> decode(T &&, std::size_t left_keys, const std::uint8_t *in, std::size_t in_len, size_t recursion_limit = 0) {
    return codec<map_tag<void>>::skip_key_value(left_keys, in, in_len, recursion_limit);
  }
};

template<typename ...TT>
struct codec<map_tag<TT...>> {
  static_assert(codec<map_tag<void>>::sorted<codec<map_tag<void>>::key_selector, TT...>::value, "");
  template<typename T>
  static std::size_t encode(const T &t, std::uint8_t *out, std::size_t out_len) {
    const std::size_t size = codec<varint_tag<std::size_t>>::encode(map_codec<TT...>::fields(t), out, out_len);
    return size + map_codec<TT...>::encode(t, out + size, size <= out_len ? out_len - size : 0);
  }
  template<typename T>
  static std::size_t size(const T &t) {
    return codec<varint_tag<std::size_t>>::size(map_codec<TT...>::fields(t)) + map_codec<TT...>::size(t);
  }
  static constexpr std::size_t size() {
    return codec<varint_tag<std::size_t>>::size(map_codec<TT...>::fields()) + map_codec<TT...>::size();
  }
  template<typename T>
  static std::pair<bool, std::size_t> decode(T &&t, const std::uint8_t *in, std::size_t in_len, std::size_t recursion_limit = 0) {
    std::size_t keys{};
    const auto parse_varint = codec<varint_tag<std::size_t>>::decode(keys, in, in_len);
    std::size_t size = parse_varint.second;
    if (not parse_varint.first) {
      return {false, size};
    }
    const auto parse_fields = map_codec<TT...>::decode(t, keys, in + size, size <= in_len ? in_len - size : 0, recursion_limit);
    size += parse_fields.second;
    return {parse_fields.first ? t.set() : false, size};
  }
};

template<typename TT, typename T>
std::size_t encode(const T &t, std::uint8_t *out, const std::size_t out_len) {
  struct magic_codec {
    static std::size_t encode(std::uint8_t *out, const std::size_t out_len) {
      std::size_t size = codec<base_tag<std::uint64_t>>::encode(kSign, out, out_len);
      size += codec<base_tag<std::uint8_t>>::encode(kVersion, out + size, size <= out_len ? out_len - size: 0);
      return size;
    }
  };
  std::size_t size = magic_codec::encode(out, out_len);
  size += codec<TT>::encode(t, out + size, size <= out_len ? out_len - size: 0);
  return size;
}

template<typename TT, typename T>
std::size_t size(const T &t) {
  struct magic_codec {
    static constexpr std::size_t size() {
      return codec<base_tag<std::uint64_t>>::size(kSign) + codec<base_tag<std::uint8_t>>::size(kVersion);
    }
  };
  return magic_codec::size() + codec<TT>::size(t);
}

template<typename TT, typename T>
std::pair<bool, std::size_t> decode(T &&t, const std::uint8_t *in, const std::size_t in_len, std::size_t recursion_limit = 0) {
  struct magic_codec {
    static std::pair<bool, std::size_t> skip(const std::uint8_t *in, std::size_t in_len) {
      std::uint64_t sign{};
      auto parse_sign = codec<base_tag<std::uint64_t>>::decode(sign, in, in_len);
      std::size_t size = parse_sign.second;
      if (not parse_sign.first or sign != kSign) {
          return {false, size};
      }
      std::uint8_t version{};
      auto parse_version = codec<base_tag<std::uint8_t>>::decode(version, in + size, in_len - size);
      size += parse_version.second;
      if (not parse_version.first or version != kVersion) {
          return {false, size};
      }
      return {true, size};
    }
  };
  const auto skip_magic = magic_codec::skip(in, in_len);
  std::size_t size = skip_magic.second;
  if (not skip_magic.first) {
    return {false, size};
  }
  const auto parse_obj = codec<TT>::decode(t, in + size, in_len - size, recursion_limit);
  size += parse_obj.second;
  return {parse_obj.first, size};
}

}

}
