#pragma once

#include "byte_slice.h"
#include "byte_stream.h"

namespace portable_scheme {

template<typename T>
bool store_to_binary(const T &t, epee::byte_stream &out);
template<typename T>
bool store_to_binary(const T &t, epee::byte_slice &out);
template<typename T>
bool load_from_binary(T &t, const epee::span<const uint8_t> &in);

}
