// Copyright (c) 2014-2018, The Monero Project
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
#include "database.h"
#include "lmdb/error.h"
#include "lmdb/util.h"

#ifdef _WIN32
namespace
{
    constexpr const mdb_mode_t open_flags = 0;
}
#else
#include <sys/stat.h>

namespace
{
    constexpr const mdb_mode_t open_flags = (S_IRUSR | S_IWUSR);
}
#endif

namespace lmdb
{
    namespace
    {
        constexpr const mdb_size_t max_resize = 1 * 1024 * 1024 * 1024; // 1 GB
        void acquire_context(context& ctx) noexcept
        {
            while (ctx.lock.test_and_set());
            ++(ctx.active);
            ctx.lock.clear();
        }

        void release_context(context& ctx) noexcept
        {
            --(ctx.active);
        }
    }

    void release_read_txn::operator()(MDB_txn* ptr) const noexcept
    {
        if (ptr)
        {
            MDB_env* const env = mdb_txn_env(ptr);
            abort_txn{}(ptr);
            if (env)
            {
                context* ctx = reinterpret_cast<context*>(mdb_env_get_userctx(env));
                if (ctx)
                    release_context(*ctx);
            }
        }
    }

    expect<environment> open_environment(const char* path, MDB_dbi max_dbs) noexcept
    {
        MONERO_PRECOND(path != nullptr);

        MDB_env* obj = nullptr;
        MONERO_LMDB_CHECK(mdb_env_create(std::addressof(obj)));
        environment out{obj};

        MONERO_LMDB_CHECK(mdb_env_set_maxdbs(out.get(), max_dbs));
        MONERO_LMDB_CHECK(mdb_env_open(out.get(), path, 0, open_flags));
        return {std::move(out)};
    }

    expect<write_txn> database::do_create_txn(unsigned int flags) noexcept
    {
        MONERO_PRECOND(handle() != nullptr);

        for (unsigned attempts = 0; attempts < 3; ++attempts)
        {
            acquire_context(ctx);

            MDB_txn* txn = nullptr;
            const int err =
                mdb_txn_begin(handle(), nullptr, flags, &txn);
            if (!err && txn != nullptr)
                return write_txn{txn};

            release_context(ctx);
            if (err != MDB_MAP_RESIZED)
                return {lmdb::error(err)};
            MONERO_CHECK(this->resize());
        }
        return {lmdb::error(MDB_MAP_RESIZED)};
    }

    database::database(environment env)
      : env(std::move(env)), ctx{{}, ATOMIC_FLAG_INIT}
    {
        if (handle())
        {
            const int err = mdb_env_set_userctx(handle(), std::addressof(ctx));
            if (err)
                MONERO_THROW(lmdb::error(err), "Failed to set user context");
        }
    }

    database::~database() noexcept
    {
        while (ctx.active);
    }

    expect<void> database::resize() noexcept
    {
        MONERO_PRECOND(handle() != nullptr);

        while (ctx.lock.test_and_set());
        while (ctx.active);

        MDB_envinfo info{};
        MONERO_LMDB_CHECK(mdb_env_info(handle(), &info));

        const mdb_size_t resize = std::min(info.me_mapsize, max_resize);
        const int err = mdb_env_set_mapsize(handle(), info.me_mapsize + resize);
        ctx.lock.clear();
        if (err)
            return {lmdb::error(err)};
        return success();
    }

    expect<read_txn> database::create_read_txn(suspended_txn txn) noexcept
    {
        if (txn)
        {
            acquire_context(ctx);
            const int err = mdb_txn_renew(txn.get());
            if (err)
            {
                release_context(ctx);
                return {lmdb::error(err)};
            }
            return read_txn{txn.release()};
        }
        auto new_txn = do_create_txn(MDB_RDONLY);
        if (new_txn)
            return read_txn{new_txn->release()};
        return new_txn.error();
    }

    expect<suspended_txn> database::reset_txn(read_txn txn) noexcept
    {
        MONERO_PRECOND(txn != nullptr);
        mdb_txn_reset(txn.get());
        release_context(ctx);
        return suspended_txn{txn.release()};
    }

    expect<write_txn> database::create_write_txn() noexcept
    {
        return do_create_txn(0);
    }

    expect<void> database::commit(write_txn txn) noexcept
    {
        MONERO_PRECOND(txn != nullptr);
        MONERO_LMDB_CHECK(mdb_txn_commit(txn.get()));
        txn.release();
        release_context(ctx);
        return success();
    }
} // lmdb
