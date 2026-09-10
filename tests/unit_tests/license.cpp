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

#include "license/monero_license.h"
#include "misc_log_ex.h"

#include <gtest/gtest.h>
#include <string>
#include <time.h>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "unit_test.license"

TEST(license, monero_first)
{
    ASSERT_GE(monero_license_num(), 1);
    const monero_license_entry_t license_entry = monero_license_get(0);
    ASSERT_EQ("The Monero Project", std::string(license_entry.holder, license_entry.holder_len));
    ASSERT_EQ("https://github.com/monero-project/monero",
        std::string(license_entry.source_location, license_entry.source_location_len));
    const std::string license_text(license_entry.text, license_entry.text_len); 
    ASSERT_NE(std::string::npos, license_text.find("All rights reserved"));
    ASSERT_NE(std::string::npos, license_text.find("POSSIBILITY OF SUCH DAMAGE."));
}

TEST(license, monero_updated_year)
{
    // Whether Monero's license contains current year

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    const int year = 1900 + tm.tm_year;

    ASSERT_GE(monero_license_num(), 1);
    const monero_license_entry_t license_entry = monero_license_get(0);
    const std::string license_year(license_entry.year, license_entry.year_len);
    ASSERT_NE(std::string::npos, license_year.find(std::to_string(year)));
}

TEST(license, dump_all_short)
{
    const size_t n_licenses = monero_license_num();
    for (size_t i = 0; i < n_licenses; ++i)
    {
        const monero_license_entry_t license_entry = monero_license_get(i);
        const std::string year(license_entry.year, license_entry.year_len);
        const std::string holder(license_entry.holder, license_entry.holder_len);
        const std::string source_location(license_entry.source_location, license_entry.source_location_len);
        const std::string text(license_entry.text, license_entry.text_len);

        LOG_PRINT_L1("Copyright (c) " << year << " ~ " << holder << " ~ " << source_location);
        LOG_PRINT_L1("    " << text.substr(0, std::min<size_t>(text.size(), 160)) << " ...");
        LOG_PRINT_L1("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%");
    }
}
