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
#include "scanner.h"

#include <algorithm>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/range/combine.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <cassert>
#include <chrono>
#include <cstring>
#include <type_traits>
#include <utility>

#include "common/error.h"
#include "crypto/crypto.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_core/cryptonote_tx_utils.h"
#include "device/device_default.hpp"
#include "light_wallet_server/db/account.h"
#include "light_wallet_server/db/data.h"
#include "light_wallet_server/error.h"
#include "light_wallet_server/json.h"
#include "misc_log_ex.h"
#include "rpc/daemon_messages.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "lws"

namespace lws
{
    std::atomic<bool> scanner::running{true};

    // Not in `rates.h` - defaulting to JSON output seems odd
    std::ostream& operator<<(std::ostream& out, lws::rates const& src)
    {
        json::rates(out, src);
        return out;
    }

    namespace
    {
        constexpr const std::chrono::seconds account_poll_interval{10};
        constexpr const std::chrono::seconds block_poll_interval{20};
        constexpr const std::chrono::minutes block_rpc_timeout{2};
        constexpr const std::chrono::seconds send_timeout{30};
        constexpr const std::chrono::seconds sync_rpc_timeout{30};

        struct thread_sync
        {
            boost::mutex sync;
            boost::condition_variable user_poll;
            std::atomic<bool> update;
        };

        struct thread_data
        {
            explicit thread_data(rpc::client client, db::storage disk, std::vector<lws::account> users)
              : client(std::move(client)), disk(std::move(disk)), users(std::move(users))
            {}

            rpc::client client;
            db::storage disk;
            std::vector<lws::account> users;
        };

        // until we have a signal-handler safe notification system
        void checked_wait(const std::chrono::nanoseconds wait)
        {
            static constexpr const std::chrono::milliseconds interval{500};

            const auto start = std::chrono::steady_clock::now();
            while (scanner::is_running())
            {
                const auto current = std::chrono::steady_clock::now() - start;
                if (wait <= current)
                    break;
                const auto sleep_time = std::min(wait - current, std::chrono::nanoseconds{interval});
                boost::this_thread::sleep_for(boost::chrono::nanoseconds{sleep_time.count()});
            }
        }

        bool send(rpc::client& client, std::string const& message)
        {
            const expect<void> sent = client.send(message, send_timeout);
            if (!sent)
            {
                if (sent.matches(std::errc::interrupted))
                    return false;
                MONERO_THROW(sent.error(), "Failed to send ZMQ RPC message");
            }
            return true;
        }

        struct by_height
        {
            bool operator()(account const& left, account const& right) const noexcept
            {
                return left.scan_height() < right.scan_height();
            }
        };

        void scan_transaction(
            epee::span<lws::account> users,
            const db::block_id height,
            const std::uint64_t timestamp,
            crypto::hash const& tx_hash,
            cryptonote::transaction const& tx,
            std::vector<std::uint64_t> const& out_ids)
        {
            if (2 < tx.version)
                throw std::runtime_error{"Unsupported tx version"};

            cryptonote::tx_extra_pub_key key;
            boost::optional<crypto::hash> prefix_hash;
            boost::optional<cryptonote::tx_extra_nonce> extra_nonce;
            std::pair<std::uint8_t, db::output::payment_id_> payment_id;

            {
                std::vector<cryptonote::tx_extra_field> extra;
                cryptonote::parse_tx_extra(tx.extra, extra);
                // allow partial parsing of tx extra (similar to wallet2.cpp)

                if (!cryptonote::find_tx_extra_field_by_type(extra, key))
                    return;

                extra_nonce.emplace();
                if (cryptonote::find_tx_extra_field_by_type(extra, *extra_nonce))
                {
                    if (cryptonote::get_payment_id_from_tx_extra_nonce(extra_nonce->nonce, payment_id.second.long_))
                        payment_id.first = sizeof(crypto::hash);
                }
                else
                    extra_nonce = boost::none;
            } // destruct extra vector

            for (account& user : users)
            {
                if (height <= user.scan_height())
                    continue; // to next user

                crypto::key_derivation derived;
                if (!crypto::generate_key_derivation(key.pub_key, user.view_key(), derived))
                    throw std::runtime_error{"Key derivation failed"};

                db::extra ext{};
                std::uint32_t mixin = 0;
                for (auto const& in : tx.vin)
                {
                    cryptonote::txin_to_key const* const in_data =
                        boost::get<cryptonote::txin_to_key>(std::addressof(in));
                    if (in_data)
                    {
                        mixin = boost::numeric_cast<std::uint32_t>(
                            std::max(std::size_t(1), in_data->key_offsets.size()) - 1
                        );

                        std::uint64_t goffset = 0;
                        for (std::uint64_t offset : in_data->key_offsets)
                        {
                            goffset += offset;
                            if (user.has_spendable(db::output_id{in_data->amount, goffset}))
                            {
                                user.add_spend(
                                    db::spend{
                                        db::transaction_link{height, tx_hash},
                                        in_data->k_image,
                                        db::output_id{in_data->amount, goffset},
                                        timestamp,
                                        tx.unlock_time,
                                        mixin,
                                        {0, 0, 0}, // reserved
                                        payment_id.first,
                                        payment_id.second.long_
                                    }
                                );
                            }
                        }
                    }
                    else if (boost::get<cryptonote::txin_gen>(std::addressof(in)))
                        ext = db::extra(ext | db::coinbase_output);
                }

                std::size_t index = -1;
                for (auto const& out : tx.vout)
                {
                    ++index;

                    cryptonote::txout_to_key const* const out_data =
                        boost::get<cryptonote::txout_to_key>(std::addressof(out.target));
                    if (!out_data)
                        continue; // to next output

                    crypto::public_key derived_pub;
                    const bool received =
                        crypto::derive_public_key(derived, index, user.spend_public(), derived_pub) &&
                        derived_pub == out_data->key;

                    if (!received)
                        continue; // to next output

                    if (!prefix_hash)
                    {
                        prefix_hash.emplace();
                        cryptonote::get_transaction_prefix_hash(tx, *prefix_hash);
                    }

                    std::uint64_t amount = out.amount;
                    rct::key mask = rct::identity();
                    if (!amount && !(ext & db::coinbase_output) && 1 < tx.version)
                    {
                        const bool bulletproof2 = (tx.rct_signatures.type == rct::RCTTypeBulletproof2);
                        const auto decrypted = cryptonote::decode_amount(
                            tx.rct_signatures.outPk.at(index).mask, tx.rct_signatures.ecdhInfo.at(index), derived, index, bulletproof2
                        );
                        if (!decrypted)
                        {
                            MWARNING(user.address() << " failed to decrypt amount for tx " << tx_hash << ", skipping output");
                            continue; // to next output
                        }
                        amount = decrypted->first;
                        mask = decrypted->second;
                        ext = db::extra(ext | db::ringct_output);
                    }

                    if (extra_nonce)
                    {
                        if (!payment_id.first && cryptonote::get_encrypted_payment_id_from_tx_extra_nonce(extra_nonce->nonce, payment_id.second.short_))
                        {
                            payment_id.first = sizeof(crypto::hash8);
                            hw::core::device_default::do_decrypt_payment_id(payment_id.second.short_, derived);
                        }
                    }

                    const bool added = user.add_out(
                        db::output{
                            db::transaction_link{height, tx_hash},
                            db::output::spend_meta_{
                                db::output_id{out.amount, out_ids.at(index)},
                                amount,
                                mixin,
                                boost::numeric_cast<std::uint32_t>(index),
                                key.pub_key
                            },
                            timestamp,
                            tx.unlock_time,
                            *prefix_hash,
                            derived_pub,
                            mask,
                            {0, 0, 0, 0, 0, 0, 0}, // reserved bytes
                            db::pack(ext, payment_id.first),
                            payment_id.second
                        }
                    );

                    if (!added)
                        MWARNING("Output not added, duplicate public key encountered");
                } // for all tx outs
            } // for all users
        }

        void update_rates(rpc::context& ctx)
        {
            const expect<boost::optional<lws::rates>> new_rates = ctx.retrieve_rates();
            if (!new_rates)
                MERROR("Failed to retrieve exchange rates: " << new_rates.error().message());
            else if (*new_rates)
                MINFO("Updated exchange rates: " << *(*new_rates));
        }

        void scan_loop(thread_sync& self, std::shared_ptr<thread_data> data) noexcept
        {
            using rpc_command = cryptonote::rpc::GetBlocksFast;

            try
            {
                // boost::thread doesn't support move-only types + attributes
                rpc::client client{std::move(data->client)};
                db::storage disk{std::move(data->disk)};
                std::vector<lws::account> users{std::move(data->users)};

                assert(!users.empty());
                assert(std::is_sorted(users.begin(), users.end(), by_height{}));

                data.reset();

                struct stop_
                {
                    thread_sync& self;
                    ~stop_() noexcept
                    {
                        self.update = true;
                        self.user_poll.notify_one();
                    }
                } stop{self};

                // RPC server assumes that `start_height == 0` means use
                // block ids. This technically skips genesis block.
                rpc_command::Request req{};
                req.start_height = std::uint64_t(users.begin()->scan_height());
                req.start_height = std::max(std::uint64_t(1), req.start_height);
                req.prune = false;

                std::string block_request = rpc::client::make_message(rpc_command::name, req);
                if (!send(client, block_request))
                    return;

                std::vector<crypto::hash> blockchain{};

                while (!self.update && scanner::is_running())
                {
                    blockchain.clear();

                    auto resp = client.receive<rpc_command::Response>(block_rpc_timeout);
                    if (!resp)
                    {
                        if (resp.matches(std::errc::interrupted))
                            return; // a signal was sent over ZMQ
                        if (resp.matches(std::errc::timed_out))
                        {
                            MWARNING("Block retrieval timeout, retrying");
                            if (!send(client, block_request))
                                return;
                            continue;
                        }
                        MONERO_THROW(resp.error(), "Failed to retrieve blocks from daemon");
                    }

                    if (resp->blocks.empty())
                        throw std::runtime_error{"Daemon unexpectedly returned zero blocks"};

                    if (resp->start_height != req.start_height)
                    {
                        MWARNING("Daemon sent wrong blocks, resetting state");
                        return;
                    }

                    // retrieve next blocks in background
                    req.start_height = resp->start_height + resp->blocks.size() - 1;
                    block_request = rpc::client::make_message(rpc_command::name, req);
                    if (!send(client, block_request))
                        return;

                    if (resp->blocks.size() <= 1)
                    {
                        // ... how about some ZMQ push stuff? we can only dream ...
                        if (client.wait(block_poll_interval).matches(std::errc::interrupted))
                            return;
                        continue;
                    }

                    if (resp->blocks.size() != resp->output_indices.size())
                        throw std::runtime_error{"Bad daemon response - need same number of blocks and indices"};

                    blockchain.push_back(cryptonote::get_block_hash(resp->blocks.front().block));

                    auto blocks = epee::to_span(resp->blocks);
                    auto indices = epee::to_span(resp->output_indices);

                    if (resp->start_height != 1)
                    {
                        // skip overlap block
                        blocks.remove_prefix(1);
                        indices.remove_prefix(1);
                    }
                    else
                        resp->start_height = 0;

                    for (auto block_data : boost::combine(blocks, indices))
                    {
                        ++(resp->start_height);

                        cryptonote::block const& block = boost::get<0>(block_data).block;
                        auto const& txes = boost::get<0>(block_data).transactions;

                        if (block.tx_hashes.size() != txes.size())
                            throw std::runtime_error{"Bad daemon response - need same number of txes and tx hashes"};

                        auto indices = epee::to_span(boost::get<1>(block_data));
                        if (indices.empty())
                            throw std::runtime_error{"Bad daemon response - missing /coinbase tx indices"};

                        crypto::hash miner_tx_hash;
                        if (!cryptonote::get_transaction_hash(block.miner_tx, miner_tx_hash))
                            throw std::runtime_error{"Failed to calculate miner tx hash"};

                        scan_transaction(
                            epee::to_mut_span(users),
                            db::block_id(resp->start_height),
                            block.timestamp,
                            miner_tx_hash,
                            block.miner_tx,
                            *(indices.begin())
                        );

                        indices.remove_prefix(1);
                        if (txes.size() != indices.size())
                            throw std::runtime_error{"Bad daemon respnse - need same number of txes and indices"};

                        for (auto tx_data : boost::combine(block.tx_hashes, txes, indices))
                        {
                            scan_transaction(
                                epee::to_mut_span(users),
                                db::block_id(resp->start_height),
                                block.timestamp,
                                boost::get<0>(tx_data),
                                boost::get<1>(tx_data),
                                boost::get<2>(tx_data)
                            );
                        }

                        blockchain.push_back(cryptonote::get_block_hash(block));
                    }

                    expect<std::size_t> updated = disk.update(
                        users.front().scan_height(), epee::to_span(blockchain), epee::to_span(users)
                    );
                    if (!updated)
                    {
                        if (updated == lws::error::blockchain_reorg)
                        {
                            MINFO("Blockchain reorg detected, resetting state");
                            return;
                        }
                        MONERO_THROW(updated.error(), "Failed to update accounts on disk");
                    }

                    MINFO("Processed " << blocks.size() << " block(s) against " << users.size() << " account(s)");
                    if (*updated != users.size())
                    {
                        MWARNING("Only updated " << *updated << " account(s) out of " << users.size() << ", resetting");
                        return;
                    }

                    for (account& user : users)
                        user.updated(db::block_id(resp->start_height));
                }
            }
            catch (std::exception const& e)
            {
                scanner::stop();
                MERROR(e.what());
            }
            catch (...)
            {
                scanner::stop();
                MERROR("Unknown exception");
            }
        }

        /*!
            Launches `thread_count` threads to run `scan_loop`, and then polls
            for active account changes in background
        */
        void check_loop(db::storage disk, rpc::context& ctx, std::size_t thread_count, std::vector<lws::account> users, std::vector<db::account_id> active)
        {
            assert(0 < thread_count);
            assert(0 < users.size());

            thread_sync self{};            
            std::vector<boost::thread> threads{};

            struct join_
            {
                thread_sync& self;
                std::vector<boost::thread>& threads;
                rpc::context& ctx;

                ~join_() noexcept
                {
                    self.update = true;
                    ctx.raise_abort_scan();
                    for (auto& thread : threads)
                        thread.join();
                }
            } join{self, threads, ctx};

            /*
                The algorithm here is extremely basic. Users are divided evenly
                amongst the configurable thread count, and grouped by scan height.
                If an old account appears, some accounts (grouped on that thread)
                will be delayed in processing waiting for that account to catch up.
                Its not the greatest, but this "will have to do" for the first cut.
                Its not expected that many people will be running
                "enterprise level" of nodes where accounts are constantly added.

                Another "issue" is that each thread works independently instead of
                more cooperatively for scanning. This requires a bit more
                synchronization, so was left for later. Its likely worth doing to
                reduce the number of transfers from the daemon, and the bottleneck
                on the writes into LMDB.

                If the active user list changes, all threads are stopped/joined,
                and everything is re-started.
            */

            boost::thread::attributes attrs;
            attrs.set_stack_size(THREAD_STACK_SIZE);

            threads.reserve(thread_count);
            std::sort(users.begin(), users.end(), by_height{});

            MINFO("Starting scan loops on " << std::min(thread_count, users.size()) << " thread(s) with " << users.size() << " account(s)");

            while (!users.empty() && --thread_count)
            {
                const std::size_t per_thread = std::max(std::size_t(1), users.size() / (thread_count + 1));
                const std::size_t count = std::min(per_thread, users.size());
                std::vector<lws::account> thread_users{
                    std::make_move_iterator(users.end() - count), std::make_move_iterator(users.end())
                };
                users.erase(users.end() - count, users.end());

                rpc::client client = MONERO_UNWRAP(ctx.connect());
                client.watch_scan_signals();

                auto data = std::make_shared<thread_data>(
                    std::move(client), disk.clone(), std::move(thread_users)
                );
                threads.emplace_back(attrs, std::bind(&scan_loop, std::ref(self), std::move(data)));
            }

            if (!users.empty())
            {
                rpc::client client = MONERO_UNWRAP(ctx.connect());
                client.watch_scan_signals();

                auto data = std::make_shared<thread_data>(
                    std::move(client), disk.clone(), std::move(users)
                );
                threads.emplace_back(attrs, std::bind(&scan_loop, std::ref(self), std::move(data)));
            }

            auto last_check = std::chrono::steady_clock::now();

            lmdb::suspended_txn read_txn{};
            db::cursor::accounts accounts_cur{};
            boost::unique_lock<boost::mutex> lock{self.sync};

            while (scanner::is_running())
            {
                update_rates(ctx);

                for (;;)
                {
                    //! \TODO use signalfd + ZMQ? Windows is the difficult case...
                    self.user_poll.wait_for(lock, boost::chrono::seconds{1});
                    if (self.update || !scanner::is_running())
                        return;
                    auto this_check = std::chrono::steady_clock::now();
                    if (account_poll_interval <= (this_check - last_check))
                    {
                        last_check = this_check;
                        break;
                    }
                }

                auto reader = disk.start_read(std::move(read_txn));
                if (!reader)
                {
                    if (reader.matches(std::errc::no_lock_available))
                    {
                        MWARNING("Failed to open DB read handle, retrying later");
                        continue;
                    }
                    MONERO_THROW(reader.error(), "Failed to open DB read handle");
                }

                auto current_users = MONERO_UNWRAP(
                    reader->get_accounts(db::account_status::active, std::move(accounts_cur))
                );
                if (current_users.count() != active.size())
                {
                    MINFO("Change in active user accounts detected, stopping scan threads...");
                    return;
                }

                for (auto user = current_users.make_iterator(); !user.is_end(); ++user)
                {
                    const db::account_id user_id = user.get_value<MONERO_FIELD(db::account, id)>();
                    if (!std::binary_search(active.begin(), active.end(), user_id))
                    {
                        MINFO("Change in active user accounts detected, stopping scan threads...");
                        return;
                    }
                }

                read_txn = reader->finish_read();
                accounts_cur = current_users.give_cursor();
            }
        }
    } // anonymous

    expect<rpc::client> scanner::sync(db::storage disk, rpc::client client)
    {
        using rpc_command = cryptonote::rpc::GetHashesFast;

        MINFO("Starting blockchain sync with daemon");

        rpc_command::Request req{};
        req.start_height = 0;
        {
            auto reader = disk.start_read();
            if (!reader)
                return reader.error();

            auto chain = reader->get_chain_sync();
            if (!chain)
                return chain.error();

            req.known_hashes = std::move(*chain);
        }

        for (;;)
        {
            if (req.known_hashes.empty())
                return {lws::error::bad_blockchain};

            expect<void> sent{lws::error::daemon_timeout};

            const std::string msg = rpc::client::make_message(rpc_command::name, req);
            auto start = std::chrono::steady_clock::now();

            while (!(sent = client.send(msg, std::chrono::seconds{1})))
            {
                if (!scanner::is_running())
                    return {lws::error::signal_abort_process};

                if (sync_rpc_timeout <= (std::chrono::steady_clock::now() - start))
                    return {lws::error::daemon_timeout};

                if (!sent.matches(std::errc::timed_out))
                    return sent.error();
            }

            expect<rpc_command::Response> resp{lws::error::daemon_timeout};
            start = std::chrono::steady_clock::now();

            while (!(resp = client.receive<rpc_command::Response>(std::chrono::seconds{1})))
            {
                if (!scanner::is_running())
                    return {lws::error::signal_abort_process};

                if (sync_rpc_timeout <= (std::chrono::steady_clock::now() - start))
                    return {lws::error::daemon_timeout};

                if (!resp.matches(std::errc::timed_out))
                    return resp.error();
            }

            //
            // Exit loop if it appears we have synced to top of chain
            //
            if (resp->hashes.size() <= 1 || resp->hashes.back() == req.known_hashes.front())
                return {std::move(client)};

            MONERO_CHECK(disk.sync_chain(db::block_id(resp->start_height), epee::to_span(resp->hashes)));

            req.known_hashes.erase(req.known_hashes.begin(), --(req.known_hashes.end()));
            for (std::size_t num = 0; num < 10; ++num)
            {
                if (resp->hashes.empty())
                    break;
              
                req.known_hashes.insert(--(req.known_hashes.end()), resp->hashes.back());
            }
        }

        return {std::move(client)};
    }

    void scanner::run(db::storage disk, rpc::context ctx, std::size_t thread_count)
    {
        thread_count = std::max(std::size_t(1), thread_count);

        rpc::client client{};
        for (;;)
        {
            const auto last = std::chrono::steady_clock::now();
            update_rates(ctx);

            std::vector<db::account_id> active;
            std::vector<lws::account> users;

            {
                MINFO("Retrieving current active account list");

                auto reader = MONERO_UNWRAP(disk.start_read());
                auto accounts = MONERO_UNWRAP(
                    reader.get_accounts(db::account_status::active)
                );

                for (db::account user : accounts.make_range())
                {
                    std::vector<db::output_id> receives{};
                    std::vector<crypto::public_key> pubs{};
                    auto receive_list = MONERO_UNWRAP(reader.get_outputs(user.id));

                    const std::size_t elems = receive_list.count();
                    receives.reserve(elems);
                    pubs.reserve(elems);

                    for (auto output = receive_list.make_iterator(); !output.is_end(); ++output)
                    {
                        receives.emplace_back(output.get_value<MONERO_FIELD(db::output, spend_meta.id)>());
                        pubs.emplace_back(output.get_value<MONERO_FIELD(db::output, pub)>());
                    }

                    users.emplace_back(user, std::move(receives), std::move(pubs));
                    active.insert(
                        std::lower_bound(active.begin(), active.end(), user.id), user.id
                    );
                }

                reader.finish_read();
            }

            if (users.empty())
            {
                MINFO("No active accounts");
                checked_wait(account_poll_interval - (std::chrono::steady_clock::now() - last));
            }
            else
                check_loop(disk.clone(), ctx, thread_count, std::move(users), std::move(active));

            if (!scanner::is_running())
                return;

            if (!client)
                client = MONERO_UNWRAP(ctx.connect());

            expect<rpc::client> synced = sync(disk.clone(), std::move(client));
            if (!synced)
            {
                if (!synced.matches(std::errc::timed_out))
                    MONERO_THROW(synced.error(), "Unable to sync blockchain");

                MWARNING("Failed to connect to daemon at " << ctx.daemon_address());
            }
            else
                client = std::move(*synced);
        }
    }
} // lws
