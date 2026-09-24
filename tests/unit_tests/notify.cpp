// Copyright (c) 2018-2024, The Monero Project

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

#include <cstdlib>
#ifndef _WIN32
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "gtest/gtest.h"

#include <boost/filesystem.hpp>

#include "misc_language.h"
#include "string_tools.h"
#include "file_io_utils.h"
#include "common/notify.h"
#include "common/spawn.h"

TEST(notify, works)
{
#ifdef __GLIBC__
  mode_t prevmode = umask(077);
#endif
  const char *tmp = getenv("TEMP");
  if (!tmp)
    tmp = "/tmp";
  static const char *filename = "monero-notify-unit-test-XXXXXX";
  const size_t len = strlen(tmp) + 1 + strlen(filename);
  std::unique_ptr<char[]> name_template_(new char[len + 1]);
  char *name_template = name_template_.get();
  ASSERT_TRUE(name_template != NULL);
  snprintf(name_template, len + 1, "%s/%s", tmp, filename);
  int fd = mkstemp(name_template);
#ifdef __GLIBC__
  umask(prevmode);
#endif
  ASSERT_TRUE(fd >= 0);
  close(fd);

  const std::string spec = epee::string_tools::get_current_module_folder() + "/test_notifier"
#ifdef _WIN32
      + ".exe"
#endif
      + " " + name_template + " %s";

  tools::Notify notify(spec.c_str());
  notify.notify("%s", "1111111111111111111111111111111111111111111111111111111111111111", NULL);

  bool ok = false;
  for (int i = 0; i < 10; ++i)
  {
    epee::misc_utils::sleep_no_w(100);

    std::string s;
    if (epee::file_io_utils::load_file_to_string(name_template, s))
    {
      if (s == "1111111111111111111111111111111111111111111111111111111111111111")
      {
        ok = true;
        break;
      }
    }
  }
  boost::filesystem::remove(name_template);
  ASSERT_TRUE(ok);
}

#ifndef _WIN32
TEST(notify, exec_failure_exits_child)
{
  const boost::filesystem::path test_dir = boost::filesystem::temp_directory_path() /
      boost::filesystem::unique_path("monero-notify-unit-test-%%%%-%%%%-%%%%");
  ASSERT_TRUE(boost::filesystem::create_directory(test_dir));

  const boost::filesystem::path interpreter = test_dir / "missing-interpreter";
  const boost::filesystem::path script = test_dir / "script";
  const boost::filesystem::path child_returned = test_dir / "child-returned";
  ASSERT_TRUE(epee::file_io_utils::save_string_to_file(script.string(), "#!" + interpreter.string() + "\n"));
  ASSERT_EQ(chmod(script.c_str(), 0700), 0);

  void (*previous_sigchld_handler)(int) = signal(SIGCHLD, SIG_DFL);
  ASSERT_NE(previous_sigchld_handler, SIG_ERR);
  const pid_t parent_pid = getpid();
  const int result = tools::spawn(script.c_str(), {script.string()}, true);
  if (getpid() != parent_pid)
  {
    const int fd = open(child_returned.c_str(), O_CREAT | O_WRONLY, 0600);
    if (fd >= 0)
      close(fd);
    _exit(0);
  }

  EXPECT_EQ(result, EXIT_FAILURE);
  EXPECT_FALSE(boost::filesystem::exists(child_returned));
  EXPECT_NE(signal(SIGCHLD, previous_sigchld_handler), SIG_ERR);
  boost::filesystem::remove_all(test_dir);
}
#endif
