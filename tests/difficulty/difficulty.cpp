// Copyright (c) 2014-2024, The Monero Project
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

#include "misc_log_ex.h"
#include "cryptonote_config.h"
#include "cryptonote_basic/difficulty.h"

using namespace std;

#define DEFAULT_TEST_DIFFICULTY_TARGET        120

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
        if (n < DIFFICULTY_WINDOW + DIFFICULTY_LAG) {
            begin = 0;
            end = min(n, (size_t) DIFFICULTY_WINDOW);
        } else {
            end = n - DIFFICULTY_LAG;
            begin = end - DIFFICULTY_WINDOW;
        }
        cryptonote::difficulty_type res = cryptonote::next_difficulty(
            std::vector<uint64_t>(timestamps.begin() + begin, timestamps.begin() + end),
            std::vector<cryptonote::difficulty_type>(cumulative_difficulties.begin() + begin, cumulative_difficulties.begin() + end), DEFAULT_TEST_DIFFICULTY_TARGET);
        if (res != difficulty) {
            cerr << "Wrong wide difficulty for block " << n << endl
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
    TRY_ENTRY();

    if (argc < 2) {
        cerr << "Wrong arguments" << endl;
        return 1;
    }
    if (argc == 3 && strcmp(argv[1], "--wide") == 0)
    {
        return test_wide_difficulty(argv[2]);
    }

    vector<uint64_t> timestamps, cumulative_difficulties;
    std::vector<cryptonote::difficulty_type> wide_cumulative_difficulties;
    fstream data(argv[1], fstream::in);
    data.exceptions(fstream::badbit);
    data.clear(data.rdstate());
    uint64_t timestamp;
    uint64_t difficulty, cumulative_difficulty = 0;
    cryptonote::difficulty_type wide_cumulative_difficulty = 0;
    size_t n = 0;
    while (data >> timestamp >> difficulty) {
        size_t begin, end;
        if (n < DIFFICULTY_WINDOW + DIFFICULTY_LAG) {
            begin = 0;
            end = min(n, (size_t) DIFFICULTY_WINDOW);
        } else {
            end = n - DIFFICULTY_LAG;
            begin = end - DIFFICULTY_WINDOW;
        }
        uint64_t res = cryptonote::next_difficulty_64(
            vector<uint64_t>(timestamps.begin() + begin, timestamps.begin() + end),
            std::vector<uint64_t>(cumulative_difficulties.begin() + begin, cumulative_difficulties.begin() + end), DEFAULT_TEST_DIFFICULTY_TARGET);
        if (res != difficulty) {
            cerr << "Wrong difficulty for block " << n << endl
                << "Expected: " << difficulty << endl
                << "Found: " << res << endl;
            return 1;
        }
        cryptonote::difficulty_type wide_res = cryptonote::next_difficulty(
            std::vector<uint64_t>(timestamps.begin() + begin, timestamps.begin() + end),
            std::vector<cryptonote::difficulty_type>(wide_cumulative_difficulties.begin() + begin, wide_cumulative_difficulties.begin() + end), DEFAULT_TEST_DIFFICULTY_TARGET);
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

    CATCH_ENTRY_L0("main", 1);
}
