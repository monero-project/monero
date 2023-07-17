// Copyright (c) 2023, The Monero Project
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

#include <iostream>
#include <fstream>

#include "gtest/gtest.h"

#include "common/compress_uint.h"
#include "common/varint.h"
#include "crypto/crypto.h"
#include "string_tools.h"

static std::string vec_to_string(const std::vector<uint64_t>& data)
{
    std::string res = "{";
    for (const auto& v : data) { res.append(std::to_string(v)); res.append(","); }
    return res + "}";
}

static std::string compressed_to_string(const std::string& compressed)
{
    return epee::string_tools::buff_to_hex_nodelimer(compressed);
}

static std::vector<uint64_t> ones_interspersed_with_noise
(
    size_t num_elems,
    double percent_rand,
    uint64_t rand_val_cap = std::numeric_limits<uint64_t>::max()
)
{
    std::vector<uint64_t> data(num_elems, 1);

    const size_t num_random = static_cast<size_t>(percent_rand * data.size());
    for (size_t i = 0; i < num_random; ++i)
    {
        const size_t rand_i = crypto::rand<size_t>() % data.size();
        const uint64_t rand_val = crypto::rand<uint64_t>() % rand_val_cap;
        data[rand_i] = rand_val;
    }

    return data;
}

static bool test_start_finish(const std::vector<uint64_t>& data, bool show_output = true)
{
    std::string compressed;
    std::vector<uint64_t> decompressed;

    try
    {
        compressed = tools::one_span_compress(data);
        decompressed = tools::one_span_decompress(compressed);

        if (decompressed == data)
        {
            return true;
        }
        else
        {
            std::cerr << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$" << std::endl;
            std::cerr << "COMPRESSION CORRECTNESS FAILURE: NOT EQUAL" << std::endl;
            if (show_output)
            {
                std::cerr << "compressed result: " << compressed_to_string(compressed) << std::endl;
                std::cerr << "output data: " << vec_to_string(decompressed) << std::endl;
            }
            std::cerr << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$" << std::endl;
            return false;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$" << std::endl;
        std::cerr << "COMPRESSION CORRECTNESS FAILURE: ERROR OCCURRED" << std::endl;
        std::cerr << "error message: " << e.what() << std::endl;
        if (show_output)
        {
            std::cerr << "compressed result: " << compressed_to_string(compressed) << std::endl;
            std::cerr << "output data: " << vec_to_string(decompressed) << std::endl;
        }
        std::cerr << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$" << std::endl;
        return false;
    }
}

static bool test_compressed_size(const std::vector<uint64_t>& data)
{
    const double target_ratio = 50.0f / data.size() + 0.95; // asymtotically 0.95 bytes / element
    const size_t expected_size = static_cast<size_t>(target_ratio * data.size());
    const std::string compressed = tools::one_span_compress(data);
    return compressed.size() <= expected_size;
}

static size_t size_of_varint(const uint64_t v)
{
    std::string s(100, '\0');
    unsigned char* p = reinterpret_cast<unsigned char*>(&s[0]);
    tools::write_varint(p, v);
    return p - reinterpret_cast<unsigned char*>(&s[0]);
}

TEST(one_span_compression, start_finish_correctness)
{
    EXPECT_TRUE(test_start_finish({1, 1, 1, 1, 1, 4, 1, 8, 1, 1, 2, 3}));
    EXPECT_TRUE(test_start_finish({}));
    EXPECT_TRUE(test_start_finish({7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7}));
    EXPECT_TRUE(test_start_finish({1, 1, 1, 1, 1}));
    EXPECT_TRUE(test_start_finish({0}));
    EXPECT_TRUE(test_start_finish({1, 5, 7, 3, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 6, 3, 7, 3, 2}));
    EXPECT_TRUE(test_start_finish({1}));
    EXPECT_TRUE(test_start_finish({std::numeric_limits<uint64_t>::max()}));
    EXPECT_TRUE(test_start_finish({std::vector<uint64_t>(127, 1)}));
    EXPECT_TRUE(test_start_finish({std::vector<uint64_t>(128, 1)}));

    std::vector<uint64_t> ascending(1000, 0);
    for (size_t i = 1; i < ascending.size(); ++i)
    {
        ascending[i] = ascending[i - 1] + 1;
    }
    EXPECT_TRUE(test_start_finish(ascending));

    std::vector<uint64_t> utterly_random;
    utterly_random.reserve(1000000);
    for (size_t i = 0; i < 1000000; ++i)
    {
        utterly_random.push_back(crypto::rand<uint64_t>());
    }
    EXPECT_TRUE(test_start_finish(utterly_random, false));

    std::vector<uint64_t> max_vals(10000, std::numeric_limits<uint64_t>::max());
    EXPECT_TRUE(test_start_finish(max_vals, false));
}

TEST(one_span_compression, size)
{
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(100, 0.1, 10)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(100, 0.2, 10)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(100, 0.3, 10)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(100, 0.4, 10)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(100, 0.5, 10)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(1000, 0.1, 10)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(1000, 0.2, 10)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(1000, 0.3, 10)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(1000, 0.4, 10)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(1000, 0.5, 10)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(10000, 0.1, 10)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(10000, 0.2, 10)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(10000, 0.3, 10)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(10000, 0.4, 10)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(10000, 0.5, 10)));

    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(100, 0.1, 25)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(100, 0.2, 25)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(100, 0.3, 25)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(100, 0.4, 25)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(100, 0.5, 25)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(1000, 0.1, 100)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(1000, 0.2, 100)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(1000, 0.3, 100)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(1000, 0.4, 100)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(1000, 0.5, 100)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(10000, 0.1, 100)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(10000, 0.2, 100)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(10000, 0.3, 100)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(10000, 0.4, 100)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(10000, 0.5, 100)));

    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(100000, 0.1, 2000)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(100000, 0.2, 2000)));
    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(100000, 0.3, 2000)));

    EXPECT_TRUE(test_compressed_size(ones_interspersed_with_noise(1000000, 0.1, 2000)));
}

TEST(one_span_compression, varint_size)
{
    EXPECT_EQ(1, size_of_varint(0));

    EXPECT_EQ(1, size_of_varint(127));
    EXPECT_EQ(2, size_of_varint(128));

    EXPECT_EQ(2, size_of_varint(16383));
    EXPECT_EQ(3, size_of_varint(16384));

    EXPECT_EQ(3, size_of_varint(2097151));
    EXPECT_EQ(4, size_of_varint(2097152));

    EXPECT_EQ(4, size_of_varint(268435455));
    EXPECT_EQ(5, size_of_varint(268435456));

    EXPECT_EQ(5, size_of_varint(34359738367));
    EXPECT_EQ(6, size_of_varint(34359738368));

    EXPECT_EQ(6, size_of_varint(4398046511103));
    EXPECT_EQ(7, size_of_varint(4398046511104));

    EXPECT_EQ(7, size_of_varint(562949953421311));
    EXPECT_EQ(8, size_of_varint(562949953421312));

    EXPECT_EQ(8, size_of_varint(72057594037927935));
    EXPECT_EQ(9, size_of_varint(72057594037927936));

    EXPECT_EQ(8, size_of_varint(72057594037927935));
    EXPECT_EQ(9, size_of_varint(72057594037927936));

    EXPECT_EQ(9, size_of_varint(9223372036854775807UL));
    EXPECT_EQ(10, size_of_varint(9223372036854775808UL));

    EXPECT_EQ(10, size_of_varint(std::numeric_limits<uint64_t>::max()));
}
