// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>

#include "currency_core/currency_config.h"
#include "currency_core/difficulty.h"

using namespace std;

#define DEFAULT_TEST_DIFFICULTY_TARGET        120

int test_big_difficulties(const char* dataFile)
{
    vector<uint64_t> timestamps;
    vector<currency::wide_difficulty_type> cumulative_difficulties;
    fstream data(dataFile, fstream::in);
    data.exceptions(fstream::badbit);
    data.clear(data.rdstate());
    uint64_t timestamp;
    currency::wide_difficulty_type difficulty, cumulative_difficulty = 0;
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
        currency::wide_difficulty_type res = currency::next_difficulty(
            vector<uint64_t>(timestamps.begin() + begin, timestamps.begin() + end),
            vector<currency::wide_difficulty_type>(cumulative_difficulties.begin() + begin, cumulative_difficulties.begin() + end), DEFAULT_TEST_DIFFICULTY_TARGET);
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
    if (argc < 2) {
        cerr << "Wrong arguments" << endl;
        return 1;
    }
	if(argc == 3 && strcmp(argv[1], "/b") == 0)
	{
		return test_big_difficulties(argv[2]);
	}
    vector<uint64_t> timestamps, cumulative_difficulties;
    vector<currency::wide_difficulty_type> wide_cumulative_difficulties;
    fstream data(argv[1], fstream::in);
    data.exceptions(fstream::badbit);
    data.clear(data.rdstate());
    uint64_t timestamp, difficulty, cumulative_difficulty = 0;
    currency::wide_difficulty_type wide_cumulative_difficulty = 0;
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
        uint64_t res = currency::next_difficulty_old(
            vector<uint64_t>(timestamps.begin() + begin, timestamps.begin() + end),
            vector<uint64_t>(cumulative_difficulties.begin() + begin, cumulative_difficulties.begin() + end), DEFAULT_TEST_DIFFICULTY_TARGET);
        if (res != difficulty) {
            cerr << "Wrong difficulty for block " << n << endl
                << "Expected: " << difficulty << endl
                << "Found: " << res << endl;
            return 1;
        }
        currency::wide_difficulty_type wide_res = currency::next_difficulty(
            vector<uint64_t>(timestamps.begin() + begin, timestamps.begin() + end),
            vector<currency::wide_difficulty_type>(wide_cumulative_difficulties.begin() + begin, wide_cumulative_difficulties.begin() + end), DEFAULT_TEST_DIFFICULTY_TARGET);
        if (wide_res.convert_to<uint64_t>() != res) {
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
