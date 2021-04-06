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
  { 1, 1, 0, 1541014386 },
  { 2, 8, 0, 1541014391 },
  { 3, 100, 0, 1541014463 },
  { 4, 45000, 0, 1549695692 },
  { 5, 106950, 0, 1560481469 },
  { 6, 181056, 0, 1573931994 },
  { 7, 352846, 0, 1595030400 },
  { 8, 426143, 0, 1603945507 },
  { 9, 500000, 0, 1612744443 },
  { 10, 548492 , 0, 1612161143}

};
const size_t num_mainnet_hard_forks = sizeof(mainnet_hard_forks) / sizeof(mainnet_hard_forks[0]);
const uint64_t mainnet_hard_fork_version_1_till = 1009826;

const hardfork_t testnet_hard_forks[] = {
  { 1, 1, 0, 1341378000 },

  { 2, 8, 0, 1445355000 },

  { 3, 10, 0, 1472415034 },
  { 4, 11, 0, 1472415035 },
  { 5, 12, 0, 1551499880 },
  { 6, 13, 0, 1571531327},
  { 7, 14, 0, 1581531327},
  { 8, 15, 0, 1591531327},
  { 9, 75 , 0, 1612161143},
  { 10, 160 , 0, 1692161143}


};
const size_t num_testnet_hard_forks = sizeof(testnet_hard_forks) / sizeof(testnet_hard_forks[0]);
const uint64_t testnet_hard_fork_version_1_till = 1;

const hardfork_t stagenet_hard_forks[] = {
  { 1, 1, 0, 1341378000 }

};
const size_t num_stagenet_hard_forks = sizeof(stagenet_hard_forks) / sizeof(stagenet_hard_forks[0]);
