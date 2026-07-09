#include "common/download.h"

#include "gtest/gtest.h"

TEST(download, content_range_starts_at)
{
  EXPECT_TRUE(tools::content_range_starts_at("bytes 1024-2047/4096", 1024));
  EXPECT_TRUE(tools::content_range_starts_at("bytes 1024-", 1024));

  EXPECT_FALSE(tools::content_range_starts_at("bytes 0-2047/4096", 1024));
  EXPECT_FALSE(tools::content_range_starts_at("bytes 10240-20479/40960", 1024));
  EXPECT_FALSE(tools::content_range_starts_at("bytes=1024-2047/4096", 1024));
  EXPECT_FALSE(tools::content_range_starts_at("", 1024));
}
