// Copyright (c) 2021, The Monero Project
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

#include "common/update_hash_checker.h"

#include <chrono>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <boost/utility/string_ref.hpp>

#include "gtest/gtest.h"

#include "net/abstract_http_client.h"
#include "string_coding.h"

namespace
{

class http_client : public epee::net_utils::http::abstract_http_client
{
public:
  http_client(std::unordered_map<std::string, std::string> files = {})
    : m_base_url(
        std::string("/api/v4/projects/") +
        epee::net_utils::convert_to_url_format_force_all("monero-project/gitian.sigs"))
    , m_files(std::move(files))
  {
  }

  virtual void set_server(
    std::string host,
    std::string port,
    boost::optional<epee::net_utils::http::login> user,
    epee::net_utils::ssl_options_t ssl_options) override
  {
  }

  virtual void set_auto_connect(bool auto_connect) override {}

  virtual bool connect(std::chrono::milliseconds timeout) override
  {
    return true;
  }

  virtual bool disconnect() override
  {
    return true;
  }

  virtual bool is_connected(bool *ssl = NULL) override
  {
    return true;
  }

  virtual bool invoke(
    const boost::string_ref uri,
    const boost::string_ref method,
    const boost::string_ref body,
    std::chrono::milliseconds timeout,
    const epee::net_utils::http::http_response_info **ppresponse_info = nullptr,
    const epee::net_utils::http::fields_list &additional_params = {}) override
  {
    return false;
  }

  virtual bool handle_repository_files(boost::string_ref path, epee::net_utils::http::http_response_info &response)
  {
    std::stringstream result;
    for (const auto &file : m_files)
    {
      if (file.first == path)
      {
        result << "{\"content\": \""
               << epee::string_encoding::base64_encode(
                    reinterpret_cast<const uint8_t *>(&file.second[0]),
                    file.second.size())
               << "\"}";
        break;
      }
    }
    response.m_body = result.str();
    return true;
  }

  virtual bool handle_repository_tree(boost::string_ref path, epee::net_utils::http::http_response_info &response)
  {
    std::unordered_map<std::string, std::string> entries;
    for (const auto &file : m_files)
    {
      boost::string_ref name(file.first);
      if (name.starts_with(std::string(path) + "/"))
      {
        name.remove_prefix(path.size() + 1);
        const auto pos = name.find('/');
        if (pos != boost::string_ref::npos)
        {
          entries.emplace(name.substr(0, pos), "tree");
        }
        else
        {
          entries.emplace(name, "blob");
        }
      }
    }

    std::stringstream result;
    result << "[";
    bool first = true;
    for (const auto &name : entries)
    {
      result << (first ? "" : ",") << "{\"type\": \"" << name.second << "\", \"name\": \"" << name.first << "\"}";
      first = false;
    }
    result << "]";

    response.m_header_info.m_etc_fields.push_back({"x-total-pages", "1"});
    response.m_body = result.str();
    return true;
  }

  virtual bool invoke_get(
    boost::string_ref uri,
    std::chrono::milliseconds timeout,
    const std::string &body = std::string(),
    const epee::net_utils::http::http_response_info **ppresponse_info = nullptr,
    const epee::net_utils::http::fields_list &additional_params = {}) override
  {
    static epee::net_utils::http::http_response_info response;
    response.clear();
    *ppresponse_info = &response;

    if (!uri.starts_with(m_base_url))
    {
      return false;
    }
    uri.remove_prefix(m_base_url.size());

    static const boost::string_ref repository_files("/repository/files/");
    if (uri.starts_with(repository_files))
    {
      uri.remove_prefix(repository_files.size());
      uri = uri.substr(0, uri.find('?'));
      return handle_repository_files(epee::net_utils::convert_from_url_format(std::string(uri)), response);
    }
    else if (uri.starts_with("/repository/tree?path="))
    {
      uri = uri.substr(0, uri.find('&'));
      uri.remove_prefix(uri.find('=') + 1);
      return handle_repository_tree(epee::net_utils::convert_from_url_format(std::string(uri)), response);
    }
    return false;
  }

  virtual bool invoke_post(
    const boost::string_ref uri,
    const std::string &body,
    std::chrono::milliseconds timeout,
    const epee::net_utils::http::http_response_info **ppresponse_info = nullptr,
    const epee::net_utils::http::fields_list &additional_params = {}) override
  {
    return false;
  }

  virtual uint64_t get_bytes_sent() const override
  {
    return 0;
  }

  virtual uint64_t get_bytes_received() const override
  {
    return 0;
  }

private:
  const std::unordered_map<std::string, std::string> m_files;
  const std::string m_base_url;
};

class http_client_mock : public http_client
{
public:
  http_client_mock(std::string body, bool result)
    : m_body(std::move(body))
    , m_result(result)
  {
  }

  bool handle_repository_tree(boost::string_ref path, epee::net_utils::http::http_response_info &response) override
  {
    response.m_body = m_body;
    return m_result;
  }

  bool handle_repository_files(boost::string_ref path, epee::net_utils::http::http_response_info &response) override
  {
    response.m_body = m_body;
    return m_result;
  }

private:
  const std::string m_body;
  const bool m_result;
};

std::unique_ptr<tools::repo_explorer> repo_explorer_2_signers()
{
  std::unordered_map<std::string, std::string> files;
  files["v0.17.2.0-win/README.md"] = {};

  files["v0.17.2.0-win/signer1/monero-win-0.17-build.assert"] = "--- !!omap\n\
- out_manifest: |\n\
    c772070ebdfe9e0d6abda5073808e648e69f8c35f8010e66b80f45a6bdb01792  monero-i686-w64-mingw32-v0.17.2.0.zip\n\
    71e531a0f799c80e3f6319888bd5b85a737091b9bd5d66366cae036163857caf  monero-x86_64-w64-mingw32-v0.17.2.0.zip";
  files["v0.17.2.0-win/signer1/monero-win-0.17-build.assert.sig"] = {};
  files["v0.17.2.0-linux/signer1/monero-linux-0.17-build.assert"] = "--- !!omap\n\
- out_manifest: |\n\
    a004fad5348549e2f1610380775353f19db7cbca0cbe4acbfab87832c484da62  monero-aarch64-linux-gnu-v0.17.2.0.tar.bz2\n\
    b5986d6dfbddee14e32b28305dd0dc6352c18b632f569227f2c7265ef7dc5081  monero-arm-linux-gnueabihf-v0.17.2.0.tar.bz2\n\
    e8a39be486549908c10524d851a006c21c30b1a49142586aff0a17e7c4d46077  monero-i686-linux-gnu-v0.17.2.0.tar.bz2\n\
    59e16c53b2aff8d9ab7a8ba3279ee826ac1f2480fbb98e79a149e6be23dd9086  monero-x86_64-linux-gnu-v0.17.2.0.tar.bz2";
  files["v0.17.2.0-linux/signer1/monero-linux-0.17-build.assert.sig"] = {};
  files["v0.17.2.0-osx/signer1/monero-osx-0.17-build.assert"] = "--- !!omap\n\
- out_manifest: '2e95dc107ab0dab36f5544bec040180264256e45407c383cfb45cfe328fe42e0  monero-x86_64-apple-darwin11-v0.17.2.0.tar.bz2\n\
\n\
'";
  files["v0.17.2.0-osx/signer1/monero-osx-0.17-build.assert.sig"] = {};
  files["v0.17.2.0-freebsd/signer1/monero-freebsd-0.17-build.assert"] = "--- !!omap\n\
- out_manifest: '34ef5702a050298f48ccea7db992137bc98c8e6eba45ecd90b47ce0a4b7bf0f8  monero-x86_64-unknown-freebsd-v0.17.2.0.tar.bz2\n\
\n\
'";
  files["v0.17.2.0-freebsd/signer1/monero-freebsd-0.17-build.assert.sig"] = {};
  files["v0.17.2.0-android/signer1/monero-android-0.17-build.assert"] = "--- !!omap\n\
- out_manifest: |\n\
    b8a353f02feaee9aae3d279c043ea33a32413a298d8b6122d00a65508f15169d  monero-aarch64-linux-android-v0.17.2.0.tar.bz2\n\
    815341f7d46f75a8905f8b51932e1034a7f6b1669757ff48224632d08339d1bf  monero-arm-linux-android-v0.17.2.0.tar.bz2";
  files["v0.17.2.0-android/signer1/monero-android-0.17-build.assert.sig"] = {};

  files["v0.17.2.0-win/signer2/monero-win-0.17-build.assert"] = "--- !!omap\n\
- out_manifest: |\n\
    c772070ebdfe9e0d6abda5073808e648e69f8c35f8010e66b80f45a6bdb01792  monero-i686-w64-mingw32-v0.17.2.0.zip\n\
    71e531a0f799c80e3f6319888bd5b85a737091b9bd5d66366cae036163857caf  monero-x86_64-w64-mingw32-v0.17.2.0.zip";
  files["v0.17.2.0-win/signer2/monero-win-0.17-build.assert.sig"] = {};
  files["v0.17.2.0-linux/signer2/monero-linux-0.17-build.assert"] = "--- !!omap\n\
- out_manifest: |\n\
    a004fad5348549e2f1610380775353f19db7cbca0cbe4acbfab87832c484da62  monero-aarch64-linux-gnu-v0.17.2.0.tar.bz2\n\
    b5986d6dfbddee14e32b28305dd0dc6352c18b632f569227f2c7265ef7dc5081  monero-arm-linux-gnueabihf-v0.17.2.0.tar.bz2\n\
    e8a39be486549908c10524d851a006c21c30b1a49142586aff0a17e7c4d46077  monero-i686-linux-gnu-v0.17.2.0.tar.bz2\n\
    59e16c53b2aff8d9ab7a8ba3279ee826ac1f2480fbb98e79a149e6be23dd9086  monero-x86_64-linux-gnu-v0.17.2.0.tar.bz2";
  files["v0.17.2.0-linux/signer2/monero-linux-0.17-build.assert.sig"] = {};
  files["v0.17.2.0-osx/signer2/monero-osx-0.17-build.assert"] = "--- !!omap\n\
- out_manifest: '2e95dc107ab0dab36f5544bec040180264256e45407c383cfb45cfe328fe42e0  monero-x86_64-apple-darwin11-v0.17.2.0.tar.bz2\n\
\n\
'";
  files["v0.17.2.0-osx/signer2/monero-osx-0.17-build.assert.sig"] = {};
  files["v0.17.2.0-freebsd/signer2/monero-freebsd-0.17-build.assert"] = "--- !!omap\n\
- out_manifest: '34ef5702a050298f48ccea7db992137bc98c8e6eba45ecd90b47ce0a4b7bf0f8  monero-x86_64-unknown-freebsd-v0.17.2.0.tar.bz2\n\
\n\
'";
  files["v0.17.2.0-freebsd/signer2/monero-freebsd-0.17-build.assert.sig"] = {};
  files["v0.17.2.0-android/signer2/monero-android-0.17-build.assert"] = "--- !!omap\n\
- out_manifest: |\n\
    b8a353f02feaee9aae3d279c043ea33a32413a298d8b6122d00a65508f15169d  monero-aarch64-linux-android-v0.17.2.0.tar.bz2\n\
    815341f7d46f75a8905f8b51932e1034a7f6b1669757ff48224632d08339d1bf  monero-arm-linux-android-v0.17.2.0.tar.bz2";
  files["v0.17.2.0-android/signer2/monero-android-0.17-build.assert.sig"] = {};

  return std::unique_ptr<tools::repo_explorer>(new tools::gitlab_repo(
    "repo.getmonero.org",
    443,
    "monero-project",
    "gitian.sigs",
    std::unique_ptr<epee::net_utils::http::abstract_http_client>(new http_client(std::move(files))),
    std::chrono::seconds(30),
    "monero-project"));
}

std::unique_ptr<tools::repo_explorer> repo_explorer_invalid_platform()
{
  std::unordered_map<std::string, std::string> files;
  files["v0.17.2.0-win/signer1/monero-win-0.17-build.assert"] = "--- !!omap\n\
- out_manifest: |\n\
    c772070ebdfe9e0d6abda5073808e648e69f8c35f8010e66b80f45a6bdb01792  monero-i686-linux-gnu-v0.17.2.0.tar.bz2\n\
    71e531a0f799c80e3f6319888bd5b85a737091b9bd5d66366cae036163857caf  monero-x86_64-linux-gnu-v0.17.2.0.tar.bz2";
  files["v0.17.2.0-win/signer1/monero-win-0.17-build.assert.sig"] = {};
  return std::unique_ptr<tools::repo_explorer>(new tools::gitlab_repo(
    "repo.getmonero.org",
    443,
    "monero-project",
    "gitian.sigs",
    std::unique_ptr<epee::net_utils::http::abstract_http_client>(new http_client(std::move(files))),
    std::chrono::seconds(30),
    "monero-project"));
}

std::unique_ptr<tools::repo_explorer> repo_explorer_invalid_hash()
{
  std::unordered_map<std::string, std::string> files;
  files["v0.17.2.0-win/signer1/monero-win-0.17-build.assert"] = "--- !!omap\n\
- out_manifest: |\n\
    c  monero-x86_64-w64-mingw32-v0.17.2.0.zip\n\
monero-x86_64-w64-mingw32-v0.17.2.0.zip";
  files["v0.17.2.0-win/signer1/monero-win-0.17-build.assert.sig"] = {};
  return std::unique_ptr<tools::repo_explorer>(new tools::gitlab_repo(
    "repo.getmonero.org",
    443,
    "monero-project",
    "gitian.sigs",
    std::unique_ptr<epee::net_utils::http::abstract_http_client>(new http_client(std::move(files))),
    std::chrono::seconds(30),
    "monero-project"));
}

} // namespace

TEST(UpdateHashChecker, BuildTag)
{
  const tools::gitian_hash_checker hash_checker(
    repo_explorer_2_signers(),
    std::unique_ptr<tools::signature_verifier>(new tools::dummy_signature_verifier()),
    "monero",
    1);
  EXPECT_NO_THROW(hash_checker.check(
    "815341f7d46f75a8905f8b51932e1034a7f6b1669757ff48224632d08339d1bf",
    "0.17.2.0",
    "android-armv7"));
  EXPECT_NO_THROW(hash_checker.check(
    "b8a353f02feaee9aae3d279c043ea33a32413a298d8b6122d00a65508f15169d",
    "0.17.2.0",
    "android-armv8"));
  EXPECT_NO_THROW(
    hash_checker.check("34ef5702a050298f48ccea7db992137bc98c8e6eba45ecd90b47ce0a4b7bf0f8", "0.17.2.0", "freebsd-x64"));
  EXPECT_NO_THROW(
    hash_checker.check("b5986d6dfbddee14e32b28305dd0dc6352c18b632f569227f2c7265ef7dc5081", "0.17.2.0", "linux-armv7"));
  EXPECT_NO_THROW(
    hash_checker.check("a004fad5348549e2f1610380775353f19db7cbca0cbe4acbfab87832c484da62", "0.17.2.0", "linux-armv8"));
  EXPECT_NO_THROW(
    hash_checker.check("59e16c53b2aff8d9ab7a8ba3279ee826ac1f2480fbb98e79a149e6be23dd9086", "0.17.2.0", "linux-x64"));
  EXPECT_NO_THROW(
    hash_checker.check("e8a39be486549908c10524d851a006c21c30b1a49142586aff0a17e7c4d46077", "0.17.2.0", "linux-x86"));
  EXPECT_NO_THROW(
    hash_checker.check("2e95dc107ab0dab36f5544bec040180264256e45407c383cfb45cfe328fe42e0", "0.17.2.0", "mac-x64"));
  EXPECT_NO_THROW(
    hash_checker.check("71e531a0f799c80e3f6319888bd5b85a737091b9bd5d66366cae036163857caf", "0.17.2.0", "win-x64"));
  EXPECT_NO_THROW(
    hash_checker.check("c772070ebdfe9e0d6abda5073808e648e69f8c35f8010e66b80f45a6bdb01792", "0.17.2.0", "win-x32"));
  EXPECT_ANY_THROW(
    hash_checker.check("c772070ebdfe9e0d6abda5073808e648e69f8c35f8010e66b80f45a6bdb01792", "0.17.2.0", "invalid"));
}

TEST(UpdateHashChecker, Hash)
{
  const tools::gitian_hash_checker hash_checker(
    repo_explorer_2_signers(),
    std::unique_ptr<tools::signature_verifier>(new tools::dummy_signature_verifier()),
    "monero",
    1);
  EXPECT_NO_THROW(
    hash_checker.check("71e531a0f799c80e3f6319888bd5b85a737091b9bd5d66366cae036163857caf", "0.17.2.0", "win-x64"));
  EXPECT_ANY_THROW(
    hash_checker.check("00e531a0f799c80e3f6319888bd5b85a737091b9bd5d66366cae036163857caf", "0.17.2.0", "win-x64"));
  EXPECT_ANY_THROW(hash_checker.check("7", "0.17.2.0", "win-x64"));
}

TEST(UpdateHashChecker, InvalidHash)
{
  const tools::gitian_hash_checker hash_checker(
    repo_explorer_invalid_hash(),
    std::unique_ptr<tools::signature_verifier>(new tools::dummy_signature_verifier()),
    "monero",
    1);
  EXPECT_ANY_THROW(
    hash_checker.check("71e531a0f799c80e3f6319888bd5b85a737091b9bd5d66366cae036163857caf", "0.17.2.0", "win-x64"));
}

TEST(UpdateHashChecker, Participants)
{
  {
    const size_t minimum_participants = 1;
    const tools::gitian_hash_checker hash_checker(
      repo_explorer_2_signers(),
      std::unique_ptr<tools::signature_verifier>(new tools::dummy_signature_verifier()),
      "monero",
      minimum_participants,
      true);
    EXPECT_NO_THROW(
      hash_checker.check("71e531a0f799c80e3f6319888bd5b85a737091b9bd5d66366cae036163857caf", "0.17.2.0", "win-x64"));
  }
  {
    const size_t minimum_participants = 1;
    const tools::gitian_hash_checker hash_checker(
      repo_explorer_2_signers(),
      std::unique_ptr<tools::signature_verifier>(new tools::dummy_signature_verifier()),
      "monero",
      minimum_participants,
      false);
    EXPECT_NO_THROW(
      hash_checker.check("71e531a0f799c80e3f6319888bd5b85a737091b9bd5d66366cae036163857caf", "0.17.2.0", "win-x64"));
  }
  {
    const size_t minimum_participants = 2;
    const tools::gitian_hash_checker hash_checker(
      repo_explorer_2_signers(),
      std::unique_ptr<tools::signature_verifier>(new tools::dummy_signature_verifier()),
      "monero",
      minimum_participants);
    EXPECT_NO_THROW(
      hash_checker.check("71e531a0f799c80e3f6319888bd5b85a737091b9bd5d66366cae036163857caf", "0.17.2.0", "win-x64"));
  }
  {
    const size_t minimum_participants = 3;
    const tools::gitian_hash_checker hash_checker(
      repo_explorer_2_signers(),
      std::unique_ptr<tools::signature_verifier>(new tools::dummy_signature_verifier()),
      "monero",
      minimum_participants);
    EXPECT_ANY_THROW(
      hash_checker.check("71e531a0f799c80e3f6319888bd5b85a737091b9bd5d66366cae036163857caf", "0.17.2.0", "win-x64"));
  }
}

TEST(UpdateHashChecker, Platform)
{
  const tools::gitian_hash_checker hash_checker(
    repo_explorer_invalid_platform(),
    std::unique_ptr<tools::signature_verifier>(new tools::dummy_signature_verifier()),
    "monero",
    1);
  EXPECT_ANY_THROW(
    hash_checker.check("71e531a0f799c80e3f6319888bd5b85a737091b9bd5d66366cae036163857caf", "0.17.2.0", "win-x64"));
}

TEST(UpdateHashChecker, Project)
{
  {
    const tools::gitian_hash_checker hash_checker(
      repo_explorer_2_signers(),
      std::unique_ptr<tools::signature_verifier>(new tools::dummy_signature_verifier()),
      "monero",
      1);
    EXPECT_NO_THROW(
      hash_checker.check("71e531a0f799c80e3f6319888bd5b85a737091b9bd5d66366cae036163857caf", "0.17.2.0", "win-x64"));
  }
  {
    const tools::gitian_hash_checker hash_checker(
      repo_explorer_2_signers(),
      std::unique_ptr<tools::signature_verifier>(new tools::dummy_signature_verifier()),
      "nonexistent-project",
      1);
    EXPECT_ANY_THROW(
      hash_checker.check("71e531a0f799c80e3f6319888bd5b85a737091b9bd5d66366cae036163857caf", "0.17.2.0", "win-x64"));
  }
}

TEST(UpdateHashChecker, Signature)
{
  {
    const bool valid_signature = true;
    const tools::gitian_hash_checker hash_checker(
      repo_explorer_2_signers(),
      std::unique_ptr<tools::signature_verifier>(new tools::dummy_signature_verifier(valid_signature)),
      "monero",
      1);
    EXPECT_NO_THROW(
      hash_checker.check("71e531a0f799c80e3f6319888bd5b85a737091b9bd5d66366cae036163857caf", "0.17.2.0", "win-x64"));
  }
  {
    const bool valid_signature = false;
    const tools::gitian_hash_checker hash_checker(
      repo_explorer_2_signers(),
      std::unique_ptr<tools::signature_verifier>(new tools::dummy_signature_verifier(valid_signature)),
      "monero",
      1);
    EXPECT_ANY_THROW(
      hash_checker.check("71e531a0f799c80e3f6319888bd5b85a737091b9bd5d66366cae036163857caf", "0.17.2.0", "win-x64"));
  }
}

TEST(UpdateHashChecker, Version)
{
  const tools::gitian_hash_checker hash_checker(
    repo_explorer_2_signers(),
    std::unique_ptr<tools::signature_verifier>(new tools::dummy_signature_verifier()),
    "monero",
    1);
  EXPECT_NO_THROW(
    hash_checker.check("71e531a0f799c80e3f6319888bd5b85a737091b9bd5d66366cae036163857caf", "0.17.2.0", "win-x64"));
  EXPECT_ANY_THROW(
    hash_checker.check("71e531a0f799c80e3f6319888bd5b85a737091b9bd5d66366cae036163857caf", "0.17", "win-x64"));
  EXPECT_ANY_THROW(
    hash_checker.check("71e531a0f799c80e3f6319888bd5b85a737091b9bd5d66366cae036163857caf", "0", "win-x64"));
  EXPECT_ANY_THROW(
    hash_checker.check("71e531a0f799c80e3f6319888bd5b85a737091b9bd5d66366cae036163857caf", "1.17.2.0", "win-x64"));
}

TEST(UpdateHashChecker, GitlabApi)
{
  const auto gitlab_repo = [](std::string body, bool result = true) {
    return std::unique_ptr<tools::repo_explorer>(new tools::gitlab_repo(
      "repo.getmonero.org",
      443,
      "monero-project",
      "gitian.sigs",
      std::unique_ptr<epee::net_utils::http::abstract_http_client>(new http_client_mock(std::move(body), result)),
      std::chrono::seconds(30),
      "monero-project"));
  };

  EXPECT_NO_THROW(gitlab_repo("[{\"name\": \"somename\", \"type\": \"tree\"}]")->list_directories("/"));
  EXPECT_ANY_THROW(gitlab_repo("[{\"name\": \"somename\", \"type\": \"tree\"}]", false)->list_directories("/"));
  EXPECT_ANY_THROW(gitlab_repo("{invalid json")->list_directories("some_directory"));

  EXPECT_NO_THROW(gitlab_repo("{\"content\": \"\"}")->get_file_contents("/SomeFile"));
  EXPECT_ANY_THROW(gitlab_repo("{\"content\": \"\"}", false)->get_file_contents("/SomeFile"));
  EXPECT_ANY_THROW(gitlab_repo("{invalid json")->get_file_contents("/SomeFile"));
}
