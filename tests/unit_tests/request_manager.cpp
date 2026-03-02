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
        ASSERT_TRUE(req_manager.add_request(crypto::hash{i}, peer));

        // Can't re-add
        ASSERT_FALSE(req_manager.add_request(crypto::hash{i}, peer));
    }

    // 2. Queue is full and can't let any others fly. Queue this one
    ASSERT_FALSE(req_manager.add_request(crypto::hash{MAX_IN_FLIGHT}, peer));

    // 3. Remove 1 from the queue to make room for one queued above
    ASSERT_TRUE(req_manager.remove_request(crypto::hash{0}));
    ASSERT_FALSE(req_manager.remove_request(crypto::hash{0}));

    // 4. Make sure the one queued above enters flight
    const auto tx_reqs = req_manager.fly_available_requests(peer);
    ASSERT_EQ(tx_reqs.size(), 1);
    ASSERT_EQ(tx_reqs.front(), crypto::hash{MAX_IN_FLIGHT});

    // 5. Remove
    ASSERT_TRUE(req_manager.remove_request(crypto::hash{1}));
    ASSERT_TRUE(req_manager.remove_request(crypto::hash{2}));
    ASSERT_TRUE(req_manager.remove_request(crypto::hash{MAX_IN_FLIGHT}));
    req_manager.remove_peer(peer);
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
        ASSERT_TRUE(req_manager.add_request(hash, peer));

        // Can't re-add
        ASSERT_FALSE(req_manager.add_request(hash, peer));
    }

    // 2. No stale requests and none available
    {
        const auto drop_peers = req_manager.remove_stale_requests();
        ASSERT_TRUE(drop_peers.empty());
        for (uint8_t i = 0; i < MAX_IN_FLIGHT; ++i)
        {
            const boost::uuids::uuid peer = uuid_from_char(i);
            const auto tx_hashes = req_manager.fly_available_requests(peer);
            ASSERT_TRUE(tx_hashes.empty());
        }
    }

    // 3. Sleep to let requests timeout
    MINFO("Sleeping for 20ms to let requests timeout");
    usleep(TIMEOUT_MS * 1000 * 2);

    // 4. Re-add the same ones for each peer, but in reverse. The hashes
    //    should get queued for each peer
    for (char i = 0; i < MAX_IN_FLIGHT; ++i)
    {
        const crypto::hash hash{i};
        const boost::uuids::uuid peer = uuid_from_char(MAX_IN_FLIGHT - i - 1);
        ASSERT_FALSE(req_manager.add_request(hash, peer));
    }

    // 5. Remove stale requests
    const auto drop_peers = req_manager.remove_stale_requests();
    ASSERT_TRUE(drop_peers.empty());

    // 6. Available reqs should be all the tx hashes from step 3
    for (char i = 0; i < MAX_IN_FLIGHT; ++i)
    {
        const crypto::hash hash{i};
        const boost::uuids::uuid peer = uuid_from_char(MAX_IN_FLIGHT - i - 1);
        const auto tx_hashes = req_manager.fly_available_requests(peer);

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
        ASSERT_TRUE(req_manager.add_request(crypto::hash{i}, peer));

    // 2. Sleep to let requests timeout
    MINFO("Sleeping for 20ms to let requests timeout");
    usleep(TIMEOUT_MS * 1000 * 2);

    // 3. Remove stale requests
    const auto drop_peers = req_manager.remove_stale_requests();
    ASSERT_EQ(drop_peers.size(), 1);
    ASSERT_EQ(drop_peers.count(peer), 1);

    // 4. Remove peers
    req_manager.remove_peer(peer);
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
