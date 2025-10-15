// Copyright (c) 2019-2025, The Monero Project
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

// References:
// - https://github.com/ArticMine/Monero-Documents/blob/master/MoneroScaling2021.pdf
// - https://github.com/monero-project/research-lab/issues/70

#include "gtest/gtest.h"
#include "cryptonote_core/blockchain.h"

TEST(fee_2021_scaling, relay_fee_cases_from_pdf)
{
  ASSERT_EQ(cryptonote::Blockchain::get_dynamic_base_fee(1200000000000, 300000), 38000);
  ASSERT_EQ(cryptonote::Blockchain::get_dynamic_base_fee(1200000000000, 1425000), 1684 /*1680*/);
  ASSERT_EQ(cryptonote::Blockchain::get_dynamic_base_fee(1200000000000, 1500000), 1520);

  ASSERT_EQ(cryptonote::Blockchain::get_dynamic_base_fee(600000000000, 300000), 19000);
  ASSERT_EQ(cryptonote::Blockchain::get_dynamic_base_fee(600000000000, 1425000), 842 /*840*/);
  ASSERT_EQ(cryptonote::Blockchain::get_dynamic_base_fee(600000000000, 1500000), 760);
}

TEST(fee_2021_scaling, wallet_fee_cases_from_pdf)
{
  std::vector<uint64_t> fees;

  fees.clear();
  cryptonote::Blockchain::get_dynamic_base_fee_estimate_2021_scaling(600000000000, 300000, 300000, fees);
  ASSERT_EQ(fees.size(), 4);
  ASSERT_EQ(fees[0], 20000);
  ASSERT_EQ(fees[1], 80000);
  ASSERT_EQ(fees[2], 320000);
  ASSERT_EQ(fees[3], 4000000);

  fees.clear();
  cryptonote::Blockchain::get_dynamic_base_fee_estimate_2021_scaling(600000000000, 15000000, 300000, fees);
  ASSERT_EQ(fees.size(), 4);
  ASSERT_EQ(fees[0], 20000);
  ASSERT_EQ(fees[1], 80000);
  ASSERT_EQ(fees[2], 320000);
  ASSERT_EQ(fees[3], 1300000);

  fees.clear();
  cryptonote::Blockchain::get_dynamic_base_fee_estimate_2021_scaling(600000000000, 1425000, 1425000, fees);
  ASSERT_EQ(fees.size(), 4);
  ASSERT_EQ(fees[0], 890);
  ASSERT_EQ(fees[1], 3600);
  ASSERT_EQ(fees[2], 68000);
  ASSERT_EQ(fees[3], 850000 /* 842000 */);

  fees.clear();
  cryptonote::Blockchain::get_dynamic_base_fee_estimate_2021_scaling(600000000000, 1500000, 1500000, fees);
  ASSERT_EQ(fees.size(), 4);
  ASSERT_EQ(fees[0], 800);
  ASSERT_EQ(fees[1], 3200);
  ASSERT_EQ(fees[2], 64000);
  ASSERT_EQ(fees[3], 800000);

  fees.clear();
  cryptonote::Blockchain::get_dynamic_base_fee_estimate_2021_scaling(600000000000, 75000000, 1500000, fees);
  ASSERT_EQ(fees.size(), 4);
  ASSERT_EQ(fees[0], 800);
  ASSERT_EQ(fees[1], 3200);
  ASSERT_EQ(fees[2], 64000);
  ASSERT_EQ(fees[3], 260000);
}

TEST(fee_2021_scaling, rounding)
{
  ASSERT_EQ(cryptonote::round_money_up("27810", 3), "27900.000000000000");
  ASSERT_EQ(cryptonote::round_money_up("37.94", 3), "38.000000000000");
  ASSERT_EQ(cryptonote::round_money_up("0.5555", 3), "0.556000000000");
  ASSERT_EQ(cryptonote::round_money_up("0.002342", 3), "0.002350000000");

  ASSERT_EQ(cryptonote::round_money_up("27810", 2), "28000.000000000000");
  ASSERT_EQ(cryptonote::round_money_up("37.94", 2), "38.000000000000");
  ASSERT_EQ(cryptonote::round_money_up("0.5555", 2), "0.560000000000");
  ASSERT_EQ(cryptonote::round_money_up("0.002342", 2), "0.002400000000");

  ASSERT_EQ(cryptonote::round_money_up("0", 8), "0.000000000000");
  ASSERT_EQ(cryptonote::round_money_up("0.0", 8), "0.000000000000");
  ASSERT_EQ(cryptonote::round_money_up("50.0", 8), "50.000000000000");
  ASSERT_EQ(cryptonote::round_money_up("0.002342", 8), "0.002342000000");
  ASSERT_EQ(cryptonote::round_money_up("0.002342", 1), "0.003000000000");
  ASSERT_EQ(cryptonote::round_money_up("12345", 8), "12345.000000000000");
  ASSERT_EQ(cryptonote::round_money_up("45678", 1), "50000.000000000000");
  ASSERT_EQ(cryptonote::round_money_up("1.234", 1), "2.000000000000");
  ASSERT_EQ(cryptonote::round_money_up("1.0000001", 4), "1.001000000000");
  ASSERT_EQ(cryptonote::round_money_up("1.0020001", 4), "1.003000000000");

  ASSERT_EQ(cryptonote::round_money_up("1.999999", 1), "2.000000000000");
  ASSERT_EQ(cryptonote::round_money_up("1.999999", 2), "2.000000000000");
  ASSERT_EQ(cryptonote::round_money_up("1.999999", 3), "2.000000000000");
  ASSERT_EQ(cryptonote::round_money_up("1.999999", 4), "2.000000000000");
  ASSERT_EQ(cryptonote::round_money_up("1.999999", 5), "2.000000000000");
  ASSERT_EQ(cryptonote::round_money_up("1.999999", 6), "2.000000000000");
  ASSERT_EQ(cryptonote::round_money_up("1.999999", 7), "1.999999000000");
  ASSERT_EQ(cryptonote::round_money_up("1.999999", 8), "1.999999000000");
  ASSERT_EQ(cryptonote::round_money_up("1.999999", 9), "1.999999000000");

  ASSERT_EQ(cryptonote::round_money_up("2.000001", 1), "3.000000000000");
  ASSERT_EQ(cryptonote::round_money_up("2.000001", 2), "2.100000000000");
  ASSERT_EQ(cryptonote::round_money_up("2.000001", 3), "2.010000000000");
  ASSERT_EQ(cryptonote::round_money_up("2.000001", 4), "2.001000000000");
  ASSERT_EQ(cryptonote::round_money_up("2.000001", 5), "2.000100000000");
  ASSERT_EQ(cryptonote::round_money_up("2.000001", 6), "2.000010000000");
  ASSERT_EQ(cryptonote::round_money_up("2.000001", 7), "2.000001000000");
  ASSERT_EQ(cryptonote::round_money_up("2.000001", 8), "2.000001000000");
  ASSERT_EQ(cryptonote::round_money_up("2.000001", 9), "2.000001000000");
  ASSERT_EQ(cryptonote::round_money_up("2.000001", 4000), "2.000001000000");

  ASSERT_EQ(cryptonote::round_money_up("999", 2), "1000.000000000000");

  ASSERT_THROW(cryptonote::round_money_up("1.23", 0), std::runtime_error);
  ASSERT_THROW(cryptonote::round_money_up("18446744.073709551615", 1), std::runtime_error);
  ASSERT_THROW(cryptonote::round_money_up("18446744.073709551615", 2), std::runtime_error);
  ASSERT_THROW(cryptonote::round_money_up("18446744.073709551615", 12), std::runtime_error);
  ASSERT_THROW(cryptonote::round_money_up("18446744.073709551615", 19), std::runtime_error);
  ASSERT_EQ(cryptonote::round_money_up("18446744.073709551615", 20), "18446744.073709551615");
}
