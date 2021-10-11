#pragma once

#include "load_store_wrappers.h"
#include "scheme.h"
#include "binary_codec.h"

namespace portable_scheme {

template<typename T>
bool store_to_binary(const T &t, std::string &out) {
  using scheme = scheme_space::scheme<T>;
  const auto &&readable = typename scheme::read_t(t);
  const std::size_t size = binary_codec::size<typename scheme::tags>(readable);
  out.resize(size);
  binary_codec::encode<typename scheme::tags>(readable, &reinterpret_cast<std::uint8_t &>(out[0]), out.size());
  return true;
}
template<typename T>
bool load_from_binary(T &t, const epee::span<const uint8_t> &in) {
  using scheme = typename scheme_space::scheme<T>;
  return binary_codec::decode<typename scheme::tags>(typename scheme::write_t(t), in.data(), in.size(), 100).first;
}

}

#define PORTABLE_SCHEME_LOAD_STORE_INSTANCE(T) \
  template bool portable_scheme::store_to_binary(const T &t, std::string &out);\
  template bool portable_scheme::load_from_binary(T &t, const epee::span<const uint8_t> &in);
