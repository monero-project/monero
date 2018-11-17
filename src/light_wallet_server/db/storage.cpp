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
#include "storage.h"

#include <boost/container/static_vector.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/iterator_range.hpp>
#include <cassert>
#include <chrono>
#include <limits>
#include <string>
#include <utility>

#include "checkpoints/checkpoints.h"
#include "crypto/crypto.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_core/cryptonote_tx_utils.h"
#include "hex.h"
#include "light_wallet_server/config.h"
#include "light_wallet_server/error.h"
#include "light_wallet_server/db/account.h"
#include "light_wallet_server/db/json.h"
#include "light_wallet_server/db/string.h"
#include "lmdb/database.h"
#include "lmdb/error.h"
#include "lmdb/key_stream.h"
#include "lmdb/table.h"
#include "lmdb/util.h"
#include "lmdb/value_stream.h"
#include "serialization/new/json_output.h"
#include "span.h"

namespace lws
{
namespace db
{
    namespace
    {
        //! Used for finding `account` instances by other indexes.
        struct account_lookup
        {
            account_id id;
            account_status status;
            char reserved[3];
        };
        static_assert(sizeof(account_lookup) == 4 + 1 + 3, "padding in account_lookup");

        //! Used for looking up accounts by their public address.
        struct account_by_address
        {
            account_address address; //!< Must be first for LMDB optimizations
            account_lookup lookup;
        };
        static_assert(sizeof(account_by_address) == 64 + 4 + 1 + 3, "padding in account_by_address");

        constexpr const unsigned blocks_version = 0;
        constexpr const unsigned by_address_version = 0;

        template<typename T>
        int less(epee::span<const std::uint8_t> left, epee::span<const std::uint8_t> right) noexcept
        {
            if (left.size() < sizeof(T))
            {
                assert(left.empty());
                return -1;
            }
            if (right.size() < sizeof(T))
            {
                assert(right.empty());
                return 1;
            }

            T left_val;
            T right_val;
            std::memcpy(std::addressof(left_val), left.data(), sizeof(T));
            std::memcpy(std::addressof(right_val), right.data(), sizeof(T));

            return (left_val < right_val) ? -1 : int(right_val < left_val);
        }

        int compare_32bytes(epee::span<const std::uint8_t> left, epee::span<const std::uint8_t> right) noexcept
        {
            if (left.size() < 32)
            {
                assert(left.empty());
                return -1;
            }
            if (right.size() < 32)
            {
                assert(right.empty());
                return 1;
            }

            return std::memcmp(left.data(), right.data(), 32);
        }

        int output_compare(MDB_val const* left, MDB_val const* right) noexcept
        {
            if (left == nullptr || right == nullptr)
            {
                assert("MDB_val nullptr" == 0);
                return -1;
            }

            auto left_bytes = lmdb::to_byte_span(*left);
            auto right_bytes = lmdb::to_byte_span(*right);

            int diff = less<lmdb::native_type<block_id>>(left_bytes, right_bytes);
            if (diff)
                return diff;

            left_bytes.remove_prefix(sizeof(block_id));
            right_bytes.remove_prefix(sizeof(block_id));

            static_assert(sizeof(crypto::hash) == 32, "bad memcmp below");
            diff = compare_32bytes(left_bytes, right_bytes);
            if (diff)
                return diff;

            left_bytes.remove_prefix(sizeof(crypto::hash));
            right_bytes.remove_prefix(sizeof(crypto::hash));
            return less<output_id>(left_bytes, right_bytes);
        }

        int spend_compare(MDB_val const* left, MDB_val const* right) noexcept
        {
            if (left == nullptr || right == nullptr)
            {
                assert("MDB_val nullptr" == 0);
                return -1;
            }

            auto left_bytes = lmdb::to_byte_span(*left);
            auto right_bytes = lmdb::to_byte_span(*right);

            int diff = less<lmdb::native_type<block_id>>(left_bytes, right_bytes);
            if (diff)
                return diff;

            left_bytes.remove_prefix(sizeof(block_id));
            right_bytes.remove_prefix(sizeof(block_id));

            static_assert(sizeof(crypto::hash) == 32, "bad memcmp below");
            diff = compare_32bytes(left_bytes, right_bytes);
            if (diff)
                return diff;

            left_bytes.remove_prefix(sizeof(crypto::hash));
            right_bytes.remove_prefix(sizeof(crypto::hash));

            static_assert(sizeof(crypto::key_image) == 32, "bad memcmp below");
            return compare_32bytes(left_bytes, right_bytes);
        }

        constexpr const lmdb::basic_table<unsigned, block_info> blocks{
            "blocks_by_id", (MDB_CREATE | MDB_DUPSORT), MONERO_SORT_BY(block_info, id)
        };
        constexpr const lmdb::basic_table<account_status, account> accounts{
            "accounts_by_status,id", (MDB_CREATE | MDB_DUPSORT), MONERO_SORT_BY(account, id)
        };
        constexpr const lmdb::basic_table<unsigned, account_by_address> accounts_by_address(
            "accounts_by_address", (MDB_CREATE | MDB_DUPSORT), MONERO_COMPARE(account_by_address, address.spend_public)
        );
        constexpr const lmdb::basic_table<block_id, account_lookup> accounts_by_height(
            "accounts_by_height,id", (MDB_CREATE | MDB_DUPSORT), MONERO_SORT_BY(account_lookup, id)
        );
        constexpr const lmdb::basic_table<account_id, output> outputs{
            "outputs_by_account_id,block_id,tx_hash,output_id", (MDB_CREATE | MDB_DUPSORT), &output_compare
        };
        constexpr const lmdb::basic_table<account_id, spend> spends{
            "spends_by_account_id,block_id,tx_hash,image", (MDB_CREATE | MDB_DUPSORT), &spend_compare
        };
        constexpr const lmdb::basic_table<output_id, db::key_image> images{
            "key_images_by_output_id,image", (MDB_CREATE | MDB_DUPSORT), MONERO_COMPARE(db::key_image, value)
        };
        constexpr const lmdb::basic_table<request, request_info> requests{
            "requests_by_type,address", (MDB_CREATE | MDB_DUPSORT), MONERO_COMPARE(request_info, address.spend_public)
        };

        template<typename D>
        expect<void> check_cursor(MDB_txn& txn, MDB_dbi tbl, std::unique_ptr<MDB_cursor, D>& cur) noexcept
        {
            if (cur)
            {
                MONERO_LMDB_CHECK(mdb_cursor_renew(&txn, cur.get()));
            }
            else
            {
                auto new_cur = lmdb::open_cursor<D>(txn, tbl);
                if (!new_cur)
                    return new_cur.error();
                cur = std::move(*new_cur);
            }
            return success();
        }

        template<typename K, typename V>
        expect<void> bulk_insert(MDB_cursor& cur, K const& key, epee::span<V> values) noexcept
        {
            while (!values.empty())
            {
                void const* const data = reinterpret_cast<void const*>(values.data());
                MDB_val key_bytes = lmdb::to_val(key);
                MDB_val value_bytes[2] = {
                    MDB_val{sizeof(V), const_cast<void*>(data)}, MDB_val{values.size(), nullptr}
                };

                int err = mdb_cursor_put(
                    &cur, &key_bytes, value_bytes, (MDB_NODUPDATA | MDB_MULTIPLE)
                );
                if (err && err != MDB_KEYEXIST)
                    return {lmdb::error(err)};

                values.remove_prefix(value_bytes[1].mv_size + (err == MDB_KEYEXIST ? 1 : 0));
            }
            return success();
        }

        //! \return a single instance of compiled-in checkpoints for lws
        cryptonote::checkpoints const& get_checkpoints()
        {
            struct initializer
            {
                cryptonote::checkpoints data;

                initializer()
                  : data()
                {
                    data.init_default_checkpoints(lws::config::network);

                    std::string const* genesis_tx = nullptr;
                    std::uint32_t genesis_nonce = 0;
 
                    switch (lws::config::network)
                    {
                        case cryptonote::TESTNET:
                            genesis_tx = std::addressof(::config::testnet::GENESIS_TX);
                            genesis_nonce = ::config::testnet::GENESIS_NONCE;
                            break;

                        case cryptonote::STAGENET:
                            genesis_tx = std::addressof(::config::stagenet::GENESIS_TX);
                            genesis_nonce = ::config::stagenet::GENESIS_NONCE;
                            break;

                        case cryptonote::MAINNET:
                            genesis_tx = std::addressof(::config::GENESIS_TX);
                            genesis_nonce = ::config::GENESIS_NONCE;
                            break;

                        default:
                            MONERO_THROW(lws::error::bad_blockchain, "Unsupported net type");
                    }
                    cryptonote::block b;
                    cryptonote::generate_genesis_block(b, *genesis_tx, genesis_nonce);
                    crypto::hash block_hash = cryptonote::get_block_hash(b);
                    if (!data.add_checkpoint(0, epee::to_hex::string(epee::as_byte_span(block_hash))))
                        MONERO_THROW(lws::error::bad_blockchain, "Genesis tx and checkpoints file mismatch");
                }
            };
            static const initializer instance;
            return instance.data;
        }

        //! \return Current block hash at `id` using `cur`.
        expect<crypto::hash> get_block_hash(MDB_cursor& cur, block_id id) noexcept
        {
            MDB_val key = lmdb::to_val(blocks_version);
            MDB_val value = lmdb::to_val(id);
            MONERO_LMDB_CHECK(mdb_cursor_get(&cur, &key, &value, MDB_GET_BOTH));
            return blocks.get_value<MONERO_FIELD(block_info, hash)>(value);
        }

        void check_blockchain(MDB_txn& txn, MDB_dbi tbl)
        {
            cursor::blocks cur = MONERO_UNWRAP(lmdb::open_cursor<cursor::close_blocks>(txn, tbl));

            std::map<std::uint64_t, crypto::hash> const& points =
                get_checkpoints().get_points();

            if (points.empty() || points.begin()->first != 0)
                MONERO_THROW(lws::error::bad_blockchain, "Checkpoints are empty/expected genesis hash");

            MDB_val key = lmdb::to_val(blocks_version);
            int err = mdb_cursor_get(cur.get(), &key, nullptr, MDB_SET);
            if (err)
            {
                if (err != MDB_NOTFOUND)
                    MONERO_THROW(lmdb::error(err), "Unable to retrieve blockchain hashes");

                // new database
                block_info checkpoint{
                    block_id(points.begin()->first), points.begin()->second
                };

                MDB_val value = lmdb::to_val(checkpoint);
                err = mdb_cursor_put(cur.get(), &key, &value, MDB_NODUPDATA);
                if (err)
                    MONERO_THROW(lmdb::error(err), "Unable to add hash to local blockchain");

                if (1 < points.size())
                {
                    checkpoint = block_info{
                        block_id(points.rbegin()->first), points.rbegin()->second
                    };

                    value = lmdb::to_val(checkpoint);
                    err = mdb_cursor_put(cur.get(), &key, &value, MDB_NODUPDATA);
                    if (err)
                        MONERO_THROW(lmdb::error(err), "Unable to add hash to local blockchain");
                }
            }
            else // inspect existing database
            {
                ///
                /// TODO Trim blockchain after a checkpoint has been reached
                ///
                const crypto::hash genesis = MONERO_UNWRAP(get_block_hash(*cur, block_id(0)));
                if (genesis != points.begin()->second)
                {
                    MONERO_THROW(
                        lws::error::bad_blockchain, "Genesis hash mismatch"
                    );
                }
            }
        }

        template<typename T>
        expect<T> get_blocks(MDB_cursor& cur, std::size_t max_internal)
        {
            T out{};

            max_internal = std::min(std::size_t(64), max_internal);
            out.reserve(12 + max_internal);

            MDB_val key = lmdb::to_val(blocks_version);
            MDB_val value{};
            MONERO_LMDB_CHECK(mdb_cursor_get(&cur, &key, &value, MDB_SET));
            MONERO_LMDB_CHECK(mdb_cursor_get(&cur, &key, &value, MDB_LAST_DUP));
            for (unsigned i = 0; i < 10; ++i)
            {
                expect<block_info> next = blocks.get_value<block_info>(value);
                if (!next)
                    return next.error();

                out.push_back(std::move(*next));

                const int err = mdb_cursor_get(&cur, &key, &value, MDB_PREV_DUP);
                if (err)
                {
                    if (err != MDB_NOTFOUND)
                        return {lmdb::error(err)};
                    if (out.back().id != block_id(0))
                        return {lws::error::bad_blockchain};
                    return out;
                }
            }

            const auto add_block = [&cur, &out] (std::uint64_t id) -> expect<void>
            {
                expect<crypto::hash> next = get_block_hash(cur, block_id(id));
                if (!next)
                    return next.error();
                out.push_back(block_info{block_id(id), std::move(*next)});
                return success();
            };

            const std::uint64_t checkpoint = get_checkpoints().get_max_height();
            const std::uint64_t anchor = lmdb::to_native(out.back().id);

            for (unsigned i = 1; i <= max_internal; ++i)
            {
                const std::uint64_t offset = 2 << i;
                if (anchor < offset || anchor - offset < checkpoint)
                    break;
                MONERO_CHECK(add_block(anchor - offset));
            }

            if (block_id(checkpoint) < out.back().id)
                MONERO_CHECK(add_block(checkpoint));
            if (out.back().id != block_id(0))
                MONERO_CHECK(add_block(0));
            return out;
        }

        expect<account_id> find_last_id(MDB_cursor& cur) noexcept
        {
            account_id best = account_id(0);

            MDB_val key{};
            MDB_val value{};

            int err = mdb_cursor_get(&cur, &key, &value, MDB_FIRST);
            if (err == MDB_NOTFOUND)
                return best;
            if (err)
                return {lmdb::error(err)};

            do
            {
                MONERO_LMDB_CHECK(mdb_cursor_get(&cur, &key, &value, MDB_LAST_DUP));
                const expect<account_id> current = 
                    accounts.get_value<MONERO_FIELD(account, id)>(value);
                if (!current)
                    return current.error();


                best = std::max(best, *current);
                err = mdb_cursor_get(&cur, &key, &value, MDB_NEXT_NODUP);
                if (err == MDB_NOTFOUND)
                    return best;
            } while (err == 0);
            return {lmdb::error(err)};
        }
    } // anonymous

    struct storage_internal : lmdb::database
    {
        struct tables_
        {
            MDB_dbi blocks;
            MDB_dbi accounts;
            MDB_dbi accounts_ba;
            MDB_dbi accounts_bh;
            MDB_dbi outputs;
            MDB_dbi spends;
            MDB_dbi images;
            MDB_dbi requests;
        } tables;

        const unsigned create_queue_max;

        explicit storage_internal(lmdb::environment env, unsigned create_queue_max)
          : lmdb::database(std::move(env)), tables{}, create_queue_max(create_queue_max)
        {
            lmdb::write_txn txn = this->create_write_txn().value();
            assert(txn != nullptr);

            tables.blocks      = blocks.open(*txn).value();
            tables.accounts    = accounts.open(*txn).value();
            tables.accounts_ba = accounts_by_address.open(*txn).value();
            tables.accounts_bh = accounts_by_height.open(*txn).value();
            tables.outputs     = outputs.open(*txn).value();
            tables.spends      = spends.open(*txn).value();
            tables.images      = images.open(*txn).value();
            tables.requests    = requests.open(*txn).value();

            check_blockchain(*txn, tables.blocks);

            MONERO_UNWRAP(this->commit(std::move(txn)));
        }
    };

    storage_reader::~storage_reader() noexcept
    {}

    expect<block_info> storage_reader::get_last_block() noexcept
    {
        MONERO_PRECOND(txn != nullptr);
        assert(db != nullptr);
        MONERO_CHECK(check_cursor(*txn, db->tables.blocks, curs.blocks_cur));

        MDB_val key = lmdb::to_val(blocks_version);
        MDB_val value{};
        MONERO_LMDB_CHECK(mdb_cursor_get(curs.blocks_cur.get(), &key, &value, MDB_SET));
        MONERO_LMDB_CHECK(mdb_cursor_get(curs.blocks_cur.get(), &key, &value, MDB_LAST_DUP));

        return blocks.get_value<block_info>(value);
    }

    expect<std::list<crypto::hash>> storage_reader::get_chain_sync()
    {
        MONERO_PRECOND(txn != nullptr);
        assert(db != nullptr);
        MONERO_CHECK(check_cursor(*txn, db->tables.blocks, curs.blocks_cur));
        auto blocks = get_blocks<std::vector<block_info>>(*curs.blocks_cur, 64);
        if (!blocks)
            return blocks.error();

        std::list<crypto::hash> out{};
        for (block_info const& block : *blocks)
            out.push_back(block.hash);
        return out;
    }

    expect<lmdb::key_stream<account_status, account, cursor::close_accounts>>
    storage_reader::get_accounts(cursor::accounts cur) noexcept
    {
        MONERO_PRECOND(txn != nullptr);
        assert(db != nullptr); // both are moved in pairs
        MONERO_CHECK(check_cursor(*txn, db->tables.accounts, cur));
        return accounts.get_key_stream(std::move(cur));
    }

    expect<lmdb::value_stream<account, cursor::close_accounts>>
    storage_reader::get_accounts(account_status status, cursor::accounts cur) noexcept
    {
        MONERO_PRECOND(txn != nullptr);
        assert(db != nullptr); // both are moved in pairs
        MONERO_CHECK(check_cursor(*txn, db->tables.accounts, cur));
        return accounts.get_value_stream(status, std::move(cur));
    }

    expect<std::pair<account_status, account>>
    storage_reader::get_account(account_address const& address, cursor::accounts& cur) noexcept
    {
        MONERO_PRECOND(txn != nullptr);
        assert(db != nullptr);

        MONERO_CHECK(check_cursor(*txn, db->tables.accounts_ba, curs.accounts_ba_cur));

        MDB_val key = lmdb::to_val(by_address_version);
        MDB_val value = lmdb::to_val(address);
        const int err = mdb_cursor_get(curs.accounts_ba_cur.get(), &key, &value, MDB_GET_BOTH);
        if (err)
        {
            if (err == MDB_NOTFOUND)
                return {lws::error::account_not_found};
            return {lmdb::error(err)};
        }

        const expect<account_lookup> lookup =
            accounts_by_address.get_value<MONERO_FIELD(account_by_address, lookup)>(value);
        if (!lookup)
            return lookup.error();

        MONERO_CHECK(check_cursor(*txn, db->tables.accounts, cur));
        assert(cur != nullptr);

        key = lmdb::to_val(lookup->status);
        value = lmdb::to_val(lookup->id);
        MONERO_LMDB_CHECK(mdb_cursor_get(cur.get(), &key, &value, MDB_GET_BOTH));

        const expect<account> user = accounts.get_value<account>(value);
        if (!user)
            return user.error();
        return {{lookup->status, *user}};
    }

    expect<lmdb::value_stream<output, cursor::close_outputs>>
    storage_reader::get_outputs(account_id id, cursor::outputs cur) noexcept
    {
        MONERO_PRECOND(txn != nullptr);
        assert(db != nullptr);
        MONERO_CHECK(check_cursor(*txn, db->tables.outputs, cur));
        return outputs.get_value_stream(id, std::move(cur));
    }

    expect<lmdb::value_stream<spend, cursor::close_spends>>
    storage_reader::get_spends(account_id id, cursor::spends cur) noexcept
    {
        MONERO_PRECOND(txn != nullptr);
        assert(db != nullptr);
        MONERO_CHECK(check_cursor(*txn, db->tables.spends, cur));
        return spends.get_value_stream(id, std::move(cur));
    }

    expect<lmdb::value_stream<db::key_image, cursor::close_images>>
    storage_reader::get_images(output_id id, cursor::images cur) noexcept
    {
        MONERO_PRECOND(txn != nullptr);
        assert(db != nullptr);
        MONERO_CHECK(check_cursor(*txn, db->tables.images, cur));
        return images.get_value_stream(id, std::move(cur));
    }

    expect<lmdb::key_stream<request, request_info, cursor::close_requests>>
    storage_reader::get_requests(cursor::requests cur) noexcept
    {
        MONERO_PRECOND(txn != nullptr);
        assert(db != nullptr);
        MONERO_CHECK(check_cursor(*txn, db->tables.requests, cur));
        return requests.get_key_stream(std::move(cur));
    }

    expect<request_info>
    storage_reader::get_request(request type, account_address const& address, cursor::requests cur) noexcept
    {
        MONERO_PRECOND(txn != nullptr);
        assert(db != nullptr);

        MONERO_CHECK(check_cursor(*txn, db->tables.requests, cur));

        MDB_val key = lmdb::to_val(type);
        MDB_val value = lmdb::to_val(address);
        MONERO_LMDB_CHECK(mdb_cursor_get(cur.get(), &key, &value, MDB_GET_BOTH));
        return requests.get_value<request_info>(value);
    }
 
    namespace
    {
        struct from_address_
        {
            expect<std::string> operator()(account_address const& addr) const
            {
                return address_string(addr);
            }
        };
        constexpr const from_address_ from_address{};

        //! Converts integers/enums to `std::string` in `::json::map`.
        struct from_integer_
        {
            template<typename T>
            expect<std::string> operator()(T value) const
            {
                using native = lmdb::native_type<T>;
                static_assert(std::is_integral<native>(), "expected integer");
                return std::to_string(native(value));
            }
        };
        constexpr const from_integer_ from_integer{};

        struct from_output_id_
        {
            expect<std::string> operator()(output_id id) const
            {
                return std::to_string(id.high) + ":" + std::to_string(id.low);
            }
        };
        constexpr const from_output_id_ from_output_id{};
    } // anonymous

    expect<void> storage_reader::json_debug(std::ostream& out, bool show_keys)
    {
        using boost::adaptors::reverse;
        using boost::adaptors::transform;

        MONERO_PRECOND(txn != nullptr);
        assert(db != nullptr);

        struct json_account_lookup
        {
            expect<void> operator()(std::ostream& dest, account_lookup const& src) const
            {
                static constexpr const auto fmt = ::json::object(
                    ::json::field("id", ::json::uint32),
                    ::json::field("status", json::status)
                );
                return fmt(dest, src.id, src.status);
            }
        };

        struct json_at_height
        {
            using input_type = std::pair<
                block_id, boost::iterator_range<lmdb::value_iterator<account_lookup>>
            >;

            expect<void> operator()(std::ostream& dest, input_type const src) const
            {
                static constexpr const auto fmt = ::json::object(
                    ::json::field("scan_height", ::json::uint64),
                    ::json::field("accounts", ::json::array(json_account_lookup{}))
                );
                return fmt(dest, src.first, src.second);
            }
        };

        const auto json_accounts = ::json::map(
            ::json::from_enum(db::status_string), ::json::array(db::json::account_{show_keys})
        );
        static constexpr const auto json_by_address = ::json::map(
            from_address, json_account_lookup{}
        );
        static constexpr const auto json_by_height = ::json::array(json_at_height{});
        static constexpr const auto json_outputs = ::json::map(
            from_integer, ::json::array(db::json::output)
        );
        static constexpr const auto json_spends = ::json::map(
            from_integer, ::json::array(db::json::spend)
        );
        static constexpr const auto json_images = ::json::map(
            from_output_id, ::json::array(db::json::key_image)
        );
        static constexpr const auto json_requests = ::json::map(
            ::json::from_enum(db::request_string), ::json::array(db::json::request_info)
        );

        const auto fmt = ::json::object(
            ::json::field(blocks.name, ::json::array(db::json::block_info)),
            ::json::field(accounts.name, json_accounts),
            ::json::field(accounts_by_address.name, json_by_address),
            ::json::field(accounts_by_height.name, json_by_height),
            ::json::field(outputs.name, json_outputs),
            ::json::field(spends.name, json_spends),
            ::json::field(images.name, json_images),
            ::json::field(requests.name, json_requests)
        );

        const auto address_as_key = [](account_by_address const& src)
        {
            return std::make_pair(src.address, src.lookup);
        };

        cursor::accounts accounts_cur;
        cursor::outputs outputs_cur;
        cursor::spends spends_cur;
        cursor::images images_cur;
        cursor::requests requests_cur;

        MONERO_CHECK(check_cursor(*txn, db->tables.blocks, curs.blocks_cur));
        MONERO_CHECK(check_cursor(*txn, db->tables.accounts, accounts_cur));
        MONERO_CHECK(check_cursor(*txn, db->tables.accounts_ba, curs.accounts_ba_cur));
        MONERO_CHECK(check_cursor(*txn, db->tables.accounts_bh, curs.accounts_bh_cur));
        MONERO_CHECK(check_cursor(*txn, db->tables.outputs, outputs_cur));
        MONERO_CHECK(check_cursor(*txn, db->tables.spends, spends_cur));
        MONERO_CHECK(check_cursor(*txn, db->tables.images, images_cur));
        MONERO_CHECK(check_cursor(*txn, db->tables.requests, requests_cur));

        auto blocks_partial =
            get_blocks<boost::container::static_vector<block_info, 12>>(*curs.blocks_cur, 0);
        if (!blocks_partial)
            return blocks_partial.error();

        auto accounts_stream = accounts.get_key_stream(std::move(accounts_cur));
        if (!accounts_stream)
            return accounts_stream.error();

        auto accounts_ba_stream = accounts_by_address.get_value_stream(
            by_address_version, std::move(curs.accounts_ba_cur)
        );
        if (!accounts_ba_stream)
            return accounts_ba_stream.error();

        auto accounts_bh_stream = accounts_by_height.get_key_stream(
            std::move(curs.accounts_bh_cur)
        );
        if (!accounts_bh_stream)
            return accounts_bh_stream.error();

        auto outputs_stream = outputs.get_key_stream(std::move(outputs_cur));
        if (!outputs_stream)
            return outputs_stream.error();

        auto spends_stream = spends.get_key_stream(std::move(spends_cur));
        if (!spends_stream)
            return spends_stream.error();

        auto images_stream = images.get_key_stream(std::move(images_cur));
        if (!images_stream)
            return images_stream.error();

        auto requests_stream = requests.get_key_stream(std::move(requests_cur));
        if (!requests_stream)
            return requests_stream.error();

        const expect<void> wrote = fmt(
            out,
            reverse(*blocks_partial),
            accounts_stream->make_range(),
            transform(accounts_ba_stream->make_range(), address_as_key),
            accounts_bh_stream->make_range(),
            outputs_stream->make_range(),
            spends_stream->make_range(),
            images_stream->make_range(),
            requests_stream->make_range()
        );

        curs.accounts_ba_cur = accounts_ba_stream->give_cursor();
        curs.accounts_bh_cur = accounts_bh_stream->give_cursor();

        if (!wrote)
            out.setstate(std::ios::failbit);
        else if (!out.good())
            return {std::io_errc::stream};
        return wrote;
    }

    lmdb::suspended_txn storage_reader::finish_read() noexcept
    {
        if (txn != nullptr)
        {
            assert(db != nullptr);
            auto suspended = db->reset_txn(std::move(txn));
            if (suspended) // errors not currently logged
                return {std::move(*suspended)};
        }
        return nullptr;
    }

    storage storage::open(const char* path, unsigned create_queue_max)
    {
        return {
            std::make_shared<storage_internal>(
                MONERO_UNWRAP(lmdb::open_environment(path, 20)), create_queue_max
            )
        };
    }

    storage::~storage() noexcept
    {}

    storage storage::clone() const noexcept
    {
        return storage{db};
    }

    expect<storage_reader> storage::start_read(lmdb::suspended_txn txn) const
    {
        MONERO_PRECOND(db != nullptr);

        expect<lmdb::read_txn> reader = db->create_read_txn(std::move(txn));
        if (!reader)
            return reader.error();

        assert(*reader != nullptr);
        return storage_reader{db, std::move(*reader)};
    }

    namespace // sub functions for `sync_chain(...)`
    {
        expect<void>
        rollback_spends(account_id user, block_id height, MDB_cursor& spends_cur, MDB_cursor& images_cur) noexcept
        {
            MDB_val key = lmdb::to_val(user);
            MDB_val value = lmdb::to_val(height);

            const int err = mdb_cursor_get(&spends_cur, &key, &value, MDB_GET_BOTH_RANGE);
            if (err == MDB_NOTFOUND)
                return success();
            if (err)
                return {lmdb::error(err)};

            for (;;)
            {
                const expect<output_id> out = spends.get_value<MONERO_FIELD(spend, source)>(value);
                if (!out)
                    return out.error();

                const expect<crypto::key_image> image =
                    spends.get_value<MONERO_FIELD(spend, image)>(value);
                if (!image)
                    return image.error();

                key = lmdb::to_val(*out);
                value = lmdb::to_val(*image);
                MONERO_LMDB_CHECK(mdb_cursor_get(&images_cur, &key, &value, MDB_GET_BOTH));
                MONERO_LMDB_CHECK(mdb_cursor_del(&images_cur, 0));

                MONERO_LMDB_CHECK(mdb_cursor_del(&spends_cur, 0));
                const int err = mdb_cursor_get(&spends_cur, &key, &value, MDB_NEXT_DUP);
                if (err == MDB_NOTFOUND)
                    break;
                if (err)
                    return {lmdb::error(err)};
            }
            return success();
        }

        expect<void>
        rollback_outputs(account_id user, block_id height, MDB_cursor& outputs_cur) noexcept
        {
            MDB_val key = lmdb::to_val(user);
            MDB_val value = lmdb::to_val(height);

            const int err = mdb_cursor_get(&outputs_cur, &key, &value, MDB_GET_BOTH_RANGE);
            if (err == MDB_NOTFOUND)
                return success();
            if (err)
                return {lmdb::error(err)};

            for (;;)
            {
                MONERO_LMDB_CHECK(mdb_cursor_del(&outputs_cur, 0));
                const int err = mdb_cursor_get(&outputs_cur, &key, &value, MDB_NEXT_DUP);
                if (err == MDB_NOTFOUND)
                    break;
                if (err)
                    return {lmdb::error(err)};
            }
            return success();
        }

        expect<void> rollback_accounts(storage_internal::tables_ const& tables, MDB_txn& txn, block_id height)
        {
            cursor::accounts_by_height accounts_bh_cur;
            MONERO_CHECK(check_cursor(txn, tables.accounts_bh, accounts_bh_cur));

            MDB_val key = lmdb::to_val(height);
            MDB_val value{};
            const int err = mdb_cursor_get(accounts_bh_cur.get(), &key, &value, MDB_SET_RANGE);
            if (err == MDB_NOTFOUND)
                return success();
            if (err)
                return {lmdb::error(err)};

            std::vector<account_lookup> new_by_heights{};

            cursor::accounts accounts_cur;
            cursor::outputs outputs_cur;
            cursor::spends spends_cur;
            cursor::images images_cur;

            MONERO_CHECK(check_cursor(txn, tables.accounts, accounts_cur));
            MONERO_CHECK(check_cursor(txn, tables.outputs, outputs_cur));
            MONERO_CHECK(check_cursor(txn, tables.spends, spends_cur));
            MONERO_CHECK(check_cursor(txn, tables.images, images_cur));

            const std::uint64_t new_height = std::uint64_t(std::max(height, block_id(1))) - 1;

            // rollback accounts
            for (;;)
            {
                const expect<account_lookup> lookup =
                    accounts_by_height.get_value<account_lookup>(value);
                if (!lookup)
                    return lookup.error();

                key = lmdb::to_val(lookup->status);
                value = lmdb::to_val(lookup->id);

                MONERO_LMDB_CHECK(mdb_cursor_get(accounts_cur.get(), &key, &value, MDB_GET_BOTH));
                expect<account> user = accounts.get_value<account>(value);
                if (!user)
                    return user.error();

                user->scan_height = block_id(new_height);
                user->start_height = std::min(user->scan_height, user->start_height);

                value = lmdb::to_val(*user);
                MONERO_LMDB_CHECK(mdb_cursor_put(accounts_cur.get(), &key, &value, MDB_CURRENT));

                new_by_heights.push_back(account_lookup{user->id, lookup->status});
                MONERO_CHECK(rollback_outputs(user->id, height, *outputs_cur));
                MONERO_CHECK(rollback_spends(user->id, height, *spends_cur, *images_cur));

                MONERO_LMDB_CHECK(mdb_cursor_del(accounts_bh_cur.get(), 0));
                int err = mdb_cursor_get(accounts_bh_cur.get(), &key, &value, MDB_NEXT_DUP);
                if (err == MDB_NOTFOUND)
                {
                    err = mdb_cursor_get(accounts_bh_cur.get(), &key, &value, MDB_NEXT_NODUP);
                    if (err == MDB_NOTFOUND)
                        break;
                }
                if (err)
                    return {lmdb::error(err)};
            }

            return bulk_insert(*accounts_bh_cur, new_height, epee::to_span(new_by_heights));
        }

        expect<void> rollback_chain(storage_internal::tables_ const& tables, MDB_txn& txn, MDB_cursor& cur, block_id height)
        {
            MDB_val key;
            MDB_val value;

            // rollback chain
            int err = 0;
            do
            {
                MONERO_LMDB_CHECK(mdb_cursor_del(&cur, 0));
                err = mdb_cursor_get(&cur, &key, &value, MDB_NEXT_DUP);
            } while (err == 0);

            if (err != MDB_NOTFOUND)
                return {lmdb::error(err)};

            return rollback_accounts(tables, txn,  height);
        }

        template<typename T>
        expect<void> append_block_hashes(MDB_cursor& cur, db::block_id first, T const& chain)
        {
            std::uint64_t height = std::uint64_t(first);
            boost::container::static_vector<block_info, 25> hashes{};
            static_assert(sizeof(hashes) <= 1024, "using more stack space than expected");

            for (auto current = chain.begin() ;; ++current)
            {
                if (current == chain.end() || hashes.size() == hashes.capacity())
                {
                    MONERO_CHECK(bulk_insert(cur, blocks_version, epee::to_span(hashes)));
                    if (current == chain.end())
                        return success();
                    hashes.clear();
                }

                hashes.push_back(block_info{db::block_id(height), *current});
                ++height;
            }
        }
    } // anonymous

    expect<void> storage::rollback(block_id height)
    {
        MONERO_PRECOND(db != nullptr);

        return db->try_write([this, height] (MDB_txn& txn) -> expect<void>
        {
            cursor::blocks blocks_cur;
            MONERO_CHECK(check_cursor(txn, this->db->tables.blocks, blocks_cur));

            MDB_val key = lmdb::to_val(blocks_version);
            MDB_val value = lmdb::to_val(height);
            const int err = mdb_cursor_get(blocks_cur.get(), &key, &value, MDB_GET_BOTH);
            if (err == MDB_NOTFOUND)
                return success();
            if (err)
                return {lmdb::error(err)};

            return rollback_chain(this->db->tables, txn, *blocks_cur, height);
        });
    }

    expect<void> storage::sync_chain(block_id height, epee::span<const crypto::hash> hashes)
    {
        MONERO_PRECOND(!hashes.empty());
        MONERO_PRECOND(db != nullptr);

        return db->try_write([this, height, hashes] (MDB_txn& txn) -> expect<void>
        {
            cursor::blocks blocks_cur;
            MONERO_CHECK(check_cursor(txn, this->db->tables.blocks, blocks_cur));

            expect<crypto::hash> hash = get_block_hash(*blocks_cur, height);
            if (!hash)
                return hash.error();

            // the first entry should always match on in the DB
            if (*hash != *(hashes.begin()))
                return {lws::error::bad_blockchain};

            MDB_val key{};
            MDB_val value{};

            std::uint64_t current = std::uint64_t(height) + 1;
            auto first = hashes.begin();
            auto chain = boost::make_iterator_range(++first, hashes.end());
            for ( ; !chain.empty(); chain.advance_begin(1), ++current)
            {
                const int err = mdb_cursor_get(blocks_cur.get(), &key, &value, MDB_NEXT_DUP);
                if (err == MDB_NOTFOUND)
                    break;
                if (err)
                    return {lmdb::error(err)};

                hash = blocks.get_value<MONERO_FIELD(block_info, hash)>(value);
                if (!hash)
                    return hash.error();

                if (*hash != chain.front())
                {
                    MONERO_CHECK(rollback_chain(this->db->tables, txn, *blocks_cur, db::block_id(current)));
                    break;
                }
            }
            return append_block_hashes(*blocks_cur, db::block_id(current), chain);
        });
    }

    namespace
    {
        expect<db::account_time> get_account_time() noexcept
        {
            const auto time = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()
            );

            if (time.count() < 0)
                return {lws::error::system_clock_invalid_range};
            if (std::numeric_limits<lmdb::native_type<db::account_time>>::max() < time.count())
                return {lws::error::system_clock_invalid_range};
            return db::account_time(time.count());
        }
    }

    expect<void> storage::update_access_time(account_address const& address) noexcept
    {
        MONERO_PRECOND(db != nullptr);
        return db->try_write([this, &address] (MDB_txn& txn) -> expect<void>
        {
            const expect<db::account_time> current_time = get_account_time();
            if (!current_time)
                return current_time.error();

            cursor::accounts accounts_cur;
            cursor::accounts accounts_ba_cur;
            MONERO_CHECK(check_cursor(txn, this->db->tables.accounts, accounts_cur));
            MONERO_CHECK(check_cursor(txn, this->db->tables.accounts_ba, accounts_ba_cur));

            MDB_val key = lmdb::to_val(by_address_version);
            MDB_val value = lmdb::to_val(address);
            const int err = mdb_cursor_get(accounts_ba_cur.get(), &key, &value, MDB_GET_BOTH);

            if (err == MDB_NOTFOUND)
                return {lws::error::account_not_found};
            if (err)
                return {lmdb::error(err)};

            const expect<account_lookup> lookup =
                accounts_by_address.get_value<MONERO_FIELD(account_by_address, lookup)>(value);
            if (!lookup)
                return lookup.error();

            key = lmdb::to_val(lookup->status);
            value = lmdb::to_val(lookup->id);
            MONERO_LMDB_CHECK(mdb_cursor_get(accounts_cur.get(), &key, &value, MDB_GET_BOTH));

            expect<account> user = accounts.get_value<account>(value);
            if (!user)
                return user.error();

            user->access = *current_time;
            value = lmdb::to_val(*user);
            MONERO_LMDB_CHECK(mdb_cursor_put(accounts_cur.get(), &key, &value, MDB_CURRENT));
            return success();
        });
    }

    expect<std::vector<account_address>>
    storage::change_status(account_status status , epee::span<const account_address> addresses)
    {
        MONERO_PRECOND(db != nullptr);
        return db->try_write([this, status, addresses] (MDB_txn& txn) -> expect<std::vector<account_address>>
        {
            std::vector<account_address> changed{};
            changed.reserve(addresses.size());

            cursor::accounts accounts_cur;
            cursor::accounts accounts_ba_cur;
            cursor::accounts accounts_bh_cur;
            MONERO_CHECK(check_cursor(txn, this->db->tables.accounts, accounts_cur));
            MONERO_CHECK(check_cursor(txn, this->db->tables.accounts_ba, accounts_ba_cur));
            MONERO_CHECK(check_cursor(txn, this->db->tables.accounts_bh, accounts_bh_cur));

            for (account_address const& address : addresses)
            {
                MDB_val key = lmdb::to_val(by_address_version);
                MDB_val value = lmdb::to_val(address);
                const int err = mdb_cursor_get(accounts_ba_cur.get(), &key, &value, MDB_GET_BOTH);

                if (err == MDB_NOTFOUND)
                    continue;
                if (err)
                    return {lmdb::error(err)};

                expect<account_by_address> by_address =
                    accounts_by_address.get_value<account_by_address>(value);
                if (!by_address)
                    return by_address.error();

                const account_status current = by_address->lookup.status;
                if (current != status)
                {
                    by_address->lookup.status = status;

                    value = lmdb::to_val(*by_address);
                    MONERO_LMDB_CHECK(mdb_cursor_put(accounts_ba_cur.get(), &key, &value, MDB_CURRENT));

                    key = lmdb::to_val(current);
                    value = lmdb::to_val(by_address->lookup.id);
                    MONERO_LMDB_CHECK(mdb_cursor_get(accounts_cur.get(), &key, &value, MDB_GET_BOTH));

                    expect<account> user = accounts.get_value<account>(value);
                    if (!user)
                        return user.error();

                    MONERO_LMDB_CHECK(mdb_cursor_del(accounts_cur.get(), 0));

                    key = lmdb::to_val(status);
                    value = lmdb::to_val(*user);
                    MONERO_LMDB_CHECK(mdb_cursor_put(accounts_cur.get(), &key, &value, MDB_NODUPDATA));

                    key = lmdb::to_val(user->scan_height);
                    value = lmdb::to_val(user->id);
                    MONERO_LMDB_CHECK(mdb_cursor_get(accounts_bh_cur.get(), &key, &value, MDB_GET_BOTH));

                    value = lmdb::to_val(by_address->lookup);
                    MONERO_LMDB_CHECK(mdb_cursor_put(accounts_bh_cur.get(), &key, &value, MDB_CURRENT));
                }

                changed.push_back(address);
            }

            return changed;
        });
    }

    namespace
    {
        expect<void> do_add_account(MDB_cursor& accounts_cur, MDB_cursor& accounts_ba_cur, MDB_cursor& accounts_bh_cur, account const& user) noexcept
        {
            {
                crypto::secret_key copy{};
                crypto::public_key verify{};
                static_assert(sizeof(copy) == sizeof(user.key), "bad memcpy");
                std::memcpy(
                    std::addressof(unwrap(copy)), std::addressof(user.key), sizeof(copy)
                );

                if (!crypto::secret_key_to_public_key(copy, verify))
                    return {lws::error::bad_view_key};

                if (verify != user.address.view_public)
                    return {lws::error::bad_view_key};
            }

            const account_by_address by_address{
                user.address, {user.id, account_status::active}
            };

            MDB_val key = lmdb::to_val(by_address_version);
            MDB_val value = lmdb::to_val(by_address);
            const int err = mdb_cursor_put(&accounts_ba_cur, &key, &value, MDB_NODUPDATA);

            if (err == MDB_KEYEXIST)
                return {lws::error::account_exists};
            if (err)
                return {lmdb::error(err)};

            key = lmdb::to_val(user.scan_height);
            value = lmdb::to_val(by_address.lookup);
            MONERO_LMDB_CHECK(
                mdb_cursor_put(&accounts_bh_cur, &key, &value, MDB_NODUPDATA)
            );

            key = lmdb::to_val(by_address.lookup.status);
            value = lmdb::to_val(user);
            MONERO_LMDB_CHECK(
                mdb_cursor_put(&accounts_cur, &key, &value, MDB_NODUPDATA)
            );
            return success();
        }
    } // anonymous

    expect<void> storage::add_account(account_address const& address, crypto::secret_key const& key) noexcept
    {
        MONERO_PRECOND(db != nullptr);
        return db->try_write([this, &address, &key] (MDB_txn& txn) -> expect<void>
        {
            const expect<db::account_time> current_time = get_account_time();
            if (!current_time)
                return current_time.error();

            cursor::blocks blocks_cur;
            cursor::accounts accounts_cur;
            cursor::accounts_by_address accounts_ba_cur;
            cursor::accounts_by_height accounts_bh_cur;

            MONERO_CHECK(check_cursor(txn, this->db->tables.blocks, blocks_cur));
            MONERO_CHECK(check_cursor(txn, this->db->tables.accounts, accounts_cur));
            MONERO_CHECK(check_cursor(txn, this->db->tables.accounts_ba, accounts_ba_cur));
            MONERO_CHECK(check_cursor(txn, this->db->tables.accounts_bh, accounts_bh_cur));

            const expect<account_id> last_id = find_last_id(*accounts_cur);
            if (!last_id)
                return last_id.error();

            MDB_val keyv = lmdb::to_val(blocks_version);
            MDB_val value{};

            MONERO_LMDB_CHECK(mdb_cursor_get(blocks_cur.get(), &keyv, &value, MDB_SET));
            MONERO_LMDB_CHECK(mdb_cursor_get(blocks_cur.get(), &keyv, &value, MDB_LAST_DUP));

            const expect<block_id> height =
                blocks.get_value<MONERO_FIELD(block_info, id)>(value);
            if (!height)
                return height.error();

            const account_id next_id = account_id(lmdb::to_native(*last_id) + 1);
            account user{};
            user.id = next_id;
            user.address = address;
            static_assert(sizeof(user.key) == sizeof(key), "bad memcpy");
            std::memcpy(std::addressof(user.key), std::addressof(key), sizeof(key));
            user.start_height = *height;
            user.scan_height = *height;
            user.access = *current_time;
            user.creation = *current_time;
            // ... leave flags set to zero ...

            return do_add_account(
                *accounts_cur, *accounts_ba_cur, *accounts_bh_cur, user
            );
        });
    }

    namespace
    {
        //! \return Success, even if `address` was not found (designed for
        expect<void>
        change_height(MDB_cursor& accounts_cur, MDB_cursor& accounts_ba_cur, MDB_cursor& accounts_bh_cur, block_id height, account_address const& address)
        {
            MDB_val key = lmdb::to_val(by_address_version);
            MDB_val value = lmdb::to_val(address);
            const int err = mdb_cursor_get(&accounts_ba_cur, &key, &value, MDB_GET_BOTH);
            if (err == MDB_NOTFOUND)
                return {lws::error::account_not_found};
            if (err)
                return {lmdb::error(err)};

            const expect<account_lookup> lookup =
                accounts_by_address.get_value<MONERO_FIELD(account_by_address, lookup)>(value);
            if (!lookup)
                return lookup.error();

            key = lmdb::to_val(lookup->status);
            value = lmdb::to_val(lookup->id);
            MONERO_LMDB_CHECK(
                mdb_cursor_get(&accounts_cur, &key, &value, MDB_GET_BOTH)
            );

            expect<account> user = accounts.get_value<account>(value);
            if (!user)
                return user.error();

            const block_id current_height = user->scan_height;
            user->scan_height = std::min(height, user->scan_height);
            user->start_height = std::min(height, user->start_height);

            value = lmdb::to_val(*user);
            MONERO_LMDB_CHECK(
                mdb_cursor_put(&accounts_cur, &key, &value, MDB_CURRENT)
            );

            key = lmdb::to_val(current_height);
            MONERO_LMDB_CHECK(
                mdb_cursor_get(&accounts_bh_cur, &key, &value, MDB_GET_BOTH)
            );
            MONERO_LMDB_CHECK(mdb_cursor_del(&accounts_bh_cur, 0));

            key = lmdb::to_val(height);
            value = lmdb::to_val(*lookup);
            MONERO_LMDB_CHECK(
                mdb_cursor_put(&accounts_bh_cur, &key, &value, MDB_NODUPDATA)
            );

            return success();
        }
    }

    expect<std::vector<account_address>>
    storage::rescan(db::block_id height, epee::span<const account_address> addresses)
    {
        MONERO_PRECOND(db != nullptr);
        return db->try_write([this, height, addresses] (MDB_txn& txn) -> expect<std::vector<account_address>>
        {
            std::vector<account_address> updated{};
            updated.reserve(addresses.size());

            cursor::accounts accounts_cur;
            cursor::accounts_by_address accounts_ba_cur;
            cursor::accounts_by_height accounts_bh_cur;

            MONERO_CHECK(check_cursor(txn, this->db->tables.accounts, accounts_cur));
            MONERO_CHECK(check_cursor(txn, this->db->tables.accounts_ba, accounts_ba_cur));
            MONERO_CHECK(check_cursor(txn, this->db->tables.accounts_bh, accounts_bh_cur));

            for (account_address const& address : addresses)
            {
                const expect<void> changed = change_height(
                    *accounts_cur, *accounts_ba_cur, *accounts_bh_cur, height, address
                );
                if (changed)
                    updated.push_back(address);
                else if (changed != lws::error::account_not_found)
                    return changed.error();
            }
            return updated;
        });
    }

    expect<void> storage::creation_request(account_address const& address, crypto::secret_key const& key, account_flags flags) noexcept
    {
        MONERO_PRECOND(db != nullptr);

        if (!db->create_queue_max)
            return {lws::error::create_queue_max};

        return db->try_write([this, &address, &key, flags] (MDB_txn& txn) -> expect<void>
        {
            const expect<db::account_time> current_time = get_account_time();
            if (!current_time)
                return current_time.error();

            cursor::accounts_by_address accounts_ba_cur;
            cursor::blocks blocks_cur;
            cursor::accounts requests_cur;

            MONERO_CHECK(check_cursor(txn, this->db->tables.accounts_ba, accounts_ba_cur));
            MONERO_CHECK(check_cursor(txn, this->db->tables.blocks, blocks_cur));
            MONERO_CHECK(check_cursor(txn, this->db->tables.requests, requests_cur));

            MDB_val keyv = lmdb::to_val(by_address_version);
            MDB_val value = lmdb::to_val(address);

            int err = mdb_cursor_get(accounts_ba_cur.get(), &keyv, &value, MDB_GET_BOTH);
            if (err != MDB_NOTFOUND)
            {
                if (err)
                    return {lmdb::error(err)};
                return {lws::error::account_exists};
            }

            const request req = request::create;
            keyv = lmdb::to_val(req);
            value = MDB_val{};
            err = mdb_cursor_get(requests_cur.get(), &keyv, &value, MDB_SET);
            if (!err)
            {
                mdb_size_t count = 0;
                MONERO_LMDB_CHECK(mdb_cursor_count(requests_cur.get(), &count));
                if (this->db->create_queue_max <= count)
                    return {lws::error::create_queue_max};
            }
            else if (err != MDB_NOTFOUND)
                return {lmdb::error(err)};

            keyv = lmdb::to_val(blocks_version);
            value = MDB_val{};

            MONERO_LMDB_CHECK(mdb_cursor_get(blocks_cur.get(), &keyv, &value, MDB_SET));
            MONERO_LMDB_CHECK(mdb_cursor_get(blocks_cur.get(), &keyv, &value, MDB_LAST_DUP));

            const expect<block_id> height =
                blocks.get_value<MONERO_FIELD(block_info, id)>(value);
            if (!height)
                return height.error();

            request_info info{};
            info.address = address;
            static_assert(sizeof(info.key) == sizeof(key), "bad memcpy");
            std::memcpy(std::addressof(info.key), std::addressof(key), sizeof(key));
            info.creation = *current_time;
            info.start_height = *height;
            info.creation_flags = flags;

            keyv = lmdb::to_val(req);
            value = lmdb::to_val(info);

            err = mdb_cursor_put(requests_cur.get(), &keyv, &value, MDB_NODUPDATA);
            if (err == MDB_KEYEXIST)
                return {lws::error::duplicate_request};
            if (err)
                return {lmdb::error(err)};

            return success();
        });
    }

    expect<void> storage::import_request(account_address const& address, block_id height) noexcept
    {
        MONERO_PRECOND(db != nullptr);
        return db->try_write([this, &address, height] (MDB_txn& txn) -> expect<void>
        {
            const expect<db::account_time> current_time = get_account_time();
            if (!current_time)
                return current_time.error();

            cursor::blocks accounts_ba_cur;
            cursor::requests requests_cur;

            MONERO_CHECK(check_cursor(txn, this->db->tables.accounts_ba, accounts_ba_cur));
            MONERO_CHECK(check_cursor(txn, this->db->tables.requests, requests_cur));

            MDB_val key = lmdb::to_val(by_address_version);
            MDB_val value = lmdb::to_val(address);

            int err = mdb_cursor_get(accounts_ba_cur.get(), &key, &value, MDB_GET_BOTH);
            if (err == MDB_NOTFOUND)
                return {lws::error::account_not_found};
            if (err)
                return {lmdb::error(err)};

            request_info info{};
            info.address = address;
            info.start_height = height;

            const request req = request::import_scan;
            key = lmdb::to_val(req);
            value = lmdb::to_val(info);

            err = mdb_cursor_put(requests_cur.get(), &key, &value, MDB_NODUPDATA);
            if (err == MDB_KEYEXIST)
                return {lws::error::duplicate_request};
            if (err)
                return {lmdb::error(err)};

            return success();
        });
    }

    namespace
    {
        expect<std::vector<account_address>>
        create_accounts(MDB_txn& txn, storage_internal::tables_ const& tables, epee::span<const account_address> addresses)
        {
            std::vector<account_address> stored{};
            stored.reserve(addresses.size());

            const expect<db::account_time> current_time = get_account_time();
            if (!current_time)
                return current_time.error();

            cursor::accounts accounts_cur;
            cursor::accounts_by_address accounts_ba_cur;
            cursor::accounts_by_height accounts_bh_cur;
            cursor::requests requests_cur;

            MONERO_CHECK(check_cursor(txn, tables.accounts, accounts_cur));
            MONERO_CHECK(check_cursor(txn, tables.accounts_ba, accounts_ba_cur));
            MONERO_CHECK(check_cursor(txn, tables.accounts_bh, accounts_bh_cur));
            MONERO_CHECK(check_cursor(txn, tables.requests, requests_cur));

            expect<account_id> last_id = find_last_id(*accounts_cur);
            if (!last_id)
                return last_id.error();

            const request req = request::create;
            for (account_address const& address : addresses)
            {
                MDB_val keyv = lmdb::to_val(req);
                MDB_val value = lmdb::to_val(address);
                int err = mdb_cursor_get(requests_cur.get(), &keyv, &value, MDB_GET_BOTH);
                if (err == MDB_NOTFOUND)
                    continue;
                if (err)
                    return {lmdb::error(err)};

                const expect<db::request_info> info = requests.get_value<db::request_info>(value);
                if (!info)
                    return info.error();

                MONERO_LMDB_CHECK(mdb_cursor_del(requests_cur.get(), 0));

                const account_id next_id = account_id(lmdb::to_native(*last_id) + 1);
                if (next_id == account_id::invalid)
                    return {lws::error::account_max};

                account user{};
                user.id = next_id;
                user.address = address;
                user.key = info->key;
                user.start_height = info->start_height;
                user.scan_height = info->start_height;
                user.access = *current_time;
                user.creation = info->creation;
                user.flags = info->creation_flags;

                const expect<void> added =
                    do_add_account(*accounts_cur, *accounts_ba_cur, *accounts_bh_cur, user);

                if (!added)
                {
                    if (added == lws::error::account_exists || added == lws::error::bad_view_key)
                        continue;
                    return added.error();
                }

                *last_id = next_id;
                stored.push_back(address);
            }
            return stored;
        }

        expect<std::vector<account_address>>
        import_accounts(MDB_txn& txn, storage_internal::tables_ const& tables, epee::span<const account_address> addresses)
        {
            std::vector<account_address> updated{};
            updated.reserve(addresses.size());

            cursor::accounts accounts_cur;
            cursor::accounts accounts_ba_cur;
            cursor::accounts accounts_bh_cur;
            cursor::requests requests_cur;

            MONERO_CHECK(check_cursor(txn, tables.accounts, accounts_cur));
            MONERO_CHECK(check_cursor(txn, tables.accounts_ba, accounts_ba_cur));
            MONERO_CHECK(check_cursor(txn, tables.accounts_bh, accounts_bh_cur));
            MONERO_CHECK(check_cursor(txn, tables.requests, requests_cur));

            const request req = request::import_scan;
            for (account_address const& address : addresses)
            {
                MDB_val key = lmdb::to_val(req);
                MDB_val value = lmdb::to_val(address);
                const int err = mdb_cursor_get(requests_cur.get(), &key, &value, MDB_GET_BOTH);
                if (err == MDB_NOTFOUND)
                    continue;
                if (err)
                    return {lmdb::error(err)};

                const expect<block_id> new_height =
                    requests.get_value<MONERO_FIELD(request_info, start_height)>(value);
                MONERO_LMDB_CHECK(mdb_cursor_del(requests_cur.get(), 0));
                if (!new_height)
                    return new_height.error();

                const expect<void> changed = change_height(
                   *accounts_cur, *accounts_ba_cur, *accounts_bh_cur, *new_height, address
                );
                if (changed)
                    updated.push_back(address);
                else if (changed != lws::error::account_not_found)
                    return changed.error();
            }
            return updated;
        }
    } // anonymous

    expect<std::vector<account_address>>
    storage::accept_requests(request req, epee::span<const account_address> addresses)
    {
        if (addresses.empty())
            return std::vector<account_address>{};

        MONERO_PRECOND(db != nullptr);
        return db->try_write([this, req, addresses] (MDB_txn& txn) -> expect<std::vector<account_address>>
        {
            switch (req)
            {
                case request::create:
                    return create_accounts(txn, this->db->tables, addresses);
                case request::import_scan:
                    return import_accounts(txn, this->db->tables, addresses);
                default:
                    break;
            }
            return {common_error::kInvalidArgument};
        });
    }

    expect<std::vector<account_address>>
    storage::reject_requests(request req, epee::span<const account_address> addresses)
    {
        if (addresses.empty())
            return std::vector<account_address>{};

        MONERO_PRECOND(db != nullptr);
        return db->try_write([this, req, addresses] (MDB_txn& txn) -> expect<std::vector<account_address>>
        {
            std::vector<account_address> rejected{};

            cursor::requests requests_cur;
            MONERO_CHECK(check_cursor(txn, this->db->tables.requests, requests_cur));

            MDB_val key = lmdb::to_val(req);
            for (account_address const& address : addresses)
            {
                MDB_val value = lmdb::to_val(address);
                const int err = mdb_cursor_get(requests_cur.get(), &key, &value, MDB_GET_BOTH);
                if (err && err != MDB_NOTFOUND)
                    return {lmdb::error(err)};

                if (!err)
                {
                    MONERO_LMDB_CHECK(mdb_cursor_del(requests_cur.get(), 0));
                    rejected.push_back(address);
                }
            }

            return rejected;
        });
    }

    namespace
    {
        expect<void>
        add_spends(MDB_cursor& spends_cur, MDB_cursor& images_cur, account_id user, epee::span<const spend> spends) noexcept
        {
            MONERO_CHECK(bulk_insert(spends_cur, user, spends));
            for (auto const& entry : spends)
            {
                const db::key_image image{entry.image, entry.link};

                MDB_val key = lmdb::to_val(entry.source);
                MDB_val value = lmdb::to_val(image);
                const int err = mdb_cursor_put(&images_cur, &key, &value, MDB_NODUPDATA);
                if (err && err != MDB_KEYEXIST)
                    return {lmdb::error(err)};
            }
            return success();
        }
    } // anonymous

    expect<std::size_t> storage::update(block_id height, epee::span<const crypto::hash> chain, epee::span<const lws::account> users)
    {
        if (users.empty() && chain.empty())
            return 0;

        MONERO_PRECOND(!chain.empty());
        MONERO_PRECOND(db != nullptr);

        return db->try_write([this, height, chain, users] (MDB_txn& txn) -> expect<std::size_t>
        {
            epee::span<const crypto::hash> chain_copy{chain};
            const std::uint64_t last_update =
                lmdb::to_native(height) + chain.size() - 1;

            if (get_checkpoints().get_max_height() <= last_update)
            {
                cursor::blocks blocks_cur;
                MONERO_CHECK(check_cursor(txn, this->db->tables.blocks, blocks_cur));

                MDB_val key = lmdb::to_val(blocks_version);
                MDB_val value;
                MONERO_LMDB_CHECK(mdb_cursor_get(blocks_cur.get(), &key, &value, MDB_SET));
                MONERO_LMDB_CHECK(mdb_cursor_get(blocks_cur.get(), &key, &value, MDB_LAST_DUP));

                const expect<block_info> last_block = blocks.get_value<block_info>(value);
                if (!last_block)
                    return last_block.error();
                if (last_block->id < height)
                    return {lws::error::bad_blockchain};

                const std::uint64_t last_same =
                    std::min(lmdb::to_native(last_block->id), last_update);

                const expect<crypto::hash> hash_check =
                    get_block_hash(*blocks_cur, block_id(last_same));
                if (!hash_check)
                    return hash_check.error();

                const std::uint64_t offset = last_same - lmdb::to_native(height);
                if (*hash_check != *(chain_copy.begin() + offset))
                    return {lws::error::blockchain_reorg};

                chain_copy.remove_prefix(offset + 1);
                MONERO_CHECK(
                    append_block_hashes(
                        *blocks_cur, block_id(lmdb::to_native(height) + offset + 1), chain_copy
                    )
                );
            }

            cursor::accounts            accounts_cur;
            cursor::accounts_by_address accounts_ba_cur;
            cursor::accounts_by_height  accounts_bh_cur;
            cursor::outputs             outputs_cur;
            cursor::spends              spends_cur;
            cursor::images              images_cur;

            MONERO_CHECK(check_cursor(txn, this->db->tables.accounts, accounts_cur));
            MONERO_CHECK(check_cursor(txn, this->db->tables.accounts_bh, accounts_bh_cur));
            MONERO_CHECK(check_cursor(txn, this->db->tables.outputs, outputs_cur));
            MONERO_CHECK(check_cursor(txn, this->db->tables.spends, spends_cur));
            MONERO_CHECK(check_cursor(txn, this->db->tables.images, images_cur));

            // for bulk inserts
            boost::container::static_vector<account_lookup, 127> heights{};
            static_assert(sizeof(heights) <= 1024, "stack vector is large");

            std::size_t updated = 0;
            for (auto user = users.begin() ;; ++user)
            {
                if (heights.size() == heights.capacity() || user == users.end())
                {
                    // bulk update account height index
                    MONERO_CHECK(
                        bulk_insert(*accounts_bh_cur, last_update, epee::to_span(heights))
                    );
                    if (user == users.end())
                        break;
                    heights.clear();
                }

                // faster to assume that account is still active
                account_status status_key = account_status::active;
                const account_id user_id = user->id();
                MDB_val key = lmdb::to_val(status_key);
                MDB_val value = lmdb::to_val(user_id);
                int err = mdb_cursor_get(accounts_cur.get(), &key, &value, MDB_GET_BOTH);
                if (err)
                {
                    if (err != MDB_NOTFOUND)
                        return {lmdb::error(err)};
                    if (accounts_ba_cur == nullptr)
                        MONERO_CHECK(check_cursor(txn, this->db->tables.accounts_ba, accounts_ba_cur));

                    MDB_val temp_key = lmdb::to_val(by_address_version);
                    MDB_val temp_value = lmdb::to_val(user->db_address());
                    err = mdb_cursor_get(accounts_ba_cur.get(), &temp_key, &temp_value, MDB_GET_BOTH);
                    if (err)
                    {
                        if (err != MDB_NOTFOUND)
                            return {lmdb::error(err)};
                        continue; // to next account
                    }

                    const expect<account_lookup> lookup =
                        accounts_by_address.get_value<MONERO_FIELD(account_by_address, lookup)>(temp_value);
                    if (!lookup)
                        return lookup.error();

                    status_key = lookup->status;
                    MONERO_LMDB_CHECK(mdb_cursor_get(accounts_cur.get(), &key, &value, MDB_GET_BOTH));
                }
                expect<account> existing = accounts.get_value<account>(value);
                if (!existing || existing->scan_height != user->scan_height())
                    continue; // to next account

                const block_id existing_height = existing->scan_height;

                existing->scan_height = block_id(last_update);
                value = lmdb::to_val(*existing);
                MONERO_LMDB_CHECK(mdb_cursor_put(accounts_cur.get(), &key, &value, MDB_CURRENT));

                heights.push_back(account_lookup{user->id(), status_key});

                key = lmdb::to_val(existing_height);
                value = lmdb::to_val(user_id);
                MONERO_LMDB_CHECK(mdb_cursor_get(accounts_bh_cur.get(), &key, &value, MDB_GET_BOTH));
                MONERO_LMDB_CHECK(mdb_cursor_del(accounts_bh_cur.get(), 0));

                MONERO_CHECK(bulk_insert(*outputs_cur, user->id(), epee::to_span(user->outputs())));
                MONERO_CHECK(add_spends(*spends_cur, *images_cur, user->id(), epee::to_span(user->spends())));

                ++updated;
            } // ... for every account being updated ...
            return updated;
        });
    }
} // db
} // lws
