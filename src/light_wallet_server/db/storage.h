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

#include <iosfwd>
#include <list>
#include <memory>
#include <utility>
#include <vector>

#include "common/expect.h"
#include "crypto/crypto.h"
#include "lmdb/transaction.h"
#include "lmdb/key_stream.h"
#include "lmdb/value_stream.h"
#include "light_wallet_server/db/data.h"
#include "light_wallet_server/fwd.h"

namespace lws
{
namespace db
{
    namespace cursor
    {
        MONERO_CURSOR(accounts);
        MONERO_CURSOR(outputs);
        MONERO_CURSOR(spends);
        MONERO_CURSOR(images);
        MONERO_CURSOR(requests);

        MONERO_CURSOR(blocks);
        MONERO_CURSOR(accounts_by_address);
        MONERO_CURSOR(accounts_by_height);
    }

    struct storage_internal;
    struct reader_internal
    {
        cursor::blocks blocks_cur;
        cursor::accounts_by_address accounts_ba_cur;
        cursor::accounts_by_height accounts_bh_cur;
    };

    //! Wrapper for LMDB read access to on-disk storage of light-weight server data.
    class storage_reader
    {
        std::shared_ptr<storage_internal> db;
        lmdb::read_txn txn;
        reader_internal curs;

    public:
        storage_reader(std::shared_ptr<storage_internal> db, lmdb::read_txn txn) noexcept
          : db(std::move(db)), txn(std::move(txn)), curs{}
        {}

        storage_reader(storage_reader&&) = default;
        storage_reader(storage_reader const&) = delete;

        ~storage_reader() noexcept;

        storage_reader& operator=(storage_reader&&) = default;
        storage_reader& operator=(storage_reader const&) = delete;

        //! \return Last known block.
        expect<block_info> get_last_block() noexcept;

        //! \return List for `GetHashesFast` to sync blockchain with daemon.
        expect<std::list<crypto::hash>> get_chain_sync();

        //! \return All registered `account`s.
        expect<lmdb::key_stream<account_status, account, cursor::close_accounts>>
            get_accounts(cursor::accounts cur = nullptr) noexcept;

        //! \return All `account`s currently in `status` or `lmdb::error(MDB_NOT_FOUND)`.
        expect<lmdb::value_stream<account, cursor::close_accounts>>
            get_accounts(account_status status, cursor::accounts cur = nullptr) noexcept;

        //! \return Info related to `address` or `lmdb::error(MDB_NOT_FOUND)`.
        expect<std::pair<account_status, account>>
            get_account(account_address const& address, cursor::accounts& cur) noexcept;

        expect<std::pair<account_status, account>>
        get_account(account_address const& address) noexcept
        {
            cursor::accounts cur;
            return get_account(address, cur);
        }

        //! \return All outputs received by `id`.
        expect<lmdb::value_stream<output, cursor::close_outputs>>
            get_outputs(account_id id, cursor::outputs cur = nullptr) noexcept;

        //! \return All potential spends by `id`.
        expect<lmdb::value_stream<spend, cursor::close_spends>>
            get_spends(account_id id, cursor::spends cur = nullptr) noexcept;

        //! \return All key images associated with `id`.
        expect<lmdb::value_stream<db::key_image, cursor::close_images>>
            get_images(output_id id, cursor::images cur = nullptr) noexcept;

        //! \return All `request_info`s.
        expect<lmdb::key_stream<request, request_info, cursor::close_requests>>
            get_requests(cursor::requests cur = nullptr) noexcept;

        //! \return A specific request from `address` of `type`.
        expect<request_info>
            get_request(request type, account_address const& address, cursor::requests cur = nullptr) noexcept;

        //! Dump the contents of the database in JSON format to `out`.
        expect<void> json_debug(std::ostream& out, bool show_keys);

        //! \return Read txn that can be re-used via `storage::start_read`.
        lmdb::suspended_txn finish_read() noexcept;
    };

    //! Wrapper for LMDB on-disk storage of light-weight server data.
    class storage
    {
        std::shared_ptr<storage_internal> db;

        storage(std::shared_ptr<storage_internal> db) noexcept
          : db(std::move(db))
        {}

    public:
        /*!
            Open a light_wallet_server LDMB database.

            \param path Directory for LMDB storage
            \param create_queue_max Maximum number of create account requests allowed.
 
            \throw std::system_error on any LMDB error (all treated as fatal).
            \throw std::bad_alloc If `std::shared_ptr` fails to allocate.

            \return A ready light-wallet server database.
        */
        static storage open(const char* path, unsigned create_queue_max);

        storage(storage&&) = default;
        storage(storage const&) = delete;

        ~storage() noexcept;

        storage& operator=(storage&&) = default;
        storage& operator=(storage const&) = delete;

        //! \return A copy of the LMDB environment, but not reusable txn/cursors.
        storage clone() const noexcept;

        //! Rollback chain and accounts to `height`.
        expect<void> rollback(block_id height);

        /*!
            Sync the local blockchain with a remote version. Pops user txes
            if reorg detected.

            \param height The height of the element in `hashes`
            \param hashes List of blockchain hashes starting at `height`.

            \return True if the local blockchain is correctly synced.
        */
        expect<void> sync_chain(block_id height, epee::span<const crypto::hash> hashes);

        //! Bump the last access time of `address` to the current time.
        expect<void> update_access_time(account_address const& address) noexcept;

        //! Change state of `address` to `status`. \return Updated `addresses`.
        expect<std::vector<account_address>>
            change_status(account_status status, epee::span<const account_address> addresses);


        //! Add an account, for immediate inclusion in the active list.
        expect<void> add_account(account_address const& address, crypto::secret_key const& key) noexcept;

        //! Reset `addresses` to `height` for scanning.
        expect<std::vector<account_address>>
            rescan(block_id height, epee::span<const account_address> addresses);

        //! Add an account for later approval. For use with the login endpoint.
        expect<void> creation_request(account_address const& address, crypto::secret_key const& key, account_flags flags) noexcept;

        /*!
            Request lock height of an existing account. No effect if the
            `start_height` is already older.
        */
        expect<void> import_request(account_address const& address, block_id height) noexcept;

        //! Accept requests by `addresses` of type `req`. \return Accepted addresses.
        expect<std::vector<account_address>>
            accept_requests(request req, epee::span<const account_address> addresses);

        //! Reject requests by `addresses` of type `req`. \return Rejected addresses.
        expect<std::vector<account_address>>
            reject_requests(request req, epee::span<const account_address> addresses);

        /*!
            Updates the status of user accounts, even if inactive or hidden.
            Duplicate receives or spends provided in `accts` are silently
            ignored. If a gap in `height` vs the stored account record is
            detected, the entire update will fail.

            \param height The first hash in `chain` is at this height.
            \param chain List of block hashes that `accts` were scanned against.
            \param accts Updated to `height + chain.size()` scan height.

            \return True iff LMDB successfully committed the update.
        */
        expect<std::size_t> update(
            block_id height, epee::span<const crypto::hash> chain, epee::span<const lws::account> accts
        );

        //! `txn` must have come from a previous call on the same thread.
        expect<storage_reader> start_read(lmdb::suspended_txn txn = nullptr) const;
    };
} // db
} // lws
