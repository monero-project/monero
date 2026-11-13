// Copyright (c) 2023-2024, The Monero Project
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

#include "gtest/gtest.h"

#include <boost/filesystem.hpp>
#include <memory>

#ifdef _WIN32
  #include <windows.h>
  #include <aclapi.h>
#endif

#include "common/util.h"
#include "file_io_utils.h"

namespace
{
#ifdef _WIN32
  struct local_free
  {
    void operator()(void* ptr) const noexcept { LocalFree(ptr); }
  };
#endif

  struct temp_dir
  {
    const boost::filesystem::path path = boost::filesystem::temp_directory_path() /
      boost::filesystem::unique_path("util_test-%%%%-%%%%-%%%%-%%%%");

    temp_dir() { boost::filesystem::create_directories(path); }
    ~temp_dir()
    {
      boost::system::error_code ec;
      boost::filesystem::remove_all(path, ec);
    }
  };

#ifndef _WIN32
  boost::filesystem::perms file_perms(const boost::filesystem::path& path)
  {
    return boost::filesystem::symlink_status(path).permissions() & boost::filesystem::perms::all_all;
  }
#endif
}

TEST(save_private_file, writes_owner_only_file)
{
  const temp_dir dir;
  const boost::filesystem::path file = dir.path / "key";

  ASSERT_FALSE(tools::save_private_file(file.string(), "secret"));

  std::string loaded;
  ASSERT_TRUE(epee::file_io_utils::load_file_to_string(file.string(), loaded));
  EXPECT_EQ("secret", loaded);
  EXPECT_FALSE(boost::filesystem::exists(file.string() + ".tmp"));
#ifndef _WIN32
  EXPECT_EQ(boost::filesystem::perms::owner_read | boost::filesystem::perms::owner_write, file_perms(file));
#endif
}

TEST(save_private_file, replaces_existing_file)
{
  const temp_dir dir;
  const boost::filesystem::path file = dir.path / "key";
  ASSERT_TRUE(epee::file_io_utils::save_string_to_file(file.string(), "old contents that are longer"));
#ifndef _WIN32
  boost::filesystem::permissions(file, boost::filesystem::perms::owner_all | boost::filesystem::perms::group_read | boost::filesystem::perms::others_read);
#endif
  ASSERT_TRUE(epee::file_io_utils::save_string_to_file(file.string() + ".tmp", "stale"));

  ASSERT_FALSE(tools::save_private_file(file.string(), "new"));

  std::string loaded;
  ASSERT_TRUE(epee::file_io_utils::load_file_to_string(file.string(), loaded));
  EXPECT_EQ("new", loaded);
  EXPECT_FALSE(boost::filesystem::exists(file.string() + ".tmp"));
#ifndef _WIN32
  EXPECT_EQ(boost::filesystem::perms::owner_read | boost::filesystem::perms::owner_write, file_perms(file));
#endif
}

#ifndef _WIN32
TEST(save_private_file, does_not_follow_symlinks)
{
  const temp_dir dir;
  const boost::filesystem::path target = dir.path / "target";
  const boost::filesystem::path file = dir.path / "key";
  ASSERT_TRUE(epee::file_io_utils::save_string_to_file(target.string(), "untouched"));
  boost::filesystem::create_symlink(target, file);
  boost::filesystem::create_symlink(target, file.string() + ".tmp");

  ASSERT_FALSE(tools::save_private_file(file.string(), "secret"));

  std::string loaded;
  ASSERT_TRUE(epee::file_io_utils::load_file_to_string(target.string(), loaded));
  EXPECT_EQ("untouched", loaded);
  ASSERT_TRUE(epee::file_io_utils::load_file_to_string(file.string(), loaded));
  EXPECT_EQ("secret", loaded);
  EXPECT_FALSE(boost::filesystem::is_symlink(file));
  EXPECT_EQ(boost::filesystem::perms::owner_read | boost::filesystem::perms::owner_write, file_perms(file));
}
#endif

#ifdef _WIN32
TEST(save_private_file, ignores_inheritable_directory_permissions)
{
  const temp_dir dir;
  const std::wstring wide_dir = dir.path.wstring();

  PACL dir_dacl = nullptr;
  PSECURITY_DESCRIPTOR dir_sd = nullptr;
  ASSERT_EQ(DWORD(ERROR_SUCCESS), GetNamedSecurityInfoW(wide_dir.c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &dir_dacl, nullptr, &dir_sd));
  const std::unique_ptr<void, local_free> dir_sd_owner{dir_sd};

  BYTE everyone[SECURITY_MAX_SID_SIZE];
  DWORD everyone_size = sizeof(everyone);
  ASSERT_TRUE(CreateWellKnownSid(WinWorldSid, nullptr, everyone, &everyone_size));

  EXPLICIT_ACCESSW access{};
  access.grfAccessPermissions = FILE_GENERIC_READ;
  access.grfAccessMode = GRANT_ACCESS;
  access.grfInheritance = OBJECT_INHERIT_ACE;
  access.Trustee.TrusteeForm = TRUSTEE_IS_SID;
  access.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
  access.Trustee.ptstrName = reinterpret_cast<LPWSTR>(everyone);

  PACL new_dacl = nullptr;
  ASSERT_EQ(DWORD(ERROR_SUCCESS), SetEntriesInAclW(1, &access, dir_dacl, &new_dacl));
  const std::unique_ptr<void, local_free> new_dacl_owner{new_dacl};
  ASSERT_EQ(DWORD(ERROR_SUCCESS), SetNamedSecurityInfoW(const_cast<LPWSTR>(wide_dir.c_str()), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, new_dacl, nullptr));

  const boost::filesystem::path file = dir.path / "key";
  ASSERT_FALSE(tools::save_private_file(file.string(), "secret"));

  PACL file_dacl = nullptr;
  PSECURITY_DESCRIPTOR file_sd = nullptr;
  ASSERT_EQ(DWORD(ERROR_SUCCESS), GetNamedSecurityInfoW(file.wstring().c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &file_dacl, nullptr, &file_sd));
  const std::unique_ptr<void, local_free> file_sd_owner{file_sd};

  SECURITY_DESCRIPTOR_CONTROL control{};
  DWORD revision = 0;
  ASSERT_TRUE(GetSecurityDescriptorControl(file_sd, &control, &revision));
  EXPECT_TRUE(control & SE_DACL_PROTECTED);

  ASSERT_NE(nullptr, file_dacl);
  EXPECT_EQ(1u, file_dacl->AceCount);
  for (DWORD i = 0; i < file_dacl->AceCount; ++i)
  {
    void* ace = nullptr;
    ASSERT_TRUE(GetAce(file_dacl, i, &ace));
    const ACE_HEADER* header = static_cast<const ACE_HEADER*>(ace);
    EXPECT_FALSE(header->AceFlags & INHERITED_ACE);
    if (header->AceType == ACCESS_ALLOWED_ACE_TYPE)
      EXPECT_FALSE(EqualSid(&static_cast<ACCESS_ALLOWED_ACE*>(ace)->SidStart, everyone));
  }
}
#endif

TEST(save_private_file, fails_in_missing_directory)
{
  const temp_dir dir;
  const boost::filesystem::path file = dir.path / "missing" / "key";

  EXPECT_TRUE(tools::save_private_file(file.string(), "secret"));
  EXPECT_FALSE(boost::filesystem::exists(file));
}

TEST(LocalAddress, localhost) { ASSERT_TRUE(tools::is_local_address("localhost")); }
TEST(LocalAddress, localhost_port) { ASSERT_TRUE(tools::is_local_address("localhost:18081")); }
TEST(LocalAddress, localhost_suffix) { ASSERT_TRUE(tools::is_local_address("test.localhost")); }
TEST(LocalAddress, loopback) { ASSERT_TRUE(tools::is_local_address("127.0.0.1")); }
TEST(LocalAddress, loopback_port) { ASSERT_TRUE(tools::is_local_address("127.0.0.1:18081")); }
TEST(LocalAddress, loopback_protocol) { ASSERT_TRUE(tools::is_local_address("http://127.0.0.1")); }
TEST(LocalAddress, loopback_hi) { ASSERT_TRUE(tools::is_local_address("127.255.255.255")); }
TEST(LocalAddress, loopback_lo) { ASSERT_TRUE(tools::is_local_address("127.0.0.0")); }
TEST(LocalAddress, loopback_ipv6) { ASSERT_TRUE(tools::is_local_address("[0:0:0:0:0:0:0:1]")); }

TEST(LocalAddress, onion) { ASSERT_FALSE(tools::is_local_address("vww6ybal4bd7szmgncyruucpgfkqahzddi37ktceo3ah7ngmcopnpyyd.onion")); }
TEST(LocalAddress, i2p) { ASSERT_FALSE(tools::is_local_address("xmrto2bturnore26xmrto2bturnore26xmrto2bturnore26xmr2.b32.i2p")); }
TEST(LocalAddress, valid_ip) { ASSERT_FALSE(tools::is_local_address("1.2.3.4")); }
TEST(LocalAddress, valid_ipv6) { ASSERT_FALSE(tools::is_local_address("[0:0:0:0:0:0:0:2]")); }
TEST(LocalAddress, valid_domain) { ASSERT_FALSE(tools::is_local_address("getmonero.org")); }
TEST(LocalAddress, local_prefix) { ASSERT_FALSE(tools::is_local_address("localhost.com")); }
TEST(LocalAddress, invalid) { ASSERT_FALSE(tools::is_local_address("test")); }
TEST(LocalAddress, empty) { ASSERT_FALSE(tools::is_local_address("")); }
