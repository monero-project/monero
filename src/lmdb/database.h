// Copyright (c) 2018, The Monero Project
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

#include <atomic>
#include <cstddef>
#include <lmdb.h>
#include <memory>
#include <type_traits>

#include "common/expect.h"
#include "lmdb/error.h"
#include "lmdb/transaction.h"

namespace lmdb
{
    //! Closes LMDB environment handle.
    struct close_env
    {
        void operator()(MDB_env* ptr) const noexcept
        {
            if (ptr)
                mdb_env_close(ptr);
        }
    };

    using environment = std::unique_ptr<MDB_env, close_env>;

    //! \return LMDB environment at `path` with a max of `max_dbs` tables.
    expect<environment> open_environment(const char* path, MDB_dbi max_dbs) noexcept;

    //! Context given to LMDB.
    struct context
    {
        std::atomic<std::size_t> active;
        std::atomic_flag lock;
    };

    //! Manages a LMDB environment for safe memory-map resizing. Thread-safe.
    class database
    {
        environment env;
        context ctx;

        //! \return The LMDB environment associated with the object.
        MDB_env* handle() const noexcept { return env.get(); }

        expect<write_txn> do_create_txn(unsigned int flags) noexcept;

    public: 
        database(environment env);

        database(database&&) = delete;
        database(database const&) = delete;

        virtual ~database() noexcept;

        database& operator=(database&&) = delete;
        database& operator=(database const&) = delete;

        /*!
            Resize the memory map for the LMDB environment. Will block until
            all reads/writes on the environment complete.
        */
        expect<void> resize() noexcept;

        //! \return A read only LMDB transaction, reusing `txn` if provided.
        expect<read_txn> create_read_txn(suspended_txn txn = nullptr) noexcept;

        //! \return `txn` after releasing context.
        expect<suspended_txn> reset_txn(read_txn txn) noexcept;

        //! \return A read-write LMDB transaction.
        expect<write_txn> create_write_txn() noexcept;

        //! Commit the read-write transaction.
        expect<void> commit(write_txn txn) noexcept;

        /*!
            Create a write transaction, pass it to `f`, then try to commit
            the write if `f` succeeds.

            \tparam F must be callable with signature `expect<T>(MDB_txn&)`.
            \param f must be re-startable if `lmdb::error(MDB_MAP_FULL)`.

            \return The result of calling `f`.
        */
        template<typename F>
        typename std::result_of<F(MDB_txn&)>::type try_write(F f, unsigned attempts = 3)
        {
            for (unsigned i = 0; i < attempts; ++i)
            {
                expect<write_txn> txn = create_write_txn();
                if (!txn)
                    return txn.error();

                MONERO_PRECOND(*txn != nullptr);
                const auto wrote = f(*(*txn));
                if (wrote)
                {
                    MONERO_CHECK(commit(std::move(*txn)));
                    return wrote;
                }
                if (wrote != lmdb::error(MDB_MAP_FULL))
                    return wrote;

                txn->reset();
                MONERO_CHECK(this->resize());
            }
            return {lmdb::error(MDB_MAP_FULL)};
        }
    };
} // lmdb

