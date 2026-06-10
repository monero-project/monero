// Copyright (c) 2026, The Monero Project
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

#include <cstddef>
#include <random>
#include <vector>

#include "cryptonote_basic/cryptonote_basic_impl.h"

using namespace cryptonote;

namespace
{
std::vector<hashable_block_header_info> gen_header_chain(
    const std::size_t n_blocks,
    const std::uint8_t major_version = MAX_HF_VERSION)
{
    static constexpr unsigned block_time = DIFFICULTY_TARGET_V2;
    static constexpr unsigned mean_n_txs_per_block = 100;

    std::vector<hashable_block_header_info> headers;
    headers.reserve(n_blocks);

    std::mt19937 gen{static_cast<std::mt19937::result_type>(crypto::random_device{}())};
    std::normal_distribution ts_dist{(double)block_time, /*stddev=*/1.0};
    std::normal_distribution txs_dist{(double)mean_n_txs_per_block, /*stddev=*/1.0};

    crypto::hash prev_id = crypto::rand<crypto::hash>();
    std::uint64_t last_timestamp = time(nullptr) - block_time * n_blocks;
    for (std::size_t i = 0; i < n_blocks; ++i)
    {
        const std::int64_t ts_diff = std::min<std::int64_t>(ts_dist(gen), last_timestamp);
        const std::uint64_t timestamp = last_timestamp + ts_diff;
        last_timestamp = timestamp;

        auto &header_info = headers.emplace_back(hashable_block_header_info{
            .header = block_header{
                .major_version = major_version,
                .minor_version = major_version,
                .timestamp = timestamp,
                .prev_id = prev_id,
                .nonce = crypto::rand<std::uint32_t>()
            },
            .block_content_hash = crypto::rand<crypto::hash>(),
            .n_txs_in_block = std::max<std::size_t>(txs_dist(gen), 1)
        });

        [[maybe_unused]] const bool r = calculate_block_hash_from_header_info(header_info, prev_id);
        assert(r);
    }

    return headers;
}
} //anonymous namespace

namespace cryptonote
{
bool operator==(const block_header &a, const block_header &b)
{
    assert(a.major_version <= MAX_HF_VERSION);
    assert(b.major_version <= MAX_HF_VERSION);
    return a.major_version == b.major_version
        && a.minor_version == b.minor_version
        && a.timestamp     == b.timestamp
        && a.prev_id       == b.prev_id
        && a.nonce         == b.nonce;
}

bool operator==(const hashable_block_header_info &a, const hashable_block_header_info &b)
{
    return a.header             == b.header
        && a.block_content_hash == b.block_content_hash
        && a.n_txs_in_block     == b.n_txs_in_block;
}
} //namespace cryptonote

TEST(cn_impl, compress_decompress_header_chain_single)
{
    static constexpr std::size_t max_normal_block_hashing_blob_size = 2 + 2 + 10 + 32 + 4 + 32 + 10;
    static constexpr std::size_t compression_overhead = 1 + 1 + 1;

    // make header chain
    const hashable_block_header_info header = gen_header_chain(/*n_blocks=*/1).at(0);

    // compress and test size
    std::string header_blob;
    ASSERT_TRUE(compress_block_header_chain({&header, 1}, header_blob));
    ASSERT_LE(header_blob.size(), max_normal_block_hashing_blob_size + compression_overhead);
    MDEBUG("Compressed header blob for 1 header is " << header_blob.size() << " bytes");

    // decompress
    std::vector<hashable_block_header_info> decompressed_header;
    ASSERT_TRUE(decompress_block_header_chain(header_blob, decompressed_header));

    // check equality
    ASSERT_EQ(1, decompressed_header.size());
    ASSERT_EQ(header, decompressed_header.front());
}

namespace
{
class CompressDecompressHeaderChainMultiple :
    public testing::TestWithParam<std::size_t>
{};
} //anonymous namespace

TEST_P(CompressDecompressHeaderChainMultiple, compress_decompress_header_chain_multiple)
{
    static constexpr std::size_t expected_average_block_hashing_blob_size = 1 + 1 + 4 + 4 + 32 + 2;
    static constexpr std::size_t compression_overhead = 1 + 1 + 1;
    static constexpr std::size_t compression_onetime_cost = 1 + 32 + 1 + 4 + 1;

    // make header chain
    const std::size_t n_blocks = GetParam();
    const std::vector<hashable_block_header_info> headers = gen_header_chain(n_blocks);
    ASSERT_EQ(n_blocks, headers.size());

    // compress and test size
    std::string headers_blob;
    ASSERT_TRUE(compress_block_header_chain(epee::to_span(headers), headers_blob));
    const std::size_t max_expected_size = compression_overhead + compression_onetime_cost
        + expected_average_block_hashing_blob_size * n_blocks;
    ASSERT_LE(headers_blob.size(), max_expected_size);
    const double avg_header_size = static_cast<double>(headers_blob.size()) / n_blocks;
    MDEBUG("Compressed header blob for " << n_blocks << " headers is " << headers_blob.size()
        << " bytes: average " << avg_header_size << " bytes/block");

    // decompress
    std::vector<hashable_block_header_info> decompressed_headers;
    ASSERT_TRUE(decompress_block_header_chain(headers_blob, decompressed_headers));

    // check equality
    ASSERT_EQ(n_blocks, decompressed_headers.size());
    ASSERT_EQ(headers, decompressed_headers);
}

INSTANTIATE_TEST_CASE_P(cn_impl, CompressDecompressHeaderChainMultiple,
    testing::Values(1, 2, 4, 8, 10, 100, 1000, 10000, 100000, 1000000));
