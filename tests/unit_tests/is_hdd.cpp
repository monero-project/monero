#include "common/util.h"
#include <string>
#include <gtest/gtest.h>


#if defined(__GLIBC__)

#include <sstream>
#include <sys/sysmacros.h>
#include <fstream>

TEST(is_hdd, linux_os_root)
{
  std::string path = "/";
  boost::optional<bool> is_hdd;
  struct stat st;
  std::string prefix;

  if (stat(path.c_str(), &st) == 0)
  {
    std::ostringstream s;
    s << "/sys/dev/block/" << major(st.st_dev) << ":" << minor(st.st_dev);
    prefix = s.str();
    std::string attr_path = prefix + "/queue/rotational";
    std::ifstream f(attr_path, std::ios_base::in);
    if(f.is_open())
    {
      is_hdd = true;
    }
    else
    {
      attr_path = prefix + "/../queue/rotational";
      f.open(attr_path, std::ios_base::in);
      if (f.is_open())
	is_hdd = true;
    }
  }
  EXPECT_TRUE(tools::is_hdd(path.c_str()) == is_hdd);
}
#else
TEST(is_hdd, unknown_os)
{
  std::string path = "";
  EXPECT_FALSE(tools::is_hdd(path.c_str()) != boost::none);
}
#endif
