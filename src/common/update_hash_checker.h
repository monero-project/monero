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

#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "net/abstract_http_client.h"
#include "span.h"

#include "crypto/hash.h"

namespace tools
{

class repo_explorer
{
public:
  virtual ~repo_explorer() = default;

  virtual std::string get_file_contents(const std::string &path) const = 0;
  virtual std::unordered_set<std::string> list_directories(const std::string &path) const = 0;
};

class signature_verifier
{
public:
  virtual ~signature_verifier() = default;

  virtual bool verify(
    const std::string &signed_by,
    epee::span<const uint8_t> message,
    epee::span<const uint8_t> signature) const = 0;
};

class dummy_signature_verifier : public signature_verifier
{
public:
  dummy_signature_verifier(bool result = true)
    : m_result(result)
  {
  }

  virtual bool verify(
    const std::string &signed_by,
    epee::span<const uint8_t> message,
    epee::span<const uint8_t> signature) const override
  {
    return m_result;
  }

private:
  bool m_result;
};

class gitian_hash_checker
{
public:
  gitian_hash_checker(
    std::unique_ptr<repo_explorer> repo_explorer,
    std::unique_ptr<signature_verifier> verifier,
    std::string project,
    size_t minimum_participants,
    bool check_all_available = true);

  void check(const crypto::hash &hash, const std::string &version, const std::string &build_tag) const;
  void check(const std::string &hash, const std::string &version, const std::string &build_tag) const;

private:
  const bool m_check_all_available;
  const std::unique_ptr<repo_explorer> m_explorer;
  const size_t m_minimum_participants;
  const std::string m_project;
  const std::unique_ptr<signature_verifier> m_verifier;
};

class gitlab_repo : public repo_explorer
{
public:
  gitlab_repo(
    const std::string &host,
    uint16_t port,
    const std::string &owner,
    const std::string &repo,
    std::unique_ptr<epee::net_utils::http::abstract_http_client> client,
    std::chrono::seconds timeout,
    std::string user_agent);

  virtual std::string get_file_contents(const std::string &path) const override;
  virtual std::unordered_set<std::string> list_directories(const std::string &path) const override;

private:
  const std::string m_base_url;
  const std::unique_ptr<epee::net_utils::http::abstract_http_client> m_http;
  const std::chrono::seconds m_timeout;
  const std::string m_user_agent;
};

} // namespace tools
