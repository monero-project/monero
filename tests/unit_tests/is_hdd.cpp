#include "common/util.h"
#include <cstdlib>
#include <string>
#include <gtest/gtest.h>
#include <boost/optional/optional_io.hpp> /* required to output boost::optional in assertions */

#ifndef GTEST_SKIP
#include <iostream>
#define SKIP_TEST(reason) do {std::cerr << "Skipping test: " << reason << std::endl; return;} while(0)
#else
#define SKIP_TEST(reason) GTEST_SKIP() << reason
#endif

#if defined(__GLIBC__)
TEST(is_hdd, rotational_drive) {
  const char *hdd = std::getenv("MONERO_TEST_DEVICE_HDD");
  if (hdd == nullptr)
    SKIP_TEST("No rotational disk device configured");
  EXPECT_EQ(tools::is_hdd(hdd), boost::optional<bool>(true));
}

TEST(is_hdd, ssd) {
  const char *ssd = std::getenv("MONERO_TEST_DEVICE_SSD");
  if (ssd == nullptr)
    SKIP_TEST("No SSD device configured");
  EXPECT_EQ(tools::is_hdd(ssd), boost::optional<bool>(false));
}

TEST(is_hdd, unknown_attrs) {
  EXPECT_EQ(tools::is_hdd("/dev/null"), boost::none);
}
#endif
TEST(is_hdd, stability)
{
  EXPECT_NO_THROW(tools::is_hdd(""));
}
