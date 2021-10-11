#pragma once

#include <cstdint>

namespace portable_scheme {

namespace tags_space {

enum struct endian_t: std::uint8_t {
  little,
  big,
  native,
};

template<typename T, endian_t = endian_t::little>
struct base_tag;

template<typename T>
struct varint_tag;

template<std::size_t MIN = 0>
struct span_tag;

enum struct optional_t {
  optional,
  required,
  exception,
};

template<typename T, typename TT, optional_t = optional_t::optional>
struct field_tag;

template<typename ...T>
struct map_tag;

template<typename T, std::size_t MIN = 0>
struct list_tag;

template<typename T>
struct field_key;

template<typename T, endian_t endian>
struct field_key<base_tag<T, endian>> {
  using type = base_tag<T>;
};

template<typename T>
struct field_key<varint_tag<T>> {
  using type = varint_tag<T>;
};

template<std::size_t MIN>
struct field_key<span_tag<MIN>> {
  using type = span_tag<>;
};

template<typename ...T>
struct field_key<map_tag<T...>> {
  using type = map_tag<>;
};

template<typename T, std::size_t MIN>
struct field_key<list_tag<T, MIN>> {
  using type = list_tag<typename field_key<T>::type>;
};

}

namespace key_space {

#if 0
https://github.com/irrequietus/typestring/blob/master/typestring.hh
#endif

template<char ...C>
struct key_t {
  static constexpr char const data[sizeof...(C)+1] = {C..., '\0'};
  static constexpr unsigned int size = sizeof...(C);
  struct array_t {
    char const data[sizeof...(C)+1] = {C..., '\0'};
    static constexpr unsigned int size = sizeof...(C);
  };
  static constexpr array_t to_array() { return {}; }
};

template<char ...C>
auto second(key_t<C...>) -> key_t<C...>;

template<char ...C, char ...CC>
auto second(key_t<C...>, key_t<'\0'>, key_t<CC>...) -> key_t<C...>;

template<char C, char ...CC, char ...CCC>
auto second(key_t<CC...>, key_t<C>, key_t<CCC>...) -> decltype(second(key_t<CC..., C>(), key_t<CCC>()...));

template<char ...C>
auto third(key_t<C...>) -> decltype(second(key_t<C>()...));

template<int A, int B>
constexpr char fourth(const char (&c)[B]) {
  static_assert(B < 32, "");
  return c[A < B ? A : B - 1];
}

#define REP2(F, I, A) F(I, A), F(I + 1, A)
#define REP4(F, I, A) REP2(F, I, A), REP2(F, I + 2, A)
#define REP8(F, I, A) REP4(F, I, A), REP4(F, I + 4, A)
#define REP16(F, I, A) REP8(F, I, A), REP8(F, I + 8, A)
#define REP32(F, I, A) REP16(F, I, A), REP16(F, I + 16, A)
#define THIRD(I, A) portable_scheme::key_space::fourth<I>(A)
#define KEY(A) decltype(portable_scheme::key_space::third(portable_scheme::key_space::key_t<REP32(THIRD, 0, A)>()))

}

}
