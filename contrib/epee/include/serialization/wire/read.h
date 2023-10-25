// Copyright (c) 2023, The Monero Project
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>

#include "byte_slice.h"
#include "serialization/wire/epee/fwd.h"
#include "serialization/wire/error.h"
#include "serialization/wire/field.h"
#include "serialization/wire/fwd.h"
#include "serialization/wire/traits.h"
#include "span.h"

/*
  Custom types (e.g. `type` in namespace `ns`) can define an input function by:
    * `namespace wire { template<> struct is_blob<ns::type> : std::true_type {}; }`
    * `namespace wire { void read_bytes(writer&, ns::type&); }`
    * `namespace ns { void read_bytes(wire::writer&, type&); }`

  See `traits.h` for `is_blob` requirements. `read_bytes` function can also
  specify derived type for faster output (i.e.
  `namespace ns { void read_bytes(wire::epee_reader&, type&); }`). Using the
  derived type allows the compiler to de-virtualize and allows for custom
  functions not defined by base interface. Using base interface allows for
  multiple formats with minimal instruction count. */

namespace wire
{
  //! Interface for converting "wire" (byte) formats to C/C++ objects without a DOM.
  class reader
  {
    std::size_t depth_; //!< Tracks number of recursive objects and arrays

    //! \throw wire::exception if max depth is reached
    void increment_depth();
    //! \throw std::logic_error if already `depth() == 0`.
    void decrement_depth();

    /*! \param min_element_size of each array element in any format - if known.
          Derived types with explicit element count should verify available
          space, and throw a `wire::exception` on issues.
        \throw wire::exception if next value not array
        \throw wire::exception if not enough bytes for all array elements
          (with epee/msgpack which has specified number of elements).
        \return Number of values to read before calling `is_array_end()`. */
    virtual std::size_t do_start_array(std::size_t min_element_size) = 0;

    //! \throw wire::exception if not object begin. \return State to be given to `key(...)` function.
    virtual std::size_t do_start_object() = 0;

  protected:
    epee::span<const std::uint8_t> remaining_; //!< Derived class tracks unprocessed bytes here

    reader(const epee::span<const std::uint8_t> remaining) noexcept
      : depth_(0), remaining_(remaining)
    {}

    reader(const reader&) = delete;
    reader(reader&&) = delete;
    reader& operator=(const reader&) = delete;
    reader& operator=(reader&&) = delete;
 
  public:
    struct key_map
    {
      const char* name;
    };

    //! \return Maximum read depth for both objects and arrays before erroring
    static constexpr std::size_t max_read_depth() noexcept { return 100; }

    //! \return Assume delimited arrays in generic interface (some optimizations disabled)
    static constexpr std::true_type delimited_arrays() noexcept { return {}; }

    virtual ~reader() noexcept
    {}

    //! \return Number of recursive objects and arrays
    std::size_t depth() const noexcept { return depth_; }

    //! \return Unprocessed bytes
    epee::span<const std::uint8_t> remaining() const noexcept { return remaining_; }

    //! \throw wire::exception if parsing is incomplete.
    virtual void check_complete() const = 0;

    //! \throw wire::exception if array, object, or end of stream.
    virtual basic_value basic() = 0;

    //! \throw wire::exception if next value not a boolean.
    virtual bool boolean() = 0;

    //! \throw wire::expception if next value not an integer.
    virtual std::intmax_t integer() = 0;

    //! \throw wire::exception if next value not an unsigned integer.
    virtual std::uintmax_t unsigned_integer() = 0;

    //! \throw wire::exception if next value not number
    virtual double real() = 0;

    //! throw wire::exception if next value not string
    virtual std::string string() = 0;

    /*! Copy upcoming string directly into `dest`.
      \throw wire::exception if next value not string
      \throw wire::exception if next string exceeds `dest.size())`
      \throw wire::exception if `exact == true` and next string is not `dest.size()`
      \return Number of bytes read into `dest`. */
    virtual std::size_t string(epee::span<char> dest, bool exact) = 0;

    // ! \throw wire::exception if next value cannot be read as binary
    virtual epee::byte_slice binary() = 0;

    /*! Copy upcoming binary directly into `dest`.
      \throw wire::exception if next value not binary
      \throw wire::exception if next binary exceeds `dest.size()`
      \throw wire::exception if `exact == true` and next binary is not `dest.size()`.
      \return Number of bytes read into `dest`. */
    virtual std::size_t binary(epee::span<std::uint8_t> dest, const bool exact) = 0;

    /*! \param min_element_size of each array element in any format - if known.
          Derived types with explicit element count should verify available
          space, and throw a `wire::exception` on issues.
        \throw wire::exception if next value not array
        \throw wire::exception if not enough bytes for all array elements
          (with epee/msgpack which has specified number of elements).
        \return Number of values to read before calling `is_array_end()`. */
    std::size_t start_array(std::size_t min_element_size);

    //! \return True if there is another element to read.
    virtual bool is_array_end(std::size_t count) = 0;

    void end_array() { decrement_depth(); }


    //! \throw wire::exception if not object begin. \return State to be given to `key(...)` function.
    std::size_t start_object();

    /*! Read a key of an object field and match against a known list of keys.
       Skips or throws exceptions on unknown fields depending on implementation
       settings.

      \param map of known keys (strings and integer) that are valid.
      \param[in,out] state returned by `start_object()` or `key(...)` whichever
        was last.
      \param[out] index of match found in `map`.

      \throw wire::exception if next value not a key.
      \throw wire::exception if next key not found in `map` and skipping
        fields disabled.

      \return True if this function found a field in `map` to process.
     */
    virtual bool key(epee::span<const key_map> map, std::size_t& state, std::size_t& index) = 0;

    void end_object() { decrement_depth(); }
  };

  template<typename R>
  inline void read_bytes(R& source, bool& dest)
  { dest = source.boolean(); }

  template<typename R>
  inline void read_bytes(R& source, double& dest)
  { dest = source.real(); }

  template<typename R>
  inline void read_bytes(R& source, std::string& dest)
  { dest = source.string(); }

  template<typename R>
  inline void read_bytes(R& source, epee::byte_slice& dest)
  { dest = source.binary(); }

  template<typename R, typename T>
  inline std::enable_if_t<is_blob<T>::value> read_bytes(R& source, T& dest)
  { source.binary(epee::as_mut_byte_span(dest), /*exact=*/ true); }

  //! Use `read_bytes(...)` method if available for `T`.
  template<typename R, typename T>
  inline auto read_bytes(R& source, T& dest) -> decltype(dest.read_bytes(source))
  { return dest.read_bytes(source); }

  namespace integer
  {
    [[noreturn]] void throw_exception(std::intmax_t value, std::intmax_t min, std::intmax_t max);
    [[noreturn]] void throw_exception(std::uintmax_t value, std::uintmax_t max);

    template<typename T, typename U>
    inline T cast_signed(const U source)
    {
      using limit = std::numeric_limits<T>;
      static_assert(
        std::is_signed<T>::value && std::is_integral<T>::value,
        "target must be signed integer type"
      );
      static_assert(
        std::is_signed<U>::value && std::is_integral<U>::value,
        "source must be signed integer type"
      );
      if (source < limit::min() || limit::max() < source)
        throw_exception(source, limit::min(), limit::max());
      return static_cast<T>(source);
    }

    template<typename T, typename U>
    inline T cast_unsigned(const U source)
    {
      using limit = std::numeric_limits<T>;
      static_assert(
        std::is_unsigned<T>::value && std::is_integral<T>::value,
        "target must be unsigned integer type"
      );
      static_assert(
        std::is_unsigned<U>::value && std::is_integral<U>::value,
        "source must be unsigned integer type"
      );
      if (limit::max() < source)
        throw_exception(source, limit::max());
      return static_cast<T>(source);
    }
  } // integer

  //! read all current and future signed integer types
  template<typename R, typename T>
  inline std::enable_if_t<std::is_signed<T>::value && std::is_integral<T>::value>
  read_bytes(R& source, T& dest)
  {
    dest = integer::cast_signed<T>(source.integer());
  }

  //! read all current and future unsigned integer types
  template<typename R, typename T>
  inline std::enable_if_t<std::is_unsigned<T>::value && std::is_integral<T>::value>
  read_bytes(R& source, T& dest)
  {
    dest = integer::cast_unsigned<T>(source.unsigned_integer());
  }
} // wire

namespace wire_read
{
    /*! Don't add a function called `read_bytes` to this namespace, it will
      prevent 2-phase lookup. 2-phase lookup delays the function searching until
      the template is used instead of when its defined. This allows the
      unqualified calls to `read_bytes` in this namespace to "find" user
      functions that are declared after these functions (the technique behind
      `boost::serialization`). */

  [[noreturn]] void throw_exception(wire::error::schema code, const char* display, epee::span<char const* const> name_list);

  template<typename R, typename T>
  inline void bytes(R& source, T&& dest)
  {
    read_bytes(source, dest); // ADL (searches every associated namespace)
  }

  //! Use `source` to store information at `dest`
  template<typename R, typename T, typename U>
  inline std::error_code from_bytes(T&& source, U& dest)
  {
    if (wire::is_optional_root<U>::value && source.empty())
      return {};

    try
    {
      R in{std::forward<T>(source)};
      bytes(in, dest);
      in.check_complete();
    }
    catch (const wire::exception& e)
    {
      return e.code();
    }

    return {};
  }

  // Trap objects that do not have standard insertion functions
  template<typename R, typename... T>
  void array_insert(const R&, const T&...) noexcept
  {
    static_assert(std::is_same<R, void>::value, "type T does not have a valid insertion function");
  }

  // Insert to sorted containers
  template<typename R, typename T, typename V = typename T::value_type>
  inline auto array_insert(R& source, T& dest) -> decltype(dest.emplace_hint(dest.end(), std::declval<V>()), bool(true))
  {
    V val{};
    wire_read::bytes(source, val);
    dest.emplace_hint(dest.end(), std::move(val));
    return true;
  }

  // Insert into unsorted containers
  template<typename R, typename T>
  inline auto array_insert(R& source, T& dest) -> decltype(dest.emplace_back(), dest.back(), bool(true))
  {
    // more efficient to process the object in-place in many cases
    dest.emplace_back();
    wire_read::bytes(source, dest.back());
    return true;
  }

  // no compile-time checks for the array constraints
  template<typename R, typename T>
  inline void array_unchecked(R& source, T& dest, const std::size_t min_element_size, const std::size_t max_element_count)
  {
    using value_type = typename T::value_type;
    static_assert(!std::is_same<value_type, char>::value, "read array of chars as string");
    static_assert(!std::is_same<value_type, std::int8_t>::value, "read array of signed chars as binary");
    static_assert(!std::is_same<value_type, std::uint8_t>::value, "read array of unsigned chars as binary");

    std::size_t count = source.start_array(min_element_size);

    // quick check for epee/msgpack formats
    if (max_element_count < count)
      throw_exception(wire::error::schema::array_max_element, "", nullptr);

    dest.clear();
    wire::reserve(dest, count);

    bool more = count;
    const std::size_t start_bytes = source.remaining().size();
    while (more || !source.is_array_end(count))
    {
      // check for json/cbor formats
      if (source.delimited_arrays() && max_element_count <= dest.size())
        throw_exception(wire::error::schema::array_max_element, "", nullptr);

      wire_read::array_insert(source, dest);
      --count;
      more &= bool(count);

      if (((start_bytes - source.remaining().size()) / dest.size()) < min_element_size)
        throw_exception(wire::error::schema::array_min_size, "", nullptr);
    }

    source.end_array();
  }

  template<typename R, typename T, std::size_t M, std::size_t N = std::numeric_limits<std::size_t>::max()>
  inline void array(R& source, T& dest, wire::min_element_size<M> min_element_size, wire::max_element_count<N> max_element_count = {})
  {
    using value_type = typename T::value_type;
    static_assert(
      min_element_size.template check<value_type>() || max_element_count.template check<value_type>(),
      "array unpacking memory issues"
    );
    // each set of template args generates unique ASM, merge them down
    array_unchecked(source, dest, min_element_size, max_element_count);
  }

  template<typename T>
  inline void reset_field(wire::field_<T, true>& dest)
  {
    // array fields are always optional, see `wire/field.h`
    if (dest.optional_on_empty())
      wire::clear(dest.get_value());
  }

  template<typename T>
  inline void reset_field(wire::field_<T, false>& dest)
  {
    dest.get_value().reset();
  }

  template<typename R, typename T>
  inline void unpack_field(R& source, wire::field_<T, true>& dest)
  {
    bytes(source, dest.get_value());
  }

  template<typename R, typename T>
  inline void unpack_field(R& source, wire::field_<T, false>& dest)
  {
    if (!bool(dest.get_value()))
      dest.get_value().emplace();
    bytes(source, *dest.get_value());
  }

  //! Tracks read status of every object field instance.
  template<typename T>
  class tracker
  {
    T field_;
    std::size_t our_index_;
    bool read_;

  public:
    static constexpr bool is_required() noexcept { return T::is_required(); }
    static constexpr std::size_t count() noexcept { return T::count(); }

    explicit tracker(T field)
      : field_(std::move(field)), our_index_(0), read_(false)
    {}

    //! \return Field name if required and not read, otherwise `nullptr`.
    const char* name_if_missing() const noexcept
    {
      return (is_required() && !read_) ? field_.name : nullptr;
    }

    //! Set all entries in `map` related to this field (expand variant types!).
    template<std::size_t N>
    std::size_t set_mapping(std::size_t index, wire::reader::key_map (&map)[N]) noexcept
    {
      our_index_ = index;
      map[index].name = field_.name;
      return index + count();
    }

    //! Try to read next value if `index` matches `this`. \return 0 if no match, 1 if optional field read, and 2 if required field read
    template<typename R>
    std::size_t try_read(R& source, const std::size_t index)
    {
      if (index < our_index_ || our_index_ + count() <= index)
        return 0;
      if (read_)
        throw_exception(wire::error::schema::invalid_key, "duplicate", {std::addressof(field_.name), 1});

      unpack_field(source, field_);
      read_ = true;
      return 1 + is_required();
    }

    //! Reset optional fields that were skipped
    bool reset_omitted()
    {
      if (!is_required() && !read_)
        reset_field(field_);
      return true;
    }
  };

  // `expand_tracker_map` writes all `tracker` types to a table

  template<std::size_t N>
  inline void expand_tracker_map(std::size_t index, const wire::reader::key_map (&)[N]) noexcept
  {}

  template<std::size_t N, typename T, typename... U>
  inline void expand_tracker_map(std::size_t index, wire::reader::key_map (&map)[N], tracker<T>& head, tracker<U>&... tail) noexcept
  {
    expand_tracker_map(head.set_mapping(index, map), map, tail...);
  }

  template<typename R, typename... T>
  inline void object(R& source, tracker<T>... fields)
  {
    static constexpr const std::size_t total_subfields = wire::sum(fields.count()...);
    static_assert(total_subfields < 100, "algorithm uses too much stack space and linear searching");
    static_assert(sizeof...(T) <= total_subfields, "subfield parameter pack size mismatch");

    std::size_t state = source.start_object();
    std::size_t required = wire::sum(std::size_t(fields.is_required())...);

    wire::reader::key_map map[total_subfields + 1] = {}; // +1 for empty object
    expand_tracker_map(0, map, fields...);

    std::size_t next = 0;
    while (source.key({map, total_subfields}, state, next))
    {
      switch (wire::sum(fields.try_read(source, next)...))
      {
      default:
      case 0:
        throw_exception(wire::error::schema::invalid_key, "bad map setup", nullptr);
        break;
      case 2:
        --required; /* fallthrough */
      case 1:
        break;
      }
    }

    if (required)
    {
      const char* missing[] = {fields.name_if_missing()..., nullptr};
      throw_exception(wire::error::schema::missing_key, "", missing);
    }

    wire::sum(fields.reset_omitted()...);
    source.end_object();
  }
} // wire_read

namespace wire
{
  template<typename R, typename T>
  inline std::enable_if_t<is_array<T>::value> read_bytes(R& source, T& dest)
  {
    static constexpr const std::size_t wire_size =
      default_min_element_size<R, typename T::value_type>::value;
    static_assert(
      wire_size != 0,
      "no sane default array constraints for the reader / value_type pair"
    );

    wire_read::array(source, dest, min_element_size<wire_size>{});
  }

  template<typename R, typename... T>
  inline std::enable_if_t<std::is_base_of<reader, R>::value> object(R& source, T... fields)
  {
    wire_read::object(source, wire_read::tracker<T>{std::move(fields)}...);
  }

  template<typename R, typename... T>
  inline void object_fwd(const std::true_type /*is_read*/, R& source, T&&... fields)
  {
    wire::object(source, std::forward<T>(fields)...);
  }
}
