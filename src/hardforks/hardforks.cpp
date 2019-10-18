// Copyright (c) 2014-2019, The Monero Project
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

#include "hardforks.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "blockchain.hardforks"

const hardfork_t mainnet_hard_forks[] = {
  // version 1 from the start of the blockchain
  { 1, 1, 0, 1341378000 },

  // version 2 starts from block 1009827, which is on or around the 20th of March, 2016. Fork time finalised on 2015-09-20. No fork voting occurs for the v2 fork.
  { 2, 1009827, 0, 1442763710 },

  // version 3 starts from block 1141317, which is on or around the 24th of September, 2016. Fork time finalised on 2016-03-21.
  { 3, 1141317, 0, 1458558528 },

  // version 4 starts from block 1220516, which is on or around the 5th of January, 2017. Fork time finalised on 2016-09-18.
  { 4, 1220516, 0, 1483574400 },

  // version 5 starts from block 1288616, which is on or around the 15th of April, 2017. Fork time finalised on 2017-03-14.
  { 5, 1288616, 0, 1489520158 }, 

  // version 6 starts from block 1400000, which is on or around the 16th of September, 2017. Fork time finalised on 2017-08-18.
  { 6, 1400000, 0, 1503046577 },

  // version 7 starts from block 1546000, which is on or around the 6th of April, 2018. Fork time finalised on 2018-03-17.
  { 7, 1546000, 0, 1521303150 },

  // version 8 starts from block 1685555, which is on or around the 18th of October, 2018. Fork time finalised on 2018-09-02.
  { 8, 1685555, 0, 1535889547 },

  // version 9 starts from block 1686275, which is on or around the 19th of October, 2018. Fork time finalised on 2018-09-02.
  { 9, 1686275, 0, 1535889548 },

  // version 10 starts from block 1788000, which is on or around the 9th of March, 2019. Fork time finalised on 2019-02-10.
  { 10, 1788000, 0, 1549792439 },

  // version 11 starts from block 1788720, which is on or around the 10th of March, 2019. Fork time finalised on 2019-02-15.
  { 11, 1788720, 0, 1550225678 },

  // version 12 starts from block 1978433, which is on or around the 30th of November, 2019. Fork time finalised on 2019-10-18.
  { 12, 1978433, 0, 1571419280 },
};
const size_t num_mainnet_hard_forks = sizeof(mainnet_hard_forks) / sizeof(mainnet_hard_forks[0]);
const uint64_t mainnet_hard_fork_version_1_till = 1009826;

const hardfork_t testnet_hard_forks[] = {
  // version 1 from the start of the blockchain
  { 1, 1, 0, 1341378000 },

  // version 2 starts from block 624634, which is on or around the 23rd of November, 2015. Fork time finalised on 2015-11-20. No fork voting occurs for the v2 fork.
  { 2, 624634, 0, 1445355000 },

  // versions 3-5 were passed in rapid succession from September 18th, 2016
  { 3, 800500, 0, 1472415034 },
  { 4, 801219, 0, 1472415035 },
  { 5, 802660, 0, 1472415036 + 86400*180 }, // add 5 months on testnet to shut the update warning up since there's a large gap to v6

  { 6, 971400, 0, 1501709789 },
  { 7, 1057027, 0, 1512211236 },
  { 8, 1057058, 0, 1533211200 },
  { 9, 1057778, 0, 1533297600 },
  { 10, 1154318, 0, 1550153694 },
  { 11, 1155038, 0, 1550225678 },
  { 12, 1308737, 0, 1569582000 },
};
const size_t num_testnet_hard_forks = sizeof(testnet_hard_forks) / sizeof(testnet_hard_forks[0]);
const uint64_t testnet_hard_fork_version_1_till = 624633;

const hardfork_t stagenet_hard_forks[] = {
  // version 1 from the start of the blockchain
  { 1, 1, 0, 1341378000 },

  // versions 2-7 in rapid succession from March 13th, 2018
  { 2, 32000, 0, 1521000000 },
  { 3, 33000, 0, 1521120000 },
  { 4, 34000, 0, 1521240000 },
  { 5, 35000, 0, 1521360000 },
  { 6, 36000, 0, 1521480000 },
  { 7, 37000, 0, 1521600000 },
  { 8, 176456, 0, 1537821770 },
  { 9, 177176, 0, 1537821771 },
  { 10, 269000, 0, 1550153694 },
  { 11, 269720, 0, 1550225678 },
  { 12, 454721, 0, 1571419280 },
};
const size_t num_stagenet_hard_forks = sizeof(stagenet_hard_forks) / sizeof(stagenet_hard_forks[0]);
