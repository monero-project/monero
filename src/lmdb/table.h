#pragma once

#include <utility>

#include "common/expect.h"
#include "lmdb/error.h"
#include "lmdb/key_stream.h"
#include "lmdb/util.h"
#include "lmdb/value_stream.h"

namespace lmdb
{
    //! Helper for grouping typical LMDB DBI options.
    struct table
    {
        char const* const name;
        const unsigned flags;
        MDB_cmp_func* const key_cmp;
        MDB_cmp_func* const value_cmp;

        //! \pre `name != nullptr` \return Open table.
        expect<MDB_dbi> open(MDB_txn& write_txn) const noexcept;
    };

    //! Helper for grouping typical LMDB DBI options when key and value are fixed types.
    template<typename K, typename V>
    struct basic_table : table
    {
        using key_type = K;
        using value_type = V;

        //! \return Additional LMDB flags based on `flags` value.
        static constexpr unsigned compute_flags(const unsigned flags) noexcept
        {
            return flags | ((flags & MDB_DUPSORT) ? MDB_DUPFIXED : 0);
        }

        constexpr explicit basic_table(const char* name, unsigned flags = 0, MDB_cmp_func value_cmp = nullptr) noexcept
          : table{name, compute_flags(flags), &lmdb::less<lmdb::native_type<K>>, value_cmp}
        {}

        /*!
            \tparam U must be same as `V`; used for sanity checking.
            \tparam F is the type within `U` that is being extracted.
            \tparam offset to `F` within `U`.

            \note If using `F` and `offset` to retrieve a specific field, use
                `MONERO_FIELD` macro in `src/lmdb/util.h` which calculates the
                offset automatically.

            \return Value of type `F` at `offset` within `value` which has
                type `U`.
        */
        template<typename U, typename F = U, std::size_t offset = 0>
        static expect<F> get_value(MDB_val value) noexcept
        {
            static_assert(std::is_same<U, V>(), "bad MONERO_FIELD?");
            static_assert(std::is_pod<F>(), "F must be POD");
            static_assert(sizeof(F) + offset <= sizeof(U), "bad field type and/or offset");

            if (value.mv_size != sizeof(U))
                return {lmdb::error(MDB_BAD_VALSIZE)};

            F out;
            std::memcpy(std::addressof(out), static_cast<char*>(value.mv_data) + offset, sizeof(out));
            return out;
        }

        /*!
            \pre `cur != nullptr`.
            \param cur Active cursor on table. Returned in object on success,
                otherwise destroyed.
            \return A handle to the first key/value in the table linked
                to `cur` or an empty `key_stream`.
        */
        template<typename D>
        expect<key_stream<K, V, D>>
        static get_key_stream(std::unique_ptr<MDB_cursor, D> cur) noexcept
        {
            MONERO_PRECOND(cur != nullptr);

            MDB_val key;
            MDB_val value;
            const int err = mdb_cursor_get(cur.get(), &key, &value, MDB_FIRST);
            if (err)
            {
                if (err != MDB_NOTFOUND)
                    return {lmdb::error(err)};
                cur.reset(); // return empty set
            }
            return key_stream<K, V, D>{std::move(cur)};
        }

        /*!
            \pre `cur != nullptr`.
            \param cur Active cursor on table. Returned in object on success,
                otherwise destroyed.
            \return A handle to the first value at `key` in the table linked
                to `cur` or an empty `value_stream`.
        */
        template<typename D>
        expect<value_stream<V, D>>
        static get_value_stream(K const& key, std::unique_ptr<MDB_cursor, D> cur) noexcept
        {
            MONERO_PRECOND(cur != nullptr);

            MDB_val key_bytes = lmdb::to_val(key);
            MDB_val value;
            const int err = mdb_cursor_get(cur.get(), &key_bytes, &value, MDB_SET);
            if (err)
            {
                if (err != MDB_NOTFOUND)
                    return {lmdb::error(err)};
                cur.reset(); // return empty set
            }
            return value_stream<V, D>{std::move(cur)};
        }
    };
} // lmdb

