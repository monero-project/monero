#include "common/util.h"
#include <string>
#include <gtest/gtest.h>

#if defined(__GLIBC__)
TEST(test_a, is_hdd_linux_os_root)
{
  bool result;
  std::string path = "/";
  EXPECT_TRUE(tools::is_hdd(path.c_str(), result));
}
#elif defined(_WIN32) and (_WIN32_WINNT >= 0x0601)
TEST(test_a, is_hdd_win_os_c)
{
  bool result;
  std::string path = "\\\\?\\C:\\Windows\\System32\\cmd.exe";
  EXPECT_TRUE(tools::is_hdd(path.c_str(), result));
  path = "\\\\?\\C:\\";
  EXPECT_TRUE(tools::is_hdd(path.c_str(), result));
  path = "C:\\";
  EXPECT_TRUE(tools::is_hdd(path.c_str(), result));
}
#else
TEST(test_a, is_hdd_unknown_os)
{
  bool result;
  std::string path = "";
  EXPECT_FALSE(tools::is_hdd(path.c_str(), result));
}
#endif
