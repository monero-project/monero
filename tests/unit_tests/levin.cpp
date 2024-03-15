// Copyright (c) 2019-2024, The Monero Project
//
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

#include <algorithm>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <cstring>
#include <gtest/gtest.h>
#include <limits>
#include <set>
#include <map>

#include "byte_slice.h"
#include "crypto/crypto.h"
#include "cryptonote_basic/connection_context.h"
#include "cryptonote_config.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_core/i_core_events.h"
#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "cryptonote_protocol/levin_notify.h"
#include "int-util.h"
#include "p2p/net_node.h"
#include "net/dandelionpp.h"
#include "net/levin_base.h"
#include "span.h"

namespace
{
    class test_endpoint final : public epee::net_utils::i_service_endpoint
    {
        boost::asio::io_service& io_service_;
        std::size_t ref_count_;

        virtual bool do_send(epee::byte_slice message) override final
        {
            send_queue_.push_back(std::move(message));
            return true;
        }

        virtual bool close() override final
        {
            return true;
        }

        virtual bool send_done() override final
        {
            throw std::logic_error{"send_done not implemented"};
        }

        virtual bool call_run_once_service_io() override final
        {
            return io_service_.run_one();
        }

        virtual bool request_callback() override final
        {
            throw std::logic_error{"request_callback not implemented"};
        }

        virtual boost::asio::io_service& get_io_service() override final
        {
            return io_service_;
        }

        virtual bool add_ref() override final
        {
            ++ref_count_;
            return true;
        }

        virtual bool release() override final
        {
            --ref_count_;
            return true;
        }

    public:
        test_endpoint(boost::asio::io_service& io_service)
          : epee::net_utils::i_service_endpoint(),
	          io_service_(io_service),
            ref_count_(0),
            send_queue_()
        {}

        virtual ~test_endpoint() noexcept(false) override final
        {
            EXPECT_EQ(0u, ref_count_);
        }

        std::deque<epee::byte_slice> send_queue_;
    };

    class test_core_events final : public cryptonote::i_core_events
    {
        std::map<cryptonote::relay_method, std::vector<cryptonote::blobdata>> relayed_;

        virtual bool is_synchronized() const final
        {
            return false;
        }

        virtual uint64_t get_current_blockchain_height() const final
        {
            return 0;
        }

        virtual void on_transactions_relayed(epee::span<const cryptonote::blobdata> txes, cryptonote::relay_method relay) override final
        {
            std::vector<cryptonote::blobdata>& cached = relayed_[relay];
            for (const auto& tx : txes)
                cached.push_back(tx);
        }

    public:
        test_core_events()
          : relayed_()
        {}

        std::size_t relayed_method_size() const noexcept
        {
            return relayed_.size();
        }

        bool has_stem_txes() const noexcept
        {
            return relayed_.count(cryptonote::relay_method::stem);
        }

        std::vector<cryptonote::blobdata> take_relayed(cryptonote::relay_method relay)
        {
            auto elems = relayed_.find(relay);
            if (elems == relayed_.end())
                throw std::logic_error{"on_transactions_relayed empty"};

            std::vector<cryptonote::blobdata> out{std::move(elems->second)};
            relayed_.erase(elems);
            return out;
        }
    };

    class test_connection
    {
        test_endpoint endpoint_;
        cryptonote::levin::detail::p2p_context context_;
        epee::levin::async_protocol_handler<cryptonote::levin::detail::p2p_context> handler_;

    public:
        test_connection(boost::asio::io_service& io_service, cryptonote::levin::connections& connections, boost::uuids::random_generator& random_generator, const bool is_incoming)
          : endpoint_(io_service),
            context_(),
            handler_(std::addressof(endpoint_), connections, context_)
        {
            using base_type = epee::net_utils::connection_context_base;
            static_cast<base_type&>(context_) = base_type{random_generator(), {}, is_incoming, false};
            context_.m_state = cryptonote::cryptonote_connection_context::state_normal;
            handler_.after_init_connection();
        }

        //\return Number of messages processed
        std::size_t process_send_queue(const bool valid = true)
        {
            std::size_t count = 0;
            for ( ; !endpoint_.send_queue_.empty(); ++count, endpoint_.send_queue_.pop_front())
            {
                EXPECT_EQ(valid, handler_.handle_recv(endpoint_.send_queue_.front().data(), endpoint_.send_queue_.front().size()));
            }
            return count;
        }

        const boost::uuids::uuid& get_id() const noexcept
        {
            return context_.m_connection_id;
        }

        bool is_incoming() const noexcept
        {
            return context_.m_is_income;
        }
    };

    struct received_message
    {
        boost::uuids::uuid connection;
        int command;
        std::string payload;
    };

    class test_receiver final : public epee::levin::levin_commands_handler<cryptonote::levin::detail::p2p_context>
    {
        std::deque<received_message> invoked_;
        std::deque<received_message> notified_;

        template<typename T>
        static std::pair<boost::uuids::uuid, typename T::request> get_message(std::deque<received_message>& queue)
        {
            if (queue.empty())
                throw std::logic_error{"Queue has no received messges"};

            if (queue.front().command != T::ID)
                throw std::logic_error{"Unexpected ID at front of message queue"};

            epee::serialization::portable_storage storage{};
            if(!storage.load_from_binary(epee::strspan<std::uint8_t>(queue.front().payload)))
                throw std::logic_error{"Unable to parse epee binary format"};

            typename T::request request{};
            if (!request.load(storage))
                throw std::logic_error{"Unable to load into expected request"};

            boost::uuids::uuid connection = queue.front().connection;
            queue.pop_front();
            return {connection, std::move(request)};
        }

        static received_message get_raw_message(std::deque<received_message>& queue)
        {
            received_message out{std::move(queue.front())};
            queue.pop_front();
            return out;
        }

        virtual int invoke(int command, const epee::span<const uint8_t> in_buff, epee::byte_stream& buff_out, cryptonote::levin::detail::p2p_context& context) override final
        {
            buff_out.clear();
            invoked_.push_back(
                {context.m_connection_id, command, std::string{reinterpret_cast<const char*>(in_buff.data()), in_buff.size()}}
            );
            return 1;
        }

        virtual int notify(int command, const epee::span<const uint8_t> in_buff, cryptonote::levin::detail::p2p_context& context) override final
        {
            notified_.push_back(
                {context.m_connection_id, command, std::string{reinterpret_cast<const char*>(in_buff.data()), in_buff.size()}}
            );
            return 1;
        }

        virtual void callback(cryptonote::levin::detail::p2p_context& context) override final
        {}

        virtual void on_connection_new(cryptonote::levin::detail::p2p_context& context) override final
        {
            if (notifier)
                notifier->on_handshake_complete(context.m_connection_id, context.m_is_income);
        }

        virtual void on_connection_close(cryptonote::levin::detail::p2p_context& context) override final
        {
            if (notifier)
                notifier->on_connection_close(context.m_connection_id);
        }

    public:
        test_receiver()
          : epee::levin::levin_commands_handler<cryptonote::levin::detail::p2p_context>(),
            invoked_(),
            notified_()
        {}

        virtual ~test_receiver() noexcept override final{}

        std::size_t invoked_size() const noexcept
        {
            return invoked_.size();
        }

        std::size_t notified_size() const noexcept
        {
            return notified_.size();
        }

        template<typename T>
        std::pair<boost::uuids::uuid, typename T::request> get_invoked()
        {
            return get_message<T>(invoked_);
        }

        template<typename T>
        std::pair<boost::uuids::uuid, typename T::request> get_notification()
        {
            return get_message<T>(notified_);
        }

        received_message get_raw_notification()
        {
            return get_raw_message(notified_);
        }

        std::shared_ptr<cryptonote::levin::notify> notifier{};
    };

    class levin_notify : public ::testing::Test
    {
        const std::shared_ptr<cryptonote::levin::connections> connections_;
        std::set<boost::uuids::uuid> connection_ids_;

    public:
        levin_notify()
          : ::testing::Test(),
            connections_(std::make_shared<cryptonote::levin::connections>()),
            connection_ids_(),
            random_generator_(),
            io_service_(),
            receiver_(),
            contexts_(),
            events_()
        {
            connections_->set_handler(std::addressof(receiver_), nullptr);
        }

        virtual void TearDown() override final
        {
            EXPECT_EQ(0u, receiver_.invoked_size());
            EXPECT_EQ(0u, receiver_.notified_size());
            EXPECT_EQ(0u, events_.relayed_method_size());
        }

        cryptonote::levin::connections& get_connections() noexcept { return *connections_; }

        void add_connection(const bool is_incoming)
        {
            contexts_.emplace_back(io_service_, *connections_, random_generator_, is_incoming);
            EXPECT_TRUE(connection_ids_.emplace(contexts_.back().get_id()).second);
            EXPECT_EQ(connection_ids_.size(), connections_->get_connections_count());
        }

        std::shared_ptr<cryptonote::levin::notify> make_notifier(const std::size_t noise_size, bool is_public, bool pad_txs)
        {
            epee::byte_slice noise = nullptr;
            if (noise_size)
                noise = epee::levin::make_noise_notify(noise_size);
            epee::net_utils::zone zone = is_public ? epee::net_utils::zone::public_ : epee::net_utils::zone::i2p;
            receiver_.notifier.reset(
              new cryptonote::levin::notify{io_service_, connections_, std::move(noise), zone, pad_txs, events_}
            );
            return receiver_.notifier;
        }

        boost::uuids::random_generator random_generator_;
        boost::asio::io_service io_service_;
        test_receiver receiver_;
        std::deque<test_connection> contexts_;
        test_core_events events_;
    };
}

TEST(make_header, no_expect_return)
{
    static constexpr const std::size_t max_length = std::numeric_limits<std::size_t>::max();

    const epee::levin::bucket_head2 header1 = epee::levin::make_header(1024, max_length, 5601, false);
    EXPECT_EQ(SWAP64LE(LEVIN_SIGNATURE), header1.m_signature);
    EXPECT_FALSE(header1.m_have_to_return_data);
    EXPECT_EQ(SWAP64LE(max_length), header1.m_cb);
    EXPECT_EQ(SWAP32LE(1024), header1.m_command);
    EXPECT_EQ(SWAP32LE(LEVIN_PROTOCOL_VER_1), header1.m_protocol_version);
    EXPECT_EQ(SWAP32LE(5601), header1.m_flags);
}

TEST(make_header, expect_return)
{
    const epee::levin::bucket_head2 header1 = epee::levin::make_header(65535, 0, 0, true);
    EXPECT_EQ(SWAP64LE(LEVIN_SIGNATURE), header1.m_signature);
    EXPECT_TRUE(header1.m_have_to_return_data);
    EXPECT_EQ(0u, header1.m_cb);
    EXPECT_EQ(SWAP32LE(65535), header1.m_command);
    EXPECT_EQ(SWAP32LE(LEVIN_PROTOCOL_VER_1), header1.m_protocol_version);
    EXPECT_EQ(0u, header1.m_flags);
}

TEST(message_writer, invoke_with_empty_payload)
{
    const epee::byte_slice message = epee::levin::message_writer{}.finalize_invoke(443);
    const epee::levin::bucket_head2 header =
        epee::levin::make_header(443, 0, LEVIN_PACKET_REQUEST, true);
    ASSERT_EQ(sizeof(header), message.size());
    EXPECT_TRUE(std::memcmp(std::addressof(header), message.data(), sizeof(header)) == 0);
}

TEST(message_writer, invoke_with_payload)
{
    std::string bytes(100, 'a');
    std::generate(bytes.begin(), bytes.end(), crypto::random_device{});

    epee::levin::message_writer writer{};
    writer.buffer.write(epee::to_span(bytes));

    const epee::byte_slice message = writer.finalize_invoke(443);
    const epee::levin::bucket_head2 header =
        epee::levin::make_header(443, bytes.size(), LEVIN_PACKET_REQUEST, true);

    ASSERT_EQ(sizeof(header) + bytes.size(), message.size());
    EXPECT_TRUE(std::memcmp(std::addressof(header), message.data(), sizeof(header)) == 0);
    EXPECT_TRUE(std::memcmp(bytes.data(), message.data() + sizeof(header), bytes.size()) == 0);
}

TEST(message_writer, notify_with_empty_payload)
{
    const epee::byte_slice message = epee::levin::message_writer{}.finalize_notify(443);
    const epee::levin::bucket_head2 header =
        epee::levin::make_header(443, 0, LEVIN_PACKET_REQUEST, false);
    ASSERT_EQ(sizeof(header), message.size());
    EXPECT_TRUE(std::memcmp(std::addressof(header), message.data(), sizeof(header)) == 0);
}

TEST(message_writer, notify_with_payload)
{
    std::string bytes(100, 'a');
    std::generate(bytes.begin(), bytes.end(), crypto::random_device{});

    epee::levin::message_writer writer{};
    writer.buffer.write(epee::to_span(bytes));

    const epee::byte_slice message = writer.finalize_notify(443);
    const epee::levin::bucket_head2 header =
        epee::levin::make_header(443, bytes.size(), LEVIN_PACKET_REQUEST, false);

    ASSERT_EQ(sizeof(header) + bytes.size(), message.size());
    EXPECT_TRUE(std::memcmp(std::addressof(header), message.data(), sizeof(header)) == 0);
    EXPECT_TRUE(std::memcmp(bytes.data(), message.data() + sizeof(header), bytes.size()) == 0);
}

TEST(message_writer, response_with_empty_payload)
{
    const epee::byte_slice message = epee::levin::message_writer{}.finalize_response(443, 1);
    epee::levin::bucket_head2 header =
        epee::levin::make_header(443, 0, LEVIN_PACKET_RESPONSE, false);
    header.m_return_code = SWAP32LE(1);
    ASSERT_EQ(sizeof(header), message.size());
    EXPECT_TRUE(std::memcmp(std::addressof(header), message.data(), sizeof(header)) == 0);
}

TEST(message_writer, response_with_payload)
{
    std::string bytes(100, 'a');
    std::generate(bytes.begin(), bytes.end(), crypto::random_device{});

    epee::levin::message_writer writer{};
    writer.buffer.write(epee::to_span(bytes));

    const epee::byte_slice message = writer.finalize_response(443, 6450);
    epee::levin::bucket_head2 header =
        epee::levin::make_header(443, bytes.size(), LEVIN_PACKET_RESPONSE, false);
    header.m_return_code = SWAP32LE(6450);

    ASSERT_EQ(sizeof(header) + bytes.size(), message.size());
    EXPECT_TRUE(std::memcmp(std::addressof(header), message.data(), sizeof(header)) == 0);
    EXPECT_TRUE(std::memcmp(bytes.data(), message.data() + sizeof(header), bytes.size()) == 0);
}

TEST(message_writer, error)
{
    epee::levin::message_writer writer{};
    writer.buffer.clear();

    EXPECT_THROW(writer.finalize_invoke(0), std::runtime_error);
    EXPECT_THROW(writer.finalize_notify(0), std::runtime_error);
    EXPECT_THROW(writer.finalize_response(0, 0), std::runtime_error);
}

TEST(make_noise, invalid)
{
    EXPECT_TRUE(epee::levin::make_noise_notify(sizeof(epee::levin::bucket_head2) - 1).empty());
}

TEST(make_noise, valid)
{
    static constexpr const std::uint32_t flags =
        LEVIN_PACKET_BEGIN | LEVIN_PACKET_END;

    const epee::byte_slice noise = epee::levin::make_noise_notify(1024);
    const epee::levin::bucket_head2 header =
        epee::levin::make_header(0, 1024 - sizeof(epee::levin::bucket_head2), flags, false);

    ASSERT_EQ(1024, noise.size());
    EXPECT_TRUE(std::memcmp(std::addressof(header), noise.data(), sizeof(header)) == 0);
    EXPECT_EQ(1024 - sizeof(header), std::count(noise.cbegin() + sizeof(header), noise.cend(), 0));
}

TEST(make_fragment, invalid)
{
    EXPECT_TRUE(epee::levin::make_fragmented_notify(0, 0, epee::levin::message_writer{}).empty());
}

TEST(make_fragment, single)
{
    const epee::byte_slice noise = epee::levin::make_noise_notify(1024);
    const epee::byte_slice fragment = epee::levin::make_fragmented_notify(noise.size(), 11, epee::levin::message_writer{});
    const epee::levin::bucket_head2 header =
        epee::levin::make_header(11, 1024 - sizeof(epee::levin::bucket_head2), LEVIN_PACKET_REQUEST, false);

    EXPECT_EQ(1024, noise.size());
    ASSERT_EQ(1024, fragment.size());
    EXPECT_TRUE(std::memcmp(std::addressof(header), fragment.data(), sizeof(header)) == 0);
    EXPECT_EQ(1024 - sizeof(header), std::count(noise.cbegin() + sizeof(header), noise.cend(), 0));
}

TEST(make_fragment, multiple)
{
    std::string bytes(1024 * 3 - 150, 'a');
    std::generate(bytes.begin(), bytes.end(), crypto::random_device{});

    epee::levin::message_writer message;
    message.buffer.write(epee::to_span(bytes));

    const epee::byte_slice noise = epee::levin::make_noise_notify(1024);
    epee::byte_slice fragment = epee::levin::make_fragmented_notify(noise.size(), 114, std::move(message));

    EXPECT_EQ(1024 * 3, fragment.size());

    epee::levin::bucket_head2 header =
        epee::levin::make_header(0, 1024 - sizeof(epee::levin::bucket_head2), LEVIN_PACKET_BEGIN, false);

    ASSERT_LE(sizeof(header), fragment.size());
    EXPECT_TRUE(std::memcmp(std::addressof(header), fragment.data(), sizeof(header)) == 0);

    fragment.take_slice(sizeof(header));
    header.m_flags = LEVIN_PACKET_REQUEST;
    header.m_cb = bytes.size();
    header.m_command = 114;

    ASSERT_LE(sizeof(header), fragment.size());
    EXPECT_TRUE(std::memcmp(std::addressof(header), fragment.data(), sizeof(header)) == 0);

    fragment.take_slice(sizeof(header));

    ASSERT_LE(bytes.size(), fragment.size());
    EXPECT_TRUE(std::memcmp(bytes.data(), fragment.data(), 1024 - sizeof(header) * 2) == 0);

    bytes.erase(0, 1024 - sizeof(header) * 2);
    fragment.take_slice(1024 - sizeof(header) * 2);
    header.m_flags = 0;
    header.m_cb = 1024 - sizeof(header);
    header.m_command = 0;

    ASSERT_LE(sizeof(header), fragment.size());
    EXPECT_TRUE(std::memcmp(std::addressof(header), fragment.data(), sizeof(header)) == 0);

    fragment.take_slice(sizeof(header));

    ASSERT_LE(bytes.size(), fragment.size());
    EXPECT_TRUE(std::memcmp(bytes.data(), fragment.data(), 1024 - sizeof(header)) == 0);

    bytes.erase(0, 1024 - sizeof(header));
    fragment.take_slice(1024 - sizeof(header));
    header.m_flags = LEVIN_PACKET_END;

    ASSERT_LE(sizeof(header), fragment.size());
    EXPECT_TRUE(std::memcmp(std::addressof(header), fragment.data(), sizeof(header)) == 0);

    fragment.take_slice(sizeof(header));
    EXPECT_TRUE(std::memcmp(bytes.data(), fragment.data(), bytes.size()) == 0);

    fragment.take_slice(bytes.size());

    EXPECT_EQ(18, fragment.size());
    EXPECT_EQ(18, std::count(fragment.cbegin(), fragment.cend(), 0));
}

TEST_F(levin_notify, defaulted)
{
    cryptonote::levin::notify notifier{};
    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    EXPECT_TRUE(notifier.send_txs({}, random_generator_(), cryptonote::relay_method::local));

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'e');
    EXPECT_FALSE(notifier.send_txs(std::move(txs), random_generator_(), cryptonote::relay_method::local));
}

TEST_F(levin_notify, fluff_without_padding)
{
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, true, false);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'f');
    txs[1].resize(200, 'e');

    ASSERT_EQ(10u, contexts_.size());
    {
        auto context = contexts_.begin();
        EXPECT_TRUE(notifier.send_txs(txs, context->get_id(), cryptonote::relay_method::fluff));

        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
        notifier.run_fluff();
        ASSERT_LT(0u, io_service_.poll());

        EXPECT_EQ(0u, context->process_send_queue());
        for (++context; context != contexts_.end(); ++context)
            EXPECT_EQ(1u, context->process_send_queue());

        EXPECT_EQ(txs, events_.take_relayed(cryptonote::relay_method::fluff));
        std::sort(txs.begin(), txs.end());
        ASSERT_EQ(9u, receiver_.notified_size());
        for (unsigned count = 0; count < 9; ++count)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
            EXPECT_EQ(txs, notification.txs);
            EXPECT_TRUE(notification._.empty());
            EXPECT_TRUE(notification.dandelionpp_fluff);
        }
    }
}

TEST_F(levin_notify, stem_without_padding)
{
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, true, false);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'f');
    txs[1].resize(200, 'e');

    std::vector<cryptonote::blobdata> sorted_txs = txs;
    std::sort(sorted_txs.begin(), sorted_txs.end());

    ASSERT_EQ(10u, contexts_.size());
    bool has_stemmed = false;
    bool has_fluffed = false;
    while (!has_stemmed || !has_fluffed)
    {
        auto context = contexts_.begin();
        EXPECT_TRUE(notifier.send_txs(txs, context->get_id(), cryptonote::relay_method::stem));

        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
        const bool is_stem = events_.has_stem_txes();
        EXPECT_EQ(txs, events_.take_relayed(is_stem ? cryptonote::relay_method::stem : cryptonote::relay_method::fluff));

        if (!is_stem)
        {
            notifier.run_fluff();
            ASSERT_LT(0u, io_service_.poll());
        }

        std::size_t send_count = 0;
        EXPECT_EQ(0u, context->process_send_queue());
        for (++context; context != contexts_.end(); ++context)
        {
            const std::size_t sent = context->process_send_queue();
            if (sent && is_stem)
            {
                EXPECT_EQ(1u, (context - contexts_.begin()) % 2);
            }
            send_count += sent;
        }

        EXPECT_EQ(is_stem ? 1u : 9u, send_count);
        ASSERT_EQ(is_stem ? 1u : 9u, receiver_.notified_size());
        for (unsigned count = 0; count < (is_stem ? 1u : 9u); ++count)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
            if (is_stem)
              EXPECT_EQ(txs, notification.txs);
            else
              EXPECT_EQ(sorted_txs, notification.txs);
            EXPECT_TRUE(notification._.empty());
            EXPECT_EQ(!is_stem, notification.dandelionpp_fluff);
        }

        has_stemmed |= is_stem;
        has_fluffed |= !is_stem;
        notifier.run_epoch();
    }
}

TEST_F(levin_notify, stem_no_outs_without_padding)
{
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, true, false);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < 10; ++count)
        add_connection(true);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'f');
    txs[1].resize(200, 'e');

    std::vector<cryptonote::blobdata> sorted_txs = txs;
    std::sort(sorted_txs.begin(), sorted_txs.end());

    ASSERT_EQ(10u, contexts_.size());

    auto context = contexts_.begin();
    EXPECT_TRUE(notifier.send_txs(txs, context->get_id(), cryptonote::relay_method::stem));

    io_service_.reset();
    ASSERT_LT(0u, io_service_.poll());
    EXPECT_EQ(txs, events_.take_relayed(cryptonote::relay_method::fluff));
    if (events_.has_stem_txes())
    {
        EXPECT_EQ(txs, events_.take_relayed(cryptonote::relay_method::stem));
    }

    notifier.run_fluff();
    ASSERT_LT(0u, io_service_.poll());

    std::size_t send_count = 0;
    EXPECT_EQ(0u, context->process_send_queue());
    for (++context; context != contexts_.end(); ++context)
    {
        send_count += context->process_send_queue();
    }

    EXPECT_EQ(9u, send_count);
    ASSERT_EQ(9u, receiver_.notified_size());
    for (unsigned count = 0; count < 9u; ++count)
    {
        auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
        EXPECT_EQ(sorted_txs, notification.txs);
        EXPECT_TRUE(notification._.empty());
        EXPECT_TRUE(notification.dandelionpp_fluff);
    }
}

TEST_F(levin_notify, local_without_padding)
{
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, true, false);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> my_txs(2);
    my_txs[0].resize(100, 'f');
    my_txs[1].resize(200, 'e');

    std::vector<cryptonote::blobdata> their_txs{2};
    their_txs[0].resize(300, 'g');
    their_txs[1].resize(250, 'h');

    std::vector<cryptonote::blobdata> my_sorted_txs = my_txs;
    std::sort(my_sorted_txs.begin(), my_sorted_txs.end());

    std::vector<cryptonote::blobdata> their_sorted_txs = their_txs;
    std::sort(their_sorted_txs.begin(), their_sorted_txs.end());

    ASSERT_EQ(10u, contexts_.size());
    bool has_stemmed = false;
    bool has_fluffed = false;
    while (!has_stemmed || !has_fluffed)
    {
        // run their "their" txes first
        auto context = contexts_.begin();
        EXPECT_TRUE(notifier.send_txs(their_txs, context->get_id(), cryptonote::relay_method::stem));

        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
        const bool is_stem = events_.has_stem_txes();
        EXPECT_EQ(their_txs, events_.take_relayed(is_stem ? cryptonote::relay_method::stem : cryptonote::relay_method::fluff));

        if (!is_stem)
        {
            notifier.run_fluff();
            ASSERT_LT(0u, io_service_.poll());
        }

        std::size_t send_count = 0;
        EXPECT_EQ(0u, context->process_send_queue());
        for (++context; context != contexts_.end(); ++context)
        {
            const std::size_t sent = context->process_send_queue();
            if (sent && is_stem)
            {
                EXPECT_EQ(1u, (context - contexts_.begin()) % 2);
            }
            send_count += sent;
        }

        EXPECT_EQ(is_stem ? 1u : 9u, send_count);
        ASSERT_EQ(is_stem ? 1u : 9u, receiver_.notified_size());
        for (unsigned count = 0; count < (is_stem ? 1u : 9u); ++count)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
	    if (is_stem)
	      EXPECT_EQ(their_txs, notification.txs);
	    else
	      EXPECT_EQ(their_sorted_txs, notification.txs);
            EXPECT_TRUE(notification._.empty());
            EXPECT_EQ(!is_stem, notification.dandelionpp_fluff);
        }

        // run "my" txes which must always be stem
        context = contexts_.begin();
        EXPECT_TRUE(notifier.send_txs(my_txs, context->get_id(), cryptonote::relay_method::local));

        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
        EXPECT_TRUE(events_.has_stem_txes());
        EXPECT_EQ(my_txs, events_.take_relayed(cryptonote::relay_method::stem));

        send_count = 0;
        EXPECT_EQ(0u, context->process_send_queue());
        for (++context; context != contexts_.end(); ++context)
        {
            const std::size_t sent = context->process_send_queue();
            if (sent)
            {
                EXPECT_EQ(1u, (context - contexts_.begin()) % 2);
            }
            send_count += sent;
        }

        EXPECT_EQ(1u, send_count);
        EXPECT_EQ(1u, receiver_.notified_size());
        auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
        EXPECT_EQ(my_txs, notification.txs);
        EXPECT_TRUE(notification._.empty());
        EXPECT_TRUE(!notification.dandelionpp_fluff);

        has_stemmed |= is_stem;
        has_fluffed |= !is_stem;
        notifier.run_epoch();
    }
}

TEST_F(levin_notify, forward_without_padding)
{
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, true, false);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'f');
    txs[1].resize(200, 'e');

    std::vector<cryptonote::blobdata> sorted_txs = txs;
    std::sort(sorted_txs.begin(), sorted_txs.end());

    ASSERT_EQ(10u, contexts_.size());
    bool has_stemmed = false;
    bool has_fluffed = false;
    while (!has_stemmed || !has_fluffed)
    {
        auto context = contexts_.begin();
        EXPECT_TRUE(notifier.send_txs(txs, context->get_id(), cryptonote::relay_method::forward));

        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
        const bool is_stem = events_.has_stem_txes();
        EXPECT_EQ(txs, events_.take_relayed(is_stem ? cryptonote::relay_method::stem : cryptonote::relay_method::fluff));

        if (!is_stem)
        {
            notifier.run_fluff();
            ASSERT_LT(0u, io_service_.poll());
        }

        std::size_t send_count = 0;
        EXPECT_EQ(0u, context->process_send_queue());
        for (++context; context != contexts_.end(); ++context)
        {
            const std::size_t sent = context->process_send_queue();
            if (sent && is_stem)
            {
                EXPECT_EQ(1u, (context - contexts_.begin()) % 2);
            }
            send_count += sent;
        }

        EXPECT_EQ(is_stem ? 1u : 9u, send_count);
        ASSERT_EQ(is_stem ? 1u : 9u, receiver_.notified_size());
        for (unsigned count = 0; count < (is_stem ? 1u : 9u); ++count)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
	    if (is_stem)
	      EXPECT_EQ(txs, notification.txs);
	    else
	      EXPECT_EQ(sorted_txs, notification.txs);
            EXPECT_TRUE(notification._.empty());
            EXPECT_EQ(!is_stem, notification.dandelionpp_fluff);
        }

        has_stemmed |= is_stem;
        has_fluffed |= !is_stem;
        notifier.run_epoch();
    }
}

TEST_F(levin_notify, block_without_padding)
{
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, true, false);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'e');
    txs[1].resize(200, 'f');

    ASSERT_EQ(10u, contexts_.size());
    {
        auto context = contexts_.begin();
        EXPECT_FALSE(notifier.send_txs(txs, context->get_id(), cryptonote::relay_method::block));

        io_service_.reset();
        ASSERT_EQ(0u, io_service_.poll());
    }
}

TEST_F(levin_notify, none_without_padding)
{
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, true, false);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'e');
    txs[1].resize(200, 'f');

    ASSERT_EQ(10u, contexts_.size());
    {
        auto context = contexts_.begin();
        EXPECT_FALSE(notifier.send_txs(txs, context->get_id(), cryptonote::relay_method::none));

        io_service_.reset();
        ASSERT_EQ(0u, io_service_.poll());
    }
}

TEST_F(levin_notify, fluff_with_padding)
{
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, true, true);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'f');
    txs[1].resize(200, 'e');

    ASSERT_EQ(10u, contexts_.size());
    {
        auto context = contexts_.begin();
        EXPECT_TRUE(notifier.send_txs(txs, context->get_id(), cryptonote::relay_method::fluff));

        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
        notifier.run_fluff();
        ASSERT_LT(0u, io_service_.poll());

        EXPECT_EQ(txs, events_.take_relayed(cryptonote::relay_method::fluff));
        std::sort(txs.begin(), txs.end());
        EXPECT_EQ(0u, context->process_send_queue());
        for (++context; context != contexts_.end(); ++context)
            EXPECT_EQ(1u, context->process_send_queue());

        ASSERT_EQ(9u, receiver_.notified_size());
        for (unsigned count = 0; count < 9; ++count)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
            EXPECT_EQ(txs, notification.txs);
            EXPECT_FALSE(notification._.empty());
            EXPECT_TRUE(notification.dandelionpp_fluff);
        }
    }
}

TEST_F(levin_notify, stem_with_padding)
{
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, true, true);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'e');
    txs[1].resize(200, 'f');

    ASSERT_EQ(10u, contexts_.size());
    bool has_stemmed = false;
    bool has_fluffed = false;
    while (!has_stemmed || !has_fluffed)
    {
        auto context = contexts_.begin();
        EXPECT_TRUE(notifier.send_txs(txs, context->get_id(), cryptonote::relay_method::stem));

        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
        const bool is_stem = events_.has_stem_txes();
        EXPECT_EQ(txs, events_.take_relayed(is_stem ? cryptonote::relay_method::stem : cryptonote::relay_method::fluff));

        if (!is_stem)
        {
            notifier.run_fluff();
            ASSERT_LT(0u, io_service_.poll());
        }

        std::size_t send_count = 0;
        EXPECT_EQ(0u, context->process_send_queue());
        for (++context; context != contexts_.end(); ++context)
        {
            const std::size_t sent = context->process_send_queue();
            if (sent && is_stem)
            {
                EXPECT_EQ(1u, (context - contexts_.begin()) % 2);
                EXPECT_FALSE(context->is_incoming());
            }
            send_count += sent;
        }

        EXPECT_EQ(is_stem ? 1u : 9u, send_count);
        ASSERT_EQ(is_stem ? 1u : 9u, receiver_.notified_size());
        for (unsigned count = 0; count < (is_stem ? 1u : 9u); ++count)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
            EXPECT_EQ(txs, notification.txs);
            EXPECT_FALSE(notification._.empty());
            EXPECT_EQ(!is_stem, notification.dandelionpp_fluff);
        }

        has_stemmed |= is_stem;
        has_fluffed |= !is_stem;
        notifier.run_epoch();
    }
}

TEST_F(levin_notify, stem_no_outs_with_padding)
{
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, true, true);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < 10; ++count)
        add_connection(true);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'f');
    txs[1].resize(200, 'e');

    std::vector<cryptonote::blobdata> sorted_txs = txs;
    std::sort(sorted_txs.begin(), sorted_txs.end());

    ASSERT_EQ(10u, contexts_.size());

    auto context = contexts_.begin();
    EXPECT_TRUE(notifier.send_txs(txs, context->get_id(), cryptonote::relay_method::stem));

    io_service_.reset();
    ASSERT_LT(0u, io_service_.poll());
    EXPECT_EQ(txs, events_.take_relayed(cryptonote::relay_method::fluff));
    if (events_.has_stem_txes())
    {
        EXPECT_EQ(txs, events_.take_relayed(cryptonote::relay_method::stem));
    }

    notifier.run_fluff();
    ASSERT_LT(0u, io_service_.poll());

    std::size_t send_count = 0;
    EXPECT_EQ(0u, context->process_send_queue());
    for (++context; context != contexts_.end(); ++context)
    {
        send_count += context->process_send_queue();
    }

    EXPECT_EQ(9u, send_count);
    ASSERT_EQ(9u, receiver_.notified_size());
    for (unsigned count = 0; count < 9u; ++count)
    {
        auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
        EXPECT_EQ(sorted_txs, notification.txs);
        EXPECT_FALSE(notification._.empty());
        EXPECT_TRUE(notification.dandelionpp_fluff);
    }
}

TEST_F(levin_notify, local_with_padding)
{
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, true, true);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> my_txs(2);
    my_txs[0].resize(100, 'e');
    my_txs[1].resize(200, 'f');

    std::vector<cryptonote::blobdata> their_txs{2};
    their_txs[0].resize(300, 'g');
    their_txs[1].resize(250, 'h');

    ASSERT_EQ(10u, contexts_.size());
    bool has_stemmed = false;
    bool has_fluffed = false;
    while (!has_stemmed || !has_fluffed)
    {
      // run their "their" txes first
        auto context = contexts_.begin();
        EXPECT_TRUE(notifier.send_txs(their_txs, context->get_id(), cryptonote::relay_method::stem));

        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
        const bool is_stem = events_.has_stem_txes();
        EXPECT_EQ(their_txs, events_.take_relayed(is_stem ? cryptonote::relay_method::stem : cryptonote::relay_method::fluff));

        if (!is_stem)
        {
            notifier.run_fluff();
            ASSERT_LT(0u, io_service_.poll());
        }

        std::size_t send_count = 0;
        EXPECT_EQ(0u, context->process_send_queue());
        for (++context; context != contexts_.end(); ++context)
        {
            const std::size_t sent = context->process_send_queue();
            if (sent && is_stem)
            {
                EXPECT_EQ(1u, (context - contexts_.begin()) % 2);
                EXPECT_FALSE(context->is_incoming());
            }
            send_count += sent;
        }

        EXPECT_EQ(is_stem ? 1u : 9u, send_count);
        ASSERT_EQ(is_stem ? 1u : 9u, receiver_.notified_size());
        for (unsigned count = 0; count < (is_stem ? 1u : 9u); ++count)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
            EXPECT_EQ(their_txs, notification.txs);
            EXPECT_FALSE(notification._.empty());
            EXPECT_EQ(!is_stem, notification.dandelionpp_fluff);
        }

        // run "my" txes which must always be stem
        context = contexts_.begin();
        EXPECT_TRUE(notifier.send_txs(my_txs, context->get_id(), cryptonote::relay_method::local));

        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
        EXPECT_TRUE(events_.has_stem_txes());
        EXPECT_EQ(my_txs, events_.take_relayed(cryptonote::relay_method::stem));

        send_count = 0;
        EXPECT_EQ(0u, context->process_send_queue());
        for (++context; context != contexts_.end(); ++context)
        {
            const std::size_t sent = context->process_send_queue();
            if (sent)
            {
                EXPECT_EQ(1u, (context - contexts_.begin()) % 2);
            }
            send_count += sent;
        }

        EXPECT_EQ(1u, send_count);
        EXPECT_EQ(1u, receiver_.notified_size());
        auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
        EXPECT_EQ(my_txs, notification.txs);
        EXPECT_FALSE(notification._.empty());
        EXPECT_TRUE(!notification.dandelionpp_fluff);

        has_stemmed |= is_stem;
        has_fluffed |= !is_stem;
        notifier.run_epoch();
    }
}

TEST_F(levin_notify, forward_with_padding)
{
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, true, true);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'e');
    txs[1].resize(200, 'f');

    ASSERT_EQ(10u, contexts_.size());
    bool has_stemmed = false;
    bool has_fluffed = false;
    while (!has_stemmed || !has_fluffed)
    {
        auto context = contexts_.begin();
        EXPECT_TRUE(notifier.send_txs(txs, context->get_id(), cryptonote::relay_method::forward));

        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
        const bool is_stem = events_.has_stem_txes();
        EXPECT_EQ(txs, events_.take_relayed(is_stem ? cryptonote::relay_method::stem : cryptonote::relay_method::fluff));

        if (!is_stem)
        {
            notifier.run_fluff();
            ASSERT_LT(0u, io_service_.poll());
        }

        std::size_t send_count = 0;
        EXPECT_EQ(0u, context->process_send_queue());
        for (++context; context != contexts_.end(); ++context)
        {
            const std::size_t sent = context->process_send_queue();
            if (sent && is_stem)
            {
                EXPECT_EQ(1u, (context - contexts_.begin()) % 2);
                EXPECT_FALSE(context->is_incoming());
            }
            send_count += sent;
        }

        EXPECT_EQ(is_stem ? 1u : 9u, send_count);
        ASSERT_EQ(is_stem ? 1u : 9u, receiver_.notified_size());
        for (unsigned count = 0; count < (is_stem ? 1u : 9u); ++count)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
            EXPECT_EQ(txs, notification.txs);
            EXPECT_FALSE(notification._.empty());
            EXPECT_EQ(!is_stem, notification.dandelionpp_fluff);
        }

        has_stemmed |= is_stem;
        has_fluffed |= !is_stem;
        notifier.run_epoch();
    }
}

TEST_F(levin_notify, block_with_padding)
{
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, true, true);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'e');
    txs[1].resize(200, 'f');

    ASSERT_EQ(10u, contexts_.size());
    {
        auto context = contexts_.begin();
        EXPECT_FALSE(notifier.send_txs(txs, context->get_id(), cryptonote::relay_method::block));

        io_service_.reset();
        ASSERT_EQ(0u, io_service_.poll());
    }
}

TEST_F(levin_notify, none_with_padding)
{
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, true, true);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'e');
    txs[1].resize(200, 'f');

    ASSERT_EQ(10u, contexts_.size());
    {
        auto context = contexts_.begin();
        EXPECT_FALSE(notifier.send_txs(txs, context->get_id(), cryptonote::relay_method::none));

        io_service_.reset();
        ASSERT_EQ(0u, io_service_.poll());
    }
}

TEST_F(levin_notify, private_fluff_without_padding)
{
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, false, false);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'e');
    txs[1].resize(200, 'f');

    ASSERT_EQ(10u, contexts_.size());
    {
        auto context = contexts_.begin();
        EXPECT_TRUE(notifier.send_txs(txs, context->get_id(), cryptonote::relay_method::fluff));

        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
        notifier.run_fluff();
        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());

        EXPECT_EQ(txs, events_.take_relayed(cryptonote::relay_method::fluff));

        EXPECT_EQ(0u, context->process_send_queue());
        for (++context; context != contexts_.end(); ++context)
        {
            const bool is_incoming = ((context - contexts_.begin()) % 2 == 0);
            EXPECT_EQ(is_incoming ? 0u : 1u, context->process_send_queue());
        }

        ASSERT_EQ(5u, receiver_.notified_size());
        for (unsigned count = 0; count < 5; ++count)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
            EXPECT_EQ(txs, notification.txs);
            EXPECT_TRUE(notification._.empty());
            EXPECT_TRUE(notification.dandelionpp_fluff);
        }
    }
}

TEST_F(levin_notify, private_stem_without_padding)
{
    // private mode always uses fluff but marked as stem
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, false, false);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'e');
    txs[1].resize(200, 'f');

    ASSERT_EQ(10u, contexts_.size());
    {
        auto context = contexts_.begin();
        EXPECT_TRUE(notifier.send_txs(txs, context->get_id(), cryptonote::relay_method::stem));

        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
        notifier.run_fluff();
        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());

        EXPECT_EQ(txs, events_.take_relayed(cryptonote::relay_method::stem));

        EXPECT_EQ(0u, context->process_send_queue());
        for (++context; context != contexts_.end(); ++context)
        {
            const bool is_incoming = ((context - contexts_.begin()) % 2 == 0);
            EXPECT_EQ(is_incoming ? 0u : 1u, context->process_send_queue());
        }

        ASSERT_EQ(5u, receiver_.notified_size());
        for (unsigned count = 0; count < 5; ++count)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
            EXPECT_EQ(txs, notification.txs);
            EXPECT_TRUE(notification._.empty());
            EXPECT_TRUE(notification.dandelionpp_fluff);
        }
    }
}

TEST_F(levin_notify, private_local_without_padding)
{
    // private mode always uses fluff but marked as stem
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, false, false);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'e');
    txs[1].resize(200, 'f');

    ASSERT_EQ(10u, contexts_.size());
    {
        auto context = contexts_.begin();
        EXPECT_TRUE(notifier.send_txs(txs, context->get_id(), cryptonote::relay_method::local));

        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
        notifier.run_fluff();
        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());

        EXPECT_EQ(txs, events_.take_relayed(cryptonote::relay_method::local));

        EXPECT_EQ(0u, context->process_send_queue());
        for (++context; context != contexts_.end(); ++context)
        {
            const bool is_incoming = ((context - contexts_.begin()) % 2 == 0);
            EXPECT_EQ(is_incoming ? 0u : 1u, context->process_send_queue());
        }

        ASSERT_EQ(5u, receiver_.notified_size());
        for (unsigned count = 0; count < 5; ++count)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
            EXPECT_EQ(txs, notification.txs);
            EXPECT_TRUE(notification._.empty());
            EXPECT_TRUE(notification.dandelionpp_fluff);
        }
    }
}

TEST_F(levin_notify, private_forward_without_padding)
{
    // private mode always uses fluff but marked as stem
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, false, false);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'e');
    txs[1].resize(200, 'f');

    ASSERT_EQ(10u, contexts_.size());
    {
        auto context = contexts_.begin();
        EXPECT_TRUE(notifier.send_txs(txs, context->get_id(), cryptonote::relay_method::forward));

        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
        notifier.run_fluff();
        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());

        EXPECT_EQ(txs, events_.take_relayed(cryptonote::relay_method::forward));

        EXPECT_EQ(0u, context->process_send_queue());
        for (++context; context != contexts_.end(); ++context)
        {
            const bool is_incoming = ((context - contexts_.begin()) % 2 == 0);
            EXPECT_EQ(is_incoming ? 0u : 1u, context->process_send_queue());
        }

        ASSERT_EQ(5u, receiver_.notified_size());
        for (unsigned count = 0; count < 5; ++count)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
            EXPECT_EQ(txs, notification.txs);
            EXPECT_TRUE(notification._.empty());
            EXPECT_TRUE(notification.dandelionpp_fluff);
        }
    }
}

TEST_F(levin_notify, private_block_without_padding)
{
    // private mode always uses fluff but marked as stem
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, false, false);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'e');
    txs[1].resize(200, 'f');

    ASSERT_EQ(10u, contexts_.size());
    {
        auto context = contexts_.begin();
        EXPECT_FALSE(notifier.send_txs(txs, context->get_id(), cryptonote::relay_method::block));

        io_service_.reset();
        ASSERT_EQ(0u, io_service_.poll());
    }
}

TEST_F(levin_notify, private_none_without_padding)
{
    // private mode always uses fluff but marked as stem
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, false, false);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'e');
    txs[1].resize(200, 'f');

    ASSERT_EQ(10u, contexts_.size());
    {
        auto context = contexts_.begin();
        EXPECT_FALSE(notifier.send_txs(txs, context->get_id(), cryptonote::relay_method::none));

        io_service_.reset();
        ASSERT_EQ(0u, io_service_.poll());
    }
}

TEST_F(levin_notify, private_fluff_with_padding)
{
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, false, true);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'e');
    txs[1].resize(200, 'f');

    ASSERT_EQ(10u, contexts_.size());
    {
        auto context = contexts_.begin();
        EXPECT_TRUE(notifier.send_txs(txs, context->get_id(), cryptonote::relay_method::fluff));

        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
        notifier.run_fluff();
        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());

        EXPECT_EQ(txs, events_.take_relayed(cryptonote::relay_method::fluff));

        EXPECT_EQ(0u, context->process_send_queue());
        for (++context; context != contexts_.end(); ++context)
        {
            const bool is_incoming = ((context - contexts_.begin()) % 2 == 0);
            EXPECT_EQ(is_incoming ? 0u : 1u, context->process_send_queue());
        }

        ASSERT_EQ(5u, receiver_.notified_size());
        for (unsigned count = 0; count < 5; ++count)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
            EXPECT_EQ(txs, notification.txs);
            EXPECT_FALSE(notification._.empty());
            EXPECT_TRUE(notification.dandelionpp_fluff);
        }
    }
}

TEST_F(levin_notify, private_stem_with_padding)
{
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, false, true);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'e');
    txs[1].resize(200, 'f');

    ASSERT_EQ(10u, contexts_.size());
    {
        auto context = contexts_.begin();
        EXPECT_TRUE(notifier.send_txs(txs, context->get_id(), cryptonote::relay_method::stem));

        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
        notifier.run_fluff();
        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());

        EXPECT_EQ(txs, events_.take_relayed(cryptonote::relay_method::stem));

        EXPECT_EQ(0u, context->process_send_queue());
        for (++context; context != contexts_.end(); ++context)
        {
            const bool is_incoming = ((context - contexts_.begin()) % 2 == 0);
            EXPECT_EQ(is_incoming ? 0u : 1u, context->process_send_queue());
        }

        ASSERT_EQ(5u, receiver_.notified_size());
        for (unsigned count = 0; count < 5; ++count)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
            EXPECT_EQ(txs, notification.txs);
            EXPECT_FALSE(notification._.empty());
            EXPECT_TRUE(notification.dandelionpp_fluff);
        }
    }
}

TEST_F(levin_notify, private_local_with_padding)
{
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, false, true);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'e');
    txs[1].resize(200, 'f');

    ASSERT_EQ(10u, contexts_.size());
    {
        auto context = contexts_.begin();
        EXPECT_TRUE(notifier.send_txs(txs, context->get_id(), cryptonote::relay_method::local));

        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
        notifier.run_fluff();
        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());

        EXPECT_EQ(txs, events_.take_relayed(cryptonote::relay_method::local));

        EXPECT_EQ(0u, context->process_send_queue());
        for (++context; context != contexts_.end(); ++context)
        {
            const bool is_incoming = ((context - contexts_.begin()) % 2 == 0);
            EXPECT_EQ(is_incoming ? 0u : 1u, context->process_send_queue());
        }

        ASSERT_EQ(5u, receiver_.notified_size());
        for (unsigned count = 0; count < 5; ++count)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
            EXPECT_EQ(txs, notification.txs);
            EXPECT_FALSE(notification._.empty());
            EXPECT_TRUE(notification.dandelionpp_fluff);
        }
    }
}

TEST_F(levin_notify, private_forward_with_padding)
{
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, false, true);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'e');
    txs[1].resize(200, 'f');

    ASSERT_EQ(10u, contexts_.size());
    {
        auto context = contexts_.begin();
        EXPECT_TRUE(notifier.send_txs(txs, context->get_id(), cryptonote::relay_method::forward));

        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
        notifier.run_fluff();
        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());

        EXPECT_EQ(txs, events_.take_relayed(cryptonote::relay_method::forward));

        EXPECT_EQ(0u, context->process_send_queue());
        for (++context; context != contexts_.end(); ++context)
        {
            const bool is_incoming = ((context - contexts_.begin()) % 2 == 0);
            EXPECT_EQ(is_incoming ? 0u : 1u, context->process_send_queue());
        }

        ASSERT_EQ(5u, receiver_.notified_size());
        for (unsigned count = 0; count < 5; ++count)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
            EXPECT_EQ(txs, notification.txs);
            EXPECT_FALSE(notification._.empty());
            EXPECT_TRUE(notification.dandelionpp_fluff);
        }
    }
}

TEST_F(levin_notify, private_block_with_padding)
{
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, false, true);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'e');
    txs[1].resize(200, 'f');

    ASSERT_EQ(10u, contexts_.size());
    {
        auto context = contexts_.begin();
        EXPECT_FALSE(notifier.send_txs(txs, context->get_id(), cryptonote::relay_method::block));

        io_service_.reset();
        ASSERT_EQ(0u, io_service_.poll());
    }
}

TEST_F(levin_notify, private_none_with_padding)
{
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, false, true);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'e');
    txs[1].resize(200, 'f');

    ASSERT_EQ(10u, contexts_.size());
    {
        auto context = contexts_.begin();
        EXPECT_FALSE(notifier.send_txs(txs, context->get_id(), cryptonote::relay_method::none));

        io_service_.reset();
        ASSERT_EQ(0u, io_service_.poll());
    }
}

TEST_F(levin_notify, stem_mappings)
{
    static constexpr const unsigned test_connections_count = (CRYPTONOTE_DANDELIONPP_STEMS + 1) * 2;

    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, true, false);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < test_connections_count; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'e');
    txs[1].resize(200, 'f');

    ASSERT_EQ(test_connections_count, contexts_.size());
    for (;;)
    {
        auto context = contexts_.begin();
        EXPECT_TRUE(notifier.send_txs(txs, context->get_id(), cryptonote::relay_method::stem));

        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
        if (events_.has_stem_txes())
            break;

        EXPECT_EQ(txs, events_.take_relayed(cryptonote::relay_method::fluff));
        notifier.run_fluff();
        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());

        EXPECT_EQ(0u, context->process_send_queue());
        for (++context; context != contexts_.end(); ++context)
            EXPECT_EQ(1u, context->process_send_queue());

        ASSERT_EQ(test_connections_count - 1, receiver_.notified_size());
        for (unsigned count = 0; count < test_connections_count - 1; ++count)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
            EXPECT_EQ(txs, notification.txs);
            EXPECT_TRUE(notification._.empty());
            EXPECT_TRUE(notification.dandelionpp_fluff);
        }

        notifier.run_epoch();
        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
    }
    EXPECT_EQ(txs, events_.take_relayed(cryptonote::relay_method::stem));

    std::set<boost::uuids::uuid> used;
    std::map<boost::uuids::uuid, boost::uuids::uuid> mappings;
    {
        std::size_t send_count = 0;
        for (auto context = contexts_.begin(); context != contexts_.end(); ++context)
        {
            const std::size_t sent = context->process_send_queue();
            if (sent)
            {
                EXPECT_EQ(1u, (context - contexts_.begin()) % 2);
                EXPECT_FALSE(context->is_incoming());
                used.insert(context->get_id());
                mappings[contexts_.front().get_id()] = context->get_id();
            }
            send_count += sent;
        }

        EXPECT_EQ(1u, send_count);
        ASSERT_EQ(1u, receiver_.notified_size());
        for (unsigned count = 0; count < 1u; ++count)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
            EXPECT_EQ(txs, notification.txs);
            EXPECT_TRUE(notification._.empty());
            EXPECT_FALSE(notification.dandelionpp_fluff);
        }
    }

    for (unsigned i = 0; i < contexts_.size() * 2; i += 2)
    {
        auto& incoming = contexts_[i % contexts_.size()];
        EXPECT_TRUE(notifier.send_txs(txs, incoming.get_id(), cryptonote::relay_method::stem));

        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
        EXPECT_EQ(txs, events_.take_relayed(cryptonote::relay_method::stem));

        std::size_t send_count = 0;
        for (auto context = contexts_.begin(); context != contexts_.end(); ++context)
        {
            const std::size_t sent = context->process_send_queue();
            if (sent)
            {
                EXPECT_EQ(1u, (context - contexts_.begin()) % 2);
                EXPECT_FALSE(context->is_incoming());
                used.insert(context->get_id());

                auto inserted = mappings.emplace(incoming.get_id(), context->get_id()).first;
                EXPECT_EQ(inserted->second, context->get_id()) << "incoming index " << i;
            }
            send_count += sent;
        }

        EXPECT_EQ(1u, send_count);
        ASSERT_EQ(1u, receiver_.notified_size());
        for (unsigned count = 0; count < 1u; ++count)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
            EXPECT_EQ(txs, notification.txs);
            EXPECT_TRUE(notification._.empty());
            EXPECT_FALSE(notification.dandelionpp_fluff);
        }
    }

    EXPECT_EQ(CRYPTONOTE_DANDELIONPP_STEMS, used.size());
}

TEST_F(levin_notify, fluff_multiple)
{
    static constexpr const unsigned test_connections_count = (CRYPTONOTE_DANDELIONPP_STEMS + 1) * 2;

    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(0, true, false);
    auto &notifier = *notifier_ptr;

    for (unsigned count = 0; count < test_connections_count; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'e');
    txs[1].resize(200, 'f');

    ASSERT_EQ(test_connections_count, contexts_.size());
    for (;;)
    {
        auto context = contexts_.begin();
        EXPECT_TRUE(notifier.send_txs(txs, context->get_id(), cryptonote::relay_method::stem));

        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
        if (!events_.has_stem_txes())
            break;

        EXPECT_EQ(txs, events_.take_relayed(cryptonote::relay_method::stem));

        std::size_t send_count = 0;
        EXPECT_EQ(0u, context->process_send_queue());
        for (++context; context != contexts_.end(); ++context)
        {
            const std::size_t sent = context->process_send_queue();
            if (sent)
            {
                EXPECT_EQ(1u, (context - contexts_.begin()) % 2);
                EXPECT_FALSE(context->is_incoming());
            }
            send_count += sent;
        }

        EXPECT_EQ(1u, send_count);
        ASSERT_EQ(1u, receiver_.notified_size());
        for (unsigned count = 0; count < 1; ++count)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
            EXPECT_EQ(txs, notification.txs);
            EXPECT_TRUE(notification._.empty());
            EXPECT_FALSE(notification.dandelionpp_fluff);
        }

        notifier.run_epoch();
        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
    }
    EXPECT_EQ(txs, events_.take_relayed(cryptonote::relay_method::fluff));
    notifier.run_fluff();
    io_service_.reset();
    ASSERT_LT(0u, io_service_.poll());
    {
        auto context = contexts_.begin();
        EXPECT_EQ(0u, context->process_send_queue());
        for (++context; context != contexts_.end(); ++context)
            EXPECT_EQ(1u, context->process_send_queue());

        ASSERT_EQ(contexts_.size() - 1, receiver_.notified_size());
        for (unsigned count = 0; count < contexts_.size() - 1; ++count)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
            EXPECT_EQ(txs, notification.txs);
            EXPECT_TRUE(notification._.empty());
            EXPECT_TRUE(notification.dandelionpp_fluff);
        }
    }

    for (unsigned i = 0; i < contexts_.size() * 2; i += 2)
    {
        auto& incoming = contexts_[i % contexts_.size()];
        EXPECT_TRUE(notifier.send_txs(txs, incoming.get_id(), cryptonote::relay_method::stem));

        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
        notifier.run_fluff();
        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());

        EXPECT_EQ(txs, events_.take_relayed(cryptonote::relay_method::fluff));

        for (auto& context : contexts_)
        {
            if (std::addressof(incoming) == std::addressof(context))
                EXPECT_EQ(0u, context.process_send_queue());
            else
                EXPECT_EQ(1u, context.process_send_queue());
        }

        ASSERT_EQ(contexts_.size() - 1, receiver_.notified_size());
        for (unsigned count = 0; count < contexts_.size() - 1; ++count)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
            EXPECT_EQ(txs, notification.txs);
            EXPECT_TRUE(notification._.empty());
            EXPECT_TRUE(notification.dandelionpp_fluff);
        }
    }
}

TEST_F(levin_notify, noise)
{
    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    std::vector<cryptonote::blobdata> txs(1);
    txs[0].resize(1900, 'h');

    const boost::uuids::uuid incoming_id = random_generator_();
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(2048, false, true);
    auto &notifier = *notifier_ptr;

    {
        const auto status = notifier.get_status();
        EXPECT_TRUE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    ASSERT_LT(0u, io_service_.poll());
    {
        const auto status = notifier.get_status();
        EXPECT_TRUE(status.has_noise);
        EXPECT_TRUE(status.connections_filled);
    }

    notifier.run_stems();
    io_service_.reset();
    ASSERT_LT(0u, io_service_.poll());
    {
        std::size_t sent = 0;
        for (auto& context : contexts_)
            sent += context.process_send_queue();

        EXPECT_EQ(2u, sent);
        EXPECT_EQ(0u, receiver_.notified_size());
    }

    EXPECT_TRUE(notifier.send_txs(txs, incoming_id, cryptonote::relay_method::local));
    notifier.run_stems();
    io_service_.reset();
    ASSERT_LT(0u, io_service_.poll());

    EXPECT_EQ(txs, events_.take_relayed(cryptonote::relay_method::local));
    {
        std::size_t sent = 0;
        for (auto& context : contexts_)
            sent += context.process_send_queue();

        ASSERT_EQ(2u, sent);
        while (sent--)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
            EXPECT_EQ(txs, notification.txs);
            EXPECT_TRUE(notification._.empty());
            EXPECT_FALSE(notification.dandelionpp_fluff);
        }
    }

    txs[0].resize(3000, 'r');
    EXPECT_TRUE(notifier.send_txs(txs, incoming_id, cryptonote::relay_method::fluff));
    notifier.run_stems();
    io_service_.reset();
    ASSERT_LT(0u, io_service_.poll());

    EXPECT_EQ(txs, events_.take_relayed(cryptonote::relay_method::fluff));
    {
        std::size_t sent = 0;
        for (auto& context : contexts_)
            sent += context.process_send_queue();

        EXPECT_EQ(2u, sent);
        EXPECT_EQ(0u, receiver_.notified_size());
    }

    notifier.run_stems();
    io_service_.reset();
    ASSERT_LT(0u, io_service_.poll());
    {
        std::size_t sent = 0;
        for (auto& context : contexts_)
            sent += context.process_send_queue();

        ASSERT_EQ(2u, sent);
        while (sent--)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
            EXPECT_EQ(txs, notification.txs);
            EXPECT_TRUE(notification._.empty());
            EXPECT_FALSE(notification.dandelionpp_fluff);
        }
    }
}

TEST_F(levin_notify, noise_stem)
{
    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    std::vector<cryptonote::blobdata> txs(1);
    txs[0].resize(1900, 'h');

    const boost::uuids::uuid incoming_id = random_generator_();
    std::shared_ptr<cryptonote::levin::notify> notifier_ptr = make_notifier(2048, false, true);
    auto &notifier = *notifier_ptr;

    {
        const auto status = notifier.get_status();
        EXPECT_TRUE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    ASSERT_LT(0u, io_service_.poll());
    {
        const auto status = notifier.get_status();
        EXPECT_TRUE(status.has_noise);
        EXPECT_TRUE(status.connections_filled);
    }

    notifier.run_stems();
    io_service_.reset();
    ASSERT_LT(0u, io_service_.poll());
    {
        std::size_t sent = 0;
        for (auto& context : contexts_)
            sent += context.process_send_queue();

        EXPECT_EQ(2u, sent);
        EXPECT_EQ(0u, receiver_.notified_size());
    }

    EXPECT_TRUE(notifier.send_txs(txs, incoming_id, cryptonote::relay_method::stem));
    notifier.run_stems();
    io_service_.reset();
    ASSERT_LT(0u, io_service_.poll());

    // downgraded to local when being notified
    EXPECT_EQ(txs, events_.take_relayed(cryptonote::relay_method::local));
    {
        std::size_t sent = 0;
        for (auto& context : contexts_)
            sent += context.process_send_queue();

        ASSERT_EQ(2u, sent);
        while (sent--)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
            EXPECT_EQ(txs, notification.txs);
            EXPECT_TRUE(notification._.empty());
            EXPECT_FALSE(notification.dandelionpp_fluff);
        }
    }
}

TEST_F(levin_notify, command_max_bytes)
{
    static constexpr int ping_command = nodetool::COMMAND_PING::ID;

    add_connection(true);

    std::string payload(4096, 'h');
    epee::byte_slice bytes;
    {
        epee::levin::message_writer dest{};
        dest.buffer.write(epee::to_span(payload));
        bytes = dest.finalize_notify(ping_command);
    }

    EXPECT_EQ(1, get_connections().send(bytes.clone(), contexts_.front().get_id()));
    EXPECT_EQ(1u, contexts_.front().process_send_queue(true));
    EXPECT_EQ(1u, receiver_.notified_size());

    const received_message msg = receiver_.get_raw_notification();
    EXPECT_EQ(ping_command, msg.command);
    EXPECT_EQ(contexts_.front().get_id(), msg.connection);
    EXPECT_EQ(payload, msg.payload);

    {
        payload.push_back('h');
        epee::levin::message_writer dest{};
        dest.buffer.write(epee::to_span(payload));
        bytes = dest.finalize_notify(ping_command);
    }

    EXPECT_EQ(1, get_connections().send(std::move(bytes), contexts_.front().get_id()));
    EXPECT_EQ(1u, contexts_.front().process_send_queue(false));
    EXPECT_EQ(0u, receiver_.notified_size());
}
