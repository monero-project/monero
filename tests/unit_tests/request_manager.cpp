// Copyright (c) 2014, The Monero Project
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

#include "gtest/gtest.h"

#include "cryptonote_protocol/request_manager.h"

#include <boost/uuid/uuid.hpp>
#include <chrono>
#include <thread>

static boost::uuids::uuid uuid_from_char(char a)
{
    boost::uuids::uuid u;
    memset(&u, a, 16);
    return u;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
TEST(request_manager, basic_usage)
{
    const uint8_t MAX_IN_FLIGHT = 3;
    request_manager req_manager(MAX_IN_FLIGHT);

    const boost::uuids::uuid peer{};

    // 1. Fill up the request manager
    for (char i = 0; i < MAX_IN_FLIGHT; ++i)
    {
        // Success case
        ASSERT_EQ(req_manager.enqueue_requests({crypto::hash{i}}, peer).tx_hashes.size(), 1);

        // Can't re-add
        ASSERT_EQ(req_manager.enqueue_requests({crypto::hash{i}}, peer).tx_hashes.size(), 0);
    }

    // 2. Queue is full and can't let any others fly. Queue this one
    ASSERT_EQ(req_manager.enqueue_requests({crypto::hash{MAX_IN_FLIGHT}}, peer).tx_hashes.size(), 0);

    // 3. Remove 1 from the queue to make room for one queued above
    ASSERT_TRUE(req_manager.remove_request(crypto::hash{0}));
    ASSERT_FALSE(req_manager.remove_request(crypto::hash{0}));

    // 4. Make sure the one queued above enters flight
    const auto tx_reqs = req_manager.fly_available_requests(peer).tx_hashes;
    ASSERT_EQ(tx_reqs.size(), 1);
    ASSERT_EQ(tx_reqs.front(), crypto::hash{MAX_IN_FLIGHT});

    // 5. Remove
    ASSERT_TRUE(req_manager.remove_request(crypto::hash{1}));
    ASSERT_TRUE(req_manager.remove_request(crypto::hash{2}));
    ASSERT_TRUE(req_manager.remove_request(crypto::hash{MAX_IN_FLIGHT}));
    req_manager.remove_peer(peer);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(request_manager, cancel_syncing_peer_requests)
{
    request_manager req_manager(2, 0);
    const auto peer = uuid_from_char(1);
    const auto other_peer = uuid_from_char(2);
    const std::vector<crypto::hash> hashes{crypto::hash{1}, crypto::hash{2}, crypto::hash{3}};
    ASSERT_EQ(req_manager.enqueue_requests(hashes, peer).tx_hashes.size(), 2);
    ASSERT_TRUE(req_manager.enqueue_requests({hashes.begin(), hashes.begin() + 2}, other_peer).tx_hashes.empty());

    req_manager.cancel_peer_requests(peer);
    EXPECT_TRUE(req_manager.remove_stale_requests().empty());
    EXPECT_EQ(req_manager.fly_available_requests(other_peer).tx_hashes.size(), 2);
    EXPECT_EQ(req_manager.fly_available_requests(peer).tx_hashes, std::vector<crypto::hash>{hashes.back()});
    EXPECT_EQ(req_manager.enqueue_requests({crypto::hash{4}}, peer).tx_hashes.size(), 1);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(request_manager, canceled_requests_preserve_completed_history)
{
    const size_t sample_size = P2P_MIN_SAMPLE_SIZE_FOR_DROPPING;
    request_manager req_manager(sample_size, 0);
    const auto peer = uuid_from_char(1);
    std::vector<crypto::hash> hashes;
    for (size_t i = 0; i < sample_size; ++i)
      hashes.push_back(crypto::hash{static_cast<char>(i)});

    // Record misses just below the sample threshold.
    ASSERT_EQ(req_manager.enqueue_requests({hashes.begin(), hashes.end() - 1}, peer).tx_hashes.size(), sample_size - 1);
    EXPECT_TRUE(req_manager.remove_stale_requests().empty());

    ASSERT_EQ(req_manager.enqueue_requests(hashes, peer).tx_hashes.size(), sample_size);
    req_manager.cancel_peer_requests(peer);
    EXPECT_TRUE(req_manager.remove_stale_requests().empty());
    ASSERT_EQ(req_manager.enqueue_requests({hashes.back()}, peer).tx_hashes.size(), 1);
    EXPECT_EQ(req_manager.remove_stale_requests().count(peer), 1);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(request_manager, deferred_requests_wait_for_resume)
{
    const size_t sample_size = P2P_MIN_SAMPLE_SIZE_FOR_DROPPING;
    request_manager req_manager(sample_size, 0);
    const auto peer = uuid_from_char(1);
    std::vector<crypto::hash> hashes;
    for (size_t i = 0; i < sample_size; ++i)
        hashes.push_back(crypto::hash{static_cast<char>(i)});

    const auto deferred = req_manager.enqueue_requests(hashes, peer, false);
    EXPECT_TRUE(deferred.tx_hashes.empty());
    EXPECT_TRUE(req_manager.remove_stale_requests().empty());

    const auto resumed = req_manager.fly_available_requests(peer);
    EXPECT_GT(resumed.nonce, deferred.nonce);
    EXPECT_EQ(std::unordered_set<crypto::hash>(resumed.tx_hashes.begin(), resumed.tx_hashes.end()),
        std::unordered_set<crypto::hash>(hashes.begin(), hashes.end()));
    // Only the resumed requests count toward the failure rate.
    EXPECT_EQ(req_manager.remove_stale_requests().count(peer), 1);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(request_manager, deferred_requests_do_not_block_other_peers)
{
    request_manager req_manager(2, 0);
    const auto peer = uuid_from_char(1);
    const auto other_peer = uuid_from_char(2);
    const crypto::hash shared{1}, deferred{2};
    EXPECT_TRUE(req_manager.enqueue_requests({shared, deferred}, peer, false).tx_hashes.empty());
    EXPECT_EQ(req_manager.enqueue_requests({shared}, other_peer).tx_hashes, std::vector<crypto::hash>{shared});
    EXPECT_TRUE(req_manager.remove_request(shared));
    EXPECT_EQ(req_manager.fly_available_requests(peer).tx_hashes, std::vector<crypto::hash>{deferred});
}
//----------------------------------------------------------------------------------------------------------------------
TEST(request_manager, deferred_requests_respect_queue_limit)
{
    request_manager req_manager(2, 0, 2);
    const auto peer = uuid_from_char(1);
    const crypto::hash first{1}, second{2}, overflow{3};
    EXPECT_TRUE(req_manager.enqueue_requests({first, first, second, overflow}, peer, false).tx_hashes.empty());
    EXPECT_FALSE(req_manager.remove_request(overflow));
    EXPECT_TRUE(req_manager.remove_request(first));
    EXPECT_FALSE(req_manager.remove_request(first));
    EXPECT_TRUE(req_manager.enqueue_requests({second, overflow}, peer, false).tx_hashes.empty());
    EXPECT_TRUE(req_manager.remove_request(second));
    EXPECT_EQ(req_manager.fly_available_requests(peer).tx_hashes, std::vector<crypto::hash>{overflow});
}
//----------------------------------------------------------------------------------------------------------------------
TEST(request_manager, multiple_peers)
{
    const uint8_t MAX_IN_FLIGHT = 4;
    const int64_t TIMEOUT_MS = 10;
    request_manager req_manager(MAX_IN_FLIGHT, TIMEOUT_MS);

    // 1. Add 1 for each peer
    for (char i = 0; i < MAX_IN_FLIGHT; ++i)
    {
        const crypto::hash hash{i};
        const boost::uuids::uuid peer = uuid_from_char(i);

        // Success case
        ASSERT_EQ(req_manager.enqueue_requests({hash}, peer).tx_hashes.size(), 1);

        // Can't re-add
        ASSERT_EQ(req_manager.enqueue_requests({hash}, peer).tx_hashes.size(), 0);
    }

    // 2. No stale requests and none available
    {
        const auto drop_peers = req_manager.remove_stale_requests();
        ASSERT_TRUE(drop_peers.empty());
        for (uint8_t i = 0; i < MAX_IN_FLIGHT; ++i)
        {
            const boost::uuids::uuid peer = uuid_from_char(i);
            const auto tx_hashes = req_manager.fly_available_requests(peer).tx_hashes;
            ASSERT_TRUE(tx_hashes.empty());
        }
    }

    // 3. Sleep to let requests timeout
    MINFO("Sleeping for 20ms to let requests timeout");
    std::this_thread::sleep_for(std::chrono::milliseconds{TIMEOUT_MS * 2});

    // 4. Re-add the same ones for each peer, but in reverse. The hashes
    //    should get queued for each peer
    for (char i = 0; i < MAX_IN_FLIGHT; ++i)
    {
        const crypto::hash hash{i};
        const boost::uuids::uuid peer = uuid_from_char(MAX_IN_FLIGHT - i - 1);
        ASSERT_EQ(req_manager.enqueue_requests({hash}, peer).tx_hashes.size(), 0);
    }

    // 5. Remove stale requests
    const auto drop_peers = req_manager.remove_stale_requests();
    ASSERT_TRUE(drop_peers.empty());

    // 6. Available reqs should be all the tx hashes from step 4
    for (char i = 0; i < MAX_IN_FLIGHT; ++i)
    {
        const crypto::hash hash{i};
        const boost::uuids::uuid peer = uuid_from_char(MAX_IN_FLIGHT - i - 1);
        const auto tx_hashes = req_manager.fly_available_requests(peer).tx_hashes;

        ASSERT_EQ(tx_hashes.size(), 1);
        ASSERT_EQ(tx_hashes.front(), hash);
    }

    // 7. Remove peers
    for (uint8_t i = 0; i < MAX_IN_FLIGHT; ++i)
    {
        const boost::uuids::uuid peer = uuid_from_char(i);
        req_manager.remove_peer(peer);
    }
}
//----------------------------------------------------------------------------------------------------------------------
TEST(request_manager, drop_peers)
{
    const uint8_t MAX_IN_FLIGHT = P2P_MIN_SAMPLE_SIZE_FOR_DROPPING;
    const int64_t TIMEOUT_MS = 10;
    request_manager req_manager(MAX_IN_FLIGHT, TIMEOUT_MS);

    const boost::uuids::uuid peer{};

    // 1. Add many requests
    for (char i = 0; i < MAX_IN_FLIGHT; ++i)
        ASSERT_EQ(req_manager.enqueue_requests({crypto::hash{i}}, peer).tx_hashes.size(), 1);

    // 2. Sleep to let requests timeout
    MINFO("Sleeping for 20ms to let requests timeout");
    std::this_thread::sleep_for(std::chrono::milliseconds{TIMEOUT_MS * 2});

    // 3. Remove stale requests
    const auto drop_peers = req_manager.remove_stale_requests();
    ASSERT_EQ(drop_peers.size(), 1);
    ASSERT_EQ(drop_peers.count(peer), 1);

    // 4. Remove peers
    req_manager.remove_peer(peer);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(request_manager, dont_rm_processing_txs)
{
    const uint8_t MAX_IN_FLIGHT = P2P_MIN_SAMPLE_SIZE_FOR_DROPPING;
    const int64_t TIMEOUT_MS = 10;
    request_manager req_manager(MAX_IN_FLIGHT, TIMEOUT_MS);

    const boost::uuids::uuid peer{};

    // 1. Add many requests
    std::vector<crypto::hash> init_hashes;
    init_hashes.reserve(MAX_IN_FLIGHT);
    for (char i = 0; i < MAX_IN_FLIGHT; ++i)
        init_hashes.emplace_back(crypto::hash{i});
    const auto tx_req = req_manager.enqueue_requests(init_hashes, peer);
    ASSERT_EQ(tx_req.tx_hashes, init_hashes);
    ASSERT_EQ(tx_req.nonce, 1);

    // 2. Protect requests until the response is processed.
    ASSERT_TRUE(req_manager.start_processing_txs(peer, tx_req.nonce, init_hashes.size() - 1));

    // 3. Sleep to let requests timeout
    MINFO("Sleeping for 20ms to let requests timeout");
    std::this_thread::sleep_for(std::chrono::milliseconds{TIMEOUT_MS * 2});

    // 4. There should be no stale requests.
    const auto drop_peers = req_manager.remove_stale_requests();
    ASSERT_EQ(drop_peers.size(), 0);

    // 5. Complete received transactions and release the omitted request.
    for (auto it = init_hashes.begin(); it != init_hashes.end() - 1; ++it)
        ASSERT_TRUE(req_manager.remove_request(*it));
    ASSERT_TRUE(req_manager.finish_processing_txs(peer, tx_req.nonce));
    ASSERT_FALSE(req_manager.remove_request(init_hashes.back()));
    ASSERT_FALSE(req_manager.finish_processing_txs(peer, tx_req.nonce));
    req_manager.remove_peer(peer);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(request_manager, dont_rm_processing_txs_enqueue_overage)
{
    const uint8_t MAX_IN_FLIGHT = 2;
    const int64_t TIMEOUT_MS = 10;
    request_manager req_manager(MAX_IN_FLIGHT, TIMEOUT_MS);

    const boost::uuids::uuid peer{};

    // 1. Enqueue 1 more request than max allowed in flight
    const uint8_t TOTAL_TXS = MAX_IN_FLIGHT + 1;
    std::vector<crypto::hash> init_hashes;
    init_hashes.reserve(TOTAL_TXS);
    for (char i = 0; i < TOTAL_TXS; ++i)
        init_hashes.emplace_back(crypto::hash{i});
    const auto tx_req = req_manager.enqueue_requests(init_hashes, peer);
    ASSERT_EQ(tx_req.tx_hashes, std::vector<crypto::hash>(init_hashes.begin(), init_hashes.begin() + MAX_IN_FLIGHT));
    ASSERT_EQ(tx_req.nonce, 1);

    // 2. Remove 1 req to make room for the last one
    ASSERT_TRUE(req_manager.remove_request(init_hashes.front()));

    // 3. Now fly the last one
    const auto tx_req2 = req_manager.fly_available_requests(peer);
    ASSERT_EQ(tx_req2.tx_hashes, std::vector<crypto::hash>{init_hashes.back()});
    ASSERT_EQ(tx_req2.nonce, 2);

    // 4. Indicate that we're processing Step 3's tx
    ASSERT_TRUE(req_manager.start_processing_txs(peer, tx_req2.nonce, tx_req2.tx_hashes.size()));

    // 5. Sleep to let requests timeout
    MINFO("Sleeping for 20ms to let requests timeout");
    std::this_thread::sleep_for(std::chrono::milliseconds{TIMEOUT_MS * 2});

    // 6. The processing tx from step 4 should still be present, since it's processing and shouldn't have been rm'd.
    //    init_hashes.at(1) should have timed out and rm'd. Since this is only the first stale tx req rm'd, the peer
    //    is not expected to drop.
    static_assert(P2P_MIN_SAMPLE_SIZE_FOR_DROPPING > 1);
    const auto drop_peers = req_manager.remove_stale_requests();
    ASSERT_EQ(drop_peers.size(), 0);
    ASSERT_FALSE(req_manager.remove_request(init_hashes.at(1)));
    ASSERT_TRUE(req_manager.remove_request(init_hashes.back()));

    // 7. Remove peer
    req_manager.remove_peer(peer);
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
TEST(request_manager, cap_queued_requests)
{
    const boost::uuids::uuid peer = uuid_from_char(1);

    // 1 request in flight, at most 2 requests per peer. Removing a request frees a slot.
    {
        request_manager req_manager(1, 30000, 2);
        const auto req = req_manager.enqueue_requests({crypto::hash{1}}, peer);
        ASSERT_EQ(req.tx_hashes.size(), 1); // flies
        ASSERT_EQ(req_manager.enqueue_requests({crypto::hash{2}}, peer).tx_hashes.size(), 0); // queued
        ASSERT_EQ(req_manager.enqueue_requests({crypto::hash{3}}, peer).tx_hashes.size(), 0); // dropped
        ASSERT_TRUE(req_manager.remove_request(crypto::hash{2}));
        ASSERT_EQ(req_manager.enqueue_requests({crypto::hash{3}}, peer).tx_hashes.size(), 0); // queued
        ASSERT_TRUE(req_manager.start_processing_txs(peer, req.nonce, 0));
        ASSERT_TRUE(req_manager.finish_processing_txs(peer, req.nonce)); // empty reply frees hash 1
        const auto tx_hashes = req_manager.fly_available_requests(peer).tx_hashes;
        ASSERT_EQ(tx_hashes.size(), 1);
        ASSERT_EQ(tx_hashes.front(), crypto::hash{3});
    }

    // Same with a 0 ms timeout: removing a stale request frees a slot.
    {
        request_manager req_manager(1, 0, 2);
        ASSERT_EQ(req_manager.enqueue_requests({crypto::hash{1}}, peer).tx_hashes.size(), 1); // flies
        ASSERT_EQ(req_manager.enqueue_requests({crypto::hash{2}}, peer).tx_hashes.size(), 0); // queued
        req_manager.remove_stale_requests(); // removes hash 1
        ASSERT_EQ(req_manager.fly_available_requests(peer).tx_hashes.size(), 1); // hash 2 flies
        ASSERT_EQ(req_manager.enqueue_requests({crypto::hash{3}}, peer).tx_hashes.size(), 0); // queued
        req_manager.remove_stale_requests(); // removes hash 2
        const auto tx_hashes = req_manager.fly_available_requests(peer).tx_hashes;
        ASSERT_EQ(tx_hashes.size(), 1);
        ASSERT_EQ(tx_hashes.front(), crypto::hash{3});
    }
}

TEST(request_manager, finish_response_preserves_other_peers_and_nonces)
{
    request_manager req_manager(2);
    const auto peer = uuid_from_char(1);
    const auto other_peer = uuid_from_char(2);
    const crypto::hash received{1}, omitted{2}, later{3};
    const auto first = req_manager.enqueue_requests({received, omitted}, peer);
    ASSERT_EQ(first.tx_hashes.size(), 2);
    ASSERT_TRUE(req_manager.enqueue_requests({received, omitted}, other_peer).tx_hashes.empty());
    ASSERT_TRUE(req_manager.start_processing_txs(peer, first.nonce, 1));
    ASSERT_TRUE(req_manager.remove_request(received));

    const auto second = req_manager.enqueue_requests({later}, peer);
    ASSERT_EQ(second.tx_hashes, std::vector<crypto::hash>{later});
    ASSERT_NE(first.nonce, second.nonce);
    ASSERT_TRUE(req_manager.finish_processing_txs(peer, first.nonce));
    ASSERT_TRUE(req_manager.remove_request(later));

    ASSERT_EQ(req_manager.fly_available_requests(other_peer).tx_hashes, std::vector<crypto::hash>{omitted});
    ASSERT_TRUE(req_manager.remove_request(omitted));
    ASSERT_FALSE(req_manager.finish_processing_txs(peer, first.nonce));
    ASSERT_EQ(req_manager.enqueue_requests({received, omitted}, peer).tx_hashes.size(), 2);
}

TEST(request_manager, unsolicited_or_oversized_response_does_not_suspend_timeouts)
{
    request_manager req_manager(1, 0);
    const auto peer = uuid_from_char(1);
    const crypto::hash in_flight{1}, queued{2};
    const auto request = req_manager.enqueue_requests({in_flight, queued}, peer);
    ASSERT_EQ(request.tx_hashes, std::vector<crypto::hash>{in_flight});
    EXPECT_FALSE(req_manager.start_processing_txs(peer, 0, 0));
    EXPECT_FALSE(req_manager.start_processing_txs(peer, request.nonce + 1, 0));
    EXPECT_FALSE(req_manager.start_processing_txs(peer, request.nonce, 2));
    EXPECT_FALSE(req_manager.finish_processing_txs(peer, 0));
    EXPECT_FALSE(req_manager.finish_processing_txs(peer, request.nonce));
    EXPECT_TRUE(req_manager.remove_stale_requests().empty());
    EXPECT_FALSE(req_manager.remove_request(in_flight));
    EXPECT_EQ(req_manager.fly_available_requests(peer).tx_hashes, std::vector<crypto::hash>{queued});
}
