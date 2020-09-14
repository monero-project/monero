// Copyright (c) 2014-2019, AEON, The Monero Project
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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <random>

#include "cryptonote_config.h"
#include "cryptonote_basic/difficulty.h"
#include "common/command_line.h"
#include "misc_language.h"

namespace po = boost::program_options;
using namespace std;

#define DEFAULT_TEST_DIFFICULTY_TARGET        120

static int generate_variant(const char *filename, uint8_t variant_version)
{
    ofstream data(filename);
    vector<uint64_t> timestamps;
    vector<uint64_t> cumulative_difficulties;
    uint64_t honest_timestamp = 0;
    uint64_t total_deviation = 0;
    uint64_t height = 0;
    std::mt19937 gen(0);

    static const struct {
      size_t num_blocks;
      double real_hashrate;
    } periods[] = {
        {3000, 1000.},
        {3000, 10000.},
        {3000, 1000.},
        {500, 10000.},
        {1000, 1000.},
        {500, 100.},
        {3000, 1000.},
    };

    const std::map<uint64_t, bool> manipulations = {
        // (height, flag) pair where flag=true means to maximize timestamp, flag=false to minimize timestamp
        {13000, true},
        {13719, false},
    };

    for (auto& p : periods)
    {
        for (size_t i = 0; i < p.num_blocks; ++i, ++height)
        {
            const size_t n = timestamps.size();
            size_t begin, end;
            if (timestamps.size() < DIFFICULTY_BLOCKS_COUNT) {
                begin = 0;
                end = n;
            } else {
                end = n;
                begin = end - DIFFICULTY_BLOCKS_COUNT;
            }

            uint64_t difficulty = cryptonote::next_difficulty_64(
                vector<uint64_t>(timestamps.begin() + begin, timestamps.begin() + end),
                vector<uint64_t>(cumulative_difficulties.begin() + begin, cumulative_difficulties.begin() + end), DEFAULT_TEST_DIFFICULTY_TARGET, n + 1, 0, 0, variant_version);

            // expected block time based on difficulty and hashrate
            double block_time_avg = difficulty / p.real_hashrate;

            // generate negative block time on rare occasions -- 243 negatives / 100000 total = 0.24%, based on observing blocks in [1000k,1100k) range
            int64_t honest_block_time = std::uniform_real_distribution<>{0.0, 1.0}(gen) > 0.0024 ?
                (int64_t)std::ceil(std::exponential_distribution<>{1. / block_time_avg}(gen)) :
                -(int64_t)std::ceil(std::exponential_distribution<>{1. / 36.7}(gen));        // rough fit over the negative samples (after removing 3 outliers below -3000)

            auto manipulation = manipulations.find(height);
            uint64_t timestamp;
            if (manipulation != manipulations.end() && manipulation->second) {
                // assign maximum possible value
                timestamp = honest_timestamp + honest_block_time + CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT;
            } else if (manipulation != manipulations.end() && timestamps.size() >= BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW) {
                // assign minimum possible value
                std::vector<uint64_t> timestamps_temp(timestamps.end() - BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW, timestamps.end());
                timestamp = epee::misc_utils::median(timestamps_temp);
            } else {
                timestamp = honest_timestamp + honest_block_time;
            }
            int64_t block_time = (int64_t)timestamp - (int64_t)(timestamps.empty() ? 0 : timestamps.back());

            timestamps.push_back(timestamp);
            cumulative_difficulties.push_back(cumulative_difficulties.empty() ? difficulty : cumulative_difficulties.back() + difficulty);

            data << block_time << " " << difficulty << endl;
            total_deviation += block_time > DEFAULT_TEST_DIFFICULTY_TARGET ? block_time - DEFAULT_TEST_DIFFICULTY_TARGET : DEFAULT_TEST_DIFFICULTY_TARGET - block_time;
            honest_timestamp += honest_block_time;
        }
    }
    cout << "Average block time: " << (timestamps.back() / (double)timestamps.size()) << ", total deviation: " << total_deviation << endl;

    return 0;
}

static int test_variant(const char *filename, uint8_t variant_version)
{
    ifstream data(filename);
    data.exceptions(fstream::badbit);
    data.clear(data.rdstate());
    vector<uint64_t> timestamps;
    vector<uint64_t> cumulative_difficulties;
    vector<cryptonote::difficulty_type> wide_cumulative_difficulties;
    int64_t block_time;
    uint64_t difficulty;

    while (data >> block_time >> difficulty) {
        const size_t n = timestamps.size();
        size_t begin, end;
        if (timestamps.size() < DIFFICULTY_BLOCKS_COUNT) {
            begin = 0;
            end = n;
        } else {
            end = n;
            begin = end - DIFFICULTY_BLOCKS_COUNT;
        }

        uint64_t res = cryptonote::next_difficulty_64(
            vector<uint64_t>(timestamps.begin() + begin, timestamps.begin() + end),
            vector<uint64_t>(cumulative_difficulties.begin() + begin, cumulative_difficulties.begin() + end), DEFAULT_TEST_DIFFICULTY_TARGET, n + 1, 0, 0, variant_version);
        if (res != difficulty) {
            cerr << "Wrong difficulty for block " << n << endl
                << "Expected: " << difficulty << endl
                << "Found: " << res << endl;
            return 1;
        }

        cryptonote::difficulty_type wide_res = cryptonote::next_difficulty(
            vector<uint64_t>(timestamps.begin() + begin, timestamps.begin() + end),
            vector<cryptonote::difficulty_type>(wide_cumulative_difficulties.begin() + begin, wide_cumulative_difficulties.begin() + end), DEFAULT_TEST_DIFFICULTY_TARGET, n + 1, 0, 0, variant_version);
        if ((wide_res & 0xffffffffffffffff).convert_to<uint64_t>() != res) {
            cerr << "Wrong wide difficulty for block " << n << endl
                << "Expected: " << res << endl
                << "Found: " << wide_res << endl;
            return 1;
        }

        timestamps.push_back(timestamps.empty() ? block_time : timestamps.back() + block_time);
        cumulative_difficulties.push_back(cumulative_difficulties.empty() ? difficulty : cumulative_difficulties.back() + difficulty);
        wide_cumulative_difficulties.push_back(wide_cumulative_difficulties.empty() ? difficulty : wide_cumulative_difficulties.back() + difficulty);
    }
    if (!data.eof()) {
        data.clear(fstream::badbit);
    }
    return 0;
}

static int test_wide_difficulty(const char *filename)
{
    std::vector<uint64_t> timestamps;
    std::vector<cryptonote::difficulty_type> cumulative_difficulties;
    fstream data(filename, fstream::in);
    data.exceptions(fstream::badbit);
    data.clear(data.rdstate());
    uint64_t timestamp;
    cryptonote::difficulty_type difficulty, cumulative_difficulty = 0;
    size_t n = 0;
    while (data >> timestamp >> difficulty) {
        size_t begin, end;
        if (n < DIFFICULTY_BLOCKS_COUNT) {
            begin = 0;
            end = n;
        } else {
            end = n;
            begin = end - DIFFICULTY_BLOCKS_COUNT;
        }
        cryptonote::difficulty_type res = cryptonote::next_difficulty(
            std::vector<uint64_t>(timestamps.begin() + begin, timestamps.begin() + end),
            std::vector<cryptonote::difficulty_type>(cumulative_difficulties.begin() + begin, cumulative_difficulties.begin() + end), DEFAULT_TEST_DIFFICULTY_TARGET, n, 850, 100, 0);
        if (res != difficulty) {
            cerr << "Wrong wide hoge difficulty for block " << n << endl
                << "Expected: " << difficulty << endl
                << "Found: " << res << endl;
            return 1;
        }
        timestamps.push_back(timestamp);
        cumulative_difficulties.push_back(cumulative_difficulty += difficulty);
        ++n;
    }
    if (!data.eof()) {
        data.clear(fstream::badbit);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    const command_line::arg_descriptor<std::string, true> arg_filename = {"filename", "Input/output file name"};
    const command_line::arg_descriptor<bool> arg_wide = {"wide", "Test wide mode", false};
    const command_line::arg_descriptor<bool> arg_generate_variant = {"generate-variant", "Generate data for algorithm variant", false};
    const command_line::arg_descriptor<bool> arg_test_variant = {"test-variant", "Test data for algorithm variant", false};
    const command_line::arg_descriptor<uint32_t> arg_variant_version  = {"variant-version",  "Variant version", 1};
    po::options_description desc_options("Allowed options");
    command_line::add_arg(desc_options, arg_filename);
    command_line::add_arg(desc_options, arg_wide);
    command_line::add_arg(desc_options, arg_generate_variant);
    command_line::add_arg(desc_options, arg_test_variant);
    command_line::add_arg(desc_options, arg_variant_version);
    po::variables_map vm;
    bool r = command_line::handle_error_helper(desc_options, [&]()
    {
        po::store(po::parse_command_line(argc, argv, desc_options), vm);
        po::notify(vm);
        return true;
    });
    if (!r)
        return 1;

    const std::string filename = command_line::get_arg(vm, arg_filename);
    const bool flag_wide = command_line::get_arg(vm, arg_wide);
    const bool flag_generate_variant = command_line::get_arg(vm, arg_generate_variant);
    const bool flag_test_variant = command_line::get_arg(vm, arg_test_variant);
    const uint32_t variant_version = command_line::get_arg(vm, arg_variant_version);
    if (flag_wide + flag_generate_variant + flag_test_variant > 1)
    {
        cerr << "More than one of --wide, --generate-variant, and --test-variant were specified." << endl;
        return 1;
    }
    if (!flag_generate_variant && !flag_test_variant && !command_line::is_arg_defaulted(vm, arg_variant_version))
    {
        cerr << "--variant-version was specified while neither --generate-variant nor --test-variant wasn't specified." << endl;
        return 1;
    }
    if (variant_version > 255)
    {
        cerr << "Variant version must not be more than 255." << endl;
        return 1;
    }
    if (flag_wide)
    {
        return test_wide_difficulty(filename.c_str());
    }
    if (flag_generate_variant)
    {
        return generate_variant(filename.c_str(), (uint8_t)variant_version);
    }
    if (flag_test_variant)
    {
        return test_variant(filename.c_str(), (uint8_t)variant_version);
    }

    vector<uint64_t> timestamps, cumulative_difficulties;
    std::vector<cryptonote::difficulty_type> wide_cumulative_difficulties;
    fstream data(filename.c_str(), fstream::in);
    data.exceptions(fstream::badbit);
    data.clear(data.rdstate());
    uint64_t timestamp;
    uint64_t difficulty, cumulative_difficulty = 0;
    cryptonote::difficulty_type wide_cumulative_difficulty = 0;
    size_t n = 0;
    while (data >> timestamp >> difficulty) {
        size_t begin, end;
        if (n < DIFFICULTY_BLOCKS_COUNT) {
            begin = 0;
            end = n;
        } else {
            end = n;
            begin = end - DIFFICULTY_BLOCKS_COUNT;
        }
        uint64_t res = cryptonote::next_difficulty_64(
            vector<uint64_t>(timestamps.begin() + begin, timestamps.begin() + end),
            std::vector<uint64_t>(cumulative_difficulties.begin() + begin, cumulative_difficulties.begin() + end), DEFAULT_TEST_DIFFICULTY_TARGET, n + 1, 850, 100, 0);
        if (res != difficulty) {
            cerr << "Wrong difficulty for block " << n << endl
                << "Expected: " << difficulty << endl
                << "Found: " << res << endl;
            return 1;
        }
        cryptonote::difficulty_type wide_res = cryptonote::next_difficulty(
            std::vector<uint64_t>(timestamps.begin() + begin, timestamps.begin() + end),
            std::vector<cryptonote::difficulty_type>(wide_cumulative_difficulties.begin() + begin, wide_cumulative_difficulties.begin() + end), DEFAULT_TEST_DIFFICULTY_TARGET, n + 1, 850, 100, 0);
        if ((wide_res & 0xffffffffffffffff).convert_to<uint64_t>() != res) {
            cerr << "Wrong wide difficulty for block " << n << endl
                << "Expected: " << res << endl
                << "Found: " << wide_res << endl;
            return 1;
        }
        timestamps.push_back(timestamp);
        cumulative_difficulties.push_back(cumulative_difficulty += difficulty);
        wide_cumulative_difficulties.push_back(wide_cumulative_difficulty += difficulty);
        ++n;
    }
    if (!data.eof()) {
        data.clear(fstream::badbit);
    }
    return 0;
}
