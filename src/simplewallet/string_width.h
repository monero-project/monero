// Copyright (c) 2026, The Monero Project
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

#include <string>
#include <utility>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <utf8proc.h>

#include "misc_log_ex.h"

namespace tools
{
  // Returns a prefix of s, cut at a UTF-8 codepoint boundary, whose display width fits within columns,
  // along with that prefix's display width (which may differ from its length in bytes or codepoints).
  inline std::pair<std::string, size_t> get_string_prefix_by_width(const std::string &s, size_t columns)
  {
    std::string sc = "";
    const utf8proc_uint8_t *ptr = reinterpret_cast<const utf8proc_uint8_t*>(s.data());
    utf8proc_ssize_t avail = s.size();
    size_t sw = 0;
    while (avail > 0)
    {
      utf8proc_int32_t cp = 0;
      utf8proc_ssize_t consumed = utf8proc_iterate(ptr, avail, &cp);
      if (consumed <= 0)
      {
        MERROR("Invalid UTF-8");
        return std::make_pair(s, s.size());
      }

      int cpw = utf8proc_charwidth(cp);
      if (cpw > 0)
      {
        if (cpw > (int)columns)
          break;
        columns -= cpw;
        sw += cpw;
      }

      sc.append(reinterpret_cast<const char*>(ptr), consumed);
      ptr += consumed;
      avail -= consumed;
    }
    return std::make_pair(sc, sw);
  }

  // The display width of s (i.e. the number of terminal columns it occupies).
  inline size_t get_string_width(const std::string &s)
  {
    return get_string_prefix_by_width(s, 999999999).second;
  }

  inline std::vector<std::pair<std::string, size_t>> split_line_by_width(const std::string &s, size_t columns)
  {
    std::vector<std::string> words;
    std::vector<std::pair<std::string, size_t>> lines;
    boost::split(words, s, boost::is_any_of(" "), boost::token_compress_on);
    // split large "words"
    for (size_t i = 0; i < words.size(); ++i)
    {
      for (;;)
      {
        std::string prefix = get_string_prefix_by_width(words[i], columns).first;
        if (prefix.empty() && !words[i].empty())
        {
          utf8proc_int32_t cp = 0;
          utf8proc_ssize_t consumed = utf8proc_iterate(reinterpret_cast<const utf8proc_uint8_t*>(words[i].data()), words[i].size(), &cp);
          prefix = words[i].substr(0, consumed > 0 ? consumed : 1);
        }
        if (prefix == words[i])
          break;
        words[i] = words[i].substr(prefix.size());
        words.insert(words.begin() + i, prefix);
      }
    }

    lines.push_back(std::make_pair("", 0));
    while (!words.empty())
    {
      const size_t word_len = get_string_width(words.front());
      size_t line_len = get_string_width(lines.back().first);
      if (line_len > 0 && line_len + 1 + word_len > columns)
      {
        lines.push_back(std::make_pair("", 0));
        line_len = 0;
      }
      if (line_len > 0)
      {
        lines.back().first += " ";
        lines.back().second++;
      }
      lines.back().first += words.front();
      lines.back().second += word_len;
      words.erase(words.begin());
    }
    return lines;
  }

  inline std::vector<std::pair<std::string, size_t>> split_string_by_width(const std::string &s, size_t columns)
  {
    std::vector<std::string> lines;
    std::vector<std::pair<std::string, size_t>> all_lines;
    boost::split(lines, s, boost::is_any_of("\n"), boost::token_compress_on);
    for (const auto &e: lines)
    {
      std::vector<std::pair<std::string, size_t>> new_lines = split_line_by_width(e, columns);
      for (auto &l: new_lines)
        all_lines.push_back(std::move(l));
    }
    return all_lines;
  }
}
