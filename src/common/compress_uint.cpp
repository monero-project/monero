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

#include <algorithm>
#include <stdexcept>
#include <unordered_map>

#include "misc_log_ex.h"
#include "compress_uint.h"
#include "varint.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "util.compress_uint"

namespace
{
std::unordered_map<uint64_t, size_t> create_histogram(const std::vector<uint64_t>& values)
{
    std::unordered_map<uint64_t, size_t> hist;

    for (uint64_t value : values)
    {
        const auto hit = hist.find(value);
        if (hit != hist.end())
            hit->second++;
        else
            hist.insert({value, 1});
    }

    return hist;
}
} // anonymous namespace

namespace tools
{
std::string varint_pack(const std::vector<uint64_t> &v)
{
    std::string s;
    s.resize(v.size() * (sizeof(uint64_t) * 8 / 7 + 1));
    char *ptr = (char*)s.data();
    for (const uint64_t &t: v)
        tools::write_varint(ptr, t);
    s.resize(ptr - s.data());
    return s;
}

std::vector<uint64_t> varint_unpack(const std::string &s)
{
    std::vector<uint64_t> v;
    v.reserve(s.size());
    int read = 0;
    const std::string::const_iterator end = s.end();
    for (std::string::const_iterator i = s.begin(); i != end; std::advance(i, read))
    {
        uint64_t t;
        read = tools::read_varint(std::string::const_iterator(i), s.end(), t);
        CHECK_AND_ASSERT_THROW_MES(read > 0 && read <= 256, "Error decompressing data");
        v.push_back(t);
    }
    return v;
}

std::string one_span_compress(const std::vector<uint64_t>& data)
{
    CHECK_AND_ASSERT_THROW_MES(data.size() <= MAX_NUM_ONE_SPAN_ELEMENTS, "Too many elements");

    std::unordered_map<uint64_t, size_t> histogram = create_histogram(data);

    std::vector<uint64_t> unpacked_output;
    unpacked_output.reserve(2 + histogram.size() + data.size() * 2);

    // Sort the histogram values by values most commonly occurring
    std::vector<std::pair<uint64_t, size_t>> sorted_histogram(histogram.cbegin(), histogram.cend());
    std::sort(sorted_histogram.begin(), sorted_histogram.end(),
        [](const std::pair<uint64_t, size_t>& l, const std::pair<uint64_t, size_t>& r) -> bool
        { return l.second > r.second; }
    );

    // Write the table size and data size
    unpacked_output.push_back(histogram.size());
    unpacked_output.push_back(data.size());

    // Construct value-to-index lookup table in memory while writing index-to-value table to output
    std::unordered_map<uint64_t, size_t> indices_by_value;
    for (size_t i = 0; i < sorted_histogram.size(); ++i)
    {
        const uint64_t hist_val = sorted_histogram[i].first; 
        indices_by_value.insert({hist_val, i});
        unpacked_output.push_back(hist_val);
    }

    uint64_t current_one_span_length = 0;
    for (const uint64_t& value : data)
    {
        if (value == 1)
        {
            if (!current_one_span_length)
            {
                // first one in contiguous sequence
                unpacked_output.push_back(indices_by_value[1]);
            }
            current_one_span_length++;
        }
        else // value != 1
        {
            if (current_one_span_length)
            {
                // contiguous one sequence just ended
                unpacked_output.push_back(current_one_span_length);
                current_one_span_length = 0;
            }
            unpacked_output.push_back(indices_by_value[value]);
        }
    }

    if (current_one_span_length)
    {
        // contiguous one sequence just ended
        unpacked_output.push_back(current_one_span_length);
    }

    return varint_pack(unpacked_output);
}

std::vector<uint64_t> one_span_decompress(const std::string& compressed)
{
    CHECK_AND_ASSERT_THROW_MES(compressed.size(), "onespan format: cannot decompress empty string");

    std::vector<uint64_t> unpacked_input = varint_unpack(compressed);

    CHECK_AND_ASSERT_THROW_MES(unpacked_input.size() >= 2, "onespan format: no header data");

    const uint64_t table_size = unpacked_input[0];
    const uint64_t data_size = unpacked_input[1];

    CHECK_AND_ASSERT_THROW_MES(table_size <= MAX_NUM_ONE_SPAN_ELEMENTS, "onespan format: table too large");
    CHECK_AND_ASSERT_THROW_MES(data_size <= MAX_NUM_ONE_SPAN_ELEMENTS, "onespan format: data would decompress too large");
    CHECK_AND_ASSERT_THROW_MES(unpacked_input.size() >= (2 + table_size), "onespan format: not enough table data");

    std::vector<uint64_t> data;
    data.reserve(data_size);

    for (size_t i = 2 + table_size; i < unpacked_input.size() && data.size() < data_size; ++i)
    {
        const uint64_t table_index = unpacked_input[i];
        CHECK_AND_ASSERT_THROW_MES(table_index < table_size, "onespan format: data index out of range");
        const uint64_t table_value = unpacked_input[2 + table_index];
        if (table_value == 1)
        {
            CHECK_AND_ASSERT_THROW_MES(i < unpacked_input.size() - 1, "onespan format: one span out of data");
            const uint64_t one_span_len = unpacked_input[i + 1];
            CHECK_AND_ASSERT_THROW_MES(data.size() + one_span_len <= data_size, "onespan format: one span too long");
            for (size_t j = 0; j < one_span_len; ++j) data.push_back(1);
            ++i; // step over next unpacket input element
        }
        else
        {
            data.push_back(table_value);
        }
    }

    CHECK_AND_ASSERT_THROW_MES(data.size() == data_size, "onespan format: data decompressed to wrong size");

    return data;
}
} // namespace tools
