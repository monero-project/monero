#include "common/util.h"
#include "3rd/boost_monero_optional.h"
#include <string>
#include <gtest/gtest.h>

#if defined(__GLIBC__)
TEST(is_hdd, linux_os_root)
{
  std::string path = "/";
  EXPECT_TRUE(tools::is_hdd(path.c_str()) != boost::none);
}
#else
TEST(is_hdd, unknown_os)
{
  std::string path = "";
  EXPECT_FALSE(tools::is_hdd(path.c_str()) != boost::none);
}
#endif
