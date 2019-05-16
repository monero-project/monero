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
#include "value_stream.h"

#include <stdexcept>

#include "common/expect.h"
#include "lmdb/error.h"
#include "lmdb/util.h"

namespace lmdb
{
    namespace stream
    {
        mdb_size_t count(MDB_cursor* cur)
        {
            mdb_size_t out = 0;
            if (cur)
            {
                const int rc = mdb_cursor_count(cur, &out);
                if (rc)
                    MONERO_THROW(lmdb::error(rc), "mdb_cursor_count");
            }
            return out;
        }

        std::pair<epee::span<const std::uint8_t>, epee::span<const std::uint8_t>>
        get(MDB_cursor& cur, MDB_cursor_op op, std::size_t key, std::size_t value)
        {
            MDB_val key_bytes{};
            MDB_val value_bytes{};
            const int rc = mdb_cursor_get(&cur, &key_bytes, &value_bytes, op);
            if (rc)
            {
                if (rc == MDB_NOTFOUND)
                    return {};
                MONERO_THROW(lmdb::error(rc), "mdb_cursor_get");
            }

            if (key && key != key_bytes.mv_size)
                MONERO_THROW(lmdb::error(MDB_BAD_VALSIZE), "mdb_cursor_get key");

            if (value && (value_bytes.mv_size % value != 0 || value_bytes.mv_size == 0))
                MONERO_THROW(lmdb::error(MDB_BAD_VALSIZE), "mdb_cursor_get value");

            return {lmdb::to_byte_span(key_bytes), lmdb::to_byte_span(value_bytes)};
        }
    }
}

