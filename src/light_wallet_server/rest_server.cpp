// Copyright (c) 2018-2019, The Monero Project
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

#include "rest_server.h"

#include <algorithm>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/utility/string_ref.hpp>
#include <boost/range/adaptor/indexed.hpp>
#include <boost/spirit/include/qi_eoi.hpp>
#include <boost/spirit/include/qi_sequence.hpp>
#include <boost/spirit/include/qi_uint.hpp>
#include <cstring>
#include <ctime>
#include <limits>
#include <rapidjson/document.h>
#include <string>
#include <sstream>
#include <type_traits>
#include <utility>

#include "common/error.h"
#include "common/expect.h"
#include "crypto/crypto.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "light_wallet_server/db/data.h"
#include "light_wallet_server/db/storage.h"
#include "light_wallet_server/db/string.h"
#include "light_wallet_server/error.h"
#include "light_wallet_server/json.h"
#include "light_wallet_server/random_outputs.h"
#include "lmdb/util.h"
#include "misc_os_dependent.h"
#include "net/http_base.h"
#include "net/http_server_impl_base.h"
#include "net/net_parse_helpers.h"
#include "ringct/rctOps.h"
#include "rpc/daemon_messages.h"
#include "serialization/new/json_input.h"
#include "serialization/new/json_output.h"
#include "string_tools.h"
#include "wallet/output_selection.h"

namespace lws
{
    namespace
    {
        namespace http = epee::net_utils::http;

        // funky format from mymonero backend
        struct rct_bytes
        {
            rct::key commitment;
            rct::key mask;
            rct::key amount;
        };
        static_assert(sizeof(rct_bytes) == 32 * 3, "padding in rct struct");

        struct context : epee::net_utils::connection_context_base 
        {
            context()
              : epee::net_utils::connection_context_base()
            {}
        }; 

        bool is_hidden(db::account_status status) noexcept
        {
            switch (status)
            {
                case db::account_status::active:
                case db::account_status::inactive:
                    return false;
                default:
                case db::account_status::hidden:
                    break;
            }
            return true;
        }

        bool key_check(db::account_address const& user, crypto::secret_key const& key)
        {
            crypto::public_key verify{};
            if (!crypto::secret_key_to_public_key(key, verify))
                return false;
            if (verify != user.view_public)
                return false;
            return true;
        }

        bool is_locked(std::uint64_t unlock_time, db::block_id last) noexcept
        {
            if (unlock_time > CRYPTONOTE_MAX_BLOCK_NUMBER)
                return std::chrono::seconds{unlock_time} > std::chrono::system_clock::now().time_since_epoch();
            return db::block_id(unlock_time) > last;
        }

        std::vector<db::output::spend_meta_>::const_iterator
        find_metadata(std::vector<db::output::spend_meta_> const& metas, db::output_id id)
        {
            struct by_output_id
            {
                bool operator()(db::output::spend_meta_ const& left, db::output_id right) const noexcept
                {
                    return left.id < right;
                }
                bool operator()(db::output_id left, db::output::spend_meta_ const& right) const noexcept
                {
                    return left < right.id;
                }
            };
            return std::lower_bound(metas.begin(), metas.end(), id, by_output_id{});
        }

        //! \TODO some field are written as uint64 strings - possibly for Javascript ?
        struct uint64_json_string_
        {
            expect<void> operator()(rapidjson::Value const& src, std::uint64_t& dest) const
            {
                namespace qi = boost::spirit::qi;

                if (!src.IsString())
                    return {::json::error::expected_string};

                char const* const str = src.GetString();
                if (!qi::parse(str, str + src.GetStringLength(), (qi::ulong_long >> qi::eoi), dest))
                    return {::json::error::overflow};

                return success();
            }
            expect<void> operator()(std::ostream& dest, std::uint64_t src) const
            {
                dest << '"' << src << '"';
                return success();
            }
        };
        constexpr const uint64_json_string_ uint64_json_string{};

        //! JSON formatter that matches existing string timestamp 
        struct timestamp_json_
        {
            expect<void> operator()(std::ostream& dest, std::uint64_t src) const
            {
                static_assert(std::is_integral<std::time_t>(), "time_t must be numeric");

                if (std::numeric_limits<std::time_t>::max() < src)
                    return {lws::error::system_clock_invalid_range};

                std::tm timestamp;
                const std::time_t current = src;
                if (!epee::misc_utils::get_gmt_time(current, timestamp))
                    return {lws::error::system_clock_invalid_range};

                const auto month = timestamp.tm_mon + 1;

                dest << '"' << timestamp.tm_year + 1900;
                dest << (month < 10 ? "-0" : "-") << month;
                dest << (timestamp.tm_mday < 10 ? "-0" : "-") << timestamp.tm_mday;
                dest << (timestamp.tm_hour < 10 ? "T0" : "T") << timestamp.tm_hour;
                dest << (timestamp.tm_min < 10 ? ":0" : ":") << timestamp.tm_min;
                dest << (timestamp.tm_sec < 10 ? ":0" : ":") << timestamp.tm_sec;
                dest << ".0-00:00\"";

                return success();
            }
        };
        constexpr const timestamp_json_ timestamp_json{};

        struct address_json_
        {
            expect<void> operator()(rapidjson::Value const& src, db::account_address& dest) const
            {
                std::string address;
                MONERO_CHECK(::json::string(src, address));

                const auto user = db::address_string(address);
                if (!user)
                    return user.error();
                dest = *user;
                return success();
            }
        };
        constexpr const address_json_ address_json{};

        struct spent_json_
        {
            using input_type = std::pair<db::output::spend_meta_, db::spend>;
            expect<void> operator()(std::ostream& dest, input_type const& src) const
            {
                static constexpr const auto fmt = ::json::object(
                    ::json::field("amount", uint64_json_string),
                    ::json::field("key_image", ::json::hex_string),
                    ::json::field("tx_pub_key", ::json::hex_string),
                    ::json::field("out_index", ::json::uint32),
                    ::json::field("mixin", ::json::uint32)
                );

                return fmt(
                    dest, src.first.amount, src.second.image,
                    src.first.tx_public, src.first.index, src.second.mixin_count
                );
            }
        };
        constexpr const spent_json_ spent_json{};

        //! \return Account info from the DB, iff key matches address AND address is NOT hidden.
        expect<db::account> get_account(rapidjson::Value const& src, db::storage_reader& reader)
        {
            static constexpr const auto fmt = ::json::object(
                ::json::field("address", address_json),
                ::json::field("view_key", ::json::hex_string)
            );

            /*!? \TODO This check can be elided iff it is checked once
            prior to this, and the address is stored in the context (so a
            user cannot login, then request info for another account). */

            db::account_address address{};
            crypto::secret_key key{};
            MONERO_CHECK(fmt(src, address, unwrap(key)));

            if (!key_check(address, key))
                return {lws::error::bad_view_key};

            const auto user = reader.get_account(address);
            if (!user)
                return user.error();
            if (is_hidden(user->first))
                return {lws::error::account_not_found};
            return user->second;
        }

        template<typename F, typename... T>
        expect<std::string> generate_body(F const& fmt, T const&... args)
        {
            /*! \TODO Performance improvement
            `std::ostream`s cannot be created quickly. `std::stringstream` must
             copy string before returning. */
            std::stringstream stream{};
            MONERO_CHECK(fmt(stream, args...));
            return stream.str();
        } 

        expect<std::string> get_address_info(rapidjson::Value const& root, db::storage disk, rpc::client const& client, context& ctx)
        {
            static constexpr const auto response = ::json::object(
                ::json::field("locked_funds", uint64_json_string),
                ::json::field("total_received", uint64_json_string),
                ::json::field("total_sent", uint64_json_string),
                ::json::field("scanned_height", ::json::uint64),
                ::json::field("scanned_block_height", ::json::uint64),
                ::json::field("start_height", ::json::uint64),
                ::json::field("transaction_height", ::json::uint64),
                ::json::field("blockchain_height", ::json::uint64),
                ::json::field("spent_outputs", ::json::array(spent_json)),
                ::json::optional_field("rates", lws::json::rates)
            );

            std::uint64_t locked = 0;
            std::uint64_t received = 0;
            std::uint64_t spent = 0;
            db::block_id chain_height;
            db::block_id user_height;
            db::block_id user_start;
            std::vector<std::pair<db::output::spend_meta_, db::spend>> spends_full;

            {
                expect<db::storage_reader> reader = disk.start_read();
                if (!reader)
                    return reader.error();

                const expect<db::account> user = get_account(root, *reader);
                if (!user)
                    return user.error();

                auto outputs = reader->get_outputs(user->id);
                if (!outputs)
                    return outputs.error();

                auto spends = reader->get_spends(user->id);
                if (!spends)
                    return spends.error();

                const expect<db::block_info> last = reader->get_last_block();
                if (!last)
                    return last.error();

                chain_height = last->id;
                user_height = user->scan_height;
                user_start = user->start_height;

                std::vector<db::output::spend_meta_> metas{};
                metas.reserve(outputs->count());

                for (auto output = outputs->make_iterator(); !output.is_end(); ++output)
                {
                    const db::output::spend_meta_ meta =
                        output.get_value<MONERO_FIELD(db::output, spend_meta)>();

                    // these outputs will usually be in correct order post ringct
                    if (metas.empty() || metas.back().id < meta.id)
                        metas.push_back(meta);
                    else
                        metas.insert(find_metadata(metas, meta.id), meta);

                    received += meta.amount;
                    if (is_locked(output.get_value<MONERO_FIELD(db::output, unlock_time)>(), chain_height))
                        locked += meta.amount;
                }

                spends_full.reserve(spends->count());
                for (auto const& spend : spends->make_range())
                {
                    const auto meta = find_metadata(metas, spend.source);
                    if (meta == metas.end() || meta->id != spend.source)
                    {
                        throw std::logic_error{
                            "Serious database error, no receive for spend"
                        };
                    }

                    spends_full.push_back({*meta, spend});
                    spent += meta->amount;
                }
            } // release temporary resources for DB reading

            const expect<lws::rates> currencies = client.get_rates();
            if (!currencies)
                MWARNING("Unable to retrieve exchange rates: " << currencies.error().message());

            return generate_body(
                response, locked, received, spent,
                user_height, user_height, user_start,
                chain_height, chain_height, spends_full, currencies
            );
        }

        expect<std::string> get_address_txs(rapidjson::Value const& root, db::storage disk, rpc::client const&, context& ctx)
        {
            struct transaction
            {
                db::output info;
                std::vector<std::pair<db::output::spend_meta_, db::spend>> spends;
                std::uint64_t spent;
            };

            struct transaction_json
            {
                expect<void> operator()(std::ostream& dest, boost::range::index_value<transaction&> src) const
                {
                    static constexpr const auto fmt = ::json::object(
                        ::json::field("id", ::json::uint64),
                        ::json::field("hash", ::json::hex_string),
                        ::json::field("timestamp", timestamp_json),
                        ::json::field("total_received", uint64_json_string),
                        ::json::field("total_sent", uint64_json_string),
                        ::json::field("unlock_time", ::json::uint64),
                        ::json::field("height", ::json::uint64),
                        ::json::optional_field("payment_id", ::json::hex_string),
                        ::json::field("coinbase", ::json::boolean),
                        ::json::field("mempool", ::json::boolean),
                        ::json::field("mixin", ::json::uint32),
                        ::json::field("spent_outputs", ::json::array(spent_json))
                    );

                    epee::span<const std::uint8_t> const* payment_id = nullptr;
                    epee::span<const std::uint8_t> payment_id_bytes;

                    const auto extra = db::unpack(src.value().info.extra);
                    if (extra.second)
                    {
                        payment_id = std::addressof(payment_id_bytes);

                        if (extra.second == sizeof(src.value().info.payment_id.short_))
                            payment_id_bytes = epee::as_byte_span(src.value().info.payment_id.short_);
                        else
                            payment_id_bytes = epee::as_byte_span(src.value().info.payment_id.long_);
                    }

                    const bool is_coinbase = (extra.first & db::coinbase_output);

                    return fmt(
                        dest,
                        std::uint64_t(src.index()), src.value().info.link.tx_hash,
                        src.value().info.timestamp,
                        src.value().info.spend_meta.amount, src.value().spent,
                        src.value().info.unlock_time, src.value().info.link.height,
                        payment_id, is_coinbase, false,
                        src.value().info.spend_meta.mixin_count, src.value().spends
                    );
                }
            };

            static constexpr const auto response = ::json::object(
                ::json::field("total_received", uint64_json_string),
                ::json::field("scanned_height", ::json::uint64),
                ::json::field("scanned_block_height", ::json::uint64),
                ::json::field("start_height", ::json::uint64),
                ::json::field("transaction_height", ::json::uint64),
                ::json::field("blockchain_height", ::json::uint64),
                ::json::field("transactions", ::json::array(transaction_json{}))
            );

            std::uint64_t received = 0;
            db::block_id user_height;
            db::block_id user_start;
            db::block_id current_height;
            std::vector<transaction> txes;
            {
                expect<db::storage_reader> reader = disk.start_read();
                if (!reader)
                    return reader.error();

                const expect<db::account> user = get_account(root, *reader);
                if (!user)
                    return user.error();

                auto outputs = reader->get_outputs(user->id);
                if (!outputs)
                    return outputs.error();

                auto spends = reader->get_spends(user->id);
                if (!spends)
                    return spends.error();

                const expect<db::block_info> last = reader->get_last_block();
                if (!last)
                    return last.error();

                user_height = user->scan_height;
                user_start = user->start_height;
                current_height = last->id;

                // merge input and output info into a single set of txes.

                auto output = outputs->make_iterator();
                auto spend = spends->make_iterator();

                std::vector<db::output::spend_meta_> metas{};

                txes.reserve(outputs->count());
                metas.reserve(txes.capacity());

                db::transaction_link next_output{};
                db::transaction_link next_spend{};

                if (!output.is_end())
                    next_output = output.get_value<MONERO_FIELD(db::output, link)>();
                if (!spend.is_end())
                    next_spend = spend.get_value<MONERO_FIELD(db::spend, link)>();

                while (!output.is_end() || !spend.is_end())
                {
                    if (!txes.empty())
                    {
                        db::transaction_link const& last = txes.back().info.link;

                        if ((!output.is_end() && next_output < last) || (!spend.is_end() && next_spend < last))
                        {
                            throw std::logic_error{"DB has unexpected sort order"};
                        }
                    }

                    if (spend.is_end() || (!output.is_end() && next_output <= next_spend))
                    {
                        std::uint64_t amount = 0;
                        if (txes.empty() || txes.back().info.link.tx_hash != next_output.tx_hash)
                        {
                            txes.push_back({*output});
                            amount = txes.back().info.spend_meta.amount;
                        }
                        else
                        {
                            amount = output.get_value<MONERO_FIELD(db::output, spend_meta.amount)>();
                            txes.back().info.spend_meta.amount += amount;
                        }

                        const db::output_id this_id = txes.back().info.spend_meta.id;
                        if (metas.empty() || metas.back().id < this_id)
                            metas.push_back(txes.back().info.spend_meta);
                        else
                            metas.insert(find_metadata(metas, this_id), txes.back().info.spend_meta);

                        received += amount;

                        ++output;
                        if (!output.is_end())
                            next_output = output.get_value<MONERO_FIELD(db::output, link)>();
                    }
                    else if (output.is_end() || (next_spend < next_output))
                    {
                        const db::output_id source_id = spend.get_value<MONERO_FIELD(db::spend, source)>();
                        const auto meta = find_metadata(metas, source_id);
                        if (meta == metas.end() || meta->id != source_id)
                        {
                            throw std::logic_error{
                                "Serious database error, no receive for spend"
                            };
                        }

                        if (txes.empty() || txes.back().info.link.tx_hash != next_spend.tx_hash)
                        {
                            txes.push_back({});
                            txes.back().spends.push_back({*meta, *spend});
                            txes.back().info.link.height = txes.back().spends.back().second.link.height;
                            txes.back().info.link.tx_hash = txes.back().spends.back().second.link.tx_hash;
                            txes.back().info.spend_meta.mixin_count =
                                txes.back().spends.back().second.mixin_count;
                            txes.back().info.timestamp = txes.back().spends.back().second.timestamp;
                            txes.back().info.unlock_time = txes.back().spends.back().second.unlock_time;
                        }
                        else
                            txes.back().spends.push_back({*meta, *spend});

                        txes.back().spent += meta->amount;

                        ++spend;
                        if (!spend.is_end())
                            next_spend = spend.get_value<MONERO_FIELD(db::spend, link)>();
                    }
                }
            } // release temporary resources for DB reading

            return generate_body(
                response, received, user_height, user_height, user_start,
                current_height, current_height, boost::adaptors::index(txes)
            );
        }

        expect<std::string> get_random_outs(rapidjson::Value const& root, db::storage, rpc::client const& gclient, context& ctx)
        {
            struct random_output_json
            {
                expect<void>
                operator()(std::ostream& dest, random_output const& src) const
                {
                    static constexpr const auto fmt = ::json::object(
                        ::json::field("global_index", uint64_json_string),
                        ::json::field("public_key", ::json::hex_string),
                        ::json::field("rct", ::json::hex_string)
                    );

                    const rct_bytes rct{src.keys.mask, rct::zero(), rct::zero()};
                    return fmt(dest, src.index, src.keys.key, rct);
                }
            };

            struct random_outputs_json
            {
                expect<void>
                operator()(std::ostream& dest, random_ring const& src) const
                {
                    static const auto fmt = ::json::object(
                        ::json::field("amount", uint64_json_string),
                        ::json::field("outputs", ::json::array(random_output_json{}))
                    );
                    return fmt(dest, src.amount, src.ring);
                }
            };

            static constexpr const auto request = ::json::object(
                ::json::field("count", ::json::uint64), // rpc to daemon is 64-bit :/
                ::json::field("amounts", ::json::array(uint64_json_string))
            );
            static constexpr const auto response = ::json::object(
                ::json::field("amount_outs", ::json::array(random_outputs_json{}))
            );

            using distribution_rpc = cryptonote::rpc::GetOutputDistribution;
            using histogram_rpc = cryptonote::rpc::GetOutputHistogram;
            using distribution_rpc = cryptonote::rpc::GetOutputDistribution;

            std::uint64_t mixin = 0;
            std::vector<std::uint64_t> amounts{};
            MONERO_CHECK(request(root, mixin, amounts));

            if (50 < mixin || 20 < amounts.size())
                return {lws::error::exceeded_rest_request_limit};

            expect<rpc::client> client = gclient.clone();
            if (!client)
                return client.error();

            const std::greater<std::uint64_t> rsort{};
            std::sort(amounts.begin(), amounts.end(), rsort);
            const std::size_t ringct_count =
                amounts.end() - std::lower_bound(amounts.begin(), amounts.end(), 0, rsort);

            std::vector<lws::histogram> histograms{};
            if (ringct_count < amounts.size())
            {
                // reuse allocated vector memory
                amounts.resize(amounts.size() - ringct_count);

                histogram_rpc::Request histogram_req{};
                histogram_req.amounts = std::move(amounts);
                histogram_req.min_count = 0;
                histogram_req.max_count = 0;
                histogram_req.unlocked = true;
                histogram_req.recent_cutoff = 0;

                const std::string msg = rpc::client::make_message(histogram_rpc::name, histogram_req);
                MONERO_CHECK(client->send(msg, std::chrono::seconds{10}));

                auto histogram_resp = client->receive<histogram_rpc::Response>(std::chrono::minutes{3});
                if (!histogram_resp)
                    return histogram_resp.error();
                if (histogram_resp->histogram.size() != histogram_req.amounts.size())
                    return {lws::error::bad_daemon_response};

                histograms = std::move(histogram_resp->histogram);

                amounts = std::move(histogram_req.amounts);
                amounts.insert(amounts.end(), ringct_count, 0);
            }

            std::vector<std::uint64_t> distributions{};
            if (ringct_count)
            {
                distribution_rpc::Request distribution_req{};
                if (ringct_count == amounts.size())
                    distribution_req.amounts = std::move(amounts);

                distribution_req.amounts.resize(1);
                distribution_req.from_height = 0;
                distribution_req.to_height = 0;
                distribution_req.cumulative = true;

                const std::string msg =
                    rpc::client::make_message(distribution_rpc::name, distribution_req);
                MONERO_CHECK(client->send(msg, std::chrono::seconds{10}));

                auto distribution_resp =
                    client->receive<distribution_rpc::Response>(std::chrono::minutes{3});
                if (!distribution_resp)
                    return distribution_resp.error();

                if (distribution_resp->distributions.size() != 1)
                    return {lws::error::bad_daemon_response};
                if (distribution_resp->distributions[0].amount != 0)
                    return {lws::error::bad_daemon_response};

                distributions = std::move(distribution_resp->distributions[0].data.distribution);

                if (amounts.empty())
                {
                    amounts = std::move(distribution_req.amounts);
                    amounts.insert(amounts.end(), ringct_count - 1, 0);
                }
            }

            class zmq_fetch_keys
            {
                /* `std::function` needs a copyable functor. The functor was
                made const and copied in the function instead of using a
                reference to make the callback in `std::function` thread-safe.
                This shouldn't be a problem now, but this is just-in-case of a
                future refactor. */
                rpc::client gclient;
            public:
                zmq_fetch_keys(rpc::client src) noexcept
                  : gclient(std::move(src))
                {}

                zmq_fetch_keys(zmq_fetch_keys&&) = default;
                zmq_fetch_keys(zmq_fetch_keys const& rhs)
                  : gclient(MONERO_UNWRAP(rhs.gclient.clone()))
                {}

                expect<std::vector<output_keys>> operator()(std::vector<lws::output_ref> ids) const
                {
                    using get_keys_rpc = cryptonote::rpc::GetOutputKeys;

                    get_keys_rpc::Request keys_req{};
                    keys_req.outputs = std::move(ids);

                    expect<rpc::client> client = gclient.clone();
                    if (!client)
                        return client.error();

                    const std::string msg = rpc::client::make_message(get_keys_rpc::name, keys_req);
                    MONERO_CHECK(client->send(msg, std::chrono::seconds{10}));

                    auto keys_resp = client->receive<get_keys_rpc::Response>(std::chrono::seconds{10});
                    if (!keys_resp)
                        return keys_resp.error();

                    return {std::move(keys_resp->keys)};
                }
            };

            tools::gamma_picker pick_rct{std::move(distributions)};
            auto rings = pick_random_outputs(
                mixin,
                epee::to_span(amounts),
                pick_rct,
                epee::to_mut_span(histograms),
                zmq_fetch_keys{std::move(*client)}
            );
            if (!rings)
                return rings.error();

            return generate_body(response, *rings);
        }

        expect<std::string> get_unspent_outs(rapidjson::Value const& root, db::storage disk, rpc::client const& gclient, context& ctx)
        {
            struct output_json
            {
                crypto::public_key const& user_public;
                crypto::secret_key const& user_key;

                expect<void>
                operator()(std::ostream& dest, std::pair<db::output, std::vector<crypto::key_image>> const& src) const
                {
                    static constexpr const auto fmt = ::json::object(
                        ::json::field("amount", uint64_json_string),
                        ::json::field("public_key", ::json::hex_string),
                        ::json::field("index", ::json::uint32),
                        ::json::field("global_index", ::json::uint64),
                        ::json::field("tx_id", ::json::uint64),
                        ::json::field("tx_hash", ::json::hex_string),
                        ::json::field("tx_prefix_hash", ::json::hex_string),
                        ::json::field("tx_pub_key", ::json::hex_string),
                        ::json::field("timestamp", timestamp_json),
                        ::json::field("height", ::json::uint64),
                        ::json::field("spend_key_images", ::json::array(::json::hex_string)),
                        ::json::optional_field("rct", ::json::hex_string)
                    );

                    /*! \TODO Sending the public key for the output isn't
                    necessary, as it can be re-computed from the other parts.
                    Same with the rct commitment and rct amount. Consider
                    dropping these from the API after client upgrades. Not
                    storing them in the DB saves 96-bytes per received out. */

                    rct_bytes rct{};
                    rct_bytes const* optional_rct = nullptr;
                    if (unpack(src.first.extra).first & db::ringct_output)
                    {
                        crypto::key_derivation derived;
                        if (!crypto::generate_key_derivation(src.first.spend_meta.tx_public, user_key, derived))
                            return {common_error::crypto_failure};

                        crypto::secret_key scalar;
                        rct::ecdhTuple encrypted{src.first.ringct_mask, rct::d2h(src.first.spend_meta.amount)};

                        crypto::derivation_to_scalar(derived, src.first.spend_meta.index, scalar);
                        rct::ecdhEncode(encrypted, rct::sk2rct(scalar), false);

                        rct.commitment = rct::commit(src.first.spend_meta.amount, src.first.ringct_mask);
                        rct.mask = encrypted.mask;
                        rct.amount = encrypted.amount;

                        optional_rct = std::addressof(rct);
                    }

                    return fmt(
                        dest, src.first.spend_meta.amount, src.first.pub, src.first.spend_meta.index,
                        src.first.spend_meta.id.low, src.first.spend_meta.id.low,
                        src.first.link.tx_hash, src.first.tx_prefix_hash,
                        src.first.spend_meta.tx_public,
                        src.first.timestamp, src.first.link.height, src.second, optional_rct
                    );
                }
            };

            static constexpr const auto request = ::json::object(
                ::json::field("address", address_json),
                ::json::field("view_key", ::json::hex_string),
                ::json::field("amount", uint64_json_string),
                ::json::optional_field("mixin", ::json::uint32),
                ::json::optional_field("use_dust", ::json::boolean),
                ::json::optional_field("dust_threshold", uint64_json_string)
            );

            expect<rpc::client> client = gclient.clone();
            if (!client)
                return client.error();

            using rpc_command = cryptonote::rpc::GetFeeEstimate;
            {
                rpc_command::Request req{};
                req.num_grace_blocks = 10;
                const std::string msg = rpc::client::make_message(rpc_command::name, req);
                MONERO_CHECK(client->send(msg, std::chrono::seconds{10}));
            }

            std::uint64_t received = 0;
            crypto::public_key spend_public{};
            crypto::secret_key key{};
            std::vector<std::pair<db::output, std::vector<crypto::key_image>>> unspent{};
            {
                db::account_address address{};
                std::uint64_t amount = 0;
                boost::optional<std::uint32_t> mixin;
                boost::optional<bool> use_dust;
                boost::optional<std::uint64_t> threshold;
                MONERO_CHECK(
                    request(root, address, unwrap(key), amount, mixin, use_dust, threshold)
                );
                if (!key_check(address, key))
                    return {lws::error::bad_view_key};

                if ((use_dust && *use_dust) || !threshold)
                    threshold = 0;

                if (!mixin)
                    mixin = 0;

                auto reader = disk.start_read();
                if (!reader)
                    return reader.error();

                const auto user = reader->get_account(address);
                if (!user)
                    return user.error();
                if (is_hidden(user->first))
                    return {lws::error::account_not_found};

                spend_public = user->second.address.spend_public;

                auto outputs = reader->get_outputs(user->second.id);
                if (!outputs)
                    return outputs.error();

                unspent.reserve(outputs->count());
                for (db::output const& out : outputs->make_range())
                {
                    if (out.spend_meta.amount < *threshold || out.spend_meta.mixin_count < *mixin)
                        continue;

                    received += out.spend_meta.amount;
                    unspent.push_back({out, {}});

                    auto images = reader->get_images(out.spend_meta.id);
                    if (!images)
                        return images.error();

                    unspent.back().second.reserve(images->count());
                    auto range = images->make_range<MONERO_FIELD(db::key_image, value)>();
                    std::copy(range.begin(), range.end(), std::back_inserter(unspent.back().second));
                }

                if (received < amount)
                    return {lws::error::account_not_found};
            } // release temporary resources for DB reading

            const auto resp = client->receive<rpc_command::Response>(std::chrono::seconds{20});
            if (!resp)
                return resp.error();

            if (resp->size_scale == 0 || 1024 < resp->size_scale || resp->fee_mask == 0)
                return {lws::error::bad_daemon_response};

            const std::uint64_t per_kb_fee =
                resp->estimated_base_fee * (1024 / resp->size_scale);
            const std::uint64_t per_kb_fee_masked =
                ((per_kb_fee + (resp->fee_mask - 1)) / resp->fee_mask) * resp->fee_mask;

            const auto response = ::json::object(
                ::json::field("per_kb_fee", ::json::uint64),
                ::json::field("fee_mask", ::json::uint64),
                ::json::field("amount", uint64_json_string),
                ::json::field("outputs", ::json::array(output_json{spend_public, key}))
            );

            return generate_body(response, per_kb_fee_masked, resp->fee_mask, received, unspent);
        }

        expect<std::string> import_request(rapidjson::Value const& root, db::storage disk, rpc::client const&, context& ctx)
        {
            static constexpr const auto request = ::json::object(
                ::json::field("address", address_json),
                ::json::field("view_key", ::json::hex_string)
            );
            static constexpr const auto response = ::json::object(
                ::json::field("import_fee", uint64_json_string),
                ::json::field("new_request", ::json::boolean),
                ::json::field("request_fulfilled", ::json::boolean),
                ::json::field("status", ::json::string)
            );

            bool new_request = false;
            bool fulfilled = false;
            db::account_address address{};
            crypto::secret_key key{};

            MONERO_CHECK(request(root, address, unwrap(key)));
            if (!key_check(address, key))
                return {lws::error::bad_view_key};
            {
                auto reader = disk.start_read();
                if (!reader)
                    return reader.error();

                const auto account = reader->get_account(address);

                if (!account || is_hidden(account->first))
                    return {lws::error::account_not_found};

                if (account->second.start_height == db::block_id(0))
                    fulfilled = true;
                else
                {
                    const expect<db::request_info> info =
                        reader->get_request(db::request::import_scan, address);

                    if (!info)
                    {
                        if (info != lmdb::error(MDB_NOTFOUND))
                            return info.error();

                        new_request = true;
                   }
                }
            }

            if (new_request)
                MONERO_CHECK(disk.import_request(address, db::block_id(0)));

            return generate_body(
                response, 0, new_request, fulfilled,
                new_request ? "Accepted, waiting for approval" : (fulfilled ? "Approved" : "Waiting for Approval")
            );
        }

        expect<std::string> login(rapidjson::Value const& root, db::storage disk, rpc::client const&, context& ctx)
        {
            static constexpr const auto request = ::json::object(
                ::json::field("address", address_json),
                ::json::field("view_key", ::json::hex_string),
                ::json::field("create_account", ::json::boolean),
                ::json::field("generated_locally", ::json::boolean)
            );
            static constexpr const auto response = ::json::object(
                ::json::field("new_address", ::json::boolean),
                ::json::field("generated_locally", ::json::boolean)
            );

            db::account_address address{};
            crypto::secret_key key{};
            bool create = false;
            bool generated = false;

            MONERO_CHECK(request(root, address, unwrap(key), create, generated));
            if (!key_check(address, key))
                return {lws::error::bad_view_key};
            {
                auto reader = disk.start_read();
                if (!reader)
                    return reader.error();

                const auto account = reader->get_account(address);
                reader->finish_read();

                if (account)
                {
                    if (is_hidden(account->first))
                        return {lws::error::account_not_found};

                    // Do not count a request for account creation as login
                    return generate_body(response, false, (account->second.flags & db::account_generated_locally));
                }
                else if (!create || account != lws::error::account_not_found)
                    return account.error();
            }

            MONERO_CHECK(disk.creation_request(address, key, (generated ? db::account_generated_locally : db::default_account)));
            return generate_body(response, true, generated);
        }

        expect<std::string> submit_raw_tx(rapidjson::Value const& root, db::storage, rpc::client const& gclient, context& ctx)
        {
            constexpr const auto request = ::json::object(::json::field("tx", ::json::string));
            constexpr const auto response =
                ::json::object(::json::field("status", ::json::string));

            using transaction_rpc = cryptonote::rpc::SendRawTx;

            expect<rpc::client> client = gclient.clone();
            if (!client)
                return client.error();

            std::string hex{};
            MONERO_CHECK(request(root, hex));

            std::string blob{};
            if (!epee::string_tools::parse_hexstr_to_binbuff(hex, blob))
                return {::json::error::invalid_hex};

            transaction_rpc::Request req{};
            req.relay = true;
            if (!cryptonote::parse_and_validate_tx_from_blob(blob, req.tx))
                return {lws::error::bad_client_tx};

            const std::string message = rpc::client::make_message(transaction_rpc::name, req);
            MONERO_CHECK(client->send(message, std::chrono::seconds{10}));

            const auto resp = client->receive<transaction_rpc::Response>(std::chrono::seconds{20});
            if (!resp)
                return resp.error();
            if (!resp->relayed)
                return {lws::error::tx_relay_failed};

            return generate_body(response, "OK");
        }

        struct endpoint
        {
            char const* const name;
            expect<std::string>
                (*const run)(rapidjson::Value const&, db::storage, rpc::client const&, context&);
            const unsigned max_size;
        };

        constexpr const endpoint endpoints[] = {
            {"/get_address_info",      &get_address_info, 2 * 1024},
            {"/get_address_txs",       &get_address_txs,  2 * 1024},
            {"/get_random_outs",       &get_random_outs,  2 * 1024},
            {"/get_txt_records",       nullptr,           0       },
            {"/get_unspent_outs",      &get_unspent_outs, 2 * 1024},
            {"/import_wallet_request", &import_request,   2 * 1024},
            {"/login",                 &login,            2 * 1024},
            {"/submit_raw_tx",         &submit_raw_tx,    50 * 1024}
        };

        struct by_name_
        {
            bool operator()(endpoint const& left, endpoint const& right) const noexcept
            {
                if (left.name && right.name)
                    return std::strcmp(left.name, right.name) < 0;
                return false;
            }
            bool operator()(const boost::string_ref left, endpoint const& right) const noexcept
            {
                if (right.name)
                    return left < right.name;
                return false;
            }
            bool operator()(endpoint const& left, const boost::string_ref right) const noexcept
            {
                if (left.name)
                    return left.name < right;
                return false;
            }
        };
        constexpr const by_name_ by_name{};
    } // anonymous

struct rest_server::internal final : public epee::http_server_impl_base<rest_server::internal, context>
{
    db::storage disk;
    rpc::client client;

    explicit internal(boost::asio::io_service& io_service, lws::db::storage disk, rpc::client client)
      : epee::http_server_impl_base<rest_server::internal, context>(io_service, true)
      , disk(std::move(disk))
      , client(std::move(client))
    {
        assert(std::is_sorted(std::begin(endpoints), std::end(endpoints), by_name));
    }

    virtual bool
    handle_http_request(http::http_request_info const& query, http::http_response_info& response, context& ctx)
    override final
    {
        const auto handler = std::lower_bound(
            std::begin(endpoints), std::end(endpoints), query.m_URI, by_name
        );
        if (handler == std::end(endpoints) || handler->name != query.m_URI)
        {
            response.m_response_code = 404;
            response.m_response_comment = "Not Found";
            return true;
        }

        if (handler->run == nullptr)
        {
            response.m_response_code = 501;
            response.m_response_comment = "Not Implemented";
            return true;
        }

        if (handler->max_size < query.m_body.size())
        {
            MINFO("Client exceeded maximum body size (" << handler->max_size << " bytes)");
            response.m_response_code = 400;
            response.m_response_comment = "Bad Request";
            return true;
        }

        if (query.m_http_method != http::http_method_post)
        {
            response.m_response_code = 405;
            response.m_response_comment = "Method Not Allowed";
            return true;
        }

        rapidjson::Document doc{};
        if (!rapidjson::ParseResult(doc.Parse(query.m_body.c_str())))
        {
            MINFO("JSON Parsing error from " << ctx.m_remote_address.str());
            response.m_response_code = 400;
            response.m_response_comment = "Bad Request";
            return true; 
        }

        auto body = handler->run(doc, disk.clone(), client, ctx);
        if (!body)
        {
            MINFO(body.error().message() << " from " << ctx.m_remote_address.str() << " on " << handler->name);

            if (body == lws::error::account_not_found)
            {
                response.m_response_code = 403;
                response.m_response_comment = "Forbidden";
            }
            else if (body.matches(std::errc::timed_out) || body.matches(std::errc::no_lock_available))
            {
                response.m_response_code = 503;
                response.m_response_comment = "Service Unavailable";
            }
            else
            {
                response.m_response_code = 500;
                response.m_response_comment = "Internal Server Error";
            }
            return true;
        }

        response.m_response_code = 200;
        response.m_response_comment = "OK";
        response.m_mime_tipe = "application/json";
        response.m_header_info.m_content_type = "application/json";
        response.m_body = std::move(*body);
        return true;
    }
};

rest_server::rest_server(epee::span<const std::string> addresses, db::storage disk, rpc::client client, configuration config)
  : io_service_(), ports_()
{
    ports_.emplace_back(io_service_, std::move(disk), std::move(client));

    if (addresses.empty())
        MONERO_THROW(common_error::kInvalidArgument, "REST server requires 1 or more addresses");

    const auto init_port = [] (internal& port, const std::string& address, configuration config) -> bool
    {
        epee::net_utils::http::url_content url{};
        if (!epee::net_utils::parse_url(address, url))
            MONERO_THROW(common_error::configuration, "REST Server URL/address is invalid");

        const bool https = url.schema == "https";
        if (!https && url.schema != "http")
            MONERO_THROW(common_error::configuration, "Unsupported scheme, only http or https supported");

        if (std::numeric_limits<std::uint16_t>::max() < url.port)
            MONERO_THROW(common_error::configuration, "Specified port for rest server is out of range");

        if (!https)
        {
            boost::system::error_code error{};
            const boost::asio::ip::address ip_host =
                ip_host.from_string(url.host, error);
            if (error)
                MONERO_THROW(common_error::configuration, "Invalid IP address for REST server");
            if (!ip_host.is_loopback() && !config.allow_external)
                MONERO_THROW(common_error::configuration, "Binding to external interface with http - consider using https or secure tunnel (ssh, etc). Use --confirm-external-bind to override");
        }

        if (url.port == 0)
            url.port = https ? 8443 : 8080;

        epee::net_utils::ssl_options_t ssl_options = https ?
            epee::net_utils::ssl_support_t::e_ssl_support_enabled :
            epee::net_utils::ssl_support_t::e_ssl_support_disabled;
        ssl_options.verification = epee::net_utils::ssl_verification_t::none; // clients verified with view key
        ssl_options.auth = std::move(config.auth);

        if (!port.init(nullptr, std::to_string(url.port), std::move(url.host), std::string{}, false, true, std::move(config.access_controls), boost::none, std::move(ssl_options)))
            MONERO_THROW(lws::error::http_server, "REST server failed to initialize");
        return https;
    };

    bool any_ssl = false;
    for (std::size_t index = 1; index < addresses.size(); ++index)
    {
        ports_.emplace_back(io_service_, ports_.front().disk.clone(), MONERO_UNWRAP(ports_.front().client.clone()));
        any_ssl |= init_port(ports_.back(), addresses[index], config);
    }

    const bool expect_ssl = !config.auth.private_key_path.empty();
    const std::size_t threads = config.threads;
    any_ssl |= init_port(ports_.front(), addresses[0], std::move(config));
    if (!any_ssl && expect_ssl)
        MONERO_THROW(common_error::configuration, "Specified SSL key/cert without specifying https capable REST server");

    if (!ports_.front().run(threads, false))
        MONERO_THROW(lws::error::http_server, "REST server failed to run");
}

rest_server::~rest_server() noexcept
{}

} // lws
