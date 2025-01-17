#include "common/util.h"
#include <cstdlib>
#include <string>
#include <gtest/gtest.h>
#include <boost/optional/optional_io.hpp> /* required to output boost::optional in assertions */

#if defined(__GLIBC__)
TEST(is_hdd, rotational_drive) {
  const char *hdd = std::getenv("MONERO_TEST_DEVICE_HDD");
  if (hdd == nullptr)
    GTEST_SKIP() << "No rotational disk device configured";
  EXPECT_EQ(tools::is_hdd(hdd), boost::optional<bool>(true));
}

TEST(is_hdd, ssd) {
  const char *ssd = std::getenv("MONERO_TEST_DEVICE_SSD");
  if (ssd == nullptr)
    GTEST_SKIP() << "No SSD device configured";
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
