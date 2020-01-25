// Copyright (c) 2019, The Monero Project
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

#include "byte_slice.h"
#include "crypto/crypto.h"
#include "cryptonote_basic/connection_context.h"
#include "cryptonote_core/cryptonote_core.h"
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
            handler_.after_init_connection();
        }

        //\return Number of messages processed
        std::size_t process_send_queue()
        {
            std::size_t count = 0;
            for ( ; !endpoint_.send_queue_.empty(); ++count, endpoint_.send_queue_.pop_front())
            {
                // invalid messages shoudn't be possible in this test;
                EXPECT_TRUE(handler_.handle_recv(endpoint_.send_queue_.front().data(), endpoint_.send_queue_.front().size()));
            }
            return count;
        }

        const boost::uuids::uuid& get_id() const noexcept
        {
            return context_.m_connection_id;
        }
    };

    struct received_message
    {
        boost::uuids::uuid connection;
        int command;
        std::string payload;
    };

    class test_receiver : public epee::levin::levin_commands_handler<cryptonote::levin::detail::p2p_context>
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

        virtual int invoke(int command, const epee::span<const uint8_t> in_buff, std::string& buff_out, cryptonote::levin::detail::p2p_context& context) override final
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

        virtual void on_connection_new(cryptonote::levin::detail::p2p_context&) override final
        {}

        virtual void on_connection_close(cryptonote::levin::detail::p2p_context&) override final
        {}

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
            contexts_()
        {
            connections_->set_handler(std::addressof(receiver_), nullptr);
        }

        virtual void TearDown() override final
        {
            EXPECT_EQ(0u, receiver_.invoked_size());
            EXPECT_EQ(0u, receiver_.notified_size());
        }

        void add_connection(const bool is_incoming)
        {
            contexts_.emplace_back(io_service_, *connections_, random_generator_, is_incoming);
            EXPECT_TRUE(connection_ids_.emplace(contexts_.back().get_id()).second);
            EXPECT_EQ(connection_ids_.size(), connections_->get_connections_count());
        }

        cryptonote::levin::notify make_notifier(const std::size_t noise_size, bool is_public, bool pad_txs)
        {
            epee::byte_slice noise = nullptr;
            if (noise_size)
                noise = epee::levin::make_noise_notify(noise_size);
            return cryptonote::levin::notify{io_service_, connections_, std::move(noise), is_public, pad_txs};
        }

        boost::uuids::random_generator random_generator_;
        boost::asio::io_service io_service_;
        test_receiver receiver_;
        std::deque<test_connection> contexts_;
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

TEST(make_notify, empty_payload)
{
    const epee::byte_slice message = epee::levin::make_notify(443, nullptr);
    const epee::levin::bucket_head2 header =
        epee::levin::make_header(443, 0, LEVIN_PACKET_REQUEST, false);
    ASSERT_EQ(sizeof(header), message.size());
    EXPECT_TRUE(std::memcmp(std::addressof(header), message.data(), sizeof(header)) == 0);
}

TEST(make_notify, with_payload)
{
    std::string bytes(100, 'a');
    std::generate(bytes.begin(), bytes.end(), crypto::random_device{});

    const epee::byte_slice message = epee::levin::make_notify(443, epee::strspan<std::uint8_t>(bytes));
    const epee::levin::bucket_head2 header =
        epee::levin::make_header(443, bytes.size(), LEVIN_PACKET_REQUEST, false);

    ASSERT_EQ(sizeof(header) + bytes.size(), message.size());
    EXPECT_TRUE(std::memcmp(std::addressof(header), message.data(), sizeof(header)) == 0);
    EXPECT_TRUE(std::memcmp(bytes.data(), message.data() + sizeof(header), bytes.size()) == 0);
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
    EXPECT_TRUE(epee::levin::make_fragmented_notify(nullptr, 0, nullptr).empty());
}

TEST(make_fragment, single)
{
    const epee::byte_slice noise = epee::levin::make_noise_notify(1024);
    const epee::byte_slice fragment = epee::levin::make_fragmented_notify(noise, 11, nullptr);
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

    const epee::byte_slice noise = epee::levin::make_noise_notify(1024);
    epee::byte_slice fragment = epee::levin::make_fragmented_notify(noise, 114, epee::strspan<std::uint8_t>(bytes));

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
    EXPECT_TRUE(notifier.send_txs({}, random_generator_()));

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'e');
    EXPECT_FALSE(notifier.send_txs(std::move(txs), random_generator_()));
}

TEST_F(levin_notify, fluff_without_padding)
{
    cryptonote::levin::notify notifier = make_notifier(0, true, false);

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();
    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled); // not tracked
    }

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'e');
    txs[1].resize(200, 'f');

    ASSERT_EQ(10u, contexts_.size());
    {
        auto context = contexts_.begin();
        EXPECT_TRUE(notifier.send_txs(txs, context->get_id()));

        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
        notifier.run_fluff();
        ASSERT_LT(0u, io_service_.poll());

        EXPECT_EQ(0u, context->process_send_queue());
        for (++context; context != contexts_.end(); ++context)
            EXPECT_EQ(1u, context->process_send_queue());

        ASSERT_EQ(9u, receiver_.notified_size());
        for (unsigned count = 0; count < 9; ++count)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
            EXPECT_EQ(txs, notification.txs);
            EXPECT_TRUE(notification._.empty());
        }
    }
}

TEST_F(levin_notify, fluff_with_padding)
{
    cryptonote::levin::notify notifier = make_notifier(0, true, true);

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();
    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled); // not tracked
    }

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'e');
    txs[1].resize(200, 'f');

    ASSERT_EQ(10u, contexts_.size());
    {
        auto context = contexts_.begin();
        EXPECT_TRUE(notifier.send_txs(txs, context->get_id()));

        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
        notifier.run_fluff();
        ASSERT_LT(0u, io_service_.poll());

        EXPECT_EQ(0u, context->process_send_queue());
        for (++context; context != contexts_.end(); ++context)
            EXPECT_EQ(1u, context->process_send_queue());

        ASSERT_EQ(9u, receiver_.notified_size());
        for (unsigned count = 0; count < 9; ++count)
        {
            auto notification = receiver_.get_notification<cryptonote::NOTIFY_NEW_TRANSACTIONS>().second;
            EXPECT_EQ(txs, notification.txs);
            EXPECT_FALSE(notification._.empty());
        }
    }
}

TEST_F(levin_notify, private_fluff_without_padding)
{
    cryptonote::levin::notify notifier = make_notifier(0, false, false);

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();
    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled); // not tracked
    }

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'e');
    txs[1].resize(200, 'f');

    ASSERT_EQ(10u, contexts_.size());
    {
        auto context = contexts_.begin();
        EXPECT_TRUE(notifier.send_txs(txs, context->get_id()));

        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
        notifier.run_fluff();
        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());

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
        }
    }
}

TEST_F(levin_notify, private_fluff_with_padding)
{
    cryptonote::levin::notify notifier = make_notifier(0, false, true);

    for (unsigned count = 0; count < 10; ++count)
        add_connection(count % 2 == 0);

    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled);
    }
    notifier.new_out_connection();
    io_service_.poll();
    {
        const auto status = notifier.get_status();
        EXPECT_FALSE(status.has_noise);
        EXPECT_FALSE(status.connections_filled); // not tracked
    }

    std::vector<cryptonote::blobdata> txs(2);
    txs[0].resize(100, 'e');
    txs[1].resize(200, 'f');

    ASSERT_EQ(10u, contexts_.size());
    {
        auto context = contexts_.begin();
        EXPECT_TRUE(notifier.send_txs(txs, context->get_id()));

        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());
        notifier.run_fluff();
        io_service_.reset();
        ASSERT_LT(0u, io_service_.poll());

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
    cryptonote::levin::notify notifier = make_notifier(2048, false, true);

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

    EXPECT_TRUE(notifier.send_txs(txs, incoming_id));
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
        }
    }

    txs[0].resize(3000, 'r');
    EXPECT_TRUE(notifier.send_txs(txs, incoming_id));
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
        }
    }
}
