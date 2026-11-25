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
#include <algorithm>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#ifdef _WIN32
  #include <windows.h>
  #include <aclapi.h>
#else
  #include <sys/stat.h>
#endif
#ifdef __APPLE__
  #include <sys/acl.h>
#endif

#include "common/util.h"
#include "file_io_utils.h"

namespace
{
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

  //! Number of entries in `dir`, to check that no temporary file was left behind.
  std::size_t entry_count(const boost::filesystem::path& dir)
  {
    return std::size_t(std::distance(boost::filesystem::directory_iterator{dir}, boost::filesystem::directory_iterator{}));
  }

#ifndef _WIN32
  boost::filesystem::perms file_perms(const boost::filesystem::path& path)
  {
    return boost::filesystem::symlink_status(path).permissions() & boost::filesystem::perms::all_all;
  }

  constexpr boost::filesystem::perms owner_read_write =
    boost::filesystem::perms::owner_read | boost::filesystem::perms::owner_write;
#endif

#ifdef _WIN32
  struct local_free
  {
    void operator()(void* ptr) const noexcept { LocalFree(ptr); }
  };

  struct everyone_sid
  {
    BYTE sid[SECURITY_MAX_SID_SIZE];
    everyone_sid()
    {
      DWORD size = sizeof(sid);
      if (!CreateWellKnownSid(WinWorldSid, nullptr, sid, &size))
        throw std::runtime_error{"CreateWellKnownSid failed"};
    }
  };

  //! \return A copy of the process user's SID, or an empty vector on error.
  std::vector<char> current_user_sid()
  {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
      return {};
    DWORD size = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &size);
    std::vector<char> buffer(size);
    const bool ok = size && GetTokenInformation(token, TokenUser, buffer.data(), size, &size);
    CloseHandle(token);
    if (!ok)
      return {};

    const PSID sid = reinterpret_cast<PTOKEN_USER>(buffer.data())->User.Sid;
    std::vector<char> out(GetLengthSid(sid));
    if (!CopySid(DWORD(out.size()), out.data(), sid))
      return {};
    return out;
  }

  //! Gives `everyone` read access to `dir`, inherited by files created in it.
  void grant_everyone_inheritable_read(const boost::filesystem::path& dir, const everyone_sid& everyone)
  {
    const std::wstring wide_dir = dir.wstring();

    PACL dir_dacl = nullptr;
    PSECURITY_DESCRIPTOR dir_sd = nullptr;
    ASSERT_EQ(DWORD(ERROR_SUCCESS), GetNamedSecurityInfoW(wide_dir.c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &dir_dacl, nullptr, &dir_sd));
    const std::unique_ptr<void, local_free> dir_sd_owner{dir_sd};

    EXPLICIT_ACCESSW access{};
    access.grfAccessPermissions = FILE_GENERIC_READ;
    access.grfAccessMode = GRANT_ACCESS;
    access.grfInheritance = OBJECT_INHERIT_ACE;
    access.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    access.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    access.Trustee.ptstrName = reinterpret_cast<LPWSTR>(const_cast<BYTE*>(everyone.sid));

    PACL new_dacl = nullptr;
    ASSERT_EQ(DWORD(ERROR_SUCCESS), SetEntriesInAclW(1, &access, dir_dacl, &new_dacl));
    const std::unique_ptr<void, local_free> new_dacl_owner{new_dacl};
    ASSERT_EQ(DWORD(ERROR_SUCCESS), SetNamedSecurityInfoW(const_cast<LPWSTR>(wide_dir.c_str()), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, new_dacl, nullptr));
  }

  //! \return True if `file` has an ACE for `everyone`.
  bool grants_everyone(const boost::filesystem::path& file, const everyone_sid& everyone)
  {
    PACL dacl = nullptr;
    PSECURITY_DESCRIPTOR sd = nullptr;
    if (GetNamedSecurityInfoW(file.wstring().c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &dacl, nullptr, &sd) != ERROR_SUCCESS)
      return false;
    const std::unique_ptr<void, local_free> sd_owner{sd};
    for (DWORD i = 0; dacl && i < dacl->AceCount; ++i)
    {
      void* ace = nullptr;
      if (GetAce(dacl, i, &ace) && static_cast<const ACE_HEADER*>(ace)->AceType == ACCESS_ALLOWED_ACE_TYPE &&
          EqualSid(&static_cast<ACCESS_ALLOWED_ACE*>(ace)->SidStart, const_cast<BYTE*>(everyone.sid)))
        return true;
    }
    return false;
  }

  //! Checks that `file` has a protected DACL with a single ACE, allowing the process user.
  void expect_user_only_dacl(const boost::filesystem::path& file)
  {
    const std::vector<char> user = current_user_sid();
    ASSERT_FALSE(user.empty());

    PACL dacl = nullptr;
    PSECURITY_DESCRIPTOR sd = nullptr;
    ASSERT_EQ(DWORD(ERROR_SUCCESS), GetNamedSecurityInfoW(file.wstring().c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &dacl, nullptr, &sd));
    const std::unique_ptr<void, local_free> sd_owner{sd};

    SECURITY_DESCRIPTOR_CONTROL control{};
    DWORD revision = 0;
    ASSERT_TRUE(GetSecurityDescriptorControl(sd, &control, &revision));
    EXPECT_TRUE(control & SE_DACL_PROTECTED);

    ASSERT_NE(nullptr, dacl);
    ASSERT_EQ(1u, dacl->AceCount);
    void* ace = nullptr;
    ASSERT_TRUE(GetAce(dacl, 0, &ace));
    const ACE_HEADER* header = static_cast<const ACE_HEADER*>(ace);
    EXPECT_FALSE(header->AceFlags & INHERITED_ACE);
    ASSERT_EQ(ACCESS_ALLOWED_ACE_TYPE, header->AceType);
    // the user, not `TokenOwner` (`BUILTIN\Administrators` when elevated)
    EXPECT_TRUE(EqualSid(&static_cast<ACCESS_ALLOWED_ACE*>(ace)->SidStart, const_cast<char*>(user.data())));
    // so that a non-elevated process can reapply the DACL to a file an elevated one created
    EXPECT_EQ(DWORD(WRITE_DAC), static_cast<ACCESS_ALLOWED_ACE*>(ace)->Mask & WRITE_DAC);
  }
#endif

#ifdef __APPLE__
  //! Gives `everyone` read access to `dir`, inherited by files created in it.
  void grant_everyone_inheritable_read(const boost::filesystem::path& dir)
  {
    const acl_t acl = acl_from_text("!#acl 1\ngroup:ABCDEFAB-CDEF-ABCD-EFAB-CDEF0000000C:everyone:12:allow,file_inherit:read\n");
    ASSERT_NE(nullptr, acl);
    const int result = acl_set_file(dir.c_str(), ACL_TYPE_EXTENDED, acl);
    acl_free(acl);
    ASSERT_EQ(0, result);
  }

  bool has_acl_entries(const boost::filesystem::path& file)
  {
    const acl_t acl = acl_get_file(file.c_str(), ACL_TYPE_EXTENDED);
    if (!acl)
      return false;
    acl_entry_t entry{};
    const bool any = acl_get_entry(acl, ACL_FIRST_ENTRY, &entry) == 0;
    acl_free(acl);
    return any;
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
  EXPECT_EQ(1u, entry_count(dir.path));
#ifdef _WIN32
  expect_user_only_dacl(file);
#else
  EXPECT_EQ(owner_read_write, file_perms(file));
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

  ASSERT_FALSE(tools::save_private_file(file.string(), "new"));

  std::string loaded;
  ASSERT_TRUE(epee::file_io_utils::load_file_to_string(file.string(), loaded));
  EXPECT_EQ("new", loaded);
  EXPECT_EQ(1u, entry_count(dir.path));
#ifndef _WIN32
  EXPECT_EQ(owner_read_write, file_perms(file));
#endif
}

TEST(save_private_file, concurrent_saves_publish_one_complete_file)
{
  const temp_dir dir;
  const boost::filesystem::path file = dir.path / "key";

  // large enough that the writes overlap
  std::vector<std::string> payloads;
  for (char c = 'a'; c < 'a' + 8; ++c)
    payloads.emplace_back(std::size_t(256) * 1024, c);

  std::vector<std::error_code> results(payloads.size());
  std::vector<std::thread> threads;
  for (std::size_t i = 0; i < payloads.size(); ++i)
  {
    threads.emplace_back([&, i]
    {
      for (int round = 0; round < 3; ++round)
      {
        const std::error_code ec = tools::save_private_file(file.string(), payloads[i]);
        if (ec)
          results[i] = ec;
      }
    });
  }
  for (auto& thread : threads)
    thread.join();

#ifndef _WIN32
  // Windows can briefly refuse to replace a file another thread is renaming
  for (const auto& ec : results)
    EXPECT_FALSE(ec) << ec.message();
#endif

  std::string loaded;
  ASSERT_TRUE(epee::file_io_utils::load_file_to_string(file.string(), loaded));
  EXPECT_NE(payloads.end(), std::find(payloads.begin(), payloads.end(), loaded));
  EXPECT_EQ(1u, entry_count(dir.path));
}

#ifndef _WIN32
TEST(save_private_file, does_not_follow_symlinks)
{
  const temp_dir dir;
  const boost::filesystem::path target = dir.path / "target";
  const boost::filesystem::path file = dir.path / "key";
  ASSERT_TRUE(epee::file_io_utils::save_string_to_file(target.string(), "untouched"));
  boost::filesystem::create_symlink(target, file);

  ASSERT_FALSE(tools::save_private_file(file.string(), "secret"));

  std::string loaded;
  ASSERT_TRUE(epee::file_io_utils::load_file_to_string(target.string(), loaded));
  EXPECT_EQ("untouched", loaded);
  ASSERT_TRUE(epee::file_io_utils::load_file_to_string(file.string(), loaded));
  EXPECT_EQ("secret", loaded);
  EXPECT_FALSE(boost::filesystem::is_symlink(file));
  EXPECT_EQ(owner_read_write, file_perms(file));
}

TEST(save_private_file, ignores_umask)
{
  const temp_dir dir;
  const boost::filesystem::path file = dir.path / "key";

  const mode_t old_umask = umask(0477); // would leave only owner-write
  const std::error_code ec = tools::save_private_file(file.string(), "secret");
  umask(old_umask);

  ASSERT_FALSE(ec) << ec.message();
  EXPECT_EQ(owner_read_write, file_perms(file));
  std::string loaded;
  ASSERT_TRUE(epee::file_io_utils::load_file_to_string(file.string(), loaded));
  EXPECT_EQ("secret", loaded);
}
#endif

#if defined(_WIN32) || defined(__APPLE__)
TEST(save_private_file, ignores_inheritable_directory_permissions)
{
  const temp_dir dir;
  const boost::filesystem::path control = dir.path / "control";
  const boost::filesystem::path file = dir.path / "key";
#ifdef _WIN32
  const everyone_sid everyone;
  ASSERT_NO_FATAL_FAILURE(grant_everyone_inheritable_read(dir.path, everyone));
  ASSERT_TRUE(epee::file_io_utils::save_string_to_file(control.string(), "control"));
  ASSERT_TRUE(grants_everyone(control, everyone)); // otherwise this test proves nothing

  ASSERT_FALSE(tools::save_private_file(file.string(), "secret"));
  expect_user_only_dacl(file);
#else
  ASSERT_NO_FATAL_FAILURE(grant_everyone_inheritable_read(dir.path));
  ASSERT_TRUE(epee::file_io_utils::save_string_to_file(control.string(), "control"));
  ASSERT_TRUE(has_acl_entries(control)); // otherwise this test proves nothing

  ASSERT_FALSE(tools::save_private_file(file.string(), "secret"));
  EXPECT_FALSE(has_acl_entries(file));
  EXPECT_EQ(owner_read_write, file_perms(file));
#endif
}
#endif

TEST(save_private_file, fails_in_missing_directory)
{
  const temp_dir dir;
  const boost::filesystem::path file = dir.path / "missing" / "key";

  EXPECT_TRUE(tools::save_private_file(file.string(), "secret"));
  EXPECT_FALSE(boost::filesystem::exists(file));
}

TEST(secure_private_file, restricts_existing_file)
{
  const temp_dir dir;
  const boost::filesystem::path file = dir.path / "key";
  ASSERT_TRUE(epee::file_io_utils::save_string_to_file(file.string(), "secret"));
#ifndef _WIN32
  boost::filesystem::permissions(file, owner_read_write | boost::filesystem::perms::group_read | boost::filesystem::perms::others_read);
#endif

  ASSERT_FALSE(tools::secure_private_file(file.string()));

  std::string loaded;
  ASSERT_TRUE(epee::file_io_utils::load_file_to_string(file.string(), loaded));
  EXPECT_EQ("secret", loaded);
#ifdef _WIN32
  expect_user_only_dacl(file);
#else
  EXPECT_EQ(owner_read_write, file_perms(file));
#endif
}

#ifndef _WIN32
TEST(secure_private_file, leaves_private_file_unchanged)
{
  // Even a no-op change fails on a read-only filesystem, or on ZFS with `aclmode=restricted`.
  // Any mode or ACL change updates the ctime.
  const temp_dir dir;
  const boost::filesystem::path file = dir.path / "key";
  ASSERT_FALSE(tools::save_private_file(file.string(), "secret"));

  const auto ctime = [&file]
  {
    struct stat st{};
    EXPECT_EQ(0, stat(file.c_str(), &st));
#ifdef __APPLE__
    return std::make_pair(st.st_ctimespec.tv_sec, st.st_ctimespec.tv_nsec);
#else
    return std::make_pair(st.st_ctim.tv_sec, st.st_ctim.tv_nsec);
#endif
  };
  const auto before = ctime();
  std::this_thread::sleep_for(std::chrono::milliseconds{50}); // longer than the ctime resolution

  ASSERT_FALSE(tools::secure_private_file(file.string()));
  EXPECT_EQ(before, ctime());
  EXPECT_EQ(owner_read_write, file_perms(file));
}
#endif

#if defined(_WIN32) || defined(__APPLE__)
TEST(secure_private_file, removes_inherited_permissions)
{
  const temp_dir dir;
  const boost::filesystem::path file = dir.path / "key";
#ifdef _WIN32
  const everyone_sid everyone;
  ASSERT_NO_FATAL_FAILURE(grant_everyone_inheritable_read(dir.path, everyone));
  ASSERT_TRUE(epee::file_io_utils::save_string_to_file(file.string(), "secret"));
  ASSERT_TRUE(grants_everyone(file, everyone));

  ASSERT_FALSE(tools::secure_private_file(file.string()));
  expect_user_only_dacl(file);
#else
  ASSERT_NO_FATAL_FAILURE(grant_everyone_inheritable_read(dir.path));
  ASSERT_TRUE(epee::file_io_utils::save_string_to_file(file.string(), "secret"));
  ASSERT_TRUE(has_acl_entries(file));

  ASSERT_FALSE(tools::secure_private_file(file.string()));
  EXPECT_FALSE(has_acl_entries(file));
  EXPECT_EQ(owner_read_write, file_perms(file));
#endif
}
#endif

TEST(secure_private_file, rejects_missing_file_and_directory)
{
  const temp_dir dir;
  EXPECT_TRUE(tools::secure_private_file((dir.path / "missing").string()));
  EXPECT_TRUE(tools::secure_private_file(dir.path.string()));
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
