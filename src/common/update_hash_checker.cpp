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

#include "update_hash_checker.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include <boost/algorithm/string.hpp>

#include "hex.h"
#include "misc_log_ex.h"
#include "serialization/keyvalue_serialization.h"
#include "storages/portable_storage_template_helper.h"
#include "string_coding.h"
#include "string_tools.h"
#include "string_tools_lexical.h"

#include "crypto/crypto.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "update_hash_checker"

namespace tools
{

namespace
{

class gitian_assert
{
public:
  gitian_assert(const std::string &assert)
  {
    boost::split(m_contents, assert, boost::is_any_of("\n"));
  }

  bool find_hash(const crypto::hash &hash, const std::string &version, const std::string &platform) const
  {
    for (const auto &line : m_contents)
    {
      if (line.find(version) == std::string::npos)
      {
        continue;
      }
      if (line.find(platform) == std::string::npos)
      {
        continue;
      }

      bool found_whitespace = false;
      const auto hash_end = std::find_if(line.rbegin(), line.rend(), [&found_whitespace](const char &ch) {
        if (ch == ' ' || ch == '\t')
        {
          found_whitespace = true;
          return false;
        }
        return found_whitespace;
      });
      const auto hash_start = std::find_if(hash_end, line.rend(), [](const char &ch) {
        if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f'))
        {
          return false;
        }
        return true;
      });

      crypto::hash current_hash;
      const size_t hash_len = std::distance(hash_start.base(), hash_end.base());
      if (!epee::from_hex::to_buffer(epee::as_mut_byte_span(current_hash), {&*hash_start.base(), hash_len}))
      {
        continue;
      }

      if (current_hash == hash)
      {
        return true;
      }
    }

    return false;
  }

private:
  std::vector<std::string> m_contents;
};

class gitian_path
{
public:
  gitian_path(const std::string &project, const std::string &version, const std::string &build_tag)
  {
    std::vector<std::string> semver;
    boost::split(semver, version, boost::is_any_of(".-"));

    if (semver.size() < 2)
    {
      throw std::runtime_error("failed to detect major/minor version");
    }

    const std::pair<std::string, std::string> &platform_os = build_tag_to_platform_tuple(build_tag);
    m_build_assert_filename = project + "-" + platform_os.first + "-" + semver[0] + "." + semver[1] + "-build.assert";
    m_platform = platform_os.second;
    m_signatures_folder = "v" + version + "-" + platform_os.first;
  }

  std::string build_assert_path(const std::string &signer) const
  {
    return m_signatures_folder + "/" + signer + "/" + m_build_assert_filename;
  }

  std::string build_assert_signature_path(const std::string &signer) const
  {
    return build_assert_path(signer) + ".sig";
  }

  std::string platform() const
  {
    return m_platform;
  }

  std::string signatures_folder() const
  {
    return m_signatures_folder;
  }

private:
  static const std::pair<std::string, std::string> &build_tag_to_platform_tuple(const std::string &build_tag)
  {
    static const std::unordered_map<std::string, std::pair<std::string, std::string>> mapping = {
      {"android", {"android", "android"}},
      {"freebsd", {"freebsd", "freebsd"}},
      {"linux", {"linux", "linux"}},
      {"mac", {"osx", "apple"}},
      {"win", {"win", "w64"}}};

    std::vector<std::string> build_tag_tuple;
    boost::split(build_tag_tuple, build_tag, boost::is_any_of("-"));

    const auto item = mapping.find(build_tag_tuple[0]);
    if (item == mapping.end())
    {
      throw std::runtime_error(std::string("Unsupported build tag: ") + build_tag);
    }

    return item->second;
  }

private:
  std::string m_build_assert_filename;
  std::string m_platform;
  std::string m_signatures_folder;
};

struct file_contents_reply
{
  std::string content;

  BEGIN_KV_SERIALIZE_MAP()
  KV_SERIALIZE(content)
  END_KV_SERIALIZE_MAP()
};

struct repository_tree_reply
{
  struct entry
  {
    std::string name;
    std::string type;

    BEGIN_KV_SERIALIZE_MAP()
    KV_SERIALIZE(name)
    KV_SERIALIZE(type)
    END_KV_SERIALIZE_MAP()
  };

  std::vector<entry> entries;

  BEGIN_KV_SERIALIZE_MAP()
  KV_SERIALIZE(entries)
  END_KV_SERIALIZE_MAP()
};

} // namespace

gitlab_repo::gitlab_repo(
  const std::string &host,
  uint16_t port,
  const std::string &owner,
  const std::string &repo,
  std::unique_ptr<epee::net_utils::http::abstract_http_client> http_client,
  std::chrono::seconds timeout,
  std::string user_agent)
  : m_base_url(std::string("/api/v4/projects/") + epee::net_utils::convert_to_url_format_force_all(owner + "/" + repo))
  , m_http(std::move(http_client))
  , m_timeout(timeout)
  , m_user_agent(std::move(user_agent))
{
  m_http->set_server(host, std::to_string(port), {});
}

std::string gitlab_repo::get_file_contents(const std::string &path) const
{
  const epee::net_utils::http::http_response_info *info;
  const bool result = m_http->invoke_get(
    m_base_url + "/repository/files/" + epee::net_utils::convert_to_url_format_force_all(path) + "?ref=master",
    m_timeout,
    {},
    &info);
  if (!result)
  {
    throw std::runtime_error("Gitlab repository files request failed");
  }

  file_contents_reply reply;
  if (!epee::serialization::load_t_from_json(reply, info->m_body))
  {
    throw std::runtime_error("failed to deserialize Gitlab repository files reply");
  }

  reply.content.erase(std::remove(reply.content.begin(), reply.content.end(), '\n'), reply.content.end());
  return epee::string_encoding::base64_decode(reply.content);
}

std::unordered_set<std::string> gitlab_repo::list_directories(const std::string &path) const
{
  std::unordered_set<std::string> directories;

  constexpr size_t per_page = 100;
  const std::string url = m_base_url +
    "/repository/tree?path=" + epee::net_utils::convert_to_url_format_force_all(path) +
    "&per_page=" + std::to_string(per_page);

  size_t page = 0;
  size_t total_pages = 1;
  do
  {
    const epee::net_utils::http::http_response_info *info;
    const bool result = m_http->invoke_get(url + "&page=" + std::to_string(page), m_timeout, {}, &info);
    if (!result)
    {
      throw std::runtime_error("Gitlab repository tree request failed");
    }

    // Gitlab /repository/tree request returns JSON array
    // Used a hack to utilize BEGIN_KV_SERIALIZE_MAP:
    // Had to manually "convert" the reply into struct: '{"entries": <JSON array>}'
    repository_tree_reply reply;
    if (!epee::serialization::load_t_from_json(reply, std::string("{\"entries\": ") + info->m_body + "}"))
    {
      throw std::runtime_error("failed to deserialize Gitlab repository tree reply");
    }

    for (const auto &entry : reply.entries)
    {
      if (entry.type == "tree")
      {
        directories.emplace(entry.name);
      }
    }

    const bool first_time = page == 0;
    if (first_time)
    {
      for (const auto &field : info->m_header_info.m_etc_fields)
      {
        static const std::string x_total_pages("x-total-pages");
        if (!epee::string_tools::compare_no_case(field.first, x_total_pages))
        {
          size_t value;
          if (epee::string_tools::get_xtype_from_string(value, field.second))
          {
            total_pages = std::max(total_pages, value);
          }
        }
      }
    }
  } while (++page < total_pages);

  return directories;
}

gitian_hash_checker::gitian_hash_checker(
  std::unique_ptr<repo_explorer> repo_explorer,
  std::unique_ptr<signature_verifier> verifier,
  std::string project,
  size_t minimum_participants,
  bool check_all_available)
  : m_check_all_available(check_all_available)
  , m_explorer(std::move(repo_explorer))
  , m_minimum_participants(minimum_participants)
  , m_project(std::move(project))
  , m_verifier(std::move(verifier))
{
}

void gitian_hash_checker::check(const crypto::hash &hash, const std::string &version, const std::string &build_tag)
  const
{
  gitian_path path(m_project, version, build_tag);

  std::unordered_set<std::string> signers = m_explorer->list_directories(path.signatures_folder());
  MDEBUG("Found " << signers.size() << " Gitian build signatures (v" << version << " " << build_tag << ")");
  if (signers.size() < m_minimum_participants)
  {
    throw std::runtime_error("failed to fetch minimum required number of Gitian signatures");
  }

  if (!m_check_all_available)
  {
    while (signers.size() > m_minimum_participants)
    {
      auto item_to_remove = signers.begin();
      std::advance(item_to_remove, crypto::rand_idx(signers.size()));
      MDEBUG(*item_to_remove << ": skipping");
      signers.erase(item_to_remove);
    }
  }

  for (const std::string &signer : signers)
  {
    std::string assert = m_explorer->get_file_contents(path.build_assert_path(signer));
    const bool found = gitian_assert(assert).find_hash(hash, version, path.platform());
    if (!found)
    {
      std::stringstream ss;
      ss << "Gitian expected hash not found. Signer: " << signer << ", version: " << version
         << ", platform: " << path.platform() << ", expected hash: " << hash;
      throw std::runtime_error(ss.str());
    }

    const std::string signature = m_explorer->get_file_contents(path.build_assert_signature_path(signer));
    const bool valid = m_verifier->verify(signer, epee::strspan<uint8_t>(assert), epee::strspan<uint8_t>(signature));
    if (!valid)
    {
      std::stringstream ss;
      ss << "Gitian signature verification failed. Signer: " << signer << ", version: " << version
         << ", platform: " << path.platform() << ", expected hash: " << hash;
      throw std::runtime_error(ss.str());
    }

    MDEBUG(signer << ": build hash matches expected hash");
  }
}

void gitian_hash_checker::check(const std::string &hash, const std::string &version, const std::string &build_tag) const
{
  crypto::hash hash_buffer;
  if (!epee::from_hex::to_buffer(epee::as_mut_byte_span(hash_buffer), hash))
  {
    throw std::runtime_error(std::string("failed to deserialize hash string '") + hash + "'");
  }
  return check(hash_buffer, version, build_tag);
}

} // namespace tools
