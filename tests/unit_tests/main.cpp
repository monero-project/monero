// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"

#include "include_base_utils.h"

int main(int argc, char** argv)
{
  epee::debug::get_set_enable_assert(true, false);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
