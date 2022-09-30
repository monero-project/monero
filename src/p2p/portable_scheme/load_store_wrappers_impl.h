#pragma once

#include "load_store_wrappers.h"
#include "scheme.h"
#include "binary_codec.h"

namespace portable_scheme {

template<typename T>
bool store_to_binary(const T &t, epee::byte_stream &out) {
  using scheme = scheme_space::scheme<T>;
  const auto &&readable = typename scheme::read_t(t);
  const std::size_t size = binary_codec::size<typename scheme::tags>(readable);
  std::vector<uint8_t> buf(size);
  binary_codec::encode<typename scheme::tags>(readable, buf.data(), buf.size());
  out.reserve(std::max(size, out.available()) - out.available());
  out.write(static_cast<const unsigned char *>(buf.data()), buf.size());
  return true;
}
template<typename T>
bool store_to_binary(const T &t, epee::byte_slice &out) {
  using scheme = scheme_space::scheme<T>;
  const auto &&readable = typename scheme::read_t(t);
  const std::size_t size = binary_codec::size<typename scheme::tags>(readable);
  std::vector<uint8_t> buf(size);
  binary_codec::encode<typename scheme::tags>(readable, buf.data(), buf.size());
  out = epee::byte_slice(std::move(buf));
  return true;
}
template<typename T>
bool load_from_binary(T &t, const epee::span<const uint8_t> &in) {
  using scheme = typename scheme_space::scheme<T>;
  return binary_codec::decode<typename scheme::tags>(typename scheme::write_t(t), in.data(), in.size(), 100).first;
}

}

#define PORTABLE_SCHEME_LOAD_STORE_INSTANCE(T) \
  template bool portable_scheme::store_to_binary(const T &t, epee::byte_stream &out);\
  template bool portable_scheme::store_to_binary(const T &t, epee::byte_slice &out);\
  template bool portable_scheme::load_from_binary(T &t, const epee::span<const uint8_t> &in);
