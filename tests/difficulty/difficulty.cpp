// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

#include "cryptonote_config.h"
#include "cryptonote_core/difficulty.h"

using namespace std;

#define DEFAULT_TEST_DIFFICULTY_TARGET        120

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Wrong arguments" << endl;
        return 1;
    }
    vector<uint64_t> timestamps, cumulative_difficulties;
    fstream data(argv[1], fstream::in);
    data.exceptions(fstream::badbit);
    data.clear(data.rdstate());
    uint64_t timestamp, difficulty, cumulative_difficulty = 0;
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
        uint64_t res = cryptonote::next_difficulty(
            vector<uint64_t>(timestamps.begin() + begin, timestamps.begin() + end),
            vector<uint64_t>(cumulative_difficulties.begin() + begin, cumulative_difficulties.begin() + end), DEFAULT_TEST_DIFFICULTY_TARGET);
        if (res != difficulty) {
            cerr << "Wrong difficulty for block " << n << endl
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
