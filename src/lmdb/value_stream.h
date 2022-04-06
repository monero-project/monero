// Copyright (c) 2018-2022, The Monero Project

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

#include <boost/range/iterator_range.hpp>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <lmdb.h>
#include <utility>

#include "span.h"

namespace lmdb
{
    namespace stream
    {
        /*
            \throw std::system_error if unexpected LMDB error.
            \return 0 if `cur == nullptr`, otherwise count of values at current key.
        */
        mdb_size_t count(MDB_cursor* cur);

        /*!
            Calls `mdb_cursor_get` and does some error checking.

            \param cur is given to `mdb_cursor_get` without modification.
            \param op is passed to `mdb_cursor_get` without modification.
            \param key expected key size or 0 to skip key size check.
            \param value expected value size or 0 to skip value size check.

            \throw std::system_error if `key != 0` and `key_.mv_size != key`.
            \throw std::system_error if `value != 0` and `value_.mv_size != value`.
            \throw std::system_error if `mdb_cursor_get` returns any error
                other than `MDB_NOTFOUND`.

            \return {key bytes, value bytes} or two empty spans if `MDB_NOTFOUND`.
        */
        std::pair<epee::span<const std::uint8_t>, epee::span<const std::uint8_t>>
            get(MDB_cursor& cur, MDB_cursor_op op, std::size_t key, std::size_t value);
    }

    /*!
        An InputIterator for a fixed-sized LMDB value at a specific key.

        \tparam T The value type at the specific key.
        \tparam F The value type being returned when dereferenced.
        \tparam offset to `F` within `T`.

        \note This meets requirements for an InputIterator only. The iterator
            can only be incremented and dereferenced. All copies of an iterator
            share the same LMDB cursor, and therefore incrementing any copy will
            change the cursor state for all (incrementing an iterator will
            invalidate all prior copies of the iterator). Usage is identical
            to `std::istream_iterator`.
    */
    template<typename T, typename F = T, std::size_t offset = 0>
    class value_iterator
    {
        MDB_cursor* cur;
        epee::span<const std::uint8_t> values;
        
        void increment()
        {
            values.remove_prefix(sizeof(T));
            if (values.empty() && cur)
                values = lmdb::stream::get(*cur, MDB_NEXT_DUP, 0, sizeof(T)).second;
        }

    public:
        using value_type = F;
        using reference = value_type;
        using pointer = void;
        using difference_type = std::size_t;
        using iterator_category = std::input_iterator_tag;

        //! Construct an "end" iterator.
        value_iterator() noexcept
          : cur(nullptr), values()
        {}

        /*!
            \param cur Iterate over values starting at this cursor position.
            \throw std::system_error if unexpected LMDB error. This can happen
                if `cur` is invalid.
        */
        value_iterator(MDB_cursor* cur)
          : cur(cur), values()
        {
            if (cur)
                values = lmdb::stream::get(*cur, MDB_GET_CURRENT, 0, sizeof(T)).second;
        }

        value_iterator(value_iterator const&) = default;
        ~value_iterator() = default;
        value_iterator& operator=(value_iterator const&) = default;

        //! \return True if `this` is one-past the last value.
        bool is_end() const noexcept { return values.empty(); }

        //! \return True iff `rhs` is referencing `this` value.
        bool equal(value_iterator const& rhs) const noexcept
        {
            return
                (values.empty() && rhs.values.empty()) ||
                values.data() == rhs.values.data();
        }

        //! Invalidates all prior copies of the iterator.
        value_iterator& operator++()
        {
            increment();
            return *this;
        }

        //! \return A copy that is already invalidated, ignore
        value_iterator operator++(int)
        {
            value_iterator out{*this};
            increment();
            return out;
        }

        /*!
            Get a specific field within `F`. Default behavior is to return
            the entirety of `U`, despite the filtering logic of `operator*`.

            \pre `!is_end()`

            \tparam U must match `T`, used for `MONERO_FIELD` sanity checking.
            \tparam G field type to extract from the value
            \tparam uoffset to `G` type, or `0` when `std::is_same<U, G>()`.

            \return The field `G`, at `uoffset` within `U`.
        */
        template<typename U, typename G = U, std::size_t uoffset = 0>
        G get_value() const noexcept
        {
            static_assert(std::is_same<U, T>(), "bad MONERO_FIELD usage?");
            static_assert(std::is_pod<U>(), "value type must be pod");
            static_assert(std::is_pod<G>(), "field type must be pod");
            static_assert(sizeof(G) + uoffset <= sizeof(U), "bad field and/or offset");
            assert(sizeof(G) + uoffset <= values.size());
            assert(!is_end());

            G value;
            std::memcpy(std::addressof(value), values.data() + uoffset, sizeof(value));
            return value;
        }

        //! \pre `!is_end()` \return The field `F`, at `offset`, within `T`.
        value_type operator*() const noexcept { return get_value<T, F, offset>(); }
    };
 
    /*!
        C++ wrapper for a LMDB read-only cursor on a fixed-sized value `T`.

        \tparam T value type being stored by each record.
        \tparam D cleanup functor for the cursor; usually unique per db/table.
    */
    template<typename T, typename D>
    class value_stream
    {
        std::unique_ptr<MDB_cursor, D> cur;
    public:

        //! Take ownership of `cur` without changing position. `nullptr` valid.
        explicit value_stream(std::unique_ptr<MDB_cursor, D> cur)
          : cur(std::move(cur))
        {}

        value_stream(value_stream&&) = default;
        value_stream(value_stream const&) = delete;
        ~value_stream() = default;
        value_stream& operator=(value_stream&&) = default;
        value_stream& operator=(value_stream const&) = delete;

        /*!
            Give up ownership of the cursor. `count()`, `make_iterator()` and
            `make_range()` can still be invoked, but return the empty set.

            \return Currently owned LMDB cursor.
        */
        std::unique_ptr<MDB_cursor, D> give_cursor() noexcept
        {
            return {std::move(cur)};
        }

        /*!
            Place the stream back at the first value. Newly created iterators
            will start at the first value again.

            \note Invalidates all current iterators from `this`, including
                those created with `make_iterator` or `make_range`.
        */
        void reset()
        {
            if (cur)
                lmdb::stream::get(*cur, MDB_FIRST_DUP, 0, 0);
        }

        /*!
            \throw std::system_error if LMDB has unexpected errors.
            \return Number of values at this key.
        */
        std::size_t count() const
        {
            return lmdb::stream::count(cur.get());
        }

        /*!
            Return a C++ iterator over database values from current cursor
            position that will reach `.is_end()` after the last duplicate key
            record. Calling `make_iterator()` will return an iterator whose
            `operator*` will return entire value (`T`).
            `make_iterator<MONERO_FIELD(account, id)>()` will return an
            iterator whose `operator*` will return a `decltype(account.id)`
            object - the other fields in the struct `account` are never copied
            from the database.

            \throw std::system_error if LMDB has unexpected errors.
            \return C++ iterator starting at current cursor position.
        */
        template<typename U = T, typename F = U, std::size_t offset = 0>
        value_iterator<U, F, offset> make_iterator() const
        {
            static_assert(std::is_same<U, T>(), "was MONERO_FIELD used with wrong type?");
            return {cur.get()};
        }

        /*!
            Return a range from current cursor position until last duplicate
            key record. Useful in for-each range loops or in templated code
            expecting a range of elements. Calling `make_range()` will return
            a range of `T` objects. `make_range<MONERO_FIELD(account, id)>()`
            will return a range of `decltype(account.id)` objects - the other
            fields in the struct `account` are never copied from the database.

            \throw std::system_error if LMDB has unexpected errors.
            \return An InputIterator range over values at cursor position.
        */
        template<typename U = T, typename F = U, std::size_t offset = 0>
        boost::iterator_range<value_iterator<U, F, offset>> make_range() const
        {
            return {make_iterator<U, F, offset>(), value_iterator<U, F, offset>{}};
        }
    };

    template<typename T, typename F, std::size_t offset>
    inline
    bool operator==(value_iterator<T, F, offset> const& lhs, value_iterator<T, F, offset> const& rhs) noexcept
    {
        return lhs.equal(rhs);
    }

    template<typename T, typename F, std::size_t offset>
    inline
    bool operator!=(value_iterator<T, F, offset> const& lhs, value_iterator<T, F, offset> const& rhs) noexcept
    {
        return !lhs.equal(rhs);
    }
} // lmdb

